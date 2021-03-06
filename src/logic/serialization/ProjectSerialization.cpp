#include "logic/serialization/ProjectSerialization.h"
#include "logic/serialization/JsonSerializers.h"
#include "logic/annotation/SerializeAnnot.h"

#include "common/Exception.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <exception>
#include <fstream>
#include <sstream>
#include <vector>

// On Apple platforms, we must use the alternative ghc::filesystem,
// because it is not fully implemented or supported prior to macOS 10.15.
#if !defined(__APPLE__)
#if defined(__cplusplus) && __cplusplus >= 201703L && defined(__has_include)
#if __has_include(<filesystem>)
#define GHC_USE_STD_FS
#include <filesystem>
namespace fs = std::filesystem;
#endif
#endif
#endif

#ifndef GHC_USE_STD_FS
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif


#if defined(_LIBCPP_VERSION) && (_LIBCPP_VERSION >= 1000)
#define HAS_IOS_BASE_FAILURE_DERIVED_FROM_SYSTEM_ERROR 1
#else
#define HAS_IOS_BASE_FAILURE_DERIVED_FROM_SYSTEM_ERROR 0
#endif


using json = nlohmann::json;


namespace
{

/// @brief Convert all project image file names to be relative to the project base path and canonical
//void makePathsRelative( json& project, const fs::path& basePath )
//{
//    auto makeRelative = [&basePath] ( json& image )
//    {
//        fs::path refImagePath = image.at( "imageFileName" ).get<std::string>();
//        if ( refImagePath.is_relative() )
//        {
//            image.at( "imageFileName" ) = fs::relative( refImagePath, basePath ).string();
//        }

//        if ( image.count( "segFileName" ) > 0 )
//        {
//            fs::path refSegPath = image.at( "segFileName" ).get<std::string>();
//            if ( refSegPath.is_relative() )
//            {
//                image.at( "segFileName" ) = fs::relative( refSegPath, basePath ).string();
//            }
//        }
//    };

//    makeRelative( project.at( "referenceImage" ) );

//    for ( auto& image : project.at( "additionalImages" ) )
//    {
//        makeRelative( image );
//    }
//}

void applyToImagePaths(
        serialize::AntropyProject& project,
        const fs::path& projectBasePath,
        std::function< void ( serialize::Image& image, const fs::path& projectBasePath ) > func )
{
    func( project.m_referenceImage, projectBasePath );

    for ( auto& image : project.m_additionalImages )
    {
        func( image, projectBasePath );
    }
}

} // anonymous


namespace imageio
{

/// Define serialization of InterpolationMode to JSON as strings
//NLOHMANN_JSON_SERIALIZE_ENUM(
//        ImageSettings::InterpolationMode, {
//            { ImageSettings::InterpolationMode::Linear, "Linear" },
//            { ImageSettings::InterpolationMode::NearestNeighbor, "NearestNeighbor" }
//        } );

} // namespace imageio


