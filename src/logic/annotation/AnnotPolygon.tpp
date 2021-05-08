#ifndef POLYGON_TPP
#define POLYGON_TPP

#include "common/AABB.h"
#include "common/UuidUtility.h"

#include <spdlog/spdlog.h>

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <uuid.h>

#include <limits>
#include <list>
#include <optional>
#include <tuple>
#include <vector>


/**
 * @brief A polygon of any winding order that can have multiple holes inside of an outer boundary.
 * The planarity of the polygon is not enforced: that is the reponsibility of the user.
 *
 * @note The polygon's outer boundary can be either open or closed. This property is not specified
 * in this class: It is left up to the user of this class to decide whether the boundary is closed or open.
 * By definition, all holes must be closed boundaries.
 *
 * @note The polygon can have a triangulation that uses only its original vertices.
 *
 * @tparam TComp Vertex component type
 * @tparam Dim Vertex dimension
 */
template< typename TComp, uint32_t Dim >
class AnnotPolygon
{
public:

    /// Vertex point type
    using PointType = glm::vec<Dim, TComp, glm::highp>;

    /// Axis-aligned bounding box type
    using AABBoxType = AABB_N<Dim, TComp>;


    /// Construct empty polygon with no triangulation
    explicit AnnotPolygon()
        :
          m_vertices(),
          m_selectedVertex( std::nullopt ),
          m_selectedEdge( std::nullopt ),
          m_triangulation(),
          m_currentUid(),
          m_aabb( std::nullopt ),
          m_centroid( 0 )
    {}

    ~AnnotPolygon() = default;


    /// Set all vertices of the polygon. The first list defines the main (outer) polygon boundary;
    /// subsequent lists define boundaries of holes within the outer boundary.
    void setAllVertices( std::vector< std::list<PointType> > vertices )
    {
        m_vertices = std::move( vertices );
        m_triangulation.clear();
        m_currentUid = generateRandomUuid();

        computeAABBox();
        computeCentroid();
        removeSelections();
    }


    /// Get all vertices from all boundaries. The first list contains vertices of the outer boundary;
    /// subsequent lists contain vertices of holes.
    const std::vector< std::list<PointType> >& getAllVertices() const
    {
        return m_vertices;
    }


    /// Set vertices for a given boundary, where 0 refers to the outer boundary;
    /// boundaries >= 1 are for holes.
    bool setBoundaryVertices( size_t boundary, std::list<PointType> vertices )
    {
        if ( boundary >= m_vertices.size() )
        {
            spdlog::error( "Invalid polygon boundary index {}", boundary );
            return false;
        }

        m_vertices.at( boundary ) = std::move( vertices );
        m_triangulation.clear();
        m_currentUid = generateRandomUuid();

        if ( 0 == boundary )
        {
            computeAABBox();
            computeCentroid();
        }

        removeSelections();

        return true;
    }


    /// Add a vertex to a given boundary, where 0 refers to the outer boundary;
    /// boundaries >= 1 are for holes
    bool addVertexToBoundary( size_t boundary, PointType vertex )
    {
        if ( boundary >= m_vertices.size() )
        {
            if ( 0 == boundary )
            {
                // Allow adding the outer boundary:
                m_vertices.emplace_back( std::list<PointType>{ vertex } );
                spdlog::info( "Added new polygon boundary with index {}", boundary );
            }
            else
            {
                spdlog::error( "Unable to add vertex {} to invalid boundary {}",
                               glm::to_string( vertex ), boundary );
                return false;
            }
        }
        else
        {
            m_vertices[boundary].emplace_back( std::move( vertex ) );
        }

        m_triangulation.clear();
        m_currentUid = generateRandomUuid();

        if ( 0 == boundary )
        {
            computeAABBox();
            updateCentroid();
        }

        removeSelections();

        return true;
    }


