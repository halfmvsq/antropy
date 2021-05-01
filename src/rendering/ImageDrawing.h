#ifndef IMAGE_DRAWING_H
#define IMAGE_DRAWING_H

#include "logic/camera/CameraTypes.h"
#include "rendering/RenderData.h"

#include <glm/fwd.hpp>

#include <uuid.h>

#include <functional>
#include <optional>
#include <utility>
#include <vector>

class GLShaderProgram;
class Image;
class View;


void drawImageQuad(
        GLShaderProgram& program,
        const camera::ViewRenderMode& shaderType,
        RenderData::Quad& quad,
        const View& view,
        const glm::vec3& worldOrigin,
        float flashlightRadius,
        bool flashlightOverlays,
        float mipSlabThickness_mm,
        bool doMaxExtentMip,
        const std::vector< std::pair< std::optional<uuids::uuid>, std::optional<uuids::uuid> > >& I,
        const std::function< const Image* ( const std::optional<uuids::uuid>& imageUid ) > getImage,
        bool showEdges );

#endif // IMAGE_DRAWING_H
