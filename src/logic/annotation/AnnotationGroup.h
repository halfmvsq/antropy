#ifndef ANNOTATION_GROUP_H
#define ANNOTATION_GROUP_H

#include "logic/annotation/Polygon.h"

#include <glm/vec3.hpp>

#include <uuid.h>

#include <memory>
#include <string>

class AppData;
enum class LayerChangeType;


/**
 * @brief A group of image annotations, each of which is a closed, planar polygon with vertices
 * defined in normalized slide coordinates [0.0, 1.0]^2.
 *
 * @todo Layering
 * @todo Should this class be for just one annotation instead of for a group?
 */
class AnnotationGroup
{
    /// Friend helper function that sets the layer depth of annotations
//    friend void setUniqueAnnotationLayers( AppData& );

    /// Friend helper function that changes the layer depth of annotations
//    friend void changeAnnotationLayering( AppData&, const uuids::uuid&, const LayerChangeType& );


public:

    /// Construct an empty annotation group
    explicit AnnotationGroup();

    ~AnnotationGroup() = default;


    /// Set/get the file name of the file from/to which landmarks were loaded/saved
    void setFileName( std::string fileName );
    const std::string& getFileName() const;

    /// Set/get the group name
    void setName( std::string name );
    const std::string& getName() const;


    /// Set the annotation's polygon
    void setPolygon( std::unique_ptr<Polygon> );

    /// Get non-const access to the annotation's polygon
    Polygon* polygon();


    /// Get the annotation layer, with 0 being the backmost layer and layers increasing in value
    /// closer towards the viewer
    uint32_t getLayer() const;

    /// Get the maximum annotation layer
    uint32_t getMaxLayer() const;


    /// Set the annotation opacity in range [0.0, 1.0]
    void setOpacity( float opacity );

    /// Get the annotation opacity
    float getOpacity() const;

    /// Set the annotation color (non-premultiplied RGB)
    void setColor( glm::vec3 color );

    /// Get the annotation color (non-premultiplied RGB)
    const glm::vec3& getColor() const;


private:

    /// Name of the file from/to which the annotations were loaded/saved
    std::string m_fileName;

    /// Name of annotation group
    std::string m_name;

    /// Annotation polygon, which can include holes
    std::unique_ptr<Polygon> m_polygon;

    /// Internal layer of the annotation: 0 is the backmost layer and higher layers are more
    /// frontwards.
    uint32_t m_layer;

    /// The maximum layer among all annotations for a given slide.
    uint32_t m_maxLayer;

    /// Annotation opacity in [0.0, 1.0] range
    float m_opacity;

    /// Annotation color (non-premultiplied RGB triple)
    glm::vec3 m_color;


    /// Set the annotation layer, with 0 being the backmost layer.
    /// @note Use the function \c changeSlideAnnotationLayering to change annotation layer
    void setLayer( uint32_t layer );

    /// Set the maximum annotation layer.
    /// @note Set using the function \c changeSlideAnnotationLayering
    void setMaxLayer( uint32_t maxLayer );
};

#endif // ANNOTATION_GROUP_H
