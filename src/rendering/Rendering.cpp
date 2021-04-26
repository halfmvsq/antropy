#include "rendering/Rendering.h"

#include "common/DataHelper.h"
#include "common/DirectionMaps.h"
#include "common/Exception.hpp"
#include "common/MathFuncs.h"
#include "common/Types.h"

#include "image/ImageColorMap.h"

#include "logic/app/Data.h"
#include "logic/camera/CameraHelpers.h"
#include "logic/camera/MathUtility.h"
#include "logic/states/FsmList.hpp"

#include "rendering/ImageDrawing.h"
#include "rendering/TextureSetup.h"
#include "rendering/VectorDrawing.h"
#include "rendering/utility/containers/Uniforms.h"
#include "rendering/utility/gl/GLShader.h"

#include "windowing/View.h"

#include <cmrc/cmrc.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>

#include <glad/glad.h>

#include <nanovg.h>
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg_gl.h>

#include <algorithm>
#include <chrono>
#include <list>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>

CMRC_DECLARE(fonts);
CMRC_DECLARE(shaders);


// These types are used when setting uniforms in the shaders
using FloatVector = std::vector< float >;
using Mat4Vector = std::vector< glm::mat4 >;
using Vec2Vector = std::vector< glm::vec2 >;
using Vec3Vector = std::vector< glm::vec3 >;

namespace
{

static const glm::mat4 sk_identMat4{ 1.0f };
static const glm::vec2 sk_zeroVec2{ 0.0f, 0.0f };
static const glm::vec3 sk_zeroVec3{ 0.0f, 0.0f, 0.0f };
static const glm::vec4 sk_zeroVec4{ 0.0f, 0.0f, 0.0f, 0.0f };
static const glm::bvec2 sk_zeroBVec2{ false, false };

static const std::string ROBOTO_LIGHT( "robotoLight" );

}

const Uniforms::SamplerIndexVectorType Rendering::msk_imgTexSamplers{ { 0, 1 } };
const Uniforms::SamplerIndexVectorType Rendering::msk_segTexSamplers{ { 2, 3 } };
const Uniforms::SamplerIndexVectorType Rendering::msk_labelTableTexSamplers{ { 4, 5 } };
const Uniforms::SamplerIndexVectorType Rendering::msk_imgCmapTexSamplers{ { 6, 7 } };
const Uniforms::SamplerIndexType Rendering::msk_metricCmapTexSampler{ 6 };

const Uniforms::SamplerIndexType Rendering::msk_imgTexSampler{ 0 };
const Uniforms::SamplerIndexType Rendering::msk_segTexSampler{ 1 };
const Uniforms::SamplerIndexType Rendering::msk_imgCmapTexSampler{ 2 };
const Uniforms::SamplerIndexType Rendering::msk_labelTableTexSampler{ 3 };


Rendering::Rendering( AppData& appData )
    :
      m_appData( appData ),

      m_nvg( nvgCreateGL3( NVG_ANTIALIAS | NVG_STENCIL_STROKES /*| NVG_DEBUG*/ ) ),

      m_crossCorrelationProgram( "CrossCorrelationProgram" ),
      m_differenceProgram( "DifferenceProgram" ),
      m_imageProgram( "ImageProgram" ),
      m_edgeProgram( "EdgeProgram" ),
      m_overlayProgram( "OverlayProgram" ),
      m_simpleProgram( "SimpleProgram" ),

      m_isAppDoneLoadingImages( false ),
      m_showOverlays( true )
{
    if ( ! m_nvg )
    {
        spdlog::error( "Could not initialize nanovg. Proceeding without vector graphics." );
    }

    try
    {
        // Load the font for anatomical labels:
        auto filesystem = cmrc::fonts::get_filesystem();
        const cmrc::file robotoFont = filesystem.open( "resources/fonts/Roboto/Roboto-Light.ttf" );

        const int robotoLightFont = nvgCreateFontMem(
                    m_nvg, ROBOTO_LIGHT.c_str(),
                    reinterpret_cast<uint8_t*>( const_cast<char*>( robotoFont.begin() ) ),
                    static_cast<int32_t>( robotoFont.size() ), 0 );

        if ( -1 == robotoLightFont )
        {
            spdlog::error( "Could not load font {}", ROBOTO_LIGHT );
        }
    }
    catch ( const std::exception& e )
    {
        spdlog::error( "Exception when loading font file: {}", e.what() );
    }

    createShaderPrograms();
}

Rendering::~Rendering()
{
    if ( m_nvg )
    {
        nvgDeleteGL3( m_nvg );
        m_nvg = nullptr;
    }
}

void Rendering::setupOpenGlState()
{
    glEnable( GL_MULTISAMPLE );
    glDisable( GL_DEPTH_TEST );
    glEnable( GL_STENCIL_TEST );
    glDisable( GL_SCISSOR_TEST );
    glEnable( GL_BLEND );
    glDisable( GL_CULL_FACE );

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glFrontFace( GL_CCW );


    // This is the state touched by NanoVG:
//    glEnable(GL_CULL_FACE);
//    glCullFace(GL_BACK);
//    glFrontFace(GL_CCW);
//    glEnable(GL_BLEND);
//    glDisable(GL_DEPTH_TEST);
//    glDisable(GL_SCISSOR_TEST);
//    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
//    glStencilMask(0xffffffff);
//    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
//    glStencilFunc(GL_ALWAYS, 0, 0xffffffff);
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, 0);
}

void Rendering::init()
{
    nvgReset( m_nvg );
}

void Rendering::initTextures()
{
    m_appData.renderData().m_labelBufferTextures = createLabelColorTableTextures( m_appData );
    if ( m_appData.renderData().m_labelBufferTextures.empty() )
    {
        spdlog::critical( "No label buffer textures loaded" );
        throw_debug( "No label buffer textures loaded" )
    }

    m_appData.renderData().m_colormapTextures = createImageColorMapTextures( m_appData );
    if ( m_appData.renderData().m_colormapTextures.empty() )
    {
        spdlog::critical( "No image color map textures loaded" );
        throw_debug( "No image color map textures loaded" )
    }

    m_appData.renderData().m_imageTextures = createImageTextures( m_appData );
    m_appData.renderData().m_segTextures = createSegTextures( m_appData );

    m_isAppDoneLoadingImages = true;
}

bool Rendering::createLabelColorTableTexture( const uuids::uuid& labelTableUid )
{
    static const glm::vec4 sk_border{ 0.0f, 0.0f, 0.0f, 0.0f };

    const auto* table = m_appData.labelTable( labelTableUid );
    if ( ! table )
    {
        spdlog::warn( "Label table {} is invalid", labelTableUid );
        return false;
    }

    auto it = m_appData.renderData().m_labelBufferTextures.emplace(
                labelTableUid, tex::Target::Texture1D );

    if ( ! it.second ) return false;

    GLTexture& T = it.first->second;

    T.generate();
    T.setSize( glm::uvec3{ table->numLabels(), 1, 1 } );

    T.setData( 0, ImageColorMap::textureFormat_RGBA_F32(),
               tex::BufferPixelFormat::RGBA,
               tex::BufferPixelDataType::Float32,
               table->colorData_RGBA_premult_F32() );

    // We should never sample outside the texture coordinate range [0.0, 1.0], anyway
    T.setBorderColor( sk_border );
    T.setWrapMode( tex::WrapMode::ClampToBorder );

    // All sampling of color maps uses linearly interpolation
    T.setAutoGenerateMipmaps( false );
    T.setMinificationFilter( tex::MinificationFilter::Nearest );
    T.setMagnificationFilter( tex::MagnificationFilter::Nearest );

    spdlog::debug( "Generated texture for label color table {}", labelTableUid );
    return true;
}

bool Rendering::createSegTexture( const uuids::uuid& segUid )
{
    // Load the first pixel component of the segmentation.
    // (Segmentations should have only one component.)
    static constexpr uint32_t comp = 0;

    static constexpr GLint sk_mipmapLevel = 0; // Load seg data into first mipmap level
    static constexpr GLint sk_alignment = 1; // Pixel pack/unpack alignment is 1 byte
    static const tex::WrapMode sk_wrapMode = tex::WrapMode::ClampToBorder;
    static const glm::vec4 sk_border{ 0.0f, 0.0f, 0.0f, 0.0f }; // Black border

    static const tex::MinificationFilter sk_minFilter = tex::MinificationFilter::Nearest;
    static const tex::MagnificationFilter sk_maxFilter = tex::MagnificationFilter::Nearest;

    GLTexture::PixelStoreSettings pixelPackSettings;
    pixelPackSettings.m_alignment = sk_alignment;
    GLTexture::PixelStoreSettings pixelUnpackSettings = pixelPackSettings;

    const auto* seg = m_appData.seg( segUid );
    if ( ! seg )
    {
        spdlog::warn( "Segmentation {} is invalid", segUid );
        return false;
    }

    const ComponentType compType = seg->header().memoryComponentType();

    auto it = m_appData.renderData().m_segTextures.try_emplace(
                segUid,
                tex::Target::Texture3D,
                GLTexture::MultisampleSettings(),
                pixelPackSettings, pixelUnpackSettings );

    if ( ! it.second ) return false;
    GLTexture& T = it.first->second;

    T.generate();
    T.setMinificationFilter( sk_minFilter );
    T.setMagnificationFilter( sk_maxFilter );
    T.setBorderColor( sk_border );
    T.setWrapMode( sk_wrapMode );
    T.setAutoGenerateMipmaps( true );
    T.setSize( seg->header().pixelDimensions() );

    T.setData( sk_mipmapLevel,
               GLTexture::getSizedInternalRedFormat( compType ),
               GLTexture::getBufferPixelRedFormat( compType ),
               GLTexture::getBufferPixelDataType( compType ),
               seg->bufferAsVoid( comp ) );

    spdlog::debug( "Created texture for segmentation {} ('{}')",
                   segUid, seg->settings().displayName() );

    return true;
}

