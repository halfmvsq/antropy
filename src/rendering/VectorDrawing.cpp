#include "rendering/VectorDrawing.h"

#include "common/DataHelper.h"
#include "common/MathFuncs.h"
#include "common/Viewport.h"

#include "image/Image.h"

#include "logic/app/Data.h"
#include "logic/camera/CameraHelpers.h"
#include "logic/camera/MathUtility.h"

#include "windowing/View.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>

#include <glad/glad.h>

#include <nanovg.h>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>


namespace
{

static const NVGcolor s_black( nvgRGBA( 0, 0, 0, 255 ) );
static const NVGcolor s_grey25( nvgRGBA( 63, 63, 63, 255 ) );
static const NVGcolor s_grey40( nvgRGBA( 102, 102, 102, 255 ) );
static const NVGcolor s_grey50( nvgRGBA( 127, 127, 127, 255 ) );
static const NVGcolor s_grey60( nvgRGBA( 153, 153, 153, 255 ) );
static const NVGcolor s_grey75( nvgRGBA( 195, 195, 195, 255 ) );
static const NVGcolor s_yellow( nvgRGBA( 255, 255, 0, 255 ) );
static const NVGcolor s_red( nvgRGBA( 255, 0, 0, 255 ) );

static const std::string ROBOTO_LIGHT( "robotoLight" );

static constexpr float sk_outlineStrokeWidth = 2.0f;

}


void startNvgFrame( NVGcontext* nvg, const Viewport& windowVP )
{
    if ( ! nvg ) return;

    nvgShapeAntiAlias( nvg, true );

    // Sets the composite operation. NVG_SOURCE_OVER is the default.
    nvgGlobalCompositeOperation( nvg, NVG_SOURCE_OVER );

    // Sets the composite operation with custom pixel arithmetic.
    // Note: The default compositing factors for NanoVG are
    // sfactor = NVG_ONE and dfactor = NVG_ONE_MINUS_SRC_ALPHA
    nvgGlobalCompositeBlendFunc( nvg, NVG_SRC_ALPHA, NVG_ONE_MINUS_SRC_ALPHA );

    nvgBeginFrame( nvg, windowVP.width(), windowVP.height(), windowVP.devicePixelRatio().x );
    nvgSave( nvg );
}

void endNvgFrame( NVGcontext* nvg )
{
    if ( ! nvg ) return;

    nvgRestore( nvg );
    nvgEndFrame( nvg );
}

void drawLoadingOverlay( NVGcontext* nvg, const Viewport& windowVP )
{
    /// @todo Progress indicators: https://github.com/ocornut/imgui/issues/1901

    static const NVGcolor s_greyTextColor( nvgRGBA( 190, 190, 190, 255 ) );
    static const NVGcolor s_greyShadowColor( nvgRGBA( 64, 64, 64, 255 ) );

    static constexpr float sk_arcAngle = 1.0f / 16.0f * NVG_PI;
    static const std::string sk_loadingText = "Loading images...";

    nvgFontSize( nvg, 64.0f );
    nvgFontFace( nvg, ROBOTO_LIGHT.c_str() );

    nvgTextAlign( nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE );

    nvgFontBlur( nvg, 2.0f );
    nvgFillColor( nvg, s_greyShadowColor );
    nvgText( nvg, 0.5f * windowVP.width(), 0.5f * windowVP.height(), sk_loadingText.c_str(), nullptr );

    nvgFontBlur( nvg, 0.0f );
    nvgFillColor( nvg, s_greyTextColor );
    nvgText( nvg, 0.5f * windowVP.width(), 0.5f * windowVP.height(), sk_loadingText.c_str(), nullptr );

    const auto ms = std::chrono::duration_cast< std::chrono::milliseconds >(
                std::chrono::system_clock::now().time_since_epoch() );

    const float C = 2.0f * NVG_PI * static_cast<float>( ms.count() % 1000 ) / 1000.0f;
    const float radius = windowVP.width() / 16.0f;

    nvgStrokeWidth( nvg, 8.0f );
    nvgStrokeColor( nvg, s_greyTextColor );

    nvgBeginPath( nvg );
    nvgArc( nvg, 0.5f * windowVP.width(), 0.75f * windowVP.height(), radius, sk_arcAngle + C, C, NVG_CCW );
    nvgStroke( nvg );
}

