#include "windowing/WindowData.h"

#include "common/Exception.hpp"
#include "common/UuidUtility.h"

#include "logic/camera/CameraTypes.h"
#include "logic/camera/CameraHelpers.h"

#include <glm/glm.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <boost/range.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/filtered.hpp>

#undef min
#undef max


namespace
{

Layout createFourUpLayout()
{
    using namespace camera;

    const int32_t numOffsets = 0;
    const UiControls uiControls( true );

    const auto noTranslationSyncGroup = std::nullopt;
    const auto noZoomSyncGroup = std::nullopt;

    const auto zoomSyncGroupUid = generateRandomUuid();

    Layout layout( false );

    auto zoomGroup = layout.cameraZoomSyncGroups().try_emplace( zoomSyncGroupUid ).first;

    {
        // top right
        auto view = std::make_shared<View>(
                    Viewport{ 0.0f, 0.0f, 1.0f, 1.0f }, numOffsets,
                    CameraType::Coronal, ViewRenderMode::Image,
                    uiControls,
                    noTranslationSyncGroup, zoomSyncGroupUid );

        const auto viewUid = generateRandomUuid();
        layout.views().emplace( viewUid, std::move( view ) );
        zoomGroup->second.push_back( viewUid );
    }
    {
        // top left
        auto view = std::make_shared<View>(
                    Viewport{ -1.0f, 0.0f, 1.0f, 1.0f }, numOffsets,
                    CameraType::Sagittal, ViewRenderMode::Image,
                    uiControls,
                    noTranslationSyncGroup, zoomSyncGroupUid );

        const auto viewUid = generateRandomUuid();
        layout.views().emplace( viewUid, std::move( view ) );
        zoomGroup->second.push_back( viewUid );
    }
    {
        // bottom left
        auto view = std::make_shared<View>(
                    Viewport{ -1.0f, -1.0f, 1.0f, 1.0f }, numOffsets,
                    CameraType::ThreeD, ViewRenderMode::Disabled,
                    uiControls,
                    noTranslationSyncGroup, noZoomSyncGroup );

        const auto viewUid = generateRandomUuid();
        layout.views().emplace( viewUid, std::move( view ) );
        zoomGroup->second.push_back( viewUid );
    }
    {
        // bottom right
        auto view = std::make_shared<View>(
                    Viewport{ 0.0f, -1.0f, 1.0f, 1.0f }, numOffsets,
                    CameraType::Axial, ViewRenderMode::Image,
                    uiControls,
                    noTranslationSyncGroup, zoomSyncGroupUid );

        const auto viewUid = generateRandomUuid();
        layout.views().emplace( viewUid, std::move( view ) );
        zoomGroup->second.push_back( viewUid );
    }

    return layout;
}


Layout createTriLayout()
{
    using namespace camera;

    const int32_t numOffsets = 0;
    const UiControls uiControls( true );

    const auto noTranslationSyncGroup = std::nullopt;
    const auto noZoomSyncGroup = std::nullopt;
    const auto zoomSyncGroupUid = generateRandomUuid();

    Layout layout( false );

    auto zoomGroup = layout.cameraZoomSyncGroups().try_emplace( zoomSyncGroupUid ).first;

    {
        // left
        auto view = std::make_shared<View>(
                    Viewport{ -1.0f, -1.0f, 1.5f, 2.0f }, numOffsets,
                    CameraType::Axial, ViewRenderMode::Image,
                    uiControls,
                    noTranslationSyncGroup, noZoomSyncGroup );

        const auto viewUid = generateRandomUuid();
        layout.views().emplace( viewUid, std::move( view ) );
    }
    {
        // bottom right
        auto view = std::make_shared<View>(
                    Viewport{ 0.5f, -1.0f, 0.5f, 1.0f }, numOffsets,
                    CameraType::Coronal, ViewRenderMode::Image,
                    uiControls,
                    noTranslationSyncGroup, zoomSyncGroupUid );

        const auto viewUid = generateRandomUuid();
        layout.views().emplace( viewUid, std::move( view ) );
        zoomGroup->second.push_back( viewUid );
    }
    {
        // top right
        auto view = std::make_shared<View>(
                    Viewport{ 0.5f, 0.0f, 0.5f, 1.0f }, numOffsets,
                    CameraType::Sagittal, ViewRenderMode::Image,
                    uiControls,
                    noTranslationSyncGroup, zoomSyncGroupUid );

        const auto viewUid = generateRandomUuid();
        layout.views().emplace( viewUid, std::move( view ) );
        zoomGroup->second.push_back( viewUid );
    }

    return layout;
}


Layout createTriTopBottomLayout( size_t numRows )
{
    using namespace camera;

    const int32_t numOffsets = 0;
    const UiControls uiControls( true );

    const auto axiTranslationSyncGroupUid = generateRandomUuid();
    const auto corTranslationSyncGroupUid = generateRandomUuid();
    const auto sagTranslationSyncGroupUid = generateRandomUuid();

    const auto axiZoomSyncGroupUid = generateRandomUuid();
    const auto corZoomSyncGroupUid = generateRandomUuid();
    const auto sagZoomSyncGroupUid = generateRandomUuid();

    Layout layout( false );

    auto axiTransGroup = layout.cameraTranslationSyncGroups().try_emplace( axiTranslationSyncGroupUid ).first;
    auto corTransGroup = layout.cameraTranslationSyncGroups().try_emplace( corTranslationSyncGroupUid ).first;
    auto sagTransGroup = layout.cameraTranslationSyncGroups().try_emplace( sagTranslationSyncGroupUid ).first;

    // Should zoom be synchronized across columns or across rows?
    auto axiZoomGroup = layout.cameraZoomSyncGroups().try_emplace( axiZoomSyncGroupUid ).first;
    auto corZoomGroup = layout.cameraZoomSyncGroups().try_emplace( corZoomSyncGroupUid ).first;
    auto sagZoomGroup = layout.cameraZoomSyncGroups().try_emplace( sagZoomSyncGroupUid ).first;

    for ( size_t r = 0; r < numRows; ++r )
    {
        const float height = 2.0f / static_cast<float>( numRows );
        const float bottom = 1.0f - ( static_cast<float>( r + 1 ) ) * height;

        {
            // axial
            auto view = std::make_shared<View>(
                        Viewport{ -1.0f, bottom, 2.0f/3.0f, height }, numOffsets,
                        CameraType::Axial, ViewRenderMode::Image,
                        uiControls,
                        axiTranslationSyncGroupUid, axiZoomSyncGroupUid );

            view->setPreferredDefaultRenderedImages( { r } );

            const auto viewUid = generateRandomUuid();
            layout.views().emplace( viewUid, std::move( view ) );

            axiTransGroup->second.push_back( viewUid );
            axiZoomGroup->second.push_back( viewUid );
        }

        {
            // coronal
            auto view = std::make_shared<View>(
                        Viewport{ -1.0f/3.0f, bottom, 2.0f/3.0f, height }, numOffsets,
                        CameraType::Coronal, ViewRenderMode::Image,
                        uiControls,
                        corTranslationSyncGroupUid, corZoomSyncGroupUid );

            view->setPreferredDefaultRenderedImages( { r } );

            const auto viewUid = generateRandomUuid();
            layout.views().emplace( viewUid, std::move( view ) );

            corTransGroup->second.push_back( viewUid );
            corZoomGroup->second.push_back( viewUid );
        }
        {
            // sagittal
            auto view = std::make_shared<View>(
                        Viewport{ 1.0f/3.0f, bottom, 2.0f/3.0f, height }, numOffsets,
                        CameraType::Sagittal, ViewRenderMode::Image,
                        uiControls,
                        sagTranslationSyncGroupUid, sagZoomSyncGroupUid );

            view->setPreferredDefaultRenderedImages( { r } );

            const auto viewUid = generateRandomUuid();
            layout.views().emplace( viewUid, std::move( view ) );

            sagTransGroup->second.push_back( viewUid );
            sagZoomGroup->second.push_back( viewUid );
        }
    }

    return layout;
}


Layout createGridLayout(
        int width,
        int height,
        bool offsetViews,
        bool isLightbox,
        const camera::CameraType& cameraType )
{
    static const camera::ViewRenderMode s_shaderType = camera::ViewRenderMode::Image;

    Layout layout( isLightbox );

    if ( isLightbox )
    {
        layout.setCameraType( cameraType );
        layout.setRenderMode( s_shaderType );

        // Lightbox layouts prefer to render reference image only by default:
        layout.setPreferredDefaultRenderedImages( { 0 } );
    }

    const auto translationSyncGroupUid = generateRandomUuid();
    auto transGroup = layout.cameraTranslationSyncGroups().try_emplace(
                translationSyncGroupUid ).first;

    const auto zoomSyncGroupUid = generateRandomUuid();
    auto zoomGroup = layout.cameraZoomSyncGroups().try_emplace( zoomSyncGroupUid ).first;

    const float w = 2.0f / static_cast<float>( width );
    const float h = 2.0f / static_cast<float>( height );

    size_t count = 0;

    for ( int j = 0; j < height; ++j )
    {
        for ( int i = 0; i < width; ++i )
        {
            const float l = -1.0f + static_cast<float>( i ) * w;
            const float b = -1.0f + static_cast<float>( j ) * h;

            const int counter = width * j + i;
            const int numOffsets = ( offsetViews ? ( counter - width * height / 2 ) : 0 );

            auto view = std::make_shared<View>(
                        Viewport{ l, b, w, h },
                        numOffsets,
                        cameraType,
                        s_shaderType,
                        UiControls( ! isLightbox ),
                        translationSyncGroupUid,
                        zoomSyncGroupUid );

            if ( ! isLightbox )
            {
                // Make each view render a different image by default:
                view->setPreferredDefaultRenderedImages( { count } );
            }

            const auto viewUid = generateRandomUuid();
            layout.views().emplace( viewUid, std::move( view ) );

            // Synchronize translations and zooms for all views in the layout:
            transGroup->second.push_back( viewUid );
            zoomGroup->second.push_back( viewUid );

            ++count;
        }
    }

    return layout;
}

} // anonymous


