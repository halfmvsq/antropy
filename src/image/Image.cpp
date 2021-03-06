#include "image/Image.h"
#include "image/ImageUtility.h"
#include "image/ImageUtility.tpp"

#include "common/Exception.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <memory>
#include <sstream>


Image::Image(
        const std::string& fileName,
        ImageRepresentation imageRep,
        MultiComponentBufferType bufferType )
    :
      m_data_int8(),
      m_data_uint8(),
      m_data_int16(),
      m_data_uint16(),
      m_data_int32(),
      m_data_uint32(),
      m_data_float32(),

      m_imageRep( std::move(imageRep) ),
      m_bufferType( std::move(bufferType) ),

      m_ioInfoOnDisk(),
      m_ioInfoInMemory()
{
    // Read all data from disk to an ITK image with float32 pixel components
    using ReadComponentType = float;
    using ComponentImageType = itk::Image<ReadComponentType, 3>;

    // Statistics per component are stored as double
    using StatsType = double;

    // Maximum number of components to load for images with interleaved buffer components
    static constexpr size_t MAX_COMPS = 4;

    try
    {
        const itk::ImageIOBase::Pointer imageIo = createStandardImageIo( fileName.c_str() );

        if ( ! imageIo || imageIo.IsNull() )
        {
            spdlog::error( "Error creating ImageIOBase for image {}", fileName );
            throw_debug( "Error creating ImageIOBase" )
        }

        if ( ! m_ioInfoOnDisk.set( imageIo ) )
        {
            spdlog::error( "Error setting image IO information for image {}", fileName );
            throw_debug( "Error setting image IO information" )
        }

        // The image information in memory may not match the information on disk
        m_ioInfoInMemory = m_ioInfoOnDisk;

        const size_t numPixels = m_ioInfoOnDisk.m_sizeInfo.m_imageSizeInPixels;
        const size_t numComps = m_ioInfoOnDisk.m_pixelInfo.m_numComponents;
        const bool isVectorImage = ( numComps > 1 );

        spdlog::info( "Attempting to load image {} with {} pixels and {} components per pixel",
                      fileName, numPixels, numComps );

        // Extract statistics of each image component into a vector
        std::vector< ComponentStats<StatsType> > componentStats;

        if ( isVectorImage )
        {
            // Load multi-component image

            typename itk::ImageBase<3>::Pointer baseImage =
                    readImage<ReadComponentType, 3, true>( fileName );

            if ( ! baseImage )
            {
                spdlog::error( "Unable to read vector image {}", fileName );
                throw_debug( "Unable to read vector image" )
            }

            // Split the base image into component images. Load a maximum of MAX_COMPS components
            // for an image with interleaved component buffers.
            size_t numCompsToLoad = numComps;

            if ( MultiComponentBufferType::InterleavedImage == m_bufferType )
            {
                numCompsToLoad = std::min( numCompsToLoad, MAX_COMPS );

                if ( numComps > MAX_COMPS )
                {
                    spdlog::warn( "The number of image components ({}) exceeds that maximum that will be loaded ({}) "
                                  "because this image uses interleaved buffer format", numComps, MAX_COMPS );
                }
            }

            std::vector< typename ComponentImageType::Pointer > componentImages =
                splitImageIntoComponents<ReadComponentType, 3>( baseImage );

            if ( componentImages.size() < numCompsToLoad )
            {
                spdlog::error( "Only {} component images were loaded, but {} components were expected",
                               componentImages.size(), numCompsToLoad );

                numCompsToLoad = componentImages.size();
            }

            if ( ImageRepresentation::Segmentation == m_imageRep )
            {
                spdlog::warn( "Loading a segmentation image {} with {} components. "
                              "Only the first component of the segmentation will be used",
                              fileName, numComps );
                numCompsToLoad = 1;
            }

            if ( 0 == numCompsToLoad )
            {
                spdlog::error( "No components to load for image {}", fileName );
                throw_debug( "No components to load for image" )
            }

            // If interleaving vector components, then create a single buffer
            std::unique_ptr<ReadComponentType[]> allComponentBuffers = nullptr;

            if ( MultiComponentBufferType::InterleavedImage == m_bufferType )
            {
                allComponentBuffers = std::make_unique<ReadComponentType[]>( numPixels * numCompsToLoad );

                if ( ! allComponentBuffers )
                {
                    spdlog::error( "Null buffer holding all components of image {}", fileName );
                    throw_debug( "Null image buffer" )
                }
            }

            // Load the buffers from the component images
            for ( size_t i = 0; i < numCompsToLoad; ++i )
            {
                if ( ! componentImages[i] )
                {
                    spdlog::error( "Null vector image component {} for image {}", i, fileName );
                    throw_debug( "Null vector component for image" )
                }

                const ReadComponentType* buffer = componentImages[i]->GetBufferPointer();

                if ( ! buffer )
                {
                    spdlog::error( "Null buffer of vector image component {} for image {}", i, fileName );
                    throw_debug( "Null buffer of vector image component" )
                }

                switch ( m_bufferType )
                {
                case MultiComponentBufferType::SeparateImages:
                {
                    if ( ImageRepresentation::Segmentation == m_imageRep )
                    {
                        loadSegBuffer( buffer, numPixels );
                    }
                    else
                    {
                        loadImageBuffer( buffer, numPixels );
                    }
                    break;
                }
                case MultiComponentBufferType::InterleavedImage:
                {
                    // Fill the interleaved buffer
                    for ( size_t p = 0; p < numPixels; ++p )
                    {
                        allComponentBuffers[numComps*p + i] = buffer[p];
                    }
                    break;
                }
                }

                componentStats.emplace_back( computeImageStatistics<ReadComponentType, StatsType, 3>(
                                                 componentImages[i] ) );
            }

            if ( MultiComponentBufferType::InterleavedImage == m_bufferType )
            {
                const size_t numElements = numPixels * numComps;

                if ( ImageRepresentation::Segmentation == m_imageRep )
                {
                    loadSegBuffer( allComponentBuffers.get(), numElements );
                }
                else
                {
                    loadImageBuffer( allComponentBuffers.get(), numElements );
                }
            }
        }
        else
        {
            // Load scalar, single-component image

            typename itk::ImageBase<3>::Pointer baseImage =
                    readImage<ReadComponentType, 3, false>( fileName );

            if ( ! baseImage )
            {
                spdlog::error( "Unable to read image {}", fileName );
                throw_debug( "Unable to read image" )
            }

            typename ComponentImageType::Pointer image =
                    downcastImageBaseToImage<ReadComponentType, 3>( baseImage );

            if ( ! image )
            {
                spdlog::error( "Null image for {}", fileName );
                throw_debug( "Null image" )
            }

            const ReadComponentType* buffer = image->GetBufferPointer();

            if ( ! buffer )
            {
                spdlog::error( "Null buffer of scalar image {}", fileName );
                throw_debug( "Null buffer of scalar image" )
            }

            if ( ImageRepresentation::Segmentation == m_imageRep )
            {
                loadSegBuffer( buffer, numPixels );
            }
            else
            {
                loadImageBuffer( buffer, numPixels );
            }

            componentStats.emplace_back( computeImageStatistics<ReadComponentType, StatsType, 3>( image ) );
        }

        m_header = ImageHeader( m_ioInfoOnDisk, m_ioInfoInMemory );

        m_tx = ImageTransformations(
                    m_header.pixelDimensions(),
                    m_header.spacing(),
                    m_header.origin(),
                    m_header.directions() );

        m_settings = ImageSettings(
                    getFileName( fileName, false ),
                    m_header.numComponentsPerPixel(),
                    m_header.memoryComponentType(),
                    std::move( componentStats ) );
    }
    catch ( const std::exception& e )
    {
        spdlog::error( "Exception while constructing image {}: {}", fileName, e.what() );
        throw_debug( "Exception while constructing image" )
    }
    catch ( ... )
    {
        spdlog::error( "Exception while constructing image {}", fileName );
        throw_debug( "Exception while constructing image" )
    }
}


