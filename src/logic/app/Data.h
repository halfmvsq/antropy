#ifndef APP_DATA_H
#define APP_DATA_H

#include "common/ParcellationLabelTable.h"
#include "common/Types.h"
#include "common/UuidRange.h"

#include "image/Image.h"
#include "image/ImageColorMap.h"

#include "logic/app/Settings.h"
#include "logic/app/State.h"
#include "logic/annotation/Annotation.h"
#include "logic/annotation/LandmarkGroup.h"
#include "logic/serialization/ProjectSerialization.h"

#include "rendering/RenderData.h"
#include "windowing/WindowData.h"

#include "ui/GuiData.h"

#include <glm/vec2.hpp>

#include <uuid.h>

#include <list>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


/**
 * @brief Holds all application data
 * @todo A simple database would be better suited for this purpose
 */
class AppData
{
public:

    AppData();
    ~AppData();

    const AppSettings& settings() const;
    AppSettings& settings();

    const AppState& state() const;
    AppState& state();

    const GuiData& guiData() const;
    GuiData& guiData();

    const RenderData& renderData() const;
    RenderData& renderData();

    const WindowData& windowData() const;
    WindowData& windowData();


    /// @todo Put into AppState
    void setProject( serialize::AntropyProject project );
    const serialize::AntropyProject& project() const;
    serialize::AntropyProject& project();


    /**
     * @brief Add an image
     * @param[in] image The image.
     * @return The image's newly generated unique identifier
     */
    uuids::uuid addImage( Image image );

    /**
     * @brief Add a segmentation.
     * @param[in] seg Segmentation image. The image must have unsigned integer pixel component type.
     * @return If added, the segmentation image's newly generated unique identifier; else nullopt.
     */
    std::optional<uuids::uuid> addSeg( Image seg );

    /**
     * @brief Add an image deformation field
     * @param[in] def Deformation field image. The image must have at least three components (x, y, z) per pixel.
     * @return If added, deformation field image's newly generated unique identifier; else nullpt.
     */
    std::optional<uuids::uuid> addDef( Image def );

    /**
     * @brief Add a segmentation label color table
     * @param[in] numLabels Number of labels in the table
     * @param[in] maxNumLabels Maximum number of labels allowed in the table
     * @return Index of the new table
     */
    size_t addLabelColorTable( size_t numLabels, size_t maxNumLabels );

    /**
     * @brief Add a landmark group
     * @param[in] lmGroup Landmark group.
     * @return The landmark group's newly generated unique identifier
     */
    uuids::uuid addLandmarkGroup( LandmarkGroup lmGroup );

    /**
     * @brief Add an annotation and associate it with an image
     * @param[in] imageUid Image UID
     * @param[in] annotation New annotation
     * @return If the image exists, return the annotation's newly generated unique identifier;
     * otherwise return nullopt.
     */
    std::optional<uuids::uuid> addAnnotation( const uuids::uuid& imageUid, Annotation annotation );

//    bool removeImage( const uuids::uuid& imageUid );
    bool removeSeg( const uuids::uuid& segUid );
    bool removeDef( const uuids::uuid& defUid );
    bool removeAnnotation( const uuids::uuid& defUid );

    const Image* image( const uuids::uuid& imageUid ) const;
    Image* image( const uuids::uuid& imageUid );

    const Image* seg( const uuids::uuid& segUid ) const;
    Image* seg( const uuids::uuid& segUid );

    const Image* def( const uuids::uuid& defUid ) const;
    Image* def( const uuids::uuid& defUid );

    const ImageColorMap* imageColorMap( const uuids::uuid& mapUid ) const;

    const ParcellationLabelTable* labelTable( const uuids::uuid& tableUid ) const;
    ParcellationLabelTable* labelTable( const uuids::uuid& tableUid );

    const LandmarkGroup* landmarkGroup( const uuids::uuid& lmGroupUid ) const;
    LandmarkGroup* landmarkGroup( const uuids::uuid& lmGroupUid );

    /**
     * @brief Get an annotation by UID
     * @param annotUid Annotation UID
     * @return Raw pointer to the annotation if it exists;
     * nullptr if the annotation does not exist
     */
    const Annotation* annotation( const uuids::uuid& annotUid ) const;
    Annotation* annotation( const uuids::uuid& annotUid );

    /// Set/get the reference image UID
    bool setRefImageUid( const uuids::uuid& refImageUid );
    std::optional<uuids::uuid> refImageUid() const;

    /// Set/get the UID of the active image: the one that is currently being transformed or
    /// whose settings are currently being changed by the user
    bool setActiveImageUid( const uuids::uuid& activeImageUid );
    std::optional<uuids::uuid> activeImageUid() const;

