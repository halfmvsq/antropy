#include "logic/camera/MathUtility.h"
#include "logic/camera/Camera.h"
#include "logic/camera/CameraHelpers.h"

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

/**
Building an Orthonormal Basis, Revisited
Tom Duff, James Burgess, Per Christensen, Christophe Hery, Andrew Kensler, Max Liani, and Ryusuke Villemin

Journal of Computer Graphics Techniques Vol. 6, No. 1, 2017
 */
/// Use this to create a camera basis with a lookat direction without any priority axes
void buildONB( const glm::vec3& n, glm::vec3& b1, glm::vec3& b2 )
{
    float sign = std::copysign( 1.0f, n.z );
    const float a = -1.0f / ( sign + n.z );
    const float b = n.x * n.y * a;

    b1 = glm::vec3{ 1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x };
    b2 = glm::vec3{ b, sign + n.y * n.y * a, -n.y };
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
