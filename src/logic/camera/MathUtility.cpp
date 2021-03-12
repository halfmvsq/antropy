#include "logic/camera/MathUtility.h"
#include "logic/camera/Camera.h"
#include "logic/camera/CameraHelpers.h"

#include "common/Exception.hpp"

#include <glm/gtc/matrix_inverse.hpp>

#define EPSILON 0.000001

#define CROSS(dest, v1, v2){                 \
    dest[0] = v1[1] * v2[2] - v1[2] * v2[1]; \
    dest[1] = v1[2] * v2[0] - v1[0] * v2[2]; \
    dest[2] = v1[0] * v2[1] - v1[1] * v2[0];}

#define DOT(v1, v2) (v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2])

#define SUB(dest, v1, v2){       \
    dest[0] = v1[0] - v2[0]; \
    dest[1] = v1[1] - v2[1]; \
    dest[2] = v1[2] - v2[2];}


namespace math
{

std::pair<glm::vec3, glm::vec3> buildOrthonormalBasis_branchless( const glm::vec3& n )
{
    float sign = std::copysign( 1.0f, n.z );
    const float a = -1.0f / ( sign + n.z );
    const float b = n.x * n.y * a;

    return { { 1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x },
             { b, sign + n.y * n.y * a, -n.y } };
}


std::pair<glm::vec3, glm::vec3> buildOrthonormalBasis( const glm::vec3& n )
{
    if ( n.z < 0.0f )
    {
        const float a = 1.0f / (1.0f - n.z);
        const float b = n.x * n.y * a;

        return { { 1.0f - n.x * n.x * a, -b, n.x },
                 { b, n.y * n.y*a - 1.0f, -n.y } };
    }
    else
    {
        const float a = 1.0f / (1.0f + n.z);
        const float b = -n.x * n.y * a;

        return { { 1.0f - n.x * n.x * a, b, -n.x },
                 { b, 1.0f - n.y * n.y * a, -n.y } };
    }
}


glm::vec3 convertVecToRGB( const glm::vec3& v )
{
    const glm::vec3 c = glm::abs( v );
    return c / glm::compMax( c );
}


glm::u8vec3 convertVecToRGB_uint8( const glm::vec3& v )
{
    return glm::u8vec3{ 255.0f * convertVecToRGB( v ) };
}


std::vector<uint32_t> sortCounterclockwise( const std::vector<glm::vec2>& points )
{
    if ( points.empty() )
    {
        return std::vector<uint32_t>{};
    }
    else if ( 1 == points.size() )
    {
        return std::vector<uint32_t>{ 0 };
    }

    glm::vec2 center{ 0.0f, 0.0f };

    for ( const auto& p : points )
    {
        center += p;
    }
    center = center / static_cast<float>( points.size() );

    const glm::vec2 a = points[0] - center;

    std::vector<float> angles;

    for ( uint32_t i = 0; i < points.size(); ++i )
    {
        const glm::vec2 b = points[i] - center;
        const float dot = a.x * b.x + a.y * b.y;
        const float det = a.x * b.y - b.x * a.y;
        const float angle = std::atan2( det, dot );
        angles.emplace_back( angle );
    }

    std::vector<uint32_t> indices( points.size() );
    std::iota( std::begin(indices), std::end(indices), 0 );

    std::sort( std::begin( indices ), std::end( indices ),
               [&angles] ( const uint32_t& i, const uint32_t& j ) {
                return ( angles[i] <= angles[j] );
                }
    );

    return indices;
}


std::vector< glm::vec2 > project3dPointsToPlane( const std::vector< glm::vec3 >& A )
{
    const glm::vec3 normal = glm::cross( A[1] - A[0], A[2] - A[0] );

    const glm::mat4 M = glm::lookAt( A[0] - normal, A[0], A[1] - A[0] );

    std::vector< glm::vec2 > B;

    for ( auto& a : A )
    {
        B.emplace_back( glm::vec2{ M * glm::vec4{ a, 1.0f } } );
    }

    return B;
}


glm::vec3 projectPointToPlane(
        const glm::vec3& point,
        const glm::vec4& planeEquation )
{
    // Plane normal is (A, B, C):
    const glm::vec3 planeNormal{ planeEquation };
    const float L = glm::length( planeNormal );

    if ( L < glm::epsilon<float>() )
    {
        throw_debug( "Cannot project point to plane: plane normal has zero length" )
    }

    // Signed distance of point to plane (positive if on same side of plane as normal vector):
    const float distancePointToPlane = glm::dot( planeEquation, glm::vec4{ point, 1.0f }) / L;

    // Point projected to plane:
    return ( point - distancePointToPlane * planeNormal );
}


glm::vec2 projectPointToPlaneLocal2dCoords(
        const glm::vec3& point,
        const glm::vec4& planeEquation,
        const glm::vec3& planeOrigin,
        const std::pair<glm::vec3, glm::vec3>& planeAxes )
{
    const glm::vec3 pointProjectedToPlane = projectPointToPlane( point, planeEquation );

    // Express projected point in 2D plane coordinates:
    return glm::vec2{
        glm::dot( pointProjectedToPlane - planeOrigin, glm::normalize( planeAxes.first ) ),
        glm::dot( pointProjectedToPlane - planeOrigin, glm::normalize( planeAxes.second ) ) };
}


void applyLayeringOffsetsToModelPositions(
        const camera::Camera& camera,
        const glm::mat4& model_T_world,
        uint32_t layer,
        std::vector<glm::vec3>& modelPositions )
{
    if ( modelPositions.empty() )
    {
        return;
    }

    // Matrix for transforming vectors from Camera to Model space:
    const glm::mat3 model_T_camera_invTrans =
            glm::inverseTranspose( glm::mat3{ model_T_world * camera.world_T_camera() } );

    // The view's Back direction transformed to Model space:
    const glm::vec3 modelTowardsViewer = glm::normalize(
                model_T_camera_invTrans * Directions::get( Directions::View::Back ) );

    // Compute offset in World units based on first position (this choice is arbitrary)
    const float worldDepth = camera::computeSmallestWorldDepthOffset( camera, modelPositions.front() );

    // Proportionally offset higher layers by more distance
    const float offsetMag = static_cast<float>( layer ) * worldDepth;
    const glm::vec3 modelOffset = offsetMag * modelTowardsViewer;

    for ( auto& p : modelPositions )
    {
        p += modelOffset;
    }
}


glm::mat3 computeSubjectAxesInCamera(
        const glm::mat3& camera_T_world_rotation,
        const glm::mat3& world_T_subject_rotation )
{
    return glm::inverseTranspose( camera_T_world_rotation * world_T_subject_rotation );
}

} // namespace math
