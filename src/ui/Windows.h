#ifndef UI_WINDOWS_H
#define UI_WINDOWS_H

#include "common/PublicTypes.h"
#include "logic/camera/CameraHelpers.h" // Framebounds
#include "logic/camera/CameraTypes.h"
#include "ui/UiControls.h"

#include <glm/fwd.hpp>

#include <uuid.h>

#include <functional>
#include <optional>
#include <utility>

class AppData;
class ImageColorMap;
class ParcellationLabelTable;


/**
 * @brief renderViewSettingsComboWindow
 * @param viewOrLayoutUid
 * @param winMouseMinMaxCoords
 * @param uiControls
 * @param hasFrameAndBackground
 * @param showApplyToAllButton
 * @param getNumImages
 * @param isImageRendered
 * @param setImageRendered
 * @param isImageUsedForMetric
 * @param setImageUsedForMetric
 * @param getImageDisplayAndFileName
 * @param getImageVisibilitySetting
 * @param cameraType
 * @param shaderType
 * @param setCameraType
 * @param setRenderMode
 * @param recenter
 * @param applyImageSelectionAndShaderToAllViews
 */
void renderViewSettingsComboWindow(
        const uuids::uuid& viewOrLayoutUid,

        const camera::FrameBounds& mindowFrameBounds,
        const UiControls& uiControls,
        bool /*hasFrameAndBackground*/,
        bool showApplyToAllButton,

        size_t numImages,

        const std::function< bool( size_t index ) >& isImageRendered,
        const std::function< void( size_t index, bool visible ) >& setImageRendered,

        const std::function< bool( size_t index ) >& isImageUsedForMetric,
        const std::function< void( size_t index, bool visible ) >& setImageUsedForMetric,

        const std::function< std::pair<const char*, const char* >( size_t index ) >& getImageDisplayAndFileName,
        const std::function< bool( size_t imageIndex ) >& getImageVisibilitySetting,
        const std::function< bool( size_t imageIndex ) >& getImageIsActive,

        const camera::CameraType& cameraType,
        const camera::ViewRenderMode& renderMode,
        const camera::IntensityProjectionMode& intensityProjMode,

        const std::function< void ( const camera::CameraType& cameraType ) >& setCameraType,
        const std::function< void ( const camera::ViewRenderMode& renderMode ) >& setRenderMode,
        const std::function< void ( const camera::IntensityProjectionMode& projMode ) >& setIntensityProjectionMode,
        const std::function< void () >& recenter,

        const std::function< void ( const uuids::uuid& viewUid ) >& applyImageSelectionAndShaderToAllViews,

        const std::function< float () >& getIntensityProjectionSlabThickness,
        const std::function< void ( float thickness ) >& setIntensityProjectionSlabThickness,

        const std::function< bool () >& getDoMaxExtentIntensityProjection,
        const std::function< void ( bool set ) >& setDoMaxExtentIntensityProjection );


void renderViewOrientationToolWindow(
        const uuids::uuid& viewOrLayoutUid,
        const camera::FrameBounds& mindowFrameBounds,
        const UiControls& uiControls,
        bool hasFrameAndBackground,
        const camera::CameraType& cameraType,
        const std::function< glm::quat () >& getViewCameraRotation,
        const std::function< void ( const glm::quat& camera_T_world_rotationDelta ) >& setViewCameraRotation,
        const std::function< void ( const glm::vec3& worldDirection ) >& setViewCameraDirection,
        const std::function< glm::vec3 () >& getViewNormal,
        const std::function< std::vector< glm::vec3 > ( const uuids::uuid& viewUidToExclude ) >& getObliqueViewDirections );


/**
 * @brief renderImagePropertiesWindow
 * @param appData
 * @param getNumImages
 * @param getImageDisplayAndFileName
 * @param getActiveImageIndex
 * @param setActiveImageIndex
 * @param getNumImageColorMaps
 * @param getImageColorMap
 * @param moveImageBackward
 * @param moveImageForward
 * @param moveImageToBack
 * @param moveImageToFront
 * @param updateImageUniforms
 * @param updateImageInterpolationMode
 * @param setLockManualImageTransformation
 */