    /// Set rainbow colors for the image border and edges
    void setRainbowColorsForAllImages();

    /// Set rainbow colors for all of the landmark groups.
    /// Copy the image edge color
    void setRainbowColorsForAllLandmarkGroups();

    /// Move the image backward/forward in layers (decrease/increase its layer order)
    /// @note It is important that these take arguments by value, since imageUids are being swapped
    /// internally. Potential for nasty bugs if a reference is used.
    bool moveImageBackwards( const uuids::uuid imageUid );
    bool moveImageForwards( const uuids::uuid imageUid );

    /// Move the image to the backmost/frontmost layer
    bool moveImageToBack( const uuids::uuid imageUid );
    bool moveImageToFront( const uuids::uuid imageUid );


    /// Move the image annotation backward/forward in layers (decrease/increase its layer order)
    bool moveAnnotationBackwards( const uuids::uuid imageUid, const uuids::uuid annotUid );
    bool moveAnnotationForwards( const uuids::uuid imageUid, const uuids::uuid annotUid );

    /// Move the image annotation to the backmost/frontmost layer
    bool moveAnnotationToBack( const uuids::uuid imageUid, const uuids::uuid annotUid );
    bool moveAnnotationToFront( const uuids::uuid imageUid, const uuids::uuid annotUid );


    size_t numImages() const;
    size_t numSegs() const;
    size_t numDefs() const;
    size_t numImageColorMaps() const;
    size_t numLabelTables() const;
    size_t numLandmarkGroups() const;
    size_t numAnnotations() const;


    uuid_range_t imageUidsOrdered() const;
    uuid_range_t segUidsOrdered() const;
    uuid_range_t defUidsOrdered() const;
    uuid_range_t imageColorMapUidsOrdered() const;
    uuid_range_t labelTableUidsOrdered() const;
    uuid_range_t landmarkGroupUidsOrdered() const;


    /// Set/get the active segmentation for an image
    bool assignActiveSegUidToImage( const uuids::uuid& imageUid, const uuids::uuid& activeSegUid );
    std::optional<uuids::uuid> imageToActiveSegUid( const uuids::uuid& imageUid ) const;

    /// Set/get the active deformation field for an image
    bool assignActiveDefUidToImage( const uuids::uuid& imageUid, const uuids::uuid& activeDefUid );
    std::optional<uuids::uuid> imageToActiveDefUid( const uuids::uuid& imageUid ) const;

    /// Assign a segmentation to an image.
    /// Makes it the active segmentation if it is the first one.
    bool assignSegUidToImage( const uuids::uuid& imageUid, const uuids::uuid& segUid );

    /// Assign a deformation field to an image.
    /// Makes it the active deformation field if it is the first one.
    bool assignDefUidToImage( const uuids::uuid& imageUid, const uuids::uuid& defUid );

    /// Get all segmentations for an image
    std::vector<uuids::uuid> imageToSegUids( const uuids::uuid& imageUid ) const;

    /// Get all deformation fields for an image
    std::vector<uuids::uuid> imageToDefUids( const uuids::uuid& imageUid ) const;

    /// Assign a group of landmarks to an image
    bool assignLandmarkGroupUidToImage( const uuids::uuid& imageUid, uuids::uuid lmGroupUid );
    const std::vector<uuids::uuid>& imageToLandmarkGroupUids( const uuids::uuid& imageUid ) const;

    /// Set/get the active landmark group for an image
    bool assignActiveLandmarkGroupUidToImage( const uuids::uuid& imageUid, const uuids::uuid& lmGroupUid );
    std::optional<uuids::uuid> imageToActiveLandmarkGroupUid( const uuids::uuid& imageUid ) const;

    /// Set/get the active annotation for an image
    bool assignActiveAnnotationUidToImage( const uuids::uuid& imageUid, const std::optional<uuids::uuid>& annotUid );
    std::optional<uuids::uuid> imageToActiveAnnotationUid( const uuids::uuid& imageUid ) const;


    /**
     * @brief Get a list of all annotations assigned to a given image. The annotation order
     * corresponds to the order in which the annotations were added to the image.
     * @param imageUid Image UID
     * @return List of (ordered) annotation UIDs for the image.
     * The list is empty if the image has no annotations or the image UID is invalid.
     */
    const std::list<uuids::uuid>& annotationsForImage( const uuids::uuid& imageUid ) const;

    /// Set/get whether the image is
    void setImageBeingSegmented( const uuids::uuid& imageUid, bool set );
    bool isImageBeingSegmented( const uuids::uuid& imageUid ) const;

    uuid_range_t imagesBeingSegmented() const;


