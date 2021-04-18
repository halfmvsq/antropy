#ifndef VECTOR_DRAWING_H
#define VECTOR_DRAWING_H

#include "logic/camera/CameraHelpers.h" // FrameBounds

#include <glm/fwd.hpp>
#include <glm/vec2.hpp>

#include <uuid.h>

#include <array>
#include <functional>
#include <list>
#include <optional>
#include <utility>
#include <vector>

class AppData;
class View;
class Viewport;

struct NVGcontext;


using ImageSegPairs = std::vector< std::pair< std::optional<uuids::uuid>, std::optional<uuids::uuid> > >;

/**
 * @brief Information needed for positioning a single anatomical label and the crosshair
 * that corresponds to this label.
 */
struct AnatomicalLabelPosInfo
{
    AnatomicalLabelPosInfo( int l ) : labelIndex( l ) {}

    /// The anatomical label index (0: L, 1: P, 2: S)
    int labelIndex;

    /// Mouse crosshairs center position (in Miewport space)
    glm::vec2 miewportXhairCenterPos{ 0.0f, 0.0f };

    /// Normalized direction vector of the label (in View Clip space)
    glm::vec2 viewClipDir{ 0.0f, 0.0f };

    /// Position of the label and the opposite label of its pair (in Miewport space)
    std::array<glm::vec2, 2> miewportLabelPositions;

    /// Positions of the crosshair-view intersections (in Miewport space).
    /// Equal to std::nullopt if there is no intersection of the crosshair with the
    /// view AABB for this label.
    std::optional< std::array<glm::vec2, 2> > miewportXhairPositions = std::nullopt;
};

void startNvgFrame(
        NVGcontext* nvg,
        const Viewport& windowVP );

void endNvgFrame( NVGcontext* nvg );

void drawLoadingOverlay(
        NVGcontext* nvg,
        const Viewport& windowVP );

void drawWindowOutline(
        NVGcontext* nvg,
        const Viewport& windowVP );

void drawViewOutline(
        NVGcontext* nvg,
        const camera::FrameBounds& miewportViewBounds,
        bool drawActiveOutline );

void drawImageViewIntersections(
        NVGcontext* nvg,
        const camera::FrameBounds& miewportViewBounds,
        AppData& appData,
        const View& view,
        const ImageSegPairs& I,
        bool renderInactiveImageIntersections );

std::list<AnatomicalLabelPosInfo>
computeAnatomicalLabelsForView(
        const View& view,
        const glm::mat4& world_T_refSubject );

std::list<AnatomicalLabelPosInfo>
computeAnatomicalLabelPosInfo(
        const camera::FrameBounds& miewportViewBounds,
        const Viewport& windowVP,
        const View& view,
        const glm::mat4& world_T_refSubject,
        const glm::vec3& worldCrosshairs );

void drawAnatomicalLabels(
        NVGcontext* nvg,
        const camera::FrameBounds& miewportViewBounds,
        const glm::vec4& color,
        const std::list<AnatomicalLabelPosInfo>& labelPosInfo );

void drawCircle(
        NVGcontext* nvg,
        const glm::vec2& miewportPos,
        float radius,
        const glm::vec4& fillColor,
        const glm::vec4& strokeColor,
        float strokeWidth );

void drawText(
        NVGcontext* nvg,
        const glm::vec2& miewportPos,
        const std::string& centeredString,
        const std::string& offsetString,
        const glm::vec4& textColor,
        float offset,
        float fontSizePixels );

void drawLandmarks(
        NVGcontext* nvg,
        const camera::FrameBounds& miewportViewBounds,
        const glm::vec3& worldCrosshairs,
        AppData& appData,
        const View& view,
        const ImageSegPairs& I );

void drawAnnotations(
        NVGcontext* nvg,
        const camera::FrameBounds& miewportViewBounds,
        const glm::vec3& worldCrosshairs,
        AppData& appData,
        const View& view,
        const ImageSegPairs& I );

void drawCrosshairs(
        NVGcontext* nvg,
        const camera::FrameBounds& miewportViewBounds,
        const View& view,
        const glm::vec4& color,
        const std::list<AnatomicalLabelPosInfo>& labelPosInfo );

#endif // VECTOR_DRAWING_H
