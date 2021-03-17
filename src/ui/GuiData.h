#ifndef GUI_DATA_H
#define GUI_DATA_H

#include <imgui/imgui.h>
#include <uuid.h>

#include <string>
#include <unordered_map>


/**
 * @brief Data for the user interface
 */
struct GuiData
{
    /// Global setting to turn on/off rendering of the UI windows
    bool m_renderUiWindows = false;

    /// Global setting to turn on/off rendering of the UI overlays (crosshairs, anatomical labels)
    bool m_renderUiOverlays = false;

    // Flags to show specific UI windows
    bool m_showImagePropertiesWindow = true; //!< Show image properties window
    bool m_showSegmentationsWindow = false; //!< Show segmentations window
    bool m_showLandmarksWindow = false; //!< Show landmarks window
    bool m_showAnnotationsWindow = false; //!< Show annotations window
    bool m_showSettingsWindow = false; //!< Show settings window
    bool m_showInspectionWindow = true; //!< Show cursor inspection window
    bool m_showOpacityBlenderWindow = false; //!< Show opacity blender window
    bool m_showDemoWindow = false; //!< Show ImGui demo window

    /// Map of imageUid to boolean of whether its image color map window is shown.
    /// (The color map window is shown as a popup from the Image Properties window)
    std::unordered_map< uuids::uuid, bool > m_showImageColormapWindow;

    // Show the color map windows for the difference, cross-correlation, and histogram metric views
    bool m_showDifferenceColormapWindow = false; //!< Show difference colormap window
    bool m_showCorrelationColormapWindow = false; //!< Show correlation colormap window
    bool m_showJointHistogramColormapWindow = false; //!< Show joint histogram colormap window

    /// Precision format string used for spatial coordinates
    std::string m_coordsPrecisionFormat = "%0.3f";
    uint32_t m_coordsPrecision = 3;

    /// Precision format string used for image transformations
    std::string m_txPrecisionFormat = "%0.3f";
    uint32_t m_txPrecision = 3;

    /// Precision format string used for image values
    std::string m_imageValuePrecisionFormat = "%0.3f";
    uint32_t m_imageValuePrecision = 3;

    // Pointers to fonts allocated by ImGui
    ImFont* m_cousineFont; //!< Main ImGui font
    ImFont* m_forkAwesomeFont; //!< Icons font
};

#endif // GUI_DATA_H
