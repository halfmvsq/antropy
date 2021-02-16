#include "logic/annotation/Annotation.h"
#include "logic/annotation/Polygon.tpp"

#include <glm/glm.hpp>


namespace
{

static constexpr float sk_defaultOpacity = 1.0f;
static const glm::vec3 sk_defaultColor{ 0.5f, 0.5f, 0.5f };

} // anonymous


Annotation::Annotation( std::shared_ptr< Polygon<float, 2> > polygon )
    :
      m_name(),
      m_polygon( polygon ),
      m_layer( 0 ),
      m_maxLayer( 0 ),
      m_visibility( true ),
      m_opacity( sk_defaultOpacity ),
      m_color{ sk_defaultColor }
{
}

std::weak_ptr< Polygon<float, 2> > Annotation::polygon()
{
    return m_polygon;
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
