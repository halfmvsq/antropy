#include "windowing/Layout.h"
#include "common/UuidUtility.h"
#include "logic/AppData.h"

Layout::Layout( bool isLightbox )
    :
      m_uid( generateRandomUuid() ),
      m_views(),
      m_cameraTranslationSyncGroups(),
      m_cameraZoomSyncGroups(),
      m_isLightbox( isLightbox ),
      m_renderedImageUids(),
      m_metricImageUids(),

      // Render the first image by default:
      m_preferredDefaultRenderdImages( { 0 } ),

      m_shaderType( camera::ShaderType::Image ),
      m_cameraType( camera::CameraType::Axial ),
      m_winMouseViewMinMaxCorners( { {0, 0}, {0, 0} } )
{
    m_uiControls = UiControls( isLightbox );
}

const uuids::uuid& Layout::uid() const
{
    return m_uid;
}

bool Layout::isLightbox() const
{
    return m_isLightbox;
}

const UiControls& Layout::uiControls() const
{
    return m_uiControls;
}

bool Layout::isImageRendered( const AppData& appData, size_t index )
{
    auto imageUid = appData.imageUid( index );
    if ( ! imageUid ) return false; // invalid image index

    auto it = std::find( std::begin( m_renderedImageUids ),
                         std::end( m_renderedImageUids ), *imageUid );
    return ( std::end( m_renderedImageUids ) != it );
}

/// @todo This code is duplicated in View.
/// Make Layout a subclass of View to reuse the code.
void Layout::setImageRendered( const AppData& appData, size_t index, bool visible )
{
    auto imageUid = appData.imageUid( index );
    if ( ! imageUid ) return; // invalid image index

    if ( ! visible )
    {
        m_renderedImageUids.remove( *imageUid );
        updateViews();
        return;
    }

    if ( std::end( m_renderedImageUids ) !=
         std::find( std::begin( m_renderedImageUids ),
                    std::end( m_renderedImageUids ), *imageUid ) )
    {
        return; // image alredy exists, so do nothing
    }

    bool inserted = false;

    for ( auto it = std::begin( m_renderedImageUids );
          std::end( m_renderedImageUids ) != it; ++it )
    {
        if ( const auto i = appData.imageIndex( *it ) )
        {
            if ( index < *i )
            {
                // Insert the desired image in the right place
                m_renderedImageUids.insert( it, *imageUid );
                inserted = true;
                break;
            }
        }
    }

    if ( ! inserted )
    {
        m_renderedImageUids.push_back( *imageUid );
    }

    updateViews();
}

const std::list<uuids::uuid>& Layout::renderedImages() const
{
    return m_renderedImageUids;
}

void Layout::setRenderedImages( const std::list<uuids::uuid>& imageUids, bool filterByDefaults )
{
    if ( filterByDefaults )
    {
        m_renderedImageUids.clear();
        size_t index = 0;

        for ( const auto& imageUid : imageUids )
        {
            if ( m_preferredDefaultRenderdImages.count( index ) > 0 )
            {
                m_renderedImageUids.push_back( imageUid );
            }
            ++index;
        }
    }
    else
    {
        m_renderedImageUids = imageUids;
    }

    updateViews();
}

bool Layout::isImageUsedForMetric( const AppData& appData, size_t index )
{
    auto imageUid = appData.imageUid( index );
    if ( ! imageUid ) return false; // invalid image index

    auto it = std::find( std::begin( m_metricImageUids ),
                         std::end( m_metricImageUids ), *imageUid );
    return ( std::end( m_metricImageUids ) != it );
}

void Layout::setImageUsedForMetric( const AppData& appData, size_t index, bool visible )
{
    static constexpr size_t MAX_IMAGES = 2;

    auto imageUid = appData.imageUid( index );
    if ( ! imageUid ) return; // invalid image index

    if ( ! visible )
    {
        m_metricImageUids.remove( *imageUid );
        updateViews();
        return;
    }

    if ( std::end( m_metricImageUids ) !=
         std::find( std::begin( m_metricImageUids ),
                    std::end( m_metricImageUids ), *imageUid ) )
    {
        return; // image alredy exists, so do nothing
    }

    if ( m_metricImageUids.size() >= MAX_IMAGES )
    {
        // If trying to add another image UID to list with 2 or more UIDs,
        // remove the last UID to make room
        m_metricImageUids.erase( std::prev( std::end( m_metricImageUids ) ) );
    }

    bool inserted = false;

    for ( auto it = std::begin( m_metricImageUids );
          std::end( m_metricImageUids ) != it; ++it )
    {
        if ( const auto i = appData.imageIndex( *it ) )
        {
            if ( index < *i )
            {
                // Insert the desired image in the right place
                m_metricImageUids.insert( it, *imageUid );
                inserted = true;
                break;
            }
        }
    }

    if ( ! inserted )
    {
        m_metricImageUids.push_back( *imageUid );
    }

    updateViews();
}

