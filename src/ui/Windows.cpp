#include "ui/Windows.h"
#include "ui/Headers.h"
#include "ui/Helpers.h"
#include "ui/ImGuiCustomControls.h"
#include "ui/Popups.h"
#include "ui/Widgets.h"

#include "common/MathFuncs.h"
#include "image/Image.h"
#include "logic/app/Data.h"

#include <IconFontCppHeaders/IconsForkAwesome.h>

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/color_space.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <string>


namespace
{

static const ImVec2 sk_toolbarButtonSize( 32, 32 );
static const ImVec2 sk_smallToolbarButtonSize( 24, 24 );

/// Color of the reference image header
//static const ImVec4 imgRefHeaderColor( 0.70f, 0.075f, 0.03f, 1.00f );
static const ImVec4 imgRefHeaderColor( 0.20f, 0.41f, 0.68f, 1.00f );

/// Color of the image header
static const ImVec4 imgHeaderColor( 0.20f, 0.41f, 0.68f, 1.00f );

/// Color of the active image header
static const ImVec4 imgActiveHeaderColor( 0.20f, 0.62f, 0.45f, 1.00f );

} // anonymous


void renderViewSettingsComboWindow(
        const uuids::uuid& viewOrLayoutUid,

        const std::pair< glm::vec2, glm::vec2 >& winMouseMinMaxCoords,
        const UiControls& uiControls,
        bool hasFrameAndBackground,
        bool showApplyToAllButton,

        const std::function< size_t(void) >& getNumImages,

        const std::function< bool( size_t index ) >& isImageRendered,
        const std::function< void( size_t index, bool visible ) >& setImageRendered,

        const std::function< bool( size_t index ) >& isImageUsedForMetric,
        const std::function< void( size_t index, bool visible ) >& setImageUsedForMetric,

        const std::function< std::pair<const char*, const char* >( size_t index ) >& getImageDisplayAndFileName,
        const std::function< bool( size_t imageIndex ) >& getImageVisibilitySetting,

        const camera::CameraType& cameraType,
        const camera::ViewRenderMode& shaderType,

        const std::function< void ( const camera::CameraType& cameraType ) >& setCameraType,
        const std::function< void ( const camera::ViewRenderMode& shaderType ) >& setRenderMode,
        const std::function< void () >& recenter,

        const std::function< void ( const uuids::uuid& viewUid ) >& applyImageSelectionAndShaderToAllViews )
{
    static const glm::vec2 sk_framePad{ 4.0f, 4.0f };
    static const ImVec2 sk_windowPadding( 0.0f, 0.0f );
    static const float sk_windowRounding( 0.0f );
    static const ImVec2 sk_itemSpacing( 4.0f, 4.0f );

    const std::string uidString = std::string( "##" ) + uuids::to_string( viewOrLayoutUid );

    // This needs to be saved somewhere
    bool windowOpen = false;

    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, sk_windowPadding );
    ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, sk_itemSpacing );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, sk_windowRounding );
    {
        const char* label;

        switch ( shaderType )
        {
        case camera::ViewRenderMode::Image:
        {
            label = ICON_FK_EYE;
            break;
        }
        case camera::ViewRenderMode::Quadrants:
        case camera::ViewRenderMode::Checkerboard:
        case camera::ViewRenderMode::Flashlight:
        case camera::ViewRenderMode::Overlay:
        case camera::ViewRenderMode::Difference:
        case camera::ViewRenderMode::CrossCorrelation:
        case camera::ViewRenderMode::JointHistogram:
        {
            label = ICON_FK_EYE;
            break;
        }
        case camera::ViewRenderMode::Disabled:
        default:
        {
            label = ICON_FK_EYE_SLASH;
            break;
        }
        }

        const glm::vec2 topLeft = winMouseMinMaxCoords.first + sk_framePad;
        ImGui::SetNextWindowPos( ImVec2( topLeft.x, topLeft.y ), ImGuiCond_Always );

        static const ImGuiWindowFlags sk_defaultWindowFlags =
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoFocusOnAppearing |
                ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGuiWindowFlags windowFlags = sk_defaultWindowFlags;

        if ( ! hasFrameAndBackground )
        {
            windowFlags |= ImGuiWindowFlags_NoBackground;
        }

        ImGui::SetNextWindowBgAlpha( 0.3f );

        ImGui::PushID( uidString.c_str() ); /*** ID = uidString ***/

        // Windows still need a unique ID set in title (with ##ID) despite having pushed an ID on the stack
        if ( ImGui::Begin( uidString.c_str(), &windowOpen, windowFlags ) )
        {
            // Popup window with images to be rendered and their visibility:
            if ( uiControls.m_hasImageComboBox )
            {
                if ( camera::ViewRenderMode::Image == shaderType )
                {
                    // Image visibility:
                    if ( ImGui::Button( label ) )
                    {
                        ImGui::OpenPopup( "imageVisibilityPopup" );
                    }

                    if ( ImGui::IsItemHovered() )
                    {
                        static const std::string sk_selectImages( "Select visible images" );
                        ImGui::SetTooltip( "%s", ( sk_selectImages ).c_str() );
                    }

                    if ( ImGui::BeginPopup( "imageVisibilityPopup" ) )
                    {
                        ImGui::Text( "Visible images:" );

                        for ( size_t i = 0; i < getNumImages(); ++i )
                        {
                            ImGui::PushID( static_cast<int>( i ) ); /*** ID = i ***/

                            auto displayAndFileName = getImageDisplayAndFileName(i);

                            std::string displayName = displayAndFileName.first;
                            if ( false == getImageVisibilitySetting( i ) )
                            {
                                displayName += " (hidden)";
                            }

                            bool rendered = isImageRendered( i );
                            const bool oldRendered = rendered;

                            ImGui::MenuItem( displayName.c_str(), "", &rendered );

                            if ( oldRendered != rendered )
                            {
                                setImageRendered( i, rendered );
                            }

                            if ( ImGui::IsItemHovered() )
                            {
                                ImGui::SetTooltip( "%s", displayAndFileName.second );
                            }

                            ImGui::PopID(); /*** ID = i ***/
                        }

                        //                            ImGui::Separator();
                        //                            if ( ImGui::MenuItem( "Apply to all views" ) )
                        //                            {

                        //                            }

                        ImGui::EndPopup();
                    }
                }
                else if ( camera::ViewRenderMode::Disabled == shaderType )
                {
                    ImGui::Button( label );
                }
                else
                {
                    // Image choice for the metric calculation:

                    if ( ImGui::Button( label ) )
                    {
                        ImGui::OpenPopup( "metricVisibilityPopup" );
                    }

                    if ( ImGui::IsItemHovered() )
                    {
                        static const std::string sk_selectImages( "Select images to compare" );
                        ImGui::SetTooltip( "%s", ( sk_selectImages ).c_str() );
                    }

                    if ( ImGui::BeginPopup( "metricVisibilityPopup" ) )
                    {
                        ImGui::Text( "Compared images:" );

                        for ( size_t i = 0; i < getNumImages(); ++i )
                        {
                            ImGui::PushID( static_cast<int>( i ) ); /*** ID = i ***/

                            const auto displayAndFileName = getImageDisplayAndFileName(i);

                            std::string displayName = displayAndFileName.first;
                            if ( false == getImageVisibilitySetting( i ) )
                            {
                                displayName += " (hidden)";
                            }

                            bool rendered = isImageUsedForMetric( i );
                            const bool oldRendered = rendered;

                            ImGui::MenuItem( displayName.c_str(), "", &rendered );

                            if ( oldRendered != rendered )
                            {
                                setImageUsedForMetric( i, rendered );
                            }

                            if ( ImGui::IsItemHovered() )
                            {
                                ImGui::SetTooltip( "%s", displayAndFileName.second );
                            }

                            ImGui::PopID();  /*** ID = i ***/
                        }

                        //                            ImGui::Separator();
                        //                            if ( ImGui::MenuItem( "Apply to all views" ) )
                        //                            {

                        //                            }

                        ImGui::EndPopup();
                    }
                }
            }


            // Shader type combo box:
            if ( uiControls.m_hasShaderTypeComboBox )
            {
                ImGui::SameLine();
                ImGui::PushItemWidth( 36.0f + 2.0f * ImGui::GetStyle().FramePadding.x );

                if ( ImGui::BeginCombo( "##shaderTypeCombo", ICON_FK_TELEVISION ) )
                {
                    if ( getNumImages() > 1 )
                    {
                        // If there are two or more images, all shader types can be used:
                        for ( const auto& st : camera::AllViewRenderModes )
                        {
                            const bool isSelected = ( st == shaderType );

                            if ( ImGui::Selectable( camera::typeString( st ).c_str(), isSelected ) )
                            {
                                setRenderMode( st );
                            }

                            if ( isSelected )
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                    }
                    else if ( 1 == getNumImages() )
                    {
                        // If there is only one image, then only non-metric shader types can be used:
                        for ( const auto& st : camera::AllNonMetricRenderModes )
                        {
                            const bool isSelected = ( st == shaderType );

                            if ( ImGui::Selectable( camera::typeString( st ).c_str(), isSelected ) )
                            {
                                setRenderMode( st );
                            }

                            if ( isSelected )
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                    }

                    //                        ImGui::Separator();
                    //                        if ( ImGui::Selectable( "Apply to all views", false ) )
                    //                        {

                    //                        }

                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();

                if ( ImGui::IsItemHovered() )
                {
                    static const std::string sk_viewTypeString( "View type: " );
                    ImGui::SetTooltip( "%s", ( sk_viewTypeString + camera::descriptionString( shaderType ) ).c_str() );
                }
            }


            if ( showApplyToAllButton )
            {
                ImGui::SameLine();
                if ( ImGui::Button( ICON_FK_RSS ) )
                {
                    // Apply image and shader settings to all views in this layout
                    applyImageSelectionAndShaderToAllViews( viewOrLayoutUid );

                }
                if ( ImGui::IsItemHovered() )
                {
                    ImGui::SetTooltip( "%s", "Apply this view's image selection and view type to the entire layout" );
                }
            }


            // Camera type combo box (with preview text):
            if ( uiControls.m_hasCameraTypeComboBox )
            {
                ImGui::SameLine();
                ImGui::PushItemWidth( 90.0f + 2.0f * ImGui::GetStyle().FramePadding.x );
                if ( ImGui::BeginCombo( "##cameraTypeCombo", camera::typeString( cameraType ).c_str() ) )
                {
                    for ( const auto& ct : camera::AllCameraTypes )
                    {
                        const bool isSelected = ( ct == cameraType );

                        if ( ImGui::Selectable( camera::typeString( ct ).c_str(), isSelected ) )
                        {
                            setCameraType( ct );
                            recenter();
                        }

                        if ( isSelected )
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();
            }

            // Text label of visible images:
            /// @todo Replace this with NanoVG text?
            {
                std::string imageNamesText;

                bool first = true; // The first image gets no comma in front of it

                if ( camera::ViewRenderMode::Image == shaderType )
                {
                    for ( size_t i = 0; i < getNumImages(); ++i )
                    {
                        if ( isImageRendered( i ) && getImageVisibilitySetting( i ) )
                        {
                            const std::string comma = ( first ? "" : ", " );
                            imageNamesText += comma + std::string( getImageDisplayAndFileName(i).first );
                            first = false;
                        }
                    }
                }
                else if ( camera::ViewRenderMode::Disabled == shaderType )
                {
                    // render no text
                    imageNamesText = "";
                }
                else
                {
                    for ( size_t i = 0; i < getNumImages(); ++i )
                    {
                        if ( isImageUsedForMetric( i ) && getImageVisibilitySetting( i ) )
                        {
                            const std::string comma = ( first ? "" : ", " );
                            imageNamesText += comma + std::string( getImageDisplayAndFileName(i).first );
                            first = false;
                        }
                    }
                }

                static const ImVec4 s_textColor( 0.75f, 0.75f, 0.75f, 1.0f );
                ImGui::TextColored( s_textColor, "%s", imageNamesText.c_str() );
            }

            ImGui::End(); // window
        }

        ImGui::PopID(); /*** ID = uidString.c_str() ***/
    }

    // ImGuiStyleVar_WindowPadding, ImGuiStyleVar_ItemSpacing, ImGuiStyleVar_WindowRounding
    ImGui::PopStyleVar( 3 );
}


void renderImagePropertiesWindow(
        AppData& appData,
        const std::function< size_t (void) >& getNumImages,
        const std::function< std::pair<const char*, const char* >( size_t index ) >& getImageDisplayAndFileName,
        const std::function< size_t (void) >& getActiveImageIndex,
        const std::function< void (size_t) >& setActiveImageIndex,
        const std::function< size_t (void) >& getNumImageColorMaps,
        const std::function< const ImageColorMap* ( size_t cmapIndex ) >& getImageColorMap,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageBackward,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageForward,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageToBack,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageToFront,
        const std::function< void ( const uuids::uuid& imageUid ) >& updateImageUniforms,
        const std::function< void ( const uuids::uuid& imageUid ) >& updateImageInterpolationMode,
        const std::function< bool ( const uuids::uuid& imageUid, bool locked ) >& setLockManualImageTransformation )
{
    if ( ImGui::Begin( "Images##Images",
                       &( appData.guiData().m_showImagePropertiesWindow ),
                       ImGuiWindowFlags_AlwaysAutoResize ) )
    {
        renderActiveImageSelectionCombo(
                    getNumImages,
                    getImageDisplayAndFileName,
                    getActiveImageIndex,
                    setActiveImageIndex );

        ImGui::Separator();

        size_t imageIndex = 0;
        const auto activeUid = appData.activeImageUid();

        for ( const auto& imageUid : appData.imageUidsOrdered() )
        {
            if ( Image* image = appData.image( imageUid ) )
            {
                const bool isActiveImage = activeUid && ( imageUid == *activeUid );

                renderImageHeader(
                            appData,
                            appData.guiData(),
                            imageUid,
                            imageIndex++,
                            image,
                            isActiveImage,
                            appData.numImages(),
                            [&imageUid, updateImageUniforms] () { updateImageUniforms( imageUid ); },
                            [&imageUid, updateImageInterpolationMode] () { updateImageInterpolationMode( imageUid ); },
                            getNumImageColorMaps,
                            getImageColorMap,
                            moveImageBackward,
                            moveImageForward,
                            moveImageToBack,
                            moveImageToFront,
                            setLockManualImageTransformation );
            }
        }

        //        const double frameRate = static_cast<double>( ImGui::GetIO().Framerate );
        //        ImGui::Text( "Rendering: %.3f ms/frame (%.1f FPS)", 1000.0 / frameRate, frameRate );

        ImGui::End();
    }
}


void renderSegmentationPropertiesWindow(
        AppData& appData,
        const std::function< ParcellationLabelTable* ( size_t tableIndex ) >& getLabelTable,
        const std::function< void ( const uuids::uuid& imageUid ) >& updateImageUniforms,
        const std::function< void ( size_t labelColorTableIndex ) >& updateLabelColorTableTexture,
        const std::function< std::optional<uuids::uuid> ( const uuids::uuid& matchingImageUid, const std::string& segDisplayName ) >& createBlankSeg,
        const std::function< bool ( const uuids::uuid& segUid ) >& clearSeg,
        const std::function< bool( const uuids::uuid& segUid ) >& removeSeg )
{
    if ( ImGui::Begin( "Segmentations##Segmentations",
                       &( appData.guiData().m_showSegmentationsWindow ),
                       ImGuiWindowFlags_AlwaysAutoResize ) )
    {
        size_t imageIndex = 0;
        const auto activeUid = appData.activeImageUid();

        for ( const auto& imageUid : appData.imageUidsOrdered() )
        {
            if ( Image* image = appData.image( imageUid ) )
            {
                const bool isActiveImage = activeUid && ( imageUid == *activeUid );

                renderSegmentationHeader(
                            appData,
                            imageUid,
                            imageIndex++,
                            image,
                            isActiveImage,
                            [&imageUid, updateImageUniforms] () { updateImageUniforms( imageUid ); },
                            getLabelTable,
                            updateLabelColorTableTexture,
                            createBlankSeg,
                            clearSeg,
                            removeSeg );
            }
        }

        ImGui::End();
    }
}


void renderLandmarkPropertiesWindow(
        AppData& appData,
        const std::function< void ( const uuids::uuid& viewUid ) >& /*recenterView*/,
        const std::function< void ( bool recenterOnCurrentCrosshairsPosition ) >& recenterCurrentViews )
{
    if ( ImGui::Begin( "Landmarks",
                       &( appData.guiData().m_showLandmarksWindow ),
                       ImGuiWindowFlags_AlwaysAutoResize ) )
    {
        size_t imageIndex = 0;
        const auto activeUid = appData.activeImageUid();

        for ( const auto& imageUid : appData.imageUidsOrdered() )
        {
            const bool isActiveImage = activeUid && ( imageUid == *activeUid );

            renderLandmarkGroupHeader(
                        appData,
                        imageUid,
                        imageIndex++,
                        isActiveImage,
                        recenterCurrentViews );
        }

        ImGui::End();
    }
}


void renderSettingsWindow(
        AppData& appData,
        const std::function< size_t (void) >& getNumImageColorMaps,
        const std::function< const ImageColorMap* ( size_t cmapIndex ) >& getImageColorMap,
        const std::function< void(void) >& updateMetricUniforms,
        const std::function< void(void) >& recenterViews )
{
    static const float sk_windowMin = 0.0f;
    static const float sk_windowMax = 1.0f;

    static const ImGuiColorEditFlags sk_colorEditFlags =
            ImGuiColorEditFlags_NoInputs |
            ImGuiColorEditFlags_PickerHueBar |
            ImGuiColorEditFlags_DisplayRGB |
            ImGuiColorEditFlags_DisplayHex |
            ImGuiColorEditFlags_Uint8 |
            ImGuiColorEditFlags_InputRGB;

    static const ImGuiColorEditFlags sk_colorAlphaEditFlags =
            ImGuiColorEditFlags_NoInputs |
            ImGuiColorEditFlags_PickerHueBar |
            ImGuiColorEditFlags_DisplayRGB |
            ImGuiColorEditFlags_DisplayHex |
            ImGuiColorEditFlags_AlphaBar |
            ImGuiColorEditFlags_AlphaPreviewHalf |
            ImGuiColorEditFlags_Uint8 |
            ImGuiColorEditFlags_InputRGB;

    auto renderMetricSettingsTab = [&updateMetricUniforms, &getNumImageColorMaps, &getImageColorMap]
            ( RenderData::MetricParams& metricParams, bool& showColormapWindow, const char* name )
    {
        // Metric windowing range slider:
        const float slope = metricParams.m_slopeIntercept[0];
        const float xcept = metricParams.m_slopeIntercept[1];

        const float windowWidth = glm::clamp( 1.0f / slope, 0.0f, 1.0f );
        const float windowCenter = glm::clamp( ( 0.5f - xcept ) / slope, 0.0f, 1.0f );

        float windowLow = std::max( windowCenter - 0.5f * windowWidth, 0.0f );
        float windowHigh = std::min( windowCenter + 0.5f * windowWidth, 1.0f );

        if ( ImGui::DragFloatRange2( "Window", &windowLow, &windowHigh, 0.01f,
                                     sk_windowMin, sk_windowMax,
                                     "Min: %.2f", "Max: %.2f", ImGuiSliderFlags_AlwaysClamp ) )
        {
            if ( ( windowHigh - windowLow ) < 0.01f )
            {
                if ( windowLow >= 0.99f ) {
                    windowLow = windowHigh - 0.01f;
                }
                else {
                    windowHigh = windowLow + 0.01f;
                }
            }

            const float newWidth = windowHigh - windowLow;
            const float newCenter = 0.5f * ( windowLow + windowHigh );

            const float newSlope = 1.0f / newWidth;
            const float newXcept = 0.5f - newCenter * newSlope;

            metricParams.m_slopeIntercept = glm::vec2{ newSlope, newXcept };
            updateMetricUniforms();
        }
        ImGui::SameLine();
        helpMarker( "Minimum and maximum of the metric window range" );


        // Metric masking:
        bool doMasking = metricParams.m_doMasking;
        if ( ImGui::Checkbox( "Masking", &doMasking ) )
        {
            metricParams.m_doMasking = doMasking;
            updateMetricUniforms();
        }
        ImGui::SameLine();
        helpMarker( "Only compute the metric within masked regions" );


        // Metric colormap dialog:
        bool* showImageColormapWindow = &showColormapWindow;
        *showImageColormapWindow |= ImGui::Button( "Colormap" );

        bool invertedCmap = metricParams.m_invertCmap;

        ImGui::SameLine();
        if ( ImGui::Checkbox( "Inverted", &invertedCmap ) )
        {
            metricParams.m_invertCmap = invertedCmap;
            updateMetricUniforms();
        }
        ImGui::SameLine();
        helpMarker( "Select/invert the metric colormap" );


        auto getColormapIndex = [&metricParams] () { return metricParams.m_colorMapIndex; };
        auto setColormapIndex = [&metricParams] ( size_t cmapIndex ) { metricParams.m_colorMapIndex = cmapIndex; };

        renderPaletteWindow(
                    std::string( "Select colormap for metric image" ).c_str(),
                    showImageColormapWindow,
                    getNumImageColorMaps,
                    getImageColorMap,
                    getColormapIndex,
                    setColormapIndex,
                    updateMetricUniforms );


        // Colormap preview:
        const float contentWidth = ImGui::GetContentRegionAvail().x;
        const float height = ( ImGui::GetIO().Fonts->Fonts[0]->FontSize * ImGui::GetIO().FontGlobalScale );

        char label[128];
        const ImageColorMap* cmap = getImageColorMap( getColormapIndex() );
        sprintf( label, "%s##cmap_%s", cmap->name().c_str(), name );

        ImGui::paletteButton(
                    label, cmap->numColors(), cmap->data_RGBA_F32(),
                    metricParams.m_invertCmap,
                    ImVec2( contentWidth, height ) );

        if ( ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "%s", cmap->description().c_str() );
        }
    };


    if ( ImGui::Begin( "Settings",
                       &( appData.guiData().m_showSettingsWindow ),
                       ImGuiWindowFlags_AlwaysAutoResize ) )
    {
        static const ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;

        if ( ImGui::BeginTabBar( "##SettingsTabs", tab_bar_flags ) )
        {
            if ( ImGui::BeginTabItem( "Views" ) )
            {
                ImGui::ColorEdit3( "Background color",
                                   glm::value_ptr( appData.renderData().m_backgroundColor ),
                                   sk_colorEditFlags );

                ImGui::ColorEdit4( "Crosshairs color",
                                   glm::value_ptr( appData.renderData().m_crosshairsColor ),
                                   sk_colorAlphaEditFlags );

                ImGui::ColorEdit4( "Anatomical label color",
                                   glm::value_ptr( appData.renderData().m_anatomicalLabelColor ),
                                   sk_colorAlphaEditFlags );

                // Show image-view intersection border
                ImGui::Checkbox( "Show image borders",
                                 &( appData.renderData().m_globalSliceIntersectionParams.renderImageViewIntersections ) );
                ImGui::SameLine();
                helpMarker( "Show borders of image intersections with views" );


                /// @note strokeWidth seems to not work with NanoVG across all platforms
                /*
                if ( appData.renderData().m_globalSliceIntersectionParams.renderImageViewIntersections )
                {
                    constexpr float k_minWidth = 1.0f;
                    constexpr float k_maxWidth = 5.0f;

                    ImGui::SliderScalar( "Border width", ImGuiDataType_Float,
                                         &appData.renderData().m_globalSliceIntersectionParams.strokeWidth,
                                         &k_minWidth, &k_maxWidth, "%.1f" );

                    ImGui::SameLine(); helpMarker( "Border width" );
                }
                */


                // Crosshair snapping
                ImGui::Checkbox( "Snap crosshairs to voxels",
                                 &( appData.renderData().m_snapCrosshairsToReferenceVoxels ) );
                ImGui::SameLine(); helpMarker( "Snap crosshairs to reference image voxel centers" );


                // Image masking
                ImGui::Checkbox( "Mask images by segmentation", &( appData.renderData().m_maskedImages ) );
                ImGui::SameLine(); helpMarker( "Render images only in regions masked by a segmentation label" );

                // Modulate opacity of segmentation with opacity of image:
                ImGui::Checkbox( "Modulate seg. with image opacity", &appData.renderData().m_modulateSegOpacityWithImageOpacity );
                ImGui::SameLine(); helpMarker( "Modulate opacity of segmentation with opacity of image" );

                ImGui::Spacing();
                ImGui::Dummy( ImVec2( 0.0f, 1.0f ) );


                // View centering:
                if ( ImGui::TreeNode( "View Recentering" ) )
                {
                    ImGui::Text( "Center views and crosshairs on:" );

                    ImGui::SameLine();
                    helpMarker( "Default view and crosshairs centering behavior" );

                    if ( ImGui::RadioButton(
                             "Reference image",
                             ImageSelection::ReferenceImage == appData.state().recenteringMode() ) )
                    {
                        appData.state().setRecenteringMode( ImageSelection::ReferenceImage );
                        recenterViews();
                    }
                    ImGui::SameLine(); helpMarker( "Recenter views and crosshairs on the reference image" );

                    if ( ImGui::RadioButton(
                             "Active image",
                             ImageSelection::ActiveImage == appData.state().recenteringMode() ) )
                    {
                        appData.state().setRecenteringMode( ImageSelection::ActiveImage );
                        recenterViews();
                    }
                    ImGui::SameLine(); helpMarker( "Recenter views and crosshairs on the active image" );

                    if ( ImGui::RadioButton(
                             "Reference and active images",
                             ImageSelection::ReferenceAndActiveImages == appData.state().recenteringMode() ) )
                    {
                        appData.state().setRecenteringMode( ImageSelection::ReferenceAndActiveImages );
                        recenterViews();
                    }
                    ImGui::SameLine(); helpMarker( "Recenter views and crosshairs on the reference and active images" );

                    /// @todo These don't work yet
                    /*
                    if ( ImGui::RadioButton( "All visible images", ImageSelection::VisibleImagesInView == appData.state().recenteringMode() ) )
                    {
                        appData.state().setRecenteringMode( ImageSelection::VisibleImagesInView );
                        recenterViews();
                    }
                    ImGui::SameLine(); helpMarker( "Recenter views and crosshairs on the visible images in each view" );

                    if ( ImGui::RadioButton( "Fixed image", ImageSelection::FixedImageInView == appData.state().recenteringMode() ) )
                    {
                        appData.state().setRecenteringMode( ImageSelection::FixedImageInView );
                        recenterViews();
                    }
                    ImGui::SameLine(); helpMarker( "Recenter views on the fixed image in each view" );

                    if ( ImGui::RadioButton( "Moving image", ImageSelection::MovingImageInView == appData.state().recenteringMode() ) )
                    {
                        appData.state().setRecenteringMode( ImageSelection::MovingImageInView );
                        recenterViews();
                    }
                    ImGui::SameLine(); helpMarker( "Recenter views on the moving image in each view" );

                    if ( ImGui::RadioButton( "Fixed and moving images", ImageSelection::FixedAndMovingImagesInView == appData.state().recenteringMode() ) )
                    {
                        appData.state().setRecenteringMode( ImageSelection::FixedAndMovingImagesInView );
                        recenterViews();
                    }
                    ImGui::SameLine(); helpMarker( "Recenter views on the fixed and moving images in each view" );
                    */

                    if ( ImGui::RadioButton(
                             "All loaded images",
                             ImageSelection::AllLoadedImages == appData.state().recenteringMode() ) )
                    {
                        appData.state().setRecenteringMode( ImageSelection::AllLoadedImages );
                        recenterViews();
                    }
                    ImGui::SameLine(); helpMarker( "Recenter views and crosshairs on all loaded images" );

                    ImGui::Spacing();

                    ImGui::TreePop();
                }


                ImGui::Separator();
                ImGui::Checkbox( "Show ImGui demo window", &( appData.guiData().m_showDemoWindow ) );

                ImGui::EndTabItem();
            }


            if ( ImGui::BeginTabItem( "Metrics" ) )
            {
                ImGui::PushID( "metrics" ); /*** PushID metrics ***/

                if ( ImGui::TreeNode( "General" ) )
                {
                    // Overlay style:
                    if ( ImGui::RadioButton( "Magenta/cyan", true == appData.renderData().m_overlayMagentaCyan ) )
                    {
                        appData.renderData().m_overlayMagentaCyan = true;
                    }

                    ImGui::SameLine();
                    if ( ImGui::RadioButton( "Red/green overlay", false == appData.renderData().m_overlayMagentaCyan ) )
                    {
                        appData.renderData().m_overlayMagentaCyan = false;
                    }
                    ImGui::SameLine();
                    helpMarker( "Color style for 'overlay' views" );


                    // Quadrants style:
                    const glm::bvec2 Q = appData.renderData().m_quadrants;

                    if ( ImGui::RadioButton( "X", true == ( Q.x && ! Q.y ) ) )
                    {
                        appData.renderData().m_quadrants = glm::bvec2{ true, false };
                    }

                    ImGui::SameLine();
                    if ( ImGui::RadioButton( "Y", true == ( ! Q.x && Q.y ) ) )
                    {
                        appData.renderData().m_quadrants = glm::bvec2{ false, true };
                    }

                    ImGui::SameLine();
                    if ( ImGui::RadioButton( "X and Y comparison", true == ( Q.x && Q.y ) ) )
                    {
                        appData.renderData().m_quadrants = glm::bvec2{ true, true };
                    }

                    ImGui::SameLine();
                    helpMarker( "Comparison directions in 'quadrant' views" );


                    // Checkerboard squares
                    int numSquares = appData.renderData().m_numCheckerboardSquares;
                    if ( ImGui::InputInt( "Checkerboard size", &numSquares ) )
                    {
                        if ( 2 <= numSquares && numSquares <= 2048 )
                        {
                            appData.renderData().m_numCheckerboardSquares = numSquares;
                        }
                    }
                    ImGui::SameLine(); helpMarker( "Number of squares in 'checkerboard' views" );


                    // Flashlight radius
                    const float radius = appData.renderData().m_flashlightRadius;
                    int radiusPercent = static_cast<int>( 100 * radius );
                    constexpr int k_minRadius = 1;
                    constexpr int k_maxRadius = 100;

                    if ( ImGui::SliderScalar( "Flashlight size", ImGuiDataType_S32, &radiusPercent, &k_minRadius, &k_maxRadius, "%d" ) )
                    {
                        appData.renderData().m_flashlightRadius = static_cast<float>( radiusPercent ) / 100.0f;
                    }
                    ImGui::SameLine();
                    helpMarker( "Circle size for 'flashlight' views, as a percentage of the view size" );

                    ImGui::Separator();
                    ImGui::TreePop();
                }


                if ( ImGui::TreeNode( "Difference" ) )
                {
                    ImGui::PushID( "diff" );
                    {
                        // Difference type:
                        if ( ImGui::RadioButton( "Absolute", false == appData.renderData().m_useSquare ) )
                        {
                            appData.renderData().m_useSquare = false;
                        }

                        ImGui::SameLine();
                        if ( ImGui::RadioButton( "Squared difference", true == appData.renderData().m_useSquare ) )
                        {
                            appData.renderData().m_useSquare = true;
                        }
                        ImGui::SameLine();
                        helpMarker( "Compute absolute or squared difference" );

                        renderMetricSettingsTab( appData.renderData().m_squaredDifferenceParams,
                                                 appData.guiData().m_showDifferenceColormapWindow,
                                                 "sqdiff" );
                    }
                    ImGui::PopID(); // "diff"

                    ImGui::Separator();
                    ImGui::TreePop();
                }


                if ( ImGui::TreeNode( "Cross-correlation" ) )
                {
                    ImGui::PushID( "crosscorr" );
                    {
                        renderMetricSettingsTab( appData.renderData().m_crossCorrelationParams,
                                                 appData.guiData().m_showCorrelationColormapWindow,
                                                 "crosscorr" );
                    }
                    ImGui::PopID(); // "crosscorr"

                    ImGui::Separator();
                    ImGui::TreePop();
                }


                ImGui::PopID(); /*** PopID metrics ***/
                ImGui::EndTabItem();
            }


            if ( ImGui::BeginTabItem( "Landmarks" ) )
            {
                ImGui::PushID( "landmarks" ); /*** PushID landmarks ***/

                bool onTop = appData.renderData().m_globalLandmarkParams.renderOnTopOfAllImagePlanes;
                if ( ImGui::Checkbox( "Landmarks on top", &onTop ) )
                {
                    appData.renderData().m_globalLandmarkParams.renderOnTopOfAllImagePlanes = onTop;
                }
                ImGui::SameLine();
                helpMarker( "Render landmarks on top of all image layers" );

                ImGui::PopID(); /*** PopID landmarks ***/
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }
}


//enum class PopupWindowPosition
//{
//    Custom,
//    TopLeft,
//    TopRight,
//    BottomLeft,
//    BottomRight
//};


void renderInspectionWindow(
        AppData& appData,
        const std::function< size_t (void) >& getNumImages,
        const std::function< std::pair<const char*, const char* >( size_t index ) >& getImageDisplayAndFileName,
        const std::function< glm::vec3 () >& getWorldDeformedPos,
        const std::function< std::optional<glm::vec3> ( size_t imageIndex ) >& getSubjectPos,
        const std::function< std::optional<glm::ivec3> ( size_t imageIndex ) >& getVoxelPos,
        const std::function< std::optional<double> ( size_t imageIndex ) >& getImageValue,
        const std::function< std::optional<int64_t> ( size_t imageIndex ) >& getSegLabel,
        const std::function< ParcellationLabelTable* ( size_t tableIndex ) >& getLabelTable )
{
    static constexpr size_t sk_refIndex = 0; // Index of the reference image
    static bool s_firstRun = false; // Is this the first run?

    static constexpr float sk_pad = 10.0f;
    static int corner = 2;
//    static PopupWindowPosition pos = PopupWindowPosition::BottomLeft;

    // Show World coordinates?
    static bool s_showWorldCoords = false;

    bool selectionButtonShown = false;

    static const ImVec4 buttonColor( 0.0f, 0.0f, 0.0f, 0.0f );
    static const ImVec4 blueColor( 0.0f, 0.5f, 1.0f, 1.0f );

    // For which images to show coordinates?
    static std::unordered_map<uuids::uuid, bool> s_showSubject;

    if ( s_firstRun )
    {
        // Show the first (reference) image coordinates by default:
        if ( const auto imageUid = appData.imageUid( sk_refIndex ) )
        {
            s_showSubject.insert( { *imageUid, true } );
        }

        s_firstRun = false;
    }


    auto contextMenu = [&getNumImages, &appData, &getImageDisplayAndFileName] ()
    {
        if ( ImGui::BeginMenu( "Show" ) )
        {
            //                if ( ImGui::MenuItem( "World", nullptr, s_showWorldCoords ) )
            //                {
            //                    s_showWorldCoords = ! s_showWorldCoords;
            //                }
            //                if ( ImGui::IsItemHovered() )
            //                {
            //                    ImGui::SetTooltip( "Show World-space crosshairs coordinates" );
            //                }

            for ( size_t imageIndex = 0; imageIndex < getNumImages(); ++imageIndex )
            {
                const auto imageUid = appData.imageUid( imageIndex );
                if ( ! imageUid ) continue;

                bool& visible = s_showSubject[*imageUid];

                const auto names = getImageDisplayAndFileName( imageIndex );

                if ( ImGui::MenuItem( names.first, nullptr, visible ) )
                {
                    visible = ! visible;
                }

                if ( ImGui::IsItemHovered() )
                {
                    ImGui::SetTooltip( "%s", names.second );
                }
            }

            ImGui::EndMenu();
        }

        if ( ImGui::BeginMenu( "Position" ) )
        {
            if ( ImGui::MenuItem( "Custom", nullptr, corner == -1 ) ) corner = -1;
            if ( ImGui::MenuItem( "Top-left", nullptr, corner == 0 ) ) corner = 0;
            if ( ImGui::MenuItem( "Top-right", nullptr, corner == 1 ) ) corner = 1;
            if ( ImGui::MenuItem( "Bottom-left", nullptr, corner == 2 ) ) corner = 2;
            if ( ImGui::MenuItem( "Bottom-right", nullptr, corner == 3 ) ) corner = 3;

            ImGui::EndMenu();
        }

        if ( appData.guiData().m_showInspectionWindow && ImGui::MenuItem( "Close" ) )
        {
            appData.guiData().m_showInspectionWindow = false;
        }

        ImGui::EndPopup();
    };


    auto showSelectionButton = [/*&contextMenu*/] ()
    {
        ImGui::PushStyleColor( ImGuiCol_Button, buttonColor );
        if ( ImGui::Button( ICON_FK_LIST_UL ) )
        {
//            ImGui::OpenPopup( "selectionPopup" );
        }
        ImGui::PopStyleColor( 1 );

        if ( ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "Select image(s) to inspect" );
        }

//        ImGui::BeginPopup( "selectionPopup" );
//        {
//            contextMenu();
//            ImGui::EndPopup();
//        }

        /// @todo This is not correct. We should be using the code above
//        if ( ImGui::BeginPopupContextWindow( "selectionPopup", ImGuiPopupFlags_MouseButtonLeft ) )
//        {
//            contextMenu();
//        }
    };


    ImGuiIO& io = ImGui::GetIO();

    ImGuiWindowFlags windowFlags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav;

    if ( corner != -1 )
    {
        windowFlags |= ImGuiWindowFlags_NoMove;

        const ImVec2 windowPos = ImVec2( ( corner & 1 ) ? io.DisplaySize.x - sk_pad : sk_pad,
                                         ( corner & 2 ) ? io.DisplaySize.y - sk_pad : sk_pad );

        const ImVec2 windowPosPivot = ImVec2( ( corner & 1 ) ? 1.0f : 0.0f,
                                              ( corner & 2 ) ? 1.0f : 0.0f );

        ImGui::SetNextWindowPos( windowPos, ImGuiCond_Always, windowPosPivot );
    }

    ImGui::SetNextWindowBgAlpha( 0.35f ); // Transparent background

    if ( ImGui::Begin( "##InspectionWindow", &( appData.guiData().m_showInspectionWindow ), windowFlags ) )
    {
//        if ( ImGui::IsMousePosValid() && ImGui::IsAnyMouseDown() )
//        {
//            ImGui::Text( "Mouse Position: (%.0f, %.0f)",
//                         static_cast<double>( io.MousePos.x ),
//                         static_cast<double>( io.MousePos.y ) );
//        }

        if ( s_showWorldCoords )
        {
            const glm::vec3 subjectPos = getWorldDeformedPos();

            //            ImGui::Text( "World:" );
            ImGui::Text( "(%.3f, %.3f, %.3f) mm",
                         static_cast<double>( subjectPos.x ),
                         static_cast<double>( subjectPos.y ),
                         static_cast<double>( subjectPos.z ) );

            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "World-space coordinates" );
            }
        }

        bool firstImageShown = true;
        bool showedAtLeastOneImage = false; // is info for at least one image shown?

        for ( size_t imageIndex = 0; imageIndex < getNumImages(); ++imageIndex )
        {
            const auto imageUid = appData.imageUid( imageIndex );
            const Image* image = ( imageUid ? appData.image( *imageUid ) : nullptr );
            if ( ! image ) continue;

            if ( 0 == s_showSubject.count( *imageUid ) )
            {
                // The reference image is shown by default in this list
                s_showSubject.insert( { *imageUid, ( sk_refIndex == imageIndex ) ? true : false } );
            }

            const bool visible = s_showSubject[*imageUid];
            if ( ! visible ) continue;

            showedAtLeastOneImage = true;

            if ( s_showWorldCoords || ! firstImageShown )
            {
                ImGui::Separator();
            }

            firstImageShown = false;

            const auto names = getImageDisplayAndFileName( imageIndex );

            if ( sk_refIndex == imageIndex )
            {
                ImGui::TextColored( blueColor, "%s (ref.):", names.first );
            }
            else
            {
                ImGui::TextColored( blueColor, "%s:", names.first );
            }

            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "%s", names.second );
            }


            /// @todo Do we want to show subject coords for all images?
            if ( sk_refIndex == imageIndex )
            {
                // Show subject coordinates for the reference image only:
                if ( const auto subjectPos = getSubjectPos( imageIndex ) )
                {
                    ImGui::Text( "(%.3f, %.3f, %.3f) mm",
                                 static_cast<double>( subjectPos->x ),
                                 static_cast<double>( subjectPos->y ),
                                 static_cast<double>( subjectPos->z ) );
                }
            }

            if ( const auto voxelPos = getVoxelPos( imageIndex ) )
            {
                ImGui::Text( "(%d, %d, %d) vox", voxelPos->x, voxelPos->y, voxelPos->z );
            }
            else
            {
                ImGui::Text( "<N/A>" );
            }

            if ( const auto imageValue = getImageValue( imageIndex ) )
            {
                if ( isComponentFloatingPoint( image->header().memoryComponentType() ) )
                {
                    if ( image->header().numComponentsPerPixel() > 1 )
                    {
                        ImGui::Text( "Value (comp. %d): %0.3f",
                                     image->settings().activeComponent(), *imageValue );
                    }
                    else
                    {
                        ImGui::Text( "Value: %0.3f", *imageValue );
                    }
                }
                else
                {
                    if ( image->header().numComponentsPerPixel() > 1 )
                    {
                        // Multi-component case: show the value of the active component
                        ImGui::Text( "Value (comp. %d): %d",
                                     image->settings().activeComponent(),
                                     static_cast<int>( *imageValue ) );
                    }
                    else
                    {
                        // Single component case
                        ImGui::Text( "Value: %d", static_cast<int32_t>( *imageValue ) );
                    }
                }
            }

            const auto segUid = appData.imageToActiveSegUid( *imageUid );
            const Image* seg = ( segUid ? appData.seg( *segUid ) : nullptr );
            if ( ! seg ) continue;

            if ( const auto segLabel = getSegLabel( imageIndex ) )
            {
                ImGui::Text( "Label: %ld", static_cast<long>( *segLabel ) );

                const auto* table = getLabelTable( seg->settings().labelTableIndex() );
                if ( table && 0 != *segLabel )
                {
                    const char* labelName = table->getName( static_cast<size_t>( *segLabel ) ).c_str();
                    ImGui::SameLine();
                    ImGui::Text( "(%s)", labelName );
                }
            }

            if ( ! selectionButtonShown )
            {
                ImGui::SameLine( ImGui::GetWindowContentRegionMax().x - 24 );
                showSelectionButton();
                selectionButtonShown = true;
            }
        }

        if ( ! showedAtLeastOneImage )
        {
            showSelectionButton();
        }

        if ( ImGui::BeginPopupContextWindow() )
        {
            contextMenu();
        }
    }

    ImGui::End();
}
