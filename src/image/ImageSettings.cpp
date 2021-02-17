#include "image/ImageSettings.h"
#include "common/Exception.hpp"

#include <spdlog/spdlog.h>

#undef min
#undef max

ImageSettings::ImageSettings(
        std::string displayName,
        uint32_t numComponents,
        ComponentType componentType,
        std::vector< ComponentStats<double> > componentStats )
    :
      m_displayName( std::move( displayName ) ),
      m_globalVisibility( true ),
      m_borderColor{ 1.0f, 0.0f, 1.0f },

      m_numComponents( numComponents ),
      m_componentType( std::move( componentType ) ),
      m_componentStats( std::move( componentStats ) ),
      m_activeComponent( 0 ),
      m_dirty( false )
{
    // Default window covers 1st to 99th quantile intensity range of the first pixel component
    static constexpr int qLow = 1;
    static constexpr int qHigh = 99;

    if ( m_componentStats.empty() )
    {
        spdlog::error( "No components in image settings" );
        throw_debug( "No components in image settings" )
    }

    for ( const auto& stat : m_componentStats )
    {
        ComponentSettings setting;

        // Min/max window, level, and threshold ranges are based on min/max component values
        const double minValue = stat.m_minimum;
        const double maxValue = stat.m_maximum;

        setting.m_minMaxWindowRange = std::make_pair( 0.0, maxValue - minValue );
        setting.m_minMaxLevelRange = std::make_pair( minValue, maxValue );
        setting.m_minMaxThresholdRange = std::make_pair( minValue, maxValue );
        setting.m_thresholdLow = minValue;
        setting.m_thresholdHigh = maxValue;

        // Default window and level are based on low and high quantiles
        const double quantileLow = stat.m_quantiles[qLow];
        const double quantileHigh = stat.m_quantiles[qHigh];
        setting.m_window = quantileHigh - quantileLow;
        setting.m_level = 0.5 * ( quantileLow + quantileHigh );

        // Default to maximum opacity and nearest neighbor interpolation
        setting.m_opacity = 1.0;
        setting.m_visible = true;

        setting.m_showEdges = false;
        setting.m_thresholdEdges = false;
        setting.m_useFreiChen = false;
        setting.m_edgeMagnitude = 0.5f;
        setting.m_windowedEdges = false;
        setting.m_overlayEdges = false;
        setting.m_colormapEdges = false;
        setting.m_edgeColor = glm::vec3{ 1.0f, 0.0f, 1.0f };
        setting.m_edgeOpacity = 1.0f;

        setting.m_interpolationMode = InterpolationMode::Linear;

        // Use the first color map and label table
        setting.m_colorMapIndex = 0;
        setting.m_colorMapInverted = false;
        setting.m_labelTableIndex = 0;

        m_settings.emplace_back( std::move( setting ) );
    }

    updateInternals();
}

void ImageSettings::setDisplayName( std::string name ) { m_displayName = std::move( name ); }
const std::string& ImageSettings::displayName() const { return m_displayName; }

void ImageSettings::setBorderColor( glm::vec3 borderColor ) { m_borderColor = std::move( borderColor ); }
const glm::vec3& ImageSettings::borderColor() const { return m_borderColor; }

bool ImageSettings::isDirty() const { return m_dirty; }
void ImageSettings::setDirty( bool set ) { m_dirty = set; }

void ImageSettings::setLevel( uint32_t i, double level )
{
    if ( m_settings[i].m_minMaxLevelRange.first <= level &&
         level <= m_settings[i].m_minMaxLevelRange.second )
    {
        m_settings[i].m_level = level;
        updateInternals();
    }
}

void ImageSettings::setLevel( double level ) { setLevel( m_activeComponent, level ); }

double ImageSettings::level( uint32_t i ) const { return m_settings[i].m_level; }
double ImageSettings::level() const { return level( m_activeComponent ); }

void ImageSettings::setWindow( uint32_t i, double window )
{
    // Don't allow window to equal the bottom of the range:
    if ( m_settings[i].m_minMaxWindowRange.first < window &&
         window <= m_settings[i].m_minMaxWindowRange.second )
    {
        m_settings[i].m_window = window;
        updateInternals();
    }
}

void ImageSettings::setWindow( double window ) { setWindow( m_activeComponent, window ); }

double ImageSettings::window( uint32_t i ) const { return m_settings[i].m_window; }
double ImageSettings::window() const { return window( m_activeComponent ); }

