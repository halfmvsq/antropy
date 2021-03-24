#include "ui/Windows.h"
#include "ui/Headers.h"
#include "ui/Helpers.h"
#include "ui/ImGuiCustomControls.h"
#include "ui/Popups.h"
#include "ui/Widgets.h"
#include "ui/imgui/imGuIZMO.quat/imGuIZMOquat.h"

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
#include <glm/gtx/component_wise.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <string>


namespace
{

static const ImVec4 sk_whiteText( 1, 1, 1, 1 );
static const ImVec4 sk_blackText( 0, 0, 0, 1 );

static const ImVec2 sk_toolbarButtonSize( 32, 32 );
static const ImVec2 sk_smallToolbarButtonSize( 24, 24 );

static const std::string sk_NA( "<N/A>" );

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
                ImGui::PushItemWidth( 120.0f + 2.0f * ImGui::GetStyle().FramePadding.x );
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
            /// @todo Replace this with NanoVG text
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


void renderViewOrientationToolWindow(
        const uuids::uuid& viewOrLayoutUid,
        const std::pair< glm::vec2, glm::vec2 >& winMouseMinMaxCoords,
        const UiControls& /*uiControls*/,
        bool hasFrameAndBackground,
        const camera::CameraType& cameraType,
        const std::function< glm::quat () >& getViewCameraRotation,
        const std::function< void ( const glm::quat& camera_T_world_rotationDelta ) >& setViewCameraRotation,
        const std::function< glm::vec3 () >& getViewNormal )
{
    static const glm::vec2 sk_framePad{ 4.0f, 4.0f };
    static const ImVec2 sk_windowPadding( 0.0f, 0.0f );
    static const float sk_windowRounding( 0.0f );
    static const ImVec2 sk_itemSpacing( 4.0f, 4.0f );

    static const ImGuiWindowFlags sk_defaultWindowFlags =
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

    static constexpr int sk_corner = 2;

    if ( camera::CameraType::Oblique != cameraType )
    {
        return;
    }

    const std::string uidString = std::string( "OrientationTool##" ) + uuids::to_string( viewOrLayoutUid );

    bool windowOpen = false;

    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, sk_windowPadding );
    ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, sk_itemSpacing );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, sk_windowRounding );

    ImGuiWindowFlags windowFlags = sk_defaultWindowFlags;

    if ( ! hasFrameAndBackground )
    {
        windowFlags |= ImGuiWindowFlags_NoBackground;
    }

    const glm::vec2 bottomLeft(
                winMouseMinMaxCoords.first.x + sk_framePad.x,
                winMouseMinMaxCoords.second.y - sk_framePad.y );

    const ImVec2 windowPosPivot(
                ( sk_corner & 1 ) ? 1.0f : 0.0f,
                ( sk_corner & 2 ) ? 1.0f : 0.0f );

    ImGui::SetNextWindowPos( ImVec2( bottomLeft.x, bottomLeft.y ), ImGuiCond_Always, windowPosPivot );

    ImGui::SetNextWindowBgAlpha( 0.3f );

    ImGui::PushID( uidString.c_str() ); /*** ID = uidString ***/

    if ( ImGui::Begin( uidString.c_str(), &windowOpen, windowFlags ) )
    {
        static constexpr float sk_size = 96.0f;
        static constexpr int sk_mode = ( imguiGizmo::mode3Axes | imguiGizmo::cubeAtOrigin );

        const glm::quat oldQuat = getViewCameraRotation();
        glm::quat newQuat = oldQuat;

        if ( ImGui::gizmo3D( "", newQuat, sk_size, sk_mode ) )
        {
            setViewCameraRotation( newQuat * glm::inverse( oldQuat ) );
        }

        if ( ImGui::IsItemHovered() )
        {
            const glm::vec3 n = getViewNormal();
            ImGui::SetTooltip( "View normal: (%0.3f, %0.3f, %0.3f)\n", n.x, n.y, n.z );
        }

        ImGui::End();
    }

    ImGui::PopID(); /*** ID = uidString.c_str() ***/

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
    static const std::string sk_showOpacityMixer = std::string( ICON_FK_SLIDERS ) + " Show opacity mixer";

    if ( ImGui::Begin( "Images##Images",
                       &( appData.guiData().m_showImagePropertiesWindow ),
                       ImGuiWindowFlags_AlwaysAutoResize ) )
    {
        renderActiveImageSelectionCombo(
                    getNumImages,
                    getImageDisplayAndFileName,
                    getActiveImageIndex,
                    setActiveImageIndex );

        if ( ImGui::Button( sk_showOpacityMixer.c_str() ) )
        {
            appData.guiData().m_showOpacityBlenderWindow = true;
        }

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
        const std::function< void ( bool recenterCrosshairs, bool recenterOnCurrentCrosshairsPosition, bool resetObliqueOrientation ) >& recenterAllViewsOnCurrentCrosshairsPosition )
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
                        recenterAllViewsOnCurrentCrosshairsPosition );
        }

        ImGui::End();
    }
}


