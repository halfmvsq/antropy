#include "ui/Widgets.h"
#include "ui/Helpers.h"
#include "ui/ImGuiCustomControls.h"

#include "common/MathFuncs.h"

#include "image/ImageColorMap.h"
#include "image/ImageTransformations.h"

#include "logic/app/Data.h"

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <IconFontCppHeaders/IconsForkAwesome.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/color_space.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <string>


void renderActiveImageSelectionCombo(
        const std::function< size_t (void) >& getNumImages,
        const std::function< std::pair<const char*, const char* >( size_t index ) >& getImageDisplayAndFileName,
        const std::function< size_t (void) >& getActiveImageIndex,
        const std::function< void (size_t) >& setActiveImageIndex,
        bool showText )
{
    const size_t activeIndex = getActiveImageIndex();

    if ( activeIndex >= getNumImages() )
    {
        spdlog::error( "Invalid active image index" );
        return;
    }

    const std::string nameString = ( showText )
            ? "Active image###imageSelectionCombo"
            : "###imageSelectionCombo";

    if ( ImGui::BeginCombo( nameString.c_str(), getImageDisplayAndFileName( activeIndex ).first ) )
    {
        for ( size_t i = 0; i < getNumImages(); ++i )
        {
            const auto displayAndFileName = getImageDisplayAndFileName( i );
            const bool isSelected = ( i == activeIndex );

            ImGui::PushID( i ); // needed in case two images have the same display name
            if ( ImGui::Selectable( displayAndFileName.first, isSelected ) )
            {
                setActiveImageIndex( i );
            }
            ImGui::PopID();

            if ( isSelected )
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::SameLine();
    helpMarker( "Select the image that is being actively transformed, adjusted, or segmented" );
}


void renderSegLabelsChildWindow(
        size_t tableIndex,
        ParcellationLabelTable* labelTable,
        const std::function< void ( size_t tableIndex ) >& updateLabelColorTableTexture )
{
    static const std::string sk_showAll = std::string( ICON_FK_EYE ) + " Show all";
    static const std::string sk_hideAll = std::string( ICON_FK_EYE_SLASH ) + " Hide all";
    static const std::string sk_addNew = std::string( ICON_FK_PLUS ) + " Add new";

    static const ImGuiColorEditFlags sk_colorEditFlags =
            ImGuiColorEditFlags_NoInputs |
            ImGuiColorEditFlags_AlphaPreviewHalf |
            ImGuiColorEditFlags_AlphaBar |
            ImGuiColorEditFlags_Uint8 |
            ImGuiColorEditFlags_DisplayRGB |
            ImGuiColorEditFlags_DisplayHSV |
            ImGuiColorEditFlags_DisplayHex;

    if ( ! labelTable )
    {
        return;
    }

    const bool childVisible = ImGui::BeginChild(
                "##labelChild", ImVec2( 0, 250.0f ), true,
                ImGuiWindowFlags_MenuBar |
                ImGuiWindowFlags_HorizontalScrollbar );

    if ( ! childVisible )
    {
        ImGui::EndChild();
        return;
    }

    if ( ImGui::BeginMenuBar() )
    {
        if ( ImGui::MenuItem( sk_showAll.c_str() ) )
        {
            for ( size_t i = 0; i < labelTable->numLabels(); ++i )
            {
                labelTable->setVisible( i, true );
            }
            updateLabelColorTableTexture( tableIndex );
        }

        if ( ImGui::MenuItem( sk_hideAll.c_str() ) )
        {
            for ( size_t i = 0; i < labelTable->numLabels(); ++i )
            {
                labelTable->setVisible( i, false );
            }
            updateLabelColorTableTexture( tableIndex );
        }

        if ( ImGui::MenuItem( sk_addNew.c_str() ) )
        {
            labelTable->addLabels( 1 );
            updateLabelColorTableTexture( tableIndex );
        }

        ImGui::EndMenuBar();
    }


    for ( size_t i = 0; i < labelTable->numLabels(); ++i )
    {
        char labelIndexBuffer[8];
        sprintf( labelIndexBuffer, "%03lu", i );

        bool labelVisible = labelTable->getVisible( i );
        std::string labelName = labelTable->getName( i );

        // ImGui::ColorEdit represents color as non-pre-multiplied colors
        glm::vec4 labelColor = glm::vec4{ labelTable->getColor( i ),
                labelTable->getAlpha( i ) };

        ImGui::PushID( static_cast<int>( i ) ); /*** PushID i ***/

        if ( ImGui::Checkbox( "##labelVisible", &labelVisible ) )
        {
            labelTable->setVisible( i, labelVisible );
            updateLabelColorTableTexture( tableIndex );
        }

        ImGui::SameLine();
        if ( ImGui::ColorEdit4( labelIndexBuffer,
                                glm::value_ptr( labelColor ),
                                sk_colorEditFlags ) )
        {
            labelTable->setColor( i, glm::vec3{ labelColor } );
            labelTable->setAlpha( i, labelColor.a );
            updateLabelColorTableTexture( tableIndex );
        }

        ImGui::SameLine();
        if ( ImGui::InputText( "##labelName", &labelName ) )
        {
            labelTable->setName( i, labelName );
        }

        ImGui::PopID(); /*** PopID i ***/
    }

    ImGui::EndChild();
}


void renderPaletteWindow(
        const char* name,
        bool* showPaletteWindow,
        const std::function< size_t (void) >& getNumImageColorMaps,
        const std::function< const ImageColorMap* ( size_t cmapIndex ) >& getImageColorMap,
        const std::function< size_t (void) >& getCurrentImageColormapIndex,
        const std::function< void ( size_t cmapIndex ) >& setCurrentImageColormapIndex,
        const std::function< void ( void ) >& updateImageUniforms )
{
    /// @todo model this after the Example: Property editor in ImGui

    static constexpr float sk_labelWidth = 0.25f;
    static constexpr float sk_cmapWidth = 0.75f;

    if ( ! showPaletteWindow || ! *showPaletteWindow ) return;

    ImGui::SetNextWindowSize( ImVec2( 600, 500 ), ImGuiCond_FirstUseEver );

    ImGui::PushID( name ); /*** PushID name ***/

    const bool showWindow = ImGui::Begin( name, showPaletteWindow, ImGuiWindowFlags_NoCollapse );

    if ( ! showWindow )
    {
        ImGui::PopID(); /*** PopID name ***/
        ImGui::End();
        return;
    }

    const auto& io = ImGui::GetIO();
    const auto& style = ImGui::GetStyle();

    // const float border = style.FramePadding.x;
    const float contentWidth = ImGui::GetContentRegionAvail().x;
    const float height = ( io.Fonts->Fonts[0]->FontSize * io.FontGlobalScale ) - style.FramePadding.y;
    const ImVec2 buttonSize( sk_cmapWidth * contentWidth, height );

    ImGui::Columns( 2, "Colormaps", false );
    ImGui::SetColumnWidth( 0, sk_labelWidth * contentWidth );

    for ( size_t i = 0; i < getNumImageColorMaps(); ++i )
    {
        ImGui::PushID( static_cast<int>( i ) );
        {
            const ImageColorMap* cmap = getImageColorMap( i );
            if ( ! cmap ) continue;

            //                    ImGui::SetCursorPosX( border );
            //                    ImGui::AlignTextToFramePadding();

            if ( ImGui::Selectable( cmap->name().c_str(),
                                    ( getCurrentImageColormapIndex() == i ),
                                    ImGuiSelectableFlags_SpanAllColumns ) )
            {
                setCurrentImageColormapIndex( i );
                updateImageUniforms();
            }

            ImGui::NextColumn();
            ImGui::paletteButton( cmap->name().c_str(),
                                  cmap->numColors(),
                                  cmap->data_RGBA_F32(),
                                  false,
                                  buttonSize );

            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "%s", cmap->description().c_str() );
            }

            ImGui::NextColumn();
        }
        ImGui::PopID(); // i
    }

    ImGui::End();

    ImGui::PopID(); /*** PopID name ***/
}


