#include "windowing/View.h"

#include "common/DataHelper.h"
#include "common/Exception.hpp"

#include "image/Image.h"

#include "logic/app/Data.h"
#include "logic/camera/CameraHelpers.h"
#include "logic/camera/CameraStartFrameType.h"
#include "logic/camera/MathUtility.h"
#include "logic/camera/OrthogonalProjection.h"
#include "logic/camera/PerspectiveProjection.h"

#include "rendering/utility/math/SliceIntersector.h"
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


View::View( glm::vec4 winClipViewport,
            ViewOffsetSetting offsetSetting,
            camera::CameraType cameraType,
            camera::ViewRenderMode renderMode,
            UiControls uiControls,
            std::optional<uuids::uuid> cameraRotationSyncGroupUid,
            std::optional<uuids::uuid> cameraTranslationSyncGroup,
            std::optional<uuids::uuid> cameraZoomSyncGroup )
    :
      ControlFrame( winClipViewport, cameraType, renderMode, uiControls ),

      m_offset( std::move( offsetSetting ) ),

      m_projectionType( smk_cameraTypeToProjectionTypeMap.at( m_cameraType ) ),
      m_camera( m_projectionType ),

      m_cameraRotationSyncGroupUid( cameraRotationSyncGroupUid ),
      m_cameraTranslationSyncGroupUid( cameraTranslationSyncGroup ),
      m_cameraZoomSyncGroupUid( cameraZoomSyncGroup ),

      m_clipPlaneDepth( 0.0f )
{
    const auto& startFrameType = smk_cameraTypeToDefaultStartFrameTypeMap.at( m_cameraType );

    m_camera.set_anatomy_T_start_provider(
        [&startFrameType] ()
        {
            return CoordinateFrame( sk_origin, smk_cameraStartFrameTypeToDefaultAnatomicalRotationMap.at( startFrameType ) );
        } );
}


bool View::updateImageSlice( const AppData& appData, const glm::vec3& worldCrosshairs )
{
    static constexpr size_t k_maxNumWarnings = 10;
    static size_t warnCount = 0;

    const glm::vec3 worldCameraOrigin = camera::worldOrigin( m_camera );
    const glm::vec3 worldCameraFront = camera::worldDirection( m_camera, Directions::View::Front );

    // Compute the depth of the view plane in camera Clip space, because it is needed for the
    // coordinates of the quad that is textured with the image.

    // Apply this view's offset from the crosshairs position in order to calculate the view plane position.
    const float offsetDist = data::computeViewOffsetDistance( appData, m_offset, worldCameraFront );
    const glm::vec3 worldPlanePos = worldCrosshairs + offsetDist * worldCameraFront;
    const glm::vec4 worldViewPlane = math::makePlane( -worldCameraFront, worldPlanePos );

    // Compute the World-space distance between the camera origin and the view plane
    float worldCameraToPlaneDistance;

    if ( math::vectorPlaneIntersection(
             worldCameraOrigin, worldCameraFront,
             worldViewPlane, worldCameraToPlaneDistance ) )
    {
        // Push camera back from its target on the view plane by a distance equal to
        // 10% of the view frustum depth, so that it doesn't clip the image quad vertices:
        static constexpr float sk_pushBackFraction = 0.10f;

        const float eyeToTargetOffset = sk_pushBackFraction *
                ( m_camera.farDistance() - m_camera.nearDistance() );

        camera::setWorldTarget(
                    m_camera,
                    worldCameraOrigin + worldCameraToPlaneDistance * worldCameraFront,
                    eyeToTargetOffset );

        warnCount = 0; // Reset warning counter
    }
    else
    {
        if ( warnCount++ < k_maxNumWarnings )
        {
            spdlog::warn( "Camera (front direction = {}) is parallel with the view (plane = {})",
                          glm::to_string( worldCameraFront ), glm::to_string( worldViewPlane ) );
        }
        else if ( k_maxNumWarnings == warnCount )
        {
            spdlog::warn( "Halting warning about camera front direction." );
        }

        return false;
    }

    const glm::vec4 clipPlanePos =
            camera::clip_T_world( m_camera ) *
            glm::vec4{ worldPlanePos, 1.0f };

    m_clipPlaneDepth = clipPlanePos.z / clipPlanePos.w;

    return true;
}


std::optional< intersection::IntersectionVerticesVec4 >
View::computeImageSliceIntersection(
        const Image* image,
        const CoordinateFrame& crosshairs ) const
{
    if ( ! image ) return std::nullopt;

    // Compute the intersections in Pixel space by transforming the camera and crosshairs frame
    // from World to Pixel space. Pixel space is needed, because the corners form an AABB in that space.
    const glm::mat4 world_T_pixel = image->transformations().worldDef_T_subject() *
            image->transformations().subject_T_pixel();

    const glm::mat4 pixel_T_world = glm::inverse( world_T_pixel );

    // Object for intersecting the view plane with the 3D images
    SliceIntersector sliceIntersector;
    sliceIntersector.setPositioningMethod( intersection::PositioningMethod::FrameOrigin, std::nullopt );
    sliceIntersector.setAlignmentMethod( intersection::AlignmentMethod::CameraZ );

    std::optional< intersection::IntersectionVertices > pixelIntersectionPositions =
            sliceIntersector.computePlaneIntersections(
                pixel_T_world * m_camera.world_T_camera(),
                pixel_T_world * crosshairs.world_T_frame(),
                image->header().pixelBBoxCorners() ).first;

    if ( ! pixelIntersectionPositions )
    {
        // No slice intersection to render
        return std::nullopt;
    }

    // Convert Subject intersection positions to World space
    intersection::IntersectionVerticesVec4 worldIntersectionPositions;

    for ( uint32_t i = 0; i < SliceIntersector::s_numVertices; ++i )
    {
        worldIntersectionPositions[i] = world_T_pixel *
                glm::vec4{ (*pixelIntersectionPositions)[i], 1.0f };
    }

    return worldIntersectionPositions;
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

std::optional<uuids::uuid>
View::cameraRotationSyncGroupUid() const { return m_cameraRotationSyncGroupUid; }

std::optional<uuids::uuid>
View::cameraTranslationSyncGroupUid() const { return m_cameraTranslationSyncGroupUid; }

std::optional<uuids::uuid>
View::cameraZoomSyncGroupUid() const { return m_cameraZoomSyncGroupUid; }

float View::clipPlaneDepth() const { return m_clipPlaneDepth; }

const ViewOffsetSetting& View::offsetSetting() const { return m_offset; }

const camera::Camera& View::camera() const { return m_camera; }
camera::Camera& View::camera() { return m_camera; }