Image::Image(
        const ImageHeader& header,
        std::string displayName,
        ImageRepresentation imageRep,
        MultiComponentBufferType bufferType )
    :
      m_data_int8(),
      m_data_uint8(),
      m_data_int16(),
      m_data_uint16(),
      m_data_int32(),
      m_data_uint32(),
      m_data_float32(),

      m_imageRep( std::move(imageRep) ),
      m_bufferType( std::move(bufferType) ),

      m_ioInfoOnDisk(),
      m_ioInfoInMemory(),

      m_header( header )
{
    // Temporary buffer component type
    using TempComponentType = float;

    // Statistics per component are stored as double
    using StatsType = double;

    // Maximum number of components to create for images with interleaved buffers
    static constexpr size_t MAX_COMPS = 4;

    // Default buffer value
    static constexpr TempComponentType DEFAULT_VALUE = 0;

    /// @todo cleanup
    m_ioInfoOnDisk.m_fileInfo.m_fileName = m_header.fileName();
    m_ioInfoOnDisk.m_componentInfo.m_componentType = itk::IOComponentEnum::UCHAR;
    m_ioInfoOnDisk.m_componentInfo.m_componentTypeString = "uchar";

    m_ioInfoInMemory = m_ioInfoOnDisk;

    const size_t numPixels = m_header.numPixels();
    const size_t numComps = m_header.numComponentsPerPixel();
    const bool isVectorImage = ( numComps > 1 );

    std::vector< ComponentStats<StatsType> > componentStats;

    if ( isVectorImage )
    {
        // Create multi-component image
        size_t numCompsToLoad = numComps;

        if ( MultiComponentBufferType::InterleavedImage == m_bufferType )
        {
            // Set a maximum of MAX_COMPS components
            numCompsToLoad = std::min( numCompsToLoad, MAX_COMPS );

            if ( numComps > MAX_COMPS )
            {
                spdlog::warn( "The number of image components ({}) exceeds that maximum that will be created ({}) "
                              "because this image uses interleaved buffer format", numComps, MAX_COMPS );
            }
        }

        if ( ImageRepresentation::Segmentation == m_imageRep )
        {
            spdlog::warn( "Attempting to create a segmentation image with {} components", numComps );
            spdlog::warn( "Only one component of the segmentation image will be created" );
            numCompsToLoad = 1;
        }

        if ( 0 == numCompsToLoad )
        {
            spdlog::error( "No components to create for image {}", m_header.fileName() );
            throw_debug( "No components to create for image" )
        }

        switch ( m_bufferType )
        {
        case MultiComponentBufferType::SeparateImages:
        {
            // Create a buffer for each component and load each separately:
            const std::vector<TempComponentType> buffer( numPixels, DEFAULT_VALUE );

            for ( size_t c = 0; c < numCompsToLoad; ++c )
            {
                if ( ImageRepresentation::Segmentation == m_imageRep )
                {
                    loadSegBuffer( buffer.data(), numPixels );
                }
                else
                {
                    loadImageBuffer( buffer.data(), numPixels );
                }
            }
            break;
        }
        case MultiComponentBufferType::InterleavedImage:
        {
            // Create a single buffer with interleaved components and load it once:
            const size_t N = numPixels * numComps;
            const std::vector<TempComponentType> allComponentBuffers( N, DEFAULT_VALUE );

            if ( ImageRepresentation::Segmentation == m_imageRep )
            {
                loadSegBuffer( allComponentBuffers.data(), N );
            }
            else
            {
                loadImageBuffer( allComponentBuffers.data(), N );
            }
            break;
        }
        }

        // Create default image statistics
        for ( size_t i = 0; i < numCompsToLoad; ++i )
        {
            componentStats.emplace_back( createDefaultImageStatistics<TempComponentType, StatsType, 3>(
                                             DEFAULT_VALUE, numPixels ) );
        }
    }
    else
    {
        // Create a scalar, single-component image
        const std::vector<TempComponentType> buffer( numPixels, DEFAULT_VALUE );

        if ( ImageRepresentation::Segmentation == m_imageRep )
        {
            loadSegBuffer( buffer.data(), numPixels );
        }
        else
        {
            loadImageBuffer( buffer.data(), numPixels );
        }

        // Create default image statistics
        componentStats.emplace_back( createDefaultImageStatistics<TempComponentType, StatsType, 3>(
                                         DEFAULT_VALUE, numPixels ) );
    }

    m_tx = ImageTransformations(
                m_header.pixelDimensions(),
                m_header.spacing(),
                m_header.origin(),
                m_header.directions() );

    m_settings = ImageSettings(
                std::move( displayName ),
                m_header.numComponentsPerPixel(),
                m_header.memoryComponentType(),
                std::move( componentStats ) );
}