bool Rendering::removeSegTexture( const uuids::uuid& segUid )
{
    const auto* seg = m_appData.seg( segUid );
    if ( ! seg )
    {
        spdlog::warn( "Segmentation {} is invalid", segUid );
        return false;
    }

    auto it = m_appData.renderData().m_segTextures.find( segUid );

    if ( std::end( m_appData.renderData().m_segTextures ) == it )
    {
        spdlog::warn( "Texture for segmentation {} does not exist and cannot be removed", segUid );
        return false;
    }

    m_appData.renderData().m_segTextures.erase( it );
    return true;
}

void Rendering::updateSegTexture(
        const uuids::uuid& segUid,
        const ComponentType& compType,
        const glm::uvec3& startOffsetVoxel,
        const glm::uvec3& sizeInVoxels,
        const void* data )
{
    // Load seg data into first mipmap level
    static constexpr GLint sk_mipmapLevel = 0;

    auto it = m_appData.renderData().m_segTextures.find( segUid );
    if ( std::end( m_appData.renderData().m_segTextures ) == it )
    {
        spdlog::error( "Cannot update segmentation {}: texture not found.", segUid );
        return;
    }

    GLTexture& T = it->second;

    const auto* seg = m_appData.seg( segUid );

    if ( ! seg )
    {
        spdlog::warn( "Segmentation {} is invalid", segUid );
        return;
    }

    T.setSubData( sk_mipmapLevel,
                  startOffsetVoxel,
                  sizeInVoxels,
                  GLTexture::getBufferPixelRedFormat( compType ),
                  GLTexture::getBufferPixelDataType( compType ),
                  data );
}

void Rendering::updateSegTexture(
        const uuids::uuid& segUid,
        const ComponentType& compType,
        const glm::uvec3& startOffsetVoxel,
        const glm::uvec3& sizeInVoxels,
        const int64_t* data )
{
    if ( ! data )
    {
        spdlog::error( "Null segmentation texture data pointer" );
        return;
    }

    if ( ! isValidSegmentationComponentType( compType ) )
    {
        spdlog::error( "Unable to update segmentation texture using buffer with invalid "
                       "component type {}", componentTypeString( compType ) );
        return;
    }

    const size_t N = sizeInVoxels.x * sizeInVoxels.y * sizeInVoxels.z;

    switch ( compType )
    {
    case ComponentType::UInt8:
    {
        const std::vector<uint8_t> castData( data, data + N );
        return updateSegTexture( segUid, compType, startOffsetVoxel, sizeInVoxels, castData.data() );
    }
    case ComponentType::UInt16:
    {
        const std::vector<uint16_t> castData( data, data + N );
        return updateSegTexture( segUid, compType, startOffsetVoxel, sizeInVoxels, castData.data() );
    }
    case ComponentType::UInt32:
    {
        const std::vector<uint32_t> castData( data, data + N );
        return updateSegTexture( segUid, compType, startOffsetVoxel, sizeInVoxels, castData.data() );
    }
    default: return;
    }
}

Rendering::CurrentImages Rendering::getImageAndSegUidsForMetricShaders(
        const std::list<uuids::uuid>& metricImageUids ) const
{
    CurrentImages I;

    for ( const auto& imageUid : metricImageUids )
    {
        if ( I.size() >= NUM_METRIC_IMAGES ) break;

        if ( std::end( m_appData.renderData().m_imageTextures ) !=
             m_appData.renderData().m_imageTextures.find( imageUid ) )
        {
            ImgSegPair imgSegPair;

            // The texture for this image exists
            imgSegPair.first = imageUid;

            // Find the segmentation that belongs to this image
            if ( const auto segUid = m_appData.imageToActiveSegUid( imageUid ) )
            {
                if ( std::end( m_appData.renderData().m_segTextures ) !=
                     m_appData.renderData().m_segTextures.find( *segUid ) )
                {
                    // The texture for this seg exists
                    imgSegPair.second = *segUid;
                }
            }

            I.push_back( imgSegPair );
        }
    }

    // Always return at least two elements.
    while ( I.size() < Rendering::NUM_METRIC_IMAGES )
    {
        I.push_back( ImgSegPair() );
    }

    return I;
}

Rendering::CurrentImages Rendering::getImageAndSegUidsForImageShaders(
        const std::list<uuids::uuid>& imageUids ) const
{
    CurrentImages I;

    for ( const auto& imageUid : imageUids )
    {
        if ( std::end( m_appData.renderData().m_imageTextures ) !=
             m_appData.renderData().m_imageTextures.find( imageUid ) )
        {
            std::pair< std::optional<uuids::uuid>, std::optional<uuids::uuid> > p;

            // The texture for this image exists
            p.first = imageUid;

            // Find the segmentation that belongs to this image
            if ( const auto segUid = m_appData.imageToActiveSegUid( imageUid ) )
            {
                if ( std::end( m_appData.renderData().m_segTextures ) !=
                     m_appData.renderData().m_segTextures.find( *segUid ) )
                {
                    // The texture for this segmentation exists
                    p.second = *segUid;
                }
            }

            I.emplace_back( std::move( p ) );
        }
    }

    return I;
}

void Rendering::updateImageInterpolation( const uuids::uuid& imageUid )
{
    const auto* image = m_appData.image( imageUid );
    if ( ! image )
    {
        spdlog::warn( "Image {} is invalid", imageUid  );
        return;
    }

    // Modify the active component
    const uint32_t activeComp = image->settings().activeComponent();

    GLTexture& texture = m_appData.renderData().m_imageTextures.at( imageUid ).at( activeComp );

    tex::MinificationFilter minFilter;
    tex::MagnificationFilter maxFilter;

    switch ( image->settings().interpolationMode( activeComp ) )
    {
    case InterpolationMode::NearestNeighbor:
    {
        minFilter = tex::MinificationFilter::Nearest;
        maxFilter = tex::MagnificationFilter::Nearest;
        break;
    }
    case InterpolationMode::Linear:
    {
        minFilter = tex::MinificationFilter::Linear;
        maxFilter = tex::MagnificationFilter::Linear;
        break;
    }
    }

    texture.setMinificationFilter( minFilter );
    texture.setMagnificationFilter( maxFilter );

    spdlog::debug( "Set image interpolation mode for image texture {}", imageUid );
}

void Rendering::updateLabelColorTableTexture( size_t tableIndex )
{
    spdlog::trace( "Begin updating texture for 1D label color map at index {}", tableIndex );

    if ( tableIndex >= m_appData.numLabelTables() )
    {
        spdlog::error( "Label color table at index {} does not exist", tableIndex );
        return;
    }

    const auto tableUid = m_appData.labelTableUid( tableIndex );
    if ( ! tableUid )
    {
        spdlog::error( "Label table index {} is invalid", tableIndex );
        return;
    }

    const auto* table = m_appData.labelTable( *tableUid );
    if ( ! table )
    {
        spdlog::error( "Label table {} is invalid", *tableUid );
        return;
    }

    auto it = m_appData.renderData().m_labelBufferTextures.find( *tableUid );
    if ( std::end( m_appData.renderData().m_colormapTextures ) == it )
    {
        spdlog::error( "Texture for label color table {} is invalid", *tableUid );
        return;
    }

    it->second.setData(
                0, ImageColorMap::textureFormat_RGBA_F32(),
                tex::BufferPixelFormat::RGBA,
                tex::BufferPixelDataType::Float32,
                table->colorData_RGBA_premult_F32() );

    spdlog::trace( "Done updating texture for label color table {}", *tableUid );
}

void Rendering::render()
{
    // Set up OpenGL state, because it changes after NanoVG calls in the render of the prior frame
    setupOpenGlState();

    // Set the OpenGL viewport in device units:
    const glm::ivec4 deviceViewport = m_appData.windowData().viewport().getDeviceAsVec4();
    glViewport( deviceViewport[0], deviceViewport[1], deviceViewport[2], deviceViewport[3] );

    glClearColor( m_appData.renderData().m_backgroundColor.r,
                  m_appData.renderData().m_backgroundColor.g,
                  m_appData.renderData().m_backgroundColor.b, 1.0f );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

    renderImageData();
//    renderOverlays();
    renderVectorOverlays();
}

