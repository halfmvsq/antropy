#ifndef ANNOTATION_HELPER_H
#define ANNOTATION_HELPER_H

#include <uuid.h>

class AppData;
class Polygon;


/**
 * @brief Triangulate a polygon using the Earcut algorithm. This algorithm can triangulate a simple,
 * planar polygon of any winding order that includes holes. It returns a robust, acceptable solution
 * for non-simple poygons. Earcut works on a 2D plane.
 *
 * @see https://github.com/mapbox/earcut.hpp
 *
 * @param[in,out] polygon Polygon to triangulate.
 */
void triangulatePolygon( Polygon& polygon );



/**
 * @brief Annotation layers for a given image may not be unique. This function sets each annotation
 * to a unique layer.
 *
 * @todo Finish this function. Current implementation is from HistoloZee.
 */
//void setUniqueAnnotationLayers( AppData& );


/// Types of changes to layering
enum class LayerChangeType
{
    Backwards,
    Forwards,
    ToBack,
    ToFront
};


/**
 * @brief Apply a change to an annotation's layering
 * @param appData
 * @param annotUid UID of the annotation
 * @param layerChange Change to apply to the layer
 *
 * @todo Finish this function. Current implementation is from HistoloZee.
 */
//void changeAnnotationLayering(
//        AppData& appData, const uuids::uuid& annotUid, const LayerChangeType& layerChange );


#endif // ANNOTATION_HELPER_H