void Image::loadImageBuffer( const float* buffer, size_t numElements )
{
    using CType = ::itk::ImageIOBase::IOComponentType;

    bool didCast = false;
    bool warnSizeConversion = false;

    switch ( m_ioInfoOnDisk.m_componentInfo.m_componentType )
    {
    case CType::UCHAR:  m_data_uint8.emplace_back( createBuffer<uint8_t>( buffer, numElements ) ); break;
    case CType::CHAR:   m_data_int8.emplace_back( createBuffer<int8_t>( buffer, numElements ) ); break;
    case CType::USHORT: m_data_uint16.emplace_back( createBuffer<uint16_t>( buffer, numElements ) ); break;
    case CType::SHORT:  m_data_int16.emplace_back( createBuffer<int16_t>( buffer, numElements ) ); break;
    case CType::UINT:   m_data_uint32.emplace_back( createBuffer<uint32_t>( buffer, numElements ) ); break;
    case CType::INT:    m_data_int32.emplace_back( createBuffer<int32_t>( buffer, numElements ) ); break;
    case CType::FLOAT:  m_data_float32.emplace_back( createBuffer<float>( buffer, numElements ) ); break;

    case CType::ULONG:
    case CType::ULONGLONG:
    {
        m_data_uint32.emplace_back( createBuffer<uint32_t>( buffer, numElements ) );
        m_ioInfoInMemory.m_componentInfo.m_componentType = CType::UINT;
        m_ioInfoInMemory.m_componentInfo.m_componentSizeInBytes = 4;

        didCast = true;
        warnSizeConversion = true;
        break;
    }

    case CType::LONG:
    case CType::LONGLONG:
    {
        m_data_int32.emplace_back( createBuffer<int32_t>( buffer, numElements ) );
        m_ioInfoInMemory.m_componentInfo.m_componentType = CType::INT;
        m_ioInfoInMemory.m_componentInfo.m_componentSizeInBytes = 4;

        didCast = true;
        warnSizeConversion = true;
        break;
    }

    case CType::DOUBLE:
    case CType::LDOUBLE:
    {
        m_data_float32.emplace_back( createBuffer<float>( buffer, numElements ) );
        m_ioInfoInMemory.m_componentInfo.m_componentType = CType::FLOAT;
        m_ioInfoInMemory.m_componentInfo.m_componentSizeInBytes = 4;

        didCast = true;
        warnSizeConversion = true;
        break;
    }

    case CType::UNKNOWNCOMPONENTTYPE:
    {
        spdlog::error( "Unknown component type in image {}", m_ioInfoOnDisk.m_fileInfo.m_fileName );
        throw_debug( "Unknown component type in image" )
        }
    }

    if ( didCast )
    {
        const std::string newTypeString = itk::ImageIOBase::GetComponentTypeAsString(
                    m_ioInfoInMemory.m_componentInfo.m_componentType );

        m_ioInfoInMemory.m_componentInfo.m_componentTypeString = newTypeString;

        m_ioInfoInMemory.m_sizeInfo.m_imageSizeInBytes =
                numElements * m_ioInfoInMemory.m_componentInfo.m_componentSizeInBytes;

        spdlog::info( "Cast image pixel component from type {} to {}",
                      m_ioInfoOnDisk.m_componentInfo.m_componentTypeString, newTypeString );

        if ( warnSizeConversion )
        {
            spdlog::warn( "Size conversion: "
                          "Possible loss of information when casting image pixel component from type {} to {}",
                          m_ioInfoOnDisk.m_componentInfo.m_componentTypeString, newTypeString );
        }
    }
}


