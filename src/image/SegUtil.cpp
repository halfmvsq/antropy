#include "image/SegUtil.h"
#include "image/Image.h"

#include "logic/camera/MathUtility.h"

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/hash.hpp>
//#include <glm/gtx/string_cast.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <queue>
#include <tuple>
#include <unordered_set>
#include <vector>


namespace
{

// Does the voxel intersect the view plane?
bool voxelInterectsViewPlane(
        const glm::vec4& voxelViewPlane,
        const glm::vec3& voxelPos )
{
    static const glm::vec3 cornerOffset{ 0.5f, 0.5f, 0.5f };
    return math::testAABBoxPlaneIntersection( voxelPos, voxelPos + cornerOffset, voxelViewPlane );
}


std::tuple< std::unordered_set< glm::ivec3 >, glm::ivec3, glm::ivec3 >
paintBrush2d(
        const glm::vec4& voxelViewPlane,
        const glm::ivec3& segDims,
        const glm::ivec3& roundedPixelPos,
        const std::array<float, 3>& mmToVoxelSpacings,
        int brushSizeInVoxels,
        bool brushIsRound )
{
    // Queue of voxels to test for intersection with the view plane
    std::queue< glm::ivec3 > voxelsToTest;

    // Set of voxels that have been sent off for testing
    std::unordered_set< glm::ivec3 > voxelsProcessed;

    // Set of voxels that intersect with the view plane and that should be painted
    std::unordered_set< glm::ivec3 > voxelsToPaint;

    // Set of voxels that do not intersect with view plane or that should not be painted
    std::unordered_set< glm::ivec3 > voxelsToIgnore;

    // Check if the voxel is inside the segmentation
    auto isVoxelInSeg = [&segDims] ( const glm::ivec3& voxelPos )
    {
        return ( voxelPos.x >= 0 && voxelPos.x < segDims.x &&
                 voxelPos.y >= 0 && voxelPos.y < segDims.y &&
                 voxelPos.z >= 0 && voxelPos.z < segDims.z );
    };

    // Insert the first voxel as a voxel to test, if it's inside the segmentation.
    // This voxel should intersect the view plane, since it was clicked by the mouse,
    // but test it to make sure.

    if ( isVoxelInSeg( roundedPixelPos ) &&
         voxelInterectsViewPlane( voxelViewPlane, roundedPixelPos ) )
    {
        voxelsToTest.push( roundedPixelPos );
        voxelsProcessed.insert( roundedPixelPos );
    }

    std::array< glm::ivec3, 6 > neighbors;

    const float radius = static_cast<float>( brushSizeInVoxels - 1 );

    // Loop over all voxels in the test queue
    while ( ! voxelsToTest.empty() )
    {
        const glm::ivec3 q = voxelsToTest.front();
        voxelsToTest.pop();

        // Check if the voxel is inside the brush
        const glm::vec3 d = q - roundedPixelPos;

        if ( ! brushIsRound )
        {
            // Equation for a rectangle:
            if ( std::max( std::max( std::abs( d.x / mmToVoxelSpacings[0] ),
                                     std::abs( d.y / mmToVoxelSpacings[1] ) ),
                           std::abs( d.z / mmToVoxelSpacings[2] ) ) > radius )
            {
                voxelsToIgnore.insert( q );
                continue;
            }
        }
        else
        {
            if ( ( d.x * d.x / ( mmToVoxelSpacings[0] * mmToVoxelSpacings[0] ) +
                   d.y * d.y / ( mmToVoxelSpacings[1] * mmToVoxelSpacings[1] ) +
                   d.z * d.z / ( mmToVoxelSpacings[2] * mmToVoxelSpacings[2] ) ) > radius * radius )
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
                 voxelInterectsViewPlane( voxelViewPlane, n ) &&
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
    std::unordered_set< glm::ivec3 > voxelsToChange;

    // Min/max corners of the set of voxels to change:
    glm::ivec3 minVoxel{ std::numeric_limits<int>::max() };
    glm::ivec3 maxVoxel{ std::numeric_limits<int>::lowest() };

    for ( const auto& p : voxelsToPaint )
    {
        voxelsToChange.insert( p );
        minVoxel = glm::min( minVoxel, p );
        maxVoxel = glm::max( maxVoxel, p );
    }

    return std::make_tuple( voxelsToChange, minVoxel, maxVoxel );
}


std::tuple< std::unordered_set< glm::ivec3 >, glm::ivec3, glm::ivec3 >
paintBrush3d(
        const glm::ivec3& segDims,
        const glm::ivec3& roundedPixelPos,
        const std::array<float, 3>& mmToVoxelSpacings,
        const std::array<int, 3>& mmToVoxelCoeffs,
        int brushSizeInVoxels,
        bool brushIsRound )
{
    // Set of unique voxels to change:
    std::unordered_set< glm::ivec3 > voxelToChange;

    // Min/max corners of the set of voxels to change:
    glm::ivec3 minVoxel{ std::numeric_limits<int>::max() };
    glm::ivec3 maxVoxel{ std::numeric_limits<int>::lowest() };

    const int radius_int = brushSizeInVoxels - 1;
    const float radius_f = static_cast<float>( radius_int );

    for ( int k = -mmToVoxelCoeffs[2] * radius_int; k <= mmToVoxelCoeffs[2] * radius_int; ++k )
    {
        const int K = roundedPixelPos.z + k;
        if ( K < 0 || K >= segDims.z ) continue;

        for ( int j = -mmToVoxelCoeffs[1] * radius_int; j <= mmToVoxelCoeffs[1] * radius_int; ++j )
        {
            const int J = roundedPixelPos.y + j;
            if ( J < 0 || J >= segDims.y ) continue;

            for ( int i = -mmToVoxelCoeffs[0] * radius_int; i <= mmToVoxelCoeffs[0] * radius_int; ++i )
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
                    if ( std::max( std::max( std::abs( ii / mmToVoxelSpacings[0] ),
                                             std::abs( jj / mmToVoxelSpacings[1] ) ),
                                   std::abs( kk / mmToVoxelSpacings[2] ) ) <= radius_f )
                    {
                        voxelToChange.insert( p );
                        changed = true;
                    }
                }
                else
                {
                    // Round brush: additional check that voxel is inside the sphere of radius N
                    if ( ( ii * ii / ( mmToVoxelSpacings[0] * mmToVoxelSpacings[0] ) +
                           jj * jj / ( mmToVoxelSpacings[1] * mmToVoxelSpacings[1] ) +
                           kk * kk / ( mmToVoxelSpacings[2] * mmToVoxelSpacings[2] ) ) <= radius_f * radius_f )
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

    return std::make_tuple( voxelToChange, minVoxel, maxVoxel );
}

} // anonymous


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
            const glm::uvec3& size, const int64_t* data ) >& updateSegTexture )
{
    static constexpr size_t sk_comp = 0;
    static const glm::ivec3 sk_voxelOne{ 1, 1, 1 };

    // Set the brush radius (not including the central voxel): Radius = (brush width - 1) / 2
    // A single voxel brush has radius zero, a width 3 voxel brush has radius 1,
    // a width 5 voxel brush has radius 2, etc.

    // Coefficients that convert mm to voxel spacing:
    std::array<float, 3> mmToVoxelSpacings{ 1.0f, 1.0f, 1.0f };

    // Integer versions of the mm to voxel coefficients:
    std::array<int, 3> mmToVoxelCoeffs{ 1, 1, 1 };

    if ( brushIsIsotropic )
    {
        // Compute factors that account for anisotropic spacing:
        static constexpr bool sk_isotropicAlongMaxSpacingAxis = false;

        const float spacing = ( sk_isotropicAlongMaxSpacingAxis )
                ? glm::compMax( segSpacing )
                : glm::compMin( segSpacing );

        for ( uint32_t i = 0; i < 3; ++i )
        {
            mmToVoxelSpacings[i] = spacing / segSpacing[static_cast<int>(i)];
            mmToVoxelCoeffs[i] = std::max( static_cast<int>( std::ceil( mmToVoxelSpacings[i] ) ), 1 );
        }
    }

    // Set of unique voxels to change:
    std::unordered_set< glm::ivec3 > voxelsToChange;

    // Min/max corners of the set of voxels to change
    glm::ivec3 maxVoxel{ std::numeric_limits<int>::lowest() };
    glm::ivec3 minVoxel{ std::numeric_limits<int>::max() };

    if ( brushIs3d )
    {
        std::tie( voxelsToChange, minVoxel, maxVoxel ) = paintBrush3d(
                    segDims, roundedPixelPos, mmToVoxelSpacings, mmToVoxelCoeffs,
                    brushSizeInVoxels, brushIsRound );
    }
    else
    {
        std::tie( voxelsToChange, minVoxel, maxVoxel ) = paintBrush2d(
                    voxelViewPlane, segDims, roundedPixelPos, mmToVoxelSpacings,
                    brushSizeInVoxels, brushIsRound );
    }

    // Create a rectangular block of contiguous voxel value data that will be set in the texture:
    std::vector< glm::ivec3 > voxelPositions;
    std::vector< int64_t > voxelValues;

    for ( int k = minVoxel.z; k <= maxVoxel.z; ++k )
    {
        for ( int j = minVoxel.y; j <= maxVoxel.y; ++j )
        {
            for ( int i = minVoxel.x; i <= maxVoxel.x; ++i )
            {
                const glm::ivec3 p{ i, j, k };
                voxelPositions.emplace_back( p );

                if ( voxelsToChange.count( p ) > 0 )
                {
                    // Marked to change, so paint it:
                    if ( brushReplacesBgWithFg )
                    {
                        const int64_t currentLabel = seg->valueAsInt64( sk_comp, i, j, k ).value_or( 0 );

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
                    voxelValues.emplace_back( seg->valueAsInt64( sk_comp, i, j, k ).value_or( 0 ) );
                }
            }
        }
    }

    if ( voxelPositions.empty() )
    {
        return;
    }

    const glm::uvec3 dataOffset{ minVoxel };
    const glm::uvec3 dataSize{ maxVoxel - minVoxel + sk_voxelOne };

    // Safety check:
    const size_t N = dataSize.x * dataSize.y * dataSize.z;

    if ( N != voxelPositions.size() || N != voxelValues.size() )
    {
        spdlog::error( "Invalid number of voxels when performing segmentation" );
        return;
    }

    // Set values in the segmentation image:
    for ( size_t i = 0; i < voxelPositions.size(); ++i )
    {
        seg->setValue( sk_comp, voxelPositions[i].x, voxelPositions[i].y, voxelPositions[i].z, voxelValues[i] );
    }

    updateSegTexture( seg->header().memoryComponentType(), dataOffset, dataSize, voxelValues.data() );
}