void drawWindowOutline(
        NVGcontext* nvg,
        const Viewport& windowVP )
{
    static constexpr float k_pad = 1.0f;

    // Outline around window
    nvgStrokeWidth( nvg, sk_outlineStrokeWidth );
    nvgStrokeColor( nvg, s_grey50 );
//    nvgStrokeColor( nvg, s_red );

    nvgBeginPath( nvg );
    nvgRoundedRect( nvg, k_pad, k_pad, windowVP.width() - 2.0f * k_pad, windowVP.height() - 2.0f * k_pad, 3.0f );
    nvgStroke( nvg );

    //        nvgStrokeWidth( nvg, 2.0f );
    //        nvgStrokeColor( nvg, s_grey50 );
    //        nvgRect( nvg, pad, pad, windowVP.width() - pad, windowVP.height() - pad );
    //        nvgStroke( nvg );

    //        nvgStrokeWidth( nvg, 1.0f );
    //        nvgStrokeColor( nvg, s_grey60 );
    //        nvgRect( nvg, pad, pad, windowVP.width() - pad, windowVP.height() - pad );
    //        nvgStroke( nvg );
}

void drawViewOutline(
        NVGcontext* nvg,
        const camera::FrameBounds& miewportViewBounds,
        bool drawActiveOutline )
{
    static constexpr float k_padOuter = 0.0f;
//    static constexpr float k_padInner = 2.0f;
    static constexpr float k_padActive = 3.0f;

    auto drawRectangle = [&nvg, &miewportViewBounds]
            ( float pad, float width, const NVGcolor& color )
    {
        nvgStrokeWidth( nvg, width );
        nvgStrokeColor( nvg, color );

        nvgBeginPath( nvg );

        nvgRect( nvg,
                 miewportViewBounds.bounds.xoffset + pad,
                 miewportViewBounds.bounds.yoffset + pad,
                 miewportViewBounds.bounds.width - 2.0f * pad,
                 miewportViewBounds.bounds.height - 2.0f * pad );

        nvgStroke( nvg );
    };


    if ( drawActiveOutline )
    {
        drawRectangle( k_padActive, 2.0f, s_yellow );
    }

    // View outline:
    drawRectangle( k_padOuter, sk_outlineStrokeWidth, s_grey50 );
//    drawRectangle( k_padInner, 1.0f, s_grey60 );
}

void drawImageViewIntersections(
        NVGcontext* nvg,
        const camera::FrameBounds& miewportViewBounds,
        AppData& appData,
        const View& view,
        const ImageSegPairs& I )
{
    // Line segment stipple length in pixels
    static constexpr float sk_stippleLen = 16.0f;

    nvgLineCap( nvg, NVG_BUTT );
    nvgLineJoin( nvg, NVG_MITER );

    startNvgFrame( nvg, appData.windowData().viewport() ); /*** START FRAME ***/

    nvgScissor( nvg, miewportViewBounds.viewport[0], miewportViewBounds.viewport[1],
            miewportViewBounds.viewport[2], miewportViewBounds.viewport[3] );

    // Render border for each image
    for ( const auto& imgSegPair : I )
    {
        if ( ! imgSegPair.first ) continue;
        const auto imgUid = *( imgSegPair.first );
        const Image* img = appData.image( imgUid );
        if ( ! img ) continue;

        auto worldIntersections = view.computeImageSliceIntersection(
                    img, appData.state().worldCrosshairs() );

        if ( ! worldIntersections ) continue;

        // The last point is the centroid of the intersection. Ignore the centroid and replace it with a
        // duplicate of the first point. We need to double-up that point in order for line stippling to
        // work correctly. Also, no need to close the path with nvgClosePath if the last point is duplicated.
        worldIntersections->at( 6 ) = worldIntersections->at( 0 );

        const glm::vec3 color = img->settings().borderColor();
        const float opacity = static_cast<float>( img->settings().visibility() * img->settings().opacity() );

        nvgStrokeColor( nvg, nvgRGBAf( color.r, color.g, color.b, opacity ) );

        const auto activeImageUid = appData.activeImageUid();
        const bool isActive = ( activeImageUid && ( *activeImageUid == imgUid ) );

        nvgStrokeWidth( nvg, isActive ? 2.0f : 1.0f );

        glm::vec2 lastPos;

        nvgBeginPath( nvg );

        for ( size_t i = 0; i < worldIntersections->size(); ++i )
        {                   
            const glm::vec2 currPos = camera::miewport_T_world(
                        appData.windowData().viewport(),
                        view.camera(),
                        view.windowClip_T_viewClip(),
                        worldIntersections->at( i ) );

            if ( 0 == i )
            {
                // Move pen to the first point:
                nvgMoveTo( nvg, currPos.x, currPos.y );
                lastPos = currPos;
                continue;
            }

            if ( isActive )
            {
                // The active image gets a stippled line pattern
                const float dist = glm::distance( lastPos, currPos );
                const uint32_t numLines = static_cast<uint32_t>( dist / sk_stippleLen );

                if ( 0 == numLines )
                {
                    // At a minimum, draw one stipple line:
                    nvgLineTo( nvg, currPos.x, currPos.y );
                }

                for ( uint32_t j = 1; j <= numLines; ++j )
                {
                    const float t = static_cast<float>( j ) / static_cast<float>( numLines );
                    const glm::vec2 pos = lastPos + t * ( currPos - lastPos );

                    // To create the stipple pattern, alternate drawing lines and
                    // moving the pen on odd/even values of j:
                    if ( j % 2 )
                    {
                        nvgLineTo( nvg, pos.x, pos.y );
                    }
                    else
                    {
                        nvgMoveTo( nvg, pos.x, pos.y );
                    }
                }
            }
            else
            {
                // Non-active images get solid lines
                nvgLineTo( nvg, currPos.x, currPos.y );
            }

            lastPos = currPos;
        }

        nvgStroke( nvg );
    }

    nvgResetScissor( nvg );

    endNvgFrame( nvg ); /*** END FRAME ***/
}

