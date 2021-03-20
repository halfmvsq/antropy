#include "windowing/View.h"

#include "common/DataHelper.h"
#include "common/Exception.hpp"

#include "image/Image.h"

#include "logic/app/Data.h"
#include "logic/camera/CameraHelpers.h"
#include "logic/camera/CameraStartFrameType.h"
#include "logic/camera/OrthogonalProjection.h"
#include "logic/camera/PerspectiveProjection.h"
#include "logic/camera/MathUtility.h"

#include "rendering/utility/UnderlyingEnumType.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <array>
#include <unordered_map>


namespace
{

static const glm::vec3 sk_origin{ 0.0f };

// Map from view camera type to projection type
static const std::unordered_map< camera::CameraType, camera::ProjectionType >
smk_cameraTypeToProjectionTypeMap =
{
    { camera::CameraType::Axial, camera::ProjectionType::Orthographic },
    { camera::CameraType::Coronal, camera::ProjectionType::Orthographic },
    { camera::CameraType::Sagittal, camera::ProjectionType::Orthographic },
    { camera::CameraType::ThreeD, camera::ProjectionType::Orthographic },
    { camera::CameraType::Oblique, camera::ProjectionType::Orthographic }
};

// Map from view camera type to start frame type
static const std::unordered_map< camera::CameraType, CameraStartFrameType >
smk_cameraTypeToDefaultStartFrameTypeMap =
{
    { camera::CameraType::Axial, CameraStartFrameType::Crosshairs_Axial_LAI },
    { camera::CameraType::Coronal, CameraStartFrameType::Crosshairs_Coronal_LSA },
    { camera::CameraType::Sagittal, CameraStartFrameType::Crosshairs_Sagittal_PSL },
    { camera::CameraType::ThreeD, CameraStartFrameType::Crosshairs_Coronal_LSA },
    { camera::CameraType::Oblique, CameraStartFrameType::Crosshairs_Axial_LAI }
};

// Map from start frame type to rotation matrix.
// This rotation matrix maps camera Start frame to World space.
static const std::unordered_map< CameraStartFrameType, glm::quat >
smk_cameraStartFrameTypeToDefaultAnatomicalRotationMap =
{
    { CameraStartFrameType::Crosshairs_Axial_LAI, glm::quat_cast( glm::mat3{ 1,0,0, 0,-1,0, 0,0,-1 } ) },
    { CameraStartFrameType::Crosshairs_Axial_RAS, glm::quat_cast( glm::mat3{ -1,0,0, 0,-1,0, 0,0,1 } ) },
    { CameraStartFrameType::Crosshairs_Coronal_LSA, glm::quat_cast( glm::mat3{ 1,0,0, 0,0,1, 0,-1,0 } ) },
    { CameraStartFrameType::Crosshairs_Coronal_RSP, glm::quat_cast( glm::mat3{ -1,0,0, 0,0,1, 0,1,0 } ) },
    { CameraStartFrameType::Crosshairs_Sagittal_PSL, glm::quat_cast( glm::mat3{ 0,1,0, 0,0,1, 1,0,0 } ) },
    { CameraStartFrameType::Crosshairs_Sagittal_ASR, glm::quat_cast( glm::mat3{ 0,-1,0, 0,0,1, -1,0,0 } ) },
};

} // anonymous


View::View( Viewport winClipViewport,
            int32_t numOffsets,
            camera::CameraType cameraType,
            camera::ViewRenderMode shaderType,
            UiControls uiControls,
            std::optional<uuids::uuid> cameraRotationSyncGroupUid,
            std::optional<uuids::uuid> cameraTranslationSyncGroup,
            std::optional<uuids::uuid> cameraZoomSyncGroup )
    :
      m_winClipViewport( std::move( winClipViewport ) ),
      m_winClip_T_viewClip( camera::compute_windowClip_T_viewClip( m_winClipViewport.getAsVec4() ) ),
      m_viewClip_T_winClip( glm::inverse( m_winClip_T_viewClip ) ),

      m_numOffsets( numOffsets ),

      m_renderedImageUids(),
      m_metricImageUids(),

      // Render the first two images by default:
      m_preferredDefaultRenderedImages( { 0, 1 } ),

      m_shaderType( shaderType ),
      m_cameraType( cameraType ),
      m_projectionType( smk_cameraTypeToProjectionTypeMap.at( m_cameraType ) ),
      m_camera( m_projectionType ),

      m_uiControls( std::move( uiControls ) ),

      m_cameraRotationSyncGroupUid( cameraRotationSyncGroupUid ),
      m_cameraTranslationSyncGroupUid( cameraTranslationSyncGroup ),
      m_cameraZoomSyncGroupUid( cameraZoomSyncGroup ),

      m_clipPlaneDepth( 0.0f ),
      m_winMouseViewMinMaxCorners( { {0, 0}, {0, 0} } ),

      m_sliceIntersector()
{
    const auto& startFrameType = smk_cameraTypeToDefaultStartFrameTypeMap.at( m_cameraType );

    m_camera.set_anatomy_T_start_provider(
        [&startFrameType] ()
        {
            return CoordinateFrame( sk_origin, smk_cameraStartFrameTypeToDefaultAnatomicalRotationMap.at( startFrameType ) );
        } );

    // Configure the slice intersector:
    m_sliceIntersector.setPositioningMethod( SliceIntersector::PositioningMethod::FrameOrigin, std::nullopt );
    m_sliceIntersector.setAlignmentMethod( SliceIntersector::AlignmentMethod::CameraZ );
}