void Rendering::updateImageUniforms( uuid_range_t imageUids )
{
    for ( const auto& imageUid : imageUids )
    {
        updateImageUniforms( imageUid );
    }
}

void Rendering::updateImageUniforms( const uuids::uuid& imageUid )
{
    auto& uniforms = m_appData.renderData().m_uniforms[imageUid];

    const Image* img = m_appData.image( imageUid );

    if ( ! img )
    {
        uniforms.imgOpacity = 0.0f;
        uniforms.segOpacity = 0.0f;
        uniforms.showEdges = false;
        spdlog::error( "Image {} is null on updating its uniforms", imageUid );
        return;
    }

    const auto& imgSettings = img->settings();

    if ( const auto cmapUid = m_appData.imageColorMapUid( imgSettings.colorMapIndex() ) )
    {
        if ( const auto* map = m_appData.imageColorMap( *cmapUid ) )
        {
            uniforms.cmapSlopeIntercept = map->slopeIntercept( imgSettings.isColorMapInverted() );
        }
        else
        {
            spdlog::error( "Null image color map {} on updating uniforms for image {}",
                           *cmapUid, imageUid );
        }
    }
    else
    {
        spdlog::error( "Invalid image color map at index {} on updating uniforms for image {}",
                       imgSettings.colorMapIndex(), imageUid );
    }

    uniforms.imgTexture_T_world = img->transformations().texture_T_worldDef();
    uniforms.slopeIntercept = imgSettings.slopeInterceptTextureVec2();
    uniforms.largestSlopeIntercept = imgSettings.largestSlopeInterceptTextureVec2();

    // Map the native thresholds to OpenGL texture values:
    uniforms.thresholds = glm::vec2{ static_cast<float>( imgSettings.mapNativeIntensityToTexture( imgSettings.thresholdLow() ) ),
                                     static_cast<float>( imgSettings.mapNativeIntensityToTexture( imgSettings.thresholdHigh() ) ) };

    uniforms.imgOpacity = static_cast<float>( ( imgSettings.visibility() ? 1.0 : 0.0 ) * imgSettings.opacity() );


    // Edges
    uniforms.showEdges = imgSettings.showEdges();
    uniforms.thresholdEdges = imgSettings.thresholdEdges();
    uniforms.edgeMagnitude = static_cast<float>( imgSettings.edgeMagnitude() );
    uniforms.useFreiChen = imgSettings.useFreiChen();
//    uniforms.windowedEdges = imgSettings.windowedEdges();
    uniforms.overlayEdges = imgSettings.overlayEdges();
    uniforms.colormapEdges = imgSettings.colormapEdges();
    uniforms.edgeColor = imgSettings.edgeOpacity() * glm::vec4{ imgSettings.edgeColor(), 1.0f };


    // The the segmentation linked to this image:
    const auto& segUid = m_appData.imageToActiveSegUid( imageUid );

    if ( ! segUid )
    {
        // The image has no segmentation
        uniforms.segOpacity = 0.0f;
        return;
    }

    const Image* seg = m_appData.seg( *segUid );
    if ( ! seg )
    {
        spdlog::error( "Segmentation {} is null on updating uniforms for image {}", *segUid, imageUid );
        return;
    }

    // Make segmentation use same texture_T_world transformation as the image.
    // Otherwise, if two images use the same segmentation, there will be a problem
    // when one image moves.

    ///////// uniforms.segTexture_T_world = seg->transformations().texture_T_worldDef();
    uniforms.segTexture_T_world = img->transformations().texture_T_worldDef();

    // Both the image and segmenation must have visibility true for the segmentation to be shown
    uniforms.segOpacity = static_cast<float>(
                ( ( seg->settings().visibility() && imgSettings.visibility() ) ? 1.0 : 0.0 ) *
                seg->settings().opacity() );
}

void Rendering::updateMetricUniforms()
{
    auto update = [this] ( RenderData::MetricParams& params, const char* name )
    {
        if ( const auto cmapUid = m_appData.imageColorMapUid( params.m_colorMapIndex ) )
        {
            if ( const auto* map = m_appData.imageColorMap( *cmapUid ) )
            {
                params.m_cmapSlopeIntercept = map->slopeIntercept( params.m_invertCmap );
            }
            else
            {
                spdlog::error( "Null image color map {} on updating uniforms for {} metric",
                               *cmapUid, name );
            }
        }
        else
        {
            spdlog::error( "Invalid image color map at index {} on updating uniforms for {} metric",
                           params.m_colorMapIndex, name );
        }
    };

    update( m_appData.renderData().m_squaredDifferenceParams, "Difference" );
    update( m_appData.renderData().m_crossCorrelationParams, "Cross-Correlation" );
    update( m_appData.renderData().m_jointHistogramParams, "Joint Histogram" );
}

std::list< std::reference_wrapper<GLTexture> >
Rendering::bindImageTextures( const ImgSegPair& p )
{
    std::list< std::reference_wrapper<GLTexture> > textures;

    const auto& imageUid = p.first;
    const auto& segUid = p.second;

    const Image* image = ( imageUid ? m_appData.image( *imageUid ) : nullptr );
    const Image* seg = ( segUid ? m_appData.seg( *segUid ) : nullptr );

    const auto cmapUid = ( image ? m_appData.imageColorMapUid( image->settings().colorMapIndex() ) : std::nullopt );
    const auto tableUid = ( seg ? m_appData.labelTableUid( seg->settings().labelTableIndex() ) : std::nullopt );

    if ( image )
    {
        // Bind the active component of the image
        const uint32_t activeComp = image->settings().activeComponent();
        GLTexture& T = m_appData.renderData().m_imageTextures.at( *imageUid ).at( activeComp );
        T.bind( msk_imgTexSampler.index );
        textures.push_back( T );
    }
    else
    {
        // No image, so bind the blank one:
        GLTexture& T = m_appData.renderData().m_blankImageTexture;
        T.bind( msk_imgTexSampler.index );
        textures.push_back( T );
    }

    if ( segUid )
    {
        GLTexture& T = m_appData.renderData().m_segTextures.at( *segUid );
        T.bind( msk_segTexSampler.index );
        textures.push_back( T );
    }
    else
    {
        // No segmentation, so bind the blank one:
        GLTexture& T = m_appData.renderData().m_blankSegTexture;
        T.bind( msk_segTexSampler.index );
        textures.push_back( T );
    }

    if ( cmapUid )
    {
        GLTexture& T = m_appData.renderData().m_colormapTextures.at( *cmapUid );
        T.bind( msk_imgCmapTexSampler.index );
        textures.push_back( T );
    }
    else
    {
        // No colormap, so bind the first available one:
        auto it = std::begin( m_appData.renderData().m_colormapTextures );
        GLTexture& T = it->second;
        T.bind( msk_imgCmapTexSampler.index );
        textures.push_back( T );
    }

    if ( tableUid )
    {
        GLTexture& T = m_appData.renderData().m_labelBufferTextures.at( *tableUid );
        T.bind( msk_labelTableTexSampler.index );
        textures.push_back( T );
    }
    else
    {
        // No label table, so bind the first available one:
        auto it = std::begin( m_appData.renderData().m_labelBufferTextures );
        GLTexture& T = it->second;
        T.bind( msk_labelTableTexSampler.index );
        textures.push_back( T );
    }

    return textures;
}

void Rendering::unbindTextures( const std::list< std::reference_wrapper<GLTexture> >& textures )
{
    for ( auto& T : textures )
    {
        T.get().unbind();
    }
}

