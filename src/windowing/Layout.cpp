#include "windowing/Layout.h"
#include "common/UuidUtility.h"
#include "logic/app/Data.h"


namespace
{

// Viewport of a full window, defined in window Clip space:
static const glm::vec4 sk_winClipFullWindowViewport{ -1.0f, -1.0f, 2.0f, 2.0f };

}


Layout::Layout( bool isLightbox )
    :
      ControlFrame(
          sk_winClipFullWindowViewport,
          camera::CameraType::Axial,
          camera::ViewRenderMode::Image,
          camera::IntensityProjectionMode::None,
          UiControls( isLightbox ) ),

      m_uid( generateRandomUuid() ),
      m_views(),

      m_cameraRotationSyncGroups(),
      m_cameraTranslationSyncGroups(),
      m_cameraZoomSyncGroups(),

      m_isLightbox( isLightbox )
{
    // Render the first image by default:
    m_preferredDefaultRenderedImages = { 0 };
}

void Layout::setImageRendered( const AppData& appData, size_t index, bool visible )
{
    ControlFrame::setImageRendered( appData, index, visible );
    updateViews();
}

void Layout::setRenderedImages( const std::list<uuids::uuid>& imageUids, bool filterByDefaults )
{
    ControlFrame::setRenderedImages( imageUids, filterByDefaults );
    updateViews();
}

void Layout::setMetricImages( const std::list<uuids::uuid>& imageUids )
{
    ControlFrame::setMetricImages( imageUids );
    updateViews();
}

void Layout::setImageUsedForMetric( const AppData& appData, size_t index, bool visible )
{
    ControlFrame::setImageUsedForMetric( appData, index, visible );
    updateViews();
}

void Layout::updateImageOrdering( uuid_range_t orderedImageUids )
{
    ControlFrame::updateImageOrdering( orderedImageUids );
    updateViews();
}

void Layout::setCameraType( const camera::CameraType& cameraType )
{
    ControlFrame::setCameraType( cameraType );
    updateViews();
}

void Layout::setRenderMode( const camera::ViewRenderMode& renderMode )
{
    ControlFrame::setRenderMode( renderMode );
    updateViews();
}

void Layout::setIntensityProjectionMode( const camera::IntensityProjectionMode& ipMode )
{
    ControlFrame::setIntensityProjectionMode( ipMode );
    updateViews();
}

void Layout::updateViews()
{
    for ( auto& view : m_views )
    {
        if ( ! view.second ) continue;
        view.second->setRenderedImages( m_renderedImageUids, false );
        view.second->setMetricImages( m_metricImageUids );
        view.second->setCameraType( m_cameraType );
        view.second->setRenderMode( m_renderMode );
    }
}

const uuids::uuid& Layout::uid() const { return m_uid; }
bool Layout::isLightbox() const { return m_isLightbox; }

std::unordered_map< uuids::uuid, std::shared_ptr<View> >&
Layout::views() { return m_views; }

std::unordered_map< uuids::uuid, std::list<uuids::uuid> >&
Layout::cameraRotationSyncGroups() { return m_cameraRotationSyncGroups; }

std::unordered_map< uuids::uuid, std::list<uuids::uuid> >&
Layout::cameraTranslationSyncGroups() { return m_cameraTranslationSyncGroups; }

std::unordered_map< uuids::uuid, std::list<uuids::uuid> >&
Layout::cameraZoomSyncGroups() { return m_cameraZoomSyncGroups; }

const std::unordered_map< uuids::uuid, std::shared_ptr<View> >&
Layout::views() const { return m_views; }

const std::unordered_map< uuids::uuid, std::list<uuids::uuid> >&
Layout::cameraRotationSyncGroups() const { return m_cameraRotationSyncGroups; }

const std::unordered_map< uuids::uuid, std::list<uuids::uuid> >&
Layout::cameraTranslationSyncGroups() const { return m_cameraTranslationSyncGroups; }

const std::unordered_map< uuids::uuid, std::list<uuids::uuid> >&
Layout::cameraZoomSyncGroups() const { return m_cameraZoomSyncGroups; }
