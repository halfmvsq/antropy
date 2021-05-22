#ifndef SEG_UTILITY_H
#define SEG_UTILITY_H

#include "common/Types.h"

#include <uuid.h>

#include <glm/fwd.hpp>

#include <functional>
#include <optional>

class Image;


/**
 * @brief paintSegmentation
 * @param seg
 * @param segDims
 * @param segSpacing
 * @param labelToPaint
 * @param labelToReplace
 * @param brushReplacesBgWithFg
 * @param brushIsRound
 * @param brushIs3d
 * @param brushIsIsotropic
 * @param brushSizeInVoxels
 * @param roundedPixelPos
 * @param voxelViewPlane
 * @param updateSegTexture
 */
void paintSegmentation(
        Image* seg,
        const glm::ivec3& segDims,
        const glm::vec3& segSpacing,

        int64_t labelToPaint,
        int64_t labelToReplace,

        bool brushReplacesBgWithFg,
        bool brushIsRound,
        bool brushIs3d,
        bool brushIsIsotropic,
        int brushSizeInVoxels,

        const glm::ivec3& roundedPixelPos,
        const glm::vec4& voxelViewPlane,

        const std::function< void (
            const ComponentType& memoryComponentType, const glm::uvec3& offset,
            const glm::uvec3& size, const int64_t* data ) >& updateSegTexture );

#endif // SEG_UTILITY_H
