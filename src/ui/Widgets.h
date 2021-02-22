#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

#include <glm/vec3.hpp>

#include <functional>
#include <utility>

class AppData;
class ImageColorMap;
class ImageTransformations;
class LandmarkGroup;
class ParcellationLabelTable;


void renderActiveImageSelectionCombo(
        const std::function< size_t (void) >& getNumImages,
        const std::function< std::pair<const char*, const char* >( size_t index ) >& getImageDisplayAndFileName,
        const std::function< size_t (void) >& getActiveImageIndex,
        const std::function< void (size_t) >& setActiveImageIndex,
        bool showText = true );

/**
 * @brief Render child window that shows the labels for a given segmentation label table
 * @param[in] tableIndex Index of the label table
 * @param[in,out] labelTable Pointer to the label table
 * @param[in] updateLabelColorTableTexture Function to update the label table texture
 */
void renderSegLabelsChildWindow(
        size_t tableIndex,
        ParcellationLabelTable* labelTable,
        const std::function< void ( size_t tableIndex ) >& updateLabelColorTableTexture );

void renderLandmarkChildWindow(
        const ImageTransformations& imageTransformations,
        LandmarkGroup* activeLmGroup,
        const glm::vec3& worldCrosshairsPos,
        const std::function< void ( const glm::vec3& worldCrosshairsPos ) >& setWorldCrosshairsPos,
        const std::function< void ( bool recenterOnCurrentCrosshairsPosition ) >& recenterCurrentViews );

void renderPaletteWindow(
        const char* name,
        bool* showPaletteWindow,
        const std::function< size_t (void) >& getNumImageColorMaps,
        const std::function< const ImageColorMap* ( size_t cmapIndex ) >& getImageColorMap,
        const std::function< size_t (void) >& getCurrentImageColormapIndex,
        const std::function< void ( size_t cmapIndex ) >& setCurrentImageColormapIndex,
        const std::function< void (void) >& updateImageUniforms );

#endif // UI_WIDGETS_H