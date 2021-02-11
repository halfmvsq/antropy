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

/// @todo This section is of course not done; needs expansion for serialization of all app data
/// @todo Create enum for all image color maps


/// Serialized data for image settings
struct ImageSettings
{
    std::string m_displayName;

    double m_level;         //! Window center value in image units
    double m_window;        //! Window width in image units

    double m_slope;         //!< Slope computed from window
    double m_intercept;     //!< Intercept computed from window and level

    double m_thresholdLow;  //!< Values below threshold not displayed
    double m_thresholdHigh; //!< Values above threshold not displayed

    double m_opacity;       //!< Opacity [0, 1]
};


/*
 * Also serialize the custom layouts
*/


/// @brief Serialized data for a segmentation image
struct Segmentation
{
    /// Segmentation image file
    std::string m_segFileName;
};


/// @brief Serialized data for a group of image landmarks
struct LandmarksGroup
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

    /// Optional 4x4 affine transformation file name
    std::optional<std::string> m_affineTxFileName = std::nullopt;

    /// Optional deformable transformation image file name
    std::optional<std::string> m_deformationFileName = std::nullopt;

    /// Segmentation image file names (each image can have multiple segmentations)
    std::vector<Segmentation> m_segmentations;

    /// Landmark groups (each iamge can have multiple landmark groups)
    std::vector<LandmarksGroup> m_landmarks;

    /// Image settings
    ImageSettings m_settings;
};


/// @brief Serialized data for an Antropy project
struct AntropyProject
{
    Image m_referenceImage;
    std::vector<Image> m_additionalImages;
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

bool openLandmarksFile( std::map< size_t, PointRecord<glm::vec3> >& landmarks, const std::string& fileName );
bool saveLandmarksFile( const std::map< size_t, PointRecord<glm::vec3> >& landmarks, const std::string& fileName );

} // namespace serialize

#endif // PARSE_ARGUMENTS_H
