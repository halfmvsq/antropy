#ifndef CALLBACK_HANDLER_H
#define CALLBACK_HANDLER_H

#include "common/Types.h"

#include <uuid.h>

#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>


class AppData;
class GlfwWrapper;
class Rendering;
class View;

struct GLFWcursor;


/**
 * @brief Handles UI callbacks to the application
 */
class CallbackHandler
{
public:

    CallbackHandler( AppData&, GlfwWrapper&, Rendering& );
    ~CallbackHandler() = default;

    /**
     * @brief Clears all voxels in a segmentation, setting them to 0
     * @param segUid
     * @return
     */
    bool clearSegVoxels( const uuids::uuid& segUid );

    /**
     * @brief executeGridCutSegmentation
     * @param imageUid
     * @param seedSegUid
     * @param resultSegUid
     * @return
     */
    bool executeGridCutSegmentation(
            const uuids::uuid& imageUid,
            const uuids::uuid& seedSegUid,
            const uuids::uuid& resultSegUid );

    /**
     * @brief Move the crosshairs
     * @param windowLastPos
     * @param windowCurrPos
     */
    void doCrosshairsMove(
            const glm::vec2& windowLastPos,
            const glm::vec2& windowCurrPos );

    /**
     * @brief Scroll the crosshairs
     * @param windowCurrPos
     * @param scrollOffset
     */
    void doCrosshairsScroll(
            const glm::vec2& windowCurrPos,
            const glm::vec2& scrollOffset );

    /**
     * @brief Segment the image
     * @param windowLastPos
     * @param windowCurrPos
     * @param leftButton
     */
    void doSegment(
            const glm::vec2& windowLastPos,
            const glm::vec2& windowCurrPos,
            bool leftButton );

    /**
     * @brief doAnnotate
     * @param windowLastPos
     * @param windowCurrPos
     */
    void doAnnotate(
            const glm::vec2& windowLastPos,
            const glm::vec2& windowCurrPos );

    /**
     * @brief Adjust image window/level
     * @param windowLastPos
     * @param windowCurrPos
     */
    void doWindowLevel(
            const glm::vec2& windowLastPos,
            const glm::vec2& windowCurrPos );

    /**
     * @brief Adjust image opacity
     * @param windowLastPos
     * @param windowCurrPos
     */
    void doOpacity(
            const glm::vec2& windowLastPos,
            const glm::vec2& windowCurrPos );

    /**
     * @brief 2D translation of the camera (panning)
     * @param windowLastPos
     * @param windowCurrPos
     * @param windowStartPos
     */
    void doCameraTranslate2d(
            const glm::vec2& windowLastPos,
            const glm::vec2& windowCurrPos,
            const glm::vec2& windowStartPos );

    /**
     * @brief 2D rotation of the camera
     * @param windowLastPos
     * @param windowCurrPos
     * @param windowStartPos
     */
    void doCameraRotate2d(
            const glm::vec2& windowLastPos,
            const glm::vec2& windowCurrPos,
            const glm::vec2& windowStartPos );

    /**
     * @brief 3D rotation of the camera
     * @param windowLastPos
     * @param windowCurrPos
     * @param windowStartPos
     * @param constraint
     */
    void doCameraRotate3d(
            const glm::vec2& windowLastPos,
            const glm::vec2& windowCurrPos,
            const glm::vec2& windowStartPos,
            const std::optional<AxisConstraint>& constraint );

    /**
     * @brief 3D rotation of the camera
     * @param viewUid
     * @param camera_T_world_rotationDelta
     */
    void doCameraRotate3d(
            const uuids::uuid& viewUid,
            const glm::quat& camera_T_world_rotationDelta );

    /**
     * @brief Set the forward direction of a view and synchronize with its linked views
     * @param worldForwardDirection
     */
    void handleSetViewForwardDirection(
            const uuids::uuid& viewUid,
            const glm::vec3& worldForwardDirection );

    /**
     * @brief 2D zoom of the camera
     * @param windowLastPos
     * @param windowCurrPos
     * @param windowStartPos
     * @param zoomBehavior
     * @param syncZoomForAllViews
     */
    void doCameraZoomDrag(
            const glm::vec2& windowLastPos,
            const glm::vec2& windowCurrPos,
            const glm::vec2& windowStartPos,
            const ZoomBehavior& zoomBehavior,
            bool syncZoomForAllViews );

