#ifndef UI_HEADERS_H
#define UI_HEADERS_H

#include <uuid.h>

#include <functional>

class AppData;
struct GuiData;

class Image;
class ImageColorMap;
class ImageHeader;
class ImageSettings;
class ImageTransformations;

class ParcellationLabelTable;


void renderImageHeaderInformation(
        const ImageHeader& imgHeader,
        const ImageTransformations& imgTx,
        ImageSettings& imgSettings );

void renderImageHeader(
        AppData& appData,
        GuiData& guiData,
        const uuids::uuid& imageUid,
        size_t imageIndex,
        Image* image,
        bool isActiveImage,
        size_t numImages,
        const std::function< void(void) >& updateImageUniforms,
        const std::function< void(void) >& updateImageInterpolationMode,
        const std::function< size_t(void) >& getNumImageColorMaps,
        const std::function< const ImageColorMap* ( size_t cmapIndex ) >& getImageColorMap,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageBackward,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageForward,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageToBack,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageToFront,
        const std::function< bool ( const uuids::uuid& imageUid, bool locked ) >& setLockManualImageTransformation );

void renderSegmentationHeader(
        AppData& appData,
        const uuids::uuid& imageUid,
        size_t imageIndex,
        Image* image,
        bool isActiveImage,
        const std::function< void(void) >& updateImageUniforms,
        const std::function< ParcellationLabelTable* ( size_t tableIndex ) >& getLabelTable,
        const std::function< void ( size_t tableIndex ) >& updateLabelColorTableTexture,
        const std::function< std::optional<uuids::uuid>( const uuids::uuid& matchingImageUid, const std::string& segDisplayName ) >& createBlankSeg,
        const std::function< bool( const uuids::uuid& segUid ) >& clearSeg,
        const std::function< bool( const uuids::uuid& segUid ) >& removeSeg );

void renderLandmarkGroupHeader(
        AppData& appData,
        const uuids::uuid& imageUid,
        size_t imageIndex,
        bool isActiveImage,
        const std::function< void ( bool recenterOnCurrentCrosshairsPosition ) >& recenterCurrentViews );


#endif // UI_HEADERS_H
