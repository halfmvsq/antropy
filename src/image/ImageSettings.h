#ifndef IMAGE_SETTINGS_H
#define IMAGE_SETTINGS_H

#include "common/Types.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <ostream>
#include <string>
#include <utility>
#include <vector>


class ImageSettings
{
public:

    explicit ImageSettings() = default;

    /**
     * @brief ImageSettings
     * @param displayName Image display name
     * @param numComponents Number of components per pixel
     * @param componentType Component type
     * @param componentStats Vector of pixel statistics, one per image component
     */
    ImageSettings( std::string displayName,
                   uint32_t numComponents,
                   ComponentType componentType,
                   std::vector< ComponentStats<double> > componentStats );

    ImageSettings( const ImageSettings& ) = default;
    ImageSettings& operator=( const ImageSettings& ) = default;

    ImageSettings( ImageSettings&& ) = default;
    ImageSettings& operator=( ImageSettings&& ) = default;

    ~ImageSettings() = default;


    /// Set the short display name of the image
    void setDisplayName( std::string name );

    /// Get the short display name of the image
    const std::string& displayName() const;


    /// Set window (in image intensity units) for a given component.
    void setWindow( uint32_t component, double window );
    void setWindow( double window );

    /// Get window (in image intensity units) for a given component
    double window( uint32_t component ) const;
    double window() const;


    /// Set level (in image intensity units) for a given component.
    void setLevel( uint32_t component, double level );
    void setLevel( double level );

    /// Get level (in image intensity units) for a given component
    double level( uint32_t component ) const;
    double level() const;


    /// Set low threshold (in image intensity units) for a given component
    void setThresholdLow( uint32_t component, double thresh );
    void setThresholdLow( double thresh );

    /// Get low threshold (in image intensity units) for a given component
    double thresholdLow( uint32_t component ) const;
    double thresholdLow() const;

    /// Set high threshold (in image intensity units) for a given component
    void setThresholdHigh( uint32_t component, double thresh );
    void setThresholdHigh( double thresh );

    /// Get high threshold (in image intensity units) for a given component
    double thresholdHigh( uint32_t component ) const;
    double thresholdHigh() const;

    /// Get thresholds as GLM vector
    glm::dvec2 thresholds( uint32_t component ) const;
    glm::dvec2 thresholds() const;

    /// Get whether the thresholds are active for a given component
    bool thresholdsActive( uint32_t component ) const;
    bool thresholdsActive() const;


    /// Set the image opacity (in [0, 1] range) for a given component
    void setOpacity( uint32_t component, double opacity );
    void setOpacity( double opacity );

    /// Get the image opacity (in [0, 1] range) of a given component
    double opacity( uint32_t component ) const;
    double opacity() const;


    /// Set the visibility for a given component
    void setVisibility( uint32_t component, bool visible );
    void setVisibility( bool visible );

    /// Get the visibility of a given component
    bool visibility( uint32_t component ) const;
    bool visibility() const;


    /// Set whether edges are shown for a given component
    void setShowEdges( uint32_t component, bool show );
    void setShowEdges( bool show );

    /// Get whether edges are shown for a given component
    bool showEdges( uint32_t component ) const;
    bool showEdges() const;

    /// Set whether edges are thresholded for a given component
    void setThresholdEdges( uint32_t component, bool threshold );
    void setThresholdEdges( bool threshold );

    /// Get whether edges are thresholded for a given component
    bool thresholdEdges( uint32_t component ) const;
    bool thresholdEdges() const;

    /// Set whether edges are computed using Frei-Chen for a given component
    void setUseFreiChen( uint32_t component, bool use );
    void setUseFreiChen( bool use );

    /// Get whether edges are thresholded for a given component
    bool useFreiChen( uint32_t component ) const;
    bool useFreiChen() const;

    /// Set edge magnitude for a given component
    void setEdgeMagnitude( uint32_t component, double mag );
    void setEdgeMagnitude( double mag );

    /// Get edge matnidue for a given component
    double edgeMagnitude( uint32_t component ) const;
    double edgeMagnitude() const;