std::pair<double, double> ImageSettings::windowRange( uint32_t i ) const { return m_settings[i].m_minMaxWindowRange; }
std::pair<double, double> ImageSettings::windowRange() const { return windowRange( m_activeComponent ); }

std::pair<double, double> ImageSettings::levelRange( uint32_t i ) const { return m_settings[i].m_minMaxLevelRange; }
std::pair<double, double> ImageSettings::levelRange() const { return levelRange( m_activeComponent ); }

void ImageSettings::setThresholdLow( uint32_t i, double t )
{
    if ( m_settings[i].m_minMaxThresholdRange.first <= t &&
         t <= m_settings[i].m_minMaxThresholdRange.second )
    {
        m_settings[i].m_thresholdLow = t;
        updateInternals();
    }
}

void ImageSettings::setThresholdLow( double t ) { setThresholdLow( m_activeComponent, t ); }

double ImageSettings::thresholdLow( uint32_t i ) const { return m_settings[i].m_thresholdLow; }
double ImageSettings::thresholdLow() const { return thresholdLow( m_activeComponent ); }

void ImageSettings::setThresholdHigh( uint32_t i, double t )
{
    if ( m_settings[i].m_minMaxThresholdRange.first <= t &&
         t <= m_settings[i].m_minMaxThresholdRange.second )
    {
        m_settings[i].m_thresholdHigh = t;
        updateInternals();
    }
}

void ImageSettings::setThresholdHigh( double t ) { setThresholdHigh( m_activeComponent, t ); }

double ImageSettings::thresholdHigh( uint32_t i ) const { return m_settings[i].m_thresholdHigh; }
double ImageSettings::thresholdHigh() const { return thresholdHigh( m_activeComponent ); }

glm::dvec2 ImageSettings::thresholds( uint32_t i ) const
{
    return { m_settings[i].m_thresholdLow, m_settings[i].m_thresholdHigh };
}

glm::dvec2 ImageSettings::thresholds() const { return thresholds( m_activeComponent ); }

bool ImageSettings::thresholdsActive( uint32_t i ) const
{
    const auto& S = m_settings[i];
    return ( S.m_minMaxThresholdRange.first < S.m_thresholdLow ||
             S.m_thresholdHigh < S.m_minMaxThresholdRange.second );
}

bool ImageSettings::thresholdsActive() const { return thresholdsActive( m_activeComponent ); }

void ImageSettings::setOpacity( uint32_t i, double o )
{
    if ( 0.0 <= o && o <= 1.0 )
    {
        m_settings[i].m_opacity = o;
    }
}

void ImageSettings::setOpacity( double o ) { setOpacity( m_activeComponent, o ); }

double ImageSettings::opacity( uint32_t i ) const { return m_settings[i].m_opacity; }
double ImageSettings::opacity() const { return opacity( m_activeComponent ); }

void ImageSettings::setVisibility( uint32_t i, bool visible ) { m_settings[i].m_visible = visible; }
void ImageSettings::setVisibility( bool visible ) { setVisibility( m_activeComponent, visible ); }

bool ImageSettings::visibility( uint32_t i ) const { return m_settings[i].m_visible; }
bool ImageSettings::visibility() const { return visibility( m_activeComponent ); }


void ImageSettings::setGlobalVisibility( bool visible ) { m_globalVisibility = visible; }
bool ImageSettings::globalVisibility() const { return m_globalVisibility; }


void ImageSettings::setShowEdges( uint32_t i, bool show ) { m_settings[i].m_showEdges = show; }
void ImageSettings::setShowEdges( bool show ) { setShowEdges( m_activeComponent, show ); }

bool ImageSettings::showEdges( uint32_t i ) const { return m_settings[i].m_showEdges; }
bool ImageSettings::showEdges() const { return showEdges( m_activeComponent ); }

void ImageSettings::setThresholdEdges( uint32_t i, bool threshold ) { m_settings[i].m_thresholdEdges = threshold; }
void ImageSettings::setThresholdEdges( bool threshold ) { setThresholdEdges( m_activeComponent, threshold ); }

bool ImageSettings::thresholdEdges( uint32_t i ) const { return m_settings[i].m_thresholdEdges; }
bool ImageSettings::thresholdEdges() const { return thresholdEdges( m_activeComponent ); }

void ImageSettings::setUseFreiChen( uint32_t i, bool use ) { m_settings[i].m_useFreiChen = use; }
void ImageSettings::setUseFreiChen( bool use ) { setUseFreiChen( m_activeComponent, use ); }

