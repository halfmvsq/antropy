#ifndef CAMERA_TYPES_H
#define CAMERA_TYPES_H

#include <array>
#include <cstdint>
#include <string>


namespace camera
{

/**
 * @brief Types of view cameras. Each view camera type is assigned a starting orientation transformation,
 * a projection transformation, and orientation alignment rules.
 */
enum class CameraType
{
    Axial, //!< Orthogonal slice view aligned with reference image subject axial plane
    Coronal, //!< Orthogonal slice view with reference image subject coronal plane
    Sagittal, //!< Orthogonal slice view with reference image subject sagittal plane
    ThreeD, //!< 3D view that is nominally aligned with reference image subject coronal plane
    Oblique, //!< Oblique slice view
    NumElements
};

/// Array of all camera types accessible to the application
inline std::array<CameraType, 5> const AllCameraTypes = {
    CameraType::Axial,
    CameraType::Coronal,
    CameraType::Sagittal,
    CameraType::Oblique,
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


/**
 * @brief Get the string of a camera type
 * @param cameraType
 * @return
 */
std::string typeString( const CameraType& cameraType );

/**
 * @brief Get the string of a view rendering mode
 * @param shaderType
 * @return
 */
std::string typeString( const ViewRenderMode& renderMode );

std::string descriptionString( const ViewRenderMode& renderMode );

} // namespace camera

#endif // CAMERA_TYPES_H
