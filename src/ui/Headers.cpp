#include "ui/Headers.h"

#include "ui/GuiData.h"
#include "ui/Helpers.h"
#include "ui/ImGuiCustomControls.h"
#include "ui/Popups.h"
#include "ui/Widgets.h"

// data::roundPointToNearestImageVoxelCenter
// data::getAnnotationSubjectPlaneName
#include "common/DataHelper.h"
#include "common/MathFuncs.h"

#include "image/Image.h"
#include "image/ImageColorMap.h"
#include "image/ImageHeader.h"
#include "image/ImageSettings.h"
#include "image/ImageTransformations.h"

#include "logic/app/Data.h"
#include "logic/camera/CameraHelpers.h"

#include <IconFontCppHeaders/IconsForkAwesome.h>

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/color_space.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <algorithm>
#include <string>

#undef min
#undef max


namespace
{

static const ImVec4 sk_whiteText( 1, 1, 1, 1 );
static const ImVec4 sk_blackText( 0, 0, 0, 1 );

/// Size of small toolbar buttons (pixels)
static const ImVec2 sk_smallToolbarButtonSize( 24, 24 );

static const std::string sk_referenceAndActiveImageMessage( "This is the reference and active image" );
static const std::string sk_referenceImageMessage( "This is the reference image" );
static const std::string sk_activeImageMessage( "This is the active image" );
static const std::string sk_nonActiveImageMessage( "This is not the active image" );

static const ImGuiColorEditFlags sk_colorEditFlags =
        ImGuiColorEditFlags_NoInputs |
        ImGuiColorEditFlags_PickerHueBar |
        ImGuiColorEditFlags_DisplayRGB |
        ImGuiColorEditFlags_DisplayHSV |
        ImGuiColorEditFlags_DisplayHex |
        ImGuiColorEditFlags_Uint8 |
        ImGuiColorEditFlags_InputRGB;

std::pair< ImVec4, ImVec4 > computeHeaderBgAndTextColors( const glm::vec3& color )
{
    glm::vec3 darkerBorderColorHsv = glm::hsvColor( color );
    darkerBorderColorHsv[2] = std::max( 0.5f * darkerBorderColorHsv[2], 0.0f );
    const glm::vec3 darkerBorderColorRgb = glm::rgbColor( darkerBorderColorHsv );

    const ImVec4 headerColor( darkerBorderColorRgb.r, darkerBorderColorRgb.g, darkerBorderColorRgb.b, 1.0f );
    const ImVec4 headerTextColor = ( glm::luminosity( darkerBorderColorRgb ) < 0.75f ) ? sk_whiteText : sk_blackText;

    return { headerColor, headerTextColor };
}

} // anonymous


void renderImageHeaderInformation(
        const AppData& appData,
        const ImageHeader& imgHeader,
        const ImageSettings& /*imgSettings*/,
        const ImageTransformations& imgTx )
{
    const char* txFormat = appData.guiData().m_txPrecisionFormat.c_str();
    const char* coordFormat = appData.guiData().m_coordsPrecisionFormat.c_str();


    // File name:
    std::string fileName = imgHeader.fileName();
    ImGui::InputText( "File name", &fileName, ImGuiInputTextFlags_ReadOnly );
    ImGui::SameLine(); helpMarker( "Image file name" );
    ImGui::Spacing();

    ImGui::Separator();


    // Dimensions:
    glm::uvec3 dimensions = imgHeader.pixelDimensions();
    ImGui::InputScalarN( "Dimensions (vox)", ImGuiDataType_U32, glm::value_ptr( dimensions ), 3,
                         nullptr, nullptr, nullptr, ImGuiInputTextFlags_ReadOnly );
    ImGui::SameLine(); helpMarker( "Matrix dimensions in voxels" );
    ImGui::Spacing();


    // Spacing:
    glm::vec3 spacing = imgHeader.spacing();
    ImGui::InputScalarN( "Spacing (mm)", ImGuiDataType_Float, glm::value_ptr( spacing ), 3,
                         nullptr, nullptr, "%0.6f", ImGuiInputTextFlags_ReadOnly );
    ImGui::SameLine(); helpMarker( "Voxel spacing (mm)" );
    ImGui::Spacing();


    // Origin:
    glm::vec3 origin = imgHeader.origin();
    ImGui::InputScalarN( "Origin (mm)", ImGuiDataType_Float, glm::value_ptr( origin ), 3,
                         nullptr, nullptr, coordFormat, ImGuiInputTextFlags_ReadOnly );
    ImGui::SameLine(); helpMarker( "Image origin (mm): physical coordinates of voxel (0, 0, 0)" );
    ImGui::Spacing();


    // Directions:
    glm::mat3 directions = imgHeader.directions();
    ImGui::Text( "Voxel coordinate directions:" );
    ImGui::SameLine(); helpMarker( "Direction vectors in physical Subject space of the X, Y, Z image voxel axes. "
                                   "Also known as the voxel direction cosines matrix." );

    ImGui::InputFloat3( "X", glm::value_ptr( directions[0] ), coordFormat, ImGuiInputTextFlags_ReadOnly );
    ImGui::InputFloat3( "Y", glm::value_ptr( directions[1] ), coordFormat, ImGuiInputTextFlags_ReadOnly );
    ImGui::InputFloat3( "Z", glm::value_ptr( directions[2] ), coordFormat, ImGuiInputTextFlags_ReadOnly );


    // Closest orientation code:
    std::string orientation = imgHeader.spiralCode();
    orientation += ( imgHeader.isOblique() ? " (oblique)" : "" );
    ImGui::InputText( "Orientation", &orientation, ImGuiInputTextFlags_ReadOnly );
    ImGui::SameLine();
    helpMarker( "Closest orientation 'SPIRAL' code (-x to +x: R to L; -y to +y: A to P; -z to +z: I to S" );

    ImGui::Spacing();
    ImGui::Separator();

#if 0
    // Ignore spacing checkbox:
    bool visible1 = imgSettings.visibility();
    if ( ImGui::Checkbox( "Ignore spacing", &visible1 ) )
    {
        imgSettings.setVisibility( visible1 );
//        updateImageUniforms();
    }
    ImGui::SameLine(); helpMarker( "Ignore voxel spacing from header: force spacing to (1.0, 1.0, 1.0) mm" );


    // Ignore origin checkbox:
    bool visible = imgSettings.visibility();
    if ( ImGui::Checkbox( "Ignore origin", &visible ) )
    {
        imgSettings.setVisibility( visible );
//        updateImageUniforms();
    }
    ImGui::SameLine(); helpMarker( "Ignore image origin from header: force origin to (0.0, 0.0, 0.0) mm" );


    // Ignore directions checkbox:
    bool visible2 = imgSettings.visibility();
    if ( ImGui::Checkbox( "Ignore directions", &visible2 ) )
    {
        imgSettings.setVisibility( visible );
//        updateImageUniforms();
    }
    ImGui::SameLine(); helpMarker( "Ignore voxel directions from header: force direction cosines matrix to identity" );

    ImGui::Spacing();
    ImGui::Separator();
#endif


    // subject_T_voxels:
    ImGui::Text( "Voxel-to-Subject transformation:" );
    ImGui::SameLine(); helpMarker( "Transformation from Voxel indices to Subject (LPS) space" );

    glm::mat4 s_T_p = glm::transpose( imgTx.subject_T_pixel() );

    ImGui::PushItemWidth( -1 );
    ImGui::InputFloat4( "", glm::value_ptr( s_T_p[0] ), txFormat, ImGuiInputTextFlags_ReadOnly );
    ImGui::InputFloat4( "", glm::value_ptr( s_T_p[1] ), txFormat, ImGuiInputTextFlags_ReadOnly );
    ImGui::InputFloat4( "", glm::value_ptr( s_T_p[2] ), txFormat, ImGuiInputTextFlags_ReadOnly );
    ImGui::InputFloat4( "", glm::value_ptr( s_T_p[3] ), txFormat, ImGuiInputTextFlags_ReadOnly );
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Separator();



    // Bounding box:
    ImGui::Text( "Bounding box (in Subject space):" );

    // Note: we used to display the min and max bounding box corners in Subject space.
    // However, this does not make sense if the Voxel-to-Subject transformation has a rotation.

    glm::vec3 boxCenter = imgHeader.subjectBBoxCenter();
    ImGui::InputScalarN( "Center (mm)", ImGuiDataType_Float, glm::value_ptr( boxCenter ), 3,
                         nullptr, nullptr, coordFormat, ImGuiInputTextFlags_ReadOnly );
    ImGui::SameLine(); helpMarker( "Bounding box center in Subject space (mm)" );
    ImGui::Spacing();


    glm::vec3 boxSize = imgHeader.subjectBBoxSize();
    ImGui::InputScalarN( "Size (mm)", ImGuiDataType_Float, glm::value_ptr( boxSize ), 3,
                         nullptr, nullptr, coordFormat, ImGuiInputTextFlags_ReadOnly );
    ImGui::SameLine(); helpMarker( "Bounding box size (mm)" );

    ImGui::Spacing();
    ImGui::Separator();


    // Pixel type:
    std::string pixelType = imgHeader.pixelTypeAsString();
    ImGui::InputText( "Pixel type", &pixelType, ImGuiInputTextFlags_ReadOnly );
    ImGui::SameLine(); helpMarker( "Image pixel type" );
    ImGui::Spacing();


    // Number of components:
    uint32_t numComponentsPerPixel = imgHeader.numComponentsPerPixel();
    ImGui::InputScalar( "Num. components", ImGuiDataType_U32, &numComponentsPerPixel,
                        nullptr, nullptr, nullptr, ImGuiInputTextFlags_ReadOnly );
    ImGui::SameLine(); helpMarker( "Number of components per pixel" );
    ImGui::Spacing();


    // Component type:
    std::string componentType = imgHeader.fileComponentTypeAsString();
    ImGui::InputText( "Component type", &componentType, ImGuiInputTextFlags_ReadOnly );
    ImGui::SameLine(); helpMarker( "Image component type" );
    ImGui::Spacing();


    // Image size (bytes):
    uint64_t fileSizeBytes = imgHeader.fileImageSizeInBytes();
    ImGui::InputScalar( "Size (bytes)", ImGuiDataType_U64, &fileSizeBytes,
                        nullptr, nullptr, nullptr, ImGuiInputTextFlags_ReadOnly );
    ImGui::SameLine(); helpMarker( "Image size in bytes" );
    ImGui::Spacing();


    // Image size (MiB):
    double fileSizeMiB = static_cast<double>( imgHeader.fileImageSizeInBytes() ) / ( 1024.0 * 1024.0 );
    ImGui::InputScalar( "Size (MiB)", ImGuiDataType_Double, &fileSizeMiB,
                        nullptr, nullptr, nullptr, ImGuiInputTextFlags_ReadOnly );
    ImGui::SameLine(); helpMarker( "Image size in mebibytes (MiB)" );

    ImGui::Spacing();


    if ( ImGui::TreeNode( "Intensity histogram" ) )
    {
        // Other plotting tools for ImGui:
        // ImPlot https://github.com/epezent/implot
        // others https://github.com/ocornut/imgui/wiki/Useful-Widgets


//        auto f = [&imgSettings] (int idx) -> f
//        static float arr[] = { 0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f };
//        ImGui::PlotHistogram( "##histogram", imgSettings.componentStatistics( 0 ).m_histogram.data(), IM_ARRAYSIZE(arr), 0, NULL, 0.0f, 1.0f, ImVec2( 0, 80.0f ) );

//        IMGUI_API void          PlotHistogram(const char* label, float(*values_getter)(void* data, int idx), void* data, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0, 0));
//        ImGui::PlotHistogram("Histogram", func, NULL, display_count, 0, NULL, -1.0f, 1.0f, ImVec2(0, 80));

//        ImGui::PlotLines("Lines", values, IM_ARRAYSIZE(values), values_offset, overlay, -1.0f, 1.0f, ImVec2(0, 80.0f));
    }
}