std::list<AnatomicalLabelPosInfo>
computeAnatomicalLabelsForView(
        const View& view,
        const glm::mat4& world_T_refSubject )
{
    // Shortcuts for the three orthogonal anatomical directions
    static constexpr int L = 0;
    static constexpr int P = 1;
    static constexpr int S = 2;

    // Visibility and directions of the labels L, P, S in View Clip/NDC space:
    std::list<AnatomicalLabelPosInfo> labels;

    // The reference subject's left, posterior, and superior directions in Camera space.
    // Columns 0, 1, and 2 of the matrix correspond to left, posterior, and superior, respectively.
    const glm::mat3 axes = math::computeSubjectAxesInCamera(
                glm::mat3{ view.camera().camera_T_world() },
                glm::mat3{ world_T_refSubject } );

    const glm::mat3 axesAbs{ glm::abs( axes[0] ), glm::abs( axes[1] ), glm::abs( axes[2] ) };
    const glm::mat3 axesSgn{ glm::sign( axes[0] ), glm::sign( axes[1] ), glm::sign( axes[2] ) };

    // Render the two sets of labels that are closest to the view plane:
    if ( axesAbs[L].z > axesAbs[P].z && axesAbs[L].z > axesAbs[S].z )
    {
        labels.emplace_back( AnatomicalLabelPosInfo{ P } );
        labels.emplace_back( AnatomicalLabelPosInfo{ S } );
    }
    else if ( axesAbs[P].z > axesAbs[L].z && axesAbs[P].z > axesAbs[S].z )
    {
        labels.emplace_back( AnatomicalLabelPosInfo{ L } );
        labels.emplace_back( AnatomicalLabelPosInfo{ S } );
    }
    else if ( axesAbs[S].z > axesAbs[L].z && axesAbs[S].z > axesAbs[P].z )
    {
        labels.emplace_back( AnatomicalLabelPosInfo{ L } );
        labels.emplace_back( AnatomicalLabelPosInfo{ P } );
    }

    // Render the translation vectors for the L (0), P (1), and S (2) labels:
    for ( auto& label : labels )
    {
        const int i = label.labelIndex;

        label.viewClipDir = ( axesAbs[i].x > 0.0f && axesAbs[i].y / axesAbs[i].x <= 1.0f )
                ? glm::vec2{ axesSgn[i].x, axesSgn[i].y * axesAbs[i].y / axesAbs[i].x }
                : glm::vec2{ axesSgn[i].x * axesAbs[i].x / axesAbs[i].y, axesSgn[i].y };
    }

    return labels;
}

