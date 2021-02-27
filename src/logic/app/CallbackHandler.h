#ifndef CALLBACK_HANDLER_H
#define CALLBACK_HANDLER_H

#include "common/Types.h"

#include <uuid.h>

#include <glm/vec2.hpp>


class AppData;
class Rendering;

struct GLFWcursor;


class CallbackHandler
{
public:

    CallbackHandler( AppData&, Rendering& );
    ~CallbackHandler() = default;

    /// Clears all voxels in the segmentation, setting them to 0
    bool clearSegVoxels( const uuids::uuid& segUid );

    bool executeGridCutSegmentation(
            const uuids::uuid& imageUid,
            const uuids::uuid& seedSegUid,
            const uuids::uuid& resultSegUid );

    /// Move the crosshairs
    void doCrosshairsMove(
            const glm::vec2& lastWindowPos,
            const glm::vec2& currWindowPos );

    /// Scroll the crosshairs
    void doCrosshairsScroll(
            const glm::vec2& currWindowPos,
            const glm::vec2& scrollOffset );

    /// Segment the image
    void doSegment(
            const glm::vec2& lastWindowPos,
            const glm::vec2& currWindowPos,
            bool leftButton );

    /// Annotate
    void doAnnotate(
            const glm::vec2& lastWindowPos,
            const glm::vec2& currWindowPos );

    /// Adjust image window/level
    void doWindowLevel(
            const glm::vec2& lastWindowPos,
            const glm::vec2& currWindowPos );

    /// Adjust image opacity
    void doOpacity(
            const glm::vec2& lastWindowPos,
            const glm::vec2& currWindowPos );

    /// 2D translation of the camera
    void doCameraTranslate2d(
            const glm::vec2& lastWindowPos,
            const glm::vec2& currWindowPos,
            const glm::vec2& startWindowPos );

    /// 2D rotation of the camera
    void doCameraRotate2d(
            const glm::vec2& lastWindowPos,
            const glm::vec2& currWindowPos,
            const glm::vec2& startWindowPos );

    /// 3d rotation of the camera
    void doCameraRotate3d(
            const glm::vec2& lastWindowPos,
            const glm::vec2& currWindowPos,
            const glm::vec2& startWindowPos );

    /// 2D zoom of the camera
    void doCameraZoomDrag(
            const glm::vec2& lastWindowPos,
            const glm::vec2& currWindowPos,
            const glm::vec2& startWindowPos,
            const ZoomBehavior& zoomBehavior,
            bool syncZoomForAllViews );

    void doCameraZoomScroll(
            const glm::vec2& scrollOffset,
            const glm::vec2& startWindowPos,
            const ZoomBehavior& zoomBehavior,
            bool syncZoomForAllViews );

    /// Image rotation
    void doImageRotate(
            const glm::vec2& lastWindowPos,
            const glm::vec2& currWindowPos,
            const glm::vec2& startWindowPos,
            bool inPlane );

    /// Image translation
    void doImageTranslate(
            const glm::vec2& lastWindowPos,
            const glm::vec2& currWindowPos,
            const glm::vec2& startWindowPos,
            bool inPlane );

    /// Image scale
    void doImageScale(
            const glm::vec2& lastWindowPos,
            const glm::vec2& currWindowPos,
            const glm::vec2& startWindowPos,
            bool constrainIsotropic );

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

    void scrollViewSlice(
            const glm::vec2& currWindowPos,
            int numSlices );

    void moveCrosshairsOnViewSlice(
            const glm::vec2& currWindowPos,
            int stepX, int stepY );

    /// Recenter all views on the selected images. Optionally recenter crosshairs there too.
    void recenterViews(
            const ImageSelection&,
            bool recenterCrosshairs,
            bool recenterOnCurrentCrosshairsPos );

    /// Recenter one view
    void recenterView(
            const ImageSelection&,
            const uuids::uuid& viewUid );

    bool showOverlays() const;
    void setShowOverlays( bool );

    void setMouseMode( MouseMode );

    /// Set whether manual transformation are locked on an image and all of its segmentations
    bool setLockManualImageTransformation( const uuids::uuid& imageUid, bool locked );

    /// Synchronize the lock on all segmentations of the image:
    bool syncManualImageTransformationOnSegs( const uuids::uuid& imageUid );


private:

    AppData& m_appData;
    Rendering& m_rendering;
};

#endif // CALLBACK_HANDLER_H
