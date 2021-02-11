#ifndef INPUT_PARAMS_H
#define INPUT_PARAMS_H

#include <spdlog/spdlog.h>

#include <optional>
#include <ostream>
#include <string>
#include <utility>


/**
 * @brief Antropy input parameters read from command line
 */
struct InputParams
{
    // Path to an image and, optionally, its corresponding segmentation
    using ImageSegPair = std::pair< std::string, std::optional<std::string> >;

    // All image files. The first image is the reference image.
    std::vector<ImageSegPair> imageFiles;

    // An optional project file with the images.
    std::optional< std::string > projectFile;

    spdlog::level::level_enum consoleLogLevel;

    // Have the parameters been successfully set?
    bool set = false;
};


std::ostream& operator<<( std::ostream&, const InputParams& );

#endif // INPUT_PARAMS_H