void Image::loadSegBuffer( const float* buffer, size_t numElements )
{
    using CType = ::itk::ImageIOBase::IOComponentType;

    bool didCast = false;
    bool warnFloatConversion = false;
    bool warnSizeConversion = false;
    bool warnSignConversion = false;

    switch ( m_ioInfoOnDisk.m_componentInfo.m_componentType )
    {
    // No casting is needed for the cases of unsigned integers with 8, 16, or 32 bytes:
    case CType::UCHAR:
    {
        m_data_uint8.emplace_back( createBuffer<uint8_t>( buffer, numElements ) );
        break;
    }
    case CType::USHORT:
    {
        m_data_uint16.emplace_back( createBuffer<uint16_t>( buffer, numElements ) );
        break;
    }
    case CType::UINT:
    {
        m_data_uint32.emplace_back( createBuffer<uint32_t>( buffer, numElements ) );
        break;
    }

    // Signed 8-, 16-, and 32-bit integers are cast to unsigned 8-, 16-, and 32-bit integers:
    case CType::CHAR:
    {
        m_data_uint8.emplace_back( createBuffer<uint8_t>( buffer, numElements ) );
        m_ioInfoInMemory.m_componentInfo.m_componentType = CType::UCHAR;
        m_ioInfoInMemory.m_componentInfo.m_componentSizeInBytes = 1;

        didCast = true;
        warnSignConversion = true;
        break;
    }
    case CType::SHORT:
    {
        m_data_uint16.emplace_back( createBuffer<uint16_t>( buffer, numElements ) );
        m_ioInfoInMemory.m_componentInfo.m_componentType = CType::USHORT;
        m_ioInfoInMemory.m_componentInfo.m_componentSizeInBytes = 2;

        didCast = true;
        warnSignConversion = true;
        break;
    }
    case CType::INT:
    {
        m_data_uint32.emplace_back( createBuffer<uint32_t>( buffer, numElements ) );
        m_ioInfoInMemory.m_componentInfo.m_componentType = CType::UINT;
        m_ioInfoInMemory.m_componentInfo.m_componentSizeInBytes = 4;

        didCast = true;
        warnSignConversion = true;
        break;
    }

    // Unsigned long (64-bit) and long long (128-bit) integers are cast to unsigned 32-bit integers:
    case CType::ULONG:
    case CType::ULONGLONG:
    {
        m_data_uint32.emplace_back( createBuffer<uint32_t>( buffer, numElements ) );
        m_ioInfoInMemory.m_componentInfo.m_componentType = CType::UINT;
        m_ioInfoInMemory.m_componentInfo.m_componentSizeInBytes = 4;

        didCast = true;
        warnSizeConversion = true;
        break;
    }

    // Signed long (64-bit) and long long (128-bit) integers are cast to unsigned 32-bit integers:
    case CType::LONG:
    case CType::LONGLONG:
    {
        m_data_uint32.emplace_back( createBuffer<uint32_t>( buffer, numElements ) );
        m_ioInfoInMemory.m_componentInfo.m_componentType = CType::UINT;
        m_ioInfoInMemory.m_componentInfo.m_componentSizeInBytes = 4;

        didCast = true;
        warnSizeConversion = true;
        warnSignConversion = true;
        break;
    }

    // Floating-points are cast to unsigned 32-bit integers:
    case CType::FLOAT:
    case CType::DOUBLE:
    case CType::LDOUBLE:
    {
        m_data_uint32.emplace_back( createBuffer<uint32_t>( buffer, numElements ) );
        m_ioInfoInMemory.m_componentInfo.m_componentType = CType::UINT;
        m_ioInfoInMemory.m_componentInfo.m_componentSizeInBytes = 4;

        didCast = true;
        warnFloatConversion = true;
        warnSignConversion = true;
        break;
    }

    case CType::UNKNOWNCOMPONENTTYPE:
    {
        spdlog::error( "Unknown component type in image {}", m_ioInfoOnDisk.m_fileInfo.m_fileName );
        throw_debug( "Unknown component type in image" )
        }
    }

    if ( didCast )
    {
        const std::string newTypeString = itk::ImageIOBase::GetComponentTypeAsString(
                    m_ioInfoInMemory.m_componentInfo.m_componentType );

        m_ioInfoInMemory.m_componentInfo.m_componentTypeString = newTypeString;

        m_ioInfoInMemory.m_sizeInfo.m_imageSizeInBytes =
                numElements * m_ioInfoInMemory.m_componentInfo.m_componentSizeInBytes;

        spdlog::info( "Casted segmentation pixel component from type {} to {}",
                      m_ioInfoOnDisk.m_componentInfo.m_componentTypeString, newTypeString );

        if ( warnFloatConversion )
        {
            spdlog::warn( "Floating point to integer conversion: "
                          "Possible loss of precision when casting segmentation pixel component from type {} to {}",
                          m_ioInfoOnDisk.m_componentInfo.m_componentTypeString, newTypeString );
        }

        if ( warnSizeConversion )
        {
            spdlog::warn( "Size conversion: "
                          "Possible loss of information when casting segmentation pixel component from type {} to {}",
                          m_ioInfoOnDisk.m_componentInfo.m_componentTypeString, newTypeString );
        }

        if ( warnSignConversion )
        {
            spdlog::warn( "Signed to unsigned integer conversion: "
                          "Possible loss of information when casting segmentation pixel component from type {} to {}",
                          m_ioInfoOnDisk.m_componentInfo.m_componentTypeString, newTypeString );
        }
    }
}