bool ImageSettings::useFreiChen( uint32_t i ) const { return m_settings[i].m_useFreiChen; }
bool ImageSettings::useFreiChen() const { return useFreiChen( m_activeComponent ); }

void ImageSettings::setEdgeMagnitude( uint32_t i, double mag ) { m_settings[i].m_edgeMagnitude = mag; }
void ImageSettings::setEdgeMagnitude( double mag ) { setEdgeMagnitude( m_activeComponent, mag ); }

double ImageSettings::edgeMagnitude( uint32_t i ) const { return m_settings[i].m_edgeMagnitude; }
double ImageSettings::edgeMagnitude() const { return edgeMagnitude( m_activeComponent ); }

void ImageSettings::setWindowedEdges( uint32_t i, bool windowed ) { m_settings[i].m_windowedEdges = windowed; }
void ImageSettings::setWindowedEdges( bool windowed ) { setWindowedEdges( m_activeComponent, windowed ); }

bool ImageSettings::windowedEdges( uint32_t i ) const { return m_settings[i].m_windowedEdges; }
bool ImageSettings::windowedEdges() const { return windowedEdges( m_activeComponent ); }

void ImageSettings::setOverlayEdges( uint32_t i, bool overlay ) { m_settings[i].m_overlayEdges = overlay; }
void ImageSettings::setOverlayEdges( bool overlay ) { setOverlayEdges( m_activeComponent, overlay ); }

bool ImageSettings::overlayEdges( uint32_t i ) const { return m_settings[i].m_overlayEdges; }
bool ImageSettings::overlayEdges() const { return overlayEdges( m_activeComponent ); }

void ImageSettings::setColormapEdges( uint32_t i, bool showEdges ) { m_settings[i].m_colormapEdges = showEdges; }
void ImageSettings::setColormapEdges( bool showEdges ) { setColormapEdges( m_activeComponent, showEdges ); }

bool ImageSettings::colormapEdges( uint32_t i ) const { return m_settings[i].m_colormapEdges; }
bool ImageSettings::colormapEdges() const { return colormapEdges( m_activeComponent ); }

void ImageSettings::setEdgeColor( uint32_t i, glm::vec3 color ) { m_settings[i].m_edgeColor = std::move( color ); }
void ImageSettings::setEdgeColor( glm::vec3 color ) { setEdgeColor( m_activeComponent, color ); }

glm::vec3 ImageSettings::edgeColor( uint32_t i ) const { return m_settings[i].m_edgeColor; }
glm::vec3 ImageSettings::edgeColor() const { return edgeColor( m_activeComponent ); }

void ImageSettings::setEdgeOpacity( uint32_t i, float opacity ) { m_settings[i].m_edgeOpacity = opacity; }
void ImageSettings::setEdgeOpacity( float opacity ) { setEdgeOpacity( m_activeComponent, opacity ); }

float ImageSettings::edgeOpacity( uint32_t i ) const { return m_settings[i].m_edgeOpacity; }
float ImageSettings::edgeOpacity() const { return edgeOpacity( m_activeComponent ); }



void ImageSettings::setColorMapIndex( uint32_t i, size_t index ) { m_settings[i].m_colorMapIndex = index; }
void ImageSettings::setColorMapIndex( size_t index ) { setColorMapIndex( m_activeComponent, index ); }

size_t ImageSettings::colorMapIndex( uint32_t i ) const { return m_settings[i].m_colorMapIndex; }
size_t ImageSettings::colorMapIndex() const { return colorMapIndex( m_activeComponent ); }

void ImageSettings::setColorMapInverted( uint32_t i, bool inverted ) { m_settings[i].m_colorMapInverted = inverted; }
void ImageSettings::setColorMapInverted( bool inverted ) { setColorMapInverted( m_activeComponent, inverted ); }

bool ImageSettings::isColorMapInverted( uint32_t i ) const { return m_settings[i].m_colorMapInverted; }
bool ImageSettings::isColorMapInverted() const { return isColorMapInverted( m_activeComponent ); }

void ImageSettings::setLabelTableIndex( uint32_t i, size_t index ) { m_settings[i].m_labelTableIndex = index; }
void ImageSettings::setLabelTableIndex( size_t index ) { setLabelTableIndex( m_activeComponent, index ); }

size_t ImageSettings::labelTableIndex( uint32_t i ) const { return m_settings[i].m_labelTableIndex; }
size_t ImageSettings::labelTableIndex() const { return labelTableIndex( m_activeComponent ); }