std::list<AnatomicalLabelPosInfo>
computeAnatomicalLabelPosInfo(
        const camera::FrameBounds& miewportViewBounds,
        const Viewport& windowVP,
        const View& view,
        const glm::mat4& world_T_refSubject,
        const glm::vec3& worldCrosshairs )
{
    // Compute intersections of the anatomical label ray with the view box:
    static constexpr bool sk_doBothLabelDirs = false;

    // Compute intersections of the crosshair ray with the view box:
    static constexpr bool sk_doBothXhairDirs = true;

    const glm::mat4 miewport_T_viewClip =
            camera::miewport_T_viewport( windowVP.height() ) *
            camera::viewport_T_windowClip( windowVP ) *
            view.windowClip_T_viewClip();

    const glm::mat3 miewport_T_viewClip_IT = glm::inverseTranspose( glm::mat3{ miewport_T_viewClip } );

    auto labelPosInfo = computeAnatomicalLabelsForView( view, world_T_refSubject );

    const float aspectRatio = miewportViewBounds.bounds.width / miewportViewBounds.bounds.height;

    const glm::vec2 aspectRatioScale = ( aspectRatio < 1.0f )
            ? glm::vec2{ aspectRatio, 1.0f }
            : glm::vec2{ 1.0f, 1.0f / aspectRatio };

    const glm::vec2 miewportMinCorner( miewportViewBounds.bounds.xoffset, miewportViewBounds.bounds.yoffset );
    const glm::vec2 miewportSize( miewportViewBounds.bounds.width, miewportViewBounds.bounds.height );
    const glm::vec2 miewportCenter = miewportMinCorner + 0.5f * miewportSize;

    glm::vec4 viewClipXhairPos = camera::clip_T_world( view.camera() ) * glm::vec4{ worldCrosshairs, 1.0f };
    viewClipXhairPos /= viewClipXhairPos.w;

    glm::vec4 miewportXhairPos = miewport_T_viewClip * viewClipXhairPos;
    miewportXhairPos /= miewportXhairPos.w;

    for ( auto& label : labelPosInfo )
    {
        const glm::vec3 viewClipXhairDir{ label.viewClipDir.x, label.viewClipDir.y, 0.0f };

        label.miewportXhairCenterPos = glm::vec2{ miewportXhairPos };

        glm::vec2 miewportXhairDir{ miewport_T_viewClip_IT * viewClipXhairDir };
        miewportXhairDir.x *= aspectRatioScale.x;
        miewportXhairDir.y *= aspectRatioScale.y;
        miewportXhairDir = glm::normalize( miewportXhairDir );

        // Intersections for the positive label (L, P, or S):
        const auto posLabelHits = math::computeRayAABoxIntersections(
                    miewportCenter,
                    miewportXhairDir,
                    miewportMinCorner,
                    miewportSize,
                    sk_doBothLabelDirs );

        // Intersections for the negative label (R, A, or I):
        const auto negLabelHits = math::computeRayAABoxIntersections(
                    miewportCenter,
                    -miewportXhairDir,
                    miewportMinCorner,
                    miewportSize,
                    sk_doBothLabelDirs );

        if ( 1 != posLabelHits.size() || 1 != negLabelHits.size() )
        {
            spdlog::warn( "Expected two intersections when computing anatomical label positions for view. "
                          "Got {} and {} intersections in the positive and negative directions, respectively.",
                          posLabelHits.size(), negLabelHits.size() );
            continue;
        }

        label.miewportLabelPositions = std::array<glm::vec2, 2>{ posLabelHits[0], negLabelHits[0] };

        const auto crosshairHits = math::computeRayAABoxIntersections(
                    label.miewportXhairCenterPos,
                    miewportXhairDir,
                    miewportMinCorner,
                    miewportSize,
                    sk_doBothXhairDirs );

        if ( 2 != crosshairHits.size() )
        {
            // Only render crosshairs when there are two intersections with the view box:
            label.miewportXhairPositions = std::nullopt;
        }
        else
        {
            label.miewportXhairPositions = std::array<glm::vec2, 2>{ crosshairHits[0], crosshairHits[1] };
        }
    }

    return labelPosInfo;
}

void drawAnatomicalLabels(
        NVGcontext* nvg,
        const camera::FrameBounds& miewportViewBounds,
        const glm::vec4& color,
        const std::list<AnatomicalLabelPosInfo>& labelPosInfo )
{
    static constexpr float sk_fontMult = 0.03f;

    // Anatomical direction labels
    static const std::array< std::string, 6 > sk_labels{ "L", "P", "S", "R", "A", "I" };

    const glm::vec2 miewportMinCorner( miewportViewBounds.bounds.xoffset, miewportViewBounds.bounds.yoffset );
    const glm::vec2 miewportSize( miewportViewBounds.bounds.width, miewportViewBounds.bounds.height );
    const glm::vec2 miewportMaxCorner = miewportMinCorner + miewportSize;

    // Clip against the view bounds, even though not strictly necessary with how lines are defined
    nvgScissor( nvg, miewportViewBounds.viewport[0], miewportViewBounds.viewport[1],
            miewportViewBounds.viewport[2], miewportViewBounds.viewport[3] );

    const float fontSizePixels = sk_fontMult *
            std::min( miewportViewBounds.bounds.width, miewportViewBounds.bounds.height );

    // For inward shift of the labels:
    const glm::vec2 inwardFontShift{ 0.8f * fontSizePixels, 0.8f * fontSizePixels };

    // For downward shift of the labels:
    const glm::vec2 vertFontShift{ 0.0f, 0.35f * fontSizePixels };

    nvgFontSize( nvg, fontSizePixels );
    nvgFontFace( nvg, ROBOTO_LIGHT.c_str() );
    nvgTextAlign( nvg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE );

    // Render the translation vectors for the L (0), P (1), and S (2) labels:
    for ( const auto& label : labelPosInfo )
    {
        const glm::vec2 miewportPositivePos = glm::clamp(
                    label.miewportLabelPositions[0],
                    miewportMinCorner + inwardFontShift, miewportMaxCorner - inwardFontShift ) + vertFontShift;

        const glm::vec2 miewportNegativePos = glm::clamp(
                    label.miewportLabelPositions[1],
                    miewportMinCorner + inwardFontShift, miewportMaxCorner - inwardFontShift ) + vertFontShift;

        nvgFontBlur( nvg, 2.0f );
        nvgFillColor( nvg, s_black );
        nvgText( nvg, miewportPositivePos.x, miewportPositivePos.y, sk_labels[label.labelIndex].c_str(), nullptr );
        nvgText( nvg, miewportNegativePos.x, miewportNegativePos.y, sk_labels[label.labelIndex + 3].c_str(), nullptr );

        nvgFontBlur( nvg, 0.0f );
        nvgFillColor( nvg, nvgRGBAf( color.r, color.g, color.b, color.a ) );
        nvgText( nvg, miewportPositivePos.x, miewportPositivePos.y, sk_labels[label.labelIndex].c_str(), nullptr );
        nvgText( nvg, miewportNegativePos.x, miewportNegativePos.y, sk_labels[label.labelIndex + 3].c_str(), nullptr );
    }

    nvgResetScissor( nvg );
}

