#ifndef TYPES_H
#define TYPES_H

#include <uuid.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>


/**
 * @brief Image pixel component types
 */
enum class ComponentType
{
    // These types are supported in Antropy. If an input image does not have
    // one of these types, then a cast is made.
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Float32,

    // These types are NOT supported in Antropy, because they are not supported
    // as OpenGL texture formats:
    Float64,
    ULong,
    Long,
    ULongLong,
    LongLong,
    LongDouble,
    Undefined
};


bool isComponentFloatingPoint( const ComponentType& );
bool isComponentUnsignedInt( const ComponentType& );
bool isValidSegmentationComponentType( const ComponentType& );

std::string componentTypeString( const ComponentType& );


/**
 * @brief Image pixel types
 */
enum class PixelType
{
    Scalar,
    RGB,
    RGBA,
    Offset,
    Vector,
    Point,
    CovariantVector,
    SymmetricSecondRankTensor,
    DiffusionTensor3D,
    Complex,
    FixedArray,
    Array,
    Matrix,
    VariableLengthVector,
    VariableSizeMatrix,
    Undefined
};


/**
 * @brief Statistics of a single image component
 */
template< typename T >
struct ComponentStats
{
    T m_minimum;
    T m_maximum;

    T m_mean;
    T m_stdDeviation;
    T m_variance;
    T m_sum;

    std::vector<double> m_histogram;
    std::array<T, 101> m_quantiles;
};


/**
 * @brief Image interpolation (resampling) mode for rendering
 */
enum class InterpolationMode
{
    NearestNeighbor,
    Linear
};


/**
 * @brief The current mouse mode
 */
enum class MouseMode
{
    Pointer, //!< Move the crosshairs
    WindowLevel, //!< Adjust window and level of the active image
    Segment, //!< Segment the active image
    Annotate, //!< Annotate the active image
    CameraTranslate, //!< Translate the view camera in plane
    CameraRotate, //!< Rotate the view camera in plane and out of plane
    CameraZoom, //!< Zoom the view camera
    ImageTranslate, //!< Translate the active image in 2D and 3D
    ImageRotate, //!< Rotate the active image in 2D and 3D
    ImageScale //!< Scale the active image in 2D
};


/**
 * @brief Array of all available mouse modes in the Toolbar
 */
inline std::array<MouseMode, 9> const AllMouseModes = {
    MouseMode::Pointer,
    MouseMode::WindowLevel,
    MouseMode::CameraTranslate,
    MouseMode::CameraRotate,
    MouseMode::CameraZoom,
    MouseMode::Segment,
    MouseMode::Annotate,
    MouseMode::ImageTranslate,
    MouseMode::ImageRotate
};


/// Get the mouse mode as a string
std::string typeString( const MouseMode& mouseMode );

/// Get the toolbar button corresponding to a mouse mode
const char* toolbarButtonIcon( const MouseMode& mouseMode );


/**
 * @brief How should view zooming behave?
 */
enum class ZoomBehavior
{
    ToCrosshairs, //!< Zoom to/from the crosshairs position
    ToStartPosition, //!< Zoom to/from the mouse start position
    ToViewCenter //!< Zoom to/from the view center position
};


/**
 * @brief Defines axis constraints for mouse/pointer rotation interactions
 */
enum class AxisConstraint
{
    X,
    Y,
    Z,
    None
};


/**
 * @brief Describes a type of image selection
 */
enum class ImageSelection
{
    /// The unique reference image that defines the World coordinate system.
    /// There is one reference image in the app at a given time.
    ReferenceImage,

    /// The unique image that is being actively transformed or modified.
    /// There is one active image in the app at a given time.
    ActiveImage,

    /// The unique reference and active images.
    ReferenceAndActiveImages,

    /// All visible images in a given view.
    /// Each view has its own set of visible images.
    VisibleImagesInView,

    /// The fixed image in a view that is currently rendering a metric.
    FixedImageInView,

    /// The moving image in a view that is currently rendering a metric.
    MovingImageInView,

    /// The fixed and moving images in a view thatis currently rendering a metric.
    FixedAndMovingImagesInView,

    /// All images loaded in the application.
    AllLoadedImages
};


/**
 * @brief Describes modes for offsetting the position of the view's image plane
 * (along the view camera's front axis) relative to the World-space crosshairs position.
 * Typically, this is used to offset the views in tiled layouts by a certain number of steps
 * (along the camera's front axis)
 */
enum class ViewOffsetMode
{
    /// Offset by a given number of view scrolls relative to the reference image
    RelativeToRefImageScrolls,

    /// Offset by a given number of view scrolls relative to an image
    RelativeToImageScrolls,

    /// Offset by an absolute distance (in physical units)
    Absolute,

    /// No offset
    None
};


/**
 * @brief Describes an offset setting for a view
 */
struct ViewOffsetSetting
{
    /// Offset mode
    ViewOffsetMode m_offsetMode = ViewOffsetMode::None;

    /// Absolute offset distance, which is used if m_offsetMode is OffsetMode::Absolute
    float m_absoluteOffset = 0.0f;

    /// Relative number of offset scrolls, which is used if m_offsetMode is
    /// OffsetMode::RelativeToRefImageScrolls or OffsetMode::RelativeToImageScrolls
    int m_relativeOffsetSteps = 0;

    /// If m_offsetMode is OffsetMode::RelativeToRefImageScrolls,
    /// then this holds the unique ID of the image relative which offsets are computed.
    /// If the image ID is not specified in OffsetMode::RelativeToRefImageScrolls,
    /// then the offset is ignored (i.e. assumed to be zero).
    std::optional<uuids::uuid> m_offsetImage = std::nullopt;
};

#endif // TYPES_H