    /// Set the vertices of the outer boundary only.
    void setOuterBoundary( std::list<PointType> vertices )
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
        computeCentroid();
        removeSelections();
    }


    /// Add a vertex to the outer boundary
    void addVertexToOuterBoundary( PointType vertex )
    {
        if ( m_vertices.size() >= 1 )
        {
            m_vertices[0].emplace_back( std::move( vertex ) );
        }
        else
        {
            m_vertices.emplace_back( std::list<PointType>{ vertex } );
        }

        m_triangulation.clear();
        m_currentUid = generateRandomUuid();

        computeAABBox();
        updateCentroid();
        removeSelections();
    }


    /// Remove a vertex from a boundary
    bool removeVertexFromBoundary( size_t boundary, size_t vertexIndex )
    {
        if ( boundary >= m_vertices.size() )
        {
            spdlog::error( "Invalid polygon boundary index {}", boundary );
            return false;
        }

        const size_t numVertices = getBoundaryVertices( boundary ).size();

        if ( 1 == numVertices )
        {
            spdlog::error( "Cannot remove the last vertex of a boundary" );
            return false;
        }

        if ( vertexIndex >= numVertices )
        {
            spdlog::error( "Invalid polygon vertex {}", vertexIndex );
            return false;
        }

        auto iter = std::begin( m_vertices[boundary] );
        std::advance( iter, vertexIndex );

        // Erase the vertex
        m_vertices.at( boundary ).erase( iter );

        m_triangulation.clear();
        m_currentUid = generateRandomUuid();

        if ( 0 == boundary )
        {
            computeAABBox();
            computeCentroid();
        }

        removeSelections();

        return true;
    }


    /// Add a hole to the polygon. The operation only succeeds if the polygon has at least
    /// an outer boundary.
    bool addHole( std::list<PointType> vertices )
    {
        if ( m_vertices.size() >= 1 )
        {
            m_vertices.emplace_back( std::move( vertices ) );
            m_triangulation.clear();
            m_currentUid = generateRandomUuid();
            removeSelections();
            return true;
        }

        return false;
    }


    /// Get all vertices of a given boundary, where 0 refers to the outer boundary;
    /// boundaries >= 1 are holes.
    /// @return Empty list if invalid boundary
    const std::list<PointType>& getBoundaryVertices( size_t boundary ) const
    {
        static const std::list< AnnotPolygon::PointType > sk_emptyBoundary;

        if ( boundary >= m_vertices.size() )
        {
            spdlog::error( "Invalid polygon boundary index {}", boundary );
            return sk_emptyBoundary;
        }

        return m_vertices.at( boundary );
    }


    /// Get the number of boundaries in the polygon, including the outer boundary and all holes.
    size_t numBoundaries() const
    {
        return m_vertices.size();
    }

    /// Get the total number of vertices among all boundaries, including the outer boundary and holes.
    size_t numVertices() const
    {
        size_t N = 0;

        for ( const auto& boundary : m_vertices )
        {
            N += boundary.size();
        }

        return N;
    }


    /// Get the i'th vertex of a given boundary, where 0 is the outer boundary and subsequent boundaries
    /// define holes.
    /// @return Null optional if invalid boundary or vertex index
    std::optional<AnnotPolygon::PointType>
    getBoundaryVertex( size_t boundary, size_t i ) const
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

        auto iter = std::begin( vertices );
        std::advance( iter, i );
        return *iter;
    }


    /// Get i'th vertex of the whole polygon. Here i indexes the collection of all ordered vertices
    /// of the outer boundary and all hole boundaries.
    /// @return Null optional if invalid vertex
    std::optional<AnnotPolygon::PointType> getVertex( size_t i ) const
    {
        size_t j = i;

        for ( const auto& boundary : m_vertices )
        {
            if ( j < boundary.size() )
            {
                auto iter = std::begin( boundary );
                std::advance( iter, j );
                return *iter;
            }
            else
            {
                j -= boundary.size();
            }
        }

        spdlog::error( "Invalid polygon vertex {}", i );
        return std::nullopt;
    }


    /// Get the axis-aligned bounding box of the polygon.
    /// @returns Null optional if the polygon is empty
    std::optional< AABBoxType > getAABBox() const
    {
        return m_aabb;
    }


    /// Get the centroid of the polygon's outer boundary.
    /// @returns Origin if the outer boundary has no points.
    const PointType& getCentroid() const
    {
        return m_centroid;
    }


    /// Set the triangulation from a vector of indices that refer to vertices of the whole polygon.
    /// Every three consecutive indices form a triangle and triangles must be clockwise.
    void setTriangulation( std::vector<size_t> indices )
    {
        m_triangulation = std::move( indices );
        m_currentUid = generateRandomUuid();
    }


    /// Return true iff the polygon has a valid triangulation.
    bool hasTriangulation() const
    {
        return ( ! m_triangulation.empty() );
    }


    /// Get the polygon triangulation: a vector of indices refering to vertices of the whole polygon.
    const std::vector<size_t>& getTriangulation() const
    {
        return m_triangulation;
    }


    /// Get indices of the i'th triangle. The triangle is oriented clockwise.
    std::optional< std::tuple<size_t, size_t, size_t> >
    getTriangle( size_t i ) const
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


    /// Get the number of triangles in the polygon triangulation.
    size_t numTriangles() const
    {
        // Every three indices make a triangle
        return m_triangulation.size() / 3;
    }


    /// Get the unique ID that is re-generated every time anything changes for this polygon,
    /// including vertices and triangulation.
    uuids::uuid getCurrentUid() const
    {
        return m_currentUid;
    }


    /// Get the selected vertex: { boundary index, vertex index },
    /// where the vertex index is for the given boundary
    std::optional< std::pair<size_t, size_t> > selectedVertex() const
    {
        return m_selectedVertex;
    }

    /// Set the selected vertex: { boundary index, vertex index },
    /// where the vertex index is for the given boundary.
    /// Null optional turns off the vertex selection.
    void setSelectedVertex( const std::optional< std::pair<size_t, size_t> >& vertex )
    {
        if ( vertex )
        {
            // Is this a valid boundary and vertex index for that boundary?
            const size_t boundary = vertex->first;
            const size_t vertexIndex = vertex->second;

            if ( getBoundaryVertex( boundary, vertexIndex ) )
            {
                m_selectedVertex = *vertex;
            }
            else
            {
                spdlog::warn( "Unable to select invalid polygon vertex {} for boundary {}.",
                              vertexIndex, boundary );
            }
        }
        else
        {
            // Turn off the selection
            m_selectedVertex = std::nullopt;
        }
    }


    /// Get the selected edge: { boundary index, {first edge vertex index, second edge vertex index} },
    /// where the vertex indices are for the given boundary.
    std::optional< std::pair<size_t, std::pair<size_t, size_t> > > selectedEdge() const
    {
        return m_selectedEdge;
    }

    /// Set the selected edge: { boundary index, {first edge vertex index, second edge vertex index} },
    /// where the vertex indices are for the given boundary.
    /// Null optional turns off the edge selection.
    void setSelectedEdge( const std::optional< std::pair<size_t, std::pair<size_t, size_t> > >& edge )
    {
        if ( edge )
        {
            // Is this a valid boundary and pair of vertices (defining an edge) for that boundary?
            const size_t boundary = edge->first;
            const size_t vertexIndex1 = edge->second.first;
            const size_t vertexIndex2 = edge->second.second;

            if ( getBoundaryVertex( boundary, vertexIndex1 ) &&
                 getBoundaryVertex( boundary, vertexIndex2 ) )
            {
                // Check that the vertices are neighbors and form an edge.
                // This happens if they are either separated by 1 or N-1,
                // the latter if the edge connects vertices 0 and N-1.

                const size_t N = m_vertices.at( boundary ).size();
                const int v1 = static_cast<int>( vertexIndex1 );
                const int v2 = static_cast<int>( vertexIndex2 );
                const int dist = std::abs( v1 - v2 );

                if ( 1 == dist || (N - 1) == dist )
                {
                    m_selectedEdge = *edge;
                }
            }
            else
            {
                spdlog::warn( "Unable to select invalid polygon edge ({}, {}) for boundary {}.",
                              vertexIndex1, vertexIndex2, boundary );
            }
        }
        else
        {
            // Turn off the selection
            m_selectedEdge = std::nullopt;
        }
    }


    /// Return true iff this polygon equals (in terms of both vertices and triangulation)
    /// another polygon. The comparison is done based on unique IDs of the polygons.
    bool equals( const AnnotPolygon& otherPolygon ) const
    {
        return ( m_currentUid == otherPolygon.getCurrentUid() );
    }