void drawCircle(
        NVGcontext* nvg,
        const glm::vec2& miewportPos,
        float radius,
        const glm::vec4& fillColor,
        const glm::vec4& strokeColor,
        float strokeWidth )
{
    nvgStrokeWidth( nvg, strokeWidth );
    nvgStrokeColor( nvg, nvgRGBAf( strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a ) );
    nvgFillColor( nvg, nvgRGBAf( fillColor.r, fillColor.g, fillColor.b, fillColor.a ) );

    nvgBeginPath( nvg );
    nvgCircle( nvg, miewportPos.x, miewportPos.y, radius );

    nvgStroke( nvg );
    nvgFill( nvg );
}

void drawText(
        NVGcontext* nvg,
        const glm::vec2& miewportPos,
        const std::string& centeredString,
        const std::string& offsetString,
        const glm::vec4& textColor,
        float offset,
        float fontSizePixels )
{
    nvgFontFace( nvg, ROBOTO_LIGHT.c_str() );

    // Draw centered text
    if ( ! centeredString.empty() )
    {
        nvgFontSize( nvg, 1.0f * fontSizePixels );
        nvgTextAlign( nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE );

        nvgFontBlur( nvg, 3.0f );
        nvgFillColor( nvg, nvgRGBAf( 0.0f, 0.0f, 0.0f, textColor.a ) );
        nvgText( nvg, miewportPos.x, miewportPos.y, centeredString.c_str(), nullptr );

        nvgFontBlur( nvg, 0.0f );
        nvgFillColor( nvg, nvgRGBAf( textColor.r, textColor.g, textColor.b, textColor.a ) );
        nvgText( nvg, miewportPos.x, miewportPos.y, centeredString.c_str(), nullptr );
    }

    // Draw offset text
    if ( ! offsetString.empty() )
    {
        nvgFontSize( nvg, 1.15f * fontSizePixels );
        nvgTextAlign( nvg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP );

        nvgFontBlur( nvg, 3.0f );
        nvgFillColor( nvg, nvgRGBAf( 0.0f, 0.0f, 0.0f, textColor.a ) );
        nvgText( nvg, offset + miewportPos.x, offset + miewportPos.y, offsetString.c_str(), nullptr );

        nvgFontBlur( nvg, 0.0f );
        nvgFillColor( nvg, nvgRGBAf( textColor.r, textColor.g, textColor.b, textColor.a ) );
        nvgText( nvg, offset + miewportPos.x, offset + miewportPos.y, offsetString.c_str(), nullptr );
    }
}