WindowData::WindowData()
    :
      m_viewport( 0, 0, 800, 800 ),
      m_layouts(),
      m_currentLayout( 0 ),
      m_activeViewUid( std::nullopt )
{
    setupViews();
    setCurrentLayoutIndex( 0 );
}

void WindowData::setupViews()
{
    m_layouts.emplace_back( createFourUpLayout() );
    m_layouts.emplace_back( createTriLayout() );
    m_layouts.emplace_back( createGridLayout ( 1, 1, false, false, camera::CameraType::Axial ) );
    m_layouts.emplace_back( createGridLayout ( 2, 1, false, false, camera::CameraType::Axial ) );
    m_layouts.emplace_back( createGridLayout ( 3, 1, false, false, camera::CameraType::Axial ) );
    updateViews();
}

void WindowData::addGridLayout( int width, int height, bool offsetViews, bool isLightbox )
{
    m_layouts.emplace_back( createGridLayout( width, height, offsetViews, isLightbox,
                                              camera::CameraType::Axial ) );
    updateViews();
}

void WindowData::addLightboxLayoutForImage( size_t numSlices )
{
    constexpr bool k_offsetViews = true;
    constexpr bool k_isLightbox = true;

    const int w = static_cast<int>( std::sqrt( numSlices + 1 ) );
    const auto div = std::div( static_cast<int>( numSlices ), w );
    const int h = div.quot + ( div.rem > 0 ? 1 : 0 );

    addGridLayout( w, h, k_offsetViews, k_isLightbox );
}

