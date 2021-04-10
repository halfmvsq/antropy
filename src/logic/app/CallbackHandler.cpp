#include "logic/app/CallbackHandler.h"

#include "common/DataHelper.h"
#include "common/MathFuncs.h"
#include "common/Types.h"

#include "image/SegUtil.tpp"

#include "logic/app/Data.h"
#include "logic/annotation/Polygon.tpp"
#include "logic/camera/CameraHelpers.h"
#include "logic/camera/MathUtility.h"

#include "rendering/Rendering.h"

#include "windowing/GlfwWrapper.h"

#include <GridCut/GridGraph_3D_6C.h>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <chrono>
#include <memory>


namespace
{

static const glm::vec2 sk_minClip{ -1, -1 };
static const glm::vec2 sk_maxClip{ 1, 1 };

static constexpr float k_viewAABBoxScaleFactor = 1.03f;

}



CallbackHandler::CallbackHandler(
        AppData& appData,
        GlfwWrapper& glfwWrapper,
        Rendering& rendering )
    :
      m_appData( appData ),
      m_glfw( glfwWrapper ),
      m_rendering( rendering )
{
}


bool CallbackHandler::clearSegVoxels( const uuids::uuid& segUid )
{
    static constexpr int64_t ZERO_VAL = 0;

    Image* seg = m_appData.seg( segUid );
    if ( ! seg ) return false;

    const glm::ivec3 dataSizeInt{ seg->header().pixelDimensions() };

    for ( int k = 0; k < dataSizeInt.z; ++k )
    {
        for ( int j = 0; j < dataSizeInt.y; ++j )
        {
            for ( int i = 0; i < dataSizeInt.x; ++i )
            {
                seg->setValue( 0, i, j, k, ZERO_VAL );
            }
        }
    }

    const glm::uvec3 dataOffset = glm::uvec3{ 0 };
    const glm::uvec3 dataSize = glm::uvec3{ seg->header().pixelDimensions() };

    m_rendering.updateSegTexture(
                segUid, seg->header().memoryComponentType(),
                dataOffset, dataSize, seg->bufferAsVoid( 0 ) );

    return true;
}


