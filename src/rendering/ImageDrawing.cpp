#include "rendering/ImageDrawing.h"
#include "rendering/utility/UnderlyingEnumType.h"
#include "rendering/utility/gl/GLShaderProgram.h"

#include "common/DataHelper.h"
#include "image/Image.h"
#include "logic/camera/CameraHelpers.h"

#include "windowing/View.h"

#include <glm/glm.hpp>

#include <glad/glad.h>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>


namespace
{

/**
 * @brief Compute Texture-space direction to sample direction along a camera view axis
 * @param[in] pixel_T_clip Clip-to-Pixel transformation matrix for the view camera and image
 * @param[in] invPixelDims
 * @param[in] axis View axis
 * @return Sampling direction in Texture space
 */
glm::vec3 computeTexSamplingDir(
        const glm::mat4& pixel_T_clip,
        const glm::vec3& invPixelDims,
        const Directions::View& axis )
{
    static const glm::vec4 clipOrigin{ 0.0f, 0.0f, -1.0f, 1.0 };
    const glm::vec4 clipPos = clipOrigin + glm::vec4{ Directions::get( axis ), 0.0f };

    const glm::vec4 pixelPos = pixel_T_clip * clipPos;
    const glm::vec4 pixelOrigin = pixel_T_clip * clipOrigin;

    const glm::vec3 pixelDir = glm::normalize( pixelPos / pixelPos.w - pixelOrigin / pixelOrigin.w );

    return glm::dot( glm::abs( pixelDir ), invPixelDims ) * pixelDir;
}


/**
 * @brief Compute half the number of samples for MIPs
 * @param camera
 * @param image
 * @param doMaxExtentMip
 * @return
 */
int computeHalfNumMipSamples(
        const camera::Camera& camera,
        const Image& image,
        float mipSlabThickness_mm,
        bool doMaxExtentMip )
{
    int halfNumMipSamples = 0;

    if ( ! doMaxExtentMip )
    {
        const float mmPerStep = data::sliceScrollDistance(
                    camera::worldDirection( camera, Directions::View::Front ), image );

        halfNumMipSamples = static_cast<int>(
                    std::floor( 0.5f * mipSlabThickness_mm / mmPerStep ) );
    }
    else
    {
        // To achieve maximum extent, use the number of samples along the image diagonal.
        // That way, the MIP will hit all voxels.
        halfNumMipSamples = static_cast<int>(
                    std::ceil( glm::length( glm::vec3{ image.header().pixelDimensions() } ) ) );
    }

    return halfNumMipSamples;
}

}


