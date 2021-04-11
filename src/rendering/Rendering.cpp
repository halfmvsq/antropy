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


/// @todo Put free functions into separate compilation units
namespace
{

static const glm::mat4 sk_identMat4{ 1.0f };
static const glm::vec2 sk_zeroVec2{ 0.0f, 0.0f };
static const glm::vec3 sk_zeroVec3{ 0.0f, 0.0f, 0.0f };
static const glm::vec4 sk_zeroVec4{ 0.0f, 0.0f, 0.0f, 0.0f };
static const glm::bvec2 sk_zeroBVec2{ false, false };

static const std::string ROBOTO_LIGHT( "robotoLight" );


void startNvgFrame( NVGcontext* nvg, const Viewport& windowVP )
{
    if ( ! nvg ) return;

    nvgShapeAntiAlias( nvg, true );

    // Sets the composite operation. NVG_SOURCE_OVER is the default.
    nvgGlobalCompositeOperation( nvg, NVG_SOURCE_OVER );

    // Sets the composite operation with custom pixel arithmetic.
    // The defaults are sfactor = NVG_ONE and dfactor = NVG_ONE_MINUS_SRC_ALPHA
    nvgGlobalCompositeBlendFunc( nvg, NVG_SRC_ALPHA, NVG_ONE_MINUS_SRC_ALPHA );

    nvgBeginFrame( nvg, windowVP.width(), windowVP.height(), windowVP.devicePixelRatio().x );
    nvgSave( nvg );
}

void endNvgFrame( NVGcontext* nvg )
{
    if ( ! nvg ) return;

    nvgRestore( nvg );
    nvgEndFrame( nvg );
}


std::unordered_map< uuids::uuid, std::vector<GLTexture> >
createImageTextures( const AppData& appData )
{
    static constexpr GLint sk_mipmapLevel = 0; // Load image data into first mipmap level
    static constexpr GLint sk_alignment = 1; // Pixel pack/unpack alignment is 1 byte
    static const tex::WrapMode sk_wrapModeClampToEdge = tex::WrapMode::ClampToEdge;
//    static const tex::WrapMode sk_wrapModeClampToBorder = tex::WrapMode::ClampToBorder;
    static const glm::vec4 sk_border{ 0.0f, 0.0f, 0.0f, 0.0f }; // Black border

    // Map from image UID to vector of textures for the image components.
    // Images with interleaved components will have one component texture.
    std::unordered_map< uuids::uuid, std::vector<GLTexture> > imageTextures;

    if ( 0 == appData.numImages() )
    {
        spdlog::warn( "No images are loaded for which to create textures" );
        return imageTextures;
    }

    spdlog::debug( "Begin creating 3D image textures" );

    GLTexture::PixelStoreSettings pixelPackSettings;
    pixelPackSettings.m_alignment = sk_alignment;
    GLTexture::PixelStoreSettings pixelUnpackSettings = pixelPackSettings;

    for ( const auto& imageUid : appData.imageUidsOrdered() )
    {
        spdlog::debug( "Begin creating texture(s) for components of image {}", imageUid );

        const auto* image = appData.image( imageUid );
        if ( ! image )
        {
            spdlog::warn( "Image {} is invalid", imageUid );
            continue;
        }

        const ComponentType compType = image->header().memoryComponentType();
        const uint32_t numComp = image->header().numComponentsPerPixel();

        std::vector<GLTexture> componentTextures;

        switch ( image->bufferType() )
        {
        case Image::MultiComponentBufferType::InterleavedImage:
        {
            spdlog::debug( "Image {} has {} interleaved components, so one texture will be created.",
                           imageUid, numComp );

            // For images with interleaved components, all components are at index 0
            constexpr uint32_t k_comp0 = 0;

            if ( 4 < numComp )
            {
                spdlog::warn( "Image {} has {} interleaved components, exceeding the maximum "
                              "of 4 allowed per texture; it will not be loaded as a texture",
                              imageUid, numComp );
                continue;
            }

            tex::MinificationFilter minFilter;
            tex::MagnificationFilter maxFilter;

            switch ( image->settings().interpolationMode( k_comp0 ) )
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

            // The texture pixel format types depend on the number of components
            tex::SizedInternalFormat sizedInternalNormalizedFormat;
            tex::BufferPixelFormat bufferPixelNormalizedFormat;

            switch ( numComp )
            {
            case 1:
            {
                // Red:
                sizedInternalNormalizedFormat = GLTexture::getSizedInternalNormalizedRedFormat( compType );
                bufferPixelNormalizedFormat = GLTexture::getBufferPixelNormalizedRedFormat( compType );
                break;
            }
            case 2:
            {
                // Red, green:
                sizedInternalNormalizedFormat = GLTexture::getSizedInternalNormalizedRGFormat( compType );
                bufferPixelNormalizedFormat = GLTexture::getBufferPixelNormalizedRGFormat( compType );
                break;
            }
            case 3:
            {
                // Red, green, blue:
                sizedInternalNormalizedFormat = GLTexture::getSizedInternalNormalizedRGBFormat( compType );
                bufferPixelNormalizedFormat = GLTexture::getBufferPixelNormalizedRGBFormat( compType );
                break;
            }
            case 4:
            {
                // Red, green, blue, alpha:
                sizedInternalNormalizedFormat = GLTexture::getSizedInternalNormalizedRGBAFormat( compType );
                bufferPixelNormalizedFormat = GLTexture::getBufferPixelNormalizedRGBAFormat( compType );
                break;
            }
            default:
            {
                spdlog::warn( "Image {} has invalid number of components ({}); "
                              "it will not be loaded as a texture", imageUid, numComp );
                continue;
            }
            }

            GLTexture& T = componentTextures.emplace_back(
                        tex::Target::Texture3D,
                        GLTexture::MultisampleSettings(),
                        pixelPackSettings, pixelUnpackSettings );

            T.generate();
            T.setMinificationFilter( minFilter );
            T.setMagnificationFilter( maxFilter );
            T.setBorderColor( sk_border );
            T.setWrapMode( sk_wrapModeClampToEdge );
            T.setAutoGenerateMipmaps( true );
            T.setSize( image->header().pixelDimensions() );

            T.setData( sk_mipmapLevel,
                       sizedInternalNormalizedFormat,
                       bufferPixelNormalizedFormat,
                       GLTexture::getBufferPixelDataType( compType ),
                       image->bufferAsVoid( k_comp0 ) );

            spdlog::debug( "Done creating the texture for all interleaved components of image {}", imageUid );
            break;
        }
        case Image::MultiComponentBufferType::SeparateImages:
        {
            spdlog::debug( "Image {} has {} separate components, so {} textures will be created.",
                           imageUid, numComp, numComp );

            for ( uint32_t comp = 0; comp < numComp; ++comp )
            {
                tex::MinificationFilter minFilter;
                tex::MagnificationFilter maxFilter;

                switch ( image->settings().interpolationMode( comp ) )
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

                // Use Red format for each component texture:
                const tex::SizedInternalFormat sizedInternalNormalizedFormat =
                        GLTexture::getSizedInternalNormalizedRedFormat( compType );

                const tex::BufferPixelFormat bufferPixelNormalizedFormat =
                        GLTexture::getBufferPixelNormalizedRedFormat( compType );

                GLTexture& T = componentTextures.emplace_back(
                            tex::Target::Texture3D,
                            GLTexture::MultisampleSettings(),
                            pixelPackSettings, pixelUnpackSettings );

                T.generate();
                T.setMinificationFilter( minFilter );
                T.setMagnificationFilter( maxFilter );
                T.setBorderColor( sk_border );
                T.setWrapMode( sk_wrapModeClampToEdge );
                T.setAutoGenerateMipmaps( true );
                T.setSize( image->header().pixelDimensions() );

                T.setData( sk_mipmapLevel,
                           sizedInternalNormalizedFormat,
                           bufferPixelNormalizedFormat,
                           GLTexture::getBufferPixelDataType( compType ),
                           image->bufferAsVoid( comp ) );
            }

            spdlog::debug( "Done creating {} image component textures", componentTextures.size() );
            break;
        }
        } // end switch ( image->bufferType() )

        imageTextures.emplace( imageUid, std::move( componentTextures ) );

        spdlog::debug( "Done creating texture(s) for image {} ('{}')",
                       imageUid, image->settings().displayName() );
    } // end for ( const auto& imageUid : appData.imageUidsOrdered() )

    spdlog::debug( "Done creating textures for {} image(s)", imageTextures.size() );
    return imageTextures;
}


std::unordered_map< uuids::uuid, GLTexture >
createSegTextures( const AppData& appData )
{
    // Load the first pixel component of the segmentation image.
    // (Segmentations should have only one component.)
    constexpr uint32_t k_comp0 = 0;

    constexpr GLint k_mipmapLevel = 0; // Load seg data into first mipmap level
    constexpr GLint k_alignment = 1; // Pixel pack/unpack alignment is 1 byte

    static const tex::WrapMode sk_wrapMode = tex::WrapMode::ClampToBorder;
    static const glm::vec4 sk_border{ 0.0f, 0.0f, 0.0f, 0.0f }; // Black border

    // Nearest-neighbor interpolation is used for segmentation textures:
    static const tex::MinificationFilter sk_minFilter = tex::MinificationFilter::Nearest;
    static const tex::MagnificationFilter sk_maxFilter = tex::MagnificationFilter::Nearest;

    std::unordered_map< uuids::uuid, GLTexture > textures;

    if ( 0 == appData.numSegs() )
    {
        spdlog::info( "No image segmentations loaded for which to create textures" );
        return textures;
    }

    spdlog::debug( "Begin creating 3D segmentation textures" );

    GLTexture::PixelStoreSettings pixelPackSettings;
    pixelPackSettings.m_alignment = k_alignment;
    GLTexture::PixelStoreSettings pixelUnpackSettings = pixelPackSettings;

    // Loop through images in order of index
    for ( const auto& segUid : appData.segUidsOrdered() )
    {
        const auto* seg = appData.seg( segUid );
        if ( ! seg )
        {
            spdlog::warn( "Segmentation {} is invalid", segUid );
            continue;
        }

        const ComponentType compType = seg->header().memoryComponentType();

        auto it = textures.try_emplace(
                    segUid,
                    tex::Target::Texture3D,
                    GLTexture::MultisampleSettings(),
                    pixelPackSettings, pixelUnpackSettings );

        if ( ! it.second ) continue;
        GLTexture& T = it.first->second;

        T.generate();
        T.setMinificationFilter( sk_minFilter );
        T.setMagnificationFilter( sk_maxFilter );
        T.setBorderColor( sk_border );
        T.setWrapMode( sk_wrapMode );
        T.setAutoGenerateMipmaps( true );
        T.setSize( seg->header().pixelDimensions() );

        T.setData( k_mipmapLevel,
                   GLTexture::getSizedInternalRedFormat( compType ),
                   GLTexture::getBufferPixelRedFormat( compType ),
                   GLTexture::getBufferPixelDataType( compType ),
                   seg->bufferAsVoid( k_comp0 ) );

        spdlog::debug( "Created texture for segmentation {} ('{}')",
                       segUid, seg->settings().displayName() );
    }

    spdlog::debug( "Done creating {} segmentation textures", textures.size() );
    return textures;
}


std::unordered_map< uuids::uuid, GLTexture >
createImageColorMapTextures( const AppData& appData )
{
    std::unordered_map< uuids::uuid, GLTexture > textures;

    if ( 0 == appData.numImageColorMaps() )
    {
        spdlog::warn( "No image color maps loaded for which to create textures" );
        return textures;
    }

    spdlog::debug( "Begin creating image color map textures" );

    // Loop through color maps in order of index
    for ( size_t i = 0; i < appData.numImageColorMaps(); ++i )
    {
        const auto cmapUid = appData.imageColorMapUid( i );
        if ( ! cmapUid )
        {
            spdlog::warn( "Image color map index {} is invalid", i );
            continue;
        }

        const auto* map = appData.imageColorMap( *cmapUid );
        if ( ! map )
        {
            spdlog::warn( "Image color map {} is invalid", *cmapUid );
            continue;
        }

        auto it = textures.emplace( *cmapUid, tex::Target::Texture1D );
        if ( ! it.second ) continue;

        GLTexture& T = it.first->second;

        T.generate();
        T.setSize( glm::uvec3{ map->numColors(), 1, 1 } );

        T.setData( 0, ImageColorMap::textureFormat_RGBA_F32(),
                   tex::BufferPixelFormat::RGBA,
                   tex::BufferPixelDataType::Float32,
                   map->data_RGBA_F32() );

        // We should never sample outside the texture coordinate range [0.0, 1.0], anyway
        T.setWrapMode( tex::WrapMode::ClampToEdge );

        // All sampling of color maps uses linearly interpolation
        T.setAutoGenerateMipmaps( false );
        T.setMinificationFilter( tex::MinificationFilter::Linear );
        T.setMagnificationFilter( tex::MagnificationFilter::Linear );

        spdlog::trace( "Generated texture for image color map {}", *cmapUid );
    }

    spdlog::debug( "Done creating {} image color map textures", textures.size() );
    return textures;
}


std::unordered_map< uuids::uuid, GLTexture >
createLabelColorTableTextures( const AppData& appData )
{
    static const glm::vec4 sk_border{ 0.0f, 0.0f, 0.0f, 0.0f };

    std::unordered_map< uuids::uuid, GLTexture > textures;

    if ( 0 == appData.numLabelTables() )
    {
        spdlog::warn( "No parcellation label color tables loaded for which to create textures" );
        return textures;
    }

    spdlog::debug( "Begin creating 1D label color map textures" );

    // Loop through label tables in order of index
    for ( size_t i = 0; i < appData.numLabelTables(); ++i )
    {
        const auto tableUid = appData.labelTableUid( i );
        if ( ! tableUid )
        {
            spdlog::warn( "Label table index {} is invalid", i );
            continue;
        }

        const auto* table = appData.labelTable( *tableUid );
        if ( ! table )
        {
            spdlog::warn( "Label table {} is invalid", *tableUid );
            continue;
        }

        auto it = textures.emplace( *tableUid, tex::Target::Texture1D );
        if ( ! it.second ) continue;

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

        spdlog::debug( "Generated texture for label color table {}", *tableUid );
    }

    spdlog::debug( "Done creating {} label color map textures", textures.size() );
    return textures;
}


void renderImageQuad(
        GLShaderProgram& program,
        const camera::ViewRenderMode& shaderType,
        RenderData::Quad& quad,
        const View& view,
        const glm::vec3& worldOrigin,
        float flashlightRadius,
        bool flashlightOverlays,
        const std::vector< std::pair< std::optional<uuids::uuid>, std::optional<uuids::uuid> > >& I,
        const std::function< const Image* ( const std::optional<uuids::uuid>& imageUid ) > getImage,
        bool showEdges )
{
    if ( I.empty() )
    {
        spdlog::error( "No images provided when rendering plane" );
        return;
    }

    // Set the view transformation uniforms that are common to all programs:
    program.setUniform( "view_T_clip", view.windowClip_T_viewClip() );
    program.setUniform( "world_T_clip", camera::world_T_clip( view.camera() ) );
    program.setUniform( "clipDepth", view.clipPlaneDepth() );

    if ( camera::ViewRenderMode::Image == shaderType ||
         camera::ViewRenderMode::Checkerboard == shaderType ||
         camera::ViewRenderMode::Quadrants == shaderType ||
         camera::ViewRenderMode::Flashlight == shaderType )
    {
        program.setUniform( "aspectRatio", view.camera().aspectRatio() );
        program.setUniform( "flashlightRadius", flashlightRadius );
        program.setUniform( "flashlightOverlays", flashlightOverlays );

        const glm::vec4 clipCrosshairs = camera::clip_T_world( view.camera() ) * glm::vec4{ worldOrigin, 1.0f };
        program.setUniform( "clipCrosshairs", glm::vec2{ clipCrosshairs / clipCrosshairs.w } );

        if ( showEdges )
        {
            const Image* image = getImage( I[0].first );

            if ( ! image )
            {
                spdlog::error( "Null image when rendering plane with edges" );
                return;
            }

            const glm::mat4 pixel_T_clip =
                    image->transformations().pixel_T_worldDef() *
                    camera::world_T_clip( view.camera() );

            glm::vec4 pO = pixel_T_clip * glm::vec4{ 0.0f, 0.0f, -1.0f, 1.0 }; pO /= pO.w;
            glm::vec4 pX = pixel_T_clip * glm::vec4{ 1.0f, 0.0f, -1.0f, 1.0 }; pX /= pX.w;
            glm::vec4 pY = pixel_T_clip * glm::vec4{ 0.0f, 1.0f, -1.0f, 1.0 }; pY /= pY.w;

            const glm::vec3 pixelDirX = glm::normalize( pX - pO );
            const glm::vec3 pixelDirY = glm::normalize( pY - pO );

            const glm::vec3 invDims = image->transformations().invPixelDimensions();
            const glm::vec3 texSamplingDirX = glm::dot( glm::abs( pixelDirX ), invDims ) * pixelDirX;
            const glm::vec3 texSamplingDirY = glm::dot( glm::abs( pixelDirY ), invDims ) * pixelDirY;

            program.setUniform( "texSampleSize", invDims );
            program.setUniform( "texSamplingDirX", texSamplingDirX );
            program.setUniform( "texSamplingDirY", texSamplingDirY );
        }
    }
    else if ( camera::ViewRenderMode::CrossCorrelation == shaderType )
    {
        if ( 2 != I.size() )
        {
            spdlog::error( "Not enough images provided when rendering plane with cross-correlation metric" );
            return;
        }

        const Image* img0 = getImage( I[0].first );
        const Image* img1 = getImage( I[1].first );

        if ( ! img0 || ! img1)
        {
            spdlog::error( "Null image when rendering plane with edges" );
            return;
        }

        const glm::mat4 pixel_T_clip =
                img0->transformations().pixel_T_worldDef() *
                camera::world_T_clip( view.camera() );

        static const glm::vec4 sk_clipO{ 0.0f, 0.0f, -1.0f, 1.0 };
        static const glm::vec4 sk_clipX{ 1.0f, 0.0f, -1.0f, 1.0 };
        static const glm::vec4 sk_clipY{ 0.0f, 1.0f, -1.0f, 1.0 };

        glm::vec4 pO = pixel_T_clip * sk_clipO; pO /= pO.w;
        glm::vec4 pX = pixel_T_clip * sk_clipX; pX /= pX.w;
        glm::vec4 pY = pixel_T_clip * sk_clipY; pY /= pY.w;

        const glm::vec3 pixelDirX = glm::normalize( pX - pO );
        const glm::vec3 pixelDirY = glm::normalize( pY - pO );

        const glm::vec3 img0_invDims = img0->transformations().invPixelDimensions();
        const glm::vec3 img1_invDims = img0->transformations().invPixelDimensions();

        const glm::vec3 tex0SamplingDirX = glm::dot( glm::abs( pixelDirX ), img0_invDims ) * pixelDirX;
        const glm::vec3 tex0SamplingDirY = glm::dot( glm::abs( pixelDirY ), img0_invDims ) * pixelDirY;

        program.setUniform( "texSampleSize", std::vector<glm::vec2>{ img0_invDims, img1_invDims } );
        program.setUniform( "tex0SamplingDirX", tex0SamplingDirX );
        program.setUniform( "tex0SamplingDirY", tex0SamplingDirY );
    }

    quad.m_vao.bind();
    {
        quad.m_vao.drawElements( quad.m_vaoParams );
    }
    quad.m_vao.release();
}


static const NVGcolor s_black( nvgRGBA( 0, 0, 0, 255 ) );
static const NVGcolor s_grey25( nvgRGBA( 63, 63, 63, 255 ) );
static const NVGcolor s_grey40( nvgRGBA( 102, 102, 102, 255 ) );
static const NVGcolor s_grey50( nvgRGBA( 127, 127, 127, 255 ) );
static const NVGcolor s_grey60( nvgRGBA( 153, 153, 153, 255 ) );
static const NVGcolor s_grey75( nvgRGBA( 195, 195, 195, 255 ) );
static const NVGcolor s_yellow( nvgRGBA( 255, 255, 0, 255 ) );
static const NVGcolor s_red( nvgRGBA( 255, 0, 0, 255 ) );


void renderWindowOutline( NVGcontext* nvg, const Viewport& windowVP )
{
    constexpr float k_pad = 1.0f;

    // Outline around window
    nvgStrokeWidth( nvg, 4.0f );
    nvgStrokeColor( nvg, s_grey50 );

    nvgBeginPath( nvg );
    nvgRect( nvg, k_pad, k_pad, windowVP.width() - 2.0f * k_pad, windowVP.height() - 2.0f * k_pad );
    nvgStroke( nvg );

    //        nvgStrokeWidth( nvg, 2.0f );
    //        nvgStrokeColor( nvg, s_grey50 );
    //        nvgRect( nvg, pad, pad, windowVP.width() - pad, windowVP.height() - pad );
    //        nvgStroke( nvg );

    //        nvgStrokeWidth( nvg, 1.0f );
    //        nvgStrokeColor( nvg, s_grey60 );
    //        nvgRect( nvg, pad, pad, windowVP.width() - pad, windowVP.height() - pad );
    //        nvgStroke( nvg );
}

/// @see https://community.vcvrack.com/t/advanced-nanovg-custom-label/6769/21
//void draw (const DrawArgs &args) override {
//  // draw the text
//  float bounds[4];
//  const char* txt = "DEMO";
//  nvgTextAlign(args.vg, NVG_ALIGN_MIDDLE);
//      nvgTextBounds(args.vg, 0, 0, txt, NULL, bounds);
//      nvgBeginPath(args.vg);
//  nvgFillColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0xff));
//      nvgRect(args.vg, bounds[0], bounds[1], bounds[2]-bounds[0], bounds[3]-bounds[1]);
//      nvgFill(args.vg);
//  nvgFillColor(args.vg, nvgRGBA(0x00, 0xff, 0x00, 0xff));
//  nvgText(args.vg, 0, 0, txt, NULL);
//}


/**
 * @brief Information needed for positioning a single anatomical label and the crosshair
 * that corresponds to this label.
 */
struct AnatomicalLabelPosInfo
{
    AnatomicalLabelPosInfo( int l ) : labelIndex( l ) {}