const std::list<uuids::uuid>& Layout::metricImages() const
{
    return m_metricImageUids;
}

void Layout::setMetricImages( const std::list<uuids::uuid>& imageUids )
{
    m_metricImageUids = imageUids;
}

const std::list<uuids::uuid>& Layout::visibleImages() const
{
    static const std::list<uuids::uuid> sk_noImages;

    if ( camera::ShaderType::Image == m_shaderType )
    {
        return renderedImages();
    }
    else if ( camera::ShaderType::Disabled == m_shaderType )
    {
        return sk_noImages;
    }
    else
    {
        return metricImages();
    }
}

void Layout::setPreferredDefaultRenderedImages( std::set<size_t> imageIndices )
{
    m_preferredDefaultRenderdImages = std::move( imageIndices );
}

const std::set<size_t>& Layout::preferredDefaultRenderedImages() const
{
    return m_preferredDefaultRenderdImages;
}

void Layout::updateImageOrdering( uuid_range_t orderedImageUids )
{
    std::list<uuids::uuid> newRenderedImageUids;
    std::list<uuids::uuid> newMetricImageUids;

    // Loop through the images in new order:
    for ( const auto& imageUid : orderedImageUids )
    {
        auto it1 = std::find( std::begin( m_renderedImageUids ),
                              std::end( m_renderedImageUids ), imageUid );

        if ( std::end( m_renderedImageUids ) != it1 )
        {
            // This image is rendered, so place in new order:
            newRenderedImageUids.push_back( imageUid );
        }

        auto it2 = std::find( std::begin( m_metricImageUids ),
                              std::end( m_metricImageUids ), imageUid );

        if ( std::end( m_metricImageUids ) != it2 &&
             newMetricImageUids.size() < 2 )
        {
            // This image is in metric computation, so place in new order:
            newMetricImageUids.push_back( imageUid );
        }
    }

    m_renderedImageUids = newRenderedImageUids;
    m_metricImageUids = newMetricImageUids;

    updateViews();
}

void Layout::setWinMouseMinMaxCoords( std::pair< glm::vec2, glm::vec2 > corners )
{
    m_winMouseViewMinMaxCorners = std::move( corners );
}

const std::pair< glm::vec2, glm::vec2 >& Layout::winMouseMinMaxCoords() const
{
    return m_winMouseViewMinMaxCorners;
}

void Layout::setCameraType( const camera::CameraType& cameraType )
{
    m_cameraType = cameraType;
    updateViews();
}

void Layout::setShaderType( const camera::ShaderType& shaderType )
{
    m_shaderType = shaderType;
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
        view.second->setShaderType( m_shaderType );
    }
}

camera::CameraType Layout::cameraType() const { return m_cameraType; }
camera::ShaderType Layout::shaderType() const { return m_shaderType; }

std::unordered_map< uuids::uuid, std::shared_ptr<View> >&
Layout::views() { return m_views; }

std::unordered_map< uuids::uuid, std::list<uuids::uuid> >&
Layout::cameraTranslationSyncGroups() { return m_cameraTranslationSyncGroups; }

std::unordered_map< uuids::uuid, std::list<uuids::uuid> >&
Layout::cameraZoomSyncGroups() { return m_cameraZoomSyncGroups; }

const std::unordered_map< uuids::uuid, std::shared_ptr<View> >&
Layout::views() const { return m_views; }

const std::unordered_map< uuids::uuid, std::list<uuids::uuid> >&
Layout::cameraTranslationSyncGroups() const { return m_cameraTranslationSyncGroups; }

const std::unordered_map< uuids::uuid, std::list<uuids::uuid> >&
Layout::cameraZoomSyncGroups() const { return m_cameraZoomSyncGroups; }
