#include "common/MathFuncs.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <random>
#include <string>


namespace math
{

std::vector< glm::vec3 > generateRandomHsvSamples(
        size_t numSamples,
        const std::pair< float, float >& hueMinMax,
        const std::pair< float, float >& satMinMax,
        const std::pair< float, float >& valMinMax,
        const std::optional<uint32_t>& seed )
{
    std::mt19937_64 generator;
    generator.seed( seed ? *seed : std::mt19937_64::default_seed );

    std::uniform_real_distribution<float> dist( 0.0f, 1.0f );

    const float A = ( satMinMax.second * satMinMax.second -
                      satMinMax.first * satMinMax.first );

    const float B = satMinMax.first * satMinMax.first;

    const float C = ( valMinMax.second * valMinMax.second * valMinMax.second -
                      valMinMax.first * valMinMax.first * valMinMax.first );

    const float D = valMinMax.first * valMinMax.first * valMinMax.first;

    std::vector< glm::vec3 > samples;
    samples.reserve( numSamples );

    for ( size_t i = 0; i < numSamples; ++i )
    {
        float hue = ( hueMinMax.second - hueMinMax.first ) * dist( generator ) + hueMinMax.first;
        float sat = std::sqrt( dist( generator ) * A + B );
        float val = std::pow( dist( generator ) * C + D, 1.0f / 3.0f );

        samples.push_back( glm::vec3{ hue, sat, val } );
    }

    return samples;
}


glm::dvec3 computeSubjectImageDimensions(
        const glm::u64vec3& pixelDimensions,
        const glm::dvec3& pixelSpacing )
{
    return glm::dvec3{ static_cast<double>( pixelDimensions.x ) * pixelSpacing.x,
                       static_cast<double>( pixelDimensions.y ) * pixelSpacing.y,
                       static_cast<double>( pixelDimensions.z ) * pixelSpacing.z };
}


glm::dmat4 computeImagePixelToSubjectTransformation(
        const glm::dmat3& directions,
        const glm::dvec3& pixelSpacing,
        const glm::dvec3& origin )
{
    return glm::dmat4{
        glm::dvec4{ pixelSpacing.x * directions[0], 0.0 }, // column 0
        glm::dvec4{ pixelSpacing.y * directions[1], 0.0 }, // column 1
        glm::dvec4{ pixelSpacing.z * directions[2], 0.0 }, // column 2
        glm::dvec4{ origin, 1.0 } };                       // column 3
}


glm::dmat4 computeImagePixelToTextureTransformation(
        const glm::u64vec3& pixelDimensions )
{
    const glm::dvec3 invDim( 1.0 / static_cast<double>( pixelDimensions.x ),
                             1.0 / static_cast<double>( pixelDimensions.y ),
                             1.0 / static_cast<double>( pixelDimensions.z ) );

    return glm::translate( 0.5 * invDim ) * glm::scale( invDim );
}


glm::vec3 computeInvPixelDimensions( const glm::u64vec3& pixelDimensions )
{
    const glm::dvec3 invDim( 1.0 / static_cast<double>( pixelDimensions.x ),
                             1.0 / static_cast<double>( pixelDimensions.y ),
                             1.0 / static_cast<double>( pixelDimensions.z ) );

    return invDim;
}


std::array< glm::vec3, 8 > computeImagePixelAABBoxCorners(
        const glm::u64vec3& pixelDims )
{
    constexpr size_t N = 8; // Image has 8 corners

    // To get the pixel edges/corners, offset integer coordinates by half of a pixel,
    // because integer pixel coordinates are at the CENTER of the pixel
    static const glm::vec3 sk_halfPixel{ 0.5f, 0.5f, 0.5f };

    const glm::vec3 D = glm::vec3{ pixelDims } - sk_halfPixel;

    const std::array< glm::vec3, N > pixelCorners =
    {
        glm::vec3{ -0.5f, -0.5f, -0.5f },
        glm::vec3{ D.x, -0.5f, -0.5f },
        glm::vec3{ -0.5f, D.y, -0.5f },
        glm::vec3{ D.x, D.y, -0.5f },
        glm::vec3{ -0.5f, -0.5f, D.z },
        glm::vec3{ D.x, -0.5f, D.z },
        glm::vec3{ -0.5f, D.y, D.z },
        glm::vec3{ D.x, D.y, D.z }
    };

    return pixelCorners;
}


std::array< glm::vec3, 8 >
computeImageSubjectBoundingBoxCorners(
        const glm::u64vec3& pixelDims,
        const glm::mat3& directions,
        const glm::vec3& spacing,
        const glm::vec3& origin )
{
    constexpr size_t N = 8; // Image has 8 corners

    const glm::mat4 subject_T_pixel = computeImagePixelToSubjectTransformation( directions, spacing, origin );

    const std::array< glm::vec3, N > pixelCorners = computeImagePixelAABBoxCorners( pixelDims );

    std::array< glm::vec3, N > subjectCorners;

    std::transform( std::begin( pixelCorners ), std::end( pixelCorners ),
                    std::begin( subjectCorners ),
    [ &subject_T_pixel ]( const glm::dvec3& v )
    {
        return glm::vec3{ subject_T_pixel * glm::vec4{ v, 1.0f } };
    } );

    return subjectCorners;
}


std::pair< glm::vec3, glm::vec3 >
computeMinMaxCornersOfAABBox(
        const std::array< glm::vec3, 8 >& subjectCorners )
{
    glm::vec3 minSubjectCorner{ std::numeric_limits<float>::max() };
    glm::vec3 maxSubjectCorner{ std::numeric_limits<float>::lowest() };

    for ( uint32_t c = 0; c < 8; ++c )
    {
        for ( int i = 0; i < 3; ++i )
        {
            if ( subjectCorners[c][i] < minSubjectCorner[i] )
            {
                minSubjectCorner[i] = subjectCorners[c][i];
            }

            if ( subjectCorners[c][i] > maxSubjectCorner[i] )
            {
                maxSubjectCorner[i] = subjectCorners[c][i];
            }
        }
    }

    return std::make_pair( minSubjectCorner, maxSubjectCorner );
}


std::array< glm::vec3, 8 >
computeAllAABBoxCornersFromMinMaxCorners(
        const std::pair< glm::vec3, glm::vec3 >& boxMinMaxCorners )
{
    const glm::vec3 size = boxMinMaxCorners.second - boxMinMaxCorners.first;

    std::array< glm::vec3, 8 > corners =
    {
        {
            glm::vec3{ 0, 0, 0 },
            glm::vec3{ size.x, 0, 0 },
            glm::vec3{ 0, size.y, 0 },
            glm::vec3{ 0, 0, size.z },
            glm::vec3{ size.x, size.y, 0 },
            glm::vec3{ size.x, 0, size.z },
            glm::vec3{ 0, size.y, size.z },
            glm::vec3{ size.x, size.y, size.z }
        }
    };

    std::for_each( std::begin(corners), std::end(corners), [ &boxMinMaxCorners ] ( glm::vec3& corner )
    {
        corner = corner + boxMinMaxCorners.first;
    } );

    return corners;
}


std::pair< std::string, bool > computeSpiralCodeFromDirectionMatrix( const glm::dmat3& directions )
{
    // Fourth character is null-terminating character
    char spiralCode[4] = "???";

    // LPS directions are positive
    static const char codes[3][2] = { {'R', 'L'}, {'A', 'P'}, {'I', 'S'} };

    bool isOblique = false;

    for ( uint32_t i = 0; i < 3; ++i )
    {
        // Direction cosine for voxel direction i
        const glm::dvec3 dir = directions[i];

        uint32_t closestDir = 0;
        double maxDot = -999.0;
        uint32_t sign = 0;

        for (uint32_t j = 0; j < 3; ++j )
        {
            glm::dvec3 a{ 0.0, 0.0, 0.0 };
            a[j] = 1.0;

            const double newDot = glm::dot( glm::abs( dir ), a );

            if ( newDot > maxDot )
            {
                maxDot = newDot;
                closestDir = j;

                if ( glm::dot( dir, a ) < 0.0 )
                {
                    sign = 0;
                }
                else
                {
                    sign = 1;
                }
            }
        }

        spiralCode[i] = codes[closestDir][sign];

        if ( glm::abs( maxDot ) < 1.0 )
        {
            isOblique = true;
        }
    }

    return std::make_pair( std::string{ spiralCode }, isOblique );
}


void rotateFrameAboutWorldPos(
        CoordinateFrame& frame,
        const glm::quat& rotation,
        const glm::vec3& worldCenter )
{
    const glm::quat oldRotation = frame.world_T_frame_rotation();
    const glm::vec3 oldOrigin = frame.worldOrigin();

    frame.setFrameToWorldRotation( rotation * oldRotation );
    frame.setWorldOrigin( rotation * ( oldOrigin - worldCenter ) + worldCenter );
}

} // namespace math