    /// Set whether edges are computed after applying windowing (width/level) for a given component
    void setWindowedEdges( uint32_t component, bool windowed );
    void setWindowedEdges( bool overlay );

    /// Get whether edges are computed after applying windowing (width/level) for a given component
    bool windowedEdges( uint32_t component ) const;
    bool windowedEdges() const;

    /// Set whether edges are shown as an overlay on the image for a given component
    void setOverlayEdges( uint32_t component, bool overlay );
    void setOverlayEdges( bool overlay );

    /// Get whether edges are shown as an overlay on the image for a given component
    bool overlayEdges( uint32_t component ) const;
    bool overlayEdges() const;

    /// Set whether edges are colormapped for a given component
    void setColormapEdges( uint32_t component, bool showEdges );
    void setColormapEdges( bool showEdges );

    /// Get whether edges are colormapped for a given component
    bool colormapEdges( uint32_t component ) const;
    bool colormapEdges() const;

    /// Set edge color a given component
    void setEdgeColor( uint32_t component, glm::vec3 color );
    void setEdgeColor( glm::vec3 color );

    /// Get whether edges are shown for a given component
    glm::vec3 edgeColor( uint32_t component ) const;
    glm::vec3 edgeColor() const;

    /// Set edge opacity a given component
    void setEdgeOpacity( uint32_t component, float opacity );
    void setEdgeOpacity( float opacity );

    /// Get edge opacity for a given component
    float edgeOpacity( uint32_t component ) const;
    float edgeOpacity() const;


    /// Get the color map index
    void setColorMapIndex( uint32_t component, size_t index );
    void setColorMapIndex( size_t index );

    /// Get the color map index
    size_t colorMapIndex( uint32_t component ) const;
    size_t colorMapIndex() const;


    // Set whether the color map is inverted
    void setColorMapInverted( uint32_t component, bool inverted );
    void setColorMapInverted( bool inverted );

    // Get whether the color map is inverted
    bool isColorMapInverted( uint32_t component ) const;
    bool isColorMapInverted() const;


    /// Get the label table index
    void setLabelTableIndex( uint32_t component, size_t index );
    void setLabelTableIndex( size_t index );

    /// Get the label table index
    size_t labelTableIndex( uint32_t component ) const;
    size_t labelTableIndex() const;


    /// Set the interpolation mode for a given component
    void setInterpolationMode( uint32_t component, InterpolationMode mode );
    void setInterpolationMode( InterpolationMode mode );

    /// Get the interpolation mode of a given component
    InterpolationMode interpolationMode( uint32_t component ) const;
    InterpolationMode interpolationMode() const;


    /// Get window/level slope 'm' and intercept 'b' for a given component.
    /// These are used to map NATIVE (raw) image intensity units 'x' to normalized units 'y' in the
    /// normalized range [0, 1]: y = m*x + b
    std::pair<double, double> slopeInterceptNative( uint32_t component ) const;
    std::pair<double, double> slopeInterceptNative() const;

    /// Get the slope/intercept mapping from NATIVE intensity to OpenGL TEXTURE intensity
    std::pair<double, double> slopeIntercept_texture_T_native() const;

    /// Get normalized window/level slope 'm' and intercept 'b' for a given component.
    /// These are used to map image TEXTURE intensity units 'x' to normalized units 'y' in the
    /// normalized range [0, 1]: y = m*x + b
    std::pair<double, double> slopeInterceptTexture( uint32_t component ) const;
    std::pair<double, double> slopeInterceptTexture() const;

    glm::dvec2 slopeInterceptTextureVec2( uint32_t component ) const;
    glm::dvec2 slopeInterceptTextureVec2() const;

    glm::dvec2 largestSlopeInterceptTextureVec2( uint32_t component ) const;
    glm::dvec2 largestSlopeInterceptTextureVec2() const;

    /// Get window range (in image intensity units) for a given component
    std::pair<double, double> windowRange( uint32_t component ) const;
    std::pair<double, double> windowRange() const;

    /// Get level range (in image intensity units) for a given component
    std::pair<double, double> levelRange( uint32_t component ) const;
    std::pair<double, double> levelRange() const;