void WindowData::addAxCorSagLayout( size_t numImages )
{
    m_layouts.emplace_back( createTriTopBottomLayout( numImages ) );
    updateViews();
}

void WindowData::removeLayout( size_t index )
{
    if ( index >= m_layouts.size() ) return;

    m_layouts.erase( std::begin( m_layouts ) + static_cast<long>( index ) );
}


void WindowData::setDefaultRenderedImagesForLayout(
        Layout& layout, uuid_range_t orderedImageUids )
{
    constexpr bool s_filterAgainstDefaults = true;

    std::list<uuids::uuid> renderedImages;
    std::list<uuids::uuid> metricImages;

    size_t count = 0;

    for ( const auto& uid : orderedImageUids )
    {
        renderedImages.push_back( uid );

        if ( count < 2 )
        {
            // By default, compute the metric using the first two images:
            metricImages.push_back( uid );
            ++count;
        }
    }

    if ( layout.isLightbox() )
    {
        layout.setRenderedImages( renderedImages, s_filterAgainstDefaults );
        layout.setMetricImages( metricImages );
        return;
    }

    for ( auto& view : layout.views() )
    {
        if ( ! view.second ) continue;
        view.second->setRenderedImages( renderedImages, s_filterAgainstDefaults );
        view.second->setMetricImages( metricImages );
    }
}