    /**
     * @brief doCameraZoomScroll
     * @param scrollOffset
     * @param windowStartPos
     * @param zoomBehavior
     * @param syncZoomForAllViews
     */
    void doCameraZoomScroll(
            const glm::vec2& scrollOffset,
            const glm::vec2& windowStartPos,
            const ZoomBehavior& zoomBehavior,
            bool syncZoomForAllViews );

    /**
     * @brief Image rotation
     * @param windowLastPos
     * @param windowCurrPos
     * @param windowStartPos
     * @param inPlane
     */
    void doImageRotate(
            const glm::vec2& windowLastPos,
            const glm::vec2& windowCurrPos,
            const glm::vec2& windowStartPos,
            bool inPlane );

    /**
     * @brief Image translation
     * @param windowLastPos
     * @param windowCurrPos
     * @param windowStartPos
     * @param inPlane
     */
    void doImageTranslate(
            const glm::vec2& windowLastPos,
            const glm::vec2& windowCurrPos,
            const glm::vec2& windowStartPos,
            bool inPlane );

    /**
     * @brief Image scale
     * @param windowLastPos
     * @param windowCurrPos
     * @param windowStartPos
     * @param constrainIsotropic
     */
    void doImageScale(
            const glm::vec2& windowLastPos,
            const glm::vec2& windowCurrPos,
            const glm::vec2& windowStartPos,
            bool constrainIsotropic );

    /**
     * @brief scrollViewSlice
     * @param windowCurrPos
     * @param numSlices
     */
    void scrollViewSlice(
            const glm::vec2& windowCurrPos,
            int numSlices );

    /**
     * @brief moveCrosshairsOnViewSlice
     * @param windowCurrPos
     * @param stepX
     * @param stepY
     */
    void moveCrosshairsOnViewSlice(
            const glm::vec2& windowCurrPos,
            int stepX, int stepY );

    /**
     * @brief Recenter all views on the selected images. Optionally recenter crosshairs there too.
     * @param recenterCrosshairs
     * @param recenterOnCurrentCrosshairsPos
     * @param resetObliqueOrientation
     */
    void recenterViews(
            const ImageSelection&,
            bool recenterCrosshairs,
            bool recenterOnCurrentCrosshairsPos,
            bool resetObliqueOrientation );

    /**
     * @brief Recenter one view
     * @param viewUid
     */
    void recenterView(
            const ImageSelection&,
            const uuids::uuid& viewUid );

    void flipImageInterpolation();
    void toggleImageVisibility();
    void toggleImageEdges();

    void decreaseSegOpacity();
    void toggleSegVisibility();
    void increaseSegOpacity();

    void cyclePrevLayout();
    void cycleNextLayout();
    void cycleOverlayAndUiVisibility();

    void cycleImageComponent( int i );

    void cycleForegroundSegLabel( int i );
    void cycleBackgroundSegLabel( int i );

    void cycleBrushSize( int i );

    bool showOverlays() const;
    void setShowOverlays( bool );

    void setMouseMode( MouseMode );

    void toggleFullScreenMode( bool forceWindowMode = false );

    /// Set whether manual transformation are locked on an image and all of its segmentations
    bool setLockManualImageTransformation( const uuids::uuid& imageUid, bool locked );

    /// Synchronize the lock on all segmentations of the image:
    bool syncManualImageTransformationOnSegs( const uuids::uuid& imageUid );


private:

    AppData& m_appData;
    GlfwWrapper& m_glfw;
    Rendering& m_rendering;

    /**
     * @brief When a view is hit by a mouse/pointer click, this structure is used to
     * return data about the view that was hit, including its ID, a reference to the view,
     * and the hit position in Clip space of the view.
     */
    struct ViewHitData
    {
        ViewHitData( View& v, uuids::uuid uid )
            : view( v ), viewUid( uid ) {}

        View& view;
        uuids::uuid viewUid;

        glm::vec2 windowClipLastPos;
        glm::vec2 windowClipCurrPos;

        glm::vec2 viewClipLastPos;
        glm::vec2 viewClipCurrPos;

        glm::vec4 worldLastPos;
        glm::vec4 worldCurrPos;

        glm::vec4 worldLastPos_offsetApplied;
        glm::vec4 worldCurrPos_offsetApplied;

        glm::vec3 worldFrontAxis;
    };

    std::optional<ViewHitData> getViewHit(
            const glm::vec2& windowPixelLastPos,
            const glm::vec2& windowPixelCurrPos,
            bool requireViewToBeActive,
            const std::optional<glm::vec2>& windowPixelStartPos = std::nullopt );

};

#endif // CALLBACK_HANDLER_H
