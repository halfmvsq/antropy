#include "common/DataHelper.h"

#include "image/Image.h"
#include "image/ImageUtility.h"

#include "logic/app/Data.h"
#include "logic/camera/Camera.h"
#include "logic/camera/CameraHelpers.h"
#include "logic/camera/MathUtility.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/vector_angle.hpp>

#include <limits>


namespace
{

// The default voxel scale is 1.0 units
static constexpr float sk_defaultRefSpaceVoxelScale = 1.0f;
static constexpr float sk_defaultSliceScrollDistance = sk_defaultRefSpaceVoxelScale;
static constexpr float sk_defaultSliceMoveDistance = sk_defaultRefSpaceVoxelScale;

} // anonymous


namespace data
{

std::vector<uuids::uuid> selectImages(
        const AppData& data,
        const ImageSelection& selection,
        const View* view )
{
    std::vector<uuids::uuid> imageUids;

    switch ( selection )
    {
    case ImageSelection::ReferenceImage:
    {
        if ( const auto refUid = data.refImageUid() )
        {
            imageUids.push_back( *refUid );
        }
        break;
    }
    case ImageSelection::ActiveImage:
    {
        if ( const auto actUid = data.activeImageUid() )
        {
            imageUids.push_back( *actUid );
        }
        break;
    }
    case ImageSelection::ReferenceAndActiveImages:
    {
        if ( const auto refUid = data.refImageUid() )
        {
            imageUids.push_back( *refUid );
        }

        if ( const auto actUid = data.activeImageUid() )
        {
            imageUids.push_back( *actUid );
        }

        break;
    }
    case ImageSelection::VisibleImagesInView:
    {
        if ( view )
        {
            for ( const auto& viewUid : view->visibleImages() )
            {
                imageUids.push_back( viewUid );
            }
        }
        break;
    }
    case ImageSelection::FixedImageInView:
    {
        if ( view && ! view->metricImages().empty() )
        {
            // The first image is the fixed one:
            imageUids.push_back( view->metricImages().front()  );
        }
        break;
    }
    case ImageSelection::MovingImageInView:
    {
        if ( view )
        {
            int index = 0;
            for ( const auto& viewUid : view->metricImages() )
            {
                if ( 1 == index )
                {
                    // The second image!
                    imageUids.push_back( viewUid );
                    break;
                }
                ++index;
            }
        }
        break;
    }
    case ImageSelection::FixedAndMovingImagesInView:
    {
        if ( view )
        {
            int index = 0;
            for ( const auto& viewUid : view->metricImages() )
            {
                if ( 0 == index || 1 == index )
                {
                    // The first and second images!
                    imageUids.push_back( viewUid );
                }
                else
                {
                    break;
                }
                ++index;
            }
        }
        break;
    }
    case ImageSelection::AllLoadedImages:
    {
        for ( const auto& imageUid : data.imageUidsOrdered() )
        {
            imageUids.push_back( imageUid );
        }
        break;
    }
    }

    return imageUids;
}


float sliceScrollDistance(
        const AppData& data,
        const glm::vec3& worldCameraFrontDir,
        const ImageSelection& imageSelection,
        const View* view )
{
    if ( 0 == data.numImages() )
    {
        return sk_defaultSliceScrollDistance;
    }

    float distance = std::numeric_limits<float>::max();

    size_t numImagesUsed = 0;

    for ( const auto& imageUid : selectImages( data, imageSelection, view ) )
    {
        const Image* image = data.image( imageUid );
        if ( ! image ) continue;

        // Scroll in image Pixel space along the camera's front direction:
        const glm::mat3 pixel_T_world_IT{ image->transformations().pixel_T_worldDef_invTransp() };
        const glm::vec3 pixelDir = glm::abs( glm::normalize( pixel_T_world_IT * worldCameraFrontDir ) );

        // Scroll distance is proportional to spacing of image along the view direction
        float d = std::abs( glm::dot( glm::vec3{ image->header().spacing() }, pixelDir ) );
        distance = std::min( distance, d );

        ++numImagesUsed;
    }

    if ( 0 == numImagesUsed )
    {
        return sk_defaultSliceScrollDistance;
    }

    return distance;
}


float sliceScrollDistance(
        const glm::vec3& worldCameraFrontDir,
        const Image& image )
{
    // Scroll in image Pixel space along the camera's front direction:
    const glm::mat3 pixel_T_world_IT{ image.transformations().pixel_T_worldDef_invTransp() };
    const glm::vec3 pixelDir = glm::abs( glm::normalize( pixel_T_world_IT * worldCameraFrontDir ) );

    // Scroll distance is proportional to spacing of image along the view direction
    return  std::abs( glm::dot( glm::vec3{ image.header().spacing() }, pixelDir ) );
}


glm::vec2 sliceMoveDistance(
        const AppData& data,
        const glm::vec3& worldCameraRightDir,
        const glm::vec3& worldCameraUpDir,
        const ImageSelection& imageSelection,
        const View* view )
{
    if ( 0 == data.numImages() )
    {
        return { sk_defaultSliceMoveDistance, sk_defaultSliceMoveDistance };
    }

    glm::vec2 distances{ std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };

    size_t numImagesUsed = 0;

    for ( const auto& imageUid : selectImages( data, imageSelection, view ) )
    {
        const Image* image = data.image( imageUid );
        if ( ! image ) continue;

        // Scroll in image Pixel space along the camera's front direction:
        const glm::mat3 pixel_T_world_IT{ image->transformations().pixel_T_worldDef_invTransp() };
        const glm::vec3 pixelRightDir = glm::abs( glm::normalize( pixel_T_world_IT * worldCameraRightDir ) );
        const glm::vec3 pixelUpDir = glm::abs( glm::normalize( pixel_T_world_IT * worldCameraUpDir ) );

        // Move distance is proportional to spacing of image along the view directions
        float distRight = std::abs( glm::dot( glm::vec3{ image->header().spacing() }, pixelRightDir ) );
        float distUp = std::abs( glm::dot( glm::vec3{ image->header().spacing() }, pixelUpDir ) );
        distances = glm::min( distances, glm::vec2{ distRight, distUp } );

        ++numImagesUsed;
    }

    if ( 0 == numImagesUsed )
    {
        return { sk_defaultSliceMoveDistance, sk_defaultSliceMoveDistance };
    }

    return distances;
}


AABB<float> computeWorldAABBoxEnclosingImages(
        const AppData& data,
        const ImageSelection& imageSelection )
{
    static const AABB<float> sk_defaultBox{ {-1, -1, -1}, {1, 1, 1} };

    std::vector< glm::vec3 > corners;
    bool anyImagesUsed = false;

    for ( const auto& imageUid : selectImages( data, imageSelection, nullptr ) )
    {
        const auto* img = data.image( imageUid );
        if ( ! img ) continue;

        anyImagesUsed = true;

        const auto world_T_subject = img->transformations().worldDef_T_subject();
        const auto subjectCorners = img->header().subjectAABboxMinMaxCorners();

        glm::vec4 corner1 = world_T_subject * glm::vec4{ subjectCorners.first, 1.0f };
        glm::vec4 corner2 = world_T_subject * glm::vec4{ subjectCorners.second, 1.0f };

        corners.push_back( glm::vec3{ corner1 / corner1.w } );
        corners.push_back( glm::vec3{ corner2 / corner2.w } );
    }

    if ( ! anyImagesUsed )
    {
        return sk_defaultBox;
    }

    return math::computeAABBox<float>( corners );
}


std::optional< uuids::uuid >
createLabelColorTableForSegmentation(
        AppData& appData,
        const uuids::uuid& segUid )
{
    Image* seg = appData.seg( segUid );

    if ( ! seg )
    {
        spdlog::error( "Cannot create label color table for invalid segmentation {}", segUid );
        return std::nullopt;
    }

    // What's the largest label value in this segmentation?
    const int64_t maxLabel = static_cast<int64_t>( seg->settings().componentStatistics().m_maximum );
    spdlog::debug( "Maximum label value in segmentation {} is {}", segUid, maxLabel );

    // What's the largest value supported by the segmentation image component type?
    const auto minMaxCompValues = componentRange( seg->header().memoryComponentType() );
    const size_t maxNumLabels = static_cast<size_t>( minMaxCompValues.second - minMaxCompValues.first + 1 );

    spdlog::debug( "Maximum label value supported by the component type ({}) of segmentation {} is {}",
                   seg->header().memoryComponentTypeAsString(), segUid, minMaxCompValues.second );

    // Allocate color table with 256 labels, so that it fits into a 1 byte segmentation image
    constexpr int64_t k_numLabels = 256;

    if ( maxLabel > k_numLabels - 1 )
    {
        spdlog::warn( "A color table is being allocated with {} labels, which is fewer than "
                      "the number required to represent the maximum label ({}) in segmentation {}",
                      k_numLabels, maxLabel, segUid );
    }

    if ( maxNumLabels > k_numLabels )
    {
        spdlog::info( "A color table is being allocated with {} labels, which is fewer than "
                      "the number of labels ({}) that can be represented by the pixel component type "
                      "({}) of segmentation {}",
                      k_numLabels, maxNumLabels, seg->header().memoryComponentTypeAsString(), segUid );
    }

    const size_t newTableIndex = appData.addLabelColorTable( k_numLabels, maxNumLabels );
    seg->settings().setLabelTableIndex( newTableIndex );

    spdlog::info( "Create new label color table (index {}) for segmentation {}", newTableIndex, segUid );

    return appData.labelTableUid( newTableIndex );
}


std::optional<glm::ivec3>
getImageVoxelCoordsAtCrosshairs(
        const AppData& appData,
        size_t imageIndex )
{
    const auto imageUid = appData.imageUid( imageIndex );
    const Image* image = imageUid ? appData.image( *imageUid ) : nullptr;

    if ( ! image ) return std::nullopt;

    const glm::vec4 pixelPos = image->transformations().pixel_T_worldDef() *
            glm::vec4{ appData.state().worldCrosshairs().worldOrigin(), 1 };

    const glm::ivec3 roundedPixelPos = glm::ivec3{ glm::round( pixelPos / pixelPos.w ) };

    if ( glm::any( glm::lessThan( roundedPixelPos, glm::ivec3{ 0, 0, 0 } ) ) ||
         glm::any( glm::greaterThanEqual( roundedPixelPos, glm::ivec3{ image->header().pixelDimensions() } ) ) )
    {
        return std::nullopt;
    }

    return roundedPixelPos;
}


void moveCrosshairsOnViewSlice(
        AppData& appData,
        const glm::vec2& currWindowPos,
        int stepX, int stepY )
{
    const auto viewUid = appData.windowData().currentViewUidAtCursor( currWindowPos );
    if ( ! viewUid ) return;

    const View* view = appData.windowData().getCurrentView( *viewUid );
    if ( ! view ) return;
    if ( camera::ViewRenderMode::Disabled == view->renderMode() ) return;

    const glm::vec3 worldRightAxis = camera::worldDirection( view->camera(), Directions::View::Right );
    const glm::vec3 worldUpAxis = camera::worldDirection( view->camera(), Directions::View::Up );

    const glm::vec2 moveDistances = data::sliceMoveDistance(
                appData, worldRightAxis, worldUpAxis, ImageSelection::VisibleImagesInView, view );

    const glm::vec3 worldCrosshairs = appData.state().worldCrosshairs().worldOrigin();

    appData.state().setWorldCrosshairsPos(
                worldCrosshairs +
                static_cast<float>( stepX ) * moveDistances.x * worldRightAxis +
                static_cast<float>( stepY ) * moveDistances.y * worldUpAxis );
}


std::optional< uuids::uuid > findAnnotationForImage(
        const AppData& appData,
        const uuids::uuid& imageUid,
        const glm::vec4& planeEquation,
        float planeDistanceThresh )
{
    constexpr float k_normalAngleThresh = 1.0e-4f;

    for ( const auto& annotUid : appData.annotationsForImage( imageUid ) )
    {
        const Annotation* annot = appData.annotation( annotUid );
        if ( ! annot ) continue;

        const glm::vec4 P = annot->getSubjectPlaneEquation();

        // Compare angle between normal vectors and distances between plane offsets:
        const glm::vec3 n1 = glm::normalize( glm::vec3{ P } );
        const glm::vec3 n2 = glm::normalize( glm::vec3{ planeEquation } );

        const float d1 = P[3];
        const float d2 = planeEquation[3];

        if ( ( std::abs( glm::angle( n1, n2 ) ) < k_normalAngleThresh ) &&
             std::abs( d1 - d2 ) < planeDistanceThresh )
        {
            return annotUid;
        }
    }

    return std::nullopt;
}

} // namespace data
