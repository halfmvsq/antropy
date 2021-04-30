#ifndef VIEW_H
#define VIEW_H

#include "common/CoordinateFrame.h"
#include "common/Types.h"

#include "logic/camera/Camera.h"
#include "rendering/utility/math/SliceIntersectorTypes.h"
#include "windowing/ControlFrame.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <uuid.h>

#include <optional>
#include <set>
#include <list>

class Image;


/**
 * @brief Represents a view in the window. Each view is a visual representation of the
 * scene from a single orientation. The view holds its camera and information about the
 * image plane being rendered in it.
 */
class View : public ControlFrame
{
public:

    /**
     * @brief Construct a view
     *
     * @param[in] winClipViewport Viewport (left, bottom, width, height) of the view,
     * defined in Clip space of its enclosing window's viewport
     * (e.g. (-1, -1, 2, 2) is a view that covers the full window viewport and
     * (0, 0, 1, 1) is a view that covers the top-right quadrant of the window viewport)
     *
     * @param[in] numOffsets Number of scroll offsets (relative to the reference image)
     * from the crosshairs at which to render this view's image planes
     *
     * @param[in] cameraType Camera type of the view
     * @param[in] shaderType Shader type of the view
     */
    View( glm::vec4 winClipViewport,
          ViewOffsetSetting offsetSetting,
          camera::CameraType cameraType,
          camera::ViewRenderMode renderMode,
          camera::IntensityProjectionMode ipMode,
          UiControls uiControls,
          std::optional<uuids::uuid> cameraRotationSyncGroupUid,
          std::optional<uuids::uuid> translationSyncGroup,
          std::optional<uuids::uuid> zoomSyncGroup );

    void setCameraType( const camera::CameraType& newCameraType ) override;

    const camera::Camera& camera() const;
    camera::Camera& camera();

    /// Update the view's camera based on the crosshairs World-space position.
    /// @return The crosshairs position on the slice
    glm::vec3 updateImageSlice( const AppData& appData, const glm::vec3& worldCrosshairs );

    std::optional< intersection::IntersectionVerticesVec4 >
    computeImageSliceIntersection( const Image* image, const CoordinateFrame& crosshairs ) const;

    float clipPlaneDepth() const;

    const ViewOffsetSetting& offsetSetting() const;

    std::optional<uuids::uuid> cameraRotationSyncGroupUid() const;
    std::optional<uuids::uuid> cameraTranslationSyncGroupUid() const;
    std::optional<uuids::uuid> cameraZoomSyncGroupUid() const;


private:

    bool updateImageSliceIntersection( const AppData& appData, const glm::vec3& worldCrosshairs );

    /// View offset setting
    ViewOffsetSetting m_offset;

    camera::ProjectionType m_projectionType;
    camera::Camera m_camera;

    /// ID of the camera synchronization groups to which this view belongs
    std::optional<uuids::uuid> m_cameraRotationSyncGroupUid;
    std::optional<uuids::uuid> m_cameraTranslationSyncGroupUid;
    std::optional<uuids::uuid> m_cameraZoomSyncGroupUid;

    /// Depth (z component) of any point on the image plane to be rendered (defined in Clip space)
    float m_clipPlaneDepth;
};

#endif // VIEW_H