void renderImageHeader(
        AppData& appData,
        GuiData& guiData,
        const uuids::uuid& imageUid,
        size_t imageIndex,
        Image* image,
        bool isActiveImage,
        size_t numImages,
        const std::function< void(void) >& updateImageUniforms,
        const std::function< void(void) >& updateImageInterpolationMode,
        const std::function< size_t(void) >& getNumImageColorMaps,
        const std::function< const ImageColorMap* ( size_t cmapIndex ) >& getImageColorMap,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageBackward,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageForward,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageToBack,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageToFront,
        const std::function< bool ( const uuids::uuid& imageUid, bool locked ) >& setLockManualImageTransformation )
{
    static const ImGuiColorEditFlags sk_colorNoAlphaEditFlags =
            ImGuiColorEditFlags_NoInputs |
            ImGuiColorEditFlags_PickerHueBar |
            ImGuiColorEditFlags_DisplayRGB |
            ImGuiColorEditFlags_DisplayHex |
            ImGuiColorEditFlags_Uint8 |
            ImGuiColorEditFlags_InputRGB ;

    static const ImGuiColorEditFlags sk_colorAlphaEditFlags =
            ImGuiColorEditFlags_PickerHueBar |
            ImGuiColorEditFlags_DisplayRGB |
            ImGuiColorEditFlags_DisplayHex |
            ImGuiColorEditFlags_AlphaBar |
            ImGuiColorEditFlags_AlphaPreviewHalf |
            ImGuiColorEditFlags_Uint8 |
            ImGuiColorEditFlags_InputRGB;

    const ImVec4* colors = ImGui::GetStyle().Colors;
    const ImVec4 activeColor = colors[ImGuiCol_ButtonActive];
    ImVec4 inactiveColor = colors[ImGuiCol_Button];

    const std::string minValuesFormatString = std::string( "Min: " ) + appData.guiData().m_imageValuePrecisionFormat;
    const std::string maxValuesFormatString = std::string( "Max: " ) + appData.guiData().m_imageValuePrecisionFormat;

    const char* minValuesFormat = minValuesFormatString.c_str();
    const char* maxValuesFormat = maxValuesFormatString.c_str();

    const char* valuesFormat = appData.guiData().m_imageValuePrecisionFormat.c_str();
    const char* txFormat = appData.guiData().m_txPrecisionFormat.c_str();


    if ( ! image ) return;

    const auto& imgHeader = image->header();
    auto& imgSettings = image->settings();
    auto& imgTx = image->transformations();

    auto getCurrentImageColormapIndex = [&imgSettings] ()
    {
        return imgSettings.colorMapIndex();
    };

    auto setCurrentImageColormapIndex = [&imgSettings] ( size_t cmapIndex )
    {
        imgSettings.setColorMapIndex( cmapIndex );
    };


    ImGuiTreeNodeFlags headerFlags = ImGuiTreeNodeFlags_CollapsingHeader;

    if ( isActiveImage )
    {
        headerFlags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    ImGui::PushID( uuids::to_string( imageUid ).c_str() );

    // Header is ID'ed only by the image index.
    // ### allows the header name to change without changing its ID.
    const std::string headerName =
            std::to_string( imageIndex ) + ") " +
            imgSettings.displayName() +
            "###" + std::to_string( imageIndex );

    const auto headerColors = computeHeaderBgAndTextColors( imgSettings.borderColor() );
    ImGui::PushStyleColor( ImGuiCol_Header, headerColors.first );
    ImGui::PushStyleColor( ImGuiCol_Text, headerColors.second );

    const bool clicked = ImGui::CollapsingHeader( headerName.c_str(), headerFlags );

    ImGui::PopStyleColor( 2 ); // ImGuiCol_Header, ImGuiCol_Text

    if ( ! clicked )
    {
        ImGui::PopID(); // imageUid
        return;
    }

    ImGui::Spacing();


    // Border color:
    glm::vec3 borderColor{ imgSettings.borderColor() };

    if ( ImGui::ColorEdit3( "##BorderColor", glm::value_ptr( borderColor ), sk_colorNoAlphaEditFlags ) )
    {
        imgSettings.setBorderColor( borderColor );
        imgSettings.setEdgeColor( borderColor ); // Set edge color to border color
        updateImageUniforms();
    }
//    ImGui::SameLine(); helpMarker( "Image border color" );

    // Display name text:
    std::string displayName = imgSettings.displayName();
    ImGui::SameLine();

    if ( ImGui::InputText( "Name", &displayName ) )
    {
        imgSettings.setDisplayName( displayName );
    }
    ImGui::SameLine(); helpMarker( "Set the image display name and border color" );


    if ( ImGui::Button( ICON_FK_HAND_O_UP, sk_smallToolbarButtonSize ) )
    {
        glm::vec3 worldPos{ imgTx.worldDef_T_subject() *
                glm::vec4{ imgHeader.subjectBBoxCenter(), 1.0f } };

        /// @todo Put this in CallbackHandler as separate function, because it is used frequently
        if ( appData.renderData().m_snapCrosshairsToReferenceVoxels )
        {
            if ( const Image* refImg = appData.refImage() )
            {
                worldPos = data::roundPointToNearestImageVoxelCenter( *refImg, worldPos );
            }
        }

        appData.state().setWorldCrosshairsPos( worldPos );
    }

    if ( ImGui::IsItemHovered() )
    {
        ImGui::SetTooltip( "Move crosshairs to the center of the image" );
    }

    ImGui::SameLine();
    ImGui::Text( "Go to image center" );


    if ( ! isActiveImage )
    {
        if ( ImGui::Button( ICON_FK_TOGGLE_OFF ) )
        {
            if ( appData.setActiveImageUid( imageUid ) ) return;
        }

        if ( ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "Make this the active image" );
        }
    }
    else
    {
        ImGui::PushStyleColor( ImGuiCol_Button, activeColor );
        ImGui::Button( ICON_FK_TOGGLE_ON );
        ImGui::PopStyleColor( 1 ); // ImGuiCol_Button

        if ( ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "This is the active image" );
        }
    }

    ImGui::SameLine();

    const bool isRef = ( 0 == imageIndex );

    if ( isRef && isActiveImage )
    {
        ImGui::Text( "%s", sk_referenceAndActiveImageMessage.c_str() );
    }
    else if ( isRef )
    {
        ImGui::Text( "%s", sk_referenceImageMessage.c_str() );
    }
    else if ( isActiveImage )
    {
        ImGui::Text( "%s", sk_activeImageMessage.c_str() );
    }
    else
    {
        ImGui::Text( "%s", sk_nonActiveImageMessage.c_str() );
    }


    if ( isActiveImage )
    {
        const bool isLocked = image->transformations().is_worldDef_T_affine_locked();

        ImGui::PushStyleColor( ImGuiCol_Button, ( isLocked ? inactiveColor : activeColor ) );
        if ( ImGui::Button( ( isLocked ? ICON_FK_LOCK : ICON_FK_UNLOCK ), sk_smallToolbarButtonSize ) )
        {
            setLockManualImageTransformation( imageUid, ! isLocked );
        }
        ImGui::PopStyleColor( 1 ); // ImGuiCol_Button

        if ( image->transformations().is_worldDef_T_affine_locked() )
        {
            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "Manual image transformation is locked.\nClick to unlock and allow movement." );
            }

            ImGui::SameLine();
            ImGui::Text( "Transformation is locked" );
        }
        else
        {
            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "Manual image transformation is unlocked.\nClick to lock and prevent movement." );
            }

            ImGui::SameLine();
            ImGui::Text( "Transformation is unlocked" );
        }
    }


    if ( 0 < imageIndex )
    {
        // Rules for showing the buttons that change image order:
        const bool showDecreaseIndex = true | ( 1 < imageIndex );
        const bool showIncreaseIndex = true | ( imageIndex < numImages - 1 );

        if ( showDecreaseIndex || showIncreaseIndex )
        {
            ImGui::Text( "Image order: " );
        }

        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0.0f, 0.0f ) );

        if ( showDecreaseIndex )
        {
            ImGui::SameLine();
            if ( ImGui::Button( ICON_FK_FAST_BACKWARD ) )
            {
                moveImageToBack( imageUid );
            }
            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "Move image to backmost layer" );
            }

            ImGui::SameLine();
            if ( ImGui::Button( ICON_FK_BACKWARD ) )
            {
                moveImageBackward( imageUid );
            }
            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "Move image backward in layers (decrease the image order)" );
            }
        }

        if ( showIncreaseIndex )
        {
            ImGui::SameLine();
            if ( ImGui::Button( ICON_FK_FORWARD ) )
            {
                moveImageForward( imageUid );
            }
            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "Move image forward in layers (increase the image order)" );
            }

            ImGui::SameLine();
            if ( ImGui::Button( ICON_FK_FAST_FORWARD ) )
            {
                moveImageToFront( imageUid );
            }
            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "Move image to frontmost layer" );
            }
        }

        /*** ImGuiStyleVar_ItemSpacing ***/
        ImGui::PopStyleVar( 1 );
    }

    ImGui::Spacing();
    ImGui::Separator();


    // Open View Properties on first appearance
    ImGui::SetNextItemOpen( true, ImGuiCond_Appearing );
    if ( ImGui::TreeNode( "View Properties" ) )
    {
        // Component selection combo selection list. The component selection is shown only for
        // multi-component images, where each component is stored as a separate image.
        const bool showComponentSelection =
                ( imgHeader.numComponentsPerPixel() > 1 &&
                  Image::MultiComponentBufferType::SeparateImages == image->bufferType() );

        if ( showComponentSelection &&
             ImGui::BeginCombo( "Component", std::to_string( imgSettings.activeComponent() ).c_str() ) )
        {
            for ( uint32_t comp = 0; comp < imgHeader.numComponentsPerPixel(); ++comp )
            {
                const bool isSelected = ( imgSettings.activeComponent() == comp );
                if ( ImGui::Selectable( std::to_string(comp).c_str(), isSelected) )
                {
                    imgSettings.setActiveComponent( comp );
                    updateImageUniforms();
                }

                if ( isSelected ) ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();

            ImGui::SameLine(); helpMarker( "Select the image component to display and adjust" );
        }


        auto activeSegUid = appData.imageToActiveSegUid( imageUid );
        Image* activeSeg = ( activeSegUid ) ? appData.seg( *activeSegUid ) : nullptr;


        // Visibility checkbox:
        bool visible = imgSettings.visibility();

        if ( ImGui::Checkbox( "Image visible", &visible ) )
        {
            imgSettings.setVisibility( visible );
            updateImageUniforms();
        }
        ImGui::SameLine(); helpMarker( "Show/hide the image (W) on all views" );

        if ( activeSeg )
        {
            bool segVisible = activeSeg->settings().visibility();

            if ( ImGui::Checkbox( "Segmentation visible", &segVisible ) )
            {
                activeSeg->settings().setVisibility( segVisible );
                updateImageUniforms();
            }
            ImGui::SameLine(); helpMarker( "Show/hide the image segmentation (S) on all views" );
        }


//        if ( visible )
        {
            // Image opacity slider:
            double imageOpacity = imgSettings.opacity();
            if ( mySliderF64( "Opacity", &imageOpacity, 0.0, 1.0 ) )
            {
                imgSettings.setOpacity( imageOpacity );
                updateImageUniforms();
            }
            ImGui::SameLine(); helpMarker( "Image layer opacity" );


            // Segmentation opacity slider:
            if ( activeSeg )
            {
                double segOpacity = activeSeg->settings().opacity();
                if ( mySliderF64( "Segmentation", &segOpacity, 0.0, 1.0 ) )
                {
                    activeSeg->settings().setOpacity( segOpacity );
                    updateImageUniforms();
                }
                ImGui::SameLine(); helpMarker( "Segmentation layer opacity" );
            }

            ImGui::Dummy( ImVec2( 0.0f, 1.0f ) );
        }


        if ( ComponentType::Float32 == imgHeader.memoryComponentType() ||
             ComponentType::Float64 == imgHeader.memoryComponentType() )
        {
            // Threshold range:
            const float threshMin = static_cast<float>( imgSettings.thresholdRange().first );
            const float threshMax = static_cast<float>( imgSettings.thresholdRange().second );

            float threshLow = static_cast<float>( imgSettings.thresholdLow() );
            float threshHigh = static_cast<float>( imgSettings.thresholdHigh() );

            /// @todo Change speed of range slider based on the image range. Right now set to 1.0
            if ( ImGui::DragFloatRange2( "Threshold", &threshLow, &threshHigh,
                                         0.1f, threshMin, threshMax,
                                         minValuesFormat, maxValuesFormat,
                                         ImGuiSliderFlags_AlwaysClamp ) )
            {
                imgSettings.setThresholdLow( static_cast<double>( threshLow ) );
                imgSettings.setThresholdHigh( static_cast<double>( threshHigh ) );
                updateImageUniforms();
            }
            ImGui::SameLine();
            helpMarker( "Lower and upper image thresholds" );


            // Window/level sliders:
            const float valueMin = static_cast<float>( imgSettings.componentStatistics().m_minimum );
            const float valueMax = static_cast<float>( imgSettings.componentStatistics().m_maximum );

            const float windowMin = static_cast<float>( imgSettings.windowRange().first );
            const float windowMax = static_cast<float>( imgSettings.windowRange().second );

            const float levelMin = static_cast<float>( imgSettings.levelRange().first );
            const float levelMax = static_cast<float>( imgSettings.levelRange().second );

            float window = static_cast<float>( imgSettings.window() );
            float level = static_cast<float>( imgSettings.level() );

            float windowLow = std::max( level - 0.5f * window, valueMin );
            float windowHigh = std::min( level + 0.5f * window, valueMax );

            if ( ImGui::DragFloatRange2( "Window", &windowLow, &windowHigh,
                                         0.1f, valueMin, valueMax,
                                         minValuesFormat, maxValuesFormat,
                                         ImGuiSliderFlags_AlwaysClamp ) )
            {
                const double newWindow = static_cast<double>( windowHigh - windowLow );
                const double newLevel = static_cast<double>( 0.5f * ( windowLow + windowHigh ) );

                imgSettings.setWindow( newWindow );
                imgSettings.setLevel( newLevel );
                updateImageUniforms();
            }
            ImGui::SameLine(); helpMarker( "Set the minimum and maximum of the window range" );

            if ( mySliderF32( "Width", &window, windowMin, windowMax, valuesFormat ) )
            {
                imgSettings.setWindow( static_cast<double>( window ) );
                updateImageUniforms();
            }
            ImGui::SameLine(); helpMarker( "Window width" );

            if ( mySliderF32( "Level", &level, levelMin, levelMax, valuesFormat ) )
            {
                imgSettings.setLevel( static_cast<double>( level ) );
                updateImageUniforms();
            }
            ImGui::SameLine(); helpMarker( "Window level (center)" );
        }
        else
        {
            const int32_t threshMin = static_cast<int32_t>( imgSettings.thresholdRange().first );
            const int32_t threshMax = static_cast<int32_t>( imgSettings.thresholdRange().second );

            int32_t threshLow = static_cast<int32_t>( imgSettings.thresholdLow() );
            int32_t threshHigh = static_cast<int32_t>( imgSettings.thresholdHigh() );

            if ( ImGui::DragIntRange2( "Threshold", &threshLow, &threshHigh,
                                       1.0f, threshMin, threshMax,
                                       "Min: %d", "Max: %d",
                                       ImGuiSliderFlags_AlwaysClamp ) )
            {
                imgSettings.setThresholdLow( static_cast<double>( threshLow ) );
                imgSettings.setThresholdHigh( static_cast<double>( threshHigh ) );
                updateImageUniforms();
            }

            ImGui::SameLine(); helpMarker( "Lower and upper image thresholds" );


            const int32_t valueMin = static_cast<int32_t>( imgSettings.componentStatistics().m_minimum );
            const int32_t valueMax = static_cast<int32_t>( imgSettings.componentStatistics().m_maximum );

            const int32_t windowMin = static_cast<int32_t>( imgSettings.windowRange().first );
            const int32_t windowMax = static_cast<int32_t>( imgSettings.windowRange().second );

            const int32_t levelMin = static_cast<int32_t>( imgSettings.levelRange().first );
            const int32_t levelMax = static_cast<int32_t>( imgSettings.levelRange().second );

            int32_t window = static_cast<int32_t>( imgSettings.window() );
            int32_t level = static_cast<int32_t>( imgSettings.level() );

            int32_t windowLow = std::max( static_cast<int32_t>( level - 0.5 * window ), valueMin );
            int32_t windowHigh = std::min( static_cast<int32_t>( level + 0.5 * window ), valueMax );

            if ( ImGui::DragIntRange2( "Window", &windowLow, &windowHigh,
                                       1.0f, valueMin, valueMax,
                                       "Min: %d", "Max: %d",
                                       ImGuiSliderFlags_AlwaysClamp ) )
            {
                const double newWindow = static_cast<double>( windowHigh - windowLow );
                const double newLevel = static_cast<double>( 0.5 * ( windowLow + windowHigh ) );

                imgSettings.setWindow( newWindow );
                imgSettings.setLevel( newLevel );
                updateImageUniforms();
            }
            ImGui::SameLine(); helpMarker( "Minimum and maximum of the window range" );

            if ( mySliderS32( "Width", &window, windowMin, windowMax ) )
            {
                imgSettings.setWindow( static_cast<double>( window ) );
                updateImageUniforms();
            }
            ImGui::SameLine(); helpMarker( "Window width" );

            if ( mySliderS32( "Level", &level, levelMin, levelMax ) )
            {
                imgSettings.setLevel( static_cast<double>( level ) );
                updateImageUniforms();
            }
            ImGui::SameLine(); helpMarker( "Window level (center)" );
        }
        ImGui::Spacing();


        // Interpolation radio buttons:
        int interp = ( InterpolationMode::NearestNeighbor == imgSettings.interpolationMode() ) ? 0 : 1;

        if ( ImGui::RadioButton( "Nearest", &interp, 0 ) )
        {
            imgSettings.setInterpolationMode( InterpolationMode::NearestNeighbor );
            updateImageInterpolationMode();
        }

        ImGui::SameLine();
        if ( ImGui::RadioButton( "Linear interpolation", &interp, 1 ) )
        {
            imgSettings.setInterpolationMode( InterpolationMode::Linear );
            updateImageInterpolationMode();
        }
        ImGui::SameLine(); helpMarker( "Nearest neighbor or trilinear interpolation" );


        // Image colormap dialog:
        bool* showImageColormapWindow = &( guiData.m_showImageColormapWindow[imageUid] );
        ImGui::Spacing();
        *showImageColormapWindow |= ImGui::Button( "Select colormap" );

        ImGui::SameLine();
        bool invertedCmap = imgSettings.isColorMapInverted();
        if ( ImGui::Checkbox( "Inverted", &invertedCmap ) )
        {
            imgSettings.setColorMapInverted( invertedCmap );
            updateImageUniforms();
        }
        ImGui::SameLine(); helpMarker( "Select/invert the image colormap" );

        renderPaletteWindow(
                    std::string( "Select colormap for image '" + imgSettings.displayName() + "'" ).c_str(),
                    showImageColormapWindow,
                    getNumImageColorMaps,
                    getImageColorMap,
                    getCurrentImageColormapIndex,
                    setCurrentImageColormapIndex,
                    updateImageUniforms );


        // Colormap preview:
        const float contentWidth = ImGui::GetContentRegionAvail().x;
        const float height = ( ImGui::GetIO().Fonts->Fonts[0]->FontSize * ImGui::GetIO().FontGlobalScale );

        char label[128];
        if ( const ImageColorMap* cmap = getImageColorMap( getCurrentImageColormapIndex() ) )
        {
            sprintf( label, "%s##cmap_%lu", cmap->name().c_str(), imageIndex );

            ImGui::paletteButton(
                        label, static_cast<int>( cmap->numColors() ),
                        cmap->data_RGBA_F32(),
                        imgSettings.isColorMapInverted(),
                        ImVec2( contentWidth, height ) );

            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "%s", cmap->description().c_str() );
            }
        }