std::list< std::reference_wrapper<GLTexture> >
Rendering::bindMetricImageTextures(
        const CurrentImages& I,
        const camera::ViewRenderMode& metricType )
{
    std::list< std::reference_wrapper<GLTexture> > textures;

    bool usesMetricColormap = false;
    size_t metricCmapIndex = 0;

    switch ( metricType )
    {
    case camera::ViewRenderMode::Difference:
    {
        usesMetricColormap = true;
        metricCmapIndex = m_appData.renderData().m_squaredDifferenceParams.m_colorMapIndex;
        break;
    }
    case camera::ViewRenderMode::CrossCorrelation:
    {
        usesMetricColormap = true;
        metricCmapIndex = m_appData.renderData().m_crossCorrelationParams.m_colorMapIndex;
        break;
    }
    case camera::ViewRenderMode::JointHistogram:
    {
        usesMetricColormap = true;
        metricCmapIndex = m_appData.renderData().m_jointHistogramParams.m_colorMapIndex;
        break;
    }
    case camera::ViewRenderMode::Overlay:
    {
        usesMetricColormap = false;
        break;
    }
    case camera::ViewRenderMode::Disabled:
    {
        return textures;
    }
    default:
    {
        spdlog::error( "Invalid metric shader type {}", camera::typeString( metricType ) );
        return textures;
    }
    }

    if ( usesMetricColormap )
    {
        const auto cmapUid = m_appData.imageColorMapUid( metricCmapIndex );

        if ( cmapUid )
        {
            GLTexture& T = m_appData.renderData().m_colormapTextures.at( *cmapUid );
            T.bind( msk_metricCmapTexSampler.index );
            textures.push_back( T );
        }
        else
        {
            auto it = std::begin( m_appData.renderData().m_colormapTextures );
            GLTexture& T = it->second;
            T.bind( msk_metricCmapTexSampler.index );
            textures.push_back( T );
        }
    }

    size_t i = 0;

    for ( const auto& imgSegPair : I )
    {
        const auto& imageUid = imgSegPair.first;
        const auto& segUid = imgSegPair.second;

        const Image* image = ( imageUid ? m_appData.image( *imageUid ) : nullptr );
        const Image* seg = ( segUid ? m_appData.seg( *segUid ) : nullptr );
        const auto tableUid = ( seg ? m_appData.labelTableUid( seg->settings().labelTableIndex() ) : std::nullopt );

        if ( image )
        {
            // Bind the active component
            const uint32_t activeComp = image->settings().activeComponent();
            GLTexture& T = m_appData.renderData().m_imageTextures.at( *imageUid ).at( activeComp );
            T.bind( msk_imgTexSamplers.indices[i] );
            textures.push_back( T );
        }
        else
        {
            GLTexture& T = m_appData.renderData().m_blankImageTexture;
            T.bind( msk_imgTexSamplers.indices[i] );
            textures.push_back( T );
        }

        if ( segUid )
        {
            GLTexture& T = m_appData.renderData().m_segTextures.at( *segUid );
            T.bind( msk_segTexSamplers.indices[i] );
            textures.push_back( T );
        }
        else
        {
            GLTexture& T = m_appData.renderData().m_blankSegTexture;
            T.bind( msk_segTexSamplers.indices[i] );
            textures.push_back( T );
        }

        if ( tableUid )
        {
            GLTexture& T = m_appData.renderData().m_labelBufferTextures.at( *tableUid );
            T.bind( msk_labelTableTexSamplers.indices[i] );
            textures.push_back( T );
        }
        else
        {
            // No table specified, so bind the first available one:
            auto it = std::begin( m_appData.renderData().m_labelBufferTextures );
            GLTexture& T = it->second;
            T.bind( msk_labelTableTexSamplers.indices[i] );
            textures.push_back( T );
        }

        ++i;
    }

    return textures;
}

void Rendering::renderOneImage(
        const View& view,
        const camera::FrameBounds& miewportViewBounds,
        const glm::vec3& worldOffsetXhairs,
        GLShaderProgram& program,
        const CurrentImages& I,
        bool showEdges )
{
    auto getImage = [this] ( const std::optional<uuids::uuid>& imageUid ) -> const Image*
    {
        return ( imageUid ? m_appData.image( *imageUid ) : nullptr );
    };

    auto& renderData = m_appData.renderData();

    drawImageQuad( program, view.renderMode(), renderData.m_quad, view, worldOffsetXhairs,
                   renderData.m_flashlightRadius, renderData.m_flashlightOverlays, I, getImage, showEdges );

    if ( ! renderData.m_globalLandmarkParams.renderOnTopOfAllImagePlanes )
    {
        drawLandmarks( m_nvg, miewportViewBounds, worldOffsetXhairs, m_appData, view, I );
        setupOpenGlState();
    }

    if ( ! renderData.m_globalAnnotationParams.renderOnTopOfAllImagePlanes )
    {
        drawAnnotations( m_nvg, miewportViewBounds, worldOffsetXhairs, m_appData, view, I );
        setupOpenGlState();
    }

    drawImageViewIntersections( m_nvg, miewportViewBounds, worldOffsetXhairs, m_appData, view, I,
                                renderData.m_globalSliceIntersectionParams.renderInactiveImageViewIntersections );
    setupOpenGlState();
}

void Rendering::renderAllImages(
        const View& view,
        const camera::FrameBounds& miewportViewBounds,
        const glm::vec3& worldOffsetXhairs )
{
    static std::list< std::reference_wrapper<GLTexture> > boundImageTextures;
    static std::list< std::reference_wrapper<GLTexture> > boundMetricTextures;

    static const RenderData::ImageUniforms sk_defaultImageUniforms;

    auto& renderData = m_appData.renderData();
    const bool modSegOpacity = renderData.m_modulateSegOpacityWithImageOpacity;

    const auto shaderType = view.renderMode();
    const auto metricImages = view.metricImages();
    const auto renderedImages = view.renderedImages();


    if ( camera::ViewRenderMode::Image == shaderType ||
         camera::ViewRenderMode::Checkerboard == shaderType ||
         camera::ViewRenderMode::Quadrants == shaderType ||
         camera::ViewRenderMode::Flashlight == shaderType )
    {
        CurrentImages I;

        int renderMode = 0;

        if ( camera::ViewRenderMode::Image == shaderType )
        {
            renderMode = 0;
            I = getImageAndSegUidsForImageShaders( renderedImages );
        }
        else if ( camera::ViewRenderMode::Checkerboard == shaderType )
        {
            renderMode = 1;
            I = getImageAndSegUidsForMetricShaders( metricImages ); // guaranteed size 2
        }
        else if ( camera::ViewRenderMode::Quadrants == shaderType )
        {
            renderMode = 2;
            I = getImageAndSegUidsForMetricShaders( metricImages );
        }
        else if ( camera::ViewRenderMode::Flashlight == shaderType )
        {
            renderMode = 3;
            I = getImageAndSegUidsForMetricShaders( metricImages );
        }


        bool isFixedImage = true; // true for the first image

        for ( const auto& imgSegPair : I )
        {
            if ( ! imgSegPair.first )
            {
                isFixedImage = false;
                continue;
            }

            boundImageTextures = bindImageTextures( imgSegPair );

            const auto& U = renderData.m_uniforms.at( *imgSegPair.first );

            GLShaderProgram& P = ( U.showEdges ) ? m_edgeProgram : m_imageProgram;

            P.use();
            {
                P.setSamplerUniform( "imgTex", msk_imgTexSampler.index );
                P.setSamplerUniform( "segTex", msk_segTexSampler.index );
                P.setSamplerUniform( "imgCmapTex", msk_imgCmapTexSampler.index );
                P.setSamplerUniform( "segLabelCmapTex", msk_labelTableTexSampler.index );

                P.setUniform( "numSquares", static_cast<float>( renderData.m_numCheckerboardSquares ) );
                P.setUniform( "imgTexture_T_world", U.imgTexture_T_world );
                P.setUniform( "segTexture_T_world", U.segTexture_T_world );
                P.setUniform( "imgSlopeIntercept", U.slopeIntercept );
                P.setUniform( "imgSlopeInterceptLargest", U.largestSlopeIntercept );
                P.setUniform( "imgCmapSlopeIntercept", U.cmapSlopeIntercept );
                P.setUniform( "imgThresholds", U.thresholds );
                P.setUniform( "imgOpacity", U.imgOpacity );
                P.setUniform( "segOpacity", U.segOpacity * ( modSegOpacity ? U.imgOpacity : 1.0f ) );
                P.setUniform( "masking", renderData.m_maskedImages );
                P.setUniform( "quadrants", renderData.m_quadrants );
                P.setUniform( "showFix", isFixedImage ); // ignored if not checkerboard or quadrants
                P.setUniform( "renderMode", renderMode );

                if ( U.showEdges )
                {
                     P.setUniform( "thresholdEdges", U.thresholdEdges );
                     P.setUniform( "edgeMagnitude", U.edgeMagnitude );
//                     P.setUniform( "useFreiChen", U.useFreiChen );
                     P.setUniform( "overlayEdges", U.overlayEdges );
                     P.setUniform( "colormapEdges", U.colormapEdges );
                     P.setUniform( "edgeColor", U.edgeColor );
                }

                renderOneImage( view, miewportViewBounds, worldOffsetXhairs,
                                P, CurrentImages{ imgSegPair }, U.showEdges );
            }
            P.stopUse();

            unbindTextures( boundImageTextures );

            isFixedImage = false;
        }
    }
    else if ( camera::ViewRenderMode::Disabled == shaderType )
    {
        return;
    }
    else
    {
        // This function guarantees that I has size at least 2:
        const CurrentImages I = getImageAndSegUidsForMetricShaders( metricImages );

        const auto& U0 = ( I.size() >= 1 && I[0].first ) ? renderData.m_uniforms.at( *I[0].first ) : sk_defaultImageUniforms;
        const auto& U1 = ( I.size() >= 2 && I[1].first ) ? renderData.m_uniforms.at( *I[1].first ) : sk_defaultImageUniforms;

        boundMetricTextures = bindMetricImageTextures( I, shaderType );

        if ( camera::ViewRenderMode::Difference == shaderType )
        {
            const auto& metricParams = renderData.m_squaredDifferenceParams;
            GLShaderProgram& P = m_differenceProgram;

            P.use();
            {
                P.setSamplerUniform( "imgTex", msk_imgTexSamplers );
                P.setSamplerUniform( "segTex", msk_segTexSamplers );
                P.setSamplerUniform( "segLabelCmapTex", msk_labelTableTexSamplers );
                P.setSamplerUniform( "metricCmapTex", msk_metricCmapTexSampler.index );

                P.setUniform( "imgTexture_T_world", std::vector<glm::mat4>{ U0.imgTexture_T_world, U1.imgTexture_T_world } );
                P.setUniform( "segTexture_T_world", std::vector<glm::mat4>{ U0.segTexture_T_world, U1.segTexture_T_world } );
                P.setUniform( "imgSlopeIntercept", std::vector<glm::vec2>{ U0.largestSlopeIntercept, U1.largestSlopeIntercept } );
                P.setUniform( "segOpacity", std::vector<float>{ U0.segOpacity, U1.segOpacity } );

                P.setUniform( "metricCmapSlopeIntercept", metricParams.m_cmapSlopeIntercept );
                P.setUniform( "metricSlopeIntercept", metricParams.m_slopeIntercept );
                P.setUniform( "metricMasking", metricParams.m_doMasking );

                P.setUniform( "useSquare", renderData.m_useSquare );

                renderOneImage( view, miewportViewBounds, worldOffsetXhairs, P, I, false );
            }
            P.stopUse();
        }
        else if ( camera::ViewRenderMode::CrossCorrelation == shaderType )
        {
            const auto& metricParams = renderData.m_crossCorrelationParams;
            GLShaderProgram& P = m_crossCorrelationProgram;

            P.use();
            {
                P.setSamplerUniform( "imgTex", msk_imgTexSamplers );
                P.setSamplerUniform( "segTex", msk_segTexSamplers );
                P.setSamplerUniform( "segLabelCmapTex", msk_labelTableTexSamplers );
                P.setSamplerUniform( "metricCmapTex", msk_metricCmapTexSampler.index );

                P.setUniform( "imgTexture_T_world", std::vector<glm::mat4>{ U0.imgTexture_T_world, U1.imgTexture_T_world } );
                P.setUniform( "segTexture_T_world", std::vector<glm::mat4>{ U0.segTexture_T_world, U1.segTexture_T_world } );
                P.setUniform( "segOpacity", std::vector<float>{ U0.segOpacity, U1.segOpacity } );

                P.setUniform( "metricCmapSlopeIntercept", metricParams.m_cmapSlopeIntercept );
                P.setUniform( "metricSlopeIntercept", metricParams.m_slopeIntercept );
                P.setUniform( "metricMasking", metricParams.m_doMasking );

                P.setUniform( "texture1_T_texture0", U1.imgTexture_T_world * glm::inverse( U0.imgTexture_T_world ) );

                renderOneImage( view, miewportViewBounds, worldOffsetXhairs, P, I, false );
            }
            P.stopUse();
        }
        else if ( camera::ViewRenderMode::Overlay == shaderType )
        {
            GLShaderProgram& P = m_overlayProgram;

            P.use();
            {
                P.setSamplerUniform( "imgTex", msk_imgTexSamplers );
                P.setSamplerUniform( "segTex", msk_segTexSamplers );
                P.setSamplerUniform( "segLabelCmapTex", msk_labelTableTexSamplers );

                P.setUniform( "imgTexture_T_world", std::vector<glm::mat4>{ U0.imgTexture_T_world, U1.imgTexture_T_world } );
                P.setUniform( "segTexture_T_world", std::vector<glm::mat4>{ U0.segTexture_T_world, U1.segTexture_T_world } );
                P.setUniform( "imgSlopeIntercept", std::vector<glm::vec2>{ U0.slopeIntercept, U1.slopeIntercept } );
                P.setUniform( "imgThresholds", std::vector<glm::vec2>{ U0.thresholds, U1.thresholds } );
                P.setUniform( "imgOpacity", std::vector<float>{ U0.imgOpacity, U1.imgOpacity } );

                P.setUniform( "segOpacity", std::vector<float>{
                                  U0.segOpacity * ( modSegOpacity ? U0.imgOpacity : 1.0f ),
                                  U1.segOpacity * ( modSegOpacity ? U1.imgOpacity : 1.0f ) } );

                P.setUniform( "magentaCyan", renderData.m_overlayMagentaCyan );

                renderOneImage( view, miewportViewBounds, worldOffsetXhairs, P, I, false );
            }
            P.stopUse();
        }

        unbindTextures( boundMetricTextures );
    }
}

