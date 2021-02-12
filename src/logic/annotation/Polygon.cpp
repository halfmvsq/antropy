#include "logic/annotation/Polygon.h"
#include "common/UuidUtility.h"

#include <glm/glm.hpp>

#include <spdlog/spdlog.h>


Polygon::Polygon()
    :
      m_vertices(),
      m_triangulation(),
      m_currentUid(),
      m_aabb( std::nullopt )
{}


void Polygon::setAllVertices( std::vector< std::vector<PointType> > vertices )
{
    m_vertices = std::move( vertices );
    m_triangulation.clear();
    m_currentUid = generateRandomUuid();

    computeAABBox();
}


const std::vector< std::vector<Polygon::PointType> >&
Polygon::getAllVertices() const
{
    return m_vertices;
}


void Polygon::setBoundaryVertices( size_t boundary, std::vector<PointType> vertices )
{
    if ( boundary >= m_vertices.size() )
    {
        spdlog::error( "Invalid polygon boundary index {}", boundary );
        return;
    }

    m_vertices.at( boundary ) = std::move( vertices );
    m_triangulation.clear();
    m_currentUid = generateRandomUuid();

    if ( 0 == boundary )
    {
        computeAABBox();
    }
}


void Polygon::setOuterBoundary( std::vector<PointType> vertices )
{
    if ( m_vertices.size() >= 1 )
    {
        m_vertices[0] = std::move( vertices );
    }
    else
    {
        m_vertices.emplace_back( std::move( vertices ) );
    }

    m_triangulation.clear();
    m_currentUid = generateRandomUuid();

    computeAABBox();
}


bool Polygon::addHole( std::vector<PointType> vertices )
{
    if ( m_vertices.size() >= 1 )
    {
        m_vertices.emplace_back( std::move( vertices ) );
        m_triangulation.clear();
        m_currentUid = generateRandomUuid();
        return true;
    }

    return false;
}


const std::vector< Polygon::PointType >&
Polygon::getBoundaryVertices( size_t boundary ) const
{
    static const std::vector< Polygon::PointType > sk_emptyBoundary;

    if ( boundary >= m_vertices.size() )
    {
        spdlog::error( "Invalid polygon boundary index {}", boundary );
        return sk_emptyBoundary;
    }

    return m_vertices.at( boundary );
}


size_t Polygon::numBoundaries() const
{
    return m_vertices.size();
}


size_t Polygon::numVertices() const
{
    size_t N = 0;

    for ( const auto& boundary : m_vertices )
    {
        N += boundary.size();
    }

    return N;
}


std::optional<Polygon::PointType>
Polygon::getBoundaryVertex( size_t boundary, size_t i ) const
{
    if ( boundary >= m_vertices.size() )
    {
        spdlog::error( "Invalid polygon boundary index {}", boundary );
        return std::nullopt;
    }

    const auto& vertices = m_vertices.at( boundary );

    if ( i >= vertices.size() )
    {
        spdlog::error( "Invalid vertex index {} for polygon boundary {}", i, boundary );
        return std::nullopt;
    }

    return vertices.at( i );
}


std::optional<Polygon::PointType>
Polygon::getVertex( size_t i ) const
{
    size_t j = i;

    for ( const auto& boundary : m_vertices )
    {
        if ( j < boundary.size() )
        {
            return boundary[j];
        }
        else
        {
            j -= boundary.size();
        }
    }

    spdlog::error( "Invalid polygon vertex {}", i );
    return std::nullopt;
}


void Polygon::setTriangulation( std::vector<IndexType> indices )
{
    m_triangulation = std::move( indices );
    m_currentUid = generateRandomUuid();
}


bool Polygon::hasTriangulation() const
{
    return ( ! m_triangulation.empty() );
}


const std::vector< Polygon::IndexType >&
Polygon::getTriangulation() const
{
    return m_triangulation;
}


std::optional< std::tuple< Polygon::IndexType, Polygon::IndexType, Polygon::IndexType > >
Polygon::getTriangle( size_t i ) const
{
    if ( 3*i + 2 >= m_triangulation.size() )
    {
        spdlog::error( "Invalid triangle index {}", i );
        return std::nullopt;
    }

    return std::make_tuple( m_triangulation.at( 3*i + 0 ),
                            m_triangulation.at( 3*i + 1 ),
                            m_triangulation.at( 3*i + 2 ) );
}


std::optional< Polygon::AABBoxType > Polygon::getAABBox() const
{
    return m_aabb;
}


size_t Polygon::numTriangles() const
{
    // Every three indices make a triangle
    return m_triangulation.size() / 3;
}


uuids::uuid Polygon::getCurrentUid() const
{
    return m_currentUid;
}


bool Polygon::equals( const Polygon& otherPolygon ) const
{
    return ( m_currentUid == otherPolygon.getCurrentUid() );
}


void Polygon::computeAABBox()
{
    if ( m_vertices.empty() || m_vertices[0].empty() )
    {
        // There is no outer boundary or there are no vertices in the outer boundary.
        m_aabb = std::nullopt;
        return;
    }

    // Compute AABB of outer boundary vertices
    m_aabb = std::make_pair( PointType( std::numeric_limits<ComponentType>::max() ),
                             PointType( std::numeric_limits<ComponentType>::lowest() ) );

    for ( const auto& v : m_vertices[0] )
    {
        m_aabb->first = glm::min( m_aabb->first, v );
        m_aabb->second = glm::max( m_aabb->second, v );
    }
}