void renderLandmarkChildWindow(
        const ImageTransformations& imageTransformations,
        LandmarkGroup* activeLmGroup,
        const glm::vec3& worldCrosshairsPos,
        const std::function< void ( const glm::vec3& worldCrosshairsPos ) >& setWorldCrosshairsPos,
        const std::function< void ( bool recenterOnCurrentCrosshairsPosition ) >& recenterAllViewsOnCurrentCrosshairsPosition )
{
    static const std::string sk_showAll = std::string( ICON_FK_EYE ) + " Show all";
    static const std::string sk_hideAll = std::string( ICON_FK_EYE_SLASH ) + " Hide all";
    static const std::string sk_addNew = std::string( ICON_FK_PLUS ) + " Add new";

    static const ImGuiColorEditFlags sk_colorEditFlags =
            ImGuiColorEditFlags_NoInputs |
            ImGuiColorEditFlags_PickerHueBar |
            ImGuiColorEditFlags_DisplayRGB |
            ImGuiColorEditFlags_DisplayHSV |
            ImGuiColorEditFlags_DisplayHex |
            ImGuiColorEditFlags_Uint8 |
            ImGuiColorEditFlags_InputRGB;

    static const auto sk_hueMinMax = std::make_pair( 0.0f, 360.0f );
    static const auto sk_satMinMax = std::make_pair( 0.3f, 1.0f );
    static const auto sk_valMinMax = std::make_pair( 0.3f, 1.0f );

    std::map< size_t, PointRecord<glm::vec3> >& points = activeLmGroup->getPoints();

    const bool childVisible = ImGui::BeginChild(
                "", ImVec2( 0, 250 ), true,
                ImGuiWindowFlags_MenuBar |
                ImGuiWindowFlags_HorizontalScrollbar );

    if ( ! childVisible )
    {
        ImGui::EndChild();
        return;
    }

    if ( ImGui::BeginMenuBar() )
    {
        if ( ImGui::MenuItem( sk_showAll.c_str() ) )
        {
            for ( auto& p : points )
            {
                p.second.setVisibility( true );
            }
        }

        if ( ImGui::MenuItem( sk_hideAll.c_str() ) )
        {
            for ( auto& p : points )
            {
                p.second.setVisibility( false );
            }
        }

        if ( ImGui::MenuItem( sk_addNew.c_str() ) )
        {
            // Add new landmark at crosshairs position in the correct space
            const glm::mat4 landmark_T_world = ( activeLmGroup->getInVoxelSpace() )
                    ? imageTransformations.pixel_T_worldDef()
                    : imageTransformations.subject_T_worldDef();

            const glm::vec4 lmPos = landmark_T_world * glm::vec4{ worldCrosshairsPos, 1.0f };

            PointRecord<glm::vec3> pointRec{ glm::vec3{ lmPos / lmPos.w } };

            // Assign the new point a random color, seeded by its index
            const size_t newIndex = ( activeLmGroup->getPoints().empty() )
                    ? 0u : activeLmGroup->maxIndex() + 1;

            const auto colors = math::generateRandomHsvSamples(
                        1, sk_hueMinMax, sk_satMinMax, sk_valMinMax,
                        static_cast<uint32_t>( newIndex ) );

            if ( ! colors.empty() )
            {
                pointRec.setColor( glm::rgbColor( colors[0] ) );
            }

            activeLmGroup->addPoint( newIndex, pointRec );

            /// @todo Scroll child window to the end of the list of landmarks
        }

        ImGui::EndMenuBar();
    }

    char pointIndexBuffer[8];

    for ( auto& p : points )
    {
        const size_t pointIndex = p.first;
        auto& point = p.second;

        sprintf( pointIndexBuffer, "%03lu", pointIndex );

        bool pointVisible = point.getVisibility();
        std::string pointName = point.getName();
        glm::vec3 pointColor = point.getColor();
        glm::vec3 pointPos = point.getPosition();

        ImGui::PushID( static_cast<int>( pointIndex ) ); /*** PushID pointIndex ***/

        if ( ImGui::Checkbox( pointIndexBuffer, &pointVisible ) )
        {
            point.setVisibility( pointVisible );
        }

        if ( ! activeLmGroup->getColorOverride() )
        {
            ImGui::SameLine();
            if ( ImGui::ColorEdit3( "", glm::value_ptr( pointColor ), sk_colorEditFlags ) )
            {
                point.setColor( pointColor );
            }
        }

        ImGui::SameLine();
        if ( ImGui::Button( ICON_FK_HAND_O_UP ) )
        {
            const glm::mat4 world_T_landmark = ( activeLmGroup->getInVoxelSpace() )
                    ? imageTransformations.worldDef_T_pixel()
                    : imageTransformations.worldDef_T_subject();

            const glm::vec4 worldPos = world_T_landmark * glm::vec4{ pointPos, 1.0f };
            setWorldCrosshairsPos( glm::vec3{ worldPos / worldPos.w } );

            // With argument set to true, this function centers all views on the crosshairs.
            // That way, views show the crosshairs even if they were not in the original view bounds.
            recenterAllViewsOnCurrentCrosshairsPosition( true );
        }
        if ( ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "Move crosshairs to landmark and center views on landmark" );
        }

        ImGui::SameLine();
        if ( ImGui::Button( ICON_FK_CROSSHAIRS ) )
        {
            const glm::mat4 landmark_T_world = ( activeLmGroup->getInVoxelSpace() )
                    ? imageTransformations.pixel_T_worldDef()
                    : imageTransformations.subject_T_worldDef();

            const glm::vec4 lmPos = landmark_T_world * glm::vec4{ worldCrosshairsPos, 1.0f };

            point.setPosition( glm::vec3{ lmPos / lmPos.w } );
        }
        if ( ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "Set landmark to the current crosshairs position" );
        }

        ImGui::SameLine();
        if ( ImGui::Button( ICON_FK_TIMES ) )
        {
            if ( activeLmGroup->removePoint( pointIndex ) )
            {
                // The point was removed, so skip rendering it
                ImGui::PopID(); // pointIndex
                ImGui::EndChild();
                return;
            }
        }
        if ( ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "Delete landmark" );
        }

        if ( activeLmGroup->getRenderLandmarkNames() )
        {
            // Edit the name if they are visible
            ImGui::SameLine();
            ImGui::PushItemWidth( 100.0f );
            if ( ImGui::InputText( "##pointName", &pointName ) )
            {
                point.setName( pointName );
            }
            ImGui::PopItemWidth();
            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "Landmark name" );
            }
        }

        ImGui::SameLine();
        ImGui::PushItemWidth( 250.0f );
        if ( ImGui::InputFloat3( "##pointPos", glm::value_ptr( pointPos ), "%.3f", 0 ) )
        {
            point.setPosition( pointPos );
        }
        ImGui::PopItemWidth();

        ImGui::PopID(); /*** PopID pointIndex ***/
    }

    ImGui::EndChild();
}