void renderAnnotationWindow(
        AppData& appData,
        const std::function< void ( bool recenterCrosshairs, bool recenterOnCurrentCrosshairsPosition, bool resetObliqueOrientation ) >& recenterAllViews )
{
    if ( ImGui::Begin( "Annotations",
                       &( appData.guiData().m_showAnnotationsWindow ),
                       ImGuiWindowFlags_AlwaysAutoResize ) )
    {
        size_t imageIndex = 0;
        const auto activeUid = appData.activeImageUid();

        for ( const auto& imageUid : appData.imageUidsOrdered() )
        {
            const bool isActiveImage = activeUid && ( imageUid == *activeUid );

            renderAnnotationsHeader(
                        appData,
                        imageUid,
                        imageIndex++,
                        isActiveImage,
                        recenterAllViews );
        }

        ImGui::End();
    }
}


void renderSettingsWindow(
        AppData& appData,
        const std::function< size_t (void) >& getNumImageColorMaps,
        const std::function< const ImageColorMap* ( size_t cmapIndex ) >& getImageColorMap,
        const std::function< void(void) >& updateMetricUniforms,
        const std::function< void ( bool recenterCrosshairs, bool recenterOnCurrentCrosshairsPosition, bool resetObliqueOrientation ) >& recenterAllViews )
{
    static constexpr bool sk_recenterCrosshairs = true;
    static constexpr bool sk_doNotRecenterOnCurrentCrosshairsPosition = false;
    static constexpr bool sk_doNotResetObliqueOrientation = false;

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
                                     "Min: %.2f", "Max: %.2f",
                                     ImGuiSliderFlags_AlwaysClamp ) )
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


                // Colors:
                ImGui::SetNextItemOpen( true, ImGuiCond_Appearing );

                if ( ImGui::TreeNode( "Colors" ) )
                {
                    ImGui::ColorEdit3( "Background",
                                       glm::value_ptr( appData.renderData().m_backgroundColor ),
                                       sk_colorEditFlags );

                    ImGui::ColorEdit4( "Crosshairs",
                                       glm::value_ptr( appData.renderData().m_crosshairsColor ),
                                       sk_colorAlphaEditFlags );

                    ImGui::ColorEdit4( "Anatomical labels",
                                       glm::value_ptr( appData.renderData().m_anatomicalLabelColor ),
                                       sk_colorAlphaEditFlags );

                    ImGui::Spacing();
                    ImGui::TreePop();
                }


                // View centering:
                ImGui::SetNextItemOpen( true, ImGuiCond_Appearing );

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

                        recenterAllViews( sk_recenterCrosshairs,
                                          sk_doNotRecenterOnCurrentCrosshairsPosition,
                                          sk_doNotResetObliqueOrientation );
                    }
                    ImGui::SameLine(); helpMarker( "Recenter views and crosshairs on the reference image" );

                    if ( ImGui::RadioButton(
                             "Active image",
                             ImageSelection::ActiveImage == appData.state().recenteringMode() ) )
                    {
                        appData.state().setRecenteringMode( ImageSelection::ActiveImage );

                        recenterAllViews( sk_recenterCrosshairs,
                                          sk_doNotRecenterOnCurrentCrosshairsPosition,
                                          sk_doNotResetObliqueOrientation );
                    }
                    ImGui::SameLine(); helpMarker( "Recenter views and crosshairs on the active image" );

                    if ( ImGui::RadioButton(
                             "Reference and active images",
                             ImageSelection::ReferenceAndActiveImages == appData.state().recenteringMode() ) )
                    {
                        appData.state().setRecenteringMode( ImageSelection::ReferenceAndActiveImages );

                        recenterAllViews( sk_recenterCrosshairs,
                                          sk_doNotRecenterOnCurrentCrosshairsPosition,
                                          sk_doNotResetObliqueOrientation );
                    }
                    ImGui::SameLine(); helpMarker( "Recenter views and crosshairs on the reference and active images" );

                    /// @todo These don't work yet
                    /*
                    if ( ImGui::RadioButton( "All visible images", ImageSelection::VisibleImagesInView == appData.state().recenteringMode() ) )
                    {
                        appData.state().setRecenteringMode( ImageSelection::VisibleImagesInView );
                        recenterAllViews( sk_recenterCrosshairs, sk_doNotRecenterOnCurrentCrosshairsPosition );
                    }
                    ImGui::SameLine(); helpMarker( "Recenter views and crosshairs on the visible images in each view" );

                    if ( ImGui::RadioButton( "Fixed image", ImageSelection::FixedImageInView == appData.state().recenteringMode() ) )
                    {
                        appData.state().setRecenteringMode( ImageSelection::FixedImageInView );
                        recenterAllViews( sk_recenterCrosshairs, sk_doNotRecenterOnCurrentCrosshairsPosition );
                    }
                    ImGui::SameLine(); helpMarker( "Recenter views on the fixed image in each view" );

                    if ( ImGui::RadioButton( "Moving image", ImageSelection::MovingImageInView == appData.state().recenteringMode() ) )
                    {
                        appData.state().setRecenteringMode( ImageSelection::MovingImageInView );
                        recenterAllViews( sk_recenterCrosshairs, sk_doNotRecenterOnCurrentCrosshairsPosition );
                    }
                    ImGui::SameLine(); helpMarker( "Recenter views on the moving image in each view" );

                    if ( ImGui::RadioButton( "Fixed and moving images", ImageSelection::FixedAndMovingImagesInView == appData.state().recenteringMode() ) )
                    {
                        appData.state().setRecenteringMode( ImageSelection::FixedAndMovingImagesInView );
                        recenterAllViews( sk_recenterCrosshairs, sk_doNotRecenterOnCurrentCrosshairsPosition );
                    }
                    ImGui::SameLine(); helpMarker( "Recenter views on the fixed and moving images in each view" );
                    */

                    if ( ImGui::RadioButton(
                             "All loaded images",
                             ImageSelection::AllLoadedImages == appData.state().recenteringMode() ) )
                    {
                        appData.state().setRecenteringMode( ImageSelection::AllLoadedImages );

                        recenterAllViews( sk_recenterCrosshairs,
                                          sk_doNotRecenterOnCurrentCrosshairsPosition,
                                          sk_doNotResetObliqueOrientation);
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

                ImGui::SetNextItemOpen( true, ImGuiCond_Appearing );
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


                ImGui::SetNextItemOpen( true, ImGuiCond_Appearing );
                if ( ImGui::TreeNode( "Cross-correlation" ) )
                {
                    ImGui::PushID( "crosscorr" );
                    {
                        renderMetricSettingsTab( appData.renderData().m_crossCorrelationParams,
                                                 appData.guiData().m_showCorrelationColormapWindow,
                                                 "crosscorr" );
                    }
                    ImGui::PopID(); // "crosscorr"
                    ImGui::TreePop();
                }


                ImGui::PopID(); /*** PopID metrics ***/
                ImGui::EndTabItem();
            }


            if ( ImGui::BeginTabItem( "Comparison modes" ) )
            {
                ImGui::PushID( "comparison" ); /*** PushID metrics ***/

//                if ( ImGui::TreeNode( "Comparison comparison" ) )
//                {
                    // Overlap style:
                    ImGui::Text( "Overlap:" );

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
                    ImGui::Spacing();
                    ImGui::Separator();


                    // Quadrants style:
                    ImGui::Text( "Quadrants:" );

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
                    ImGui::Spacing();
                    ImGui::Separator();


                    // Checkerboard squares
                    ImGui::Text( "Checkerboard:" );

                    int numSquares = appData.renderData().m_numCheckerboardSquares;
                    if ( ImGui::InputInt( "Number of checkers", &numSquares ) )
                    {
                        if ( 2 <= numSquares && numSquares <= 2048 )
                        {
                            appData.renderData().m_numCheckerboardSquares = numSquares;
                        }
                    }
                    ImGui::SameLine(); helpMarker( "Number of squares in Checkerboard mode" );
                    ImGui::Spacing();
                    ImGui::Separator();


                    // Flashlight
                    ImGui::Text( "Flashlight:" );

                    // Flashlight radius
                    const float radius = appData.renderData().m_flashlightRadius;
                    int radiusPercent = static_cast<int>( 100 * radius );
                    constexpr int k_minRadius = 1;
                    constexpr int k_maxRadius = 100;

                    if ( ImGui::SliderScalar( "Circle size", ImGuiDataType_S32,
                                              &radiusPercent, &k_minRadius, &k_maxRadius, "%d" ) )
                    {
                        appData.renderData().m_flashlightRadius = static_cast<float>( radiusPercent ) / 100.0f;
                    }
                    ImGui::SameLine();
                    helpMarker( "Circle size (as a percentage of the view size) for Flashlight rendering" );


                    ImGui::Spacing();
                    if ( ImGui::RadioButton( "Overlay moving image atop fixed image",
                                             true == appData.renderData().m_flashlightOverlays ) )
                    {
                        appData.renderData().m_flashlightOverlays = true;
                    }

                    if ( ImGui::RadioButton( "Replace fixed image with moving image",
                                             false == appData.renderData().m_flashlightOverlays ) )
                    {
                        appData.renderData().m_flashlightOverlays = false;
                    }
                    ImGui::SameLine();
                    helpMarker( "Mode for Flashlight rendering: overlay or replacement" );

//                    ImGui::Separator();
//                    ImGui::TreePop();
//                }

                ImGui::PopID(); /*** PopID comparison ***/
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


            if ( ImGui::BeginTabItem( "Other" ) )
            {
                static constexpr uint32_t sk_minPrecision = 0;
                static constexpr uint32_t sk_maxPrecision = 6;
                static constexpr uint32_t sk_stepPrecision = 1;

                ImGui::PushID( "other" ); /*** PushID other ***/

                uint32_t valuePrecision = appData.guiData().m_imageValuePrecision;
                uint32_t coordPrecision = appData.guiData().m_coordsPrecision;
                uint32_t txPrecision = appData.guiData().m_txPrecision;

                ImGui::Text( "Floating-point precision:" );

                if ( ImGui::InputScalar( "Image values", ImGuiDataType_U32,
                                         &valuePrecision, &sk_stepPrecision, &sk_stepPrecision, "%d" ) )
                {
                    appData.guiData().m_imageValuePrecision = std::min( std::max( valuePrecision, sk_minPrecision ), sk_maxPrecision );

                    appData.guiData().m_imageValuePrecisionFormat =
                            std::string( "%0." ) +
                            std::to_string( appData.guiData().m_imageValuePrecision ) +
                            std::string( "f" );
                }
                ImGui::SameLine(); helpMarker( "Floating-point precision of image values (e.g. in Inspector window)" );

                if ( ImGui::InputScalar( "Coordinates", ImGuiDataType_U32,
                                         &coordPrecision, &sk_stepPrecision, &sk_stepPrecision, "%d" ) )
                {
                    appData.guiData().m_coordsPrecision = std::min( std::max( coordPrecision, sk_minPrecision ), sk_maxPrecision );

                    appData.guiData().m_coordsPrecisionFormat =
                            std::string( "%0." ) +
                            std::to_string( appData.guiData().m_coordsPrecision ) +
                            std::string( "f" );
                }
                ImGui::SameLine(); helpMarker( "Floating-point precision of image spatial coordinates (e.g. in Inspector window)" );

                if ( ImGui::InputScalar( "Transformations", ImGuiDataType_U32,
                                         &txPrecision, &sk_stepPrecision, &sk_stepPrecision, "%d" ) )
                {
                    appData.guiData().m_txPrecision = std::min( std::max( txPrecision, sk_minPrecision ), sk_maxPrecision );

                    appData.guiData().m_txPrecisionFormat =
                            std::string( "%0." ) +
                            std::to_string( appData.guiData().m_txPrecision ) +
                            std::string( "f" );
                }
                ImGui::SameLine(); helpMarker( "Floating-point precision of image transformation parameters" );

                ImGui::PopID(); /*** PopID other ***/
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


    auto showSelectionButton = [] ()
    {
        ImGui::PushStyleColor( ImGuiCol_Button, buttonColor );
        if ( ImGui::Button( ICON_FK_LIST_ALT ) )
        {
            ImGui::OpenPopup( "selectionPopup" );
        }
        ImGui::PopStyleColor( 1 );

        if ( ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "Select image(s) to inspect" );
        }
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
            // Show context menu on right-button click:
            contextMenu();
        }
        else if ( ImGui::BeginPopup( "selectionPopup" ) )
        {
            // Show context menu if the user has clicked the popup button:
            contextMenu();
            ImGui::EndPopup();
        }
    }

    ImGui::End();
}