void WindowData::setDefaultRenderedImagesForAllLayouts( uuid_range_t orderedImageUids )
{
    constexpr bool s_filterAgainstDefaults = true;

    std::list<uuids::uuid> renderedImages;
    std::list<uuids::uuid> metricImages;

    size_t count = 0;

    for ( const auto& uid : orderedImageUids )
    {
        renderedImages.push_back( uid );

        if ( count < 2 )
        {
            // By default, compute the metric using the first two images:
            metricImages.push_back( uid );
            ++count;
        }
    }

    for ( auto& layout : m_layouts )
    {
        if ( layout.isLightbox() )
        {
            layout.setRenderedImages( renderedImages, s_filterAgainstDefaults );
            layout.setMetricImages( metricImages );
            continue;
        }

        for ( auto& view : layout.views() )
        {
            if ( ! view.second ) continue;
            view.second->setRenderedImages( renderedImages, s_filterAgainstDefaults );
            view.second->setMetricImages( metricImages );
        }
    }
}


void WindowData::updateImageOrdering( uuid_range_t orderedImageUids )
{
    for ( auto& layout : m_layouts )
    {
        if ( layout.isLightbox() )
        {
            layout.updateImageOrdering( orderedImageUids );
            continue;
        }

        for ( auto& view : layout.views() )
        {
            if ( ! view.second ) continue;
            view.second->updateImageOrdering( orderedImageUids );
        }
    }
}


void WindowData::recenterViews(
        const glm::vec3& worldCenter,
        const glm::vec3& worldFov,
        float scale, bool resetZoom )
{
    for ( auto& layout : m_layouts )
    {
        for ( auto& view : layout.views() )
        {
            if ( ! view.second ) continue;

            if ( resetZoom )
            {
                camera::resetZoom( view.second->camera() );
            }

            camera::positionCameraForWorldTargetAndFov(
                        view.second->camera(), worldFov, worldCenter, scale );
        }
    }

    updateViews();
}

void WindowData::recenterView(
        const uuids::uuid& viewUid,
        const glm::vec3& worldCenter,
        const glm::vec3& worldFov )
{
    View* view = currentView( viewUid );
    if ( ! view ) return;

    // This function doesn't mess with the view's FOV
    camera::positionCameraForWorldTarget( view->camera(), worldFov, worldCenter );

    updateViews();
}


uuid_range_t WindowData::currentViewUids() const
{
    return ( m_layouts.at( m_currentLayout ).views() | boost::adaptors::map_keys );
}

const View* WindowData::currentView( const uuids::uuid& uid ) const
{
    const auto& views = m_layouts.at( m_currentLayout ).views();
    auto it = views.find( uid );
    if ( std::end( views ) != it )
    {
        if ( it->second ) return it->second.get();
    }
    return nullptr;
}

