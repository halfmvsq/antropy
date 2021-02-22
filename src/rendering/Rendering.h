#ifndef RENDERING_H
#define RENDERING_H

#include "common/Types.h"
#include "common/UuidRange.h"

#include "logic/camera/CameraTypes.h"
#include "rendering/utility/gl/GLShaderProgram.h"

#include <uuid.h>

#include <array>
#include <list>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

class AppData;
class GLTexture;
struct NVGcontext;


/**
 * @brief Encapsulates all rendering
 * @todo Split this giant class apart
 */
class Rendering
{
public:

    Rendering( AppData& );
    ~Rendering();

    /// Initialization
    void init();

    /// Create image and segmentation textures
    void initTextures();

    /// Render the scene
    void render();

    /// Set the viewport to match framebuffer dimensions
    void resize( int width, int height );

    /// Update all texture interpolation parameters for the active image component
    void updateImageInterpolation( const uuids::uuid& imageUid );

    /// Update image uniforms after any settings have changed
    void updateImageUniforms( uuid_range_t imageUids );
    void updateImageUniforms( const uuids::uuid& imageUid );

    /// Update the metric uniforms after any settings have changed
    void updateMetricUniforms();

    /// Update a label color table texture
    void updateLabelColorTableTexture( size_t tableIndex );

    /**
     * @brief Updates the texture representation of a segmentation
     * @param segUid
     * @param compType
     * @param startOffsetVoxel
     * @param sizeInVoxels
     * @param data
     */
    void updateSegTexture(
            const uuids::uuid& segUid,
            const ComponentType& compType,
            const glm::uvec3& startOffsetVoxel,
            const glm::uvec3& sizeInVoxels,
            const void* data );

    bool createLabelColorTableTexture( const uuids::uuid& labelTableUid );

    bool createSegTexture( const uuids::uuid& segUid );
    bool removeSegTexture( const uuids::uuid& segUid );

    /// Get/set the overlay visibility
    bool showVectorOverlays() const;
    void setShowVectorOverlays( bool show );


private:

    // Number of images rendered per metric view
    static constexpr size_t NUM_METRIC_IMAGES = 2;

    using ImgSegPair = std::pair< std::optional<uuids::uuid>, std::optional<uuids::uuid> >;

    // Vector of current image/segmentation pairs rendered by image shaders
    using CurrentImages = std::vector< ImgSegPair >;

    void setupOpenGlState();

    void createShaderPrograms();

    bool createCrossCorrelationProgram( GLShaderProgram& program );
    bool createImageProgram( GLShaderProgram& program );
    bool createEdgeProgram( GLShaderProgram& program );
    bool createOverlayProgram( GLShaderProgram& program );
    bool createSimpleProgram( GLShaderProgram& program );
    bool createDifferenceProgram( GLShaderProgram& program );

    void renderImages();
    void renderOverlays();
    void renderVectorOverlays();


    void doRenderingAllImagePlanes(
            const camera::ViewRenderMode& shaderType,
            const std::list<uuids::uuid>& metricImages,
            const std::list<uuids::uuid>& renderedImages,
            const std::function< void ( GLShaderProgram&, const CurrentImages& I, bool showEdges ) > renderFunc );

    void doRenderingImageLandmarks(
            const camera::ViewRenderMode& shaderType,
            const std::list<uuids::uuid>& metricImages,
            const std::list<uuids::uuid>& renderedImages,
            const std::function< void ( const CurrentImages& I ) > renderFunc );

    void doRenderingImageAnnotations(
            const camera::ViewRenderMode& shaderType,
            const std::list<uuids::uuid>& metricImages,
            const std::list<uuids::uuid>& renderedImages,
            const std::function< void ( const CurrentImages& I ) > renderFunc );

    // Bind/unbind images, segmentations, color maps, and label tables
    std::list< std::reference_wrapper<GLTexture> > bindImageTextures( const ImgSegPair& P );
    void unbindTextures( const std::list< std::reference_wrapper<GLTexture> >& textures );

    // Bind/unbind images, segmentations, color maps, and label tables
    std::list< std::reference_wrapper<GLTexture> >
    bindMetricImageTextures( const CurrentImages& P, const camera::ViewRenderMode& metricType );

    // Get current image and segmentation UIDs to render in the metric shaders
    CurrentImages getImageAndSegUidsForMetricShaders( const std::list<uuids::uuid>& metricImageUids ) const;

    // Get current image and segmentation UIDs to render in the image shaders
    CurrentImages getImageAndSegUidsForImageShaders( const std::list<uuids::uuid>& imageUids ) const;

    AppData& m_appData;

    // NanoVG context for vector graphics (owned by this class)
    NVGcontext* m_nvg;

    GLShaderProgram m_crossCorrelationProgram;
    GLShaderProgram m_differenceProgram;
    GLShaderProgram m_imageProgram;
    GLShaderProgram m_edgeProgram;
    GLShaderProgram m_overlayProgram;
    GLShaderProgram m_simpleProgram;

    // Samplers for metric shaders:
    static const Uniforms::SamplerIndexVectorType msk_imgTexSamplers; // pair of images
    static const Uniforms::SamplerIndexVectorType msk_segTexSamplers; // pair of segmentations
    static const Uniforms::SamplerIndexVectorType msk_labelTableTexSamplers; // pair of label tables
    static const Uniforms::SamplerIndexVectorType msk_imgCmapTexSamplers; // pair of image colormaps
    static const Uniforms::SamplerIndexType msk_metricCmapTexSampler; // one colormap

    // Samplers for image shaders:
    static const Uniforms::SamplerIndexType msk_imgTexSampler; // one image
    static const Uniforms::SamplerIndexType msk_segTexSampler; // one segmentation
    static const Uniforms::SamplerIndexType msk_imgCmapTexSampler; // one image colormap
    static const Uniforms::SamplerIndexType msk_labelTableTexSampler; // one label table

    /// Is the application done loading images?
    bool m_isAppDoneLoadingImages;

    bool m_showOverlays;
};

#endif // RENDERING_H