void ImageSettings::setInterpolationMode( uint32_t i, InterpolationMode mode ) { m_settings[i].m_interpolationMode = mode; }
void ImageSettings::setInterpolationMode( InterpolationMode mode ) { setInterpolationMode( m_activeComponent, mode ); }

InterpolationMode ImageSettings::interpolationMode( uint32_t i ) const { return m_settings[i].m_interpolationMode; }
InterpolationMode ImageSettings::interpolationMode() const { return interpolationMode( m_activeComponent ); }

std::pair<double, double> ImageSettings::thresholdRange( uint32_t i ) const { return m_settings[i].m_minMaxThresholdRange; }
std::pair<double, double> ImageSettings::thresholdRange() const { return thresholdRange( m_activeComponent ); }

std::pair<double, double> ImageSettings::slopeInterceptNative( uint32_t i ) const { return { m_settings[i].m_slope_native, m_settings[i].m_intercept_native }; }
std::pair<double, double> ImageSettings::slopeInterceptNative() const { return slopeInterceptNative( m_activeComponent ); }

std::pair<double, double> ImageSettings::slopeInterceptTexture( uint32_t i ) const { return { m_settings[i].m_slope_texture, m_settings[i].m_intercept_texture }; }
std::pair<double, double> ImageSettings::slopeInterceptTexture() const { return slopeInterceptTexture( m_activeComponent ); }

glm::dvec2 ImageSettings::slopeInterceptTextureVec2( uint32_t i ) const { return { m_settings[i].m_slope_texture, m_settings[i].m_intercept_texture }; }
glm::dvec2 ImageSettings::slopeInterceptTextureVec2() const { return slopeInterceptTextureVec2( m_activeComponent ); }

glm::dvec2 ImageSettings::largestSlopeInterceptTextureVec2( uint32_t i ) const { return { m_settings[i].m_largest_slope_texture, m_settings[i].m_largest_intercept_texture }; }
glm::dvec2 ImageSettings::largestSlopeInterceptTextureVec2() const { return largestSlopeInterceptTextureVec2( m_activeComponent ); }

uint32_t ImageSettings::numComponents() const { return m_numComponents; }

const ComponentStats<double>& ImageSettings::componentStatistics( uint32_t i ) const
{
    if ( m_componentStats.size() <= i )
    {
        throw_debug( "Invalid image component" )
    }

    return m_componentStats.at( i );
}

const ComponentStats<double>& ImageSettings::componentStatistics() const { return componentStatistics( m_activeComponent ); }


void ImageSettings::setActiveComponent( uint32_t component )
{
    if ( component < m_numComponents )
    {
        m_activeComponent = component;
    }
    else
    {
        spdlog::error( "Attempting to set invalid active component {} (only {} componnets total for image {})",
                       component, m_numComponents, m_displayName );
    }
}

uint32_t ImageSettings::activeComponent() const { return m_activeComponent; }

