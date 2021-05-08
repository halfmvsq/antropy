#include "logic/annotation/Annotation.h"
#include "logic/camera/MathUtility.h"

#include "common/Exception.hpp"

#include <glm/glm.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>


namespace
{

static constexpr float sk_defaultOpacity = 1.0f;
static constexpr float sk_defaultThickness = 1.5f;

} // anonymous


Annotation::Annotation(
        std::string name,
        const glm::vec4& color,
        const glm::vec4& subjectPlaneEquation )
    :
      m_displayName( std::move( name ) ),
      m_fileName(),
      m_polygon(),

      m_selectedVertices(),
      m_selectedEdges(),
      m_selected( false ),

      m_closed( false ),
      m_visible( true ),
      m_filled( false ),
      m_vertexVisibility( true ),

      m_opacity( sk_defaultOpacity ),
      m_vertexColor{ color },
      m_fillColor{ color.r, color.g, color.b, 0.5f * color.a },
      m_lineColor{ color },
      m_lineThickness( sk_defaultThickness ),

      m_subjectPlaneEquation{ 1.0f, 0.0f, 0.0f, 0.0f },
      m_subjectPlaneOrigin{ 0.0f, 0.0f, 0.0f },
      m_subjectPlaneAxes( { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } )
{
    if ( ! setSubjectPlane( subjectPlaneEquation ) )
    {
        throw_debug( "Cannot construct Annotation with invalid plane" )
    }
}


void Annotation::setDisplayName( std::string displayName )
{
    m_displayName = std::move( displayName );
}

const std::string& Annotation::getDisplayName() const
{
    return m_displayName;
}

void Annotation::setFileName( std::string fileName )
{
    m_fileName = std::move( fileName );
}

const std::string& Annotation::getFileName() const
{
    return m_fileName;
}

bool Annotation::setSubjectPlane( const glm::vec4& subjectPlaneEquation )
{
    static constexpr float k_minLength = 1.0e-4f;
    static const glm::vec3 k_origin( 0.0f, 0.0f, 0.0f );

    const glm::vec3 subjectPlaneNormal = glm::vec3{ subjectPlaneEquation };

    if ( glm::length( subjectPlaneNormal ) < k_minLength )
    {
        spdlog::error( "The annotation plane normal vector {} is invalid",
                       glm::to_string( subjectPlaneNormal ) );
        return false;
    }

    m_subjectPlaneEquation = glm::vec4{ glm::normalize( subjectPlaneNormal ), subjectPlaneEquation[3] };
    m_subjectPlaneOrigin = math::projectPointToPlane( k_origin, subjectPlaneEquation );
    m_subjectPlaneAxes = math::buildOrthonormalBasis_branchless( subjectPlaneNormal );

    // Make double sure that the axes are normalized:
    m_subjectPlaneAxes.first = glm::normalize( m_subjectPlaneAxes.first );
    m_subjectPlaneAxes.second = glm::normalize( m_subjectPlaneAxes.second );

    return true;
}

AnnotPolygon<float, 2>& Annotation::polygon()
{
    return m_polygon;
}

const AnnotPolygon<float, 2>& Annotation::polygon() const
{
    return m_polygon;
}

const std::vector< std::list< glm::vec2 > >&
Annotation::getAllVertices() const
{
    return m_polygon.getAllVertices();
}

const std::list<glm::vec2>&
Annotation::getBoundaryVertices( size_t boundary ) const
{
    return m_polygon.getBoundaryVertices( boundary );
}

void Annotation::addPlanePointToBoundary(
        size_t boundary, const glm::vec2& planePoint )
{
    m_polygon.addVertexToBoundary( boundary, planePoint );
}

std::optional<glm::vec2> Annotation::addSubjectPointToBoundary(
        size_t boundary, const glm::vec3& subjectPoint )
{
    const glm::vec2 projectedPlanePoint =
            math::projectPointToPlaneLocal2dCoords(
                subjectPoint,
                m_subjectPlaneEquation,
                m_subjectPlaneOrigin,
                m_subjectPlaneAxes );

    addPlanePointToBoundary( boundary, projectedPlanePoint );

    return projectedPlanePoint;
}

void Annotation::removeVertexSelections()
{
    m_selectedVertices.clear();
}

void Annotation::removeEdgeSelections()
{
    m_selectedEdges.clear();
}

const std::set< std::pair<size_t, size_t> >&
Annotation::selectedVertices() const
{
    return m_selectedVertices;
}

