#ifndef CAMERA_TYPES_H
#define CAMERA_TYPES_H

#include <array>
#include <cstdint>
#include <string>


namespace camera
{

/**
 * @brief Types of view cameras. Each can have a different starting orientation, projection,
 * and alignment rules.
 */
enum class CameraType : uint32_t
{
    /// Aligned with reference image subject axial plane
    Axial = 0,

    /// Aligned with reference image subject coronal plane
    Coronal,

    /// Aligned with reference image subject sagittal plane
    Sagittal,

    ThreeD,

    NumElements
};

/// Array of all camera types
inline std::array<CameraType, 4> const AllCameraTypes = {
    CameraType::Axial,
    CameraType::Coronal,
    CameraType::Sagittal,
    CameraType::ThreeD
};


/**
 * @brief Types of camera projections
 */
enum class ProjectionType
{
    /// Orthographic projection is used for the "2D" views, since there's no compelling reason
    /// to do otherwise. Orthographic projections made zooming and rotation about arbitrary points
    /// behave nicer.
    Orthographic,

    /// Perspective projection is used for the main and big 3D views. It lets us fly inside the scene!
    Perspective
};


/**
 * @brief Type of fragment shader to use for view rendering
 */
enum class ShaderType : uint32_t
{
    Image = 0, //!< Images rendered using color maps
    Edge,
    Overlay, //!< Highlight overlap between images
    Checkerboard,
    Quadrants,
    Flashlight,
    Difference,
    CrossCorrelation,
    JointHistogram,
    Disabled, //!< Disabled view
    NumElements
};

/// Array of all shader types
inline std::array<ShaderType, 8> const AllShaderTypes = {
    ShaderType::Image,
    ShaderType::Overlay,
    ShaderType::Checkerboard,
    ShaderType::Quadrants,
    ShaderType::Flashlight,
    ShaderType::Difference,
//    ShaderType::CrossCorrelation,
    ShaderType::JointHistogram,
    ShaderType::Disabled
};


/// Array of all shader types that are valid for one image:
inline std::array<ShaderType, 2> const AllNonMetricShaderTypes = {
    ShaderType::Image,
    ShaderType::Disabled
};


std::string typeString( const CameraType& cameraType );

std::string typeString( const ShaderType& shaderType );
std::string descriptionString( const ShaderType& shaderType );

} // namespace camera

#endif // CAMERA_TYPES_H
