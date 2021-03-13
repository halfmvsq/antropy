#ifndef ANNOTATION_H
#define ANNOTATION_H

#include "logic/annotation/Polygon.tpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <uuid.h>

#include <memory>
#include <string>
#include <utility>


class AppData;
//enum class LayerChangeType;

//template<typename TComp, uint32_t Dim>
//class Polygon;



/**
 * @brief An image annotation, which (for now) is a closed, planar polygon with vertices
 * defined with 2D coordinates. The annotation plane is defined in the image's Subject coordinate system.
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

    explicit Annotation( const glm::vec4& subjectPlaneEquation );
    ~Annotation() = default;


    /// @brief Set/get the annotation name
    //void setName( std::string name );
    //const std::string& getName() const;

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


    /// @brief Set/get the annotation visibility
    void setVisibility( bool visibility );
    bool getVisibility() const;

    /// @brief Set/get the annotation opacity in range [0.0, 1.0]
    void setOpacity( float opacity );
    float getOpacity() const;

    /// @brief Set/get the annotation color (non-premultiplied RGB)
    void setColor( glm::vec3 color );
    const glm::vec3& getColor() const;

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

    /// Annotation name
    std::string m_name;

    /// Annotation polygon, which can include holes
    AnnotPolygon<float, 2> m_polygon;

    /// Annotation layer: 0 is the backmost layer and higher layers are more frontwards
    uint32_t m_layer;

    /// The maximum layer among all annotations in the same plane as this annotation
    uint32_t m_maxLayer;

    /// Annotation visibility
    bool m_visibility;

    /// Annotation opacity in [0.0, 1.0] range
    float m_opacity;

    /// Annotation color (non-premultiplied RGB triple)
    glm::vec3 m_color;

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