void renderImagePropertiesWindow(
        AppData& appData,
        size_t numImages,
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


/**
 * @brief renderSegmentationPropertiesWindow
 * @param appData
 * @param getLabelTable
 * @param updateImageUniforms
 * @param updateLabelColorTableTexture
 * @param createBlankSeg
 * @param clearSeg
 * @param removeSeg
 */
void renderSegmentationPropertiesWindow(
        AppData& appData,
        const std::function< ParcellationLabelTable* ( size_t tableIndex ) >& getLabelTable,
        const std::function< void ( const uuids::uuid& imageUid ) >& updateImageUniforms,
        const std::function< void ( size_t labelColorTableIndex ) >& updateLabelColorTableTexture,
        const std::function< void ( const uuids::uuid& imageUid, size_t labelIndex ) >& moveCrosshairsToSegLabelCentroid,
        const std::function< std::optional<uuids::uuid> ( const uuids::uuid& matchingImageUid, const std::string& segDisplayName ) >& createBlankSeg,
        const std::function< bool ( const uuids::uuid& segUid ) >& clearSeg,
        const std::function< bool( const uuids::uuid& segUid ) >& removeSeg );


/**
 * @brief renderLandmarkPropertiesWindow
 * @param appData
 * @param recenterAllViews
 */
void renderLandmarkPropertiesWindow(
        AppData& appData,
        const AllViewsRecenterType& recenterAllViews );


/**
 * @brief renderAnnotationWindow
 * @param appData
 * @param recenterAllViews
 */
void renderAnnotationWindow(
        AppData& appData,
        const std::function< void ( const uuids::uuid& viewUid, const glm::vec3& worldFwdDirection ) >& setViewCameraDirection,
        const std::function< void () >& paintActiveSegmentationWithActivePolygon,
        const AllViewsRecenterType& recenterAllViews );


/**
 * @brief renderSettingsWindow
 * @param appData
 * @param getNumImageColorMaps
 * @param getImageColorMap
 * @param updateMetricUniforms
 * @param recenterAllViews
 */
void renderSettingsWindow(
        AppData& appData,
        const std::function< size_t (void) >& getNumImageColorMaps,
        const std::function< const ImageColorMap* ( size_t cmapIndex ) >& getImageColorMap,
        const std::function< void(void) >& updateMetricUniforms,
        const AllViewsRecenterType& recenterAllViews );


/**
 * @brief renderInspectionWindow
 * @param appData
 * @param getNumImages
 * @param getImageDisplayAndFileName
 * @param getWorldDeformedPos
 * @param getSubjectPos
 * @param getVoxelPos
 * @param getImageValue
 * @param getSegLabel
 * @param getLabelTable
 */
void renderInspectionWindow(
        AppData& appData,
        size_t numImages,
        const std::function< std::pair<const char*, const char* >( size_t index ) >& getImageDisplayAndFileName,
        const std::function< glm::vec3 () >& getWorldDeformedPos,
        const std::function< std::optional<glm::vec3> ( size_t imageIndex ) >& getSubjectPos,
        const std::function< std::optional<glm::ivec3> ( size_t imageIndex ) >& getVoxelPos,
        const std::function< std::optional<double> ( size_t imageIndex ) >& getImageValue,
        const std::function< std::optional<int64_t> ( size_t imageIndex ) >& getSegLabel,
        const std::function< ParcellationLabelTable* ( size_t tableIndex ) >& getLabelTable );


/**
 * @brief renderInspectionWindowWithTable
 * @param appData
 * @param getImageDisplayAndFileName
 * @param getSubjectPos
 * @param getVoxelPos
 * @param getImageValue
 * @param getSegLabel
 * @param getLabelTable
 */
void renderInspectionWindowWithTable(
        AppData& appData,
        const std::function< std::pair<const char*, const char* >( size_t index ) >& getImageDisplayAndFileName,
        const std::function< std::optional<glm::vec3> ( size_t imageIndex ) >& getSubjectPos,
        const std::function< std::optional<glm::ivec3> ( size_t imageIndex ) >& getVoxelPos,
        const std::function< void ( size_t imageIndex, const glm::vec3& subjectPos ) > setSubjectPos,
        const std::function< void ( size_t imageIndex, const glm::ivec3& voxelPos ) > setVoxelPos,
        const std::function< std::optional<double> ( size_t imageIndex ) >& getImageValue,
        const std::function< std::optional<int64_t> ( size_t imageIndex ) >& getSegLabel,
        const std::function< ParcellationLabelTable* ( size_t tableIndex ) >& getLabelTable );


/**
 * @brief renderOpacityBlenderWindow
 * @param appData
 * @param updateImageUniforms
 */
void renderOpacityBlenderWindow(
        AppData& appData,
        const std::function< void ( const uuids::uuid& imageUid ) >& updateImageUniforms );

#endif // UI_WINDOWS_H
