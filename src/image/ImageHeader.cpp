#include "image/ImageHeader.h"
#include "image/ImageUtility.h"

#include "common/Exception.hpp"
#include "common/MathFuncs.h"

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <spdlog/spdlog.h>


ImageHeader::ImageHeader( const ImageIoInfo& ioInfoOnDisk, const ImageIoInfo& ioInfoInMemory )
    :
      m_existsOnDisk( true ),
      m_fileName( ioInfoOnDisk.m_fileInfo.m_fileName ),
      m_numComponentsPerPixel( ioInfoOnDisk.m_pixelInfo.m_numComponents ),
      m_numPixels( ioInfoOnDisk.m_sizeInfo.m_imageSizeInPixels ),

      m_fileImageSizeInBytes( ioInfoOnDisk.m_sizeInfo.m_imageSizeInBytes ),
      m_memoryImageSizeInBytes( ioInfoInMemory.m_sizeInfo.m_imageSizeInBytes ),

      m_pixelType( fromItkPixelType( ioInfoOnDisk.m_pixelInfo.m_pixelType ) ),
      m_pixelTypeAsString( ioInfoOnDisk.m_pixelInfo.m_pixelTypeString ),

      m_fileComponentType( fromItkComponentType( ioInfoOnDisk.m_componentInfo.m_componentType ) ),
      m_fileComponentTypeAsString( ioInfoOnDisk.m_componentInfo.m_componentTypeString ),
      m_fileComponentSizeInBytes( ioInfoOnDisk.m_componentInfo.m_componentSizeInBytes ),

      m_memoryComponentType( fromItkComponentType( ioInfoInMemory.m_componentInfo.m_componentType ) ),
      m_memoryComponentTypeAsString( ioInfoInMemory.m_componentInfo.m_componentTypeString ),
      m_memoryComponentSizeInBytes( ioInfoInMemory.m_componentInfo.m_componentSizeInBytes )
{
    if ( ComponentType::Undefined == m_memoryComponentType )
    {
        spdlog::error( "Cannot set header for image {} with undefined component type",
                       ioInfoInMemory.m_fileInfo.m_fileName );
        throw_debug( "Undefined component type" )
    }
    else if ( PixelType::Undefined == m_pixelType )
    {
        spdlog::error( "Cannot set header for image {} with undefined pixel type",
                       ioInfoInMemory.m_fileInfo.m_fileName );
        throw_debug( "undefined pixel type" )
    }

    setSpace( ioInfoInMemory );
    std::tie( m_spiralCode, m_isOblique ) = math::computeSpiralCodeFromDirectionMatrix( m_directions );
}


void ImageHeader::setSpace( const ImageIoInfo& ioInfo )
{
    const uint32_t numDim = ioInfo.m_spaceInfo.m_numDimensions;
    std::vector<size_t> dims = ioInfo.m_spaceInfo.m_dimensions;
    std::vector<double> orig = ioInfo.m_spaceInfo.m_origin;
    std::vector<double> space = ioInfo.m_spaceInfo.m_spacing;
    std::vector< std::vector<double> > dirs = ioInfo.m_spaceInfo.m_directions;

    // Expect a 3D image
    if ( numDim != 3 || orig.size() != 3 || space.size() != 3 || dims.size() != 3 || dirs.size() != 3 )
    {
        spdlog::debug( "Vector sizes: numDims = {}, origin = {}, spacing = {}, dims = {}, directions = {}",
                       numDim, orig.size(), space.size(), dims.size(), dirs.size() );

        if ( numDim == 2 && orig.size() == 2 && space.size() == 2 && dims.size() == 2 && dirs.size() == 4 )
        {
            // The image is 2D: augment to 3D
            orig.push_back( 0.0 );
            space.push_back( 1.0 );
            dims.push_back( 1 );

            std::vector< std::vector<double> > d3x3( 3 );
            d3x3[0].resize( 3 );
            d3x3[1].resize( 3 );
            d3x3[2].resize( 3 );

            d3x3[0][0] = dirs[0][0];  d3x3[0][1] = dirs[0][1];  d3x3[0][2] = 0.0;
            d3x3[1][0] = dirs[1][0];  d3x3[1][1] = dirs[1][1];  d3x3[1][2] = 0.0;
            d3x3[2][0] = 0.0;         d3x3[2][1] = 0.0;         d3x3[2][2] = 1.0;

            dirs = std::move( d3x3 );
        }
        else
        {
            throw_debug( "Image must have dimension of 2 or 3" )
        }
    }

    m_pixelDimensions = glm::uvec3{ dims[0], dims[1], dims[2] };
    m_spacing = glm::vec3{ space[0], space[1], space[2] };
    m_origin = glm::vec3{ orig[0], orig[1], orig[2] };

    // Set matrix of direction vectors in column-major order
    m_directions = glm::mat3{ dirs[0][0], dirs[0][1], dirs[0][2],
                              dirs[1][0], dirs[1][1], dirs[1][2],
                              dirs[2][0], dirs[2][1], dirs[2][2] };

    setBoundingBox();
}


void ImageHeader::setBoundingBox()
{
    m_boundingBoxMinMaxCorners = math::computeImageSubjectAABBoxCorners(
                m_pixelDimensions, m_directions, m_spacing, m_origin );

    m_boundingBoxCorners = math::computeAllBoxCorners( m_boundingBoxMinMaxCorners );
    m_boundingBoxCenter = 0.5f * ( m_boundingBoxMinMaxCorners.first + m_boundingBoxMinMaxCorners.second );
    m_boundingBoxSize = m_boundingBoxMinMaxCorners.second - m_boundingBoxMinMaxCorners.first;
}


