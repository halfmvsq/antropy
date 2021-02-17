#ifndef ANNOTATION_H
#define ANNOTATION_H

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <uuid.h>

#include <memory>
#include <string>


class AppData;
//enum class LayerChangeType;

template<typename TComp, uint32_t Dim>
class Polygon;



/**
 * @brief An image annotation, which (for now) is a closed, planar polygon with vertices
 * defined with 2D coordinates [0.0, 1.0]^2.
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

    explicit Annotation( std::shared_ptr< Polygon<float, 2> > polygon );

    ~Annotation() = default;


    /// Set/get the annotation name
    void setName( std::string name );
    const std::string& getName() const;

    /// Gget the annotation's polygon
    std::weak_ptr< Polygon<float, 2> > polygon();


    /// Get the annotation layer, with 0 being the backmost layer and layers increasing in value
    /// closer towards the viewer
    uint32_t getLayer() const;

    /// Get the maximum annotation layer
    uint32_t getMaxLayer() const;


    /// Set/get the annotation visibility
    void setVisibility( bool visibility );
    bool getVisibility() const;

    /// Set/get the annotation opacity in range [0.0, 1.0]
    void setOpacity( float opacity );
    float getOpacity() const;

    /// Set/get the annotation color (non-premultiplied RGB)
    void setColor( glm::vec3 color );
    const glm::vec3& getColor() const;


private:

    /// Set the annotation layer, with 0 being the backmost layer.
    /// @note Use the function \c changeSlideAnnotationLayering to change annotation layer
    void setLayer( uint32_t layer );

    /// Set the maximum annotation layer.
    /// @note Set using the function \c changeSlideAnnotationLayering
    void setMaxLayer( uint32_t maxLayer );

    /// Name of annotation
    std::string m_name;

    /// Annotation polygon, which can include holes
    std::shared_ptr< Polygon<float, 2> > m_polygon;

    /// Internal layer of the annotation: 0 is the backmost layer and higher layers are more
    /// frontwards.
    uint32_t m_layer;

    /// The maximum layer among all annotations for a given slide.
    uint32_t m_maxLayer;

    /// Annotation visibility
    bool m_visibility;

    /// Annotation opacity in [0.0, 1.0] range
    float m_opacity;

    /// Annotation color (non-premultiplied RGB triple)
    glm::vec3 m_color;

    /// Equation of the 3D plane containing this annotation.
    /// The plane is defined by (A, B, C, D), where Ax + By + Cz + D = 0
    glm::vec4 m_plane;
};

#endif // ANNOTATION_H
