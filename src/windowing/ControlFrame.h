#ifndef CONTROL_FRAME_H
#define CONTROL_FRAME_H

#include "common/UuidRange.h"

#include "logic/camera/CameraTypes.h"
#include "ui/UiControls.h"

#include <list>
#include <set>

class AppData;


class ControlFrame
{
public:

    ControlFrame(
            camera::CameraType cameraType,
            camera::ViewRenderMode renderMode,
            UiControls uiControls );

    virtual ~ControlFrame() = default;

    camera::CameraType cameraType() const;
    virtual void setCameraType( const camera::CameraType& cameraType );

    camera::ViewRenderMode renderMode() const;
    virtual void setRenderMode( const camera::ViewRenderMode& shaderType );

    bool isImageRendered( const AppData& appData, size_t index );
    virtual void setImageRendered( const AppData& appData, size_t index, bool visible );

    const std::list<uuids::uuid>& renderedImages() const;
    virtual void setRenderedImages( const std::list<uuids::uuid>& imageUids, bool filterByDefaults );

    bool isImageUsedForMetric( const AppData& appData, size_t index );
    virtual void setImageUsedForMetric( const AppData& appData, size_t index, bool visible );

    const std::list<uuids::uuid>& metricImages() const;
    virtual void setMetricImages( const std::list<uuids::uuid>& imageUids );

    /// This one accounts for both rendered and metric images.
    const std::list<uuids::uuid>& visibleImages() const;

    void setPreferredDefaultRenderedImages( std::set<size_t> imageIndices );
    const std::set<size_t>& preferredDefaultRenderedImages() const;

    /// Call this when image order changes in order to update rendered and metric images:
    virtual void updateImageOrdering( uuid_range_t orderedImageUids );

    const UiControls& uiControls() const;


protected:

    /// Uids of images rendered in this frame. They are listed in the order in which they are
    /// rendered, with image 0 at the bottom.
    std::list<uuids::uuid> m_renderedImageUids;

    /// Uids of images used for metric calculation in this frame. The first image is the
    /// fixed image; the second image is the moving image. As of now, all metrics use two
    /// images, but we could potentially include metrics that use more than two images.
    std::list<uuids::uuid> m_metricImageUids;

    /// What images does this view prefer to render by default?
    std::set<size_t> m_preferredDefaultRenderedImages;

    /// Rendering mode
    camera::ViewRenderMode m_renderMode;

    /// Camera type
    camera::CameraType m_cameraType;

    /// What UI controls are show in the frame?
    UiControls m_uiControls;
};

#endif // CONTROL_FRAME_H