const std::set< std::pair<size_t, std::pair<size_t, size_t> > >&
Annotation::selectedEdge() const
{
    return m_selectedEdges;
}

void Annotation::addSelectedVertex( const std::pair<size_t, size_t>& vertex )
{
    // Is this a valid boundary and vertex index for that boundary?
    const size_t boundary = vertex.first;
    const size_t vertexIndex = vertex.second;

    if ( m_polygon.getBoundaryVertex( boundary, vertexIndex ) )
    {
        m_selectedVertices.insert( vertex );
    }
    else
    {
        spdlog::warn( "Unable to select invalid polygon vertex {} for boundary {}.",
                      vertexIndex, boundary );
    }
}

void Annotation::addSelectedEdge( const std::pair<size_t, std::pair<size_t, size_t> >& edge )
{
    // Is this a valid boundary and pair of vertices (defining an edge) for that boundary?
    const size_t boundary = edge.first;
    const size_t vertexIndex1 = edge.second.first;
    const size_t vertexIndex2 = edge.second.second;

    if ( m_polygon.getBoundaryVertex( boundary, vertexIndex1 ) &&
         m_polygon.getBoundaryVertex( boundary, vertexIndex2 ) )
    {
        // Check that the vertices are neighbors and form an edge.
        // This happens if they are either separated by 1 or N-1,
        // the latter if the edge connects vertices 0 and N-1.

        const size_t N = m_polygon.getBoundaryVertices( boundary ).size();
        const int v1 = static_cast<int>( vertexIndex1 );
        const int v2 = static_cast<int>( vertexIndex2 );
        const size_t dist = std::abs( v1 - v2 );

        if ( 1 == dist || (N - 1) == dist )
        {
            m_selectedEdges.insert( edge );
        }
    }
    else
    {
        spdlog::warn( "Unable to select invalid polygon edge ({}, {}) for boundary {}.",
                      vertexIndex1, vertexIndex2, boundary );
    }
}

void Annotation::setSelected( bool selected )
{
    m_selected = selected;
}

bool Annotation::isSelected() const
{
    return m_selected;
}

void Annotation::setClosed( bool closed )
{
    m_closed = closed;
}

bool Annotation::isClosed() const
{
    return m_closed;
}

void Annotation::setVisible( bool visibility )
{
    m_visible = visibility;
}

bool Annotation::isVisible() const
{
    return m_visible;
}

void Annotation::setVertexVisibility( bool visibility )
{
    m_vertexVisibility = visibility;
}

bool Annotation::getVertexVisibility() const
{
    return m_vertexVisibility;
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

void Annotation::setVertexColor( glm::vec4 color )
{
    m_vertexColor = std::move( color );
}

const glm::vec4& Annotation::getVertexColor() const
{
    return m_vertexColor;
}

void Annotation::setLineColor( glm::vec4 color )
{
    m_lineColor = std::move( color );
}

const glm::vec4& Annotation::getLineColor() const
{
    return m_lineColor;
}

void Annotation::setLineThickness( float thickness )
{
    if ( thickness >= 0.0f )
    {
        m_lineThickness = thickness;
    }
}

float Annotation::getLineThickness() const
{
    return m_lineThickness;
}

void Annotation::setFilled( bool filled )
{
    m_filled = filled;
}

bool Annotation::isFilled() const
{
    return m_filled;
}

void Annotation::setFillColor( glm::vec4 color )
{
    m_fillColor = std::move( color );
}

const glm::vec4& Annotation::getFillColor() const
{
    return m_fillColor;
}

const glm::vec4& Annotation::getSubjectPlaneEquation() const
{
    return m_subjectPlaneEquation;
}

const glm::vec3& Annotation::getSubjectPlaneOrigin() const
{
    return m_subjectPlaneOrigin;
}

const std::pair<glm::vec3, glm::vec3>&
Annotation::getSubjectPlaneAxes() const
{
    return m_subjectPlaneAxes;
}

glm::vec2 Annotation::projectSubjectPointToAnnotationPlane(
        const glm::vec3& point3d ) const
{
    return math::projectPointToPlaneLocal2dCoords(
                point3d,
                m_subjectPlaneEquation,
                m_subjectPlaneOrigin,
                m_subjectPlaneAxes );
}

glm::vec3 Annotation::unprojectFromAnnotationPlaneToSubjectPoint(
        const glm::vec2& planePoint2d ) const
{
    return m_subjectPlaneOrigin +
            planePoint2d[0] * m_subjectPlaneAxes.first +
            planePoint2d[1] * m_subjectPlaneAxes.second;
}