namespace serialize
{

/*
void to_json( json& j, const ImageSettings& settings )
{
    j = json {
    { "level", settings.m_level },
    { "window", settings.m_window },
    { "slope", settings.m_slope },
    { "intercept", settings.m_intercept },
    { "thresholdLow", settings.m_thresholdLow },
    { "thresholdHigh", settings.m_thresholdHigh },
    { "opacity", settings.m_opacity }
            //        { "interpolationMode", settings.m_interpolationMode },
            //        { "colorMapName", settings.m_colorMapName }
    };
}

void from_json( const json& j, ImageSettings& settings )
{
    j.at( "level" ).get_to( settings.m_level );
    j.at( "window" ).get_to( settings.m_window );
    j.at( "slope" ).get_to( settings.m_slope );
    j.at( "intercept" ).get_to( settings.m_intercept );
    j.at( "thresholdLow" ).get_to( settings.m_thresholdLow );
    j.at( "thresholdHigh" ).get_to( settings.m_thresholdHigh );
    j.at( "opacity" ).get_to( settings.m_opacity );
//    j.at( "interpolationMode" ).get_to( settings.m_interpolationMode );
//    j.at( "colorMapName" ).get_to( settings.m_colorMapName );
}
*/


void to_json( json& j, const serialize::Segmentation& seg )
{
    j = json
    {
        { "path", seg.m_segFileName }
    };
}

void from_json( const json& j, serialize::Segmentation& seg )
{
    j.at( "path" ).get_to( seg.m_segFileName );
}


void to_json( json& j, const serialize::LandmarkGroup& landmarks )
{
    j = json
    {
        { "path", landmarks.m_csvFileName },
        { "inVoxelSpace",landmarks.m_inVoxelSpace }
    };
}

void from_json( const json& j, serialize::LandmarkGroup& landmarks )
{
    j.at( "path" ).get_to( landmarks.m_csvFileName );

    if ( j.count( "inVoxelSpace" ) )
    {
        landmarks.m_inVoxelSpace = j.at( "inVoxelSpace" ).get<bool>();
    }
    else
    {
        // If not defined, then assume false
        landmarks.m_inVoxelSpace = false;
    }
}


void to_json( json& j, const serialize::Image& image )
{
    j = json
    {
        { "image", image.m_imageFileName }
    };

    if ( image.m_affineTxFileName )
    {
        j[ "affine" ] = *image.m_affineTxFileName;
    }

    if ( image.m_deformationFileName )
    {
        j[ "deformation" ] = *image.m_deformationFileName;
    }

    if ( image.m_annotationsFileName )
    {
        j[ "annotations" ] = *image.m_annotationsFileName;
    }

    if ( ! image.m_segmentations.empty() )
    {
        j[ "segmentations" ] = image.m_segmentations;
    }

    if ( ! image.m_landmarkGroups.empty() )
    {
        j[ "landmarks" ] = image.m_landmarkGroups;
    }
}

void from_json( const json& j, serialize::Image& image )
{
    j.at( "image" ).get_to( image.m_imageFileName );

    if ( j.count( "affine" ) )
    {
        image.m_affineTxFileName = j.at( "affine" ).get<std::string>();
    }

    if ( j.count( "deformation" ) )
    {
        image.m_deformationFileName = j.at( "deformation" ).get<std::string>();
    }

    if ( j.count( "annotations" ) )
    {
        image.m_annotationsFileName = j.at( "annotations" ).get<std::string>();
    }

    if ( j.count( "segmentations" ) )
    {
        j.at( "segmentations" ).get_to( image.m_segmentations );
    }

    if ( j.count( "landmarks" ) )
    {
        j.at( "landmarks" ).get_to( image.m_landmarkGroups );
    }
}


void to_json( json& j, const AntropyProject& project )
{
    j = json
    {
        { "reference", project.m_referenceImage }
    };

    if ( ! project.m_additionalImages.empty() )
    {
        j[ "additional" ] = project.m_additionalImages;
    }
}

void from_json( const json& j, AntropyProject& project )
{
    j.at( "reference" ).get_to( project.m_referenceImage );

    if ( j.count( "additional" ) )
    {
        j.at( "additional" ).get_to( project.m_additionalImages );
    }
}


bool open( AntropyProject& project, const std::string& fileName )
{
    // Make all paths in the image absolute:
    auto makeCanonicalAbsolute = [] ( serialize::Image& image, const fs::path& projectBasePath )
    {
        const fs::path saveCurrentPath = fs::current_path(); // save current path
        fs::current_path( projectBasePath ); // set current path to project path

        image.m_imageFileName = fs::canonical( image.m_imageFileName ).string();

        if ( image.m_affineTxFileName )
        {
            image.m_affineTxFileName = fs::canonical( *image.m_affineTxFileName ).string();
        }

        if ( image.m_deformationFileName )
        {
            image.m_deformationFileName = fs::canonical( *image.m_deformationFileName ).string();
        }

        if ( image.m_annotationsFileName )
        {
            image.m_annotationsFileName = fs::canonical( *image.m_annotationsFileName ).string();
        }

        for ( Segmentation& seg : image.m_segmentations )
        {
            seg.m_segFileName = fs::canonical( seg.m_segFileName ).string();
        }

        for ( LandmarkGroup& lm : image.m_landmarkGroups )
        {
            lm.m_csvFileName = fs::canonical( lm.m_csvFileName ).string();
        }

        fs::current_path( saveCurrentPath ); // restore current path
    };

    std::ifstream inFile;
    inFile.exceptions( inFile.exceptions() | std::ios::failbit | std::ifstream::badbit );

    try
    {
        inFile.open( fileName, std::ios_base::in );

        if ( ! inFile )
        {
            throw std::system_error( errno, std::system_category(),
                                     "Failed to open project file " + fileName );
        }

        json j;
        inFile >> j;

        // Image paths in the project file can be specified relative to the project location,
        // so we need to make all image paths absolute.
        project = j.get<AntropyProject>();
        spdlog::debug( "Parsed project JSON:\n{}", j.dump( 2 ) );

        fs::path projectBasePath( fileName );
        projectBasePath.remove_filename();

        if ( projectBasePath.empty() )
        {
            projectBasePath = fs::current_path();
            spdlog::warn( "Project base path is empty; using current path ({})",
                          projectBasePath );
        }

        projectBasePath = fs::canonical( projectBasePath );
        spdlog::debug( "Base path for the project file is {}", projectBasePath );

        applyToImagePaths( project, projectBasePath, makeCanonicalAbsolute );

        const json jAbs = project;
        spdlog::debug( "Parsed project JSON (with absolute paths):\n{}", jAbs.dump( 2 ) );
        spdlog::info( "Loaded project from file {}", fileName );

        return true;
    }
    catch ( const std::ios_base::failure& e )
    {
#if HAS_IOS_BASE_FAILURE_DERIVED_FROM_SYSTEM_ERROR
        // e.code() is only available if the lib actually follows ISO ??27.5.3.1.1
        // and derives ios_base::failure from system_error
        spdlog::error( "Error #{}: {}", e.code().value(), e.code().message() );

        if ( std::make_error_condition( std::io_errc::stream ) == e.code() )
        {
            spdlog::error( "Stream error opening file" );
        }
        else
        {
            spdlog::error( "Unknown failure opening file" );
        }
#else
        spdlog::error( "Error #{}: {}", errno, ::strerror(errno) );
#endif

        spdlog::error( "Failure while project from JSON file {}: {}",
                       fileName, e.what() );
        return false;
    }
    catch ( const std::exception& e )
    {
        spdlog::error( "Error opening project from JSON file {}: {}",
                       fileName, e.what() );
        return false;
    }
}


bool save( const AntropyProject& project, const std::string& fileName )
{
    // Make all paths in the image relative to the base path:
    auto makeRelative = [] ( serialize::Image& image, const fs::path& projectBasePath )
    {
        image.m_imageFileName = fs::relative(
                    image.m_imageFileName, projectBasePath ).string();

        if ( image.m_affineTxFileName )
        {
            image.m_affineTxFileName = fs::relative(
                        *image.m_affineTxFileName, projectBasePath ).string();
        }

        if ( image.m_deformationFileName )
        {
            image.m_deformationFileName = fs::relative(
                        *image.m_deformationFileName, projectBasePath ).string();
        }

        if ( image.m_annotationsFileName )
        {
            image.m_annotationsFileName = fs::relative(
                        *image.m_annotationsFileName, projectBasePath ).string();
        }

        for ( serialize::Segmentation& seg : image.m_segmentations )
        {
            seg.m_segFileName = fs::relative(
                        seg.m_segFileName, projectBasePath ).string();
        }

        for ( serialize::LandmarkGroup& lm : image.m_landmarkGroups )
        {
            lm.m_csvFileName = fs::relative(
                        lm.m_csvFileName, projectBasePath ).string();
        }
    };

    try
    {
        // Create version of project with image paths relative to project filename base path:
        AntropyProject projectRelative = project;

        fs::path projectBasePath( fileName );
        projectBasePath.remove_filename();

        if ( projectBasePath.empty() )
        {
            spdlog::warn( "Project base path is empty; using current path" );
            projectBasePath = fs::current_path();
        }

        projectBasePath = fs::canonical( projectBasePath );
        spdlog::debug( "Base path for the project file is {}", projectBasePath );

        // Make all image paths relative to the project file:
        applyToImagePaths( projectRelative, projectBasePath, makeRelative );
        const json j = projectRelative;

        std::ofstream outFile( fileName );
        outFile << j;

        spdlog::debug( "Saved JSON for project (with relative image paths):\n{}", j.dump( 2 ) );
        spdlog::info( "Saved project to file {}", fileName );

        return true;
    }
    catch ( const std::exception& e )
    {
        spdlog::error( "Error saving project to JSON file: {}", e.what() );
        return false;
    }
}


bool openAffineTxFile( glm::dmat4& matrix, const std::string& fileName )
{
    std::ifstream inFile;
    inFile.exceptions( inFile.exceptions() | std::ifstream::badbit );

    try
    {
        inFile.open( fileName, std::ios_base::in );

        if ( ! inFile )
        {
            throw std::system_error( errno, std::system_category(),
                                     "Failed to open input file " + fileName );
        }

        std::vector< std::vector<double> > rows;
        std::string temp;

        while ( std::getline( inFile, temp ) )
        {
            std::istringstream buffer( temp );

            const std::vector<double> row{ ( std::istream_iterator<double>(buffer) ),
                        std::istream_iterator<double>() };

            if ( 4 != row.size() )
            {
                std::ostringstream ss;
                ss << "4x4 affine matrix row " << rows.size() + 1
                   << " read with invalid length (" << row.size() << ")" << std::ends;
                throw std::length_error( ss.str() );
            }

            rows.push_back( row );
        }

        if ( 4 != rows.size() )
        {
            std::ostringstream ss;
            ss << "4x4 affine matrix read with invalid number of rows ("
               << rows.size() << ")" << std::ends;
            throw std::length_error( ss.str() );
        }

        for ( uint32_t c = 0; c < 4; ++c )
        {
            for ( uint32_t r = 0; r < 4; ++r )
            {
                matrix[static_cast<int>(c)][static_cast<int>(r)] = rows[r][c];
            }
        }

        return true;
    }
    catch ( const std::ios_base::failure& e )
    {
#if HAS_IOS_BASE_FAILURE_DERIVED_FROM_SYSTEM_ERROR
        // e.code() is only available if the lib actually follows ISO ??27.5.3.1.1
        // and derives ios_base::failure from system_error
        spdlog::error( "Error #{} on opening file {}: {}",
                       e.code().value(), fileName, e.code().message() );

        if ( std::make_error_condition( std::io_errc::stream ) == e.code() )
        {
            spdlog::error( "Stream error opening file {}", fileName );
        }
        else
        {
            spdlog::error( "Unknown failure opening file {}", fileName );
        }
#else
        spdlog::error( "Error #{}: {}", errno, ::strerror(errno) );
#endif

        spdlog::error( "Failure while reading affine transformation from file {}: {}",
                       fileName, e.what() );
        return false;
    }
    catch ( const std::exception& e )
    {
        spdlog::error( "Invalid 4x4 affine transformation matrix in file {}: {}",
                       fileName, e.what() );
        return false;
    }
}


bool saveAffineTxFile( const glm::dmat4& matrix, const std::string& fileName )
{
    std::ofstream outFile;
    outFile.exceptions( outFile.exceptions() | std::ofstream::badbit | std::ofstream::failbit );

    try
    {
        outFile.open( fileName, std::ofstream::out );

        if ( ! outFile )
        {
            throw std::system_error( errno, std::system_category(),
                                     "Failed to open output file " + fileName );
        }

        for ( int r = 0; r < 4; ++r )
        {
            for ( int c = 0; c < 4; ++c )
            {
                outFile << matrix[c][r] << " ";
            }
            outFile << "\n";
        }

        return true;
    }
    catch ( const std::ios_base::failure& e )
    {
#if HAS_IOS_BASE_FAILURE_DERIVED_FROM_SYSTEM_ERROR
        // e.code() is only available if the lib actually follows ISO ??27.5.3.1.1
        // and derives ios_base::failure from system_error
        spdlog::error( "Error #{} on opening file {}: {}",
                       e.code().value(), fileName, e.code().message() );

        if ( std::make_error_condition( std::io_errc::stream ) == e.code() )
        {
            spdlog::error( "Stream error opening file {}", fileName );
        }
        else
        {
            spdlog::error( "Unknown failure opening file {}", fileName );
        }
#else
        spdlog::error( "Error #{}: {}", errno, ::strerror(errno) );
#endif

        spdlog::error( "Failure while writing affine transformation to file {}: {}",
                       fileName, e.what() );
        return false;
    }
    catch ( const std::exception& e )
    {
        spdlog::error( "Could not write 4x4 affine transformation matrix to file {}: {}",
                       fileName, e.what() );
        return false;
    }
}


bool openLandmarkGroupCsvFile(
        std::map< size_t, PointRecord<glm::vec3> >& landmarks,
        const std::string& csvFileName )
{
    std::ifstream inFile;
    inFile.exceptions( inFile.exceptions() | std::ifstream::badbit );

    try
    {
        spdlog::debug( "Opening landmarks CSV file {}", csvFileName );
        inFile.open( csvFileName, std::ios_base::in );

        if ( ! inFile || ! inFile.good() )
        {
            spdlog::error( "Error opening landmarks CSV file {}", csvFileName );
            throw std::system_error( errno, std::system_category(),
                                     "Failed to open CSV file " + csvFileName );
        }

        int lineNum = 1;

        std::string line;
        std::string colName;
        int numCols = 0;

        // Read the first line
        std::getline( inFile, line );
        ++lineNum;

//        spdlog::trace( "\n\nReading line: {}\n\n\n", line );

        std::istringstream ssHeader( line );

        // Read the column headers into colName (they are not used)
        while ( std::getline( ssHeader, colName, ',' ) )
        {
            spdlog::debug( "Read column name {}", colName );
            ++numCols;
        }

        // The expected columns are (with the last column optional)
        // index ,X ,Y ,Z [,name]
        if ( numCols < 4 )
        {
            spdlog::error( "Expected at least four columns (id, x, y, z) "
                           "when reading landmarks CSV file {}, "
                           "but only read {} columns", csvFileName, numCols );
            return false;
        }

        // Is the name column provided?
        const bool nameProvided = ( numCols >= 5 );

        // Read all lines containing landmark data
        while ( std::getline( inFile, line ) )
        {
//            spdlog::trace( "Reading line: {}", line );

            std::stringstream ssLm( line );

            int landmarkIndex = 0;
            glm::vec3 landmarkPos{ 0.0f };
            std::optional<std::string> landmarkName;

            int col = 0;
            std::string val;

            while ( std::getline( ssLm, val, ',' ) )
            {
//                spdlog::trace( "\tval: {}", val );

                switch ( col )
                {
                case 0: landmarkIndex = std::stoi( val ); break;
                case 1: landmarkPos.x = std::stof( val ); break;
                case 2: landmarkPos.y = std::stof( val ); break;
                case 3: landmarkPos.z = std::stof( val ); break;
                case 4:
                {
                    if ( nameProvided )
                    {
                        landmarkName = val;
                    }
                    break;
                }
                default: break; // ignore any more columns
                }

                // If the next token is a comma, ignore it
                if ( ',' == ssLm.peek() )
                {
                    ssLm.ignore();
                }

                ++col;
            }

            if ( nameProvided && ( col < numCols - 1 ) )
            {
                // The name is optional, so only check col against numCols - 1
                spdlog::error( "Line {} of landmarks CSV file {} has {} entries, "
                               "which is less than the expected {} entries",
                               lineNum, csvFileName, col, numCols - 1 );
                return false;
            }
            else if ( ! nameProvided && ( col < numCols ) )
            {
                spdlog::error( "Line {} of landmarks CSV file {} has {} entries, "
                               "which is less than the expected {} entries",
                               lineNum, csvFileName, col, numCols );
                return false;
            }

//            if ( ! landmarkName )
//            {
//                // Assign default name:
//                std::ostringstream ss;
//                ss << "LM " << landmarkIndex;
//                landmarkName = ss.str();
//            }

            if ( landmarkIndex < 0 )
            {
                spdlog::error( "Invalid negative landmark index ({}) on line {} "
                               "of landmarks CSV file {}",
                               landmarkIndex, lineNum, csvFileName );
                return false;
            }

            const auto r = landmarks.try_emplace(
                        static_cast<uint32_t>( landmarkIndex ), landmarkPos, *landmarkName );

            if ( ! r.second )
            {
                spdlog::warn( "Unable to insert landmark '{}', because index {} is already used",
                              *landmarkName, landmarkIndex );
            }

            ++lineNum;
        }

        return true;
    }
    catch ( const std::ios_base::failure& e )
    {
#if HAS_IOS_BASE_FAILURE_DERIVED_FROM_SYSTEM_ERROR
        // e.code() is only available if the lib actually follows ISO ??27.5.3.1.1
        // and derives ios_base::failure from system_error
        spdlog::error( "Error #{} on opening CSV file {}: {}",
                       e.code().value(), csvFileName, e.code().message() );

        if ( std::make_error_condition( std::io_errc::stream ) == e.code() )
        {
            spdlog::error( "Stream error opening CSV file {}", csvFileName );
        }
        else
        {
            spdlog::error( "Unknown failure opening CSV file {}", csvFileName );
        }
#else
        spdlog::error( "Error #{}: {}", errno, ::strerror(errno) );
#endif

        spdlog::error( "Failure while reading landmark CSV file {}: {}",
                       csvFileName, e.what() );
        return false;
    }
    catch ( const std::exception& e )
    {
        spdlog::error( "Invalid landmark CSV file {}: {}", csvFileName, e.what() );
        return false;
    }
}


bool saveLandmarkGroupCsvFile(
        const std::map< size_t, PointRecord<glm::vec3> >& landmarks,
        const std::string& csvFileName )
{
    std::ofstream outFile;
    outFile.exceptions( outFile.exceptions() | std::ofstream::badbit | std::ofstream::failbit );

    try
    {
        outFile.open( csvFileName, std::ofstream::out );

        if ( ! outFile )
        {
            throw std::system_error(
                        errno, std::system_category(),
                        "Failed to open output CSV file " + csvFileName );
        }

        static const std::string sk_header( "ID,X,Y,Z,Name" );

        outFile << sk_header << "\n";

        for ( const auto& lm : landmarks )
        {
            const auto id = lm.first;
            const auto pos = lm.second.getPosition();
            const auto name = lm.second.getName();

            outFile << id << "," << pos.x << "," << pos.y
                    << "," << pos.z << "," << name << "\n";
        }

        return true;
    }
    catch ( const std::ios_base::failure& e )
    {
#if HAS_IOS_BASE_FAILURE_DERIVED_FROM_SYSTEM_ERROR
        // e.code() is only available if the lib actually follows ISO ??27.5.3.1.1
        // and derives ios_base::failure from system_error
        spdlog::error( "Error #{} on opening CSV file {}: {}",
                       e.code().value(), csvFileName, e.code().message() );

        if ( std::make_error_condition( std::io_errc::stream ) == e.code() )
        {
            spdlog::error( "Stream error opening CSV file {}", csvFileName );
        }
        else
        {
            spdlog::error( "Unknown failure opening CSV file {}", csvFileName );
        }
#else
        spdlog::error( "Error #{}: {}", errno, ::strerror(errno) );
#endif

        spdlog::error( "Failure while writing landmarks to CSV file {}: {}",
                       csvFileName, e.what() );
        return false;
    }
    catch ( const std::exception& e )
    {
        spdlog::error( "Could not write landmarks to CSV file {}: {}",
                       csvFileName, e.what() );
        return false;
    }
}

bool openAnnotationsFromJsonFile(
        std::vector<Annotation>& annots,
        const std::string& jsonFileName )
{
    std::ifstream inFile;
    inFile.exceptions( inFile.exceptions() | std::ifstream::badbit );

    try
    {
        spdlog::debug( "Opening annotations JSON file {}", jsonFileName );
        inFile.open( jsonFileName, std::ios_base::in );

        if ( ! inFile || ! inFile.good() )
        {
            spdlog::error( "Error opening annotations JSON file {}", jsonFileName );
            throw std::system_error(
                        errno, std::system_category(),
                        "Failed to open JSON file " + jsonFileName );
        }

        json j;
        inFile >> j;

        annots = j.get< std::vector<Annotation> >();

        spdlog::debug( "Parsed {} annotation(s) from JSON:\n{}", annots.size(), j.dump( 2 ) );

        return true;
    }
    catch ( const std::ios_base::failure& e )
    {
#if HAS_IOS_BASE_FAILURE_DERIVED_FROM_SYSTEM_ERROR
        // e.code() is only available if the lib actually follows ISO ??27.5.3.1.1
        // and derives ios_base::failure from system_error
        spdlog::error( "Error #{} on opening annotations JSON file {}: {}",
                       e.code().value(), jsonFileName, e.code().message() );

        if ( std::make_error_condition( std::io_errc::stream ) == e.code() )
        {
            spdlog::error( "Stream error opening annotations JSON file {}", jsonFileName );
        }
        else
        {
            spdlog::error( "Unknown failure opening annotations JSON file {}", jsonFileName );
        }
#else
        spdlog::error( "Error #{}: {}", errno, ::strerror(errno) );
#endif

        spdlog::error( "Failure while reading annotations JSON file {}: {}",
                       jsonFileName, e.what() );
        return false;
    }
    catch ( const std::exception& e )
    {
        spdlog::error( "Invalid annotations JSON file {}: {}", jsonFileName, e.what() );
        return false;
    }
}

void appendAnnotationToJson( const Annotation& annot, json& j )
{
    j.emplace_back( annot );
}

bool saveToJsonFile(
        const nlohmann::json& j,
        const std::string& jsonFileName )
{
    std::ofstream outFile;
    outFile.exceptions( outFile.exceptions() | std::ofstream::badbit | std::ofstream::failbit );

    try
    {
        outFile.open( jsonFileName, std::ofstream::out );

        if ( ! outFile )
        {
            throw std::system_error(
                        errno, std::system_category(),
                        "Failed to open output JSON file " + jsonFileName );
        }

        outFile << j.dump( 2 );

        spdlog::debug( "Saved to JSON file {}:\n{}", jsonFileName, j.dump( 2 ) );
        spdlog::info( "Saved to JSON file {}", jsonFileName );

        return true;
    }
    catch ( const std::ios_base::failure& e )
    {
#if HAS_IOS_BASE_FAILURE_DERIVED_FROM_SYSTEM_ERROR
        // e.code() is only available if the lib actually follows ISO ??27.5.3.1.1
        // and derives ios_base::failure from system_error
        spdlog::error( "Error #{} on opening JSON file {}: {}",
                       e.code().value(), jsonFileName, e.code().message() );

        if ( std::make_error_condition( std::io_errc::stream ) == e.code() )
        {
            spdlog::error( "Stream error opening JSON file {}", jsonFileName );
        }
        else
        {
            spdlog::error( "Unknown failure opening JSON file {}", jsonFileName );
        }
#else
        spdlog::error( "Error #{}: {}", errno, ::strerror(errno) );
#endif

        spdlog::error( "Failure while writing to JSON file {}: {}",
                       jsonFileName, e.what() );
        return false;
    }
    catch ( const std::exception& e )
    {
        spdlog::error( "Could not write to JSON file {}: {}",
                       jsonFileName, e.what() );
        return false;
    }
}

} // namespace serialize