void drawLandmarks(
        NVGcontext* nvg,
        const camera::FrameBounds& miewportViewBounds,
        const glm::vec3& worldCrosshairs,
        AppData& appData,
        const View& view,
        const ImageSegPairs& I )
{
    static constexpr float sk_minSize = 4.0f;
    static constexpr float sk_maxSize = 128.0f;

    startNvgFrame( nvg, appData.windowData().viewport() ); /*** START FRAME ***/

    // Clip against the view bounds
    nvgScissor( nvg, miewportViewBounds.viewport[0], miewportViewBounds.viewport[1],
            miewportViewBounds.viewport[2], miewportViewBounds.viewport[3] );

    const float strokeWidth = appData.renderData().m_globalLandmarkParams.strokeWidth;

    const glm::vec3 worldViewNormal = camera::worldDirection( view.camera(), Directions::View::Back );
    const glm::vec4 worldViewPlane = math::makePlane( worldViewNormal, worldCrosshairs );

    // Render landmarks for each image
    for ( const auto& imgSegPair : I )
    {
        if ( ! imgSegPair.first )
        {
            // Non-existent image
            continue;
        }

        const auto imgUid = *( imgSegPair.first );
        const Image* img = appData.image( imgUid );

        if ( ! img )
        {
            spdlog::error( "Null image {} when rendering landmarks", imgUid );
            continue;
        }

        // Don't render landmarks for invisible image:
        /// @todo Need to properly manage global visibility vs. visibility for just one component
        if ( ! img->settings().globalVisibility() ||
             ( 1 == img->header().numComponentsPerPixel() && ! img->settings().visibility() ) )
        {
            continue;
        }

        const auto lmGroupUids = appData.imageToLandmarkGroupUids( imgUid );
        if ( lmGroupUids.empty() ) continue;

        // Slice spacing of the image along the view normal
        const float sliceSpacing = data::sliceScrollDistance( -worldViewNormal, *img );

        for ( const auto& lmGroupUid : lmGroupUids )
        {
            const LandmarkGroup* lmGroup = appData.landmarkGroup( lmGroupUid );
            if ( ! lmGroup )
            {
                spdlog::error( "Null landmark group for image {}", imgUid );
                continue;
            }

            if ( ! lmGroup->getVisibility() )
            {
                continue;
            }

            // Matrix that transforms landmark position from either Voxel or Subject to World space.
            const glm::mat4 world_T_landmark = ( lmGroup->getInVoxelSpace() )
                    ? img->transformations().worldDef_T_pixel()
                    : img->transformations().worldDef_T_subject();

            const float minDim = std::min( miewportViewBounds.bounds.width, miewportViewBounds.bounds.height );
            const float pixelsMaxLmSize = glm::clamp( lmGroup->getRadiusFactor() * minDim, sk_minSize, sk_maxSize );

            for ( const auto& p : lmGroup->getPoints() )
            {
                const size_t index = p.first;
                const PointRecord<glm::vec3>& point = p.second;

                if ( ! point.getVisibility() ) continue;

                // Put landmark into World space
                const glm::vec4 worldLmPos = world_T_landmark * glm::vec4{ point.getPosition(), 1.0f };
                const glm::vec3 worldLmPos3 = glm::vec3{ worldLmPos / worldLmPos.w };

                // Landmark must be within a distance of half the image slice spacing along the
                // direction of the view to be rendered in the view
                const float distLmToPlane = std::abs( math::signedDistancePointToPlane( worldLmPos3, worldViewPlane ) );

                // Maximum distance beyond which the landmark is not rendered:
                const float maxDist = 0.5f * sliceSpacing;

                if ( distLmToPlane >= maxDist )
                {
                    continue;
                }

                const glm::vec2 miewportPos = camera::miewport_T_world(
                            appData.windowData().viewport(),
                            view.camera(),
                            view.windowClip_T_viewClip(),
                            worldLmPos3 );

                const bool inView = ( miewportViewBounds.bounds.xoffset < miewportPos.x &&
                                      miewportViewBounds.bounds.yoffset < miewportPos.y &&
                                      miewportPos.x < miewportViewBounds.bounds.xoffset + miewportViewBounds.bounds.width &&
                                      miewportPos.y < miewportViewBounds.bounds.yoffset + miewportViewBounds.bounds.height );

                if ( ! inView ) continue;

                // Use the landmark group color if defined
                const bool lmGroupColorOverride = lmGroup->getColorOverride();
                const glm::vec3 lmGroupColor = lmGroup->getColor();
                const float lmGroupOpacity = lmGroup->getOpacity();

                // Non-premult. alpha:
                const glm::vec4 fillColor{ ( lmGroupColorOverride )
                            ? lmGroupColor : point.getColor(),
                            lmGroupOpacity };

                /// @todo If landmark is selected, then highlight it here:
                const float strokeOpacity = 1.0f - std::pow( ( lmGroupOpacity - 1.0f ), 2.0f );

                const glm::vec4 strokeColor{ ( lmGroupColorOverride )
                            ? lmGroupColor : point.getColor(),
                            strokeOpacity };

                // Landmark radius depends on distance of the view plane from the landmark center
                const float radius = pixelsMaxLmSize *
                        std::sqrt( std::abs( 1.0f - std::pow( distLmToPlane / maxDist, 2.0f ) ) );

                drawCircle( nvg, miewportPos, radius, fillColor, strokeColor, strokeWidth );

                const bool renderIndices = lmGroup->getRenderLandmarkIndices();
                const bool renderNames = lmGroup->getRenderLandmarkNames();

                if ( renderIndices || renderNames )
                {
                    const float textOffset = radius + 0.7f;
                    const float textSize = 0.9f * pixelsMaxLmSize;

                    std::string indexString = ( renderIndices ) ? std::to_string( index ) : "";
                    std::string nameString = ( renderNames ) ? point.getName() : "";

                    // Non premult. alpha:
                    const auto lmGroupTextColor = lmGroup->getTextColor();
                    const glm::vec4 textColor{ ( lmGroupTextColor )
                                ? *lmGroupTextColor : fillColor, lmGroupOpacity };

                    drawText( nvg, miewportPos, indexString, nameString, textColor, textOffset, textSize );
                }
            }
        }
    }

    nvgResetScissor( nvg );

    endNvgFrame( nvg ); /*** END FRAME ***/
}