bool View::updateImageSlice( const AppData& appData, const glm::vec3& worldCrosshairs )
{
    const glm::vec3 worldCameraOrigin = camera::worldOrigin( m_camera );
    const glm::vec3 worldCameraFront = camera::worldDirection( m_camera, Directions::View::Front );

    // Compute the depth of the view plane in camera Clip space, because it is needed for the
    // coordinates of the quad that is textured with the image.

    // Apply this view's offset from the crosshairs position in order to calculate the view plane position.
    const float offsetDist = static_cast<float>( m_numOffsets ) *
            data::sliceScrollDistance( appData, worldCameraFront, ImageSelection::ReferenceImage, this );

    const glm::vec3 worldPlanePos = worldCrosshairs + offsetDist * worldCameraFront;
    const glm::vec4 worldViewPlane = math::makePlane( -worldCameraFront, worldPlanePos );

    // Compute the World-space distance between the camera origin and the view plane
    float worldCameraToPlaneDistance;

    if ( math::vectorPlaneIntersection( worldCameraOrigin, worldCameraFront, worldViewPlane, worldCameraToPlaneDistance ) )
    {
        // Push camera back from its target on the view plane by a distance equal to 10% of the view frustum depth,
        // so that it doesn't clip the image quad vertices
        static constexpr float sk_pushBackFraction = 0.10f;
        const float eyeToTargetOffset = sk_pushBackFraction * ( m_camera.farDistance() - m_camera.nearDistance() );
        camera::setWorldTarget( m_camera, worldCameraOrigin + worldCameraToPlaneDistance * worldCameraFront, eyeToTargetOffset );
    }
    else
    {
        spdlog::warn( "Camera (front direction = {}) is parallel with the view (plane = {})",
                      glm::to_string( worldCameraFront ), glm::to_string( worldViewPlane ) );

        return false;
    }

    const glm::vec4 clipPlanePos = camera::clip_T_world( m_camera ) * glm::vec4{ worldPlanePos, 1.0f };
    m_clipPlaneDepth = clipPlanePos.z / clipPlanePos.w;

    return true;
}


std::optional< SliceIntersector::IntersectionVerticesVec4 >
View::computeImageSliceIntersection(
        const Image* image,
        const CoordinateFrame& crosshairs ) const
{
    if ( ! image ) return std::nullopt;

    SliceIntersector intersector;
    intersector.setPositioningMethod( SliceIntersector::PositioningMethod::FrameOrigin, std::nullopt );
    intersector.setAlignmentMethod( SliceIntersector::AlignmentMethod::CameraZ );

    // Compute the intersections in Pixel space by transforming the camera and crosshairs frame
    // from World to Pixel space. Pixel space is needed, because the corners form an AABB in that space.
    const glm::mat4 world_T_pixel = image->transformations().worldDef_T_subject() *
            image->transformations().subject_T_pixel();

    const glm::mat4 pixel_T_world = glm::inverse( world_T_pixel );

    std::optional< SliceIntersector::IntersectionVertices > pixelIntersectionPositions =
            intersector.computePlaneIntersections(
                pixel_T_world * m_camera.world_T_camera(),
                pixel_T_world * crosshairs.world_T_frame(),
                image->header().pixelBBoxCorners() ).first;

    if ( ! pixelIntersectionPositions )
    {
        // No slice intersection to render
        return std::nullopt;
    }

    // Convert Subject intersection positions to World space
    SliceIntersector::IntersectionVerticesVec4 worldIntersectionPositions;

    for ( uint32_t i = 0; i < SliceIntersector::s_numVertices; ++i )
    {
        worldIntersectionPositions[i] = world_T_pixel *
                glm::vec4{ (*pixelIntersectionPositions)[i], 1.0f };
    }

    return worldIntersectionPositions;
}


