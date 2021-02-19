#ifndef LAYOUT_H
#define LAYOUT_H

#include "common/UuidRange.h"
#include "ui/UiControls.h"
#include "windowing/View.h"

#include <uuid.h>

#include <list>
#include <memory>
#include <set>
#include <unordered_map>

class AppData;


/// @brief Represents a set of views rendered together in the window at one time
class Layout
{
public:

    Layout( bool isLightbox );

    const uuids::uuid& uid() const;
    bool isLightbox() const;

    /**** START COMMON FUNCTIONALITY WITH VIEW ****/

    const UiControls& uiControls() const;

    void setCameraType( const camera::CameraType& cameraType );
    void setRenderMode( const camera::ViewRenderMode& shaderType );

    camera::CameraType cameraType() const;
    camera::ViewRenderMode renderMode() const;


    // Returns true iff the image at index is rendered in the view
    bool isImageRendered( const AppData& appData, size_t index );
    void setImageRendered( const AppData& appData, size_t index, bool visible );

    /// Get/set the list of images rendered in views of this layout
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

    /**** END COMMON FUNCTIONALITY WITH VIEW ****/


    void setWinMouseMinMaxCoords( std::pair< glm::vec2, glm::vec2 > corners );
    const std::pair< glm::vec2, glm::vec2 >& winMouseMinMaxCoords() const;

    void recenter();

    std::unordered_map< uuids::uuid, std::shared_ptr<View> >& views();
    std::unordered_map< uuids::uuid, std::list<uuids::uuid> >& cameraTranslationSyncGroups();
    std::unordered_map< uuids::uuid, std::list<uuids::uuid> >& cameraZoomSyncGroups();

    const std::unordered_map< uuids::uuid, std::shared_ptr<View> >& views() const;
    const std::unordered_map< uuids::uuid, std::list<uuids::uuid> >& cameraTranslationSyncGroups() const;
    const std::unordered_map< uuids::uuid, std::list<uuids::uuid> >& cameraZoomSyncGroups() const;


private:

    void updateViews();

    uuids::uuid m_uid;

    // Views of the layout, keyed by their UID
    std::unordered_map< uuids::uuid, std::shared_ptr<View> > m_views;

    // Map of camera translation synchronization group UID to the list of view UIDs in the group
    std::unordered_map< uuids::uuid, std::list<uuids::uuid> > m_cameraTranslationSyncGroups;

    // Map of camera zoom synchronization group UID to the list of view UIDs in the group
    std::unordered_map< uuids::uuid, std::list<uuids::uuid> > m_cameraZoomSyncGroups;

    // If true, then this layout has UI controls that affect all of its views,
    // rather than each view having its own UI controls
    bool m_isLightbox;

    /**** START COMMON FUNCTIONALITY WITH VIEW ****/

    /// @note the members below only apply if m_isLightbox is true.
    /// We should do some inheritance here...
    UiControls m_uiControls;

    // Uids of images rendered in this layout's views, in order
    std::list<uuids::uuid> m_renderedImageUids;

    // Uids of images used for metric calculation in this view, in order
    std::list<uuids::uuid> m_metricImageUids;

    // What images does this view prefer to render by default?
    std::set<size_t> m_preferredDefaultRenderdImages;

    camera::ViewRenderMode m_shaderType;
    camera::CameraType m_cameraType;

    /**** START COMMON FUNCTIONALITY WITH VIEW ****/

    // Min and max corners of the view in coordinates of the enclosing window
    std::pair< glm::vec2, glm::vec2 > m_winMouseViewMinMaxCorners;
};

#endif // LAYOUT_H