private:

    /// Compute the 2D AABB of the outer polygon boundary, if it exists.
    void computeAABBox()
    {
        if ( m_vertices.empty() || m_vertices[0].empty() )
        {
            // There is no outer boundary or there are no vertices in the outer boundary.
            m_aabb = std::nullopt;
            return;
        }

        // Compute AABB of outer boundary vertices
        m_aabb = std::make_pair( PointType( std::numeric_limits<TComp>::max() ),
                                 PointType( std::numeric_limits<TComp>::lowest() ) );

        for ( const auto& v : m_vertices[0] )
        {
            m_aabb->first = glm::min( m_aabb->first, v );
            m_aabb->second = glm::max( m_aabb->second, v );
        }
    }


    /// Update the centroid of the outer boundary with a new point.
    /// Call this function AFTER adding the new point to the boundary.
    void updateCentroid()
    {
        if ( m_vertices.size() < 0 )
        {
            // No outer boundary
            m_centroid = PointType{ 0 };
            return;
        }

        const auto& outerBoundary = m_vertices[0];
        const size_t N = outerBoundary.size();

        if ( N < 0 )
        {
            m_centroid = PointType{ 0 };
            return;
        }
        else if ( 1 == N )
        {
            m_centroid = outerBoundary.front();
            return;
        }

        m_centroid += ( outerBoundary.back() - m_centroid ) / static_cast<TComp>( N );
    }


    /// Compute the centroid of the outer boundary from scratch
    void computeCentroid()
    {
        if ( m_vertices.size() < 0 )
        {
            // No outer boundary
            m_centroid = PointType{ 0 };
            return;
        }

        const auto& outerBoundary = m_vertices[0];
        const size_t N = outerBoundary.size();

        if ( N < 1 )
        {
            m_centroid = PointType{ 0 };
            return;
        }

        for ( const auto& p : outerBoundary )
        {
            m_centroid += p;
        }

        m_centroid /= outerBoundary.size();
    }


    /// Remove the vertex and edge selections
    void removeSelections()
    {
        m_selectedVertex = std::nullopt;
        m_selectedEdge = std::nullopt;
    }


    /// Polygon stored as vector of lists of points. The first list defines the outer polygon
    /// boundary; subsequent lists define holes in the main polygon. Any winding order for the
    /// outer boundary and holes is valid.
    std::vector< std::list<PointType> > m_vertices;

    /// Selected vertex: { boundary index, vertex index }
    std::optional< std::pair<size_t, size_t> > m_selectedVertex;

    /// Selected edge: { boundary index, {vertex index 1, vertex index 2} }
    std::optional< std::pair<size_t, std::pair<size_t, size_t> > > m_selectedEdge;

    /// Vector of indices that refer to the vertices of the input polygon.
    /// Three consecutive indices form a clockwise triangle.
    std::vector<size_t> m_triangulation;

    /// A unique ID that is re-generated every time anything changes for this polygon,
    /// including vertices and triangulation.
    uuids::uuid m_currentUid;

    /// Axis-aligned bounding box of the polygon; set to none if the polygon is empty.
    std::optional<AABBoxType> m_aabb;

    /// Centroid of the polygon's outer boundary. Set to origin if the outer boundary is empty.
    PointType m_centroid;
};

#endif // POLYGON_TPP
