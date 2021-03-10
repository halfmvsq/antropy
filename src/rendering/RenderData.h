#ifndef RENDER_DATA_H
#define RENDER_DATA_H

#include "rendering/utility/containers/VertexAttributeInfo.h"
#include "rendering/utility/containers/VertexIndicesInfo.h"
#include "rendering/utility/gl/GLTexture.h"
#include "rendering/utility/gl/GLVertexArrayObject.h"
#include "rendering/utility/gl/GLBufferObject.h"
#include "rendering/utility/gl/GLBufferTexture.h"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <uuid.h>

#include <unordered_map>
#include <vector>


/**
 * @brief Objects that encapsulate OpenGL state
 */
struct RenderData
{
    /**
     * @brief Uniforms for a single image component
     */
    struct ImageUniforms
    {
        glm::vec2 cmapSlopeIntercept{ 1.0f, 0.0f }; // Slope and intercept for image colormap

        glm::mat4 imgTexture_T_world{ 1.0f }; // Mapping from World to image Texture space
        glm::mat4 segTexture_T_world{ 1.0f }; // Mapping from World to segmentation Texture space

        glm::vec2 slopeIntercept{ 1.0f, 0.0f }; // Image intensity slope and intercept
        glm::vec2 largestSlopeIntercept{ 1.0f, 0.0f }; // Image intensity slope and intercept (giving the largest window)
        glm::vec2 thresholds{ 0.0f, 1.0f }; // Image intensity lower & upper thresholds, after mapping to OpenGL texture values

        float imgOpacity{ 0.0f }; // Image opacity
        float segOpacity{ 0.0f }; // Segmentation opacity

        bool showEdges = false;
        bool thresholdEdges = true;
        float edgeMagnitude = 0.0f;
        bool useFreiChen = false;
//        bool windowedEdges = false;
        bool overlayEdges = false;
        bool colormapEdges = false;
        glm::vec4 edgeColor{ 0.0f }; // RGBA, premultiplied by alpha
    };


    struct Quad
    {
        Quad();

        VertexAttributeInfo m_positionsInfo;
        VertexIndicesInfo m_indicesInfo;

        GLBufferObject m_positionsObject;
        GLBufferObject m_indicesObject;

        GLVertexArrayObject m_vao;
        GLVertexArrayObject::IndexedDrawParams m_vaoParams;
    };

    struct Circle
    {
        Circle();

        VertexAttributeInfo m_positionsInfo;
        VertexIndicesInfo m_indicesInfo;

        GLBufferObject m_positionsObject;
        GLBufferObject m_indicesObject;

        GLVertexArrayObject m_vao;
        GLVertexArrayObject::IndexedDrawParams m_vaoParams;
    };


    RenderData();

    Quad m_quad;
    Circle m_circle;

    std::unordered_map< uuids::uuid, std::vector<GLTexture> > m_imageTextures;
    std::unordered_map< uuids::uuid, GLTexture > m_segTextures;
    std::unordered_map< uuids::uuid, GLTexture > m_labelBufferTextures;
    std::unordered_map< uuids::uuid, GLTexture > m_colormapTextures;

    // Blank textures that are bound to image and segmentation units
    // in case no image or segmentation is loaded from disk:
    GLTexture m_blankImageTexture;
    GLTexture m_blankSegTexture;

    // Map of image uniforms, keyed by image UID
    std::unordered_map< uuids::uuid, ImageUniforms > m_uniforms;


    // Flag that crosshairs shall snap to center of the nearest reference image voxel
    bool m_snapCrosshairsToReferenceVoxels;

    // Should the images only be shown inside of masked regions?
    bool m_maskedImages;

    // Should image segmentation opacity be modulated by the image opacity?
    bool m_modulateSegOpacityWithImageOpacity;

    // Flag that image opacities are adjusted in "mix" mode, which allows
    // blending between a pair of images
    bool m_opacityMixMode;

    glm::vec3 m_backgroundColor; // View background (clear) color
    glm::vec4 m_crosshairsColor; // Crosshairs color (non-premultiplied by alpha)
    glm::vec4 m_anatomicalLabelColor; // Anatomical label text color (non-premultiplied by alpha)

    /// @brief Metric parameters
    struct MetricParams
    {
        // Index of the colormap to apply to metric images
        size_t m_colorMapIndex = 0;

        // Slope and intercept to apply to metric values prior to indexing into the colormap.
        // This value gets updated when m_colorMapIndex or m_invertCmap changes.
        glm::vec2 m_cmapSlopeIntercept{ 1.0f, 0.0f };

        // Slope and intercept to apply to metric values
        glm::vec2 m_slopeIntercept{ 1.0f, 0.0f };

        // Is the color map inverted?
        bool m_invertCmap = false;

        // Should the metric only be computed inside the masked region?
        bool m_doMasking = false;

        // Should the metric be computed in 3D (across the full volume) or
        // in 2D (across only the current slice)?
        // Not currently implemented.
        bool m_volumetric = false;
    };

    MetricParams m_squaredDifferenceParams;
    MetricParams m_crossCorrelationParams;
    MetricParams m_jointHistogramParams;


    // Edge detection magnitude and smoothing
    glm::vec2 m_edgeMagnitudeSmoothing;

    // Number of squares along the longest dimensions for the checkerboard shader
    int m_numCheckerboardSquares;

    // Magenta/cyan (true) overlay colors or red/green (false)?
    bool m_overlayMagentaCyan;

    // Should comparison be done in x,y directions?
    glm::bvec2 m_quadrants;

    // Should the difference metric use squared difference (true) or absolute difference (false)?
    bool m_useSquare;

    // Flashlight radius
    float m_flashlightRadius;


    struct LandmarkParams
    {
        float strokeWidth = 1.0f;
        glm::vec3 textColor;

        /// Flag to either render landmarks on top of all image planes (true)
        /// or interspersed with each image plane (false)
        bool renderOnTopOfAllImagePlanes = false;
    };

    struct AnnotationParams
    {
        float strokeWidth = 1.0f;
        glm::vec3 textColor;

        /// Flag to either render annotations on top of all image planes (true)
        /// or interspersed with each image plane (false)
        bool renderOnTopOfAllImagePlanes = false;
    };

    struct SliceIntersectionParams
    {
        float strokeWidth = 1.0f;

        /// Render the intersections of images with the view planes?
        bool renderImageViewIntersections = true;
    };

    LandmarkParams m_globalLandmarkParams;
    AnnotationParams m_globalAnnotationParams;
    SliceIntersectionParams m_globalSliceIntersectionParams;
};

#endif // RENDER_DATA_H