void Rendering::renderAllLandmarks(
        const View& view,
        const camera::FrameBounds& miewportViewBounds,
        const glm::vec3& worldOffsetXhairs )
{
    const auto shaderType = view.renderMode();
    const auto metricImages = view.metricImages();
    const auto renderedImages = view.renderedImages();

    CurrentImages I;

    if ( camera::ViewRenderMode::Image == shaderType )
    {
        I = getImageAndSegUidsForImageShaders( renderedImages );

        for ( const auto& imgSegPair : I )
        {
            drawLandmarks( m_nvg, miewportViewBounds, worldOffsetXhairs,
                           m_appData, view, CurrentImages{ imgSegPair } );
            setupOpenGlState();
        }
    }
    else if ( camera::ViewRenderMode::Checkerboard == shaderType ||
              camera::ViewRenderMode::Quadrants == shaderType ||
              camera::ViewRenderMode::Flashlight == shaderType )
    {
        I = getImageAndSegUidsForMetricShaders( metricImages ); // guaranteed size 2

        for ( const auto& imgSegPair : I )
        {
            drawLandmarks( m_nvg, miewportViewBounds, worldOffsetXhairs,
                           m_appData, view, CurrentImages{ imgSegPair } );
            setupOpenGlState();
        }
    }
    else if ( camera::ViewRenderMode::Disabled == shaderType )
    {
        return;
    }
    else
    {
        // This function guarantees that I has size at least 2:
        drawLandmarks( m_nvg, miewportViewBounds, worldOffsetXhairs,
                       m_appData, view, getImageAndSegUidsForMetricShaders( metricImages ) );
        setupOpenGlState();
    }
}

void Rendering::renderAllAnnotations(
        const View& view,
        const camera::FrameBounds& miewportViewBounds,
        const glm::vec3& worldOffsetXhairs )
{
    const auto shaderType = view.renderMode();
    const auto metricImages = view.metricImages();
    const auto renderedImages = view.renderedImages();

    CurrentImages I;

    if ( camera::ViewRenderMode::Image == shaderType )
    {
        I = getImageAndSegUidsForImageShaders( renderedImages );

        for ( const auto& imgSegPair : I )
        {
            drawAnnotations( m_nvg, miewportViewBounds, worldOffsetXhairs,
                             m_appData, view, CurrentImages{ imgSegPair } );
            setupOpenGlState();
        }
    }
    else if ( camera::ViewRenderMode::Checkerboard == shaderType ||
              camera::ViewRenderMode::Quadrants == shaderType ||
              camera::ViewRenderMode::Flashlight == shaderType )
    {
        I = getImageAndSegUidsForMetricShaders( metricImages ); // guaranteed size 2

        for ( const auto& imgSegPair : I )
        {
            drawAnnotations( m_nvg, miewportViewBounds, worldOffsetXhairs,
                             m_appData, view, CurrentImages{ imgSegPair } );
            setupOpenGlState();
        }
    }
    else if ( camera::ViewRenderMode::Disabled == shaderType )
    {
        return;
    }
    else
    {
        // This function guarantees that I has size at least 2:
        drawAnnotations( m_nvg, miewportViewBounds, worldOffsetXhairs,
                         m_appData, view, getImageAndSegUidsForMetricShaders( metricImages ) );
        setupOpenGlState();
    }
}

void Rendering::renderImageData()
{
    if ( ! m_isAppDoneLoadingImages )
    {
        // Don't render images if the app is still loading them
        return;
    }

    auto& renderData = m_appData.renderData();

    const bool renderLandmarksOnTop = renderData.m_globalLandmarkParams.renderOnTopOfAllImagePlanes;
    const bool renderAnnotationsOnTop = renderData.m_globalAnnotationParams.renderOnTopOfAllImagePlanes;

    for ( const auto& viewPair : m_appData.windowData().currentLayout().views() )
    {
        if ( ! viewPair.second ) continue;
        View& view = *( viewPair.second );

        const glm::vec3 worldOffsetXhairs = view.updateImageSlice(
                    m_appData, m_appData.state().worldCrosshairs().worldOrigin() );

        const auto miewportViewBounds = camera::computeMiewportFrameBounds(
                    view.windowClipViewport(), m_appData.windowData().viewport().getAsVec4() );

        renderAllImages( view, miewportViewBounds, worldOffsetXhairs );

        if ( renderLandmarksOnTop )
        {
            renderAllLandmarks( view, miewportViewBounds, worldOffsetXhairs );
        }

        if ( renderAnnotationsOnTop )
        {
            renderAllAnnotations( view, miewportViewBounds, worldOffsetXhairs );
        }
    }
}