bool Image::saveToDisk( const std::optional<std::string>& newFileName )
{
    constexpr uint32_t DIM = 3;
    constexpr bool s_isVectorImage = false;

    const std::string fileName = ( newFileName ) ? *newFileName : m_header.fileName();

    if ( m_header.numComponentsPerPixel() > 1 )
    {
        // Saving is only supported for scalar types right now
        return false;
    }

    std::array< uint32_t, DIM > dims;
    std::array< double, DIM > origin;
    std::array< double, DIM > spacing;
    std::array< std::array<double, DIM>, DIM > directions;

    for ( uint32_t i = 0; i < DIM; ++i )
    {
        dims[i] = m_header.pixelDimensions()[i];
        origin[i] = m_header.origin()[i];
        spacing[i] = m_header.spacing()[i];

        directions[i] = {
            m_header.directions()[i].x,
            m_header.directions()[i].y,
            m_header.directions()[i].z
        };
    }

    bool saved = false;

    switch ( m_header.memoryComponentType() )
    {
    case ComponentType::Int8:
    {
        auto image = makeScalarImage( dims, origin, spacing, directions, m_data_int8[0].data() );
        saved = writeImage<int8_t, DIM, s_isVectorImage>( image, fileName );
        break;
    }
    case ComponentType::UInt8:
    {
        auto image = makeScalarImage( dims, origin, spacing, directions, m_data_uint8[0].data() );
        saved = writeImage<uint8_t, DIM, s_isVectorImage>( image, fileName );
        break;
    }
    case ComponentType::Int16:
    {
        auto image = makeScalarImage( dims, origin, spacing, directions, m_data_int16[0].data() );
        saved = writeImage<int16_t, DIM, s_isVectorImage>( image, fileName );
        break;
    }
    case ComponentType::UInt16:
    {
        auto image = makeScalarImage( dims, origin, spacing, directions, m_data_uint16[0].data() );
        saved = writeImage<uint16_t, DIM, s_isVectorImage>( image, fileName );
        break;
    }
    case ComponentType::Int32:
    {
        auto image = makeScalarImage( dims, origin, spacing, directions, m_data_int32[0].data() );
        saved = writeImage<int32_t, DIM, s_isVectorImage>( image, fileName );
        break;
    }
    case ComponentType::UInt32:
    {
        auto image = makeScalarImage( dims, origin, spacing, directions, m_data_uint32[0].data() );
        saved = writeImage<uint32_t, DIM, s_isVectorImage>( image, fileName );
        break;
    }
    case ComponentType::Float32:
    {
        auto image = makeScalarImage( dims, origin, spacing, directions, m_data_float32[0].data() );
        saved = writeImage<float, DIM, s_isVectorImage>( image, fileName );
        break;
    }
    default:
    {
        saved = false;
        break;
    }
    }

    return saved;
}


