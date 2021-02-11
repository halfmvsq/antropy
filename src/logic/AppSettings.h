#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include "common/CoordinateFrame.h"
#include "common/ParcellationLabelTable.h"
#include "common/Types.h"

#include "logic/interaction/events/ButtonState.h"
//#include "logic/ipc/IPCHandler.h"

#include <glm/vec3.hpp>

#include <optional>


/**
 * @brief Holds all application settings
 *
 * @note the IPC handler for communication of crosshairs coordinates with ITK-SNAP
 * is not hooked up yet. It wasn't working properly across all platforms.
 */
class AppSettings
{
public:

    AppSettings();
    ~AppSettings() = default;

    void setWorldRotationCenter( const std::optional< glm::vec3 >& worldRotationCenter );
    const std::optional< glm::vec3 >& worldRotationCenter() const;

    void setWorldCrosshairsPos( const glm::vec3& worldCrosshairs );
    const CoordinateFrame& worldCrosshairs() const;

    MouseMode mouseMode() const;
    void setMouseMode( MouseMode mode );

    ButtonState& buttonState();

    bool synchronizeZooms() const;
    void setSynchronizeZooms( bool );

    bool animating() const;
    void setAnimating( bool );

    bool overlays() const;
    void setOverlays( bool );

    size_t foregroundLabel() const;
    size_t backgroundLabel() const;

    void setForegroundLabel( size_t label, const ParcellationLabelTable& activeLabelTable );
    void setBackgroundLabel( size_t label, const ParcellationLabelTable& activeLabelTable );

    void adjustActiveSegmentationLabels( const ParcellationLabelTable& activeLabelTable );
    void swapForegroundAndBackgroundLabels( const ParcellationLabelTable& activeLabelTable );

    bool replaceBackgroundWithForeground() const;
    void setReplaceBackgroundWithForeground( bool set );

    bool use3dBrush() const;
    void setUse3dBrush( bool set );

    bool useIsotropicBrush() const;
    void setUseIsotropicBrush( bool set );

    bool useVoxelBrushSize() const;
    void setUseVoxelBrushSize( bool set );

    bool useRoundBrush() const;
    void setUseRoundBrush( bool set );

    bool crosshairsMoveWithBrush() const;
    void setCrosshairsMoveWithBrush( bool set );

    uint32_t brushSizeInVoxels() const;
    void setBrushSizeInVoxels( uint32_t size );

    float brushSizeInMm() const;
    void setBrushSizeInMm( float size );


private:

    // void broadcastCrosshairsPosition();
    // IPCHandler m_ipcHandler;

    MouseMode m_mouseMode; //!< Current mouse interaction mode
    ButtonState m_buttonState; //!< Global button state

    bool m_synchronizeZoom; //!< Synchronize zoom between views
    bool m_animating; //!< Is the app currently animating?
    bool m_overlays; //!< Render UI and vector overlays


    /* Begin segmentation drawing variables */
    size_t m_foregroundLabel; //!< Foreground segmentation label
    size_t m_backgroundLabel; //!< Background segmentation label

    bool m_replaceBackgroundWithForeground; /// Paint foreground label only over background label
    bool m_use3dBrush; //!< Paint with a 3D brush
    bool m_useIsotropicBrush; //!< Paint with an isotropic brush
    bool m_useVoxelBrushSize; //!< Measure brush size in voxel units
    bool m_useRoundBrush; //!< Brush is round (true) or rectangular (false)
    bool m_crosshairsMoveWithBrush; //!< Crosshairs move witht the brush
    uint32_t m_brushSizeInVoxels; //!< Brush size (diameter) in voxels
    float m_brushSizeInMm; //!< Brush size (diameter) in millimeters
    /* End segmentation drawing variables */


    CoordinateFrame m_worldCrosshairs; //!< Crosshairs in World space
    std::optional< glm::vec3 > m_worldRotationCenter; //!< Rotation center in World space
};

#endif // APP_SETTINGS_H