    /// The anatomical label index (0: L, 1: P, 2: S)
    int labelIndex;

    /// Mouse crosshairs center position (in Mouse space)
    glm::vec2 miewportXhairCenterPos{ 0.0f, 0.0f };

    /// Normalized direction vector of the label (in View Clip space)
    glm::vec2 viewClipDir{ 0.0f, 0.0f };

    /// Position of the label and the opposite label of its pair (in Mouse space)
    std::array<glm::vec2, 2> miewportLabelPositions;

    /// Positions of the crosshair-view intersections (in Mouse space).
    /// Equal to std::nullopt if there is no intersection of the crosshair with the
    /// view AABB for this label.
    std::optional< std::array<glm::vec2, 2> > miewportXhairPositions = std::nullopt;
};


std::list<AnatomicalLabelPosInfo>
computeAnatomicalLabelsForView(
        const View& view,
        const glm::mat4& world_T_refSubject )
{
    // Shortcuts for the three orthogonal anatomical directions
    static constexpr int L = 0;
    static constexpr int P = 1;
    static constexpr int S = 2;

    // Visibility and directions of the labels L, P, S in View Clip/NDC space:
    std::list<AnatomicalLabelPosInfo> labels;

    // The reference subject's left, posterior, and superior directions in Camera space.
    // Columns 0, 1, and 2 of the matrix correspond to left, posterior, and superior, respectively.
    const glm::mat3 axes = math::computeSubjectAxesInCamera(
                glm::mat3{ view.camera().camera_T_world() },
                glm::mat3{ world_T_refSubject } );

    const glm::mat3 axesAbs{ glm::abs( axes[0] ), glm::abs( axes[1] ), glm::abs( axes[2] ) };
    const glm::mat3 axesSgn{ glm::sign( axes[0] ), glm::sign( axes[1] ), glm::sign( axes[2] ) };

    // Render the two sets of labels that are closest to the view plane:
    if ( axesAbs[L].z > axesAbs[P].z && axesAbs[L].z > axesAbs[S].z )
    {
        labels.emplace_back( AnatomicalLabelPosInfo{ P } );
        labels.emplace_back( AnatomicalLabelPosInfo{ S } );
    }
    else if ( axesAbs[P].z > axesAbs[L].z && axesAbs[P].z > axesAbs[S].z )
    {
        labels.emplace_back( AnatomicalLabelPosInfo{ L } );
        labels.emplace_back( AnatomicalLabelPosInfo{ S } );
    }
    else if ( axesAbs[S].z > axesAbs[L].z && axesAbs[S].z > axesAbs[P].z )
    {
        labels.emplace_back( AnatomicalLabelPosInfo{ L } );
        labels.emplace_back( AnatomicalLabelPosInfo{ P } );
    }

    // Render the translation vectors for the L (0), P (1), and S (2) labels:
    for ( auto& label : labels )
    {
        const int i = label.labelIndex;

        label.viewClipDir = ( axesAbs[i].x > 0.0f && axesAbs[i].y / axesAbs[i].x <= 1.0f )
                ? glm::vec2{ axesSgn[i].x, axesSgn[i].y * axesAbs[i].y / axesAbs[i].x }
                : glm::vec2{ axesSgn[i].x * axesAbs[i].x / axesAbs[i].y, axesSgn[i].y };
    }

    return labels;
}



std::list<AnatomicalLabelPosInfo>
computeAnatomicalLabelPosInfo(
        const camera::FrameBounds& miewportViewBounds,
        const Viewport& windowVP,
        const View& view,
        const glm::mat4& world_T_refSubject,
        const glm::vec3& worldCrosshairs )
{
    // Compute intersections of the anatomical label ray with the view box:
    static constexpr bool sk_doBothLabelDirs = false;

    // Compute intersections of the crosshair ray with the view box:
    static constexpr bool sk_doBothXhairDirs = true;

    const glm::mat4 miewport_T_viewClip =
            camera::miewport_T_viewport( windowVP.height() ) *
            camera::viewport_T_windowClip( windowVP ) *
            view.windowClip_T_viewClip();

    const glm::mat3 miewport_T_viewClip_IT = glm::inverseTranspose( glm::mat3{ miewport_T_viewClip } );

    auto labelPosInfo = computeAnatomicalLabelsForView( view, world_T_refSubject );

    const float aspectRatio = miewportViewBounds.bounds.width / miewportViewBounds.bounds.height;

    const glm::vec2 aspectRatioScale = ( aspectRatio < 1.0f )
            ? glm::vec2{ aspectRatio, 1.0f }
            : glm::vec2{ 1.0f, 1.0f / aspectRatio };

    const glm::vec2 miewportMinCorner( miewportViewBounds.bounds.xoffset, miewportViewBounds.bounds.yoffset );
    const glm::vec2 miewportSize( miewportViewBounds.bounds.width, miewportViewBounds.bounds.height );
    const glm::vec2 miewportCenter = miewportMinCorner + 0.5f * miewportSize;

    glm::vec4 viewClipXhairPos = camera::clip_T_world( view.camera() ) * glm::vec4{ worldCrosshairs, 1.0f };
    viewClipXhairPos /= viewClipXhairPos.w;

    glm::vec4 miewportXhairPos = miewport_T_viewClip * viewClipXhairPos;
    miewportXhairPos /= miewportXhairPos.w;

    for ( auto& label : labelPosInfo )
    {
        const glm::vec3 viewClipXhairDir{ label.viewClipDir.x, label.viewClipDir.y, 0.0f };

        label.miewportXhairCenterPos = glm::vec2{ miewportXhairPos };

        glm::vec2 miewportXhairDir{ miewport_T_viewClip_IT * viewClipXhairDir };
        miewportXhairDir.x *= aspectRatioScale.x;
        miewportXhairDir.y *= aspectRatioScale.y;
        miewportXhairDir = glm::normalize( miewportXhairDir );

        // Intersections for the positive label (L, P, or S):
        const auto posLabelHits = math::computeRayAABoxIntersections(
                    miewportCenter,
                    miewportXhairDir,
                    miewportMinCorner,
                    miewportSize,
                    sk_doBothLabelDirs );

        // Intersections for the negative label (R, A, or I):
        const auto negLabelHits = math::computeRayAABoxIntersections(
                    miewportCenter,
                    -miewportXhairDir,
                    miewportMinCorner,
                    miewportSize,
                    sk_doBothLabelDirs );

        if ( 1 != posLabelHits.size() || 1 != negLabelHits.size() )
        {
            spdlog::warn( "Expected two intersections when computing anatomical label positions for view. "
                          "Got {} and {} intersections in the positive and negative directions, respectively.",
                          posLabelHits.size(), negLabelHits.size() );
            continue;
        }

        label.miewportLabelPositions = std::array<glm::vec2, 2>{ posLabelHits[0], negLabelHits[0] };

        const auto crosshairHits = math::computeRayAABoxIntersections(
                    label.miewportXhairCenterPos,
                    miewportXhairDir,
                    miewportMinCorner,
                    miewportSize,
                    sk_doBothXhairDirs );

        if ( 2 != crosshairHits.size() )
        {
            // Only render crosshairs when there are two intersections with the view box:
            label.miewportXhairPositions = std::nullopt;
        }
        else
        {
            label.miewportXhairPositions = std::array<glm::vec2, 2>{ crosshairHits[0], crosshairHits[1] };
        }
    }

    return labelPosInfo;
}


/**
 * @brief renderAnatomicalLabels
 * @param nvg
 * @param windowVP
 * @param view
 * @param world_T_refSubject
 * @param color Non-premultiplied by alpha
 */
void renderAnatomicalLabels(
        NVGcontext* nvg,
        const camera::FrameBounds& miewportViewBounds,
        const glm::vec4& color,
        const std::list<AnatomicalLabelPosInfo>& labelPosInfo )
{
    static constexpr float sk_fontMult = 0.03f;

    // Anatomical direction labels
    static const std::array< std::string, 6 > sk_labels{ "L", "P", "S", "R", "A", "I" };

    const glm::vec2 miewportMinCorner( miewportViewBounds.bounds.xoffset, miewportViewBounds.bounds.yoffset );
    const glm::vec2 miewportSize( miewportViewBounds.bounds.width, miewportViewBounds.bounds.height );
    const glm::vec2 miewportMaxCorner = miewportMinCorner + miewportSize;

    // Clip against the view bounds, even though not strictly necessary with how lines are defined
    nvgScissor( nvg, miewportViewBounds.viewport[0], miewportViewBounds.viewport[1],
            miewportViewBounds.viewport[2], miewportViewBounds.viewport[3] );

    const float fontSizePixels = sk_fontMult *
            std::min( miewportViewBounds.bounds.width, miewportViewBounds.bounds.height );

    // For inward shift of the labels:
    const glm::vec2 inwardFontShift{ 0.8f * fontSizePixels, 0.8f * fontSizePixels };

    // For downward shift of the labels:
    const glm::vec2 vertFontShift{ 0.0f, 0.35f * fontSizePixels };

    nvgFontSize( nvg, fontSizePixels );
    nvgFontFace( nvg, ROBOTO_LIGHT.c_str() );
    nvgTextAlign( nvg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE );

    // Render the translation vectors for the L (0), P (1), and S (2) labels:
    for ( const auto& label : labelPosInfo )
    {
        const glm::vec2 miewportPositiveMousePos = glm::clamp(
                    label.miewportLabelPositions[0],
                    miewportMinCorner + inwardFontShift, miewportMaxCorner - inwardFontShift ) + vertFontShift;

        const glm::vec2 miewportNegativeMousePos = glm::clamp(
                    label.miewportLabelPositions[1],
                    miewportMinCorner + inwardFontShift, miewportMaxCorner - inwardFontShift ) + vertFontShift;

        nvgFontBlur( nvg, 2.0f );
        nvgFillColor( nvg, s_black );
        nvgText( nvg, miewportPositiveMousePos.x, miewportPositiveMousePos.y, sk_labels[label.labelIndex].c_str(), nullptr );
        nvgText( nvg, miewportNegativeMousePos.x, miewportNegativeMousePos.y, sk_labels[label.labelIndex + 3].c_str(), nullptr );

        nvgFontBlur( nvg, 0.0f );
        nvgFillColor( nvg, nvgRGBAf( color.r, color.g, color.b, color.a ) );
        nvgText( nvg, miewportPositiveMousePos.x, miewportPositiveMousePos.y, sk_labels[label.labelIndex].c_str(), nullptr );
        nvgText( nvg, miewportNegativeMousePos.x, miewportNegativeMousePos.y, sk_labels[label.labelIndex + 3].c_str(), nullptr );
    }

    nvgResetScissor( nvg );
}


// Draw a circle
void drawCircle(
        NVGcontext* nvg,
        const glm::vec2& miewportPos,
        float radius,
        const glm::vec4& fillColor,
        const glm::vec4& strokeColor,
        float strokeWidth )
{
    nvgStrokeWidth( nvg, strokeWidth );
    nvgStrokeColor( nvg, nvgRGBAf( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a ) );
    nvgFillColor( nvg, nvgRGBAf( fillColor.r, fillColor.g, fillColor.b, fillColor.a ) );

    nvgBeginPath( nvg );
    nvgCircle( nvg, miewportPos.x, miewportPos.y, radius );

    nvgStroke( nvg );
    nvgFill( nvg );
}


// Draw text
void drawText(
        NVGcontext* nvg,
        const glm::vec2& miewportPos,
        const std::string& centeredString,
        const std::string& offsetString,
        const glm::vec4& textColor,
        float offset,
        float fontSizePixels )
{
    nvgFontFace( nvg, ROBOTO_LIGHT.c_str() );

    // Draw centered text
    if ( ! centeredString.empty() )
    {
        nvgFontSize( nvg, 1.0f * fontSizePixels );
        nvgTextAlign( nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE );

        nvgFontBlur( nvg, 3.0f );
        nvgFillColor( nvg, nvgRGBAf( 0.0f, 0.0f, 0.0f, textColor.a ) );
        nvgText( nvg, miewportPos.x, miewportPos.y, centeredString.c_str(), nullptr );

        nvgFontBlur( nvg, 0.0f );
        nvgFillColor( nvg, nvgRGBAf( textColor.r, textColor.g, textColor.b, textColor.a ) );
        nvgText( nvg, miewportPos.x, miewportPos.y, centeredString.c_str(), nullptr );
    }

    // Draw offset text
    if ( ! offsetString.empty() )
    {
        nvgFontSize( nvg, 1.15f * fontSizePixels );
        nvgTextAlign( nvg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP );

        nvgFontBlur( nvg, 3.0f );
        nvgFillColor( nvg, nvgRGBAf( 0.0f, 0.0f, 0.0f, textColor.a ) );
        nvgText( nvg, offset + miewportPos.x, offset + miewportPos.y, offsetString.c_str(), nullptr );

        nvgFontBlur( nvg, 0.0f );
        nvgFillColor( nvg, nvgRGBAf( textColor.r, textColor.g, textColor.b, textColor.a ) );
        nvgText( nvg, offset + miewportPos.x, offset + miewportPos.y, offsetString.c_str(), nullptr );
    }
}


void renderLandmarks(
        NVGcontext* nvg,
        const camera::FrameBounds& miewportViewBounds,
        const glm::vec3& worldCrosshairs,
        AppData& appData,
        const View& view,
        const std::vector< std::pair< std::optional<uuids::uuid>, std::optional<uuids::uuid> > >& I )
/// @todo use CurrentImages
{
    static constexpr float sk_minSize = 4.0f;
    static constexpr float sk_maxSize = 128.0f;

    const Viewport& windowVP = appData.windowData().viewport();

    // Convert a 3D position from World space to the view's Miewport space
    auto convertWorldToMiewportPos = [&view, &windowVP] ( const glm::vec3& worldPos ) -> glm::vec2
    {
        const glm::vec4 winClipPos =
                view.windowClip_T_viewClip() *
                camera::clip_T_world( view.camera() ) *
                glm::vec4{ worldPos, 1.0f };

        const glm::vec2 viewportPos = camera::viewport_T_windowClip( windowVP, glm::vec2{ winClipPos / winClipPos.w } );
        const glm::vec2 miewportPos = camera::miewport_T_viewport( windowVP.height(), viewportPos );
        return miewportPos;
    };


    startNvgFrame( nvg, windowVP ); /*** START FRAME ***/

    // Clip against the view bounds
    nvgScissor( nvg, miewportViewBounds.viewport[0], miewportViewBounds.viewport[1],
            miewportViewBounds.viewport[2], miewportViewBounds.viewport[3] );

    const float strokeWidth = appData.renderData().m_globalLandmarkParams.strokeWidth;

    const glm::vec3 worldViewNormal = camera::worldDirection( view.camera(), Directions::View::Back );
    const glm::vec4 worldViewPlane = math::makePlane( worldViewNormal, worldCrosshairs );

    // Render landmarks for each image
    for ( const auto& imgSegPair : I )
    {
        if ( ! imgSegPair.first )
        {
            // Non-existent image
            continue;
        }

        const auto imgUid = *( imgSegPair.first );
        const Image* img = appData.image( imgUid );

        if ( ! img )
        {
            spdlog::error( "Null image {} when rendering landmarks", imgUid );
            continue;
        }

        // Don't render landmarks for invisible image:
        /// @todo Need to properly manage global visibility vs. visibility for just one component
        if ( ! img->settings().globalVisibility() ||
             ( 1 == img->header().numComponentsPerPixel() && ! img->settings().visibility() ) )
        {
            continue;
        }

        const auto lmGroupUids = appData.imageToLandmarkGroupUids( imgUid );
        if ( lmGroupUids.empty() ) continue;

        // Slice spacing of the image along the view normal
        const float sliceSpacing = data::sliceScrollDistance( -worldViewNormal, *img );

        for ( const auto& lmGroupUid : lmGroupUids )
        {
            const LandmarkGroup* lmGroup = appData.landmarkGroup( lmGroupUid );
            if ( ! lmGroup )
            {
                spdlog::error( "Null landmark group for image {}", imgUid );
                continue;
            }

            if ( ! lmGroup->getVisibility() )
            {
                continue;
            }

            // Matrix that transforms landmark position from either Voxel or Subject to World space.
            const glm::mat4 world_T_landmark = ( lmGroup->getInVoxelSpace() )
                    ? img->transformations().worldDef_T_pixel()
                    : img->transformations().worldDef_T_subject();

            const float minDim = std::min( miewportViewBounds.bounds.width, miewportViewBounds.bounds.height );
            const float pixelsMaxLmSize = glm::clamp( lmGroup->getRadiusFactor() * minDim, sk_minSize, sk_maxSize );

            for ( const auto& p : lmGroup->getPoints() )
            {
                const size_t index = p.first;
                const PointRecord<glm::vec3>& point = p.second;

                if ( ! point.getVisibility() ) continue;

                // Put landmark into World space
                const glm::vec4 worldLmPos = world_T_landmark * glm::vec4{ point.getPosition(), 1.0f };
                const glm::vec3 worldLmPos3 = glm::vec3{ worldLmPos / worldLmPos.w };

                // Landmark must be within a distance of half the image slice spacing along the
                // direction of the view to be rendered in the view
                const float distLmToPlane = std::abs( math::signedDistancePointToPlane( worldLmPos3, worldViewPlane ) );

                // Maximum distance beyond which the landmark is not rendered:
                const float maxDist = 0.5f * sliceSpacing;

                if ( distLmToPlane >= maxDist )
                {
                    continue;
                }

                const glm::vec2 miewportPos = convertWorldToMiewportPos( worldLmPos3 );

                const bool inView = ( miewportViewBounds.bounds.xoffset < miewportPos.x &&
                                      miewportViewBounds.bounds.yoffset < miewportPos.y &&
                                      miewportPos.x < miewportViewBounds.bounds.xoffset + miewportViewBounds.bounds.width &&
                                      miewportPos.y < miewportViewBounds.bounds.yoffset + miewportViewBounds.bounds.height );

                if ( ! inView ) continue;

                // Use the landmark group color if defined
                const bool lmGroupColorOverride = lmGroup->getColorOverride();
                const glm::vec3 lmGroupColor = lmGroup->getColor();
                const float lmGroupOpacity = lmGroup->getOpacity();

                // Non-premult. alpha:
                const glm::vec4 fillColor{ ( lmGroupColorOverride )
                            ? lmGroupColor : point.getColor(),
                            lmGroupOpacity };

                /// @todo If landmark is selected, then highlight it here:
                const float strokeOpacity = 1.0f - std::pow( ( lmGroupOpacity - 1.0f ), 2.0f );

                const glm::vec4 strokeColor{ ( lmGroupColorOverride )
                            ? lmGroupColor : point.getColor(),
                            strokeOpacity };

                // Landmark radius depends on distance of the view plane from the landmark center
                const float radius = pixelsMaxLmSize *
                        std::sqrt( std::abs( 1.0f - std::pow( distLmToPlane / maxDist, 2.0f ) ) );

                drawCircle( nvg, miewportPos, radius, fillColor, strokeColor, strokeWidth );

                const bool renderIndices = lmGroup->getRenderLandmarkIndices();
                const bool renderNames = lmGroup->getRenderLandmarkNames();

                if ( renderIndices || renderNames )
                {
                    const float textOffset = radius + 0.7f;
                    const float textSize = 0.9f * pixelsMaxLmSize;

                    std::string indexString = ( renderIndices ) ? std::to_string( index ) : "";
                    std::string nameString = ( renderNames ) ? point.getName() : "";

                    // Non premult. alpha:
                    const auto lmGroupTextColor = lmGroup->getTextColor();
                    const glm::vec4 textColor{ ( lmGroupTextColor )
                                ? *lmGroupTextColor : fillColor, lmGroupOpacity };

                    drawText( nvg, miewportPos, indexString, nameString, textColor, textOffset, textSize );
                }
            }
        }
    }

    nvgResetScissor( nvg );

    endNvgFrame( nvg ); /*** END FRAME ***/
}


void renderAnnotations(
        NVGcontext* nvg,
        const camera::FrameBounds& miewportViewBounds,
        const glm::vec3& worldCrosshairs,
        AppData& appData,
        const View& view,
        const std::vector< std::pair< std::optional<uuids::uuid>, std::optional<uuids::uuid> > >& I )
{
    const Viewport& windowVP = appData.windowData().viewport();

    auto convertWorldToMiewportPos = [&view, &windowVP] ( const glm::vec3& worldPos ) -> glm::vec2
    {
        const glm::vec4 winClipPos =
                view.windowClip_T_viewClip() *
                camera::clip_T_world( view.camera() ) *
                glm::vec4{ worldPos, 1.0f };

        const glm::vec2 viewportPos = camera::viewport_T_windowClip( windowVP, glm::vec2{ winClipPos / winClipPos.w } );
        const glm::vec2 miewportPos = camera::miewport_T_viewport( windowVP.height(), viewportPos );
        return miewportPos;
    };


    nvgLineCap( nvg, NVG_BUTT );
    nvgLineJoin( nvg, NVG_MITER );

    startNvgFrame( nvg, windowVP ); /*** START FRAME ***/

    nvgScissor( nvg, miewportViewBounds.viewport[0], miewportViewBounds.viewport[1],
            miewportViewBounds.viewport[2], miewportViewBounds.viewport[3] );

    const glm::vec3 worldViewNormal = camera::worldDirection( view.camera(), Directions::View::Back );

    // Render annotations for each image
    for ( const auto& imgSegPair : I )
    {
        if ( ! imgSegPair.first )
        {
            continue; // Non-existent image
        }

        const auto imgUid = *( imgSegPair.first );
        const Image* img = appData.image( imgUid );

        if ( ! img )
        {
            spdlog::error( "Null image {} when rendering annotations", imgUid );
            continue;
        }

        // Don't render landmarks for invisible image:
        /// @todo Need to properly manage global visibility vs. visibility for just one component
        if ( ! img->settings().globalVisibility() ||
             ( 1 == img->header().numComponentsPerPixel() && ! img->settings().visibility() ) )
        {
            continue;
        }

//        spdlog::trace( "Rendering annotations for image {}", imgUid );

        // Compute plane equation in image Subject space:
        /// @todo Pull this out into a MathHelper function
        const glm::mat4& subject_T_world = img->transformations().subject_T_worldDef();
        const glm::mat4& world_T_subject = img->transformations().worldDef_T_subject();
        const glm::mat3& subject_T_world_IT = img->transformations().subject_T_worldDef_invTransp();

        const glm::vec3 subjectPlaneNormal{ subject_T_world_IT * worldViewNormal };

        glm::vec4 subjectPlanePoint = subject_T_world * glm::vec4{ worldCrosshairs, 1.0f };
        subjectPlanePoint /= subjectPlanePoint.w;

        const glm::vec4 subjectPlaneEquation = math::makePlane(
                    subjectPlaneNormal, glm::vec3{ subjectPlanePoint } );

        // Slice spacing of the image along the view normal is the plane distance threshold
        // for annotation searching:
        const float sliceSpacing = data::sliceScrollDistance( -worldViewNormal, *img );

//        spdlog::trace( "Finding annotations for plane {} with distance threshold {}",
//                       glm::to_string( subjectPlaneEquation ), sliceSpacing );

        const auto annotUids = data::findAnnotationsForImage(
                    appData, imgUid, subjectPlaneEquation, sliceSpacing );

        if ( annotUids.empty() ) continue;

        const Annotation* annot = appData.annotation( annotUids[0] );
        if ( ! annot ) continue;

        const bool visible = ( img->settings().visibility() && annot->getVisibility() );
        if ( ! visible ) continue;

//        spdlog::trace( "Found annotation {}", *annotUid );

        // Annotation vertices in Subject space:
        const std::vector<glm::vec2>& subjectPlaneVertices = annot->getBoundaryVertices( 0 );

        if ( subjectPlaneVertices.empty() ) continue;

        /// @todo Should annotation opacity be modulated with image opacity?
        /// Landmarks opacity is not.
        const glm::vec3 color = annot->getColor();
        const float opacity = annot->getOpacity() * static_cast<float>( img->settings().opacity() );

        nvgStrokeColor( nvg, nvgRGBAf( color.r, color.g, color.b, opacity ) );
        nvgStrokeWidth( nvg, annot->getLineThickness() );

        nvgBeginPath( nvg );

        for ( size_t i = 0; i < subjectPlaneVertices.size(); ++i )
        {
            const glm::vec3 subjectPos = annot->unprojectFromAnnotationPlaneToSubjectPoint( subjectPlaneVertices[i] );
            const glm::vec4 worldPos = world_T_subject * glm::vec4{ subjectPos, 1.0f };
            const glm::vec2 miewportPos = convertWorldToMiewportPos( glm::vec3{ worldPos / worldPos.w } );

            if ( 0 == i )
            {
                // Move pen to the first point:
                nvgMoveTo( nvg, miewportPos.x, miewportPos.y );
                continue;
            }
            else
            {
                nvgLineTo( nvg, miewportPos.x, miewportPos.y );
            }

//            spdlog::trace( "Point i = {}: {}", i, glm::to_string( mousePos ) );
        }

        nvgStroke( nvg );
    }

    nvgResetScissor( nvg );

    endNvgFrame( nvg ); /*** END FRAME ***/
}


void renderImageViewIntersections(
        NVGcontext* nvg,
        const camera::FrameBounds& miewportViewBounds,
        AppData& appData,
        const View& view,
        const std::vector< std::pair< std::optional<uuids::uuid>, std::optional<uuids::uuid> > >& I )
{
    // Line segment stipple length in pixels
    static constexpr float sk_stippleLen = 16.0f;

    const Viewport& windowVP = appData.windowData().viewport();

    auto miewport_T_world = [&view, &windowVP] ( const glm::vec4& worldPos ) -> glm::vec2
    {
        const glm::vec4 winClipPos = view.windowClip_T_viewClip() * camera::clip_T_world( view.camera() ) * worldPos;
        const glm::vec2 viewportPos = camera::viewport_T_windowClip( windowVP, glm::vec2{ winClipPos } );
        return camera::miewport_T_viewport( windowVP.height(), viewportPos );
    };

    nvgLineCap( nvg, NVG_BUTT );
    nvgLineJoin( nvg, NVG_MITER );

    startNvgFrame( nvg, windowVP ); /*** START FRAME ***/

    nvgScissor( nvg, miewportViewBounds.viewport[0], miewportViewBounds.viewport[1],
            miewportViewBounds.viewport[2], miewportViewBounds.viewport[3] );

    // Render border for each image
    for ( const auto& imgSegPair : I )
    {
        if ( ! imgSegPair.first ) continue;
        const auto imgUid = *( imgSegPair.first );
        const Image* img = appData.image( imgUid );
        if ( ! img ) continue;

        auto worldIntersections = view.computeImageSliceIntersection(
                    img, appData.state().worldCrosshairs() );

        if ( ! worldIntersections ) continue;

        // The last point is the centroid of the intersection. Ignore the centroid and replace it with a
        // duplicate of the first point. We need to double-up that point in order for line stippling to
        // work correctly. Also, no need to close the path with nvgClosePath if the last point is duplicated.
        worldIntersections->at( 6 ) = worldIntersections->at( 0 );

        const glm::vec3 color = img->settings().borderColor();
        const float opacity = static_cast<float>( img->settings().visibility() * img->settings().opacity() );

        nvgStrokeColor( nvg, nvgRGBAf( color.r, color.g, color.b, opacity ) );

        const auto activeImageUid = appData.activeImageUid();
        const bool isActive = ( activeImageUid && ( *activeImageUid == imgUid ) );

        nvgStrokeWidth( nvg, isActive ? 2.0f : 1.0f );

        glm::vec2 lastPos;

        nvgBeginPath( nvg );

        for ( size_t i = 0; i < worldIntersections->size(); ++i )
        {
            const glm::vec2 currPos = miewport_T_world( worldIntersections->at( i ) );

            if ( 0 == i )
            {
                // Move pen to the first point:
                nvgMoveTo( nvg, currPos.x, currPos.y );
                lastPos = currPos;
                continue;
            }

            if ( isActive )
            {
                // The active image gets a stippled line pattern
                const float dist = glm::distance( lastPos, currPos );
                const uint32_t numLines = static_cast<uint32_t>( dist / sk_stippleLen );

                if ( 0 == numLines )
                {
                    // At a minimum, draw one stipple line:
                    nvgLineTo( nvg, currPos.x, currPos.y );
                }

                for ( uint32_t j = 1; j <= numLines; ++j )
                {
                    const float t = static_cast<float>( j ) / static_cast<float>( numLines );
                    const glm::vec2 pos = lastPos + t * ( currPos - lastPos );

                    // To create the stipple pattern, alternate drawing lines and
                    // moving the pen on odd/even values of j:
                    if ( j % 2 )
                    {
                        nvgLineTo( nvg, pos.x, pos.y );
                    }
                    else
                    {
                        nvgMoveTo( nvg, pos.x, pos.y );
                    }
                }
            }
            else
            {
                // Non-active images get solid lines
                nvgLineTo( nvg, currPos.x, currPos.y );
            }

            lastPos = currPos;
        }

        nvgStroke( nvg );
    }

    nvgResetScissor( nvg );

    endNvgFrame( nvg ); /*** END FRAME ***/
}


void renderViewOutline(
        NVGcontext* nvg,
        const camera::FrameBounds& miewportViewBounds,
        bool drawActiveOutline )
{
    static constexpr float k_padOuter = 0.0f;
//    static constexpr float k_padInner = 2.0f;
    static constexpr float k_padActive = 3.0f;

    auto drawRectangle = [&nvg, &miewportViewBounds] ( float pad, float width, const NVGcolor& color )
    {
        nvgStrokeWidth( nvg, width );
        nvgStrokeColor( nvg, color );

        nvgBeginPath( nvg );

        nvgRect( nvg,
                 miewportViewBounds.bounds.xoffset + pad,
                 miewportViewBounds.bounds.yoffset + pad,
                 miewportViewBounds.bounds.width - 2.0f * pad,
                 miewportViewBounds.bounds.height - 2.0f * pad );

        nvgStroke( nvg );
    };


    if ( drawActiveOutline )
    {
        drawRectangle( k_padActive, 1.0f, s_yellow );
    }

    // View outline:
    drawRectangle( k_padOuter, 4.0f, s_grey50 );
//    drawRectangle( k_padInner, 1.0f, s_grey60 );
}


/**
 * @brief renderCrosshairsOverlay
 * @param nvg
 * @param windowVP
 * @param view
 * @param world_T_refSubject
 * @param worldCrosshairs
 * @param color RGBA, non-premultiplied by alpha
 */
void renderCrosshairsOverlay(
        NVGcontext* nvg,
        const camera::FrameBounds& miewportViewBounds,
        const View& view,
        const glm::vec4& color,
        const std::list<AnatomicalLabelPosInfo>& labelPosInfo )
{
    // Line segment stipple length in pixels
    static constexpr float sk_stippleLen = 8.0f;

    nvgLineCap( nvg, NVG_BUTT );
    nvgLineJoin( nvg, NVG_MITER );

    const auto& offset = view.offsetSetting();

    // Is the view offset from the crosshairs position?
    const bool viewIsOffset =
            ( ViewOffsetMode::RelativeToRefImageScrolls == offset.m_offsetMode &&
              0 != offset.m_relativeOffsetSteps ) ||

            ( ViewOffsetMode::RelativeToImageScrolls == offset.m_offsetMode &&
              0 != offset.m_relativeOffsetSteps ) ||

            ( ViewOffsetMode::Absolute == offset.m_offsetMode &&
              glm::epsilonNotEqual( offset.m_absoluteOffset, 0.0f, glm::epsilon<float>() ) );

    if ( viewIsOffset )
    {
        // Offset views get thinner, transparent crosshairs
        nvgStrokeWidth( nvg, 1.0f );
        nvgStrokeColor( nvg, nvgRGBAf( color.r, color.g, color.b, 0.5f * color.a ) );
    }
    else
    {
        nvgStrokeWidth( nvg, 2.0f );
        nvgStrokeColor( nvg, nvgRGBAf( color.r, color.g, color.b, color.a ) );
    }

    // Clip against the view bounds, even though not strictly necessary with how lines are defined
    nvgScissor( nvg, miewportViewBounds.viewport[0], miewportViewBounds.viewport[1],
            miewportViewBounds.viewport[2], miewportViewBounds.viewport[3] );

    for ( const auto& pos : labelPosInfo )
    {
        if ( ! pos.miewportXhairPositions )
        {
            // Only render crosshairs when there are two intersections with the view box:
            continue;
        }

        const auto& hits = *( pos.miewportXhairPositions );

        if ( camera::CameraType::Oblique != view.cameraType() )
        {
            // Orthogonal views get solid crosshairs:
            nvgBeginPath( nvg );
            nvgMoveTo( nvg, hits[0].x, hits[0].y );
            nvgLineTo( nvg, hits[1].x, hits[1].y );
            nvgStroke( nvg );
        }
        else
        {
            // Oblique views get stippled crosshairs:
            for ( int line = 0; line < 2; ++line )
            {
                const uint32_t numLines = glm::distance( hits[line], pos.miewportXhairCenterPos ) / sk_stippleLen;

                nvgBeginPath( nvg );
                for ( uint32_t i = 0; i <= numLines; ++i )
                {
                    const float t = static_cast<float>( i ) / static_cast<float>( numLines );
                    const glm::vec2 p = pos.miewportXhairCenterPos + t * ( hits[line] - pos.miewportXhairCenterPos );

                    if ( i % 2 ) nvgLineTo( nvg, p.x, p.y ); // when i odd
                    else nvgMoveTo( nvg, p.x, p.y ); // when i even
                }
                nvgStroke( nvg );
            }
        }
    }

    nvgResetScissor( nvg );
}


void renderLoadingOverlay( NVGcontext* nvg, const Viewport& windowVP )
{
    /// @todo Progress indicators: https://github.com/ocornut/imgui/issues/1901

    static const NVGcolor s_greyTextColor( nvgRGBA( 190, 190, 190, 255 ) );
    static const NVGcolor s_greyShadowColor( nvgRGBA( 64, 64, 64, 255 ) );

    static constexpr float sk_arcAngle = 1.0f / 16.0f * NVG_PI;
    static const std::string sk_loadingText = "Loading images...";

    nvgFontSize( nvg, 64.0f );
    nvgFontFace( nvg, ROBOTO_LIGHT.c_str() );

    nvgTextAlign( nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE );

    nvgFontBlur( nvg, 2.0f );
    nvgFillColor( nvg, s_greyShadowColor );
    nvgText( nvg, 0.5f * windowVP.width(), 0.5f * windowVP.height(), sk_loadingText.c_str(), nullptr );

    nvgFontBlur( nvg, 0.0f );
    nvgFillColor( nvg, s_greyTextColor );
    nvgText( nvg, 0.5f * windowVP.width(), 0.5f * windowVP.height(), sk_loadingText.c_str(), nullptr );

    const auto ms = std::chrono::duration_cast< std::chrono::milliseconds >(
                std::chrono::system_clock::now().time_since_epoch() );

    const float C = 2.0f * NVG_PI * static_cast<float>( ms.count() % 1000 ) / 1000.0f;
    const float radius = windowVP.width() / 16.0f;

    nvgStrokeWidth( nvg, 8.0f );
    nvgStrokeColor( nvg, s_greyTextColor );

    nvgBeginPath( nvg );
    nvgArc( nvg, 0.5f * windowVP.width(), 0.75f * windowVP.height(), radius, sk_arcAngle + C, C, NVG_CCW );
    nvgStroke( nvg );
}

} // anonymous


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

    auto it = m_appData.renderData().m_labelBufferTextures.emplace( labelTableUid, tex::Target::Texture1D );
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

    glClearColor( m_appData.renderData().m_backgroundColor.r,
                  m_appData.renderData().m_backgroundColor.g,
                  m_appData.renderData().m_backgroundColor.b, 1.0f );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

    renderImages();
