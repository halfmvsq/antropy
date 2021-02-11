#ifndef TYPES_H
#define TYPES_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>


/**
 * @brief Image pixel component types
 */
enum class ComponentType
{
    // These types are supported in Antropy
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Float32,

    // These types are NOT supported in Antropy, because they are not supported
    // as OpenGL texture formats
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


/// @brief Image interpolation (resampling) mode for rendering
enum class InterpolationMode
{
    NearestNeighbor,
    Linear
};


/// @brief The current mouse mode
enum class MouseMode : uint32_t
{
    Pointer,
    WindowLevel,
    Segment,
    Annotate,
    CameraTranslate2D,
    CameraZoom,
    ImageTranslate,
    ImageRotate,
    ImageScale
};

/// Array of all available mouse modes
inline std::array<MouseMode, 8> const AllMouseModes = {
    MouseMode::Pointer,
    MouseMode::WindowLevel,
    MouseMode::CameraTranslate2D,
    MouseMode::CameraZoom,
    MouseMode::Segment,
    MouseMode::Annotate,
    MouseMode::ImageTranslate,
    MouseMode::ImageRotate
};

// Mouse mode as a string
std::string typeString( const MouseMode& mouseMode );

// Toolbar button corresponding to each mouse mode
const char* toolbarButtonIcon( const MouseMode& mouseMode );


/// @brief How should view zooming behave?
enum class ZoomBehavior
{
    ToCrosshairs, //!< Zoom to/from the crosshairs position
    ToStartPosition, //!< Zoom to/from the mouse start position
    ToViewCenter //!< Zoom to/from the view center position
};


/// Describes a type of image selection
enum class ImageSelection
{
    /// The unique reference image that defines the World coordinate system.
    /// Tthere is one reference image in the app at a given time.
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

#endif // TYPES_H
