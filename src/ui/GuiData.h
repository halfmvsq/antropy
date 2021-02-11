#ifndef GUI_DATA_H
#define GUI_DATA_H

#include <imgui/imgui.h>
#include <uuid.h>
#include <unordered_map>


/**
 * @brief Data for the user interface
 */
struct GuiData
{
    // Global setting to turn on/off rendering of the UI windows
    bool m_renderUiWindows = false;

    // Global setting to turn on/off rendering of the UI overlays (crosshairs, anatomical labels)
    bool m_renderUiOverlays = false;

    // Flags to show the particular UI windows
    bool m_showDemoWindow = false;
    bool m_showImagePropertiesWindow = true;
    bool m_showLandmarksWindow = false;
    bool m_showAnnotationsWindow = false;
    bool m_showSegmentationsWindow = false;
    bool m_showSettingsWindow = false;
    bool m_showInspectionWindow = true;

    // Map of imageUid to boolean of whether its image color map window is shown.
    // (The color map window is shown as a popup from the Image Properties window)
    std::unordered_map< uuids::uuid, bool > m_showImageColormapWindow;

    // Show the color map windows for the difference, cross-correlation, and histogram metric views
    bool m_showDifferenceColormapWindow = false;
    bool m_showCorrelationColormapWindow = false;
    bool m_showJointHistogramColormapWindow = false;

    ImFont* m_cousineFont;
    ImFont* m_forkAwesomeFont;
};

#endif // GUI_DATA_H