bool CallbackHandler::executeGridCutSegmentation(
        const uuids::uuid& imageUid,
        const uuids::uuid& seedSegUid,
        const uuids::uuid& resultSegUid )
{
    using namespace std::chrono;

    constexpr short K = 1000;
    constexpr double SIGMA2 = 100.0;

    auto weight = [&] ( double A ) -> short
    {
        return static_cast<short>( 1 + static_cast<double>( K ) * std::exp( -A * A / SIGMA2 ) );
    };

    using Grid = GridGraph_3D_6C<short, short, int>;

    const Image* image = m_appData.image( imageUid );
    const Image* seedSeg = m_appData.seg( seedSegUid );
    Image* resultSeg = m_appData.seg( resultSegUid );

    if ( ! image )
    {
        spdlog::error( "Invalid image {} to segment", imageUid );
        return false;
    }

    if ( ! seedSeg )
    {
        spdlog::error( "Invalid seed segmentation {} for GridCuts", seedSegUid );
        return false;
    }

    if ( ! resultSeg )
    {
        spdlog::error( "Invalid result segmentation {} for GridCuts", resultSegUid );
        return false;
    }

    spdlog::debug( "Executing GridCuts on image {} with seeds {}", imageUid, seedSegUid );

    const glm::ivec3 pixelDims = static_cast<glm::ivec3>( image->header().pixelDimensions() );

    spdlog::debug( "Start creating grid" );
    auto grid = std::make_unique<Grid>( pixelDims.x, pixelDims.y, pixelDims.z );
    spdlog::debug( "Done creating grid" );

    if ( ! grid )
    {
        spdlog::error( "Null grid" );
        return false;
    }

    /// @todo Speed up filling by accessing raw array

    spdlog::debug( "Start filling grid" );
    for ( int z = 0; z < pixelDims.z; ++z )
    {
        for ( int y = 0; y < pixelDims.y; ++y )
        {
            for ( int x = 0; x < pixelDims.x; ++x )
            {
                const std::optional<int64_t> optSeed = seedSeg->valueAsInt64( 0, x, y, z );
                const int64_t seed = ( optSeed ) ? *optSeed : 0;

                grid->set_terminal_cap( grid->node_id( x, y, z ),
                                        ( seed == 2 ) ? K : 0,
                                        ( seed == 1 ) ? K : 0 );

                if ( x < pixelDims.x - 1 )
                {
                    const auto optValueX0 = image->valueAsDouble( 0, x, y, z );
                    const auto optValueX1 = image->valueAsDouble( 0, x + 1, y, z );

                    const double valueX0 = ( optValueX0 ) ? *optValueX0 : 0.0;
                    const double valueX1 = ( optValueX1 ) ? *optValueX1 : 0.0;

                    const short cap = weight( valueX0 - valueX1 );
                    grid->set_neighbor_cap( grid->node_id( x, y, z ), +1, 0, 0, cap );
                    grid->set_neighbor_cap( grid->node_id( x + 1, y, z ), -1, 0, 0, cap );
                }

                if ( y < pixelDims.y - 1 )
                {
                    const auto optValueY0 = image->valueAsDouble( 0, x, y, z );
                    const auto optValueY1 = image->valueAsDouble( 0, x, y + 1, z );

                    const double valueY0 = ( optValueY0 ) ? *optValueY0 : 0.0;
                    const double valueY1 = ( optValueY1 ) ? *optValueY1 : 0.0;

                    const short cap = weight( valueY0 - valueY1 );
                    grid->set_neighbor_cap( grid->node_id( x, y, z ), 0, +1, 0, cap );
                    grid->set_neighbor_cap( grid->node_id( x, y + 1, z ), 0, -1, 0, cap );
                }

                if ( z < pixelDims.z - 1 )
                {
                    const auto optValueZ0 = image->valueAsDouble( 0, x, y, z );
                    const auto optValueZ1 = image->valueAsDouble( 0, x, y, z + 1 );

                    const double valueZ0 = ( optValueZ0 ) ? *optValueZ0 : 0.0;
                    const double valueZ1 = ( optValueZ1 ) ? *optValueZ1 : 0.0;

                    const short cap = weight( valueZ0 - valueZ1 );
                    grid->set_neighbor_cap( grid->node_id( x, y, z ), 0, 0, +1, cap );
                    grid->set_neighbor_cap( grid->node_id( x, y, z + 1 ), 0, 0, -1, cap );
                }
            }
        }
    }
    spdlog::debug( "Done filling grid" );

    spdlog::debug( "Start computing max flow" );
    auto start = high_resolution_clock::now();
    {
        grid->compute_maxflow();
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>( stop - start );

    spdlog::debug( "Done computing max flow" );
    spdlog::debug( "GridCuts execution time: {} us", duration.count() );

    spdlog::debug( "Start reading back segmentation results" );
    for ( int z = 0; z < pixelDims.z; ++z )
    {
        for ( int y = 0; y < pixelDims.y; ++y )
        {
            for ( int x = 0; x < pixelDims.x; ++x )
            {
                const int64_t seg = static_cast<int64_t>( grid->get_segment( grid->node_id( x, y, z) ) ? 1 : 0 );
                resultSeg->setValue( 0, x, y, z, seg );
            }
        }
    }
    spdlog::debug( "Done reading back segmentation results" );

    const glm::uvec3 dataOffset = glm::uvec3{ 0 };
    const glm::uvec3 dataSize = glm::uvec3{ resultSeg->header().pixelDimensions() };

    spdlog::debug( "Start updating segmentation texture" );

    m_rendering.updateSegTexture(
                resultSegUid, resultSeg->header().memoryComponentType(),
                dataOffset, dataSize,
                resultSeg->bufferAsVoid( 0 ) );

    spdlog::debug( "Done updating segmentation texture" );

    return true;
}


void CallbackHandler::recenterViews(
        const ImageSelection& imageSelection,
        bool recenterCrosshairs,
        bool recenterOnCurrentCrosshairsPos,
        bool resetObliqueOrientation )
{
    /// @todo Add argument to reset zoom or not
    // Option to reset zoom or not:
    static constexpr bool k_doNotResetZoom = false;
    static constexpr bool k_resetZoom = true;

    if ( 0 == m_appData.numImages() )
    {
        spdlog::warn( "No images loaded: preparing views using default bounds" );
    }

    // Compute the AABB that we are recentering views on:
    const auto worldBox = data::computeWorldAABBoxEnclosingImages( m_appData, imageSelection );

    if ( recenterCrosshairs )
    {
        glm::vec3 worldPos = math::computeAABBoxCenter( worldBox );

        if ( const Image* refImg = m_appData.refImage() )
        {
            worldPos = data::roundPointToNearestImageVoxelCenter( *refImg, worldPos );
        }

        m_appData.state().setWorldCrosshairsPos( worldPos );
    }

    if ( recenterOnCurrentCrosshairsPos )
    {
        // Size the views based on the enclosing AABB; position the views based on the curent crosshairs.
        // This is a "soft reset".
        m_appData.windowData().recenterAllViews(
                    m_appData.state().worldCrosshairs().worldOrigin(),
                    k_viewAABBoxScaleFactor * math::computeAABBoxSize( worldBox ),
                    k_doNotResetZoom,
                    resetObliqueOrientation );
    }
    else
    {
        // Size and position the views based on the enclosing AABB. This is a "hard reset".
        m_appData.windowData().recenterAllViews(
                    math::computeAABBoxCenter( worldBox ),
                    k_viewAABBoxScaleFactor * math::computeAABBoxSize( worldBox ),
                    k_resetZoom,
                    resetObliqueOrientation );
    }
}


void CallbackHandler::recenterView(
        const ImageSelection& imageSelection,
        const uuids::uuid& viewUid )
{
    static constexpr bool k_resetZoom = false;
    static constexpr bool k_resetObliqueOrientation = true;

    if ( 0 == m_appData.numImages() )
    {
        spdlog::warn( "No images loaded: recentering view {} using default bounds", viewUid );
    }

    // Size and position the views based on the enclosing AABB of the image selection:
    const auto worldBox = data::computeWorldAABBoxEnclosingImages( m_appData, imageSelection );
    const auto worldBoxSize = math::computeAABBoxSize( worldBox );

    m_appData.windowData().recenterView(
                viewUid,
                m_appData.state().worldCrosshairs().worldOrigin(),
                k_viewAABBoxScaleFactor * worldBoxSize,
                k_resetZoom,
                k_resetObliqueOrientation );
}

void CallbackHandler::doCrosshairsMove(
        const glm::vec2& windowLastPos,
        const glm::vec2& windowCurrPos )
{
    const auto hitData = getViewHit( windowLastPos, windowCurrPos );
    if ( ! hitData ) return;

    m_appData.windowData().setActiveViewUid( hitData->m_viewUid );

    m_appData.state().setWorldCrosshairsPos( glm::vec3{ hitData->m_worldCurrPos } );
}

void CallbackHandler::doCrosshairsScroll(
        const glm::vec2& windowCurrPos,
        const glm::vec2& scrollOffset )
{
    auto hitData = getViewHit( windowCurrPos, windowCurrPos );
    if ( ! hitData ) return;

    // Option 1) scroll based on the reference image only:
//    const Image* image = refImage();
//    if ( ! image ) return;
//    const float scrollDistance = data::sliceScrollDistance( worldFrontAxis, *image );

    // Option 2) scroll based on all visible images in the view. Let's go with this one for now.
    const float scrollDistance = data::sliceScrollDistance(
                m_appData, hitData->m_worldFrontAxis,
                ImageSelection::VisibleImagesInView, &( hitData->m_view ) );

    glm::vec4 worldPos{ m_appData.state().worldCrosshairs().worldOrigin() +
                scrollOffset.y * scrollDistance * hitData->m_worldFrontAxis, 1.0f };

    if ( m_appData.renderData().m_snapCrosshairsToReferenceVoxels )
    {
        if ( const Image* refImg = m_appData.refImage() )
        {
            worldPos = glm::vec4{ data::roundPointToNearestImageVoxelCenter( *refImg, worldPos ), 1.0f };
        }
    }

    m_appData.state().setWorldCrosshairsPos( glm::vec3{ worldPos } );
}

void CallbackHandler::doSegment(
        const glm::vec2& windowLastPos,
        const glm::vec2& windowCurrPos,
        bool leftButton )
{
    static const glm::ivec3 sk_voxelZero{ 0, 0, 0 };

    auto updateSegTexture = [this]
            ( const uuids::uuid& segUid, const Image* seg,
              const glm::uvec3& offset, const glm::uvec3& size, const void* data )
    {
        if ( ! seg ) return;
        m_rendering.updateSegTexture( segUid, seg->header().memoryComponentType(),
                                      offset, size, data );
    };

    auto getSegValue = [] ( const Image* seg, int i, int j, int k ) -> std::optional<int64_t>
    {
        if ( ! seg ) return std::nullopt;
        return seg->valueAsInt64( 0, i, j, k );
    };

    auto setSegValue = [] ( Image* seg, int i, int j, int k, int64_t value )
    {
        if ( ! seg ) return false;
        return seg->setValue( 0, i, j, k, value );
    };


    const auto activeImageUid = m_appData.activeImageUid();
    if ( ! activeImageUid ) return;

    auto hitData = getViewHit( windowLastPos, windowCurrPos );
    if ( ! hitData ) return;

    if ( 0 == std::count( std::begin( hitData->m_view.visibleImages() ),
                          std::end( hitData->m_view.visibleImages() ),
                          *activeImageUid ) )
    {
        // The active image is not visible
        return;
    }

    const auto activeSegUid = m_appData.imageToActiveSegUid( *activeImageUid );
    if ( ! activeSegUid ) return;

    // Do nothing if the active segmentation is null
    Image* activeSeg = m_appData.seg( *activeSegUid );
    if ( ! activeSeg ) return;

    // Gather all synchronized segmentations
    std::unordered_set< uuids::uuid > segUids;
    segUids.insert( *activeSegUid );

    for ( const auto& imageUid : m_appData.imagesBeingSegmented() )
    {
        if ( const auto segUid = m_appData.imageToActiveSegUid( imageUid ) )
        {
            segUids.insert( *segUid );
        }
    }

    // The position is in the view bounds; make this the active view:
    m_appData.windowData().setActiveViewUid( hitData->m_viewUid );

    // Note: when moving crosshairs, the worldPos would be offset at this stage.
    // i.e.: worldPos -= glm::vec4{ offsetDist * worldCameraFront, 0.0f };
    // However, we want to allow the user to segment on any view, regardless of its offset.
    // Therefore, the offset is not applied.

    const int64_t labelToPaint = static_cast<int64_t>(
                ( leftButton ) ? m_appData.settings().foregroundLabel()
                               : m_appData.settings().backgroundLabel() );

    const int64_t labelToReplace = static_cast<int64_t>(
                ( leftButton ) ? m_appData.settings().backgroundLabel()
                               : m_appData.settings().foregroundLabel() );

    const int brushSize = static_cast<int>( m_appData.settings().brushSizeInVoxels() );

    const AppSettings& settings = m_appData.settings();

    // Paint on each segmentation
    for ( const auto& segUid : segUids )
    {
        Image* seg = m_appData.seg( segUid );
        if ( ! seg ) continue;

        const glm::vec3 spacing{ seg->header().spacing() };
        const glm::ivec3 dims{ seg->header().pixelDimensions() };

        const glm::mat4& pixel_T_worldDef = seg->transformations().pixel_T_worldDef();
        const glm::vec4 pixelPos = pixel_T_worldDef * hitData->m_worldCurrPos;
        const glm::vec3 pixelPos3 = pixelPos / pixelPos.w;
        const glm::ivec3 roundedPixelPos = glm::ivec3{ glm::round( pixelPos3 ) };

        if ( glm::any( glm::lessThan( roundedPixelPos, sk_voxelZero ) ) ||
             glm::any( glm::greaterThanEqual( roundedPixelPos, dims ) ) )
        {
            // This pixel is outside the image
            continue;
        }

        // View plane normal vector transformed into Voxel space:
        const glm::vec3 voxelViewPlaneNormal = glm::normalize(
                    glm::inverseTranspose( glm::mat3( pixel_T_worldDef ) ) * (-hitData->m_worldFrontAxis) );

        // View plane equation:
        const glm::vec4 voxelViewPlane = math::makePlane( voxelViewPlaneNormal, pixelPos3 );

        switch ( seg->header().memoryComponentType() )
        {
        case ComponentType::UInt8:
        {
            paint3d< uint8_t >( segUid, seg, dims, spacing,
                                static_cast<uint8_t>( labelToPaint ),
                                static_cast<uint8_t>( labelToReplace ),
                                settings.replaceBackgroundWithForeground(),
                                settings.useRoundBrush(),
                                settings.use3dBrush(),
                                settings.useIsotropicBrush(),
                                brushSize, roundedPixelPos, voxelViewPlane,
                                getSegValue, setSegValue, updateSegTexture );
            break;
        }
        case ComponentType::UInt16:
        {
            paint3d< uint16_t >( segUid, seg, dims, spacing,
                                 static_cast<uint16_t>( labelToPaint ),
                                 static_cast<uint16_t>( labelToReplace ),
                                 settings.replaceBackgroundWithForeground(),
                                 settings.useRoundBrush(),
                                 settings.use3dBrush(),
                                 settings.useIsotropicBrush(),
                                 brushSize, roundedPixelPos, voxelViewPlane,
                                 getSegValue, setSegValue, updateSegTexture );
            break;
        }
        case ComponentType::UInt32:
        {
            paint3d< uint32_t >( segUid, seg, dims, spacing,
                                 static_cast<uint32_t>( labelToPaint ),
                                 static_cast<uint32_t>( labelToReplace ),
                                 settings.replaceBackgroundWithForeground(),
                                 settings.useRoundBrush(),
                                 settings.use3dBrush(),
                                 settings.useIsotropicBrush(),
                                 brushSize, roundedPixelPos, voxelViewPlane,
                                 getSegValue, setSegValue, updateSegTexture );
            break;
        }
        default: continue;
        }
    }
}

void CallbackHandler::doAnnotate(
        const glm::vec2& windowPrevPos,
        const glm::vec2& windowCurrPos )
{
    auto hitData = getViewHit( windowPrevPos, windowCurrPos );
    if ( ! hitData ) return;

    // We need a reference image to compute the offset distance
    const Image* refImg = m_appData.refImage();
    if ( ! refImg ) return;

    // Annotate on the active image
    const auto activeImageUid = m_appData.activeImageUid();
    const Image* activeImage = ( activeImageUid ? m_appData.image( *activeImageUid ) : nullptr );
    if ( ! activeImage ) return;

    const auto activeViewUid = m_appData.windowData().activeViewUid();

    // Ignore if actively annotating and there is an active view not equal to this view:
    /// @todo Create AnnotatingState: { Nothing, PickingPoint, MovingPoint, SelectingPoint }
    if ( m_appData.state().annotating() && activeViewUid )
    {
        if ( activeViewUid != hitData->m_viewUid ) return;
    }

    // The pointer is in the view bounds! Make this the active view:
    m_appData.windowData().setActiveViewUid( hitData->m_viewUid );

    // Apply this view's offset from the crosshairs position in order to calculate the view plane position.
    // The offset is calculated based on the slice scroll distance of the reference image.

    // Compute the equation of the view plane in the space of the active image Subject:
    /// @todo Pull this out into a MathHelper function
    const glm::mat4& subject_T_world = activeImage->transformations().subject_T_worldDef();
    const glm::mat4 subject_T_world_IT = glm::inverseTranspose( subject_T_world );

    const glm::vec4 worldPlaneNormal{ -hitData->m_worldFrontAxis, 0.0f };
    const glm::vec3 subjectPlaneNormal{ subject_T_world_IT * worldPlaneNormal };

    glm::vec4 subjectPlanePoint = subject_T_world * hitData->m_worldCurrPos;
    subjectPlanePoint /= subjectPlanePoint.w;

    const glm::vec4 subjectPlaneEquation = math::makePlane(
                subjectPlaneNormal, glm::vec3{ subjectPlanePoint } );

    // Use the image slice scroll distance as the threshold for plane distances:
    const float planeDistanceThresh = data::sliceScrollDistance( hitData->m_worldFrontAxis, *activeImage );

    /// @todo Create AnnotationGroup, which consists of all annotations for an image that fall
    /// on one of the slices.
    const auto annotUids = data::findAnnotationsForImage(
                m_appData, *activeImageUid,
                subjectPlaneEquation, planeDistanceThresh );

    std::optional<uuids::uuid> annotUid;

    if ( ! annotUids.empty() )
    {
        annotUid = annotUids[0];
    }
    else
    {
        // Create new annotation for this image:
        try
        {
            const std::string name = std::string( "Annotation " ) +
                    std::to_string( m_appData.annotationsForImage( *activeImageUid ).size() );

            annotUid = m_appData.addAnnotation(
                        *activeImageUid,
                        Annotation( name, activeImage->settings().borderColor(),
                                    subjectPlaneEquation ) );

            if ( ! annotUid )
            {
                spdlog::error( "Unable to add new annotation (subject plane: {}) for image {}",
                               glm::to_string( subjectPlaneEquation ), *activeImageUid );
                return;
            }

            m_appData.assignActiveAnnotationUidToImage( *activeImageUid, *annotUid );

            spdlog::debug( "Added new annotation {} (subject plane: {}) for image {}",
                           *annotUid, glm::to_string( subjectPlaneEquation ), *activeImageUid );
        }
        catch ( const std::exception& e )
        {
            spdlog::error( "Unable to create new annotation (subject plane: {}) for image {}: {}",
                           glm::to_string( subjectPlaneEquation ), *activeImageUid, e.what() );
            return;
        }
    }

    Annotation* annot = m_appData.annotation( *annotUid );
    if ( ! annot ) return;

//    spdlog::trace( "Got annotation uid: {}" , *annotUid );

    // Add points to the outer boundary (0) for now:
    /// @todo Ability to select boundary
    static constexpr uint32_t k_outerBoundary = 0;

    const auto projectedPoint = annot->addSubjectPointToBoundary(
                k_outerBoundary, glm::vec3{ subjectPlanePoint } );

    if ( ! projectedPoint )
    {
        spdlog::error( "Unable to add point {} to boundary {}",
                       glm::to_string( hitData->m_worldCurrPos ), k_outerBoundary );
    }
//    else
//    {
//        spdlog::trace( "Projected annotation point {} to plane {}",
//                       glm::to_string( worldPos ), glm::to_string( *projectedPoint ) );
//    }
}

void CallbackHandler::doWindowLevel(
        const glm::vec2& windowLastPos,
        const glm::vec2& windowCurrPos )
{
    auto hitData = getViewHit( windowLastPos, windowCurrPos );
    if ( ! hitData ) return;

    const auto activeImageUid = m_appData.activeImageUid();
    if ( ! activeImageUid ) return;

    Image* activeImage = m_appData.image( *activeImageUid );
    if ( ! activeImage ) return;

    if ( 0 == std::count( std::begin( hitData->m_view.visibleImages() ),
                          std::end( hitData->m_view.visibleImages() ),
                          *activeImageUid ) )
    {
        // The active image is not visible
        return;
    }

    auto& settings = activeImage->settings();

    const float levelMin = static_cast<float>( settings.levelRange().first );
    const float levelMax = static_cast<float>( settings.levelRange().second );
    const float winMin = static_cast<float>( settings.windowRange().first );
    const float winMax = static_cast<float>( settings.windowRange().second );

    const float levelDelta = static_cast<float>( ( levelMax - levelMin ) ) *
            ( hitData->m_windowClipCurrPos.y - hitData->m_windowClipLastPos.y ) / 2.0f;

    const float winDelta = static_cast<float>( ( winMax - winMin ) ) *
            ( hitData->m_windowClipCurrPos.x - hitData->m_windowClipLastPos.x ) / 2.0f;

    const float oldLevel = static_cast<float>( settings.level() );
    const float oldWindow = static_cast<float>( settings.window() );

    const float newLevel = std::min( std::max( oldLevel + levelDelta, levelMin ), levelMax );
    const float newWindow = std::min( std::max( oldWindow + winDelta, winMin ), winMax );

    settings.setLevel( static_cast<double>( newLevel ) );
    settings.setWindow( static_cast<double>( newWindow ) );

    m_rendering.updateImageUniforms( *activeImageUid );
}


void CallbackHandler::doOpacity(
        const glm::vec2& windowLastPos,
        const glm::vec2& windowCurrPos )
{
    static constexpr double sk_opMin = 0.0;
    static constexpr double sk_opMax = 1.0;

    auto hitData = getViewHit( windowLastPos, windowCurrPos );
    if ( ! hitData ) return;

    const auto activeImageUid = m_appData.activeImageUid();
    if ( ! activeImageUid ) return;

    if ( 0 == std::count( std::begin( hitData->m_view.visibleImages() ),
                          std::end( hitData->m_view.visibleImages() ),
                          *activeImageUid ) )
    {
        // The active image is not visible
        return;
    }

    Image* activeImage = m_appData.image( *activeImageUid );
    if ( ! activeImage ) return;

    const double opacityDelta = ( sk_opMax - sk_opMin ) *
            ( hitData->m_windowClipCurrPos.y - hitData->m_windowClipLastPos.y ) / 2.0f;

    const double newOpacity = std::min( std::max( activeImage->settings().opacity() +
                                                  opacityDelta, sk_opMin ), sk_opMax );

    activeImage->settings().setOpacity( newOpacity );

    m_rendering.updateImageUniforms( *activeImageUid );
}

void CallbackHandler::doCameraTranslate2d(
        const glm::vec2& windowLastPos,
        const glm::vec2& windowCurrPos,
        const glm::vec2& windowStartPos )
{
    auto hitData = getViewHit( windowLastPos, windowCurrPos, false, windowStartPos, true );
    if ( ! hitData ) return;

    if ( const auto transGroupUid = hitData->m_view.cameraTranslationSyncGroupUid() )
    {
        for ( const auto& uid : m_appData.windowData().cameraTranslationGroupViewUids( *transGroupUid ) )
        {
            View* syncedView = m_appData.windowData().getCurrentView( uid );
            if ( ! syncedView ) continue;

            // Only synchronize translations if the view camera types are the same:
            if ( syncedView->cameraType() != hitData->m_view.cameraType() ) continue;

            panRelativeToWorldPosition( syncedView->camera(),
                                        glm::vec2{ hitData->m_viewClipLastPos },
                                        glm::vec2{ hitData->m_viewClipCurrPos },
                                        m_appData.state().worldCrosshairs().worldOrigin() );
        }
    }
    else
    {
        panRelativeToWorldPosition( hitData->m_view.camera(),
                                    glm::vec2{ hitData->m_viewClipLastPos },
                                    glm::vec2{ hitData->m_viewClipCurrPos },
                                    m_appData.state().worldCrosshairs().worldOrigin() );
    }
}

void CallbackHandler::doCameraRotate2d(
        const glm::vec2& lastWindowPos,
        const glm::vec2& currWindowPos,
        const glm::vec2& startWindowPos )
{
    auto& windowData = m_appData.windowData();

    // Get view based on start position
    const auto viewUid = windowData.currentViewUidAtCursor( startWindowPos );
    if ( ! viewUid ) return;

    View* view = windowData.getCurrentView( *viewUid );
    if ( ! view ) return;

    // Only allow rotation of oblique views (and later 3D views, too)
    if ( camera::ViewRenderMode::Disabled == view->renderMode() ) return;
    if ( camera::CameraType::Oblique != view->cameraType() ) return;

    const auto& windowVP = windowData.viewport();

    const glm::vec4 winClipLastPos{ camera::windowNdc2d_T_windowPixels( windowVP, lastWindowPos ), view->clipPlaneDepth(), 1 };
    const glm::vec4 winClipCurrPos{ camera::windowNdc2d_T_windowPixels( windowVP, currWindowPos ), view->clipPlaneDepth(), 1 };

    const glm::vec4 viewClipLastPos = view->viewClip_T_windowClip() * winClipLastPos;
    const glm::vec4 viewClipCurrPos = view->viewClip_T_windowClip() * winClipCurrPos;

    const glm::vec2 viewNdcLastPos = glm::vec2{ viewClipLastPos / viewClipLastPos.w };
    const glm::vec2 viewNdcCurrPos = glm::vec2{ viewClipCurrPos / viewClipCurrPos.w };

    /// @todo Different behavior for rotation 3D view types!

    glm::vec4 clipRotationCenterPos = camera::clip_T_world( view->camera() ) *
            glm::vec4{ m_appData.state().worldCrosshairs().worldOrigin(), 1.0f };

    clipRotationCenterPos /= clipRotationCenterPos.w;

    if ( const auto rotGroupUid = view->cameraRotationSyncGroupUid() )
    {
        for ( const auto& uid : windowData.cameraRotationGroupViewUids( *rotGroupUid ) )
        {
            View* syncedView = windowData.getCurrentView( uid );
            if ( ! syncedView ) continue;

            // Only synchronize rotations if the view camera types are the same:
            if ( syncedView->cameraType() != view->cameraType() ) continue;

            // Rotations only synchronize if the view normals are the same:
            const glm::vec3 n1 = camera::worldDirection( syncedView->camera(), Directions::View::Back );
            const glm::vec3 n2 = camera::worldDirection( view->camera(), Directions::View::Back );

            if ( std::abs( glm::dot( n1, n2 ) - 1.0f ) > 1.0e-4f ) continue;

            camera::rotateInPlane( syncedView->camera(), viewNdcLastPos, viewNdcCurrPos,
                                   glm::vec2{ clipRotationCenterPos } );
        }
    }
    else
    {
        camera::rotateInPlane( view->camera(), viewNdcLastPos, viewNdcCurrPos,
                               glm::vec2{ clipRotationCenterPos } );
    }
}

void CallbackHandler::doCameraRotate3d(
        const glm::vec2& windowLastPos,
        const glm::vec2& windowCurrPos,
        const glm::vec2& windowStartPos,
        const std::optional<AxisConstraint>& constraint )
{
    auto& windowData = m_appData.windowData();

    // Get view based on start position
    const auto viewUid = windowData.currentViewUidAtCursor( windowStartPos );
    if ( ! viewUid ) return;

    View* view = windowData.getCurrentView( *viewUid );
    if ( ! view ) return;

    // Only allow rotation of oblique views (and later 3D views, too)
    if ( camera::ViewRenderMode::Disabled == view->renderMode() ) return;
    if ( camera::CameraType::Oblique != view->cameraType() ) return;

    const auto& windowVP = windowData.viewport();

    const glm::vec4 winClipLastPos{ camera::windowNdc2d_T_windowPixels( windowVP, windowLastPos ), view->clipPlaneDepth(), 1 };
    const glm::vec4 winClipCurrPos{ camera::windowNdc2d_T_windowPixels( windowVP, windowCurrPos ), view->clipPlaneDepth(), 1 };

    const glm::vec4 viewClipLastPos = view->viewClip_T_windowClip() * winClipLastPos;
    const glm::vec4 viewClipCurrPos = view->viewClip_T_windowClip() * winClipCurrPos;

    glm::vec2 viewNdcLastPos = glm::vec2{ viewClipLastPos / viewClipLastPos.w };
    glm::vec2 viewNdcCurrPos = glm::vec2{ viewClipCurrPos / viewClipCurrPos.w };

    if ( constraint )
    {
        switch ( *constraint )
        {
        case AxisConstraint::X :
        {
            viewNdcLastPos.x = 0.0f;
            viewNdcCurrPos.x = 0.0f;
            break;
        }
        case AxisConstraint::Y :
        {
            viewNdcLastPos.y = 0.0f;
            viewNdcCurrPos.y = 0.0f;
            break;
        }
        default: break;
        }
    }

    /// @todo Different behavior for rotation 3D view types!

    if ( const auto rotGroupUid = view->cameraRotationSyncGroupUid() )
    {
        for ( const auto& uid : windowData.cameraRotationGroupViewUids( *rotGroupUid ) )
        {
            View* syncedView = windowData.getCurrentView( uid );
            if( ! syncedView ) continue;

            // Only synchronize rotations if the view camera types are the same:
            if ( syncedView->cameraType() != view->cameraType() ) continue;

            // Rotations only synchronize if the view normals are close:
            /// @todo Check angle instead of dot product using acos
            const glm::vec3 n1 = camera::worldDirection( syncedView->camera(), Directions::View::Back );
            const glm::vec3 n2 = camera::worldDirection( view->camera(), Directions::View::Back );
            if ( std::abs( glm::dot( n1, n2 ) - 1.0f ) > 1.0e-2f ) continue;

            camera::rotateAboutWorldPoint( syncedView->camera(), viewNdcLastPos, viewNdcCurrPos,
                                           m_appData.state().worldCrosshairs().worldOrigin() );
        }
    }
    else
    {
        camera::rotateAboutWorldPoint( view->camera(), viewNdcLastPos, viewNdcCurrPos,
                                       m_appData.state().worldCrosshairs().worldOrigin() );
    }
}

void CallbackHandler::doCameraRotate3d(
        const uuids::uuid& viewUid,
        const glm::quat& camera_T_world_rotationDelta )
{
    auto& windowData = m_appData.windowData();

    View* view = windowData.getView( viewUid );
    if ( ! view ) return;

    // Only allow rotation of oblique views (and later 3D views, too)
    if ( camera::ViewRenderMode::Disabled == view->renderMode() ) return;
    if ( camera::CameraType::Oblique != view->cameraType() ) return;

    if ( const auto rotGroupUid = view->cameraRotationSyncGroupUid() )
    {
        for ( const auto& uid : windowData.cameraRotationGroupViewUids( *rotGroupUid ) )
        {
            View* syncedView = windowData.getCurrentView( uid );
            if( ! syncedView ) continue;

            // Only synchronize rotations if the view camera types are the same:
            if ( syncedView->cameraType() != view->cameraType() ) continue;

            // Rotations only synchronize if the view normals are the same:
            const glm::vec3 n1 = camera::worldDirection( syncedView->camera(), Directions::View::Back );
            const glm::vec3 n2 = camera::worldDirection( view->camera(), Directions::View::Back );

            if ( std::abs( glm::dot( n1, n2 ) - 1.0f ) > 1.0e-2f ) continue;

            camera::applyViewRotationAboutWorldPoint(
                        syncedView->camera(),
                        camera_T_world_rotationDelta,
                        m_appData.state().worldCrosshairs().worldOrigin() );
        }
    }
    else
    {
        camera::applyViewRotationAboutWorldPoint(
                    view->camera(),
                    camera_T_world_rotationDelta,
                    m_appData.state().worldCrosshairs().worldOrigin() );
    }
}

void CallbackHandler::doCameraZoomDrag(
        const glm::vec2& windowLastPos,
        const glm::vec2& windowCurrPos,
        const glm::vec2& windowStartPos,
        const ZoomBehavior& zoomBehavior,
        bool syncZoomForAllViews )
{
    static const glm::vec2 sk_ndcCenter{ 0.0f, 0.0f };

    // Get view based on start position
    const auto currViewUid = m_appData.windowData().currentViewUidAtCursor( windowStartPos );
    if ( ! currViewUid ) return;

    View* currView = m_appData.windowData().getCurrentView( *currViewUid );
    if ( ! currView ) return;

    if ( camera::ViewRenderMode::Disabled == currView->renderMode() ) return;

    const auto& windowVP = m_appData.windowData().viewport();

    const glm::vec2 winClipLastPos = camera::windowNdc2d_T_windowPixels( windowVP, windowLastPos );
    const glm::vec2 winClipCurrPos = camera::windowNdc2d_T_windowPixels( windowVP, windowCurrPos );
    const glm::vec4 winClipStartPos{ camera::windowNdc2d_T_windowPixels( windowVP, windowStartPos ), currView->clipPlaneDepth(), 1 };
    const glm::vec4 viewClipStartPos = currView->viewClip_T_windowClip() * winClipStartPos;
    const glm::vec4 worldStartPos = camera::world_T_clip( currView->camera() ) * viewClipStartPos;

    auto getCenterViewClipPos = [this, &zoomBehavior, &worldStartPos]
            ( const View* view ) -> glm::vec2
    {
        glm::vec2 viewClipCenterPos;

        switch ( zoomBehavior )
        {
        case ZoomBehavior::ToCrosshairs:
        {
            viewClipCenterPos = camera::ndc_T_world(
                        view->camera(), m_appData.state().worldCrosshairs().worldOrigin() );
            break;
        }
        case ZoomBehavior::ToStartPosition:
        {
            const glm::vec4 _viewClipStartPos = camera::clip_T_world( view->camera() ) * worldStartPos;
            viewClipCenterPos = glm::vec2{ _viewClipStartPos / _viewClipStartPos.w };
            break;
        }
        case ZoomBehavior::ToViewCenter:
        {
            viewClipCenterPos = sk_ndcCenter;
            break;
        }
        }

        return viewClipCenterPos;
    };

    const float factor = 2.0f * ( winClipCurrPos.y - winClipLastPos.y ) / 2.0f + 1.0f;

    if ( syncZoomForAllViews )
    {
        for ( const auto& viewUid : m_appData.windowData().currentViewUids() )
        {
            if ( View* syncedView = m_appData.windowData().getCurrentView( viewUid ) )
            {
                camera::zoomNdc( syncedView->camera(), factor, getCenterViewClipPos( syncedView ) );
            }
        }
    }
    else
    {
        if ( const auto zoomGroupUid = currView->cameraZoomSyncGroupUid() )
        {
            for ( const auto& viewUid : m_appData.windowData().cameraZoomGroupViewUids( *zoomGroupUid ) )
            {
                if ( View* syncedView = m_appData.windowData().getCurrentView( viewUid ) )
                {
                    camera::zoomNdc( syncedView->camera(), factor, getCenterViewClipPos( syncedView ) );
                }
            }
        }
        else
        {
            camera::zoomNdc( currView->camera(), factor, getCenterViewClipPos( currView ) );
        }
    }
}

void CallbackHandler::doCameraZoomScroll(
        const glm::vec2& scrollOffset,
        const glm::vec2& windowCurrPos,
        const ZoomBehavior& zoomBehavior,
        bool syncZoomForAllViews )
{
    static constexpr float sk_zoomFactor = 0.01f;
    static const glm::vec2 sk_ndcCenter{ 0.0f, 0.0f };

    // Get view based on start position
    const auto currViewUid = m_appData.windowData().currentViewUidAtCursor( windowCurrPos );
    if ( ! currViewUid ) return;

    // Ignore if there is an active view and this is not it
    if ( const auto activeViewUid = m_appData.windowData().activeViewUid() )
    {
        if ( activeViewUid != *currViewUid ) return;
    }

    View* currView = m_appData.windowData().getCurrentView( *currViewUid );
    if ( ! currView ) return;

    if ( camera::ViewRenderMode::Disabled == currView->renderMode() ) return;

    // The pointer is in the view bounds! Make this the active view
    m_appData.windowData().setActiveViewUid( *currViewUid );

    const auto& windowVP = m_appData.windowData().viewport();

    const glm::vec4 winClipCurrPos{ camera::windowNdc2d_T_windowPixels( windowVP, windowCurrPos ), currView->clipPlaneDepth(), 1 };
    const glm::vec4 viewClipCurrPos = currView->viewClip_T_windowClip() * winClipCurrPos;
    const glm::vec4 worldCurrPos = camera::world_T_clip( currView->camera() ) * viewClipCurrPos;

    auto getCenterViewClipPos = [this, &zoomBehavior, &worldCurrPos]
            ( const View* view ) -> glm::vec2
    {
        glm::vec2 viewClipCenterPos;

        switch ( zoomBehavior )
        {
        case ZoomBehavior::ToCrosshairs:
        {
            viewClipCenterPos = camera::ndc_T_world(
                        view->camera(), m_appData.state().worldCrosshairs().worldOrigin() );
            break;
        }
        case ZoomBehavior::ToStartPosition:
        {
            const glm::vec4 _viewClipCurrPos = camera::clip_T_world( view->camera() ) * worldCurrPos;
            viewClipCenterPos = glm::vec2{ _viewClipCurrPos / _viewClipCurrPos.w };
            break;
        }
        case ZoomBehavior::ToViewCenter:
        {
            viewClipCenterPos = sk_ndcCenter;
            break;
        }
        }

        return viewClipCenterPos;
    };

    const float factor = 1.0f + sk_zoomFactor * scrollOffset.y;

    if ( syncZoomForAllViews )
    {
        for ( const auto& viewUid : m_appData.windowData().currentViewUids() )
        {
            if ( View* syncedView = m_appData.windowData().getCurrentView( viewUid ) )
            {
                camera::zoomNdc( syncedView->camera(), factor, getCenterViewClipPos( syncedView ) );
            }
        }
    }
    else
    {
        if ( const auto zoomGroupUid = currView->cameraZoomSyncGroupUid() )
        {
            for ( const auto& viewUid : m_appData.windowData().cameraZoomGroupViewUids( *zoomGroupUid ) )
            {
                if ( View* syncedView = m_appData.windowData().getCurrentView( viewUid ) )
                {
                    camera::zoomNdc( syncedView->camera(), factor, getCenterViewClipPos( syncedView ) );
                }
            }
        }
        else
        {
            camera::zoomNdc( currView->camera(), factor, getCenterViewClipPos( currView ) );
        }
    }
}

void CallbackHandler::doImageTranslate(
        const glm::vec2& lastWindowPos,
        const glm::vec2& currWindowPos,
        const glm::vec2& startWindowPos,
        bool inPlane )
{
    // Get view based on start position
    const auto viewUid = m_appData.windowData().currentViewUidAtCursor( startWindowPos );
    if ( ! viewUid ) return;

    View* view = m_appData.windowData().getCurrentView( *viewUid );
    if ( ! view ) return;
    if ( camera::ViewRenderMode::Disabled == view->renderMode() ) return;

    const auto activeImageUid = m_appData.activeImageUid();
    if ( ! activeImageUid ) return;

    if ( 0 == std::count( std::begin( view->visibleImages() ),
                          std::end( view->visibleImages() ), *activeImageUid ) )
    {
        // The active image is not visible
        return;
    }

    Image* activeImage = m_appData.image( *activeImageUid );
    if ( ! activeImage ) return;

    const auto& windowVP = m_appData.windowData().viewport();

    const glm::vec4 lastWinClipPos{ camera::windowNdc2d_T_windowPixels( windowVP, lastWindowPos ), view->clipPlaneDepth(), 1 };
    const glm::vec4 currWinClipPos{ camera::windowNdc2d_T_windowPixels( windowVP, currWindowPos ), view->clipPlaneDepth(), 1 };

    const glm::vec4 lastViewClipPos = view->viewClip_T_windowClip() * lastWinClipPos;
    const glm::vec4 currViewClipPos = view->viewClip_T_windowClip() * currWinClipPos;

    const glm::vec2 lastViewNdcPos{ lastViewClipPos / lastViewClipPos.w };
    const glm::vec2 currViewNdcPos{ currViewClipPos / currViewClipPos.w };

    glm::vec3 T{ 0.0f, 0.0f, 0.0f };

    if ( inPlane )
    {
        // Translate the image along the view plane
        static const float ndcZ = -1.0f; //camera::ndcZofWorldPoint( view->camera(), imgTx.getWorldSubjectOrigin() );
        T = camera::translationInCameraPlane( view->camera(), lastViewNdcPos, currViewNdcPos, ndcZ );
    }
    else
    {
        // Translate the image in and out of the view plane
        static constexpr float scale = 10.0f;

        const glm::vec3 worldFrontAxis = camera::worldDirection( view->camera(), Directions::View::Front );

        // Translate by an amount proportional to the slice distance of the active image (the one being translated)
        const float scrollDistance = data::sliceScrollDistance( worldFrontAxis, *activeImage );

        T = translationAboutCameraFrontBack(
                    view->camera(), lastViewNdcPos, currViewNdcPos, scale * scrollDistance );
    }

    auto& imgTx = activeImage->transformations();
    imgTx.set_worldDef_T_affine_translation( imgTx.get_worldDef_T_affine_translation() + T );

    // Apply same transformation to the segmentations:
    for ( const auto segUid : m_appData.imageToSegUids( *activeImageUid ) )
    {
        if ( auto* seg = m_appData.seg( segUid ) )
        {
            auto& segTx = seg->transformations();
            segTx.set_worldDef_T_affine_translation( segTx.get_worldDef_T_affine_translation() + T );
        }
    }

    m_rendering.updateImageUniforms( *activeImageUid );
}

void CallbackHandler::doImageRotate(
        const glm::vec2& lastWindowPos,
        const glm::vec2& currWindowPos,
        const glm::vec2& startWindowPos,
        bool inPlane )
{
    // Get view based on start position
    const auto viewUid = m_appData.windowData().currentViewUidAtCursor( startWindowPos );
    if ( ! viewUid ) return;

    View* view = m_appData.windowData().getCurrentView( *viewUid );
    if ( ! view ) return;
    if ( camera::ViewRenderMode::Disabled == view->renderMode() ) return;

    const auto activeImageUid = m_appData.activeImageUid();
    if ( ! activeImageUid ) return;

    if ( 0 == std::count( std::begin( view->visibleImages() ),
                          std::end( view->visibleImages() ), *activeImageUid ) )
    {
        // The active image is not visible
        return;
    }

    Image* activeImage = m_appData.image( *activeImageUid );
    if ( ! activeImage ) return;

    /// @todo Forbid transformation if the view does NOT show the active image [1]

    const auto& windowVP = m_appData.windowData().viewport();

    const glm::vec4 lastWinClipPos{ camera::windowNdc2d_T_windowPixels( windowVP, lastWindowPos ), view->clipPlaneDepth(), 1 };
    const glm::vec4 currWinClipPos{ camera::windowNdc2d_T_windowPixels( windowVP, currWindowPos ), view->clipPlaneDepth(), 1 };

    const glm::vec4 lastViewClipPos = view->viewClip_T_windowClip() * lastWinClipPos;
    const glm::vec4 currViewClipPos = view->viewClip_T_windowClip() * currWinClipPos;

    const glm::vec2 lastViewNdcPos{ lastViewClipPos / lastViewClipPos.w };
    const glm::vec2 currViewNdcPos{ currViewClipPos / currViewClipPos.w };

    // Center of rotation is the crosshairs origin by default:
    const auto p = m_appData.state().worldRotationCenter();

    const glm::vec3 worldRotationCenter =
            ( p ? *p : m_appData.state().worldCrosshairs().worldOrigin() );

    auto& imgTx = activeImage->transformations();

    CoordinateFrame imageFrame( imgTx.get_worldDef_T_affine_translation(),
                                imgTx.get_worldDef_T_affine_rotation() );

    if ( inPlane )
    {
        const glm::vec2 ndcRotationCenter = ndc_T_world( view->camera(), worldRotationCenter );

        const glm::quat R = camera::rotation2dInCameraPlane(
                    view->camera(), lastViewNdcPos, currViewNdcPos, ndcRotationCenter );

        math::rotateFrameAboutWorldPos( imageFrame, R, worldRotationCenter );
    }
    else
    {
        const glm::quat R = rotation3dAboutCameraPlane(
                    view->camera(), lastViewNdcPos, currViewNdcPos );

        math::rotateFrameAboutWorldPos( imageFrame, R, worldRotationCenter );
    }

    imgTx.set_worldDef_T_affine_translation( imageFrame.worldOrigin() );
    imgTx.set_worldDef_T_affine_rotation( imageFrame.world_T_frame_rotation() );

    // Apply same transformation to the segmentations:
    for ( const auto segUid : m_appData.imageToSegUids( *activeImageUid ) )
    {
        if ( auto* seg = m_appData.seg( segUid ) )
        {
            auto& segTx = seg->transformations();
            segTx.set_worldDef_T_affine_translation( imageFrame.worldOrigin() );
            segTx.set_worldDef_T_affine_rotation( imageFrame.world_T_frame_rotation() );
        }
    }

    m_rendering.updateImageUniforms( *activeImageUid );
}

void CallbackHandler::doImageScale(
        const glm::vec2& lastWindowPos,
        const glm::vec2& currWindowPos,
        const glm::vec2& startWindowPos,
        bool constrainIsotropic )
{
    static const glm::vec3 sk_zero( 0.0f, 0.0f, 0.0f );
    static const glm::vec3 sk_minScale( 0.1f );
    static const glm::vec3 sk_maxScale( 10.0f );

    // Get view based on start position
    const auto viewUid = m_appData.windowData().currentViewUidAtCursor( startWindowPos );
    if ( ! viewUid ) return;

    View* view = m_appData.windowData().getCurrentView( *viewUid );
    if ( ! view ) return;
    if ( camera::ViewRenderMode::Disabled == view->renderMode() ) return;

    const auto activeImageUid = m_appData.activeImageUid();
    if ( ! activeImageUid ) return;

    if ( 0 == std::count( std::begin( view->visibleImages() ),
                          std::end( view->visibleImages() ), *activeImageUid ) )
    {
        // The active image is not visible
        return;
    }

    Image* activeImage = m_appData.image( *activeImageUid );
    if ( ! activeImage ) return;

    /// @todo Forbid transformation if the view does NOT show the active image [1]

    const auto& windowVP = m_appData.windowData().viewport();

    const glm::vec4 lastWinClipPos{ camera::windowNdc2d_T_windowPixels( windowVP, lastWindowPos ), view->clipPlaneDepth(), 1 };
    const glm::vec4 currWinClipPos{ camera::windowNdc2d_T_windowPixels( windowVP, currWindowPos ), view->clipPlaneDepth(), 1 };

    const glm::vec4 lastViewClipPos = view->viewClip_T_windowClip() * lastWinClipPos;
    const glm::vec4 currViewClipPos = view->viewClip_T_windowClip() * currWinClipPos;

    glm::vec4 lastWorldPos = camera::world_T_clip( view->camera() ) * lastViewClipPos;
    glm::vec4 currWorldPos = camera::world_T_clip( view->camera() ) * currViewClipPos;

    lastWorldPos /= lastWorldPos.w;
    currWorldPos /= currWorldPos.w;

    auto& imgTx = activeImage->transformations();

    // Center of scale is the subject center:
//    const auto p = m_appData.worldRotationCenter();
//    const glm::vec3 worldScaleCenter = ( p ? *p : m_appData.worldCrosshairs().worldOrigin() );
//    glm::vec4 subjectScaleCenter = tx.subject_T_world() * glm::vec4{ worldScaleCenter, 1.0f };

    glm::vec4 lastSubjectPos = imgTx.subject_T_worldDef() * lastWorldPos;
    glm::vec4 currSubjectPos = imgTx.subject_T_worldDef() * currWorldPos;
    glm::vec4 subjectScaleCenter = imgTx.subject_T_texture() * glm::vec4{ 0.5, 0.5f, 0.5f, 1.0f };

    lastSubjectPos /= lastSubjectPos.w;
    currSubjectPos /= currSubjectPos.w;
    subjectScaleCenter /= subjectScaleCenter.w;

    const glm::vec3 numer = glm::vec3{ currSubjectPos } - glm::vec3{ subjectScaleCenter };
    const glm::vec3 denom = glm::vec3{ lastSubjectPos } - glm::vec3{ subjectScaleCenter };

    if ( glm::any( glm::epsilonEqual( denom, sk_zero, glm::epsilon<float>() ) ) )
    {
        return;
    }

    glm::vec3 scaleDelta = numer / denom;

//    glm::vec3 test = glm::abs( scaleDelta - glm::vec3{ 1.0f } );

//    if ( test.x >= test.y && test.x >= test.z )
//    {
//        scaleDelta = glm::vec3{ scaleDelta.x, 1.0f, 1.0f };
//    }
//    else if ( test.y >= test.x && test.y >= test.z )
//    {
//        scaleDelta = glm::vec3{ 1.0f, scaleDelta.y, 1.0f };
//    }
//    else if ( test.z >= test.x && test.z >= test.y )
//    {
//        scaleDelta = glm::vec3{ 1.0f, 1.0f, scaleDelta.z };
//    }

    if ( constrainIsotropic )
    {
        float minScale = glm::compMin( scaleDelta );
        float maxScale = glm::compMax( scaleDelta );

        if ( maxScale > 1.0f ) {
            scaleDelta = glm::vec3( maxScale );
        }
        else {
            scaleDelta = glm::vec3( minScale );
        }
    }

    // To prevent flipping and making the slide too small:
    if ( glm::any( glm::lessThan( scaleDelta, sk_minScale ) ) ||
         glm::any( glm::greaterThan( scaleDelta, sk_maxScale ) ) )
    {
        return;
    }

    imgTx.set_worldDef_T_affine_scale( scaleDelta * imgTx.get_worldDef_T_affine_scale() );

    // Apply same transformation to the segmentations:
    for ( const auto segUid : m_appData.imageToSegUids( *activeImageUid ) )
    {
        if ( auto* seg = m_appData.seg( segUid ) )
        {
            auto& segTx = seg->transformations();
            segTx.set_worldDef_T_affine_scale( scaleDelta * segTx.get_worldDef_T_affine_scale() );
        }
    }

    m_rendering.updateImageUniforms( *activeImageUid );
}

void CallbackHandler::flipImageInterpolation()
{
    const auto imgUid = m_appData.activeImageUid();
    if ( ! imgUid ) return;

    Image* image = m_appData.image( *imgUid );
    if ( ! image ) return;

    const InterpolationMode newMode =
            ( InterpolationMode::NearestNeighbor == image->settings().interpolationMode() )
            ? InterpolationMode::Linear
            : InterpolationMode::NearestNeighbor;

    image->settings().setInterpolationMode( newMode );

    m_rendering.updateImageInterpolation( *imgUid );
}

void CallbackHandler::toggleImageVisibility()
{
    const auto imageUid = m_appData.activeImageUid();
    if ( ! imageUid ) return;

    Image* image = m_appData.image( *imageUid );
    if ( ! image ) return;

    image->settings().setVisibility( ! image->settings().visibility() );

    m_rendering.updateImageUniforms( *imageUid );
}

void CallbackHandler::toggleImageEdges()
{
    const auto imageUid = m_appData.activeImageUid();
    if ( ! imageUid ) return;

    Image* image = m_appData.image( *imageUid );
    if ( ! image ) return;

    image->settings().setShowEdges( ! image->settings().showEdges() );

    m_rendering.updateImageUniforms( *imageUid );
}

void CallbackHandler::decreaseSegOpacity()
{
    const auto imgUid = m_appData.activeImageUid();
    if ( ! imgUid ) return;

    const auto segUid = m_appData.imageToActiveSegUid( *imgUid );
    if ( ! segUid ) return;

    Image* seg = m_appData.seg( *segUid );

    const double op = seg->settings().opacity();
    seg->settings().setOpacity( std::max( op - 0.05, 0.0 ) );

    m_rendering.updateImageUniforms( *imgUid );
}

void CallbackHandler::toggleSegVisibility()
{
    const auto imgUid = m_appData.activeImageUid();
    if ( ! imgUid ) return;

    const auto segUid = m_appData.imageToActiveSegUid( *imgUid );
    if ( ! segUid ) return;

    Image* seg = m_appData.seg( *segUid );

    const bool vis = seg->settings().visibility();
    seg->settings().setVisibility( ! vis );

    m_rendering.updateImageUniforms( *imgUid );
}

void CallbackHandler::increaseSegOpacity()
{
    const auto imgUid = m_appData.activeImageUid();
    if ( ! imgUid ) return;

    const auto segUid = m_appData.imageToActiveSegUid( *imgUid );
    if ( ! segUid ) return;

    Image* seg = m_appData.seg( *segUid );

    const double op = seg->settings().opacity();
    seg->settings().setOpacity( std::min( op + 0.05, 1.0 ) );

    m_rendering.updateImageUniforms( *imgUid );
}


void CallbackHandler::cyclePrevLayout()
{
    m_appData.windowData().cycleCurrentLayout( -1 );
}

void CallbackHandler::cycleNextLayout()
{
    m_appData.windowData().cycleCurrentLayout( 1 );
}

void CallbackHandler::cycleOverlayAndUiVisibility()
{
    static int toggle = 0;

    if ( 0 == ( toggle % 2 ) )
    {
        m_appData.guiData().m_renderUiWindows = ( ! m_appData.guiData().m_renderUiWindows );
    }
    else if ( 1 == ( toggle % 2 ) )
    {
        setShowOverlays( ! showOverlays() );
    }

    toggle = ( toggle + 1 ) % 4;
}

void CallbackHandler::cycleImageComponent( int i )
{
    const auto imageUid = m_appData.activeImageUid();
    if ( ! imageUid ) return;

    Image* image = m_appData.image( *imageUid );
    if ( ! image ) return;

    const int N = static_cast<int>( image->settings().numComponents() );
    const int c = static_cast<int>( image->settings().activeComponent() );

    image->settings().setActiveComponent( static_cast<uint32_t>( ( N + c + i ) % N ) );
}


/// @todo Put into DataHelper
void CallbackHandler::cycleForegroundSegLabel( int i )
{
    constexpr int64_t k_minLabel = 0;

    int64_t label = static_cast<int64_t>( m_appData.settings().foregroundLabel() );
    label = std::max( label + i, k_minLabel );

    if ( const auto* table = m_appData.activeLabelTable() )
    {
        m_appData.settings().setForegroundLabel( static_cast<size_t>( label ), *table );
    }
}

/// @todo Put into DataHelper
void CallbackHandler::cycleBackgroundSegLabel( int i )
{
    constexpr int64_t k_minLabel = 0;

    int64_t label = static_cast<int64_t>( m_appData.settings().backgroundLabel() );
    label = std::max( label + i, k_minLabel );

    if ( const auto* table = m_appData.activeLabelTable() )
    {
        m_appData.settings().setBackgroundLabel( static_cast<size_t>( label ), *table );
    }
}

/// @todo Put into DataHelper
void CallbackHandler::cycleBrushSize( int i )
{
    constexpr int k_minBrushVox = 1;
    constexpr int k_maxBrushVox = 101;

    int brushSizeVox = static_cast<int>( m_appData.settings().brushSizeInVoxels() );
    brushSizeVox = std::min( std::max( brushSizeVox + i, k_minBrushVox ), k_maxBrushVox );
    m_appData.settings().setBrushSizeInVoxels( static_cast<uint32_t>( brushSizeVox ) );
}

bool CallbackHandler::showOverlays() const
{
    return m_appData.settings().overlays();
}

/// @todo Put into DataHelper
void CallbackHandler::setShowOverlays( bool show )
{
    m_appData.settings().setOverlays( show ); // this holds the data

    m_rendering.setShowVectorOverlays( show );
    m_appData.guiData().m_renderUiOverlays = show;
}

/// @todo Put into DataHelper
void CallbackHandler::scrollViewSlice( const glm::vec2& currWindowPos, int numSlices )
{
    const auto viewUid = m_appData.windowData().currentViewUidAtCursor( currWindowPos );
    if ( ! viewUid ) return;

    const View* view = m_appData.windowData().getCurrentView( *viewUid );
    if ( ! view ) return;
    if ( camera::ViewRenderMode::Disabled == view->renderMode() ) return;

    const glm::vec3 worldFrontAxis = camera::worldDirection( view->camera(), Directions::View::Front );

    // option 1) scroll based on the reference image
//    const Image* image = refImage();
//    if ( ! image ) return;
//    const float scrollDistance = data::sliceScrollDistance( worldFrontAxis, *image );

    // option 2) scroll based on all visible images in the view
    const float scrollDistance = data::sliceScrollDistance(
                m_appData, worldFrontAxis, ImageSelection::VisibleImagesInView, view );

    const glm::vec3 worldCrosshairs = m_appData.state().worldCrosshairs().worldOrigin();

    m_appData.state().setWorldCrosshairsPos(
                worldCrosshairs + static_cast<float>( numSlices ) * scrollDistance * worldFrontAxis );
}

void CallbackHandler::moveCrosshairsOnViewSlice( const glm::vec2& currWindowPos, int stepX, int stepY )
{
    data::moveCrosshairsOnViewSlice( m_appData, currWindowPos, stepX, stepY );
}

void CallbackHandler::setMouseMode( MouseMode mode )
{
    m_appData.state().setMouseMode( mode );
//    return m_glfw.cursor( mode );
}

void CallbackHandler::toggleFullScreenMode( bool forceWindowMode )
{
    m_glfw.toggleFullScreenMode( forceWindowMode );
}

bool CallbackHandler::setLockManualImageTransformation(
        const uuids::uuid& imageUid,
        bool locked )
{
    Image* image = m_appData.image( imageUid );
    if ( ! image ) return false;

    image->transformations().set_worldDef_T_affine_locked( locked );

    // Lock/unlock all of the image's segmentations:
    for ( const auto segUid : m_appData.imageToSegUids( imageUid ) )
    {
        if ( auto* seg = m_appData.seg( segUid ) )
        {
            seg->transformations().set_worldDef_T_affine_locked( locked );
        }
    }

    return true;
}

bool CallbackHandler::syncManualImageTransformationOnSegs( const uuids::uuid& imageUid )
{
    Image* image = m_appData.image( imageUid );
    if ( ! image ) return false;

    for ( const auto segUid : m_appData.imageToSegUids( imageUid ) )
    {
        if ( auto* seg = m_appData.seg( segUid ) )
        {
            seg->transformations().set_worldDef_T_affine_locked(
                        image->transformations().is_worldDef_T_affine_locked() );

            seg->transformations().set_worldDef_T_affine_scale(
                        image->transformations().get_worldDef_T_affine_scale() );

            seg->transformations().set_worldDef_T_affine_rotation(
                        image->transformations().get_worldDef_T_affine_rotation() );

            seg->transformations().set_worldDef_T_affine_translation(
                        image->transformations().get_worldDef_T_affine_translation() );
        }
    }

    return true;
}

std::optional<CallbackHandler::ViewHitData>
CallbackHandler::getViewHit(
        const glm::vec2& windowPixelLastPos,
        const glm::vec2& windowPixelCurrPos,
        bool requireViewToBeActive,
        const std::optional<glm::vec2> windowPixelStartPos,
        bool hitBasedOnStartPos )
{
    const glm::vec2 pos = ( hitBasedOnStartPos && windowPixelStartPos )
            ? *windowPixelStartPos
            : windowPixelCurrPos;

    const auto viewUid = m_appData.windowData().currentViewUidAtCursor( pos );
    if ( ! viewUid ) return std::nullopt;

    // Ignore if there is an active view and this is not it
    if ( const auto activeViewUid = m_appData.windowData().activeViewUid() )
    {
        if ( requireViewToBeActive && ( activeViewUid != *viewUid ) )
        {
            return std::nullopt;
        }
    }

    View* view = m_appData.windowData().getCurrentView( *viewUid );
    if ( ! view ) return std::nullopt;

    if ( camera::ViewRenderMode::Disabled == view->renderMode() ) return std::nullopt;

    const glm::vec4 windowClipLastPos(
                camera::windowNdc2d_T_windowPixels(
                    m_appData.windowData().viewport(), windowPixelLastPos ),
                view->clipPlaneDepth(), 1.0f );

    const glm::vec4 windowClipCurrPos(
                camera::windowNdc2d_T_windowPixels(
                    m_appData.windowData().viewport(), windowPixelCurrPos ),
                view->clipPlaneDepth(), 1.0f );

    ViewHitData hitData( *view, *viewUid );

    hitData.m_windowClipLastPos = windowClipLastPos;
    hitData.m_windowClipCurrPos = windowClipCurrPos;

    hitData.m_viewClipLastPos = view->viewClip_T_windowClip() * windowClipLastPos;
    hitData.m_viewClipCurrPos = view->viewClip_T_windowClip() * windowClipCurrPos;

    hitData.m_viewClipLastPos /= hitData.m_viewClipLastPos.w;
    hitData.m_viewClipCurrPos /= hitData.m_viewClipCurrPos.w;

    hitData.m_worldFrontAxis = camera::worldDirection( view->camera(), Directions::View::Front );

    if ( ! hitBasedOnStartPos && glm::any( glm::greaterThan( glm::abs( glm::vec2{ hitData.m_viewClipCurrPos } ), sk_maxClip ) ) )
    {
        return std::nullopt;
    }

    // Apply this view's offset from the crosshairs position in order to calculate the
    // view plane position:
    const float offsetDist = data::computeViewOffsetDistance(
                m_appData, view->offsetSetting(), hitData.m_worldFrontAxis );

    glm::vec4 worldPos = camera::world_T_clip( view->camera() ) * hitData.m_viewClipCurrPos;
    worldPos = worldPos / worldPos.w;

    worldPos -= glm::vec4{ offsetDist * hitData.m_worldFrontAxis, 0 };

    if ( m_appData.renderData().m_snapCrosshairsToReferenceVoxels )
    {
        if ( const Image* refImg = m_appData.refImage() )
        {
            worldPos = glm::vec4{ data::roundPointToNearestImageVoxelCenter( *refImg, worldPos ), 1.0f };
        }
    }

    hitData.m_worldCurrPos = worldPos;

    return hitData;
}
