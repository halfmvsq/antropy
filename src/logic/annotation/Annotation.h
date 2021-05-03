#ifndef ANNOTATION_H
#define ANNOTATION_H

#include "logic/annotation/AnnotPolygon.tpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <uuid.h>

#include <memory>
#include <string>
#include <utility>


class AppData;
//enum class LayerChangeType;


/**
 * @brief An image annotation, which (for now) is a planar polygon with vertices
 * defined with 2D coordinates. Note: Each polygon vertex is parameterized in 2D,
 * but it may represent a point in 3D.
 *
 * The annotation plane is defined in the image's Subject coordinate system.
 *
 * @todo Layering
 * @todo Text and regular shape annotations
 */
class Annotation
{
    /// Friend helper function that sets the layer depth of annotations
//    friend void setUniqueAnnotationLayers( AppData& );

    /// Friend helper function that changes the layer depth of annotations
//    friend void changeAnnotationLayering( AppData&, const uuids::uuid&, const LayerChangeType& );


public:

    explicit Annotation(
            std::string displayName,
            const glm::vec4& color,
            const glm::vec4& subjectPlaneEquation );

    ~Annotation() = default;


    /// @brief Set/get the annotation display name
    void setDisplayName( std::string displayName );
    const std::string& getDisplayName() const;

    /// @brief Set/get the annotation file name
    void setFileName( std::string fileName );
    const std::string& getFileName() const;


    /// @brief Get the annotation's polygon as a const/non-const reference
    AnnotPolygon<float, 2>& polygon();
    const AnnotPolygon<float, 2>& polygon() const;

    const std::vector< std::vector<glm::vec2> >& getAllVertices() const;

    const std::vector<glm::vec2>& getBoundaryVertices( size_t boundary ) const;


    /**
     * @brief Add a 3D Subject point to the annotation polygon.
     * @param[in] boundary Boundary to add point to
     * @param[in] subjectPoint 3D point in Subject space to project
     * @return Projected point in 2D Subject plane coordinates
     */
    std::optional<glm::vec2> addSubjectPointToBoundary(
            size_t boundary, const glm::vec3& subjectPoint );


    /// Get the annotation layer, with 0 being the backmost layer and layers increasing in value
    /// closer towards the viewer
    uint32_t getLayer() const;

    /// Get the maximum annotation layer
    uint32_t getMaxLayer() const;


    /// @brief Set/get the annotation selected state
    void setSelected( bool selected );
    bool isSelected() const;

    /// @brief Set/get whether the annotation's outer boundary is closed.
    /// If closed, then it is assumed that the last vertex conntects to the first vertex.
    /// If a polygon boundary is specified as closed,  then it is assumed that its last vertex is
    /// connected by an edge to the first vertex. The user need NOT specify a final vertex that is
    /// identical to the first vertex. For example, a closed triangular polygon can be defined with
    /// exactly three vertices.
    void setClosed( bool closed );
    bool isClosed() const;

    /// @brief Set/get the annotation visibility
    void setVisible( bool visibility );
    bool isVisible() const;

    /// @brief Set/get the vertex visibility
    void setVertexVisibility( bool visibility );
    bool getVertexVisibility() const;

    /// @brief Set/get the overall annotation opacity in range [0.0, 1.0], which gets modulated
    /// with the color opacities
    void setOpacity( float opacity );
    float getOpacity() const;

    /// @brief Set/get the annotation vertex color (non-premultiplied RGBA)
    void setVertexColor( glm::vec4 color );
    const glm::vec4& getVertexColor() const;

    /// @brief Set/get the annotation line color (non-premultiplied RGBA)
    void setLineColor( glm::vec4 color );
    const glm::vec4& getLineColor() const;

    /// @brief Set/get the annotation line stroke thickness
    void setLineThickness( float thickness );
    float getLineThickness() const;

    /// @brief Set/get whether the annotation interior is filled
    void setFilled( bool filled );
    bool isFilled() const;

    /// @brief Set/get the annotation fill color (non-premultiplied RGB)
    void setFillColor( glm::vec4 color );
    const glm::vec4& getFillColor() const;


    /// @brief Get the annotation plane equation in Subject space
    const glm::vec4& getSubjectPlaneEquation() const;

    /// @brief Get the annotation plane origin in Subject space
    const glm::vec3& getSubjectPlaneOrigin() const;

    /// @brief Get the annotation plane coordinate axes in Subject space
    const std::pair<glm::vec3, glm::vec3>& getSubjectPlaneAxes() const;


    /// @brief Compute the projection of a 3D point (in Subject space) into
    /// 2D annotation Subject plane coordinates
    glm::vec2 projectSubjectPointToAnnotationPlane(
            const glm::vec3& subjectPoint ) const;

    /// @brief Compute the un-projected 3D coordinates (in Subject space) of a
    /// 2D point defined in annotation Subject plane coordinates
    glm::vec3 unprojectFromAnnotationPlaneToSubjectPoint(
            const glm::vec2& planePoint2d ) const;


private:

    /// Set the axes of the plane in Subject space
    bool setSubjectPlane( const glm::vec4& subjectPlaneEquation );

    /// Set the annotation layer, with 0 being the backmost layer.
    /// @note Use the function \c changeSlideAnnotationLayering to change annotation layer
    void setLayer( uint32_t layer );

    /// Set the maximum annotation layer.
    /// @note Set using the function \c changeSlideAnnotationLayering
    void setMaxLayer( uint32_t maxLayer );

    std::string m_displayName; //!< Annotation display name
    std::string m_fileName; //!< Annotation file name

    /// Annotation polygon, which can include holes
    AnnotPolygon<float, 2> m_polygon;

    /// Annotation layer: 0 is the backmost layer and higher layers are more frontwards
    uint32_t m_layer;

    /// The maximum layer among all annotations in the same plane as this annotation
    uint32_t m_maxLayer;

    bool m_selected; //!< Is the annotation selected?
    bool m_closed; //!< Is the annotation's outer boundary closed?
    bool m_visible; //!< Is the annotation visible?
    bool m_filled; //!< Is the annotation filled?
    bool m_vertexVisibility; //!< Are the annotation boundary vertices visible?

    /// Overall annotation opacity in [0.0, 1.0] range.
    /// The annotation fill and line colors opacities are modulated by this value.
    float m_opacity;

    glm::vec4 m_vertexColor; //!< Vertex color (non-premultiplied RGBA)
    glm::vec4 m_fillColor; //!< Fill color (non-premultiplied RGBA)
    glm::vec4 m_lineColor; //!< Line color (non-premultiplied RGBA)
    float m_lineThickness; //!< Line thickness


    /// Equation of the 3D plane containing this annotation. The plane is defined by the
    /// coefficients (A, B, C, D) of equation Ax + By + Cz + D = 0, where (x, y, z) are
    /// coordinates in Subject space.
    glm::vec4 m_subjectPlaneEquation;

    /// 3D origin of the plane in Subject space
    glm::vec3 m_subjectPlaneOrigin;

    /// 3D axes of the plane in Subject space
    std::pair<glm::vec3, glm::vec3> m_subjectPlaneAxes;
};

#endif // ANNOTATION_H