void renderInspectionWindowWithTable(
        AppData& appData,
        const std::function< std::pair<const char*, const char* >( size_t index ) >& getImageDisplayAndFileName,
        const std::function< std::optional<glm::vec3> ( size_t imageIndex ) >& getSubjectPos,
        const std::function< std::optional<glm::ivec3> ( size_t imageIndex ) >& getVoxelPos,
        const std::function< void ( size_t imageIndex, const glm::vec3& subjectPos ) > setSubjectPos,
        const std::function< void ( size_t imageIndex, const glm::ivec3& voxelPos ) > setVoxelPos,
        const std::function< std::optional<double> ( size_t imageIndex ) >& getImageValue,
        const std::function< std::optional<int64_t> ( size_t imageIndex ) >& getSegLabel,
        const std::function< ParcellationLabelTable* ( size_t tableIndex ) >& getLabelTable )
{
    static bool s_firstRun = true; // Is this the first run?

    static constexpr float sk_pad = 10.0f;
    static int corner = 2;

//    bool selectionButtonShown = false;

    static const ImVec2 sk_windowPadding( 0.0f, 0.0f );
//    static const float sk_windowRounding( 0.0f );

    static const ImVec4 buttonColor( 0.0f, 0.0f, 0.0f, 0.0f );
    static const ImVec4 blueColor( 0.0f, 0.5f, 1.0f, 1.0f );

    static const ImGuiTableFlags sk_tableFlags =
            ImGuiTableFlags_Resizable |
            ImGuiTableFlags_Reorderable |
            ImGuiTableFlags_Hideable |
            ImGuiTableFlags_Borders |
            ImGuiTableFlags_SizingFixedFit |
            ImGuiTableFlags_ScrollX |
            ImGuiTableFlags_ScrollY;

    static const ImGuiWindowFlags sk_windowFlags =
            ImGuiWindowFlags_MenuBar |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoNav;

    static bool s_showTitleBar = false;

    // For which images to show coordinates?
    static std::unordered_map<uuids::uuid, bool> s_showSubject;

    if ( s_firstRun )
    {
        // Show all images by default:
        for ( const auto& imageUid : appData.imageUidsOrdered() )
        {
            s_showSubject.insert( { imageUid, true } );
        }

        s_firstRun = false;
    }


    auto contextMenu = [&appData, &getImageDisplayAndFileName] ()
    {
        if ( ImGui::BeginMenu( "Show" ) )
        {
            for ( size_t imageIndex = 0; imageIndex < appData.numImages(); ++imageIndex )
            {
                const auto imageUid = appData.imageUid( imageIndex );
                if ( ! imageUid ) continue;

                auto it = s_showSubject.find( *imageUid );
                if ( std::end( s_showSubject ) == it )
                {
                    s_showSubject.insert( { *imageUid, true } );
                }

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

        if ( ImGui::BeginMenu( "Window" ) )
        {
            if ( ImGui::BeginMenu( "Position" ) )
            {
                if ( ImGui::MenuItem( "Custom", nullptr, corner == -1 ) ) corner = -1;
                if ( ImGui::MenuItem( "Top-left", nullptr, corner == 0 ) ) corner = 0;
                if ( ImGui::MenuItem( "Top-right", nullptr, corner == 1 ) ) corner = 1;
                if ( ImGui::MenuItem( "Bottom-left", nullptr, corner == 2 ) ) corner = 2;
                if ( ImGui::MenuItem( "Bottom-right", nullptr, corner == 3 ) ) corner = 3;
                ImGui::EndMenu();
            }

            if ( ImGui::MenuItem( "Show title bar", nullptr, s_showTitleBar ) )
            {
                s_showTitleBar = ! s_showTitleBar;
            }

            ImGui::Separator();
            if ( appData.guiData().m_showInspectionWindow && ImGui::MenuItem( "Close" ) )
            {
                appData.guiData().m_showInspectionWindow = false;
            }

            ImGui::EndMenu();
        }

//        ImGui::EndPopup();
    };


    auto showSelectionButton = [] ()
    {
        ImGui::PushStyleColor( ImGuiCol_Button, buttonColor );
        if ( ImGui::Button( "..." ) )
        {
            ImGui::OpenPopup( "selectionPopup" );
        }
        ImGui::PopStyleColor( 1 );

        if ( ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "Select image(s) to inspect" );
        }
    };


    ImGuiWindowFlags windowFlags = sk_windowFlags;

    if ( corner != -1 )
    {
        windowFlags |= ImGuiWindowFlags_NoMove;

        ImGuiIO& io = ImGui::GetIO();

        const ImVec2 windowPos(
                    ( corner & 1 ) ? io.DisplaySize.x - sk_pad : sk_pad,
                    ( corner & 2 ) ? io.DisplaySize.y - sk_pad : sk_pad );

        const ImVec2 windowPosPivot(
                    ( corner & 1 ) ? 1.0f : 0.0f,
                    ( corner & 2 ) ? 1.0f : 0.0f );

        ImGui::SetNextWindowPos( windowPos, ImGuiCond_Always, windowPosPivot );
    }

    if ( ! s_showTitleBar )
    {
        windowFlags |= ImGuiWindowFlags_NoDecoration;
    }

    const ImVec4* colors = ImGui::GetStyle().Colors;
    ImVec4 menuBarBgColor = colors[ImGuiCol_MenuBarBg];
    menuBarBgColor.w /= 2.0f;

    ImGui::SetNextWindowBgAlpha( 0.0f ); // Transparent background

    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, sk_windowPadding );
//    ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, sk_windowRounding );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );

    if ( ImGui::Begin( "##InspectionWindow", &( appData.guiData().m_showInspectionWindow ), windowFlags ) )
    {
        ImGui::PushStyleColor( ImGuiCol_MenuBarBg, menuBarBgColor );
        if ( ImGui::BeginMenuBar() )
        {
            contextMenu();
            ImGui::EndMenuBar();
        }
        ImGui::PopStyleColor( 1 ); // ImGuiCol_MenuBarBg


        if ( ImGui::BeginTable( "Image Information", 6, sk_tableFlags ) )
        {
            ImGui::TableSetupScrollFreeze( 1, 1 );

            ImGui::TableSetupColumn( "Image" );

            ImGui::TableSetupColumn( "Value" );
            ImGui::TableSetupColumn( "Label" );
            ImGui::TableSetupColumn( "Region", ImGuiTableColumnFlags_DefaultHide );

            ImGui::TableSetupColumn( "Voxel" );
            ImGui::TableSetupColumn( "Physical (mm)" );


            ImGui::TableHeadersRow();

            for ( size_t imageIndex = 0; imageIndex < appData.numImages(); ++imageIndex )
            {
                const auto imageUid = appData.imageUid( imageIndex );
                Image* image = ( imageUid ? appData.image( *imageUid ) : nullptr );
                if ( ! image ) continue;

                auto it = s_showSubject.find( *imageUid );
                if ( std::end( s_showSubject ) == it )
                {
                    s_showSubject.insert( { *imageUid, true } );
                }

                if ( ! s_showSubject[*imageUid] ) continue;

                ImGui::PushID( static_cast<int>( imageIndex ) ); /** PushID: imageIndex **/

                const auto segUid = appData.imageToActiveSegUid( *imageUid );
                const Image* seg = ( segUid ? appData.seg( *segUid ) : nullptr );
                ParcellationLabelTable* table = getLabelTable( seg->settings().labelTableIndex() );

                const std::optional<double> imageValue = getImageValue( imageIndex );
                const std::optional<long> segLabel = getSegLabel( imageIndex );

                const std::optional<glm::ivec3> voxelPos = getVoxelPos( imageIndex );
                const std::optional<glm::vec3> subjectPos = getSubjectPos( imageIndex );

                ImGui::TableNextColumn(); // "Image"              

                glm::vec3 darkerBorderColorHsv = glm::hsvColor( image->settings().borderColor() );
                darkerBorderColorHsv[2] = std::max( 0.5f * darkerBorderColorHsv[2], 0.0f );
                const glm::vec3 darkerBorderColorRgb = glm::rgbColor( darkerBorderColorHsv );

                const ImVec4 inputTextBgColor( darkerBorderColorRgb.r, darkerBorderColorRgb.g, darkerBorderColorRgb.b, 1.0f );
                const ImVec4 inputTextFgColor = ( glm::luminosity( darkerBorderColorRgb ) < 0.75f ) ? sk_whiteText : sk_blackText;

                ImGui::PushStyleColor( ImGuiCol_FrameBg, inputTextBgColor );
                ImGui::PushStyleColor( ImGuiCol_Text, inputTextFgColor );
                ImGui::PushItemWidth( -1 );
                {
                    std::string displayName = image->settings().displayName();
                    if ( ImGui::InputText( "##displayName", &displayName ) )
                    {
                        image->settings().setDisplayName( displayName );
                    }
                }
                ImGui::PopItemWidth();
                ImGui::PopStyleColor( 2 ); // ImGuiCol_FrameBg, ImGuiCol_Text

                if ( ImGui::IsItemHovered() )
                {
                    ImGui::SetTooltip( "%s", image->header().fileName().c_str() );
                }

                ImGui::SameLine(); showSelectionButton();


                ImGui::TableNextColumn(); // "Value"

                if ( imageValue )
                {
                    if ( isComponentFloatingPoint( image->header().memoryComponentType() ) )
                    {
                        double a = *imageValue;
                        ImGui::PushItemWidth( -1 );
                        ImGui::InputScalar( "##imageValue", ImGuiDataType_Double, &a, nullptr, nullptr,
                                            appData.guiData().m_imageValuePrecisionFormat.c_str() );
                        ImGui::PopItemWidth();

                        if ( image->header().numComponentsPerPixel() > 1 )
                        {
                            if ( ImGui::IsItemHovered() )
                            {
                                ImGui::SetTooltip( "Active component: %d", image->settings().activeComponent() );
                            }
                        }
                    }
                    else
                    {
                        int64_t a = static_cast<int64_t>( *imageValue );
                        ImGui::PushItemWidth( -1 );
                        ImGui::InputScalar( "##imageValue", ImGuiDataType_S64, &a, nullptr, nullptr, "%ld" );
                        ImGui::PopItemWidth();

                        if ( image->header().numComponentsPerPixel() > 1 )
                        {
                            // Multi-component case: show the value of the active component
                            if ( ImGui::IsItemHovered() )
                            {
                                ImGui::SetTooltip( "Active component: %d", image->settings().activeComponent() );
                            }
                        }
                    }
                }
                else
                {
                    ImGui::Text( "<N/A>" );
                }


                if ( segLabel )
                {
                    ImGui::TableNextColumn(); // "Label"

                    size_t l = static_cast<size_t>( *segLabel );
                    ImGui::PushItemWidth( -1 );
                    ImGui::InputScalar( "##segLabel", ImGuiDataType_U64, &l, nullptr, nullptr, "%ld" );
                    ImGui::PopItemWidth();

                    if ( table )
                    {
                        std::string labelName = table->getName( l );

                        if ( ImGui::IsItemHovered() )
                        {
                            // Tooltip for the segmentation label
                            ImGui::SetTooltip( "%s", labelName.c_str() );
                        }

                        ImGui::TableNextColumn(); // "Region"

                        ImGui::PushItemWidth( -1 );
                        if ( ImGui::InputText( "##labelName", &labelName ) )
                        {
                            table->setName( l, labelName );
                        }
                        ImGui::PopItemWidth();
                    }
                    else
                    {
                        ImGui::TableNextColumn(); // "Region"
                        ImGui::Text( "<N/A>" );
                    }
                }
                else
                {
                    ImGui::TableNextColumn(); // "Label"
                    ImGui::Text( "<N/A>" );

                    ImGui::TableNextColumn(); // "Region"
                    ImGui::Text( "<N/A>" );
                }


                if ( voxelPos )
                {
                    static const glm::ivec3 sk_zero{ 0 };
                    static const glm::ivec3 sk_minDim{ 0 };

                    ImGui::TableNextColumn(); // "Voxel"

                    const glm::ivec3 sk_maxDim = static_cast<glm::ivec3>( image->header().pixelDimensions() ) - glm::ivec3{ 1, 1, 1 };

                    glm::ivec3 a = *voxelPos;
                    ImGui::PushItemWidth( -1 );
                    if ( ImGui::DragScalarN( "##voxelPos", ImGuiDataType_S32, glm::value_ptr( a ), 3, 1.0f,
                                             glm::value_ptr( sk_minDim ), glm::value_ptr( sk_maxDim ), "%d" ) )
                    {
                        if ( glm::all( glm::greaterThanEqual( a, sk_zero ) ) &&
                             glm::all( glm::lessThan( a, glm::ivec3{ image->header().pixelDimensions() } ) ) )
                        {
                            setVoxelPos( imageIndex, a );
                        }
                    }
                    ImGui::PopItemWidth();

                    if ( ImGui::IsItemHovered() )
                    {
                        ImGui::SetTooltip( "Voxel index (i: column, j: row, k: slice)" );
                    }
                }
                else
                {
                    ImGui::TableNextColumn(); // "Voxel"
                    ImGui::Text( "<N/A>" );
                }


                if ( subjectPos )
                {
                    ImGui::TableNextColumn(); // "Physical"

                    // Step size is the  minimum voxel spacing
                    const float stepSize = glm::compMin( image->header().spacing() );
                    glm::vec3 a = *subjectPos;

                    ImGui::PushItemWidth( -1 );
                    if ( ImGui::DragScalarN( "##physicalPos", ImGuiDataType_Float, glm::value_ptr( a ), 3, stepSize,
                                             nullptr, nullptr, appData.guiData().m_coordsPrecisionFormat.c_str() ) )
                    {
                        setSubjectPos( imageIndex, a );
                    }
                    ImGui::PopItemWidth();

                    if ( ImGui::IsItemHovered() )
                    {
                        ImGui::SetTooltip( "Physical coordinate (x: R->L, y: A->P, z: I->S)" );
                    }
                }
                else
                {
                    ImGui::TableNextColumn(); // "Physical"
                    ImGui::Text( "<N/A>" );
                }

                ImGui::PopID(); /** PopID: imageIndex **/
            }

            ImGui::EndTable();
        }

        if ( ImGui::BeginPopupContextWindow() )
        {
            // Show context menu on right-button click:
            contextMenu();
        }
        else if ( ImGui::BeginPopup( "selectionPopup" ) )
        {
            // Show context menu if the user has clicked the popup button:
            contextMenu();
            ImGui::EndPopup();
        }

        ImGui::End();
    }

    // ImGuiStyleVar_WindowPadding, /*ImGuiStyleVar_WindowRounding*/, ImGuiStyleVar_WindowBorderSize
    ImGui::PopStyleVar( 2 );
}