void ImageHeader::adjustToScalarUCharFormat()
{
    m_numComponentsPerPixel = 1;

    m_pixelType = PixelType::Scalar;
    m_pixelTypeAsString = "scalar";

    m_fileComponentType = ComponentType::UInt8;
    m_fileComponentTypeAsString = "uchar";
    m_fileComponentSizeInBytes =  1;

    m_memoryComponentType = ComponentType::UInt8;
    m_memoryComponentTypeAsString = "uchar";
    m_memoryComponentSizeInBytes = 1;

    m_fileImageSizeInBytes = m_fileComponentSizeInBytes * m_numComponentsPerPixel * m_numPixels;
    m_memoryImageSizeInBytes = m_memoryComponentSizeInBytes * m_numComponentsPerPixel * m_numPixels;
}


bool ImageHeader::existsOnDisk() const { return m_existsOnDisk; }
void ImageHeader::setExistsOnDisk( bool onDisk ) { m_existsOnDisk = onDisk; }

const std::string& ImageHeader::fileName() const { return m_fileName; }
void ImageHeader::setFileName( std::string fileName ) { m_fileName = std::move( fileName ); }

uint32_t ImageHeader::numComponentsPerPixel() const { return m_numComponentsPerPixel; }
uint64_t ImageHeader::numPixels() const { return m_numPixels; }

uint64_t ImageHeader::fileImageSizeInBytes() const { return m_fileImageSizeInBytes; }
uint64_t ImageHeader::memoryImageSizeInBytes() const { return m_memoryImageSizeInBytes; }

PixelType ImageHeader::pixelType() const { return m_pixelType; }
std::string ImageHeader::pixelTypeAsString() const { return m_pixelTypeAsString; }

ComponentType ImageHeader::fileComponentType() const { return m_fileComponentType; }
std::string ImageHeader::fileComponentTypeAsString() const { return m_fileComponentTypeAsString; }
uint32_t ImageHeader::fileComponentSizeInBytes() const { return m_fileComponentSizeInBytes; }

ComponentType ImageHeader::memoryComponentType() const { return m_memoryComponentType; }
std::string ImageHeader::memoryComponentTypeAsString() const { return m_memoryComponentTypeAsString; }
uint32_t ImageHeader::memoryComponentSizeInBytes() const { return m_memoryComponentSizeInBytes; }

const glm::uvec3& ImageHeader::pixelDimensions() const { return m_pixelDimensions; }
const glm::vec3& ImageHeader::origin() const { return m_origin; }
const glm::vec3& ImageHeader::spacing() const { return m_spacing; }
const glm::mat3& ImageHeader::directions() const { return m_directions; }

const std::pair< glm::vec3, glm::vec3 >& ImageHeader::boundingBoxMinMaxCorners() const
{ return m_boundingBoxMinMaxCorners; }
const std::array< glm::vec3, 8 >& ImageHeader::boundingBoxCorners() const { return m_boundingBoxCorners; }
const glm::vec3& ImageHeader::boundingBoxCenter() const { return m_boundingBoxCenter; }
const glm::vec3& ImageHeader::boundingBoxSize() const { return m_boundingBoxSize; }

const std::string& ImageHeader::spiralCode() const { return m_spiralCode; }
bool ImageHeader::isOblique() const { return m_isOblique; }


std::ostream& operator<< ( std::ostream& os, const ImageHeader& header )
{
    os << "Exists on disk: "                   << std::boolalpha << header.m_existsOnDisk
       << "\nFile name: "                      << header.m_fileName
       << "\nPixel type: "                     << header.m_pixelTypeAsString
       << "\nNum. components per pixel: "      << header.m_numComponentsPerPixel

       << "\n\nComponent type (disk): "        << header.m_fileComponentTypeAsString
       << "\nComponent size (bytes, disk): "   << header.m_fileComponentSizeInBytes

       << "\nComponent type (memory): "        << header.m_memoryComponentTypeAsString
       << "\nComponent size (bytes, memory): " << header.m_memoryComponentSizeInBytes

       << "\n\nImage size (pixels): "          << header.m_numPixels
       << "\nImage size (bytes, disk): "       << header.m_fileImageSizeInBytes
       << "\nImage size (bytes, memory): "     << header.m_memoryImageSizeInBytes

       << "\n\nDimensions (pixels): "          << glm::to_string( header.m_pixelDimensions )
       << "\nOrigin (mm): "                    << glm::to_string( header.m_origin )
       << "\nSpacing (mm): "                   << glm::to_string( header.m_spacing )
       << "\nDirections: "                     << glm::to_string( header.m_directions )

       << "\n\nBounding box corners (mm): "
       << glm::to_string( header.m_boundingBoxMinMaxCorners.first ) << ", "
       << glm::to_string( header.m_boundingBoxMinMaxCorners.second )

       << "\nBounding box center (mm): "       << glm::to_string( header.m_boundingBoxCenter )
       << "\nBounding box size (mm): "         << glm::to_string( header.m_boundingBoxSize )

       << "\n\nSPIRAL code: "                  << header.m_spiralCode
       << "\nIs oblique: "                     << std::boolalpha << header.m_isOblique;

    return os;
}
