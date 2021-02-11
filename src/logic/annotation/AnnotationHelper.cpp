#include "logic/annotation/AnnotationHelper.h"
#include "logic/annotation/AnnotationGroup.h"
#include "logic/annotation/Polygon.h"

#include "logic/AppData.h"

#include <mapbox/earcut.hpp>

#include <glm/glm.hpp>

#include <algorithm>
#include <iostream>
#include <list>
#include <utility>


namespace mapbox
{
namespace util
{

template <>
struct nth<0, Polygon::PointType>
{
    inline static auto get( const Polygon::PointType& point )
    {
        return point[0];
    }
};

template <>
struct nth<1, Polygon::PointType>
{
    inline static auto get( const Polygon::PointType& point )
    {
        return point[1];
    }
};

} // namespace util
} // namespace mapbox


void triangulatePolygon( Polygon& polygon )
{
    polygon.setTriangulation( mapbox::earcut<Polygon::IndexType>( polygon.getAllVertices() ) );
}


/*
void setUniqueAnnotationLayers( AppData& appData )
{
    // Pair consisting of annotation UID and its layer
    using AnnotUidAndLayer = std::pair<UID, int>;

    // Loop over all images
    for ( auto imageUid : appData.orderedimageUids() )
    {
        std::cout << "image " << imageUid << std::endl;

        // Create list of all annotations for this image, ordered by their layer
        std::list<AnnotUidAndLayer> orderedAnnotations;

        // Loop in order of annotations for the image
        for ( auto annotUid : appData.orderedImageAnnotationUids( imageUid ) )
        {
            auto annot = appData.imageAnnotationRecord( annotUid );
            auto r = annot.lock();
            if ( r && r->cpuData() )
            {
                std::cout << "\tAnnot " << annotUid << ", orig layer = " << r->cpuData()->getLayer() << std::endl;

                // Note: there is no guarantee that layers are unique
                orderedAnnotations.push_back( std::make_pair( r->uid(), r->cpuData()->getLayer() ) );
            }
        }

        // Sort annotation UIDs based on layer
        orderedAnnotations.sort( [] ( const AnnotUidAndLayer& a, const AnnotUidAndLayer& b ) {
            return ( a.second < b.second );
        } );


        // Reassign unique layers, starting at 0:
        uint32_t layer = 0;
        const uint32_t maxLayer = orderedAnnotations.size() - 1;

        for ( const auto& a : orderedAnnotations )
        {
            auto r = appData.imageAnnotationRecord( a.first ).lock();
            if ( r && r->cpuData() )
            {
                std::cout << "\t\tAnnot " << a.first
                          << ", new layer = " << layer << std::endl;

                r->cpuData()->setLayer( layer );
                r->cpuData()->setMaxLayer( maxLayer );
                ++layer;
            }
        }
    }
}
*/

/*
void changeAnnotationLayering(
        AppData& appData, const uuids::uuid& imageAnnotUid, const LayerChangeType& layerChange )
{
    // Pair consisting of annotation UID and its layer
    using AnnotUidAndLayer = std::pair<UID, int>;

    // First assign unique layers to all annotations
    setUniqueAnnotationLayers( appData );

    auto imageUid = appData.imageUid_of_annotation( imageAnnotUid );
    if ( ! imageUid )
    {
        std::cerr << "Error: No image associated with annotatation " << imageAnnotUid << std::endl;
        return;
    }

    // List of ordered annotations for the image
    std::list<AnnotUidAndLayer> orderedAnnotations;

    for ( auto uid : appData.annotationUids_of_image( *imageUid ) )
    {
        auto annot = appData.imageAnnotationRecord( uid );
        auto r = annot.lock();
        if ( r && r->cpuData() )
        {
            orderedAnnotations.push_back( std::make_pair( r->uid(), r->cpuData()->getLayer() ) );
        }
    }

    // Sort annotation UIDs based on layer
    orderedAnnotations.sort( [] ( const AnnotUidAndLayer& a, const AnnotUidAndLayer& b ) {
        return ( a.second < b.second );
    } );


    // Find the annotation to be changed:
    auto it = std::find_if( std::begin( orderedAnnotations ), std::end( orderedAnnotations ),
                            [imageAnnotUid] ( const AnnotUidAndLayer& a )
    { return ( a.first == imageAnnotUid ); } );


    // Apply the layer change:
    switch ( layerChange )
    {
    case LayerChangeType::Backwards:
    {
        orderedAnnotations.splice( std::prev( it ), orderedAnnotations, it );
        break;
    }
    case LayerChangeType::Forwards:
    {
        orderedAnnotations.splice( std::next( it ), orderedAnnotations, it );
        break;
    }
    case LayerChangeType::ToBack:
    {
        orderedAnnotations.splice( std::begin( orderedAnnotations ), orderedAnnotations, it );
        break;
    }
    case LayerChangeType::ToFront:
    {
        orderedAnnotations.splice( std::end( orderedAnnotations ), orderedAnnotations, it );
        break;
    }
    }


    // Reassign the layers and depths based on their new order. Start assigning layers at 0:
    uint32_t layer = 0;
    const uint32_t maxLayer = orderedAnnotations.size() - 1;

    for ( const auto& a : orderedAnnotations )
    {
        auto r = appData.imageAnnotationRecord( a.first ).lock();
        if ( r && r->cpuData() )
        {
            r->cpuData()->setLayer( layer );
            r->cpuData()->setMaxLayer( maxLayer );
            ++layer;
        }
    }
}
*/
