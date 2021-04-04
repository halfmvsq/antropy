#ifndef VIEW_H
#define VIEW_H

#include "common/CoordinateFrame.h"
#include "common/Types.h"
#include "common/UuidRange.h"

#include "logic/camera/Camera.h"
#include "logic/camera/CameraTypes.h"

#include "rendering/utility/math/SliceIntersector.h"

#include "ui/UiControls.h"

#include "windowing/ControlFrame.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <uuid.h>

#include <optional>
#include <set>
#include <list>
#include <utility>

class AppData;
class Image;


/**
 * @brief Represents a view in the window. Each view is a visual representation of the
 * scene from a single orientation. The view holds its camera and information about the
 * image plane being rendered in it.
 */
class View // : public ControlFrame
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
          camera::ViewRenderMode shaderType,
          UiControls uiControls,
          std::optional<uuids::uuid> cameraRotationSyncGroupUid,
          std::optional<uuids::uuid> translationSyncGroup,
          std::optional<uuids::uuid> zoomSyncGroup );

    /// Update the view's camera based on the crosshairs World-space position.
    /// @return True iff successful view update
    bool updateImageSlice( const AppData& appData, const glm::vec3& worldCrosshairs );

    std::optional< intersection::IntersectionVerticesVec4 >
    computeImageSliceIntersection( const Image* image, const CoordinateFrame& crosshairs );

    const glm::vec4& winClipViewport() const;
    const glm::mat4& winClip_T_viewClip() const;
    const glm::mat4& viewClip_T_winClip() const;

    float clipPlaneDepth() const;

    const ViewOffsetSetting& offsetSetting() const;

    void setWinMouseMinMaxCoords( std::pair< glm::vec2, glm::vec2 > corners );
    const std::pair< glm::vec2, glm::vec2 >& winMouseMinMaxCoords() const;

    std::optional<uuids::uuid> cameraRotationSyncGroupUid() const;
    std::optional<uuids::uuid> cameraTranslationSyncGroupUid() const;
    std::optional<uuids::uuid> cameraZoomSyncGroupUid() const;


    /**** START COMMON FUNCTIONALITY WITH LAYOUT ****/
    const camera::Camera& camera() const;
    camera::Camera& camera();

    camera::CameraType cameraType() const;
    void setCameraType( const camera::CameraType& cameraType );

    camera::ViewRenderMode renderMode() const;
    void setRenderMode( const camera::ViewRenderMode& shaderType );

    bool isImageRendered( const AppData& appData, size_t index );
    void setImageRendered( const AppData& appData, size_t index, bool visible );

    const std::list<uuids::uuid>& renderedImages() const;
    void setRenderedImages( const std::list<uuids::uuid>& imageUids, bool filterByDefaults );

    bool isImageUsedForMetric( const AppData& appData, size_t index );
    void setImageUsedForMetric( const AppData& appData, size_t index, bool visible );

    const std::list<uuids::uuid>& metricImages() const;
    void setMetricImages( const std::list<uuids::uuid>& imageUids );

    // This one accounts for both rendered and metric images.
    const std::list<uuids::uuid>& visibleImages() const;

    void setPreferredDefaultRenderedImages( std::set<size_t> imageIndices );
    const std::set<size_t>& preferredDefaultRenderedImages() const;

    // Call this when image order changes in order to update rendered and metric images:
    void updateImageOrdering( uuid_range_t orderedImageUids );

    const UiControls& uiControls() const;

    /**** END COMMON FUNCTIONALITY WITH LAYOUT ****/


private:

    bool updateImageSliceIntersection( const AppData& appData, const glm::vec3& worldCrosshairs );

    // Viewport of the view defined in Clip space of the enclosing window,
    // which spans from bottom left [-1, -1] to top right [1, 1].
    // A full-window view has viewport (left = -1, bottom = -1, width = 2, height = 2)
    glm::vec4 m_winClipViewport;

    // Transformations between Clip spaces of the view and its enclosing window
    glm::mat4 m_winClip_T_viewClip;
    glm::mat4 m_viewClip_T_winClip;

    // View offset setting
    ViewOffsetSetting m_offset;


    /**** START COMMON FUNCTIONALITY WITH LAYOUT ****/

    // Uids of images rendered in this view, in order
    std::list<uuids::uuid> m_renderedImageUids;

    // Uids of images used for metric calculation in this view, in order
    std::list<uuids::uuid> m_metricImageUids;

    // What images does this view prefer to render by default?
    std::set<size_t> m_preferredDefaultRenderedImages;

    // Camera data
    camera::ViewRenderMode m_shaderType;
    camera::CameraType m_cameraType;
    camera::ProjectionType m_projectionType;
    camera::Camera m_camera;

    UiControls m_uiControls;
    /**** END COMMON FUNCTIONALITY WITH LAYOUT ****/


    // ID of the camera synchronization groups to which this view belongs
    std::optional<uuids::uuid> m_cameraRotationSyncGroupUid;
    std::optional<uuids::uuid> m_cameraTranslationSyncGroupUid;
    std::optional<uuids::uuid> m_cameraZoomSyncGroupUid;

    // Depth (z component) of any point on the image plane to be rendered (defined in Clip space)
    float m_clipPlaneDepth;

    // Min and max corners of the view in coordinates of the enclosing window
    std::pair< glm::vec2, glm::vec2 > m_winMouseViewMinMaxCorners;

    /// Object for intersecting the view plane with the 3D images
    SliceIntersector m_sliceIntersector;
};

#endif // VIEW_H