const Image::ImageRepresentation& Image::imageRep() const { return m_imageRep; }
const Image::MultiComponentBufferType& Image::bufferType() const { return m_bufferType; }

const ImageHeader& Image::header() const { return m_header; }
ImageHeader& Image::header() { return m_header; }

const ImageTransformations& Image::transformations() const { return m_tx; }
ImageTransformations& Image::transformations() { return m_tx; }

const ImageSettings& Image::settings() const { return m_settings; }
ImageSettings& Image::settings() { return m_settings; }


const void* Image::bufferAsVoid( uint32_t comp ) const
{
    auto F = [this] (uint32_t i) -> const void*
    {
        switch ( m_header.memoryComponentType() )
        {
        case ComponentType::Int8:    return static_cast<const void*>( m_data_int8.at(i).data() );
        case ComponentType::UInt8:   return static_cast<const void*>( m_data_uint8.at(i).data() );
        case ComponentType::Int16:   return static_cast<const void*>( m_data_int16.at(i).data() );
        case ComponentType::UInt16:  return static_cast<const void*>( m_data_uint16.at(i).data() );
        case ComponentType::Int32:   return static_cast<const void*>( m_data_int32.at(i).data() );
        case ComponentType::UInt32:  return static_cast<const void*>( m_data_uint32.at(i).data() );
        case ComponentType::Float32: return static_cast<const void*>( m_data_float32.at(i).data() );
        default: return static_cast<const void*>( nullptr );
        }
    };

    switch ( m_bufferType )
    {
    case MultiComponentBufferType::SeparateImages:
    {
        if ( m_header.numComponentsPerPixel() <= comp )
        {
            return nullptr;
        }
        return F( comp );
    }
    case MultiComponentBufferType::InterleavedImage:
    {
        if ( 1 <= comp )
        {
            return nullptr;
        }
        return F( 0 );
    }
    }

    return nullptr;
}