View* WindowData::currentView( const uuids::uuid& uid )
{
    auto& views = m_layouts.at( m_currentLayout ).views();
    auto it = views.find( uid );
    if ( std::end( views ) != it )
    {
        if ( it->second ) return it->second.get();
    }
    return nullptr;
}

std::optional<uuids::uuid> WindowData::currentViewUidAtCursor(
        const glm::vec2& windowPos ) const
{
    if ( m_layouts.empty() ) return std::nullopt;

    const glm::vec2 winClipPos = camera::ndc2d_T_view( m_viewport, windowPos );

    for ( const auto& view : m_layouts.at( m_currentLayout ).views() )
    {
        if ( ! view.second ) continue;
        const auto& winClipVp = view.second->winClipViewport();

        if ( ( winClipVp.left() <= winClipPos.x ) &&
             ( winClipPos.x < winClipVp.left() + winClipVp.width() ) &&
             ( winClipVp.bottom() <= winClipPos.y ) &&
             ( winClipPos.y < winClipVp.bottom() + winClipVp.height() ) )
        {
            return view.first;
        }
    }

    return std::nullopt;
}

std::optional<uuids::uuid> WindowData::activeViewUid() const
{
    return m_activeViewUid;
}

void WindowData::setActiveViewUid( const std::optional<uuids::uuid>& uid )
{
    m_activeViewUid = uid;
}

size_t WindowData::numLayouts() const
{
    return m_layouts.size();
}

size_t WindowData::currentLayoutIndex() const
{
    return m_currentLayout;
}

const Layout* WindowData::layout( size_t index ) const
{
    if ( index < m_layouts.size() )
    {
        return &( m_layouts.at( index ) );
    }
    return nullptr;
}

const Layout& WindowData::currentLayout() const
{
    return m_layouts.at( m_currentLayout );
}

Layout& WindowData::currentLayout()
{
    return m_layouts.at( m_currentLayout );
}

void WindowData::setCurrentLayoutIndex( size_t index )
{
    if ( index >= m_layouts.size() ) return;
    m_currentLayout = index;
}

void WindowData::cycleCurrentLayout( int step )
{
    const int i = static_cast<int>( currentLayoutIndex() );
    const int N = static_cast<int>( numLayouts() );

    setCurrentLayoutIndex( static_cast<size_t>( ( N + i + step ) % N ) );
}

const Viewport& WindowData::viewport() const
{
    return m_viewport;
}

void WindowData::resizeViewport( int width, int height )
{
    m_viewport.setWidth( static_cast<float>( width ) );
    m_viewport.setHeight( static_cast<float>( height ) );
    updateViews();
}

void WindowData::setDeviceScaleRatio( const glm::vec2& scale )
{
    spdlog::trace( "Setting device scale ratio to {}x{}", scale.x, scale.y );
    m_viewport.setDevicePixelRatio( scale );
    updateViews();
}

uuid_range_t WindowData::cameraTranslationGroupViewUids(
        const uuids::uuid& syncGroupUid ) const
{
    const auto& currentLayout = m_layouts.at( m_currentLayout );
    const auto it = currentLayout.cameraTranslationSyncGroups().find( syncGroupUid );
    if ( std::end( currentLayout.cameraTranslationSyncGroups() ) != it ) return it->second;
    return {};
}

uuid_range_t WindowData::cameraZoomGroupViewUids(
        const uuids::uuid& syncGroupUid ) const
{
    const auto& currentLayout = m_layouts.at( m_currentLayout );
    const auto it = currentLayout.cameraZoomSyncGroups().find( syncGroupUid );
    if ( std::end( currentLayout.cameraZoomSyncGroups() ) != it ) return it->second;
    return {};
}