void View::setWinMouseMinMaxCoords( std::pair< glm::vec2, glm::vec2 > corners )
{
    m_winMouseViewMinMaxCorners = std::move( corners );
}

const std::pair< glm::vec2, glm::vec2 >& View::winMouseMinMaxCoords() const
{
    return m_winMouseViewMinMaxCorners;
}

void View::setCameraType( const camera::CameraType& newCameraType )
{
    if ( newCameraType == m_cameraType )
    {
        return;
    }

    const auto newProjType = smk_cameraTypeToProjectionTypeMap.at( newCameraType );

    if ( m_projectionType != newProjType )
    {
        std::shared_ptr<camera::Projection> projection;

        switch ( m_projectionType )
        {
        case camera::ProjectionType::Orthographic:
        {
            projection = std::make_shared<camera::OrthographicProjection>();
            break;
        }
        case camera::ProjectionType::Perspective:
        {
            projection = std::make_shared<camera::PerspectiveProjection>();
            break;
        }
        }

        m_camera.setProjection( std::move( projection ) );
    }

    CoordinateFrame anatomy_T_start;

    if ( camera::CameraType::Oblique == newCameraType )
    {
        // Transitioning to an Oblique view type from an Orthogonal view type:
        // The new anatomy_T_start frame is set to the (old) Orthogonal view type's anatomy_T_start frame.
        const auto& startFrameType = smk_cameraTypeToDefaultStartFrameTypeMap.at( m_cameraType );
        anatomy_T_start = CoordinateFrame( sk_origin, smk_cameraStartFrameTypeToDefaultAnatomicalRotationMap.at( startFrameType ) );
    }
    else
    {
        // Transitioning to an Orthogonal view type:
        const auto& startFrameType = smk_cameraTypeToDefaultStartFrameTypeMap.at( newCameraType );
        anatomy_T_start = CoordinateFrame( sk_origin, smk_cameraStartFrameTypeToDefaultAnatomicalRotationMap.at( startFrameType ) );

        if ( camera::CameraType::Oblique == m_cameraType )
        {
            // Transitioning to an Orthogonal view type from an Oblique view type.
            // Reset the manually applied view transformations, because view might have rotations applied.
            camera::resetViewTransformation( m_camera );
        }
    }

    m_camera.set_anatomy_T_start_provider( [anatomy_T_start] () { return anatomy_T_start; } );

    m_cameraType = newCameraType;
}

void View::setRenderMode( const camera::ViewRenderMode& shaderType )
{
    m_shaderType = shaderType;
}

bool View::isImageRendered( const AppData& appData, size_t index )
{
    auto imageUid = appData.imageUid( index );
    if ( ! imageUid ) return false; // invalid image index

    auto it = std::find( std::begin( m_renderedImageUids ), std::end( m_renderedImageUids ), *imageUid );
    return ( std::end( m_renderedImageUids ) != it );
}

void View::setImageRendered( const AppData& appData, size_t index, bool visible )
{
    auto imageUid = appData.imageUid( index );
    if ( ! imageUid ) return; // invalid image index

    if ( ! visible )
    {
        m_renderedImageUids.remove( *imageUid );
        return;
    }

    if ( std::end( m_renderedImageUids ) !=
         std::find( std::begin( m_renderedImageUids ), std::end( m_renderedImageUids ), *imageUid ) )
    {
        return; // image already exists, so do nothing
    }

    bool inserted = false;

    for ( auto it = std::begin( m_renderedImageUids ); std::end( m_renderedImageUids ) != it; ++it )
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
}

const std::list<uuids::uuid>& View::renderedImages() const
{
    return m_renderedImageUids;
}

