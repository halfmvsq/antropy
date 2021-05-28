#include "logic/annotation/SerializeAnnot.h"

#include <vector>

using json = nlohmann::json;


void to_json( json& j, const AnnotPolygon<float, 2>& poly )
{
    // Create a json with vector for each boundary.
    // Each boundary is itself a vector of pairs of vertices.

    for ( const auto& boundary : poly.getAllVertices() )
    {
        json boundaryJson;

        for ( const auto& vertices : boundary )
        {
            boundaryJson.emplace_back( std::vector<float>{ vertices.x, vertices.y } );
        }

        j.emplace_back( std::move( boundaryJson ) );
    }
}

void from_json( const json& /*j*/, AnnotPolygon<float, 2>& /*poly*/ )
{

}


void to_json( json& j, const Annotation& annot )
{
    const glm::vec4& lineCol = annot.getLineColor();
    const glm::vec4& fillCol = annot.getFillColor();

    const glm::vec4& planeEq = annot.getSubjectPlaneEquation();
    const glm::vec3& planeOr = annot.getSubjectPlaneOrigin();
    const std::pair< glm::vec3, glm::vec3 >& planeAxes = annot.getSubjectPlaneAxes();

    j = json
    {
        { "name", annot.getDisplayName() },
        { "visible", annot.isVisible() },
        { "opacity", annot.getOpacity() },
        { "lineThickness", annot.getLineThickness() },
        { "lineColor", { lineCol.r, lineCol.g, lineCol.b, lineCol.a } },
        { "fillColor", { fillCol.r, fillCol.g, fillCol.b, fillCol.a } },
        { "verticesVisible", annot.getVertexVisibility() },
        { "closed", annot.isClosed() },
        { "filled", annot.isFilled() },
        { "smoothed", annot.isSmoothed() },
        { "smoothingFactor", annot.getSmoothingFactor() },
        { "subjectPlaneNormal", { planeEq.x, planeEq.x, planeEq.z } },
        { "subjectPlaneOffset", planeEq[3] },
        { "subjectPlaneOrigin", { planeOr.x, planeOr.y, planeOr.z } },
        { "subjectPlaneAxes", { { planeAxes.first.x, planeAxes.first.y, planeAxes.first.z },
                                { planeAxes.second.x, planeAxes.second.y, planeAxes.second.z } } },
        { "polygon", annot.polygon() }
    };
}

void from_json( const json& /*j*/, Annotation& /*annot*/ )
{

}
