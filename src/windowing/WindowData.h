#ifndef WINDOW_DATA_H
#define WINDOW_DATA_H

#include "common/UuidRange.h"
#include "common/Viewport.h"
#include "ui/UiControls.h"
#include "windowing/Layout.h"
#include "windowing/View.h"

#include <uuid.h>

#include <optional>
#include <unordered_map>
#include <vector>


/**
 * @brief Data for the window
 */
class WindowData
{   
public:

    WindowData();
    ~WindowData() = default;

    void setDefaultRenderedImagesForAllLayouts( uuid_range_t orderedImageUids );
    void setDefaultRenderedImagesForLayout( Layout& layout, uuid_range_t orderedImageUids );

    // Call this when image order changes in order to update rendered and metric images:
    void updateImageOrdering( uuid_range_t orderedImageUids );

    /// Initialize the view to the given center and FOV, defined in World space
    void recenterViews( const glm::vec3& worldCenter, const glm::vec3& worldFov, float scale, bool resetZoom );

    /// Recenter a view to the given center position, without changing its FOV.
    /// (FOV is passed in only to adjust camera pullback distance.)
    void recenterView( const uuids::uuid& viewUid, const glm::vec3& worldCenter, const glm::vec3& worldFov );

    /// Get all current view UIDs
    uuid_range_t currentViewUids() const;

    /// In which view is the window position?
    std::optional<uuids::uuid> currentViewUidAtCursor( const glm::vec2& windowPos ) const;

    /// Get const pointer to a current view
    const View* currentView( const uuids::uuid& ) const;

    /// Get non-const pointer to a current view
    View* currentView( const uuids::uuid& );

    /// Get UID of active view
    std::optional<uuids::uuid> activeViewUid() const;

    /// Set UID of the active view
    void setActiveViewUid( const std::optional<uuids::uuid>& );

    /// Number of layouts
    size_t numLayouts() const; 

    /// Current layout index
    size_t currentLayoutIndex() const;

    void setCurrentLayoutIndex( size_t index );
    void cycleCurrentLayout( int step );

    const Layout* layout( size_t index ) const;

    const Layout& currentLayout() const;
    Layout& currentLayout();

    /// Add a grid layout
    void addGridLayout( int width, int height, bool offsetViews, bool isLightbox );

    /// Add a lightbox grid layout with enough views to hold a given number of slices
    void addLightboxLayoutForImage( size_t numSlices );

    /// Add a layout with one row per image and columns for axial, coronal, and sagittal views
    void addAxCorSagLayout( size_t numImages );

    /// Remove a layout
    void removeLayout( size_t index );

    /// Get the viewport
    const Viewport& viewport() const;

    /// Resize the viewport
    void resizeViewport( int width, int height );

    /// Set ratio of framebuffer to window unit coordinates
    void setDeviceScaleRatio( const glm::vec2& ratio );

    /// Get view UIDs in a camera translation synchronization group
    uuid_range_t cameraTranslationGroupViewUids( const uuids::uuid& syncGroupUid ) const;

    /// Get view UIDs in a camera zoom synchronization group
    uuid_range_t cameraZoomGroupViewUids( const uuids::uuid& syncGroupUid ) const;

    /// Apply a given view's image selection to all views of the current layout
    void applyImageSelectionToAllCurrentViews( const uuids::uuid& referenceViewUid );

    /// Apply a given view's shader type to all views of the current layout
    void applyViewShaderToAllCurrentViews( const uuids::uuid& referenceViewUid );


private:

    void setupViews();
    void updateViews();
    void recomputeViewAspectRatios();
    void recomputeViewCorners();

    Viewport m_viewport; // Window viewport (encompassing all views)
    std::vector<Layout> m_layouts; // All view layouts
    size_t m_currentLayout; // Index of the layout currently on display

    // UID of the view in which the user is currently interacting with the mouse.
    // The mouse must be held down for the view to be active.
    std::optional<uuids::uuid> m_activeViewUid;
};

#endif // WINDOW_DATA_H