void drawAnnotations(
        NVGcontext* nvg,
        const camera::FrameBounds& miewportViewBounds,
        const glm::vec3& worldCrosshairs,
        AppData& appData,
        const View& view,
        const ImageSegPairs& I )
{
    nvgLineCap( nvg, NVG_BUTT );
    nvgLineJoin( nvg, NVG_MITER );

    startNvgFrame( nvg, appData.windowData().viewport() ); /*** START FRAME ***/

    nvgScissor( nvg, miewportViewBounds.viewport[0], miewportViewBounds.viewport[1],
            miewportViewBounds.viewport[2], miewportViewBounds.viewport[3] );

    const glm::vec3 worldViewNormal = camera::worldDirection( view.camera(), Directions::View::Back );

    // Render annotations for each image
    for ( const auto& imgSegPair : I )
    {
        if ( ! imgSegPair.first )
        {
            continue; // Non-existent image
        }

        const auto imgUid = *( imgSegPair.first );
        const Image* img = appData.image( imgUid );

        if ( ! img )
        {
            spdlog::error( "Null image {} when rendering annotations", imgUid );
            continue;
        }

        // Don't render landmarks for invisible image:
        /// @todo Need to properly manage global visibility vs. visibility for just one component
        if ( ! img->settings().globalVisibility() ||
             ( 1 == img->header().numComponentsPerPixel() && ! img->settings().visibility() ) )
        {
            continue;
        }

//        spdlog::trace( "Rendering annotations for image {}", imgUid );

        // Compute plane equation in image Subject space:
        /// @todo Pull this out into a MathHelper function
        const glm::mat4& subject_T_world = img->transformations().subject_T_worldDef();
        const glm::mat4& world_T_subject = img->transformations().worldDef_T_subject();
        const glm::mat3& subject_T_world_IT = img->transformations().subject_T_worldDef_invTransp();

        const glm::vec3 subjectPlaneNormal{ subject_T_world_IT * worldViewNormal };

        glm::vec4 subjectPlanePoint = subject_T_world * glm::vec4{ worldCrosshairs, 1.0f };
        subjectPlanePoint /= subjectPlanePoint.w;

        const glm::vec4 subjectPlaneEquation = math::makePlane(
                    subjectPlaneNormal, glm::vec3{ subjectPlanePoint } );

        // Slice spacing of the image along the view normal is the plane distance threshold
        // for annotation searching:
        const float sliceSpacing = data::sliceScrollDistance( -worldViewNormal, *img );

//        spdlog::trace( "Finding annotations for plane {} with distance threshold {}",
//                       glm::to_string( subjectPlaneEquation ), sliceSpacing );

        const auto annotUids = data::findAnnotationsForImage(
                    appData, imgUid, subjectPlaneEquation, sliceSpacing );

        if ( annotUids.empty() ) continue;

        const Annotation* annot = appData.annotation( annotUids[0] );
        if ( ! annot ) continue;

        const bool visible = ( img->settings().visibility() && annot->getVisibility() );
        if ( ! visible ) continue;

//        spdlog::trace( "Found annotation {}", *annotUid );

        // Annotation vertices in Subject space:
        const std::vector<glm::vec2>& subjectPlaneVertices = annot->getBoundaryVertices( 0 );

        if ( subjectPlaneVertices.empty() ) continue;

        /// @todo Should annotation opacity be modulated with image opacity?
        /// Landmarks opacity is not.
        const glm::vec3 color = annot->getColor();
        const float opacity = annot->getOpacity() * static_cast<float>( img->settings().opacity() );

        nvgStrokeColor( nvg, nvgRGBAf( color.r, color.g, color.b, opacity ) );
        nvgStrokeWidth( nvg, annot->getLineThickness() );

        nvgBeginPath( nvg );

        for ( size_t i = 0; i < subjectPlaneVertices.size(); ++i )
        {
            const glm::vec3 subjectPos = annot->unprojectFromAnnotationPlaneToSubjectPoint( subjectPlaneVertices[i] );
            const glm::vec4 worldPos = world_T_subject * glm::vec4{ subjectPos, 1.0f };

            const glm::vec2 miewportPos = camera::miewport_T_world(
                        appData.windowData().viewport(),
                        view.camera(),
                        view.windowClip_T_viewClip(),
                        glm::vec3{ worldPos / worldPos.w } );

            if ( 0 == i )
            {
                // Move pen to the first point:
                nvgMoveTo( nvg, miewportPos.x, miewportPos.y );
                continue;
            }
            else
            {
                nvgLineTo( nvg, miewportPos.x, miewportPos.y );
            }

//            spdlog::trace( "Point i = {}: {}", i, glm::to_string( mousePos ) );
        }

        nvgStroke( nvg );
    }

    nvgResetScissor( nvg );

    endNvgFrame( nvg ); /*** END FRAME ***/
}