    std::optional<uuids::uuid> imageUid( size_t index ) const;
    std::optional<uuids::uuid> segUid( size_t index ) const;
    std::optional<uuids::uuid> defUid( size_t index ) const;
    std::optional<uuids::uuid> imageColorMapUid( size_t index ) const;
    std::optional<uuids::uuid> labelTableUid( size_t index ) const;
    std::optional<uuids::uuid> landmarkGroupUid( size_t index ) const;

    std::optional<size_t> imageIndex( const uuids::uuid& imageUid ) const;
    std::optional<size_t> segIndex( const uuids::uuid& segUid ) const;
    std::optional<size_t> defIndex( const uuids::uuid& defUid ) const;
    std::optional<size_t> imageColorMapIndex( const uuids::uuid& mapUid ) const;
    std::optional<size_t> labelTableIndex( const uuids::uuid& tableUid ) const;
    std::optional<size_t> landmarkGroupIndex( const uuids::uuid& lmGroupUid ) const;
    std::optional<size_t> annotationIndex( const uuids::uuid& imageUid, const uuids::uuid& annotUid ) const;

    /// @todo Put into DataHelper
    Image* refImage();
    const Image* refImage() const;

    Image* activeImage();
    const Image* activeImage() const;

    Image* activeSeg();

    ParcellationLabelTable* activeLabelTable();

    std::string getAllImageDisplayNames() const;


private:

    void loadImageColorMaps();

    /// @todo Put into AntropyApp
    AppSettings m_settings; //!< Application settings
    AppState m_state; //!< Application state

    GuiData m_guiData; //!< Data for the UI
    RenderData m_renderData; //!< Data for rendering
    WindowData m_windowData; //!< Data for windowing

    serialize::AntropyProject m_project; //!< Project that is used for serialization

    std::unordered_map<uuids::uuid, Image> m_images; //!< Images
    std::vector<uuids::uuid> m_imageUidsOrdered; //!< Image UIDs in order

    std::unordered_map<uuids::uuid, Image> m_segs; //!< Segmentations, also stored as images
    std::vector<uuids::uuid> m_segUidsOrdered; //!< Segmentation UIDs in order

    std::unordered_map<uuids::uuid, Image> m_defs; //!< Deformation fields, also stored as images
    std::vector<uuids::uuid> m_defUidsOrdered; //!< Deformation field UIDs in order

    std::unordered_map<uuids::uuid, ImageColorMap> m_imageColorMaps; //!< Image color maps
    std::vector<uuids::uuid> m_imageColorMapUidsOrdered; //!< Image color map UIDs in order

    std::unordered_map<uuids::uuid, ParcellationLabelTable> m_labelTables; //!< Segmentation label tables
    std::vector<uuids::uuid> m_labelTablesUidsOrdered; //!< Segmentation label table UIDs in order

    std::unordered_map<uuids::uuid, LandmarkGroup> m_landmarkGroups; //!< Landmark groups
    std::vector<uuids::uuid> m_landmarkGroupUidsOrdered; //!< Landmark group UIDs in order

    std::unordered_map<uuids::uuid, Annotation> m_annotations; //!< Annotations

    /// ID of the reference image. This is null iff there are no images.
    std::optional<uuids::uuid> m_refImageUid;

    /// ID of the image being actively transformed or whose settings are being changed.
    /// This is null iff there are no images.
    std::optional<uuids::uuid> m_activeImageUid;

    std::unordered_map< uuids::uuid, std::vector<uuids::uuid> > m_imageToSegs; //!< Map of image to its segmentations
    std::unordered_map< uuids::uuid, uuids::uuid > m_imageToActiveSeg; //!< Map of image to its active segmentation

    std::unordered_map< uuids::uuid, std::vector<uuids::uuid> > m_imageToDefs; //!< Map of image to its deformation fields
    std::unordered_map< uuids::uuid, uuids::uuid > m_imageToActiveDef; //!< Map of image to its active deformation field

    /// Map of image to its landmark groups
    std::unordered_map< uuids::uuid, std::vector<uuids::uuid> > m_imageToLandmarkGroups;

    /// Map of image to its active landmark group
    std::unordered_map< uuids::uuid, uuids::uuid > m_imageToActiveLandmarkGroup;

    /// Map of image to its annotations.
    /// The order of the annotations matches the order in the list.
    std::unordered_map< uuids::uuid, std::list<uuids::uuid> > m_imageToAnnotations;

    /// Map of image to its active/selected annotation
    std::unordered_map< uuids::uuid, uuids::uuid > m_imageToActiveAnnotation;

    /// Is an image being segmented (in addition to the active image)?
    /// @todo Move to AppState
    std::unordered_set< uuids::uuid > m_imagesBeingSegmented;
};

#endif // APP_DATA_H
