#ifndef SEG_UTILITY_TPP
#define SEG_UTILITY_TPP

#include "image/Image.h"

/// @note Only used for testAABBoxPlaneIntersection
/// @todo Move this function to a math function helper in common
#include "logic/camera/MathUtility.h"

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/string_cast.hpp>

#include <uuid.h>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <functional>
#include <optional>
#include <queue>
#include <unordered_set>
#include <vector>


template< typename SegComponentType >
void paint3d(
        const uuids::uuid& segUid,
        Image* seg,
        const glm::ivec3& segDims,
        const glm::vec3& segSpacing,
        SegComponentType labelToPaint,
        SegComponentType labelToReplace,
        bool brushReplacesBgWithFg,
        bool brushIsRound,
        bool brushIs3d,
        bool brushIsIsotropic,
        int brushSizeInVoxels,
        const glm::ivec3& roundedPixelPos,
        const glm::vec4& voxelViewPlane,

        const std::function< std::optional<int64_t> ( const Image* seg, int i, int j, int k ) >& getSegValue,

        const std::function< bool ( Image* seg, int i, int j, int k, int64_t value ) >& setSegValue,

        const std::function< void ( const uuids::uuid& segUid, const Image* seg,
                                    const glm::uvec3& offset, const glm::uvec3& size, const void* data ) >& updateSegTexture )
{
    static const glm::ivec3 sk_voxelOne{ 1, 1, 1 };

    // Does the voxel intersect the view plane?
    auto voxelInterectsViewPlane = [&voxelViewPlane] ( const glm::vec3& voxelPos )
    {
        static const glm::vec3 cornerOffset{ 0.5f, 0.5f, 0.5f };
        return math::testAABBoxPlaneIntersection( voxelPos, voxelPos + cornerOffset, voxelViewPlane );
    };

    // The brush radius (not including the central voxel).
    // So, a single voxel brush has radius zero, a width-3 voxel brush has radius 1,
    // a width-5 voxel brush has radius 2, etc.
    // Radius = (brush width - 1) / 2
    const int radius = brushSizeInVoxels - 1;
    const float radius_f = static_cast<float>( radius );

    // Set of unique voxels to change:
    std::unordered_set< glm::ivec3 > voxelToChange;

    // Min/max corners of the set of voxels to change
    glm::ivec3 maxVoxel{ std::numeric_limits<int>::lowest() };
    glm::ivec3 minVoxel{ std::numeric_limits<int>::max() };

    // Coefficients that convert mm to voxel spacing:
    std::array<float, 3> b{ 1.0f, 1.0f, 1.0f };

    // Integer versions of the mm to voxel coefficients:
    std::array<int, 3> a{ 1, 1, 1 };

    if ( brushIsIsotropic )
    {
        // Compute factors that account for anisotropic spacing:
        constexpr bool k_isotropicAlongMaxSpacingAxis = false;

        const float spacing = ( k_isotropicAlongMaxSpacingAxis )
                ? glm::compMax( segSpacing )
                : glm::compMin( segSpacing );

        for ( uint32_t i = 0; i < 3; ++i )
        {
            b[i] = spacing / segSpacing[static_cast<int>(i)];
            a[i] = std::max( static_cast<int>( std::ceil( b[i] ) ), 1 );
        }
    }


    if ( brushIs3d )
    {
        for ( int k = -a[2] * radius; k <= a[2] * radius; ++k )
        {
            const int K = roundedPixelPos.z + k;
            if ( K < 0 || K >= segDims.z ) continue;

            for ( int j = -a[1] * radius; j <= a[1] * radius; ++j )
            {
                const int J = roundedPixelPos.y + j;
                if ( J < 0 || J >= segDims.y ) continue;

                for ( int i = -a[0] * radius; i <= a[0] * radius; ++i )
                {
                    const int I = roundedPixelPos.x + i;
                    if ( I < 0 || I >= segDims.x ) continue;

                    const glm::ivec3 p{ I, J, K };
                    bool changed = false;

                    const float ii = static_cast<float>( i );
                    const float jj = static_cast<float>( j );
                    const float kk = static_cast<float>( k );

                    if ( ! brushIsRound )
                    {
                        // Square brush
                        if ( std::max( std::max( std::abs( ii / b[0] ), std::abs( jj / b[1] ) ),
                                       std::abs( kk / b[2] ) ) <= radius_f )
                        {
                            voxelToChange.insert( p );
                            changed = true;
                        }
                    }
                    else
                    {
                        // Round brush: additional check that voxel is inside the sphere of radius N
                        if ( ( ii * ii / (b[0]*b[0]) + jj * jj / (b[1]*b[1]) + kk * kk / (b[2]*b[2]) )
                             <= radius_f * radius_f )
                        {
                            voxelToChange.insert( p );
                            changed = true;
                        }
                    }

                    if ( changed )
                    {
                        minVoxel = glm::min( minVoxel, p );
                        maxVoxel = glm::max( maxVoxel, p );
                    }
                }
            }
        }
    }
    else
    {
        // Check if the voxel is inside the segmentation
        auto isVoxelInSeg = [&segDims] ( const glm::ivec3& voxelPos )
        {
            return ( voxelPos.x >= 0 && voxelPos.x < segDims.x &&
                     voxelPos.y >= 0 && voxelPos.y < segDims.y &&
                     voxelPos.z >= 0 && voxelPos.z < segDims.z );
        };

        // Queue of voxels to test for intersection with view plane
        std::queue< glm::ivec3 > voxelsToTest;

        // Set of voxels that have been sent off for testing
        std::unordered_set< glm::ivec3 > voxelsProcessed;

        // Set of voxels that intersect with view plane and that should be painted:
        std::unordered_set< glm::ivec3 > voxelsToPaint;

        // Set of voxels that do not intersect with view plane or that should not be painted:
        std::unordered_set< glm::ivec3 > voxelsToIgnore;

        // Insert the first voxel as a voxel to test, if it's inside the segmentation.
        // This voxel should intersect the view plane, since it was clicked by the mouse,
        // but test it to make sure.
        if ( isVoxelInSeg( roundedPixelPos ) &&
             voxelInterectsViewPlane( roundedPixelPos ) )
        {
            voxelsToTest.push( roundedPixelPos );
            voxelsProcessed.insert( roundedPixelPos );
        }

        std::array< glm::ivec3, 6 > neighbors;

        // Loop over all voxels in the test queue
        while ( ! voxelsToTest.empty() )
        {
            const glm::ivec3 q = voxelsToTest.front();
            voxelsToTest.pop();

            // Check if the voxel is inside the brush:
            const glm::vec3 d = q - roundedPixelPos;

            if ( ! brushIsRound )
            {
                // Equation for a rectangle:
                if ( std::max( std::max( std::abs( d.x / b[0] ), std::abs( d.y / b[1] ) ),
                               std::abs( d.z / b[2] ) ) > radius_f )
                {
                    voxelsToIgnore.insert( q );
                    continue;
                }
            }
            else
            {
                if ( ( d.x * d.x / (b[0] * b[0]) + d.y * d.y / (b[1] * b[1]) + d.z * d.z / (b[2] * b[2]) )
                     > radius_f * radius_f )
                {
                    voxelsToIgnore.insert( q );
                    continue;
                }
            }

            // Mark that voxel intersects the view plane and is inside the brush:
            voxelsToPaint.insert( q );

            // Test its six neighbors, too:
            neighbors.fill( q );

            neighbors[0].x -= 1;
            neighbors[1].x += 1;
            neighbors[2].y -= 1;
            neighbors[3].y += 1;
            neighbors[4].z -= 1;
            neighbors[5].z += 1;

            for ( const auto& n : neighbors )
            {
                if ( 0 == voxelsProcessed.count( n ) &&
                     0 == voxelsToIgnore.count( n ) &&
                     0 == voxelsToPaint.count( n ) &&
                     voxelInterectsViewPlane( n ) &&
                     isVoxelInSeg( n ) )
                {
                    voxelsToTest.push( n );
                    voxelsProcessed.insert( n );
                }
                else
                {
                    voxelsToIgnore.insert( n );
                }
            }
        }

        // Create a unique set of voxels to change:
        for ( const auto& p : voxelsToPaint )
        {
            voxelToChange.insert( p );

            minVoxel = glm::min( minVoxel, p );
            maxVoxel = glm::max( maxVoxel, p );
        }
    }


    // Create a rectangular block of contiguous voxel value data that will be set in the texture:
    std::vector< glm::ivec3 > voxelPositions;
    std::vector< SegComponentType > voxelValues;

    for ( int k = minVoxel.z; k <= maxVoxel.z; ++k )
    {
        for ( int j = minVoxel.y; j <= maxVoxel.y; ++j )
        {
            for ( int i = minVoxel.x; i <= maxVoxel.x; ++i )
            {
                const glm::ivec3 p{ i, j, k };
                voxelPositions.emplace_back( p );

                if ( voxelToChange.count( p ) > 0 )
                {
                    // Marked to change, so paint it:

                    if ( brushReplacesBgWithFg )
                    {
                        const SegComponentType currentLabel = static_cast<SegComponentType>(
                                    getSegValue( seg, i, j, k ).value_or( 0 ) );

                        if ( labelToReplace == currentLabel )
                        {
                            voxelValues.emplace_back( labelToPaint );
                        }
                        else
                        {
                            voxelValues.emplace_back( currentLabel );
                        }
                    }
                    else
                    {
                        voxelValues.emplace_back( labelToPaint );
                    }
                }
                else
                {
                    // Not marked to change, so replace with the current label:
                    const SegComponentType currentLabel = static_cast<SegComponentType>(
                                getSegValue( seg, i, j, k ).value_or( 0 ) );

                    voxelValues.emplace_back( currentLabel );
                }
            }
        }
    }

    if ( voxelPositions.empty() )
    {
        return;
    }

    const glm::uvec3 dataOffset = glm::uvec3{ minVoxel };
    const glm::uvec3 dataSize = glm::uvec3{ maxVoxel - minVoxel + sk_voxelOne };

    // Safety check:
    const size_t N = dataSize.x * dataSize.y * dataSize.z;

    if ( N != voxelPositions.size() ||
         N != voxelValues.size() )
    {
        spdlog::error( "Invalid number of voxels when performing segmentation" );
        return;
    }

    // Set values in the segmentation image:
    for ( size_t i = 0; i < voxelPositions.size(); ++i )
    {
        setSegValue( seg, voxelPositions[i].x, voxelPositions[i].y, voxelPositions[i].z,
                     static_cast<int64_t>( voxelValues[i] ) );
    }

    updateSegTexture( segUid, seg, dataOffset, dataSize,
                      static_cast<const void*>( voxelValues.data() ) );
}

#endif // SEG_UTILITY_TPP
