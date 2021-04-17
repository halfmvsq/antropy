#ifndef SEG_UTILITY_H
#define SEG_UTILITY_H

#include <uuid.h>

#include <glm/fwd.hpp>

#include <functional>
#include <optional>

class Image;


void paintSegmentation(
        const uuids::uuid& segUid,
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

        const std::function< void ( const uuids::uuid& segUid, const Image* seg,
                                    const glm::uvec3& offset, const glm::uvec3& size,
                                    const int64_t* data ) >& updateSegTexture );

#endif // SEG_UTILITY_H
