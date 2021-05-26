#ifndef PARSE_ARGUMENTS_H
#define PARSE_ARGUMENTS_H

#include "logic/annotation/PointRecord.h"

#include <nlohmann/json.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
//#include <glm/gtc/quaternion.hpp>

#include <map>
#include <optional>
#include <string>
#include <vector>


namespace serialize
{

/// @todo Create enum for all image color maps
/// @todo Serialize rest of app data, like custom layouts


/// Serialized data for image settings
struct ImageSettings
{
    std::string m_displayName;

    double m_level;         //! Window center value in image units
    double m_window;        //! Window width in image units

    double m_thresholdLow;  //!< Values below threshold not displayed
    double m_thresholdHigh; //!< Values above threshold not displayed

    double m_opacity;       //!< Opacity [0, 1]
};


/// Serialized data for image segmentation settings
struct SegSettings
{
    double m_opacity;

    /// @todo Add rest of the segmentation options, e.g.
    /// visibility, color label table, etc.
};


/// @brief Serialized data for a segmentation image
struct Segmentation
{
    /// Segmentation image file
    std::string m_segFileName;

    /// Segmentation settings
    serialize::SegSettings m_settings;
};


/// @brief Serialized data for a group of image landmarks
struct LandmarkGroup
{
    /// CSV file holding the landmarks
    std::string m_csvFileName;

    /// Flag indicating whether landmarks are defined in image voxel space (true)
    /// or in physical/subject space (false)
    bool m_inVoxelSpace = false;
};


/// @brief Serialized data for an image in Antropy
struct Image
{
    /// Image file name
    std::string m_imageFileName;

    /// Optional 4x4 affine transformation text file name
    std::optional<std::string> m_affineTxFileName = std::nullopt;

    /// Optional deformable transformation image file name
    std::optional<std::string> m_deformationFileName = std::nullopt;

    /// Optional annotations JSON file name
    std::optional<std::string> m_annotationsFileName = std::nullopt;

    /// Segmentation image file names (each image can have multiple segmentations)
    std::vector<serialize::Segmentation> m_segmentations;

    /// Landmark groups (each image can have multiple landmark groups)
    std::vector<serialize::LandmarkGroup> m_landmarkGroups;

    /// Image settings
    serialize::ImageSettings m_settings;
};


/// @brief Serialized data for an Antropy project
struct AntropyProject
{
    serialize::Image m_referenceImage;
    std::vector<serialize::Image> m_additionalImages;
};


/// @todo Put these in a separate header file
/**
 * @brief Open a project from a file
 * @param[in/out] project
 * @param[in] fileName
 * @return True iff a valid project file was successfully opened and parsed
 */
bool open( AntropyProject& project, const std::string& fileName );

/// Save a project to a file
bool save( const AntropyProject& project, const std::string& fileName );

bool openAffineTxFile( glm::dmat4& matrix, const std::string& fileName );
bool saveAffineTxFile( const glm::dmat4& matrix, const std::string& fileName );

/**
 * @brief openLandmarkGroupCsvFile
 * @param landmarks Map of landmark ID to point record
 * @param csvFileName
 * @return
 */
bool openLandmarkGroupCsvFile( std::map< size_t, PointRecord<glm::vec3> >& landmarks, const std::string& csvFileName );

/**
 * @brief saveLandmarkGroupCsvFile
 * @param landmarks Map of landmark ID to point record
 * @param csvFileName
 * @return
 */
bool saveLandmarkGroupCsvFile( const std::map< size_t, PointRecord<glm::vec3> >& landmarks, const std::string& csvFileName );

} // namespace serialize

#endif // PARSE_ARGUMENTS_H
