#ifndef CONTROL_FRAME_H
#define CONTROL_FRAME_H

#include "common/Types.h"
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
            camera::ViewRenderMode shaderType,
            UiControls uiControls );

    virtual ~ControlFrame() = 0;

    camera::CameraType cameraType() const;
    virtual void setCameraType( const camera::CameraType& cameraType );

    camera::ViewRenderMode renderMode() const;
    virtual void setRenderMode( const camera::ViewRenderMode& shaderType );

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


private:

    // Uids of images rendered in this view, in order
    std::list<uuids::uuid> m_renderedImageUids;

    // Uids of images used for metric calculation in this view, in order
    std::list<uuids::uuid> m_metricImageUids;

    // What images does this view prefer to render by default?
    std::set<size_t> m_preferredDefaultRenderedImages;

    // Camera data
    camera::ViewRenderMode m_shaderType;
    camera::CameraType m_cameraType;

    UiControls m_uiControls;
};

#endif // CONTROL_FRAME_H
