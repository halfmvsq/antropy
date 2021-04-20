#include "rendering/ImageDrawing.h"
#include "rendering/utility/gl/GLShaderProgram.h"

#include "image/Image.h"
#include "logic/camera/CameraHelpers.h"

#include "windowing/View.h"

#include <glm/glm.hpp>

#include <glad/glad.h>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>


void drawImageQuad(
        GLShaderProgram& program,
        const camera::ViewRenderMode& shaderType,
        RenderData::Quad& quad,
        const View& view,
        const glm::vec3& worldCrosshairs,
        float flashlightRadius,
        bool flashlightOverlays,
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

        const glm::vec4 clipCrosshairs = camera::clip_T_world( view.camera() ) * glm::vec4{ worldCrosshairs, 1.0f };
        program.setUniform( "clipCrosshairs", glm::vec2{ clipCrosshairs / clipCrosshairs.w } );

        if ( showEdges )
        {
            const Image* image = getImage( I[0].first );

            if ( ! image )
            {
                spdlog::error( "Null image when rendering plane with edges" );
                return;
            }

            const glm::mat4 imagePixel_T_clip =
                    image->transformations().pixel_T_worldDef() *
                    camera::world_T_clip( view.camera() );

            glm::vec4 pO = imagePixel_T_clip * glm::vec4{ 0.0f, 0.0f, -1.0f, 1.0 }; pO /= pO.w;
            glm::vec4 pX = imagePixel_T_clip * glm::vec4{ 1.0f, 0.0f, -1.0f, 1.0 }; pX /= pX.w;
            glm::vec4 pY = imagePixel_T_clip * glm::vec4{ 0.0f, 1.0f, -1.0f, 1.0 }; pY /= pY.w;

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

        const glm::mat4 imagePixel_T_clip =
                img0->transformations().pixel_T_worldDef() *
                camera::world_T_clip( view.camera() );

        glm::vec4 pO = imagePixel_T_clip * sk_clipO; pO /= pO.w;
        glm::vec4 pX = imagePixel_T_clip * sk_clipX; pX /= pX.w;
        glm::vec4 pY = imagePixel_T_clip * sk_clipY; pY /= pY.w;

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