//    renderOverlays();
    renderVectorOverlays();
}


void Rendering::setDeviceViewport( const glm::ivec4& deviceViewport  )
{
    // Set the OpenGL viewport in device units:
    glViewport( deviceViewport[0], deviceViewport[1], deviceViewport[2], deviceViewport[3] );
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


bool Rendering::showVectorOverlays() const { return m_showOverlays; }
void Rendering::setShowVectorOverlays( bool show ) { m_showOverlays = show; }


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


void Rendering::doRenderingAllImagePlanes(
        const View& view,
        const std::function< void ( GLShaderProgram&, const CurrentImages&, bool showEdges ) > renderFunc )
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

                renderFunc( P, CurrentImages{ imgSegPair }, U.showEdges );
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

                renderFunc( P, I, false );
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

                renderFunc( P, I, false );
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

                renderFunc( P, I, false );
            }
            P.stopUse();
        }

        unbindTextures( boundMetricTextures );
    }
}


void Rendering::doRenderingImageLandmarks(
        const View& view,
        const std::function< void ( const CurrentImages& ) > renderFunc )
{
    const auto shaderType = view.renderMode();
    const auto metricImages = view.metricImages();
    const auto renderedImages = view.renderedImages();

    if ( camera::ViewRenderMode::Image == shaderType ||
         camera::ViewRenderMode::Checkerboard == shaderType ||
         camera::ViewRenderMode::Quadrants == shaderType ||
         camera::ViewRenderMode::Flashlight == shaderType )
    {
        CurrentImages I;

        if ( camera::ViewRenderMode::Image == shaderType )
        {
            I = getImageAndSegUidsForImageShaders( renderedImages );
        }
        else if ( camera::ViewRenderMode::Checkerboard == shaderType ||
                  camera::ViewRenderMode::Quadrants == shaderType ||
                  camera::ViewRenderMode::Flashlight == shaderType )
        {
            I = getImageAndSegUidsForMetricShaders( metricImages ); // guaranteed size 2
        }

        for ( const auto& imgSegPair : I )
        {
            renderFunc( CurrentImages{ imgSegPair } );
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
        renderFunc( I );
    }
}


void Rendering::doRenderingImageAnnotations(
        const View& /*view*/,
        const std::function< void ( const CurrentImages& ) > /*renderFunc*/ )
{
//    const auto shaderType = view.renderMode();
//    const auto metricImages = view.metricImages();
//    const auto renderedImages = view.renderedImages();
}

void Rendering::renderImages()
{
    if ( ! m_isAppDoneLoadingImages )
    {
        // Don't render images if the app is still loading them
        return;
    }

    auto getImage = [this] ( const std::optional<uuids::uuid>& imageUid ) -> const Image*
    {
        return ( imageUid ? m_appData.image( *imageUid ) : nullptr );
    };

    const glm::vec3 worldCrosshairsOrigin = m_appData.state().worldCrosshairs().worldOrigin();

    const bool renderLandmarksOnTop = m_appData.renderData().m_globalLandmarkParams.renderOnTopOfAllImagePlanes;
    const bool renderAnnotationsOnTop = m_appData.renderData().m_globalAnnotationParams.renderOnTopOfAllImagePlanes;
    const bool renderImageIntersections = m_appData.renderData().m_globalSliceIntersectionParams.renderImageViewIntersections;

    for ( const auto& viewPair : m_appData.windowData().currentLayout().views() )
    {
        if ( ! viewPair.second ) continue;

        View& view = *( viewPair.second );

        if ( ! view.updateImageSlice( m_appData, worldCrosshairsOrigin ) ) continue;

        // Bounds of the view frame in Miewport space:
        const auto miewportViewBounds = camera::computeMiewportFrameBounds(
                    view.windowClipViewport(), m_appData.windowData().viewport().getAsVec4() );

        auto renderOneImage = [this, view, &worldCrosshairsOrigin, &renderLandmarksOnTop,
                &renderAnnotationsOnTop, &renderImageIntersections, &getImage, &miewportViewBounds]
                ( GLShaderProgram& program, const CurrentImages& I, bool showEdges )
        {
            renderImageQuad( program,
                             view.renderMode(),
                             m_appData.renderData().m_quad,
                             view,
                             worldCrosshairsOrigin,
                             m_appData.renderData().m_flashlightRadius,
                             m_appData.renderData().m_flashlightOverlays,
                             I, getImage, showEdges );

            if ( ! renderLandmarksOnTop )
            {
                renderLandmarks( m_nvg, miewportViewBounds, worldCrosshairsOrigin, m_appData, view, I );
                setupOpenGlState();
            }

            if ( ! renderAnnotationsOnTop )
            {
                renderAnnotations( m_nvg, miewportViewBounds, worldCrosshairsOrigin, m_appData, view, I );
                setupOpenGlState();
            }

            if ( renderImageIntersections )
            {
                renderImageViewIntersections( m_nvg, miewportViewBounds, m_appData, view, I );
                setupOpenGlState();
            }
        };

        doRenderingAllImagePlanes( view, renderOneImage );

        if ( renderLandmarksOnTop )
        {
            auto renderLandmarksForView = [this, view, &worldCrosshairsOrigin, &miewportViewBounds]
                    ( const CurrentImages& I )
            {
                renderLandmarks( m_nvg, miewportViewBounds, worldCrosshairsOrigin, m_appData, view, I );
                setupOpenGlState();
            };

            doRenderingImageLandmarks( view, renderLandmarksForView );
        }

        if ( renderAnnotationsOnTop )
        {
            auto renderAnnotationsForView = [this, view, &worldCrosshairsOrigin, &miewportViewBounds]
                    ( const CurrentImages& I )
            {
                renderAnnotations( m_nvg, miewportViewBounds, worldCrosshairsOrigin, m_appData, view, I );
                setupOpenGlState();
            };

            doRenderingImageAnnotations( view, renderAnnotationsForView );
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

        if ( const Image* refImage = m_appData.refImage() )
        {
            world_T_refSubject = refImage->transformations().worldDef_T_subject();
        }

        const auto activeViewUid = windowData.activeViewUid();
        const bool annotating = ( MouseMode::Annotate == m_appData.state().mouseMode() );

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

                renderCrosshairsOverlay( m_nvg, miewportViewBounds, *view, m_appData.renderData().m_crosshairsColor, labelPosInfo );
                renderAnatomicalLabels( m_nvg, miewportViewBounds, m_appData.renderData().m_anatomicalLabelColor, labelPosInfo );
            }

            const bool drawActiveOutline = ( annotating && activeViewUid && ( *activeViewUid == viewUid ) );
            renderViewOutline( m_nvg, miewportViewBounds, drawActiveOutline );
        }

        renderWindowOutline( m_nvg, windowVP );
    }
    else
    {
        renderLoadingOverlay( m_nvg, windowVP );

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

        fsUniforms.insertUniform( "texSampleSize", UniformType::Vec3, sk_zeroVec3 );
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
        fsUniforms.insertUniform( "texSampleSize", UniformType::Vec3Vector, Vec3Vector{ sk_zeroVec3, sk_zeroVec3 } );
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

