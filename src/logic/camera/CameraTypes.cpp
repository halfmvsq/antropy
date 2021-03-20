#include "logic/camera/CameraTypes.h"

#include <unordered_map>


namespace camera
{

std::string typeString( const CameraType& type, bool crosshairsRotated )
{
    // View camera type names when the crosshairs are NOT rotated:
    static const std::unordered_map< CameraType, std::string > s_typeToStringMap
    {
        { CameraType::Axial, "Axial" },
        { CameraType::Coronal, "Coronal" },
        { CameraType::Sagittal, "Sagittal" },
        { CameraType::ThreeD, "3D" },
        { CameraType::Oblique, "Oblique" }
    };

    // View camera type names when the crosshairs are ARE rotated:
    static const std::unordered_map< CameraType, std::string > s_rotatedTypeToStringMap
    {
        { CameraType::Axial, "Crosshairs Z" },
        { CameraType::Coronal, "Crosshairs Y" },
        { CameraType::Sagittal, "Crosshairs X" },
        { CameraType::ThreeD, "3D" },
        { CameraType::Oblique, "Oblique" }
    };

    return ( crosshairsRotated ? s_rotatedTypeToStringMap.at( type ) : s_typeToStringMap.at( type ) );
}

std::string typeString( const ViewRenderMode& mode )
{
    static const std::unordered_map< ViewRenderMode, std::string > s_modeToStringMap
    {
        { ViewRenderMode::Image, "Layers" },
        { ViewRenderMode::Overlay, "Overlap" },
        { ViewRenderMode::Checkerboard, "Checkerboard" },
        { ViewRenderMode::Quadrants, "Quadrants" },
        { ViewRenderMode::Flashlight, "Flashlight" },
        { ViewRenderMode::Difference, "Difference" },
        { ViewRenderMode::CrossCorrelation, "Correlation" },
        { ViewRenderMode::JointHistogram, "Joint Histogram" },
        { ViewRenderMode::Disabled, "Disabled" }
    };

    return s_modeToStringMap.at( mode );
}

std::string descriptionString( const ViewRenderMode& mode )
{
    static const std::unordered_map< ViewRenderMode, std::string > s_modeToStringMap
    {
        { ViewRenderMode::Image, "Overlay of image layers" },
        { ViewRenderMode::Overlay, "Overlap comparison" },
        { ViewRenderMode::Checkerboard, "Checkerboard comparison" },
        { ViewRenderMode::Quadrants, "Quadrants comparison" },
        { ViewRenderMode::Flashlight, "Flashlight comparison" },
        { ViewRenderMode::Difference, "Difference metric" },
        { ViewRenderMode::CrossCorrelation, "Correlation metric" },
        { ViewRenderMode::JointHistogram, "Joint histogram metric" },
        { ViewRenderMode::Disabled, "Disabled" }
    };

    return s_modeToStringMap.at( mode );
}

} // namespace camera