void drawImageQuad(
        GLShaderProgram& program,
        const camera::ViewRenderMode& renderMode,
        RenderData::Quad& quad,
        const View& view,
        const glm::vec3& worldCrosshairs,
        float flashlightRadius,
        bool flashlightOverlays,
        float mipSlabThickness_mm,
        bool doMaxExtentMip,
        const std::vector< std::pair< std::optional<uuids::uuid>, std::optional<uuids::uuid> > >& I,
        const std::function< const Image* ( const std::optional<uuids::uuid>& imageUid ) > getImage,
        bool showEdges )
{
    static const glm::vec4 sk_clipO{ 0.0f, 0.0f, -1.0f, 1.0 };
    static const glm::vec4 sk_clipX{ 1.0f, 0.0f, -1.0f, 1.0 };
    static const glm::vec4 sk_clipY{ 0.0f, 1.0f, -1.0f, 1.0 };

    if ( I.empty() )
    {
        spdlog::error( "No images provided when rendering plane" );
        return;
    }

    const Image* image0 = getImage( I[0].first );

    if ( ! image0 )
    {
        spdlog::error( "Null image when rendering textured quad" );
        return;
    }

    const glm::mat4 world_T_clip = camera::world_T_clip( view.camera() );

    // Direction to sample direction along the camera view's Z axis for image 0:
    glm::vec3 texSamplingDirZ( 0.0f );

    // Half the number of samples for MIPs (for image 0):
    int halfNumMipSamples = 0;

    // Only compute these if doing a MIP:
    if ( camera::IntensityProjectionMode::None != view.intensityProjectionMode() )
    {
        const glm::mat4 pixel_T_clip = world_T_clip * image0->transformations().pixel_T_worldDef();

        texSamplingDirZ = computeTexSamplingDir(
                    pixel_T_clip, image0->transformations().invPixelDimensions(),
                    Directions::View::Back );

        halfNumMipSamples = computeHalfNumMipSamples(
                    view.camera(), *image0, mipSlabThickness_mm, doMaxExtentMip );
    }


    // Set the view transformation uniforms that are common to all programs:
    program.setUniform( "view_T_clip", view.windowClip_T_viewClip() );
    program.setUniform( "world_T_clip", camera::world_T_clip( view.camera() ) );
    program.setUniform( "clipDepth", view.clipPlaneDepth() );


    if ( camera::ViewRenderMode::Image == renderMode ||
         camera::ViewRenderMode::Checkerboard == renderMode ||
         camera::ViewRenderMode::Quadrants == renderMode ||
         camera::ViewRenderMode::Flashlight == renderMode )
    {
        program.setUniform( "aspectRatio", view.camera().aspectRatio() );
        program.setUniform( "flashlightRadius", flashlightRadius );
        program.setUniform( "flashlightOverlays", flashlightOverlays );

        const glm::vec4 clipCrosshairs =
                camera::clip_T_world( view.camera() ) * glm::vec4{ worldCrosshairs, 1.0f };

        program.setUniform( "clipCrosshairs", glm::vec2{ clipCrosshairs / clipCrosshairs.w } );

        if ( showEdges )
        {
            const glm::mat4 pixel_T_clip = world_T_clip * image0->transformations().pixel_T_worldDef();

            const glm::vec3 texSamplingDirX = computeTexSamplingDir(
                        pixel_T_clip, image0->transformations().invPixelDimensions(),
                        Directions::View::Right );

            const glm::vec3 texSamplingDirY = computeTexSamplingDir(
                        pixel_T_clip, image0->transformations().invPixelDimensions(),
                        Directions::View::Up );

            program.setUniform( "texSamplingDirX", texSamplingDirX );
            program.setUniform( "texSamplingDirY", texSamplingDirY );
        }
        else
        {
            program.setUniform( "mipMode", underlyingType_asInt32( view.intensityProjectionMode() ) );
            program.setUniform( "halfNumMipSamples", halfNumMipSamples );
            program.setUniform( "texSamplingDirZ", texSamplingDirZ );
        }
    }
    else if ( camera::ViewRenderMode::Difference == renderMode )
    {
        program.setUniform( "mipMode", underlyingType_asInt32( view.intensityProjectionMode() ) );
        program.setUniform( "halfNumMipSamples", halfNumMipSamples );
        program.setUniform( "texSamplingDirZ", texSamplingDirZ );
    }
    else if ( camera::ViewRenderMode::CrossCorrelation == renderMode )
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

        const glm::mat4 img0Pixel_T_clip =
                img0->transformations().pixel_T_worldDef() *
                camera::world_T_clip( view.camera() );

        const glm::vec4 ppO = img0Pixel_T_clip * sk_clipO;
        const glm::vec4 ppX = img0Pixel_T_clip * sk_clipX;
        const glm::vec4 ppY = img0Pixel_T_clip * sk_clipY;

        const glm::vec3 pixelDirX = glm::normalize( ppX / ppX.w - ppO / ppO.w );
        const glm::vec3 pixelDirY = glm::normalize( ppY / ppY.w - ppO / ppO.w );

        const glm::vec3 img0_invDims = img0->transformations().invPixelDimensions();

        const glm::vec3 tex0SamplingDirX = glm::dot( glm::abs( pixelDirX ), img0_invDims ) * pixelDirX;
        const glm::vec3 tex0SamplingDirY = glm::dot( glm::abs( pixelDirY ), img0_invDims ) * pixelDirY;

        program.setUniform( "tex0SamplingDirX", tex0SamplingDirX );
        program.setUniform( "tex0SamplingDirY", tex0SamplingDirY );
    }

    quad.m_vao.bind();
    {
        quad.m_vao.drawElements( quad.m_vaoParams );
    }
    quad.m_vao.release();
}
