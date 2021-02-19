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
enum class CameraType
{
    /// Aligned with reference image subject axial plane
    Axial,

    /// Aligned with reference image subject coronal plane
    Coronal,

    /// Aligned with reference image subject sagittal plane
    Sagittal,

    /// 3D camera
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
 * @brief View rendering mode
 */
enum class ViewRenderMode
{
    Image, //!< Image rendered using color maps
    Edge, //!< Image rendered using color maps and edge overlay
    Overlay, //!< Image pair rendered with overlap highlighted
    Checkerboard,
    Quadrants,
    Flashlight,
    Difference,
    CrossCorrelation,
    JointHistogram,
    Disabled, //!< Disabled view
    NumElements
};


/// Array of all rendering modes in use by the application
inline std::array<ViewRenderMode, 8> const AllViewRenderModes = {
    ViewRenderMode::Image,
    ViewRenderMode::Overlay,
    ViewRenderMode::Checkerboard,
    ViewRenderMode::Quadrants,
    ViewRenderMode::Flashlight,
    ViewRenderMode::Difference,
    ViewRenderMode::JointHistogram,
    ViewRenderMode::Disabled
};


/// Array of all view render modes that are valid for views with only one image
inline std::array<ViewRenderMode, 2> const AllNonMetricRenderModes = {
    ViewRenderMode::Image,
    ViewRenderMode::Disabled
};


std::string typeString( const CameraType& cameraType );

std::string typeString( const ViewRenderMode& shaderType );
std::string descriptionString( const ViewRenderMode& shaderType );

} // namespace camera

#endif // CAMERA_TYPES_H