void* Image::bufferAsVoid( uint32_t comp )
{
    auto F = [this] (uint32_t i) -> void*
    {
        switch ( m_header.memoryComponentType() )
        {
        case ComponentType::Int8:    return static_cast<void*>( m_data_int8.at(i).data() );
        case ComponentType::UInt8:   return static_cast<void*>( m_data_uint8.at(i).data() );
        case ComponentType::Int16:   return static_cast<void*>( m_data_int16.at(i).data() );
        case ComponentType::UInt16:  return static_cast<void*>( m_data_uint16.at(i).data() );
        case ComponentType::Int32:   return static_cast<void*>( m_data_int32.at(i).data() );
        case ComponentType::UInt32:  return static_cast<void*>( m_data_uint32.at(i).data() );
        case ComponentType::Float32: return static_cast<void*>( m_data_float32.at(i).data() );
        default: return static_cast<void*>( nullptr );
        }
    };

    switch ( m_bufferType )
    {
    case MultiComponentBufferType::SeparateImages:
    {
        if ( m_header.numComponentsPerPixel() <= comp )
        {
            return nullptr;
        }
        return F( comp );
    }
    case MultiComponentBufferType::InterleavedImage:
    {
        if ( 1 <= comp )
        {
            return nullptr;
        }
        return F( 0 );
    }
    }

    return nullptr;
}


std::optional< std::pair< size_t, size_t > >
Image::getComponentAndOffsetForBuffer( uint32_t comp, int i, int j, int k ) const
{
    // 1) component buffer to index
    // 2) offset into that buffer
    std::optional< std::pair< size_t, size_t > > ret;

    const glm::uvec3 dims = m_header.pixelDimensions();
    const auto ncomps = m_header.numComponentsPerPixel();

    if ( comp > ncomps )
    {
        // Invalid image component requested
        return std::nullopt;
    }

    if ( i < 0 || j < 0 || k < 0 )
    {
        // Invalid index
        return std::nullopt;
    }

    // Component to index the image buffer at:
    size_t c = 0;

    // Offset into the buffer:
    size_t offset = dims.x * dims.y * static_cast<size_t>(k) +
            dims.x * static_cast<size_t>(j) +
            static_cast<size_t>(i);

    switch ( m_bufferType )
    {
    case MultiComponentBufferType::SeparateImages:
    {
        c = comp;
        break;
    }
    case MultiComponentBufferType::InterleavedImage:
    {
        // There is just one buffer (0) that holds all components:
        c = 0;

        // Offset into the buffer accounts for the desired component:
        offset = ( comp + 1 ) * offset;
        break;
    }
    }

    ret = { c, offset };
    return ret;
}

std::optional<double> Image::valueAsDouble( uint32_t comp, int i, int j, int k ) const
{
    const auto compAndOffset = getComponentAndOffsetForBuffer( comp, i, j, k );
    if ( ! compAndOffset )
    {
        // Invalid input
        return std::nullopt;
    }

    const size_t c = compAndOffset->first;
    const size_t offset = compAndOffset->second;

    switch ( m_header.memoryComponentType() )
    {
    case ComponentType::Int8:    return static_cast<double>( m_data_int8.at(c)[offset] );
    case ComponentType::UInt8:   return static_cast<double>( m_data_uint8.at(c)[offset] );
    case ComponentType::Int16:   return static_cast<double>( m_data_int16.at(c)[offset] );
    case ComponentType::UInt16:  return static_cast<double>( m_data_uint16.at(c)[offset] );
    case ComponentType::Int32:   return static_cast<double>( m_data_int32.at(c)[offset] );
    case ComponentType::UInt32:  return static_cast<double>( m_data_uint32.at(c)[offset] );
    case ComponentType::Float32: return static_cast<double>( m_data_float32.at(c)[offset] );
    default: return std::nullopt;
    }

    return std::nullopt;
}

