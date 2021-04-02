#ifndef UI_HEADERS_H
#define UI_HEADERS_H

#include "common/PublicTypes.h"

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


/**
 * @brief Render UI for image header information, including data for pixel and component types
 * and transformations.
 *
 * @param[in] appData App data
 * @param[in] imgHeader Image header data
 * @param[in] imgSettings Image settings data
 * @param[in] imgTx Image transformation data
 */
void renderImageHeaderInformation(
        const AppData& appData,
        const ImageHeader& imgHeader,
        const ImageSettings& imgSettings,
        const ImageTransformations& imgTx );


/**
 * @brief renderImageHeader
 * @param[in,out] appData
 * @param guiData
 * @param imageUid
 * @param imageIndex
 * @param image
 * @param isActiveImage
 * @param numImages
 * @param updateImageUniforms
 * @param updateImageInterpolationMode
 * @param getNumImageColorMaps
 * @param getImageColorMap
 * @param moveImageBackward
 * @param moveImageForward
 * @param moveImageToBack
 * @param moveImageToFront
 * @param setLockManualImageTransformation
 */
void renderImageHeader(
        AppData& appData,
        GuiData& guiData,
        const uuids::uuid& imageUid,
        size_t imageIndex,
        Image* image,
        bool isActiveImage,
        size_t numImages,
        const std::function< void ( void ) >& updateImageUniforms,
        const std::function< void ( void ) >& updateImageInterpolationMode,
        const std::function< size_t ( void ) >& getNumImageColorMaps,
        const std::function< const ImageColorMap* ( size_t cmapIndex ) >& getImageColorMap,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageBackward,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageForward,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageToBack,
        const std::function< bool ( const uuids::uuid& imageUid ) >& moveImageToFront,
        const std::function< bool ( const uuids::uuid& imageUid, bool locked ) >& setLockManualImageTransformation );


/**
 * @brief Render UI for an image segmentation header.
 * @param[in,out] appData
 * @param imageUid
 * @param imageIndex
 * @param[in,out] image
 * @param isActiveImage
 * @param updateImageUniforms
 * @param getLabelTable
 * @param updateLabelColorTableTexture
 * @param createBlankSeg
 * @param clearSeg
 * @param removeSeg
 */
void renderSegmentationHeader(
        AppData& appData,
        const uuids::uuid& imageUid,
        size_t imageIndex,
        Image* image,
        bool isActiveImage,
        const std::function< void ( void ) >& updateImageUniforms,
        const std::function< ParcellationLabelTable* ( size_t tableIndex ) >& getLabelTable,
        const std::function< void ( size_t tableIndex ) >& updateLabelColorTableTexture,
        const std::function< std::optional<uuids::uuid> ( const uuids::uuid& matchingImageUid, const std::string& segDisplayName ) >& createBlankSeg,
        const std::function< bool ( const uuids::uuid& segUid ) >& clearSeg,
        const std::function< bool ( const uuids::uuid& segUid ) >& removeSeg );


/**
 * @brief Render UI for image's landmarks
 * @param[in,out] appData
 * @param imageUid
 * @param imageIndex
 * @param isActiveImage
 * @param recenterCurrentViews
 */
void renderLandmarkGroupHeader(
        AppData& appData,
        const uuids::uuid& imageUid,
        size_t imageIndex,
        bool isActiveImage,
        const AllViewsRecenterType& recenterAllViews );


void renderAnnotationsHeader(
        AppData& appData,
        const uuids::uuid& imageUid,
        size_t imageIndex,
        bool isActiveImage,
        const AllViewsRecenterType& recenterAllViews );


#endif // UI_HEADERS_H