    /// Get threshold range (in image intensity units) for a given component
    std::pair<double, double> thresholdRange( uint32_t component ) const;
    std::pair<double, double> thresholdRange() const;

    uint32_t numComponents() const; //!< Number of components per pixel

    /// Get statistics for an image component
    /// The component must be in the range [0, numComponents() - 1].
    const ComponentStats<double>& componentStatistics( uint32_t component ) const;
    const ComponentStats<double>& componentStatistics() const;

    /// Set the active component
    void setActiveComponent( uint32_t component );

    /// Get the active component
    uint32_t activeComponent() const;

    /// Map a native image value to its representation as an OpenGL texture.
    /// This mappings accounts for component type.
    /// @see https://www.khronos.org/opengl/wiki/Normalized_Integer
    double mapNativeIntensityToTexture( double nativeImageValue ) const;

    friend std::ostream& operator<< ( std::ostream&, const ImageSettings& );


private:

    void updateInternals();

    /// @brief Settings for one image component
    struct ComponentSettings
    {
        double m_level; //! Window center value in NATIVE image value units
        double m_window; //! Window width in NATIVE image value units

        // The following slope (m) and intercept (b) are used to map NATIVE image intensity
        // values (x) into the range [0.0, 1.0], via m*x + b
        double m_slope_native; //!< Slope (m) computed from window
        double m_intercept_native; //!< Intercept (b) computed from window and level

        // The following slope (m) and intercept (b) are used to map image TEXTURE intensity
        // values (x) into the range [0.0, 1.0], via m*x + b
        double m_slope_texture; //!< Slope computed from window
        double m_intercept_texture; //!< Intercept computed from window and level

        // The following values of slope (m) and intercept (b) are used to map image TEXTURE intensity
        // values (x) into the range [0.0, 1.0], via m*x + b
        // These values represent the largest window possible
        double m_largest_slope_texture; //!< Slope computed from window
        double m_largest_intercept_texture; //!< Intercept computed from window and level

        double m_thresholdLow;  //!< Values below threshold not displayed
        double m_thresholdHigh; //!< Values above threshold not displayed

        double m_opacity; //!< Opacity [0.0, 1.0]
        bool m_visible; //!< Visibility

        bool m_showEdges; //!< Flag to show edges
        bool m_thresholdEdges; //!< Flag to threshold edges
        bool m_useFreiChen; //!< Flag to use Frei-Chen filters
        double m_edgeMagnitude; //!< Magnitude of edges to show [0.0, 4.0] if thresholding is turned one
        bool m_windowedEdges; //!< Flag to compute edges after applying windowing (width/level) to the image
        bool m_overlayEdges; //!< Flag to overlay edges atop image (true) or show edges on their own (false)
        bool m_colormapEdges; //!< Flag to apply colormap to edges (true) or to render edges with a solid color (false)
        glm::vec3 m_edgeColor; //!< Edge color (used if not rendering edges using colormap)
        float m_edgeOpacity; //!< Edge opacity: only applies when shown as an overlay atop the image

        size_t m_colorMapIndex; //!< Color map index
        bool m_colorMapInverted; //!< Whether the color map is inverted

        size_t m_labelTableIndex; //!< Label table index (for segmentations only)

        InterpolationMode m_interpolationMode;

        std::pair<double, double> m_minMaxWindowRange; //!< Valid window size range (not-inclusive of the min value)
        std::pair<double, double> m_minMaxLevelRange; //!< Valid level value range
        std::pair<double, double> m_minMaxThresholdRange; //!< Valid threshold range
    };

    std::string m_displayName;

    uint32_t m_numComponents; //!< Number of components per pixel
    ComponentType m_componentType; //!< Component type
    std::vector< ComponentStats<double> > m_componentStats; //!< Per-component statistics
    std::vector<ComponentSettings> m_settings; //!< Per-component settings

    uint32_t m_activeComponent; //!< Active component
};


std::ostream& operator<< ( std::ostream&, const ImageSettings& );

#endif // IMAGE_SETTINGS_H