void Rendering::renderOverlays()
{
    /*
    m_simpleProgram.use();
    {
        for ( const auto& view : m_appData.m_windowData.currentViews() )
        {
            switch ( view.renderMode() )
            {
            case camera::ShaderType::Image:
            case camera::ShaderType::MetricMI:
            case camera::ShaderType::MetricNCC:
            case camera::ShaderType::MetricSSD:
            case camera::ShaderType::Checkerboard:
            {
                renderCrosshairs( m_simpleProgram, m_appData.renderData().m_quad, view,
                                  m_appData.m_worldCrosshairs.worldOrigin() );
                break;
            }
            case camera::ShaderType::None:
            {
                break;
            }
            }
        }
    }
    m_simpleProgram.stopUse();
    */
}

void Rendering::renderVectorOverlays()
{
    if ( ! m_nvg ) return;

    WindowData& windowData = m_appData.windowData();
    const Viewport& windowVP = windowData.viewport();

    startNvgFrame( m_nvg, windowVP );

    if ( m_isAppDoneLoadingImages )
    {
        glm::mat4 world_T_refSubject( 1.0f );

        if ( m_appData.settings().lockAnatomicalCoordinateAxesWithReferenceImage() )
        {
            if ( const Image* refImage = m_appData.refImage() )
            {
                world_T_refSubject = refImage->transformations().worldDef_T_subject();
            }
        }

        for ( const auto& viewUid : windowData.currentViewUids() )
        {
            const View* view = windowData.getCurrentView( viewUid );
            if ( ! view ) continue;

            // Bounds of the view frame in Miewport space:
            const auto miewportViewBounds = camera::computeMiewportFrameBounds(
                        view->windowClipViewport(), windowVP.getAsVec4() );

            if ( m_showOverlays &&
                 camera::ViewRenderMode::Disabled != view->renderMode() )
            {
                const auto labelPosInfo = computeAnatomicalLabelPosInfo(
                            miewportViewBounds,
                            windowVP, *view, world_T_refSubject,
                            m_appData.state().worldCrosshairs().worldOrigin() );

                drawCrosshairs( m_nvg, miewportViewBounds, *view, m_appData.renderData().m_crosshairsColor, labelPosInfo );
                drawAnatomicalLabels( m_nvg, miewportViewBounds, m_appData.renderData().m_anatomicalLabelColor, labelPosInfo );
            }


            ViewOutlineMode outlineMode = ViewOutlineMode::None;

            if ( ASM::current_state_ptr )
            {
                if ( const auto hoveredViewUid = ASM::current_state_ptr->m_hoveredViewUid )
                {
                    if ( ( viewUid == *hoveredViewUid ) )
                    {
                        outlineMode = ViewOutlineMode::Hovered;
                    }
                }

                if ( const auto selectedViewUid = ASM::current_state_ptr->m_selectedViewUid )
                {
                    if ( ( viewUid == *selectedViewUid ) )
                    {
                        outlineMode = ViewOutlineMode::Selected;
                    }
                }
            }

            drawViewOutline( m_nvg, miewportViewBounds, outlineMode );
        }

        drawWindowOutline( m_nvg, windowVP );
    }
    else
    {
        drawLoadingOverlay( m_nvg, windowVP );

        //            nvgFontSize( m_nvg, 64.0f );
        //            const char* txt = "Text me up.";
        //            float bounds[4];
        //            nvgTextBounds( m_nvg, 10, 10, txt, NULL, bounds );
        //            nvgBeginPath( m_nvg );
        ////            nvgRoundedRect( m_nvg, bounds[0],bounds[1], bounds[2]-bounds[0], bounds[3]-bounds[1], 0 );
        //            nvgText( m_nvg, vp.width() / 2, vp.height() / 2, "Loading images...", NULL );
        //            nvgFill( m_nvg );
    }

    endNvgFrame( m_nvg );
}

void Rendering::createShaderPrograms()
{   
    if ( ! createCrossCorrelationProgram( m_crossCorrelationProgram ) )
    {
        throw_debug( "Failed to create cross-correlation program" )
    }

    if ( ! createDifferenceProgram( m_differenceProgram ) )
    {
        throw_debug( "Failed to create difference program" )
    }

    if ( ! createEdgeProgram( m_edgeProgram ) )
    {
        throw_debug( "Failed to create edge program" )
    }

    if ( ! createImageProgram( m_imageProgram ) )
    {
        throw_debug( "Failed to create image program" )
    }

    if ( ! createOverlayProgram( m_overlayProgram ) )
    {
        throw_debug( "Failed to create overlay program" )
    }

    if ( ! createSimpleProgram( m_simpleProgram ) )
    {
        throw_debug( "Failed to create simple program" )
    }
}

bool Rendering::createImageProgram( GLShaderProgram& program )
{
    static const std::string vsFileName{ "src/rendering/shaders/Image.vs" };
    static const std::string fsFileName{ "src/rendering/shaders/Image.fs" };

    auto filesystem = cmrc::shaders::get_filesystem();
    std::string vsSource;
    std::string fsSource;

    try
    {
        cmrc::file vsData = filesystem.open( vsFileName.c_str() );
        cmrc::file fsData = filesystem.open( fsFileName.c_str() );

        vsSource = std::string( vsData.begin(), vsData.end() );
        fsSource = std::string( fsData.begin(), fsData.end() );
    }
    catch ( const std::exception& e )
    {
        spdlog::critical( "Exception when loading shader file: {}", e.what() );
        throw_debug( "Unable to load shader" )
    }

    {
        Uniforms vsUniforms;
        vsUniforms.insertUniform( "view_T_clip", UniformType::Mat4, sk_identMat4 );
        vsUniforms.insertUniform( "world_T_clip", UniformType::Mat4, sk_identMat4 );
        vsUniforms.insertUniform( "clipDepth", UniformType::Float, 0.0f );

        // For checkerboarding:
        vsUniforms.insertUniform( "aspectRatio", UniformType::Float, 1.0f );
        vsUniforms.insertUniform( "numSquares", UniformType::Int, 1 );

        vsUniforms.insertUniform( "imgTexture_T_world", UniformType::Mat4, sk_identMat4 );
        vsUniforms.insertUniform( "segTexture_T_world", UniformType::Mat4, sk_identMat4 );

        auto vs = std::make_shared<GLShader>( "vsImage", ShaderType::Vertex, vsSource.c_str() );
        vs->setRegisteredUniforms( std::move( vsUniforms ) );
        program.attachShader( vs );

        spdlog::debug( "Compiled vertex shader {}", vsFileName );
    }

    {   
        Uniforms fsUniforms;

        fsUniforms.insertUniform( "imgTex", UniformType::Sampler, msk_imgTexSampler );
        fsUniforms.insertUniform( "segTex", UniformType::Sampler, msk_segTexSampler );
        fsUniforms.insertUniform( "imgCmapTex", UniformType::Sampler, msk_imgCmapTexSampler );
        fsUniforms.insertUniform( "segLabelCmapTex", UniformType::Sampler, msk_labelTableTexSampler );

        fsUniforms.insertUniform( "imgSlopeIntercept", UniformType::Vec2, sk_zeroVec2 );
        fsUniforms.insertUniform( "imgCmapSlopeIntercept", UniformType::Vec2, sk_zeroVec2 );
        fsUniforms.insertUniform( "imgThresholds", UniformType::Vec2, sk_zeroVec2 );
        fsUniforms.insertUniform( "imgOpacity", UniformType::Float, 0.0f );
        fsUniforms.insertUniform( "segOpacity", UniformType::Float, 0.0f );

        fsUniforms.insertUniform( "masking", UniformType::Bool, false );

        fsUniforms.insertUniform( "quadrants", UniformType::BVec2, sk_zeroBVec2 ); // For quadrants
        fsUniforms.insertUniform( "showFix", UniformType::Bool, true ); // For checkerboarding
        fsUniforms.insertUniform( "renderMode", UniformType::Int, 0 ); // 0: image, 1: checkerboard, 2: quadrants, 3: flashlight

        // For flashlighting:
        fsUniforms.insertUniform( "flashlightRadius", UniformType::Float, 0.5f );
        fsUniforms.insertUniform( "flashlightOverlays", UniformType::Bool, true );

        auto fs = std::make_shared<GLShader>( "fsImage", ShaderType::Fragment, fsSource.c_str() );
        fs->setRegisteredUniforms( std::move( fsUniforms ) );
        program.attachShader( fs );

        spdlog::debug( "Compiled fragment shader {}", fsFileName );
    }

    if ( ! program.link() )
    {
        spdlog::critical( "Failed to link shader program {}", program.name() );
        return false;
    }

    spdlog::debug( "Linked shader program {}", program.name() );
    return true;
}

