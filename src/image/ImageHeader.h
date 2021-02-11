#ifndef IMAGE_HEADER_H
#define IMAGE_HEADER_H

#include "common/Types.h"
#include "image/ImageIoInfo.h"

#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <ostream>
#include <string>
#include <utility>


/**
 * @brief Image header with data set upon creation or loading of image.
 */
class ImageHeader
{
public:

    explicit ImageHeader() = default;
    ImageHeader( const ImageIoInfo& ioInfoOnDisk, const ImageIoInfo& ioInfoInMemory );

    ImageHeader( const ImageHeader& ) = default;
    ImageHeader& operator=( const ImageHeader& ) = default;

    ImageHeader( ImageHeader&& ) = default;
    ImageHeader& operator=( ImageHeader&& ) = default;

    ~ImageHeader() = default;


    void adjustToScalarUCharFormat();

    bool existsOnDisk() const;
    void setExistsOnDisk( bool );

    const std::string& fileName() const; //!< File name
    void setFileName( std::string fileName );

    uint32_t numComponentsPerPixel() const; //!< Number of components per pixel
    uint64_t numPixels() const; //!< Number of pixels in the image

    uint64_t fileImageSizeInBytes() const; //!< Image size in bytes (in file)
    uint64_t memoryImageSizeInBytes() const; //!< Image size in bytes (in memory)

    PixelType pixelType() const; //!< Pixel type
    std::string pixelTypeAsString() const;

    ComponentType fileComponentType() const; //!< Pixel component type
    std::string fileComponentTypeAsString() const;
    uint32_t fileComponentSizeInBytes() const; //!< Size of component in bytes

    ComponentType memoryComponentType() const; //!< Pixel component type in memory
    std::string memoryComponentTypeAsString() const;
    uint32_t memoryComponentSizeInBytes() const; //!< Size of component in bytes in memory


    const glm::uvec3& pixelDimensions() const; //!< Pixel dimensions (i.e. pixel matrix size)
    const glm::vec3& origin() const; //!< Origin in Subject space
    const glm::vec3& spacing() const; //!< Pixel spacing in Subject space
    const glm::mat3& directions() const; //!< Axis directions in Subject space, stored column-wise

    /// Minimum and maximum corners of the image's axis-aligned bounding box in Subject space
    const std::pair< glm::vec3, glm::vec3 >& boundingBoxMinMaxCorners() const;

    /// All corners of the image's axis-aligned bounding box in Subject space
    const std::array< glm::vec3, 8 >& boundingBoxCorners() const;

    /// Center of the image's axis-aligned bounding box in Subject space
    const glm::vec3& boundingBoxCenter() const;

    /// Size of the image's axis-aligned bounding box in Subject space
    const glm::vec3& boundingBoxSize() const;

    /// Three-character "SPIRAL" code defining the anatomical orientation of the image in Subject space,
    /// where positive X, Y, and Z axes correspond to the physical Left, Posterior, and Superior directions,
    /// respectively. The acroynm stands for "Superior, Posterior, Inferior, Right, Anterior, Left".
    const std::string& spiralCode() const;

    /// Flag indicating whether the image directions are oblique (i.e. skew w.r.t. the physical X, Y, Z, axes)
    bool isOblique() const;

    friend std::ostream& operator<< ( std::ostream&, const ImageHeader& );


private:

    void setSpace( const ImageIoInfo& ioInfo );
    void setBoundingBox();

    bool m_existsOnDisk; //!< Flag that the image exists on disk
    std::string m_fileName; //!< File name

    uint32_t m_numComponentsPerPixel; //!< Number of components per pixel
    uint64_t m_numPixels; //!< Number of pixels in the image

    uint64_t m_fileImageSizeInBytes; //!< Image size in bytes (in file on disk)
    uint64_t m_memoryImageSizeInBytes; //!< Image size in bytes (in memory)

    PixelType m_pixelType; //!< Pixel type
    std::string m_pixelTypeAsString;

    ComponentType m_fileComponentType; //!< Original file pixel component type
    std::string m_fileComponentTypeAsString;
    uint32_t m_fileComponentSizeInBytes; //!< Size of original fie pixel component in bytes

    ComponentType m_memoryComponentType; //!< Pixel component type, as loaded in memory buffer
    std::string m_memoryComponentTypeAsString;
    uint32_t m_memoryComponentSizeInBytes; //!< Size of component in bytes, as loaded in memory buffer

    glm::uvec3 m_pixelDimensions; //!< Pixel dimensions (i.e. pixel matrix size)
    glm::vec3 m_origin; //!< Origin in Subject space
    glm::vec3 m_spacing; /// Pixel spacing in Subject space
    glm::mat3 m_directions; /// Axis directions in Subject space, stored column-wise

    /// Minimum and maximum corners of the image's axis-aligned bounding box in Subject space
    std::pair< glm::vec3, glm::vec3 > m_boundingBoxMinMaxCorners;

    /// All corners of the image's axis-aligned bounding box in Subject space
    std::array< glm::vec3, 8 > m_boundingBoxCorners;

    /// Center of the image's axis-aligned bounding box in Subject space
    glm::vec3 m_boundingBoxCenter;

    /// Size of the image's axis-aligned bounding box in Subject space
    glm::vec3 m_boundingBoxSize;

    /// Three-character "SPIRAL" code defining the anatomical orientation of the image in Subject space,
    /// where positive X, Y, and Z axes correspond to the physical Left, Posterior, and Superior directions,
    /// respectively. The acroynm stands for "Superior, Posterior, Inferior, Right, Anterior, Left".
    std::string m_spiralCode;

    /// Flag indicating whether the image directions are oblique (i.e. skew w.r.t. the physical X, Y, Z, axes)
    bool m_isOblique;
};


std::ostream& operator<< ( std::ostream&, const ImageHeader& );

#endif // IMAGE_HEADER_H
