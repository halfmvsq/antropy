#include "logic/annotation/BezierHelper.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace
{

std::pair< float, float > computeLineParams( const glm::vec2& a, const glm::vec2& b )
{
    const glm::vec2 d = b - a;
    const float length = glm::length( d );
    const float angle = std::atan2( d.y, d.x );
    return { length, angle };
}

glm::vec2 computeControlPoint(
        const glm::vec2& prev, const glm::vec2& curr, const glm::vec2& next,
        bool reverse, float smoothing )
{
    float length, angle;
    std::tie( length, angle ) = computeLineParams( prev, next );

    angle = angle + ( reverse ? glm::pi<float>() : 0.0f );
    length *= smoothing;

   return curr + length * glm::vec2{ std::cos( angle ), std::sin( angle ) };
}

} // anonymous


std::vector< std::tuple< glm::vec2, glm::vec2, glm::vec2 > >
computeBezierCommands( const std::vector<glm::vec2>& points, float smoothing, bool closed )
{
    std::vector< std::tuple< glm::vec2, glm::vec2, glm::vec2 > > commands;

    const int N = static_cast<int>( points.size() );

    for ( int i = 0; i < ( closed ? N + 1 : N ); ++i )
    {
        const glm::vec2 curr = points[ glm::clamp( i % N, 0, N - 1 ) ];
        const glm::vec2 m2 = points[ glm::clamp( closed ? (i - 2) % N : (i - 2), 0, N - 1 ) ];
        const glm::vec2 m1 = points[ glm::clamp( closed ? (i - 1) % N : (i - 1), 0, N - 1 ) ];
        const glm::vec2 p1 = points[ glm::clamp( closed ? (i + 1) % N : (i + 1), 0, N - 1 ) ];

        commands.emplace_back(
                    computeControlPoint( m2, m1, curr, false, smoothing ),
                    computeControlPoint( m1, curr, p1, true, smoothing ),
                    curr );
    }

    return commands;
}