void View::setRenderedImages( const std::list<uuids::uuid>& imageUids, bool filterByDefaults )
{
    if ( filterByDefaults )
    {
        m_renderedImageUids.clear();
        size_t index = 0;

        for ( const auto& imageUid : imageUids )
        {
            if ( m_preferredDefaultRenderedImages.count( index ) > 0 )
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
}

bool View::isImageUsedForMetric( const AppData& appData, size_t index )
{
    auto imageUid = appData.imageUid( index );
    if ( ! imageUid ) return false; // invalid image index

    auto it = std::find( std::begin( m_metricImageUids ), std::end( m_metricImageUids ), *imageUid );
    return ( std::end( m_metricImageUids ) != it );
}

void View::setImageUsedForMetric( const AppData& appData, size_t index, bool visible )
{
    static constexpr size_t MAX_IMAGES = 2;

    auto imageUid = appData.imageUid( index );
    if ( ! imageUid ) return; // invalid image index

    if ( ! visible )
    {
        m_metricImageUids.remove( *imageUid );
        return;
    }

    if ( std::end( m_metricImageUids ) !=
         std::find( std::begin( m_metricImageUids ), std::end( m_metricImageUids ), *imageUid ) )
    {
        return; // image already exists, so do nothing
    }

    if ( m_metricImageUids.size() >= MAX_IMAGES )
    {
        // If trying to add another image UID to list with 2 or more UIDs,
        // remove the last UID to make room
        m_metricImageUids.erase( std::prev( std::end( m_metricImageUids ) ) );
    }

    bool inserted = false;

    for ( auto it = std::begin( m_metricImageUids ); std::end( m_metricImageUids ) != it; ++it )
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
}

const std::list<uuids::uuid>& View::metricImages() const
{
    return m_metricImageUids;
}

void View::setMetricImages( const std::list<uuids::uuid>& imageUids )
{
    m_metricImageUids = imageUids;
}

const std::list<uuids::uuid>& View::visibleImages() const
{
    static const std::list<uuids::uuid> sk_noImages;

    if ( camera::ViewRenderMode::Image == m_shaderType )
    {
        return renderedImages();
    }
    else if ( camera::ViewRenderMode::Disabled == m_shaderType )
    {
        return sk_noImages;
    }
    else
    {
        return metricImages();
    }
}

void View::setPreferredDefaultRenderedImages( std::set<size_t> imageIndices )
{
    m_preferredDefaultRenderedImages = std::move( imageIndices );
}

const std::set<size_t>& View::preferredDefaultRenderedImages() const
{
    return m_preferredDefaultRenderedImages;
}

void View::updateImageOrdering( uuid_range_t orderedImageUids )
{
    std::list<uuids::uuid> newRenderedImageUids;
    std::list<uuids::uuid> newMetricImageUids;

    // Loop through the images in new order:
    for ( const auto& imageUid : orderedImageUids )
    {
        auto it1 = std::find( std::begin( m_renderedImageUids ), std::end( m_renderedImageUids ), imageUid );

        if ( std::end( m_renderedImageUids ) != it1 )
        {
            // This image is rendered, so place in new order:
            newRenderedImageUids.push_back( imageUid );
        }

        auto it2 = std::find( std::begin( m_metricImageUids ), std::end( m_metricImageUids ), imageUid );

        if ( std::end( m_metricImageUids ) != it2 &&
             newMetricImageUids.size() < 2 )
        {
            // This image is in metric computation, so place in new order:
            newMetricImageUids.push_back( imageUid );
        }
    }

    m_renderedImageUids = newRenderedImageUids;
    m_metricImageUids = newMetricImageUids;
}

std::optional<uuids::uuid> View::cameraRotationSyncGroupUid() const { return m_cameraRotationSyncGroupUid; }
std::optional<uuids::uuid> View::cameraTranslationSyncGroupUid() const { return m_cameraTranslationSyncGroupUid; }
std::optional<uuids::uuid> View::cameraZoomSyncGroupUid() const { return m_cameraZoomSyncGroupUid; }

const UiControls& View::uiControls() const { return m_uiControls; }

const camera::Camera& View::camera() const { return m_camera; }
camera::Camera& View::camera() { return m_camera; }

camera::CameraType View::cameraType() const { return m_cameraType; }
camera::ViewRenderMode View::renderMode() const { return m_shaderType; }

const Viewport& View::winClipViewport() const { return m_winClipViewport; }
float View::clipPlaneDepth() const { return m_clipPlaneDepth; }

int32_t View::numOffsets() const { return m_numOffsets; }

const glm::mat4& View::winClip_T_viewClip() const { return m_winClip_T_viewClip; }
const glm::mat4& View::viewClip_T_winClip() const { return m_viewClip_T_winClip; }
