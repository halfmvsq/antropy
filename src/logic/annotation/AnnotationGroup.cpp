#include "logic/annotation/AnnotationGroup.h"

#include <glm/glm.hpp>


namespace
{

static constexpr float sk_defaultOpacity = 1.0f;
static const glm::vec3 sk_defaultColor{ 0.5f, 0.5f, 0.5f };

} // anonymous


AnnotationGroup::AnnotationGroup()
    :
      m_fileName(),
      m_name(),
      m_polygon( nullptr ),
      m_layer( 0 ),
      m_maxLayer( 0 ),
      m_opacity( sk_defaultOpacity ),
      m_color{ sk_defaultColor }
{
}

void AnnotationGroup::setFileName( std::string fileName )
{
    m_fileName = std::move( fileName );
}

const std::string& AnnotationGroup::getFileName() const
{
    return m_fileName;
}

void AnnotationGroup::setPolygon( std::unique_ptr<Polygon> polygon )
{
    m_polygon = std::move( polygon );
}

Polygon* AnnotationGroup::polygon()
{
    if ( m_polygon )
    {
        return m_polygon.get();
    }
    return nullptr;
}

void AnnotationGroup::setLayer( uint32_t layer )
{
    m_layer = layer;
}

uint32_t AnnotationGroup::getLayer() const
{
    return m_layer;
}

void AnnotationGroup::setMaxLayer( uint32_t maxLayer )
{
    m_maxLayer = maxLayer;
}

uint32_t AnnotationGroup::getMaxLayer() const
{
    return m_maxLayer;
}

void AnnotationGroup::setOpacity( float opacity )
{
    if ( 0.0f <= opacity && opacity <= 1.0f )
    {
        m_opacity = opacity;
    }
}

float AnnotationGroup::getOpacity() const
{
    return m_opacity;
}

void AnnotationGroup::setColor( glm::vec3 color )
{
    m_color = std::move( color );
}

const glm::vec3& AnnotationGroup::getColor() const
{
    return m_color;
}
