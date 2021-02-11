#include "logic/camera/CameraTypes.h"

#include <unordered_map>

namespace camera
{

std::string typeString( const CameraType& cameraType )
{
    static const std::unordered_map< CameraType, std::string > s_typeToStringMap
    {
        { CameraType::Axial, "Axial" },
        { CameraType::Coronal, "Coronal" },
        { CameraType::Sagittal, "Sagittal" },
        { CameraType::ThreeD, "3D" }
    };

    return s_typeToStringMap.at( cameraType );
}

std::string typeString( const ShaderType& shaderType )
{
    static const std::unordered_map< ShaderType, std::string > s_typeToStringMap
    {
        { ShaderType::Image, "Layers" },
        { ShaderType::Edge, "Edges" },
        { ShaderType::Overlay, "Overlap" },
        { ShaderType::Checkerboard, "Checkerboard" },
        { ShaderType::Quadrants, "Quadrants" },
        { ShaderType::Flashlight, "Flashlight" },
        { ShaderType::Difference, "Difference" },
        { ShaderType::CrossCorrelation, "Correlation" },
        { ShaderType::JointHistogram, "Joint Histogram" },
        { ShaderType::Disabled, "Disabled" }
    };

    return s_typeToStringMap.at( shaderType );
}

std::string descriptionString( const ShaderType& shaderType )
{
    static const std::unordered_map< ShaderType, std::string > s_typeToStringMap
    {
        { ShaderType::Image, "Overlay of image layers" },
        { ShaderType::Edge, "Overlay of image edges" },
        { ShaderType::Overlay, "Overlap comparison" },
        { ShaderType::Checkerboard, "Checkerboard comparison" },
        { ShaderType::Quadrants, "Quadrants comparison" },
        { ShaderType::Flashlight, "Flashlight comparison" },
        { ShaderType::Difference, "Difference metric" },
        { ShaderType::CrossCorrelation, "Correlation metric" },
        { ShaderType::JointHistogram, "Joint histogram metric" },
        { ShaderType::Disabled, "Disabled" }
    };

    return s_typeToStringMap.at( shaderType );
}

} // namespace camera