void drawCrosshairs(
        NVGcontext* nvg,
        const camera::FrameBounds& miewportViewBounds,
        const View& view,
        const glm::vec4& color,
        const std::list<AnatomicalLabelPosInfo>& labelPosInfo )
{
    // Line segment stipple length in pixels
    static constexpr float sk_stippleLen = 8.0f;

    nvgLineCap( nvg, NVG_BUTT );
    nvgLineJoin( nvg, NVG_MITER );

    const auto& offset = view.offsetSetting();

    // Is the view offset from the crosshairs position?
    const bool viewIsOffset =
            ( ViewOffsetMode::RelativeToRefImageScrolls == offset.m_offsetMode &&
              0 != offset.m_relativeOffsetSteps ) ||

            ( ViewOffsetMode::RelativeToImageScrolls == offset.m_offsetMode &&
              0 != offset.m_relativeOffsetSteps ) ||

            ( ViewOffsetMode::Absolute == offset.m_offsetMode &&
              glm::epsilonNotEqual( offset.m_absoluteOffset, 0.0f, glm::epsilon<float>() ) );

    if ( viewIsOffset )
    {
        // Offset views get thinner, transparent crosshairs
        nvgStrokeWidth( nvg, 1.0f );
        nvgStrokeColor( nvg, nvgRGBAf( color.r, color.g, color.b, 0.5f * color.a ) );
    }
    else
    {
        nvgStrokeWidth( nvg, 2.0f );
        nvgStrokeColor( nvg, nvgRGBAf( color.r, color.g, color.b, color.a ) );
    }

    // Clip against the view bounds, even though not strictly necessary with how lines are defined
    nvgScissor( nvg, miewportViewBounds.viewport[0], miewportViewBounds.viewport[1],
            miewportViewBounds.viewport[2], miewportViewBounds.viewport[3] );

    for ( const auto& pos : labelPosInfo )
    {
        if ( ! pos.miewportXhairPositions )
        {
            // Only render crosshairs when there are two intersections with the view box:
            continue;
        }

        const auto& hits = *( pos.miewportXhairPositions );

        if ( camera::CameraType::Oblique != view.cameraType() )
        {
            // Orthogonal views get solid crosshairs:
            nvgBeginPath( nvg );
            nvgMoveTo( nvg, hits[0].x, hits[0].y );
            nvgLineTo( nvg, hits[1].x, hits[1].y );
            nvgStroke( nvg );
        }
        else
        {
            // Oblique views get stippled crosshairs:
            for ( int line = 0; line < 2; ++line )
            {
                const uint32_t numLines = glm::distance( hits[line], pos.miewportXhairCenterPos ) / sk_stippleLen;

                nvgBeginPath( nvg );
                for ( uint32_t i = 0; i <= numLines; ++i )
                {
                    const float t = static_cast<float>( i ) / static_cast<float>( numLines );
                    const glm::vec2 p = pos.miewportXhairCenterPos + t * ( hits[line] - pos.miewportXhairCenterPos );

                    if ( i % 2 ) nvgLineTo( nvg, p.x, p.y ); // when i odd
                    else nvgMoveTo( nvg, p.x, p.y ); // when i even
                }
                nvgStroke( nvg );
            }
        }
    }

    nvgResetScissor( nvg );
}

/// @see https://community.vcvrack.com/t/advanced-nanovg-custom-label/6769/21
//void draw (const DrawArgs &args) override {
//  // draw the text
//  float bounds[4];
//  const char* txt = "DEMO";
//  nvgTextAlign(args.vg, NVG_ALIGN_MIDDLE);
//      nvgTextBounds(args.vg, 0, 0, txt, NULL, bounds);
//      nvgBeginPath(args.vg);
//  nvgFillColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0xff));
//      nvgRect(args.vg, bounds[0], bounds[1], bounds[2]-bounds[0], bounds[3]-bounds[1]);
//      nvgFill(args.vg);
//  nvgFillColor(args.vg, nvgRGBA(0x00, 0xff, 0x00, 0xff));
//  nvgText(args.vg, 0, 0, txt, NULL);
//}
