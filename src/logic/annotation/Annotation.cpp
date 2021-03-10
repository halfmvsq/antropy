#include "logic/annotation/Annotation.h"

#include "common/Exception.hpp"

#include "logic/annotation/Polygon.tpp"
#include "logic/camera/MathUtility.h"

#include <glm/glm.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>


namespace
{

static constexpr float sk_defaultOpacity = 1.0f;
static const glm::vec3 sk_defaultColor{ 0.5f, 0.5f, 0.5f };

} // anonymous


Annotation::Annotation(
        const glm::vec4& subjectPlaneEquation,
        std::shared_ptr< Polygon<float, 2> > polygon )
    :
      m_name(),
      m_polygon( polygon ),
      m_layer( 0 ),
      m_maxLayer( 0 ),
      m_visibility( true ),
      m_opacity( sk_defaultOpacity ),
      m_color{ sk_defaultColor },

      m_subjectPlaneEquation{ 1.0f, 0.0f, 0.0f, 0.0f },
      m_subjectPlaneOrigin{ 0.0f, 0.0f, 0.0f },
      m_subjectPlaneAxes( { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } )
{
    if ( ! setsubjectPlane( subjectPlaneEquation ) )
    {
        throw_debug( "Cannot construct Annotation with invalid plane" )
    }
}

bool Annotation::setsubjectPlane( const glm::vec4& subjectPlaneEquation )
{
    constexpr float k_minLength = 1.0e-4f;
    static const glm::vec3 k_origin( 0.0f, 0.0f, 0.0f );

    const glm::vec3 planeNormal = glm::vec3{ subjectPlaneEquation };

    if ( glm::length( planeNormal ) < k_minLength )
    {
        spdlog::error( "The annotation plane normal vector {} is invalid",
                       glm::to_string( planeNormal ) );
        return false;
    }

    m_subjectPlaneEquation = glm::vec4{ glm::normalize( planeNormal ), subjectPlaneEquation[3] };
    m_subjectPlaneOrigin = math::projectPointToPlane( k_origin, subjectPlaneEquation );
    m_subjectPlaneAxes = math::buildOrthonormalBasis_branchless( planeNormal );

    // Make double sure that the axes are normalized:
    m_subjectPlaneAxes.first = glm::normalize( m_subjectPlaneAxes.first );
    m_subjectPlaneAxes.second = glm::normalize( m_subjectPlaneAxes.second );

    return true;
}

std::weak_ptr< Polygon<float, 2> > Annotation::polygon()
{
    return m_polygon;
}

Polygon<float, 2>* Annotation::polygon() const
{
    return ( m_polygon ? m_polygon.get() : nullptr );
}

const std::vector< std::vector< glm::vec2 > >&
Annotation::getAllVertices() const
{
    static const std::vector< std::vector< glm::vec2 > > sk_empty{};
    if ( ! m_polygon ) return sk_empty;
    return m_polygon->getAllVertices();
}

const std::vector<glm::vec2>&
Annotation::getBoundaryVertices( size_t boundary ) const
{
    static const std::vector< glm::vec2 > sk_empty{};
    if ( ! m_polygon ) return sk_empty;
    return m_polygon->getBoundaryVertices( boundary );
}

std::optional<glm::vec2>
Annotation::addPointToBoundary( size_t boundary, const glm::vec3& point )
{
    if ( ! m_polygon )
    {
        return std::nullopt;
    }

    const glm::vec2 projectedPoint = math::projectPointToPlaneLocal2dCoords(
                point, m_subjectPlaneEquation, m_subjectPlaneOrigin, m_subjectPlaneAxes );

    m_polygon->addVertexToBoundary( boundary, projectedPoint );
    return projectedPoint;
}

void Annotation::setLayer( uint32_t layer )
{
    m_layer = layer;
}

uint32_t Annotation::getLayer() const
{
    return m_layer;
}

void Annotation::setMaxLayer( uint32_t maxLayer )
{
    m_maxLayer = maxLayer;
}

uint32_t Annotation::getMaxLayer() const
{
    return m_maxLayer;
}

void Annotation::setVisibility( bool visibility )
{
    m_visibility = visibility;
}

bool Annotation::getVisibility() const
{
    return m_visibility;
}

void Annotation::setOpacity( float opacity )
{
    if ( 0.0f <= opacity && opacity <= 1.0f )
    {
        m_opacity = opacity;
    }
}

float Annotation::getOpacity() const
{
    return m_opacity;
}

void Annotation::setColor( glm::vec3 color )
{
    m_color = std::move( color );
}

const glm::vec3& Annotation::getColor() const
{
    return m_color;
}

const glm::vec4& Annotation::getSubjectPlaneEquation() const { return m_subjectPlaneEquation; }
const glm::vec3& Annotation::getSubjectPlaneOrigin() const { return m_subjectPlaneOrigin; }
const std::pair<glm::vec3, glm::vec3>& Annotation::getSubjectPlaneAxes() const { return m_subjectPlaneAxes; }


glm::vec2 Annotation::projectPointToAnnotationPlane( const glm::vec3& point3d ) const
{
    return math::projectPointToPlaneLocal2dCoords(
                point3d, m_subjectPlaneEquation, m_subjectPlaneOrigin, m_subjectPlaneAxes );
}

glm::vec3 Annotation::unprojectPointFromAnnotationPlane( const glm::vec2& planePoint2d ) const
{
    return m_subjectPlaneOrigin +
            planePoint2d[0] * m_subjectPlaneAxes.first +
            planePoint2d[1] * m_subjectPlaneAxes.second;
}