void renderOpacityBlenderWindow(
        AppData& appData,
        const std::function< void ( const uuids::uuid& imageUid ) >& updateImageUniforms )
{
    /// @todo Use the "Drag and drop to copy/swap items" ImGui demo in order to allow reordering image layers
    /// by dragging the opacity sliders

    static const char* windowName = "Image Opacity Mixer";

    if ( ! appData.guiData().m_showOpacityBlenderWindow ) return;

    const bool showWindow = ImGui::Begin(
                windowName,
                &( appData.guiData().m_showOpacityBlenderWindow ),
                ImGuiWindowFlags_NoCollapse
                | ImGuiWindowFlags_AlwaysAutoResize );

    if ( ! showWindow )
    {
        ImGui::End();
        return;
    }

    int imageIndex = 0;

    for ( const auto& imageUid : appData.imageUidsOrdered() )
    {
        Image* image = appData.image( imageUid );
        if ( ! image ) continue;

        ImageSettings& imgSettings = image->settings();
        const ImageHeader& imgHeader = image->header();

        double opacity = imgSettings.opacity();

        const glm::vec3 borderColorHsv = glm::hsvColor( imgSettings.borderColor() );

        const float hue = borderColorHsv[0];
        const float sat = borderColorHsv[1];
        const float val = borderColorHsv[2];

        const glm::vec3 bgColor = glm::rgbColor( glm::vec3{ hue, 0.5f * sat, 0.5f * val } );
        const glm::vec3 bgHoveredColor = glm::rgbColor( glm::vec3{ hue, 0.6f * sat, 0.5f * val } );
        const glm::vec3 frameBgActiveColor = glm::rgbColor( glm::vec3{ hue, 0.7f * sat, 0.5f * val } );
        const glm::vec3 sliderGrabColor = glm::rgbColor( glm::vec3{ hue, sat, val } );

        ImGui::PushID( imageIndex );
        ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( bgColor.r, bgColor.g, bgColor.b, 1.0f ) );
        ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, ImVec4( bgHoveredColor.r, bgHoveredColor.g, bgHoveredColor.b, 1.0f ) );
        ImGui::PushStyleColor( ImGuiCol_FrameBgActive, ImVec4( frameBgActiveColor.r, frameBgActiveColor.g, frameBgActiveColor.b, 1.0f ) );
        ImGui::PushStyleColor( ImGuiCol_SliderGrab, ImVec4( sliderGrabColor.r, sliderGrabColor.g, sliderGrabColor.b, 1.0f ) );

        const std::string name = imgSettings.displayName() + "##" + std::to_string( imageIndex );

        if ( mySliderF64( name.c_str(), &opacity, 0.0, 1.0 ) && ! appData.renderData().m_opacityMixMode )
        {
            imgSettings.setOpacity( opacity );
            updateImageUniforms( imageUid );
        }

        if ( ImGui::IsItemActive() || ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "%s", imgHeader.fileName().c_str() );
        }

        ImGui::PopStyleColor( 4 );
        ImGui::PopID();
    }


    static double mix = 0.0;

    if ( appData.numImages() > 1 )
    {
        ImGui::Checkbox( "Comparison blender", &( appData.renderData().m_opacityMixMode ) );
        ImGui::SameLine(); helpMarker( "Use a single slider to blend across all adjacent image layers" );
    }
    else
    {
        appData.renderData().m_opacityMixMode = false;
    }

    if ( appData.renderData().m_opacityMixMode )
    {
        mySliderF64( "Blend", &mix, 0.0, static_cast<double>( appData.numImages() - 1 ) );

        const double imgIndex = mix;

        const double frac = imgIndex - std::floor( imgIndex );

        size_t imgIndexLo = static_cast<size_t>( std::floor( imgIndex ) );
        size_t imgIndexHi = static_cast<size_t>( std::ceil( imgIndex ) );

        for ( size_t i = 0; i < appData.numImages(); ++i )
        {
            const auto imgUid = appData.imageUid( i );
            if ( ! imgUid ) continue;

            Image* img = appData.image( *imgUid );
            if ( ! img ) continue;

            double op = 0.0;

            if ( i < imgIndexLo || imgIndexHi < i )
            {
                op = 0.0;
            }
            else if ( imgIndexLo == i )
            {
                op = 1.0 - frac;
            }
            else if ( imgIndexHi == i )
            {
                op = frac;
            }

            img->settings().setOpacity( op );
            updateImageUniforms( *imgUid );
        }
    }

    ImGui::End();
}
