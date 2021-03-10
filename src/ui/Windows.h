#ifndef UI_WINDOWS_H
#define UI_WINDOWS_H

#include "logic/camera/CameraTypes.h"
#include "ui/UiControls.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <uuid.h>

#include <functional>
#include <optional>
#include <utility>

class AppData;
class ImageColorMap;
class ParcellationLabelTable;


void renderViewSettingsComboWindow(
        const uuids::uuid& viewOrLayoutUid,

        const std::pair< glm::vec2, glm::vec2 >& winMouseMinMaxCoords,
        const UiControls& uiControls,
        bool hasFrameAndBackground,
        bool showApplyToAllButton,

        const std::function< size_t(void) >& getNumImages,

        const std::function< bool( size_t index ) >& isImageRendered,
        const std::function< void( size_t index, bool visible ) >& setImageRendered,

        const std::function< bool( size_t index ) >& isImageUsedForMetric,
        const std::function< void( size_t index, bool visible ) >& setImageUsedForMetric,

        const std::function< std::pair<const char*, const char* >( size_t index ) >& getImageDisplayAndFileName,
        const std::function< bool( size_t imageIndex ) >& getImageVisibilitySetting,

        const camera::CameraType& cameraType,
        const camera::ViewRenderMode& shaderType,

        const std::function< void ( const camera::CameraType& cameraType ) >& setCameraType,
        const std::function< void ( const camera::ViewRenderMode& shaderType ) >& setRenderMode,
        const std::function< void () >& recenter,

        const std::function< void ( const uuids::uuid& viewUid ) >& applyImageSelectionAndShaderToAllViews );


void renderImagePropertiesWindow(
        AppData& appData,
        const std::function< size_t (void) >& getNumImages,
        const std::function< std::pair<const char*, const char* >( size_t index ) >& getImageDisplayAndFileName,
        const std::function< size_t (void) >& getActiveImageIndex,
        const std::function< void (size_t) >& setActiveImageIndex,
        const std::function< size_t (void) >& getNumImageColorMaps,
        const std::function< const ImageColorMap* ( size_t cmapIndex ) >& getImageColorMap,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageBackward,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageForward,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageToBack,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageToFront,
        const std::function< void ( const uuids::uuid& imageUid ) >& updateImageUniforms,
        const std::function< void ( const uuids::uuid& imageUid ) >& updateImageInterpolationMode,
        const std::function< bool ( const uuids::uuid& imageUid, bool locked ) >& setLockManualImageTransformation );


void renderSegmentationPropertiesWindow(
        AppData& appData,
        const std::function< ParcellationLabelTable* ( size_t tableIndex ) >& getLabelTable,
        const std::function< void ( const uuids::uuid& imageUid ) >& updateImageUniforms,
        const std::function< void ( size_t labelColorTableIndex ) >& updateLabelColorTableTexture,
        const std::function< std::optional<uuids::uuid> ( const uuids::uuid& matchingImageUid, const std::string& segDisplayName ) >& createBlankSeg,
        const std::function< bool ( const uuids::uuid& segUid ) >& clearSeg,
        const std::function< bool( const uuids::uuid& segUid ) >& removeSeg );


void renderLandmarkPropertiesWindow(
        AppData& appData,
        const std::function< void ( const uuids::uuid& viewUid ) >& /*recenterView*/,
        const std::function< void ( bool recenterOnCurrentCrosshairsPosition ) >& recenterCurrentViews );


void renderSettingsWindow(
        AppData& appData,
        const std::function< size_t (void) >& getNumImageColorMaps,
        const std::function< const ImageColorMap* ( size_t cmapIndex ) >& getImageColorMap,
        const std::function< void(void) >& updateMetricUniforms,
        const std::function< void(void) >& recenterViews );


void renderInspectionWindow(
        AppData& appData,
        const std::function< size_t (void) >& getNumImages,
        const std::function< std::pair<const char*, const char* >( size_t index ) >& getImageDisplayAndFileName,
        const std::function< glm::vec3 () >& getWorldDeformedPos,
        const std::function< std::optional<glm::vec3> ( size_t imageIndex ) >& getSubjectPos,
        const std::function< std::optional<glm::ivec3> ( size_t imageIndex ) >& getVoxelPos,
        const std::function< std::optional<double> ( size_t imageIndex ) >& getImageValue,
        const std::function< std::optional<int64_t> ( size_t imageIndex ) >& getSegLabel,
        const std::function< ParcellationLabelTable* ( size_t tableIndex ) >& getLabelTable );

void renderOpacityBlenderWindow(
        AppData& appData,
        const std::function< void ( const uuids::uuid& imageUid ) >& updateImageUniforms );

#endif // UI_WINDOWS_H