std::optional<int64_t> Image::valueAsInt64( uint32_t comp, int i, int j, int k ) const
{
    const auto compAndOffset = getComponentAndOffsetForBuffer( comp, i, j, k );
    if ( ! compAndOffset )
    {
        // Invalid input
        return std::nullopt;
    }

    const size_t c = compAndOffset->first;
    const size_t offset = compAndOffset->second;

    switch ( m_header.memoryComponentType() )
    {
    case ComponentType::Int8:    return static_cast<int64_t>( m_data_int8.at(c)[offset] );
    case ComponentType::UInt8:   return static_cast<int64_t>( m_data_uint8.at(c)[offset] );
    case ComponentType::Int16:   return static_cast<int64_t>( m_data_int16.at(c)[offset] );
    case ComponentType::UInt16:  return static_cast<int64_t>( m_data_uint16.at(c)[offset] );
    case ComponentType::Int32:   return static_cast<int64_t>( m_data_int32.at(c)[offset] );
    case ComponentType::UInt32:  return static_cast<int64_t>( m_data_uint32.at(c)[offset] );
    case ComponentType::Float32: return static_cast<int64_t>( m_data_float32.at(c)[offset] );
    default: return std::nullopt;
    }

    return std::nullopt;
}


bool Image::setValue( uint32_t component, int i, int j, int k, int64_t value )
{
    const auto compAndOffset = getComponentAndOffsetForBuffer( component, i, j, k );
    if ( ! compAndOffset )
    {
        // Invalid input
        return false;
    }

    const size_t c = compAndOffset->first;
    const size_t offset = compAndOffset->second;

    switch ( m_header.memoryComponentType() )
    {
    case ComponentType::Int8: m_data_int8.at(c)[offset] = static_cast<int8_t>( value ); return true;
    case ComponentType::UInt8: m_data_uint8.at(c)[offset] = static_cast<uint8_t>( value ); return true;
    case ComponentType::Int16: m_data_int16.at(c)[offset] = static_cast<int16_t>( value ); return true;
    case ComponentType::UInt16: m_data_uint16.at(c)[offset] = static_cast<uint16_t>( value ); return true;
    case ComponentType::Int32: m_data_int32.at(c)[offset] = static_cast<int32_t>( value ); return true;
    case ComponentType::UInt32: m_data_uint32.at(c)[offset] = static_cast<uint32_t>( value ); return true;
    case ComponentType::Float32: m_data_float32.at(c)[offset] = static_cast<float>( value ); return true;
    default: return false;
    }

    return false;
}

bool Image::setValue( uint32_t component, int i, int j, int k, double value )
{
    const auto compAndOffset = getComponentAndOffsetForBuffer( component, i, j, k );
    if ( ! compAndOffset )
    {
        // Invalid input
        return false;
    }

    const size_t c = compAndOffset->first;
    const size_t offset = compAndOffset->second;

    switch ( m_header.memoryComponentType() )
    {
    case ComponentType::Int8: m_data_int8.at(c)[offset] = static_cast<int8_t>( value ); return true;
    case ComponentType::UInt8: m_data_uint8.at(c)[offset] = static_cast<uint8_t>( value ); return true;
    case ComponentType::Int16: m_data_int16.at(c)[offset] = static_cast<int16_t>( value ); return true;
    case ComponentType::UInt16: m_data_uint16.at(c)[offset] = static_cast<uint16_t>( value ); return true;
    case ComponentType::Int32: m_data_int32.at(c)[offset] = static_cast<int32_t>( value ); return true;
    case ComponentType::UInt32: m_data_uint32.at(c)[offset] = static_cast<uint32_t>( value ); return true;
    case ComponentType::Float32: m_data_float32.at(c)[offset] = static_cast<float>( value ); return true;
    default: return false;
    }

    return false;
}


std::ostream& Image::metaData( std::ostream& os ) const
{
    for ( const auto& p : m_ioInfoInMemory.m_metaData )
    {
        os << p.first << ": ";
        std::visit( [&os] ( const auto& e ) { os << e; }, p.second );
        os << std::endl;
    }
    return os;
}