bool Rendering::createEdgeProgram( GLShaderProgram& program )
{
    static const std::string vsFileName{ "src/rendering/shaders/Edge.vs" };
    static const std::string fsFileName{ "src/rendering/shaders/Edge.fs" };

    auto filesystem = cmrc::shaders::get_filesystem();
    std::string vsSource;
    std::string fsSource;

    try
    {
        cmrc::file vsData = filesystem.open( vsFileName.c_str() );
        cmrc::file fsData = filesystem.open( fsFileName.c_str() );

        vsSource = std::string( vsData.begin(), vsData.end() );
        fsSource = std::string( fsData.begin(), fsData.end() );
    }
    catch ( const std::exception& e )
    {
        spdlog::critical( "Exception when loading shader file: {}", e.what() );
        throw_debug( "Unable to load shader" )
    }

    {
        Uniforms vsUniforms;
        vsUniforms.insertUniform( "view_T_clip", UniformType::Mat4, sk_identMat4 );
        vsUniforms.insertUniform( "world_T_clip", UniformType::Mat4, sk_identMat4 );
        vsUniforms.insertUniform( "clipDepth", UniformType::Float, 0.0f );

        // For checkerboarding:
        vsUniforms.insertUniform( "aspectRatio", UniformType::Float, 1.0f );
        vsUniforms.insertUniform( "numSquares", UniformType::Int, 1 );

        vsUniforms.insertUniform( "imgTexture_T_world", UniformType::Mat4, sk_identMat4 );
        vsUniforms.insertUniform( "segTexture_T_world", UniformType::Mat4, sk_identMat4 );

        auto vs = std::make_shared<GLShader>( "vsEdge", ShaderType::Vertex, vsSource.c_str() );
        vs->setRegisteredUniforms( std::move( vsUniforms ) );
        program.attachShader( vs );

        spdlog::debug( "Compiled vertex shader {}", vsFileName );
    }

    {
        Uniforms fsUniforms;

        fsUniforms.insertUniform( "imgTex", UniformType::Sampler, msk_imgTexSampler );
        fsUniforms.insertUniform( "segTex", UniformType::Sampler, msk_segTexSampler );
        fsUniforms.insertUniform( "imgCmapTex", UniformType::Sampler, msk_imgCmapTexSampler );
        fsUniforms.insertUniform( "segLabelCmapTex", UniformType::Sampler, msk_labelTableTexSampler );

        fsUniforms.insertUniform( "imgSlopeIntercept", UniformType::Vec2, sk_zeroVec2 );
        fsUniforms.insertUniform( "imgSlopeInterceptLargest", UniformType::Vec2, sk_zeroVec2 );
        fsUniforms.insertUniform( "imgCmapSlopeIntercept", UniformType::Vec2, sk_zeroVec2 );
        fsUniforms.insertUniform( "imgThresholds", UniformType::Vec2, sk_zeroVec2 );
        fsUniforms.insertUniform( "imgOpacity", UniformType::Float, 0.0f );
        fsUniforms.insertUniform( "segOpacity", UniformType::Float, 0.0f );

        fsUniforms.insertUniform( "masking", UniformType::Bool, false );

        fsUniforms.insertUniform( "quadrants", UniformType::BVec2, sk_zeroBVec2 );
        fsUniforms.insertUniform( "showFix", UniformType::Bool, true );
        fsUniforms.insertUniform( "renderMode", UniformType::Int, 0 );

        // For flashlighting:
        fsUniforms.insertUniform( "flashlightRadius", UniformType::Float, 0.5f );
        fsUniforms.insertUniform( "flashlightOverlays", UniformType::Bool, true );

        fsUniforms.insertUniform( "thresholdEdges", UniformType::Bool, true );
        fsUniforms.insertUniform( "edgeMagnitude", UniformType::Float, 0.0f );
//        fsUniforms.insertUniform( "useFreiChen", UniformType::Bool, false );
//        fsUniforms.insertUniform( "windowedEdges", UniformType::Bool, false );
        fsUniforms.insertUniform( "overlayEdges", UniformType::Bool, false );
        fsUniforms.insertUniform( "colormapEdges", UniformType::Bool, false );
        fsUniforms.insertUniform( "edgeColor", UniformType::Vec4, sk_zeroVec4 );

        fsUniforms.insertUniform( "texSamplingDirX", UniformType::Vec3, sk_zeroVec3 );
        fsUniforms.insertUniform( "texSamplingDirY", UniformType::Vec3, sk_zeroVec3 );

        auto fs = std::make_shared<GLShader>( "fsEdge", ShaderType::Fragment, fsSource.c_str() );
        fs->setRegisteredUniforms( std::move( fsUniforms ) );
        program.attachShader( fs );

        spdlog::debug( "Compiled fragment shader {}", fsFileName );
    }

    if ( ! program.link() )
    {
        spdlog::critical( "Failed to link shader program {}", program.name() );
        return false;
    }

    spdlog::debug( "Linked shader program {}", program.name() );
    return true;
}

bool Rendering::createOverlayProgram( GLShaderProgram& program )
{
    static const std::string vsFileName{ "src/rendering/shaders/Overlay.vs" };
    static const std::string fsFileName{ "src/rendering/shaders/Overlay.fs" };

    auto filesystem = cmrc::shaders::get_filesystem();
    std::string vsSource;
    std::string fsSource;

    try
    {
        cmrc::file vsData = filesystem.open( vsFileName.c_str() );
        cmrc::file fsData = filesystem.open( fsFileName.c_str() );

        vsSource = std::string( vsData.begin(), vsData.end() );
        fsSource = std::string( fsData.begin(), fsData.end() );
    }
    catch ( const std::exception& e )
    {
        spdlog::critical( "Exception when loading shader file: {}", e.what() );
        throw_debug( "Unable to load shader" )
    }

    {
        Uniforms vsUniforms;
        vsUniforms.insertUniform( "view_T_clip", UniformType::Mat4, sk_identMat4 );
        vsUniforms.insertUniform( "world_T_clip", UniformType::Mat4, sk_identMat4 );
        vsUniforms.insertUniform( "clipDepth", UniformType::Float, 0.0f );

        vsUniforms.insertUniform( "imgTexture_T_world", UniformType::Mat4Vector, Mat4Vector{ sk_identMat4, sk_identMat4 } );
        vsUniforms.insertUniform( "segTexture_T_world", UniformType::Mat4Vector, Mat4Vector{ sk_identMat4, sk_identMat4 } );

        auto vs = std::make_shared<GLShader>( "vsOverlay", ShaderType::Vertex, vsSource.c_str() );
        vs->setRegisteredUniforms( std::move( vsUniforms ) );
        program.attachShader( vs );

        spdlog::debug( "Compiled vertex shader {}", vsFileName );
    }

    {
        Uniforms fsUniforms;

        fsUniforms.insertUniform( "imgTex", UniformType::SamplerVector, msk_imgTexSamplers );
        fsUniforms.insertUniform( "segTex", UniformType::SamplerVector, msk_segTexSamplers );

        fsUniforms.insertUniform( "segLabelCmapTex", UniformType::SamplerVector, msk_labelTableTexSamplers );

        fsUniforms.insertUniform( "imgSlopeIntercept", UniformType::Vec2Vector, Vec2Vector{ sk_zeroVec2, sk_zeroVec2 } );

        fsUniforms.insertUniform( "imgThresholds", UniformType::Vec2Vector, Vec2Vector{ sk_zeroVec2, sk_zeroVec2 } );
        fsUniforms.insertUniform( "imgOpacity", UniformType::FloatVector, FloatVector{ 0.0f, 0.0f } );
        fsUniforms.insertUniform( "segOpacity", UniformType::FloatVector, FloatVector{ 0.0f, 0.0f } );

        fsUniforms.insertUniform( "magentaCyan", UniformType::Bool, true );

        auto fs = std::make_shared<GLShader>( "fsOverlay", ShaderType::Fragment, fsSource.c_str() );
        fs->setRegisteredUniforms( std::move( fsUniforms ) );
        program.attachShader( fs );

        spdlog::debug( "Compiled fragment shader {}", fsFileName );
    }

    if ( ! program.link() )
    {
        spdlog::critical( "Failed to link shader program {}", program.name() );
        return false;
    }

    spdlog::debug( "Linked shader program {}", program.name() );
    return true;
}

