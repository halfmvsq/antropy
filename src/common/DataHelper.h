#ifndef DATA_HELPER_H
#define DATA_HELPER_H

#include "windowing/View.h"

#include "common/AABB.h"
#include "common/CoordinateFrame.h"
#include "common/Types.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <uuid.h>

#include <array>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>


class AppData;
class Image;

namespace camera
{
class Camera;
}


/**
 * This is an aggregation of free functions for helping out with application data.
 */
namespace data
{

std::vector<uuids::uuid> selectImages(
        const AppData& data,
        const ImageSelection& selection,
        const View* view );

/**
 * @brief Compute the distance by which to scroll the view plane with each "tick" of the
 * mouse scroll wheel or track pad. The distance is based on the minimum voxel spacing
 * of a given set of images along the view camera's direction in World space.
 *
 * @param worldCameraFront Normalized front direction of the camera in World space.
 */
float sliceScrollDistance(
        const AppData&,
        const glm::vec3& worldCameraFrontDir,
        const ImageSelection&, const View* view );

float sliceScrollDistance(
        const glm::vec3& worldCameraFrontDir,
        const Image& );

glm::vec2 sliceMoveDistance(
        const AppData&,
        const glm::vec3& worldCameraRightDir,
        const glm::vec3& worldCameraUpDir,
        const ImageSelection&,
        const View* view );


/**
 * @brief Compute the enclosing World-space AABB of the given image selection
 * @return AABB in World-space coordinates
 */
AABB<float> computeWorldAABBoxEnclosingImages(
        const AppData& appData,
        const ImageSelection& imageSelection );


std::optional< uuids::uuid >
createLabelColorTableForSegmentation(
        AppData& appData,
        const uuids::uuid& segUid );

std::optional<glm::ivec3>
getImageVoxelCoordsAtCrosshairs(
        const AppData& appData,
        size_t imageIndex );

void moveCrosshairsOnViewSlice(
        AppData& appData,
        const glm::vec2& currWindowPos,
        int stepX, int stepY );

std::optional< uuids::uuid > findAnnotationForImage(
        const AppData& appData,
        const uuids::uuid& imageUid,
        const glm::vec4& planeEquation,
        float planeDistanceThresh );

} // namespace data

#endif // DATA_HELPER_H