//        ImGui::Dummy( ImVec2( 0.0f, 1.0f ) );


        // Edge settings
        ImGui::Separator();

        // Show edges:
        bool showEdges = imgSettings.showEdges();
        if ( ImGui::Checkbox( "Show edges", &showEdges ) )
        {
            imgSettings.setShowEdges( showEdges );
            updateImageUniforms();
        }
        ImGui::SameLine(); helpMarker( "Show/hide the edges of the image (E)" );


        if ( showEdges )
        {
            // Recommend linear interpolation:
            if ( InterpolationMode::NearestNeighbor == imgSettings.interpolationMode() )
            {
                ImGui::Text( "Note: Linear interpolation is recommended when showing edges." );
                //                    imgSettings.setInterpolationMode( InterpolationMode::Linear );
                //                    updateImageInterpolationMode();
            }


            // Threshold edges:
            bool thresholdEdges = imgSettings.thresholdEdges();
            if ( ImGui::Checkbox( "Hard edges", &thresholdEdges ) )
            {
                imgSettings.setThresholdEdges( thresholdEdges );
                updateImageUniforms();
            }
            ImGui::SameLine(); helpMarker( "Apply thresholding to edge gradient magnitude to get hard edges" );



            //                // Windowed edges:
            //                bool windowedEdges = imgSettings.windowedEdges();
            //                if ( ImGui::Checkbox( "Compute edges after windowing", &windowedEdges ) )
            //                {
            //                    imgSettings.setWindowedEdges( windowedEdges );
            //                    updateImageUniforms();
            //                }
            //                ImGui::SameLine();
            //                HelpMarker( "Compute edges after applying windowing (width/level) to the image" );


            // Use Sobel or Frei-Chen:
            //                bool useFreiChen = imgSettings.useFreiChen();
            //                if ( ImGui::Checkbox( "Frei-Chen filter", &useFreiChen ) )
            //                {
            //                    imgSettings.setUseFreiChen( useFreiChen );
            //                    updateImageUniforms();
            //                }
            //                ImGui::SameLine();
            //                HelpMarker( "Compute edges using Sobel or Frei-Chen convolution filters" );


            // Overlay edges:
            bool overlayEdges = imgSettings.overlayEdges();
            if ( ImGui::Checkbox( "Overlay edges on image", &overlayEdges ) )
            {
                if ( imgSettings.colormapEdges() )
                {
                    // Do not allow edge overlay if edges are colormapped
                    overlayEdges = false;
                }

                imgSettings.setOverlayEdges( overlayEdges );
                updateImageUniforms();
            }
            ImGui::SameLine(); helpMarker( "Overlay edges on top of the image" );


            // Colormap the edges (always false if overlaying the edges or thresholding the edges):
            if ( overlayEdges || thresholdEdges )
            {
                imgSettings.setColormapEdges( false );
                updateImageUniforms();
            }


            bool colormapEdges = imgSettings.colormapEdges();

            if ( ! overlayEdges && ! thresholdEdges )
            {
                if ( ImGui::Checkbox( "Apply colormap to edges", &colormapEdges ) )
                {
                    if ( overlayEdges )
                    {
                        colormapEdges = false;
                    }

                    imgSettings.setColormapEdges( colormapEdges );
                    updateImageUniforms();
                }
                ImGui::SameLine(); helpMarker( "Apply the image colormap to image edges" );
            }


            if ( ! colormapEdges )
            {
                glm::vec4 edgeColor{ imgSettings.edgeColor(), imgSettings.edgeOpacity() };

                if ( ImGui::ColorEdit4( "Edge color", glm::value_ptr( edgeColor ), sk_colorAlphaEditFlags ) )
                {
                    imgSettings.setEdgeColor( edgeColor );
                    imgSettings.setEdgeOpacity( edgeColor.a );
                    updateImageUniforms();
                }
                ImGui::SameLine(); helpMarker( "Edge color and opacity" );
            }
            else
            {
                // Cannot overlay edges with colormapping enabled
                imgSettings.setOverlayEdges( false );
                updateImageUniforms();
            }


            // Edge magnitude (only shown if thresholding edges):
            if ( thresholdEdges )
            {
                double edgeMag = imgSettings.edgeMagnitude();
                if ( mySliderF64( "Magnitude", &edgeMag, 0.01, 1.00 ) )
                {
                    imgSettings.setEdgeMagnitude( edgeMag );
                    updateImageUniforms();
                }
                ImGui::SameLine(); helpMarker( "Magnitude of threshold above which hard edges are shown" );
            }
            else
            {
                double edgeMag = 1.0 - imgSettings.edgeMagnitude();
                if ( mySliderF64( "Scale", &edgeMag, 0.01, 1.00 ) )
                {
                    imgSettings.setEdgeMagnitude( 1.0 - edgeMag );
                    updateImageUniforms();
                }
                ImGui::SameLine(); helpMarker( "Scale applied to edge magnitude" );
            }
        }

        ImGui::Separator();
        ImGui::TreePop();
    }

    if ( ImGui::TreeNode( "Transformations" ) )
    {
        ImGui::Text( "Initial affine transformation:" );
        ImGui::SameLine();
        helpMarker( "Initial affine transformation matrix (read from file)" );


        bool enable_affine_T_subject = imgTx.get_enable_affine_T_subject();
        if ( ImGui::Checkbox( "Enabled##affine_T_subject", &enable_affine_T_subject ) )
        {
            imgTx.set_enable_affine_T_subject( enable_affine_T_subject );
            updateImageUniforms();
        }
        ImGui::SameLine();
        helpMarker( "Enable/disable application of the initial affine transformation" );

        if ( enable_affine_T_subject )
        {
            if ( auto fileName = imgTx.get_affine_T_subject_fileName() )
            {
                ImGui::InputText( "File", &( *fileName ), ImGuiInputTextFlags_ReadOnly );
                ImGui::Spacing();
            }

            glm::mat4 aff_T_sub = glm::transpose( imgTx.get_affine_T_subject() );

            ImGui::PushItemWidth( -1 );
            ImGui::InputFloat4( "", glm::value_ptr( aff_T_sub[0] ), txFormat, ImGuiInputTextFlags_ReadOnly );
            ImGui::InputFloat4( "", glm::value_ptr( aff_T_sub[1] ), txFormat, ImGuiInputTextFlags_ReadOnly );
            ImGui::InputFloat4( "", glm::value_ptr( aff_T_sub[2] ), txFormat, ImGuiInputTextFlags_ReadOnly );
            ImGui::InputFloat4( "", glm::value_ptr( aff_T_sub[3] ), txFormat, ImGuiInputTextFlags_ReadOnly );
            ImGui::PopItemWidth();

            ImGui::Spacing();
        }
        ImGui::Separator();


        ImGui::Text( "Manual affine transformation:" );
        ImGui::SameLine(); helpMarker( "Manual affine transformation from Subject to World space" );


        bool enable_worldDef_T_affine = imgTx.get_enable_worldDef_T_affine();
        if ( ImGui::Checkbox( "Enabled##worldDef_T_affine", &enable_worldDef_T_affine ) )
        {
            imgTx.set_enable_worldDef_T_affine( enable_worldDef_T_affine );
            updateImageUniforms();
        }
        ImGui::SameLine();
        helpMarker( "Enable/disable application of the manual affine transformation from Subject to World space" );


        if ( enable_worldDef_T_affine )
        {
            glm::quat w_T_s_rotation = imgTx.get_worldDef_T_affine_rotation();
            glm::vec3 w_T_s_scale = imgTx.get_worldDef_T_affine_scale();
            glm::vec3 w_T_s_trans = imgTx.get_worldDef_T_affine_translation();

            float angle = glm::degrees( glm::angle( w_T_s_rotation ) );
            glm::vec3 axis = glm::normalize( glm::axis( w_T_s_rotation ) );

            if ( ImGui::InputFloat3( "Translation", glm::value_ptr( w_T_s_trans ), txFormat ) )
            {
                imgTx.set_worldDef_T_affine_translation( w_T_s_trans );
                updateImageUniforms();
            }
            ImGui::SameLine();
            helpMarker( "Image translation in x, y, z" );

            if ( ImGui::InputFloat3( "Scale", glm::value_ptr( w_T_s_scale ), txFormat ) )
            {
                if ( glm::epsilonNotEqual( w_T_s_scale.x, 0.0f, glm::epsilon<float>() ) &&
                     glm::epsilonNotEqual( w_T_s_scale.y, 0.0f, glm::epsilon<float>() ) &&
                     glm::epsilonNotEqual( w_T_s_scale.z, 0.0f, glm::epsilon<float>() ) )
                {
                    imgTx.set_worldDef_T_affine_scale( w_T_s_scale );
                    updateImageUniforms();
                }
            }
            ImGui::SameLine(); helpMarker( "Image scale in x, y, z" );

            /// @todo Put in a more friendly rotation widget. For now, disable changing the rotation
            /// @see https://github.com/BrutPitt/imGuIZMO.quat
            /// @see https://github.com/CedricGuillemet/ImGuizmo

            //            ImGui::Text( "Rotation" );
            if ( ImGui::InputFloat( "Rotation angle", &angle, 0.01f, 0.1f, txFormat ) )
            {
                //                const float n = glm::length( axis );
                //                if ( n < 1e-6f )
                //                {
                //                    const glm::quat newRot = glm::angleAxis( glm::radians( angle ), glm::normalize( axis ) );
                //                    imgTx.set_worldDef_T_affine_rotation( newRot );
                //                    updateImageUniforms();
                //                }
            }
            ImGui::SameLine(); helpMarker( "Image rotation angle (degrees) [editing disabled]" );

            if ( ImGui::InputFloat3( "Rotation axis", glm::value_ptr( axis ), txFormat ) )
            {
                //                const float n = glm::length( axis );
                //                if ( n < 1e-6f )
                //                {
                //                    const glm::quat newRot = glm::angleAxis( glm::radians( angle ), glm::normalize( axis ) );
                //                    imgTx.set_worldDef_T_affine_rotation( newRot );
                //                    updateImageUniforms();
                //                }
            }
            ImGui::SameLine(); helpMarker( "Image rotation axis [editing disabled]" );

            //            if ( ImGui::InputFloat4( "Rotation", glm::value_ptr( w_T_s_rotation ), "%.3f" ) )
            //            {
            //                imgTx.set_worldDef_T_affine_rotation( w_T_s_rotation );
            //                updateImageUniforms();
            //            }
            //            ImGui::SameLine();
            //            HelpMarker( "Image rotation defined as a quaternion" );


            ImGui::Spacing();
            glm::mat4 world_T_affine = glm::transpose( imgTx.get_worldDef_T_affine() );

            ImGui::PushItemWidth( -1 );
            ImGui::Text( "Subject-to-World matrix:" );
            ImGui::InputFloat4( "", glm::value_ptr( world_T_affine[0] ), txFormat, ImGuiInputTextFlags_ReadOnly );
            ImGui::InputFloat4( "", glm::value_ptr( world_T_affine[1] ), txFormat, ImGuiInputTextFlags_ReadOnly );
            ImGui::InputFloat4( "", glm::value_ptr( world_T_affine[2] ), txFormat, ImGuiInputTextFlags_ReadOnly );
            ImGui::InputFloat4( "", glm::value_ptr( world_T_affine[3] ), txFormat, ImGuiInputTextFlags_ReadOnly );
            ImGui::PopItemWidth();


            ImGui::Spacing();
            if ( ImGui::Button( "Reset to identity" ) )
            {
                imgTx.reset_worldDef_T_affine();
                updateImageUniforms();
            }
            ImGui::SameLine(); helpMarker( "Reset the manual component of the affine transformation from Subject to World space" );


            // Save manual tx to file:
            static const char* sk_buttonText( "Save manual transformation..." );
            static const char* sk_dialogTitle( "Select Manual Transformation" );
            static const std::vector< const char* > sk_dialogFilters{};

            const auto selectedFile = ImGui::renderFileButtonDialogAndWindow(
                        sk_buttonText, sk_dialogTitle, sk_dialogFilters );

            ImGui::SameLine(); helpMarker( "Save the manual component of the affine transformation matrix from Subject to World space" );

            if ( selectedFile )
            {
                const glm::dmat4 worldDef_T_affine{ imgTx.get_worldDef_T_affine() };

                if ( serialize::saveAffineTxFile( worldDef_T_affine, *selectedFile ) )
                {
                    spdlog::info( "Saved manual transformation matrix to file {}", *selectedFile );
                }
                else
                {
                    spdlog::error( "Error saving manual transformation matrix to file {}", *selectedFile );
                }
            }
        }

        ImGui::Separator();
        ImGui::TreePop();
    }

    if ( ImGui::TreeNode( "Header Information" ) )
    {
        renderImageHeaderInformation( appData, imgHeader, imgSettings, imgTx );
        ImGui::TreePop();
    }

    ImGui::Spacing();

    ImGui::PopID(); // imageUid
}