bool Rendering::createDifferenceProgram( GLShaderProgram& program )
{
    static const std::string vsFileName{ "src/rendering/shaders/Difference.vs" };
    static const std::string fsFileName{ "src/rendering/shaders/Difference.fs" };

    auto filesystem = cmrc::shaders::get_filesystem();
    std::string vsSource;
    std::string fsSource;

    try
    {
        cmrc::file vsData = filesystem.open( vsFileName.c_str() );
        cmrc::file fsData = filesystem.open( fsFileName.c_str() );

        vsSource = std::string( vsData.begin(), vsData.end() );
        fsSource = std::string( fsData.begin(), fsData.end() );
    }
    catch ( const std::exception& e )
    {
        spdlog::critical( "Exception when loading shader file: {}", e.what() );
        throw_debug( "Unable to load shader" )
    }

    {
        Uniforms vsUniforms;
        vsUniforms.insertUniform( "view_T_clip", UniformType::Mat4, sk_identMat4 );
        vsUniforms.insertUniform( "world_T_clip", UniformType::Mat4, sk_identMat4 );
        vsUniforms.insertUniform( "clipDepth", UniformType::Float, 0.0f );

        vsUniforms.insertUniform( "imgTexture_T_world", UniformType::Mat4Vector, Mat4Vector{ sk_identMat4, sk_identMat4 } );
        vsUniforms.insertUniform( "segTexture_T_world", UniformType::Mat4Vector, Mat4Vector{ sk_identMat4, sk_identMat4 } );

        auto vs = std::make_shared<GLShader>( "vsDiff", ShaderType::Vertex, vsSource.c_str() );
        vs->setRegisteredUniforms( std::move( vsUniforms ) );
        program.attachShader( vs );

        spdlog::debug( "Compiled vertex shader {}", vsFileName );
    }

    {
        Uniforms fsUniforms;

        fsUniforms.insertUniform( "imgTex", UniformType::SamplerVector, msk_imgTexSamplers );
        fsUniforms.insertUniform( "segTex", UniformType::SamplerVector, msk_segTexSamplers );
        fsUniforms.insertUniform( "metricCmapTex", UniformType::Sampler, msk_metricCmapTexSampler );
        fsUniforms.insertUniform( "segLabelCmapTex", UniformType::SamplerVector, msk_labelTableTexSamplers );

        fsUniforms.insertUniform( "imgSlopeIntercept", UniformType::Vec2Vector, Vec2Vector{ sk_zeroVec2, sk_zeroVec2 } );
        fsUniforms.insertUniform( "segOpacity", UniformType::FloatVector, FloatVector{ 0.0f, 0.0f } );

        fsUniforms.insertUniform( "metricCmapSlopeIntercept", UniformType::Vec2, sk_zeroVec2 );
        fsUniforms.insertUniform( "metricSlopeIntercept", UniformType::Vec2, sk_zeroVec2 );
        fsUniforms.insertUniform( "metricMasking", UniformType::Bool, false );

        fsUniforms.insertUniform( "useSquare", UniformType::Bool, true );

        auto fs = std::make_shared<GLShader>( "fsDiff", ShaderType::Fragment, fsSource.c_str() );
        fs->setRegisteredUniforms( std::move( fsUniforms ) );
        program.attachShader( fs );

        spdlog::debug( "Compiled fragment shader {}", fsFileName );
    }

    if ( ! program.link() )
    {
        spdlog::critical( "Failed to link shader program {}", program.name() );
        return false;
    }

    spdlog::debug( "Linked shader program {}", program.name() );
    return true;
}

bool Rendering::createCrossCorrelationProgram( GLShaderProgram& program )
{
    static const std::string vsFileName{ "src/rendering/shaders/Correlation.vs" };
    static const std::string fsFileName{ "src/rendering/shaders/Correlation.fs" };

    auto filesystem = cmrc::shaders::get_filesystem();
    std::string vsSource;
    std::string fsSource;

    try
    {
        cmrc::file vsData = filesystem.open( vsFileName.c_str() );
        cmrc::file fsData = filesystem.open( fsFileName.c_str() );

        vsSource = std::string( vsData.begin(), vsData.end() );
        fsSource = std::string( fsData.begin(), fsData.end() );
    }
    catch ( const std::exception& e )
    {
        spdlog::critical( "Exception when loading shader file: {}", e.what() );
        throw_debug( "Unable to load shader" )
    }

    {
        Uniforms vsUniforms;
        vsUniforms.insertUniform( "view_T_clip", UniformType::Mat4, sk_identMat4 );
        vsUniforms.insertUniform( "world_T_clip", UniformType::Mat4, sk_identMat4 );
        vsUniforms.insertUniform( "clipDepth", UniformType::Float, 0.0f );

        vsUniforms.insertUniform( "imgTexture_T_world", UniformType::Mat4Vector, Mat4Vector{ sk_identMat4, sk_identMat4 } );
        vsUniforms.insertUniform( "segTexture_T_world", UniformType::Mat4Vector, Mat4Vector{ sk_identMat4, sk_identMat4 } );

        auto vs = std::make_shared<GLShader>( "vsCorr", ShaderType::Vertex, vsSource.c_str() );
        vs->setRegisteredUniforms( std::move( vsUniforms ) );
        program.attachShader( vs );

        spdlog::debug( "Compiled vertex shader {}", vsFileName );
    }

    {
        Uniforms fsUniforms;

        fsUniforms.insertUniform( "imgTex", UniformType::SamplerVector, msk_imgTexSamplers );
        fsUniforms.insertUniform( "segTex", UniformType::SamplerVector, msk_segTexSamplers );
        fsUniforms.insertUniform( "metricCmapTex", UniformType::Sampler, msk_metricCmapTexSampler );
        fsUniforms.insertUniform( "segLabelCmapTex", UniformType::SamplerVector, msk_labelTableTexSamplers );

        fsUniforms.insertUniform( "segOpacity", UniformType::FloatVector, FloatVector{ 0.0f, 0.0f } );

        fsUniforms.insertUniform( "metricCmapSlopeIntercept", UniformType::Vec2, sk_zeroVec2 );
        fsUniforms.insertUniform( "metricSlopeIntercept", UniformType::Vec2, sk_zeroVec2 );
        fsUniforms.insertUniform( "metricMasking", UniformType::Bool, false );

        fsUniforms.insertUniform( "texture1_T_texture0", UniformType::Mat4, sk_identMat4 );
        fsUniforms.insertUniform( "tex0SamplingDirX", UniformType::Vec3, sk_zeroVec3 );
        fsUniforms.insertUniform( "tex0SamplingDirY", UniformType::Vec3, sk_zeroVec3 );

        auto fs = std::make_shared<GLShader>( "fsCorr", ShaderType::Fragment, fsSource.c_str() );
        fs->setRegisteredUniforms( std::move( fsUniforms ) );
        program.attachShader( fs );

        spdlog::debug( "Compiled fragment shader {}", fsFileName );
    }

    if ( ! program.link() )
    {
        spdlog::critical( "Failed to link shader program {}", program.name() );
        return false;
    }

    spdlog::debug( "Linked shader program {}", program.name() );
    return true;
}

bool Rendering::createSimpleProgram( GLShaderProgram& program )
{
    auto filesystem = cmrc::shaders::get_filesystem();
    std::string vsSource;
    std::string fsSource;

    try
    {
        cmrc::file vsData = filesystem.open( "src/rendering/shaders/Simple.vs" );
        cmrc::file fsData = filesystem.open( "src/rendering/shaders/Simple.fs" );

        vsSource = std::string( vsData.begin(), vsData.end() );
        fsSource = std::string( fsData.begin(), fsData.end() );
    }
    catch ( const std::exception& e )
    {
        spdlog::critical( "Exception when loading shader file: {}", e.what() );
        throw_debug( "Unable to load shader" )
    }

    {
        Uniforms vsUniforms;
        vsUniforms.insertUniform( "view_T_clip", UniformType::Mat4, sk_identMat4 );
        vsUniforms.insertUniform( "clipDepth", UniformType::Float, 0.0f );
        vsUniforms.insertUniform( "clipMin", UniformType::Float, 0.0f );
        vsUniforms.insertUniform( "clipMax", UniformType::Float, 0.0f );

        auto vs = std::make_shared<GLShader>( "vsSimple", ShaderType::Vertex, vsSource.c_str() );
        vs->setRegisteredUniforms( std::move( vsUniforms ) );
        program.attachShader( vs );
        spdlog::debug( "Compiled simple vertex shader" );
    }

    {
        Uniforms fsUniforms;
        fsUniforms.insertUniform( "color", UniformType::Vec4, glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f } );

        auto fs = std::make_shared<GLShader>( "fsSimple", ShaderType::Fragment, fsSource.c_str() );
        fs->setRegisteredUniforms( std::move( fsUniforms ) );
        program.attachShader( fs );
        spdlog::debug( "Compiled simple fragment shader" );
    }

    if ( ! program.link() )
    {
        spdlog::critical( "Failed to link shader program {}", program.name() );
        return false;
    }

    spdlog::debug( "Linked shader program {}", program.name() );
    return true;
}


bool Rendering::showVectorOverlays() const { return m_showOverlays; }
void Rendering::setShowVectorOverlays( bool show ) { m_showOverlays = show; }