void ImageSettings::updateInternals()
{
    for ( auto& S : m_settings )
    {
        const double imageMin = S.m_minMaxLevelRange.first;
        const double imageMax = S.m_minMaxLevelRange.second;

        const double imageRange = imageMax - imageMin;

        if ( imageRange <= 0.0 || S.m_window <= 0.0 )
        {
            // Resort to default slope/intercept and normalized threshold values
            // if either the image range or the window width are not positive:

            S.m_slope_native = 1.0;
            S.m_intercept_native = 0.0;

            S.m_slope_texture = 1.0;
            S.m_intercept_texture = 0.0;

            S.m_largest_slope_texture = 1.0;
            S.m_largest_intercept_texture = 0.0;

            continue;
        }

        S.m_slope_native = 1.0 / S.m_window;
        S.m_intercept_native = 0.5 - S.m_level / S.m_window;

        /**
         * @note In OpenGL, UNSIGNED normalized floats are computed as
         * float = int / MAX, where MAX = 2^B - 1 = 255
         *
         * SIGNED normalized floats are computed as either
         * float = max( int / MAX, -1 ) where MAX = 2^(B-1) - 1 = 127
         * (this is the method used most commonly in OpenGL 4.2 and above)
         *
         * or alternatively as (depending on implementation)
         * float = (2*int + 1) / (2^B - 1) = (2*int + 1) / 255
         *
         * @see https://www.khronos.org/opengl/wiki/Normalized_Integer
         */

        double M = 0.0;

        switch ( m_componentType )
        {
        case ComponentType::Int8:
        case ComponentType::UInt8:
        {
            M = static_cast<double>( std::numeric_limits<uint8_t>::max() );
            break;
        }
        case ComponentType::Int16:
        case ComponentType::UInt16:
        {
            M = static_cast<double>( std::numeric_limits<uint16_t>::max() );
            break;
        }
        case ComponentType::Int32:
        case ComponentType::UInt32:
        {
            M = static_cast<double>( std::numeric_limits<uint32_t>::max() );
            break;
        }
        case ComponentType::Float32:
        {
            M = 0.0;
            break;
        }
        default: break;
        }


        switch ( m_componentType )
        {
        case ComponentType::Int8:
        case ComponentType::Int16:
        case ComponentType::Int32:
        {
            /// @todo This mapping may be slightly wrong for the signed integer case
            S.m_slope_texture = 0.5 * M / imageRange;
            S.m_intercept_texture = -( imageMin + 0.5 ) / imageRange;
            break;
        }
        case ComponentType::UInt8:
        case ComponentType::UInt16:
        case ComponentType::UInt32:
        {
            S.m_slope_texture = M / imageRange;
            S.m_intercept_texture = -imageMin / imageRange;
            break;
        }
        case ComponentType::Float32:
        {
            S.m_slope_texture = 1.0 / imageRange;
            S.m_intercept_texture = -imageMin / imageRange;
            break;
        }
        default: break;
        }

        const double a = 1.0 / imageRange;
        const double b = -imageMin / imageRange;

        // Normalized window and level:
        const double windowNorm = a * S.m_window;
        const double levelNorm = a * S.m_level + b;

        // The slope and intercept that give the largest window:
        S.m_largest_slope_texture = S.m_slope_texture;
        S.m_largest_intercept_texture = S.m_intercept_texture;

        // Apply windowing and leveling to the slope and intercept:
        S.m_slope_texture = S.m_slope_texture / windowNorm;
        S.m_intercept_texture = S.m_intercept_texture / windowNorm + ( 0.5 - levelNorm / windowNorm );
    }
}


double ImageSettings::mapNativeIntensityToTexture( double nativeImageValue ) const
{
    switch ( m_componentType )
    {
    case ComponentType::Int8:
    {
        const double M = static_cast<double>( std::numeric_limits<int8_t>::max() );
        return std::max( nativeImageValue / M, -1.0 );

        /// @note An alternate mapping for signed integers is sometimes used in OpenGL < 4.2:
        /// return ( 2.0 * nativeImageValue + 1.0 ) / ( 2^B - 1 )
        /// This mapping does not allow for a signed integer to exactly express the value zero.
    }
    case ComponentType::Int16:
    {
        const double M = static_cast<double>( std::numeric_limits<int16_t>::max() );
        return std::max( nativeImageValue / M, -1.0 );
    }
    case ComponentType::Int32:
    {
        const double M = static_cast<double>( std::numeric_limits<int32_t>::max() );
        return std::max( nativeImageValue / M, -1.0 );
    }
    case ComponentType::UInt8:
    {
        const double M = static_cast<double>( std::numeric_limits<uint8_t>::max() );
        return nativeImageValue / M;
    }

    case ComponentType::UInt16:
    {
        const double M = static_cast<double>( std::numeric_limits<uint16_t>::max() );
        return nativeImageValue / M;
    }

    case ComponentType::UInt32:
    {
        const double M = static_cast<double>( std::numeric_limits<uint32_t>::max() );
        return nativeImageValue / M;
    }
    case ComponentType::Float32:
    {
        return nativeImageValue;
    }
    default:
    {
        spdlog::error( "Invalid component type {}", m_componentType );
        return nativeImageValue;
    }
    }
}


std::ostream& operator<< ( std::ostream& os, const ImageSettings& s )
{
    os << "Display name: " << s.m_displayName;

    for ( size_t i = 0; i < s.m_componentStats.size(); ++i )
    {
        const auto& t = s.m_componentStats[i];

        os << "\nStatistics (component " << i << "):"
           << "\n\tMin: " << t.m_minimum
           << "\n\tQ01: " << t.m_quantiles[1]
           << "\n\tQ25: " << t.m_quantiles[25]
           << "\n\tMed: " << t.m_quantiles[50]
           << "\n\tQ75: " << t.m_quantiles[75]
           << "\n\tQ99: " << t.m_quantiles[99]
           << "\n\tMax: " << t.m_maximum
           << "\n\tAvg: " << t.m_mean
           << "\n\tStd: " << t.m_stdDeviation;
    }

    return os;
}