void WindowData::applyImageSelectionToAllCurrentViews(
        const uuids::uuid& referenceViewUid )
{
    constexpr bool s_filterAgainstDefaults = false;

    const View* referenceView = currentView( referenceViewUid );
    if ( ! referenceView ) return;

    const auto renderedImages = referenceView->renderedImages();
    const auto metricImages = referenceView->metricImages();

    for ( auto& viewUid : currentViewUids() )
    {
        View* view = currentView( viewUid );
        if ( ! view ) continue;

        view->setRenderedImages( renderedImages, s_filterAgainstDefaults );
        view->setMetricImages( metricImages );
    }
}

void WindowData::applyViewShaderToAllCurrentViews( const uuids::uuid& referenceViewUid )
{
    const View* referenceView = currentView( referenceViewUid );
    if ( ! referenceView ) return;

    const auto shaderType = referenceView->renderMode();

    for ( auto& viewUid : currentViewUids() )
    {
        View* view = currentView( viewUid );
        if ( ! view ) continue;

        if ( camera::CameraType::ThreeD == view->cameraType() )
        {
            // Don't allow changing shader of 3D views
            continue;
        }

        view->setRenderMode( shaderType );
    }
}

void WindowData::updateViews()
{
    recomputeViewAspectRatios();
    recomputeViewCorners();
}

void WindowData::recomputeViewAspectRatios()
{
    for ( auto& layout : m_layouts )
    {
        for ( auto& view : layout.views() )
        {
            if ( ! view.second ) continue;

            const auto ratio = m_viewport.aspectRatio() *
                    view.second->winClipViewport().aspectRatio();

            view.second->camera().setAspectRatio( ratio );
        }
    }
}

void WindowData::recomputeViewCorners()
{
    // Bottom-left and top-right coordinates in Clip space
    static const glm::vec4 s_clipViewBL{ -1, -1, 0, 1 };
    static const glm::vec4 s_clipViewTR{ 1, 1, 0, 1 };

    for ( auto& layout : m_layouts )
    {
        for ( auto& view : layout.views() )
        {
            if ( layout.isLightbox() )
            {
                const glm::vec4 winClipViewBL = s_clipViewBL;
                const glm::vec2 winPixelViewBL = camera::view_T_ndc( m_viewport, glm::vec2{ winClipViewBL } );
                const glm::vec2 winMouseViewBL = camera::mouse_T_view( m_viewport, winPixelViewBL );

                const glm::vec4 winClipViewTR = s_clipViewTR;
                const glm::vec2 winPixelViewTR = camera::view_T_ndc( m_viewport, glm::vec2{ winClipViewTR } );
                const glm::vec2 winMouseViewTR = camera::mouse_T_view( m_viewport, winPixelViewTR );

                const glm::vec2 cornerMin = glm::min( winMouseViewBL, winMouseViewTR );
                const glm::vec2 cornerMax = glm::max( winMouseViewBL, winMouseViewTR );

                layout.setWinMouseMinMaxCoords( { cornerMin, cornerMax } );
            }

            if ( ! view.second ) continue;

            const glm::vec4 winClipViewBL = view.second->winClip_T_viewClip() * s_clipViewBL;
            const glm::vec2 winPixelViewBL = camera::view_T_ndc( m_viewport, glm::vec2{ winClipViewBL } );
            const glm::vec2 winMouseViewBL = camera::mouse_T_view( m_viewport, winPixelViewBL );

            const glm::vec4 winClipViewTR = view.second->winClip_T_viewClip() * s_clipViewTR;
            const glm::vec2 winPixelViewTR = camera::view_T_ndc( m_viewport, glm::vec2{ winClipViewTR } );
            const glm::vec2 winMouseViewTR = camera::mouse_T_view( m_viewport, winPixelViewTR );

            const glm::vec2 cornerMin = glm::min( winMouseViewBL, winMouseViewTR );
            const glm::vec2 cornerMax = glm::max( winMouseViewBL, winMouseViewTR );

            view.second->setWinMouseMinMaxCoords( { cornerMin, cornerMax } );
        }
    }
}