void renderSegmentationHeader(
        AppData& appData,
        const uuids::uuid& imageUid,
        size_t imageIndex,
        Image* image,
        bool isActiveImage,
        const std::function< void( void ) >& updateImageUniforms,
        const std::function< ParcellationLabelTable* ( size_t tableIndex ) >& getLabelTable,
        const std::function< void ( size_t tableIndex ) >& updateLabelColorTableTexture,
        const std::function< void ( size_t labelIndex ) >& moveCrosshairsToSegLabelCentroid,
        const std::function< std::optional<uuids::uuid>( const uuids::uuid& matchingImageUid, const std::string& segDisplayName ) >& createBlankSeg,
        const std::function< bool( const uuids::uuid& segUid ) >& clearSeg,
        const std::function< bool( const uuids::uuid& segUid ) >& removeSeg )
{
    static const std::string sk_addNewSegString = std::string( ICON_FK_FILE_O ) + std::string( " Create" );
    static const std::string sk_clearSegString = std::string( ICON_FK_ERASER ) + std::string( " Clear" );
    static const std::string sk_removeSegString = std::string( ICON_FK_TRASH_O ) + std::string( " Remove" );
    static const std::string sk_SaveSegString = std::string( ICON_FK_FLOPPY_O ) + std::string( " Save..." );

    if ( ! image )
    {
        spdlog::error( "Null image" );
        return;
    }

    const ImVec4* colors = ImGui::GetStyle().Colors;
    const ImVec4 activeColor = colors[ImGuiCol_ButtonActive];

    ImGuiTreeNodeFlags headerFlags = ImGuiTreeNodeFlags_CollapsingHeader;

    if ( isActiveImage )
    {
        // Open header for the active image by default:
        headerFlags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    ImGui::PushID( uuids::to_string( imageUid ).c_str() ); /*** PushID imageUid ***/

    auto& imgSettings = image->settings();

    // Header is ID'ed only by the image index.
    // ### allows the header name to change without changing its ID.
    const std::string headerName =
            std::to_string( imageIndex ) + ") " +
            imgSettings.displayName() +
            "###" + std::to_string( imageIndex );

    const auto headerColors = computeHeaderBgAndTextColors( imgSettings.borderColor() );
    ImGui::PushStyleColor( ImGuiCol_Header, headerColors.first );
    ImGui::PushStyleColor( ImGuiCol_Text, headerColors.second );

    const bool open = ImGui::CollapsingHeader( headerName.c_str(), headerFlags );

    ImGui::PopStyleColor( 2 ); // ImGuiCol_Header, ImGuiCol_Text


    if ( ! open )
    {
        ImGui::PopID(); /*** PopID imageUid ***/
        return;
    }

    ImGui::Spacing();

    if ( ! isActiveImage )
    {
        if ( ImGui::Button( ICON_FK_TOGGLE_OFF ) )
        {
            if ( appData.setActiveImageUid( imageUid ) ) return;
        }
        if ( ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "Make this the active image" );
        }
    }
    else
    {
        ImGui::PushStyleColor( ImGuiCol_Button, activeColor );
        ImGui::Button( ICON_FK_TOGGLE_ON );
        ImGui::PopStyleColor( 1 ); // ImGuiCol_Button
    }


    const bool isRef = ( 0 == imageIndex );

    ImGui::SameLine();

    if ( isRef && isActiveImage )
    {
        ImGui::Text( "%s", sk_referenceAndActiveImageMessage.c_str() );
    }
    else if ( isRef )
    {
        ImGui::Text( "%s", sk_referenceImageMessage.c_str() );
    }
    else if ( isActiveImage )
    {
        ImGui::Text( "%s", sk_activeImageMessage.c_str() );
    }
    else
    {
        ImGui::Text( "%s", sk_nonActiveImageMessage.c_str() );
    }


    const auto segUids = appData.imageToSegUids( imageUid );
    if ( segUids.empty() )
    {
        ImGui::Text( "This image has no segmentation" );
        spdlog::error( "Image {} has no segmentations", imageUid );
        return;
    }

    auto activeSegUid = appData.imageToActiveSegUid( imageUid );

    if ( ! activeSegUid )
    {
        spdlog::error( "Image {} has no active segmentation", imageUid );
        return;
    }

    Image* activeSeg = appData.seg( *activeSegUid );

    if ( ! activeSeg )
    {
        spdlog::error( "Active segmentation for image {} is null", imageUid );
        return;
    }

    ImGui::Separator();
    ImGui::Text( "Active segmentation:" );

//    ImGui::PushItemWidth( -1 );
    if ( ImGui::BeginCombo( "", activeSeg->settings().displayName().c_str() ) )
    {
        size_t segIndex = 0;
        for ( const auto& segUid : segUids )
        {
            ImGui::PushID( static_cast<int>( segIndex++ ) );
            {
                if ( Image* seg = appData.seg( segUid ) )
                {
                    const bool isSelected = ( segUid == *activeSegUid );

                    if ( ImGui::Selectable( seg->settings().displayName().c_str(), isSelected ) )
                    {
                        appData.assignActiveSegUidToImage( imageUid, segUid );
                        activeSeg = appData.seg( segUid );
                        updateImageUniforms();
                    }

                    if ( isSelected ) ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::PopID(); // lmGroupIndex
        }
        ImGui::EndCombo();
    }
//    ImGui::PopItemWidth();
    ImGui::SameLine(); helpMarker( "Select the active segmentation for this image" );


    /// @todo Add button for copying the segmentation to a new segmentation

    // Add segmentation:
    if ( ImGui::Button( sk_addNewSegString.c_str() ) )
    {
        const size_t numSegsForImage = appData.imageToSegUids( imageUid ).size();

        std::string segDisplayName =
                std::string( "Untitled segmentation " ) +
                std::to_string( numSegsForImage + 1 ) +
                " for image '" +
                image->settings().displayName() + "'";

        createBlankSeg( imageUid, std::move( segDisplayName ) );
        updateImageUniforms();
    }
    if ( ImGui::IsItemHovered() )
    {
        ImGui::SetTooltip( "Create a new blank segmentation for this image" );
    }


    // Remove segmentation:
    // (Do not allow removal of the segmentation if it the only one for this image)
    if ( appData.imageToSegUids( imageUid ).size() > 1 )
    {
        ImGui::SameLine();
        if ( ImGui::Button( sk_removeSegString.c_str() ) )
        {
            if ( removeSeg( *activeSegUid ) )
            {
                updateImageUniforms();
                return;
            }
        }
        if ( ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "Remove this segmentation from the image" );
        }
    }


    // Clear segmentation:
    ImGui::SameLine();
    if ( ImGui::Button( sk_clearSegString.c_str() ) )
    {
        clearSeg( *activeSegUid );
    }
    if ( ImGui::IsItemHovered() )
    {
        ImGui::SetTooltip( "Clear all values in this segmentation" );
    }


    // Save segmentation:
    static const char* sk_dialogTitle( "Select Segmentation Image" );
    static const std::vector< const char* > sk_dialogFilters{};

    ImGui::SameLine();
    const auto selectedFile = ImGui::renderFileButtonDialogAndWindow(
                sk_SaveSegString.c_str(), sk_dialogTitle, sk_dialogFilters );

    if ( ImGui::IsItemHovered() )
    {
        ImGui::SetTooltip( "Save the segmentation to an image file on disk" );
    }

    if ( selectedFile )
    {
        if ( activeSeg->saveToDisk( *selectedFile ) )
        {
            spdlog::info( "Saved segmentation image to file {}", *selectedFile );
            activeSeg->header().setFileName( *selectedFile );
        }
        else
        {
            spdlog::error( "Error saving segmentation image to file {}", *selectedFile );
        }
    }

    ImGui::Separator();

    // Double check that we still have the active segmentation:
    if ( ! activeSeg )
    {
        spdlog::error( "Active segmentation for image {} is null", imageUid );
        return;
    }

    const auto& segHeader = activeSeg->header();
    const auto& segTx = activeSeg->transformations();
    auto& segSettings = activeSeg->settings();

    // Header is ID'ed only by "seg_" and the image index.
    // ### allows the header name to change without changing its ID.
    const std::string segHeaderName =
            std::string( "Seg: " ) +
            segSettings.displayName() +
            "###seg_" + std::to_string( imageIndex );

    /// @todo add "*" to end of name and change color of seg header if seg has been modified

    ImGui::Spacing();


    // Open segmentation View Properties on first appearance
    ImGui::SetNextItemOpen( true, ImGuiCond_Appearing );

    if ( ImGui::TreeNode( "View Properties" ) )
    {
        // Visibility:
        bool segVisible = segSettings.visibility();
        if ( ImGui::Checkbox( "Visible", &segVisible ) )
        {
            segSettings.setVisibility( segVisible );
            updateImageUniforms();
        }
        ImGui::SameLine(); helpMarker( "Show/hide the segmentation on all views (S)" );

//        if ( segVisible )
        {
            // Opacity (only shown if segmentation is visible):
            double segOpacity = segSettings.opacity();
            if ( mySliderF64( "Opacity", &segOpacity, 0.0, 1.0 ) )
            {
                segSettings.setOpacity( segOpacity );
                updateImageUniforms();
            }
            ImGui::SameLine(); helpMarker( "Segmentation layer opacity" );
        }

        ImGui::Separator();
        ImGui::TreePop();
    }

    if ( ImGui::TreeNode( "Segmentation Labels" ) )
    {
        renderSegLabelsChildWindow(
                    segSettings.labelTableIndex(),
                    getLabelTable( segSettings.labelTableIndex() ),
                    updateLabelColorTableTexture,
                    moveCrosshairsToSegLabelCentroid );

        ImGui::Separator();
        ImGui::TreePop();
    }

    if ( ImGui::TreeNode( "Header Information" ) )
    {
        renderImageHeaderInformation( appData, segHeader, segSettings, segTx );

        ImGui::Separator();
        ImGui::TreePop();
    }

    ImGui::Spacing();

    ImGui::PopID(); /*** PopID imageUid ***/
}


void renderLandmarkGroupHeader(
        AppData& appData,
        const uuids::uuid& imageUid,
        size_t imageIndex,
        bool isActiveImage,
        const AllViewsRecenterType& recenterAllViews )
{
    static const char* sk_newLmGroupButtonText( "Create new group of landmarks" );
    static const char* sk_saveLmsButtonText( "Save landmarks..." );
    static const char* sk_saveLmsDialogTitle( "Save Landmark Group" );
    static const std::vector< const char* > sk_saveLmsDialogFilters{};

    Image* image = appData.image( imageUid );
    if ( ! image ) return;


    auto addNewLmGroupButton = [&appData, &image, &imageUid] ()
    {
        if ( ImGui::Button( sk_newLmGroupButtonText ) )
        {
            LandmarkGroup newGroup;
            newGroup.setName( std::string( "Landmarks for " ) + image->settings().displayName() );

            const auto newLmGroupUid = appData.addLandmarkGroup( std::move( newGroup ) );
            appData.assignLandmarkGroupUidToImage( imageUid, newLmGroupUid );
            appData.setRainbowColorsForAllLandmarkGroups();
            appData.assignActiveLandmarkGroupUidToImage( imageUid, newLmGroupUid );
        }
    };


    ImGuiTreeNodeFlags headerFlags = ImGuiTreeNodeFlags_CollapsingHeader;

    /// @todo This annoyingly pops up the active header each time... not sure why
    if ( isActiveImage )
    {
        headerFlags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    ImGui::PushID( uuids::to_string( imageUid ).c_str() ); /** PushID imageUid **/

    // Header is ID'ed only by the image index.
    // ### allows the header name to change without changing its ID.
    const std::string headerName =
            std::to_string( imageIndex ) + ") " +
            image->settings().displayName() +
            "###" + std::to_string( imageIndex );

    const auto imgSettings = image->settings();

    const auto headerColors = computeHeaderBgAndTextColors( imgSettings.borderColor() );
    ImGui::PushStyleColor( ImGuiCol_Header, headerColors.first );
    ImGui::PushStyleColor( ImGuiCol_Text, headerColors.second );

    const bool open = ImGui::CollapsingHeader( headerName.c_str(), headerFlags );

    ImGui::PopStyleColor( 2 ); // ImGuiCol_Header, ImGuiCol_Text

    if ( ! open )
    {
        ImGui::PopID(); // imageUid
        return;
    }

    ImGui::Spacing();

    const auto lmGroupUids = appData.imageToLandmarkGroupUids( imageUid );

    if ( lmGroupUids.empty() )
    {
        ImGui::Text( "This image has no landmarks." );
        addNewLmGroupButton();
        ImGui::PopID(); // imageUid
        return;
    }

    // Show a combo box if there are multiple landmark groups
    const bool showLmGroupCombo = ( lmGroupUids.size() > 1 );

    std::optional<uuids::uuid> activeLmGroupUid =
            appData.imageToActiveLandmarkGroupUid( imageUid );

    // The default active landmark group is at index 0
    if ( ! activeLmGroupUid )
    {
        if ( appData.assignActiveLandmarkGroupUidToImage( imageUid, lmGroupUids[0] ) )
        {
            activeLmGroupUid = appData.imageToActiveLandmarkGroupUid( imageUid );
        }
        else
        {
            spdlog::error( "Unable to assign active landmark group {} to image {}",
                           lmGroupUids[0], imageUid );
            ImGui::PopID(); // imageUid
            return;
        }
    }

    LandmarkGroup* activeLmGroup = appData.landmarkGroup( *activeLmGroupUid );

    if ( ! activeLmGroup )
    {
        spdlog::error( "Landmark group {} for image {} is null", *activeLmGroupUid, imageUid );
        ImGui::PopID(); // imageUid
        return;
    }

    if ( showLmGroupCombo )
    {
        if ( ImGui::BeginCombo( "Landmark group", activeLmGroup->getName().c_str() ) )
        {
            size_t lmGroupIndex = 0;
            for ( const auto& lmGroupUid : lmGroupUids )
            {
                ImGui::PushID( static_cast<int>( lmGroupIndex++ ) );

                if ( LandmarkGroup* lmGroup = appData.landmarkGroup( lmGroupUid ) )
                {
                    const bool isSelected = ( lmGroupUid == *activeLmGroupUid );

                    if ( ImGui::Selectable( lmGroup->getName().c_str(), isSelected) )
                    {
                        appData.assignActiveLandmarkGroupUidToImage( imageUid, lmGroupUid );
                        activeLmGroup = appData.landmarkGroup( lmGroupUid );
                    }

                    if ( isSelected ) ImGui::SetItemDefaultFocus();
                }

                ImGui::PopID(); // lmGroupIndex
            }

            ImGui::EndCombo();
        }

        ImGui::SameLine();
        helpMarker( "Select the group of landmarks to view" );

        ImGui::Separator();
    }

    if ( ! activeLmGroup )
    {
        spdlog::error( "Active landmark group for image {} is null", imageUid );
        ImGui::PopID(); // imageUid
        return;
    }


    // Landmark group display name:
    std::string groupName = activeLmGroup->getName();
    if ( ImGui::InputText( "Name", &groupName ) )
    {
        activeLmGroup->setName( groupName );
    }
    ImGui::SameLine(); helpMarker( "Edit the name of the group of landmarks" );


    // Landmark group file name:
    std::string fileName = activeLmGroup->getFileName();
    ImGui::InputText( "File", &fileName, ImGuiInputTextFlags_ReadOnly );
    ImGui::SameLine(); helpMarker( "Comma-separated file with the landmarks" );
    ImGui::Spacing();


    // Visibility checkbox:
    bool groupVisible = activeLmGroup->getVisibility();
    if ( ImGui::Checkbox( "Visible", &groupVisible ) )
    {
        activeLmGroup->setVisibility( groupVisible );
    }
    ImGui::SameLine(); helpMarker( "Show/hide the landmarks" );


    // Opacity slider:
    float groupOpacity = activeLmGroup->getOpacity();
    if ( mySliderF32( "Opacity", &groupOpacity, 0.0f, 1.0f ) )
    {
        activeLmGroup->setOpacity( groupOpacity );
    }
    ImGui::SameLine(); helpMarker( "Landmark opacity" );


    // Radius slider:
    float groupRadius = 100.0f * activeLmGroup->getRadiusFactor();
    if ( mySliderF32( "Radius", &groupRadius, 0.1f, 10.0f ) )
    {
        activeLmGroup->setRadiusFactor( groupRadius / 100.0f );
    }
    ImGui::SameLine(); helpMarker( "Landmark circle radius" );
    ImGui::Spacing();


    // Rendering of landmark indices:
    bool renderLandmarkIndices = activeLmGroup->getRenderLandmarkIndices();
    if ( ImGui::Checkbox( "Show indices", &renderLandmarkIndices ) )
    {
        activeLmGroup->setRenderLandmarkIndices( renderLandmarkIndices );
    }
    ImGui::SameLine(); helpMarker( "Show/hide the landmark indices" );


    // Rendering of landmark indices:
    bool renderLandmarkNames = activeLmGroup->getRenderLandmarkNames();
    if ( ImGui::Checkbox( "Show names", &renderLandmarkNames ) )
    {
        activeLmGroup->setRenderLandmarkNames( renderLandmarkNames );
    }
    ImGui::SameLine(); helpMarker( "Show/hide the landmark names" );


    // Uniform color for all landmarks:
    bool hasGroupColor = activeLmGroup->getColorOverride();

    if ( ImGui::Checkbox( "Global color", &hasGroupColor ) )
    {
        activeLmGroup->setColorOverride( hasGroupColor );
    }

    if ( hasGroupColor )
    {
        auto groupColor = activeLmGroup->getColor();

        ImGui::SameLine();
        if ( ImGui::ColorEdit3( "##uniformColor", glm::value_ptr( groupColor ), sk_colorEditFlags ) )
        {
            activeLmGroup->setColor( groupColor );
        }
    }
    ImGui::SameLine(); helpMarker( "Set a global color for all landmarks in this group" );


    // Text color for all landmarks:
    if ( activeLmGroup->getTextColor().has_value() )
    {
        auto textColor = activeLmGroup->getTextColor().value();
        if ( ImGui::ColorEdit3( "Text color", glm::value_ptr( textColor ), sk_colorEditFlags ) )
        {
            activeLmGroup->setTextColor( textColor );
        }
        ImGui::SameLine(); helpMarker( "Set text color for all landmarks" );
        ImGui::Spacing();
    }


    // Voxel vs physical space radio buttons:
    ImGui::Spacing();
    ImGui::Text( "Landmark coordinate space:" );
    int inVoxelSpace = activeLmGroup->getInVoxelSpace() ? 1 : 0;

    if ( ImGui::RadioButton( "Physical subject (mm)", &inVoxelSpace, 0 ) )
    {
        activeLmGroup->setInVoxelSpace( ( 1 == inVoxelSpace ) ? true : false );
    }

    ImGui::SameLine();
    if ( ImGui::RadioButton( "Voxels", &inVoxelSpace, 1 ) )
    {
        activeLmGroup->setInVoxelSpace( ( 1 == inVoxelSpace ) ? true : false );
    }

    ImGui::SameLine(); helpMarker( "Space in which landmark coordinates are defined" );
    ImGui::Spacing();


    // Child window for points:
    ImGui::Dummy( ImVec2( 0.0f, 4.0f ) );

    auto setWorldCrosshairsPos = [&appData] ( const glm::vec3& worldCrosshairsPos )
    {
        appData.state().setWorldCrosshairsPos( worldCrosshairsPos );
    };

    renderLandmarkChildWindow(
                appData,
                image->transformations(),
                activeLmGroup,
                appData.state().worldCrosshairs().worldOrigin(),
                setWorldCrosshairsPos,
                recenterAllViews );



    ImGui::Separator();

    addNewLmGroupButton();


    // Save landmarks to CSV and save settings to project file:
    const auto selectedFile = ImGui::renderFileButtonDialogAndWindow(
                sk_saveLmsButtonText, sk_saveLmsDialogTitle, sk_saveLmsDialogFilters );

    ImGui::SameLine(); helpMarker( "Save the landmarks to a CSV file" );

    if ( selectedFile )
    {
        if ( serialize::saveLandmarksFile( activeLmGroup->getPoints(), *selectedFile ) )
        {
            spdlog::info( "Saved landmarks to CSV file {}", *selectedFile );

            /// @todo How to handle changing the file name?
            activeLmGroup->setFileName( *selectedFile );
        }
        else
        {
            spdlog::error( "Error saving landmarks to CSV file {}", *selectedFile );
        }
    }

    ImGui::Spacing();

    ImGui::PopID(); /** PopID imageUid **/
}


void renderAnnotationsHeader(
        AppData& appData,
        const uuids::uuid& imageUid,
        size_t imageIndex,
        bool isActiveImage,
        const std::function< void ( const uuids::uuid& viewUid, const glm::vec3& worldFwdDirection ) >& setViewDirection,
        const AllViewsRecenterType& recenterAllViews )
{
    static constexpr bool sk_doNotRecenterCrosshairs = false;
    static constexpr bool sk_recenterOnCurrentCrosshairsPosition = true;
    static constexpr bool sk_doNotResetObliqueOrientation = false;

    static const ImGuiColorEditFlags sk_annotColorEditFlags =
            ImGuiColorEditFlags_PickerHueBar |
            ImGuiColorEditFlags_DisplayRGB |
            ImGuiColorEditFlags_DisplayHex |
            ImGuiColorEditFlags_AlphaBar |
            ImGuiColorEditFlags_AlphaPreviewHalf |
            ImGuiColorEditFlags_Uint8 |
            ImGuiColorEditFlags_InputRGB;

    static const char* sk_saveAnnotButtonText( "Save..." );
    static const char* sk_saveAnnotDialogTitle( "Save Annotation to SVG" );
    static const std::vector< const char* > sk_saveAnnotDialogFilters{};

    Image* image = appData.image( imageUid );
    if ( ! image ) return;

    // Move crosshairs to the annotation centroid:
    auto moveCrosshairsToAnnotationCenter = [&appData, &image] ( const Annotation* annot )
    {
        if ( ! annot ) return;

        const glm::vec4 subjectCentroid{ annot->unprojectFromAnnotationPlaneToSubjectPoint(
                        annot->polygon().getCentroid() ), 1.0f };

        const glm::vec4 worldCentroid = image->transformations().worldDef_T_subject() * subjectCentroid;

        appData.state().setWorldCrosshairsPos( glm::vec3{ worldCentroid / worldCentroid.w } );
    };

    // Find a view with normal vector maching the annotation plane.
    // Todo: make this view active.
    // If none found, make the largest view oblique and align it to the annotation.
    auto alignViewToAnnotationPlane = [&appData, &imageUid, &image, &setViewDirection]
            ( const Annotation* annot )
    {
        if ( ! annot ) return;

        const glm::mat3 world_T_subject_invTranspose = glm::inverseTranspose(
                    glm::mat3{ image->transformations().worldDef_T_subject() } );

        const glm::vec3 worldAnnotNormal = glm::normalize(
                    world_T_subject_invTranspose *
                    glm::vec3{ annot->getSubjectPlaneEquation() } );

        // Does the current layout have a view with this orientaion?
        const auto viewsWithNormal = appData.windowData().findCurrentViewsWithNormal( worldAnnotNormal );

        if ( viewsWithNormal.empty() )
        {
            const uuids::uuid largestCurrentViewUid = appData.windowData().findLargestCurrentView();

            if ( View* view = appData.windowData().getCurrentView( largestCurrentViewUid ) )
            {
                // Rather than check if the plane of the annotation (which, recall is defined in
                // Subject space) is aligned with either an axial, coronal, or sagittal view,
                // we simple set the view to oblique.
                view->setCameraType( camera::CameraType::Oblique );
                setViewDirection( largestCurrentViewUid, worldAnnotNormal );

                // Render the image in this view if not currently rendered:
                if ( ! view->isImageRendered( imageUid ) )
                {
                    view->setImageRendered( appData, imageUid, true );
                }

                spdlog::trace( "Changed view {} normal direction to {}",
                               largestCurrentViewUid, glm::to_string( worldAnnotNormal ) );
            }
            else
            {
                spdlog::error( "Unable to orient a view to the annotation plane" );
            }
        }
    };


    ImGuiTreeNodeFlags headerFlags = ImGuiTreeNodeFlags_CollapsingHeader;

    /// @todo This annoyingly pops up the active header each time... not sure why
    if ( isActiveImage )
    {
        headerFlags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    ImGui::PushID( uuids::to_string( imageUid ).c_str() ); /** PushID imageUid **/

    // Header is ID'ed only by the image index.
    // ### allows the header name to change without changing its ID.
    const std::string headerName =
            std::to_string( imageIndex ) + ") " +
            image->settings().displayName() +
            "###" + std::to_string( imageIndex );

    const auto& imgSettings = image->settings();

    const auto headerColors = computeHeaderBgAndTextColors( imgSettings.borderColor() );
    ImGui::PushStyleColor( ImGuiCol_Header, headerColors.first );
    ImGui::PushStyleColor( ImGuiCol_Text, headerColors.second );

    const bool open = ImGui::CollapsingHeader( headerName.c_str(), headerFlags );

    ImGui::PopStyleColor( 2 ); // ImGuiCol_Header, ImGuiCol_Text

    if ( ! open )
    {
        ImGui::PopID(); // imageUid
        return;
    }

    ImGui::Spacing();

    const auto& annotUids = appData.annotationsForImage( imageUid );

    if ( annotUids.empty() )
    {
        ImGui::Text( "This image has no annotations." );
        ImGui::PopID(); // imageUid
        return;
    }


    auto activeAnnotUid = appData.imageToActiveAnnotationUid( imageUid );

    // The default active annotation is at index 0
    if ( ! activeAnnotUid )
    {
        if ( appData.assignActiveAnnotationUidToImage( imageUid, annotUids.front() ) )
        {
            activeAnnotUid = appData.imageToActiveAnnotationUid( imageUid );
        }
        else
        {
            spdlog::error( "Unable to assign active annotation {} to image {}", annotUids.front(), imageUid );
            ImGui::PopID(); // imageUid
            return;
        }
    }

    Annotation* activeAnnot = appData.annotation( *activeAnnotUid );
    size_t activeAnnotIndex = 0;

    if ( ! activeAnnot )
    {
        spdlog::error( "Annotation {} for image {} is null", *activeAnnotUid, imageUid );
        ImGui::PopID(); // imageUid
        return;
    }

    const ImVec4* colors = ImGui::GetStyle().Colors;
    ImGui::PushStyleColor( ImGuiCol_Header, colors[ImGuiCol_ButtonActive] );

    static constexpr size_t sk_minNumLines = 5;
    static constexpr size_t sk_maxNumLines = 10;

    const size_t numLines = std::max( std::min( annotUids.size(), sk_maxNumLines ), sk_minNumLines );

    /// @todo Change this into a child window, like for Landmarks.
    /// then do ImGui::SetScrollHereY( 1.0f ); to put activeAnnot at bottom

    const ImVec2 listBoxSize( -FLT_MIN, static_cast<float>(numLines) * ImGui::GetTextLineHeightWithSpacing() );

    if ( ImGui::BeginListBox( "##annotList", listBoxSize ) )
    {
        size_t annotIndex = 0;
        for ( const auto& annotUid : annotUids )
        {
            ImGui::PushID( static_cast<int>( annotIndex ) );

            if ( Annotation* annot = appData.annotation( annotUid ) )
            {
                bool isSelected = false;

                if ( annotUid == *activeAnnotUid )
                {
                    isSelected = true;
                    activeAnnotIndex = annotIndex;
                }

                /// @see Line 2791 of demo:
                /// ImGui::SetScrollHereY(i * 0.25f); // 0.0f:top, 0.5f:center, 1.0f:bottom

                const std::string text = annot->getDisplayName() +
                        " [" + data::getAnnotationSubjectPlaneName( *annot ) + "]";

                if ( ImGui::Selectable( text.c_str(), isSelected ) )
                {
                    // Make the annotation active and move crosshairs to it:
                    appData.assignActiveAnnotationUidToImage( imageUid, annotUid );
                    activeAnnot = appData.annotation( annotUid );

                    moveCrosshairsToAnnotationCenter( activeAnnot );
                    alignViewToAnnotationPlane( activeAnnot );

                    recenterAllViews( sk_doNotRecenterCrosshairs,
                                      sk_recenterOnCurrentCrosshairsPosition,
                                      sk_doNotResetObliqueOrientation );
                }

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if ( isSelected ) ImGui::SetItemDefaultFocus();
            }

            ImGui::PopID(); // lmGroupIndex

            ++annotIndex;
        }

        ImGui::EndListBox();
    }
    ImGui::PopStyleColor( 1 ); // ImGuiCol_Header

    if ( ! activeAnnot )
    {
        spdlog::error( "Active annotation for image {} is null", imageUid );
        ImGui::PopID(); // imageUid
        return;
    }

    ImGui::Separator();


    // Annotation display name:
    std::string displayName = activeAnnot->getDisplayName();
    if ( ImGui::InputText( "Name", &displayName ) )
    {
        activeAnnot->setDisplayName( displayName );
    }
    ImGui::SameLine(); helpMarker( "Edit the name of the annotation" );


    // Annotation file name:
    std::string fileName = activeAnnot->getFileName();
    ImGui::InputText( "File", &fileName, ImGuiInputTextFlags_ReadOnly );
    ImGui::SameLine(); helpMarker( "File storing the annotation in Scalar Vector Graphics (SVG) format" );



    // Remove the annotation:
    bool removeAnnot = false;
    static bool doNotAskAagain = false;

    const bool clickedRemoveButton = ImGui::Button( "Remove" );

    if ( ImGui::IsItemHovered() )
    {
        ImGui::SetTooltip( "Remove the annotation. The file will not be deleted." );
    }

    if ( clickedRemoveButton )
    {
        if ( ! doNotAskAagain && ! ImGui::IsPopupOpen( "Remove Annotation" ) )
        {
            ImGui::OpenPopup( "Remove Annotation", ImGuiWindowFlags_AlwaysAutoResize );
        }
        else if ( doNotAskAagain )
        {
            removeAnnot = true;
        }
    }

    const ImVec2 center( ImGui::GetIO().DisplaySize.x * 0.5f,
                         ImGui::GetIO().DisplaySize.y * 0.5f );

    ImGui::SetNextWindowPos( center, ImGuiCond_Appearing, ImVec2( 0.5f, 0.5f ) );

    if ( ImGui::BeginPopupModal( "Remove Annotation", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
    {
        const std::string removeText =
                std::string( "Are you sure that you want to remove annotation '" ) +
                activeAnnot->getDisplayName() + "'?";

        ImGui::Text( removeText.c_str() );
        ImGui::Separator();

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2(0, 0) );
        ImGui::Checkbox( "Don't ask again", &doNotAskAagain );
        ImGui::PopStyleVar();


        if ( ImGui::Button( "Yes", ImVec2( 80, 0 ) ) )
        {
            removeAnnot = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();

        ImGui::SameLine();
        if ( ImGui::Button( "No", ImVec2( 80, 0 ) ) )
        {
            removeAnnot = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if ( removeAnnot )
    {
        const bool removed = appData.removeAnnotation( *activeAnnotUid );

        if ( removed  )
        {
            spdlog::info( "Removed annotation {}", *activeAnnotUid );
            return;
        }
        else
        {
            spdlog::error( "Unable to remove annotation {}", *activeAnnotUid );
        }
    }


    // Save annotation SVG and save settings to project file:
    ImGui::SameLine();
    const auto selectedFile = ImGui::renderFileButtonDialogAndWindow(
                sk_saveAnnotButtonText, sk_saveAnnotDialogTitle, sk_saveAnnotDialogFilters );

    if ( ImGui::IsItemHovered() )
    {
        ImGui::SetTooltip( "Save the annotation to an SVG file." );
    }


//    if ( selectedFile )
//    {
//        if ( serialize::saveLandmarksFile( activeLmGroup->getPoints(), *selectedFile ) )
//        {
//            spdlog::info( "Saved annotation to SVG file {}", *selectedFile );

//            /// @todo How to handle changing the file name?
//            activeAnnot->setFileName( *selectedFile );
//        }
//        else
//        {
//            spdlog::error( "Error saving annotation to SVG file {}", *selectedFile );
//        }
//    }

    ImGui::Separator();

    // Rules for showing the buttons that change annotation layer order:
    const bool showDecreaseLayer = true | ( 1 <= activeAnnotIndex );
    const bool showIncreaseLayer = true | ( activeAnnotIndex <= annotUids.size() - 2 );

    if ( showDecreaseLayer || showIncreaseLayer )
    {
        ImGui::Text( "Layer order: " );
    }

    ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0.0f, 0.0f ) );

    if ( showDecreaseLayer )
    {
        ImGui::SameLine();
        if ( ImGui::Button( ICON_FK_FAST_BACKWARD ) )
        {
            appData.moveAnnotationToBack( imageUid, *activeAnnotUid );
        }
        if ( ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "Move annotation to backmost layer" );
        }

        ImGui::SameLine();
        if ( ImGui::Button( ICON_FK_BACKWARD ) )
        {
            appData.moveAnnotationBackwards( imageUid, *activeAnnotUid );
        }
        if ( ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "Move annotation backward in layers (decrease the annotation order)" );
        }
    }

    if ( showIncreaseLayer )
    {
        ImGui::SameLine();
        if ( ImGui::Button( ICON_FK_FORWARD ) )
        {
            appData.moveAnnotationForwards( imageUid, *activeAnnotUid );
        }
        if ( ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "Move annotation forward in layers (increase the annotation order)" );
        }

        ImGui::SameLine();
        if ( ImGui::Button( ICON_FK_FAST_FORWARD ) )
        {
            appData.moveAnnotationToFront( imageUid, *activeAnnotUid );
        }
        if ( ImGui::IsItemHovered() )
        {
            ImGui::SetTooltip( "Move annotation to frontmost layer" );
        }
    }

    /*** ImGuiStyleVar_ItemSpacing ***/
    ImGui::PopStyleVar( 1 );


    ImGui::Separator();
    ImGui::Spacing();


    ImGui::Text( "Boundary:" );

    bool isClosed = activeAnnot->isClosed();
    if ( ImGui::RadioButton( "Open", ! isClosed ) )
    {
        activeAnnot->setClosed( false );
    }

    ImGui::SameLine();
    if ( ImGui::RadioButton( "Closed", isClosed ) )
    {
        activeAnnot->setClosed( true );
    }
    ImGui::SameLine(); helpMarker( "Set whether the annotation polygon boundary is open or closed" );

    ImGui::Spacing();


    // Visibility checkbox:
    bool annotVisible = activeAnnot->isVisible();
    if ( ImGui::Checkbox( "Visible", &annotVisible ) )
    {
        activeAnnot->setVisible( annotVisible );
    }
    ImGui::SameLine(); helpMarker( "Show/hide the annotation" );


    // Show vertices checkbox:
    if ( ! appData.renderData().m_globalAnnotationParams.hidePolygonVertices )
    {
        bool showVertices = activeAnnot->getVertexVisibility();
        if ( ImGui::Checkbox( "Show vertices", &showVertices ) )
        {
            activeAnnot->setVertexVisibility( showVertices );
        }
        ImGui::SameLine(); helpMarker( "Show/hide the annotation vertices" );
    }


    // Filled checkbox:
    if ( activeAnnot->isClosed() )
    {
        bool filled = activeAnnot->isFilled();
        if ( ImGui::Checkbox( "Filled", &filled ) )
        {
            activeAnnot->setFilled( filled );
        }
        ImGui::SameLine(); helpMarker( "Fill the annotation interior" );
    }


    // Opacity slider:
    float annotOpacity = activeAnnot->getOpacity();
    if ( mySliderF32( "Opacity", &annotOpacity, 0.0f, 1.0f ) )
    {
        activeAnnot->setOpacity( annotOpacity );
    }
    ImGui::SameLine(); helpMarker( "Overall annotation opacity" );


    // Line stroke thickness:
    float annotThickness = activeAnnot->getLineThickness();
    if ( ImGui::InputFloat( "Line thickness", &annotThickness, 0.1f, 1.0f, "%0.2f" ) )
    {
        if ( annotThickness >= 0.0f )
        {
            activeAnnot->setLineThickness( annotThickness );
        }
    }
    ImGui::SameLine(); helpMarker( "Annotation line thickness" );


    // Line color:
    glm::vec4 annotLineColor = activeAnnot->getLineColor();
    if ( ImGui::ColorEdit4( "Line color", glm::value_ptr( annotLineColor ), sk_annotColorEditFlags ) )
    {
        activeAnnot->setLineColor( annotLineColor );
        activeAnnot->setVertexColor( annotLineColor );
    }
    ImGui::SameLine(); helpMarker( "Annotation line color" );


    if ( activeAnnot->isClosed() && activeAnnot->isFilled() )
    {
        // Fill color:
        glm::vec4 annotFillColor = activeAnnot->getFillColor();
        if ( ImGui::ColorEdit4( "Fill color", glm::value_ptr( annotFillColor ), sk_annotColorEditFlags ) )
        {
            activeAnnot->setFillColor( annotFillColor );
        }
        ImGui::SameLine(); helpMarker( "Annotation fill color" );
    }
    ImGui::Spacing();


    ImGui::Separator();
    ImGui::Spacing();

    // Plane normal vector and offset:
    ImGui::Text( "Annotation plane (Subject space):" );

    const char* coordFormat = appData.guiData().m_coordsPrecisionFormat.c_str();

    glm::vec4 annotPlaneEq = activeAnnot->getSubjectPlaneEquation();
    ImGui::InputFloat3( "Normal", glm::value_ptr( annotPlaneEq ), coordFormat );
    ImGui::SameLine(); helpMarker( "Annotation plane normal vector (x, y, z) in image Subject space" );

    ImGui::InputFloat( "Offset (mm)", &annotPlaneEq[3], 0.0f, 0.0f, coordFormat );
    ImGui::SameLine(); helpMarker( "Offset distance (mm) of annotation plane from the image Subject space origin" );
    ImGui::Spacing();

    ImGui::PopID(); /** PopID imageUid **/
}
