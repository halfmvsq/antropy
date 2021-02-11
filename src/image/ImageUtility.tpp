#ifndef IMAGE_UTILITY_TPP
#define IMAGE_UTILITY_TPP

#include "common/Exception.hpp"
#include "common/Types.h"

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkImageToHistogramFilter.h>
#include <itkImportImageFilter.h>
#include <itkStatisticsImageFilter.h>
#include <itkVectorImage.h>

#include <spdlog/spdlog.h>

#include <array>
#include <string>
#include <utility>
#include <vector>


template< typename T, typename U, uint32_t NDim >
ComponentStats<U> computeImageStatistics( const typename itk::Image<T, NDim>::Pointer image )
{
    using ImageType = itk::Image<T, NDim>;
    using StatisticsFilterType = itk::StatisticsImageFilter<ImageType>;
    using ImageToHistogramFilterType = itk::Statistics::ImageToHistogramFilter<ImageType>;
    using HistogramType = typename ImageToHistogramFilterType::HistogramType;

    auto statsFilter = StatisticsFilterType::New();
    statsFilter->SetInput( image );
    statsFilter->Update();

    static constexpr size_t sk_numComponents = 1;
    static constexpr size_t sk_numBins = 101;

    typename HistogramType::SizeType size( sk_numComponents );
    size.Fill( sk_numBins );

    auto histogramFilter = ImageToHistogramFilterType::New();

    // Note: this is another way of setting the min and max histogram bounds:
    // using MeasType = typename ImageToHistogramFilterType::HistogramType::MeasurementVectorType;
    // MeasType lowerBound( sk_numBins );
    // MeasType upperBound( sk_numBins );
    // lowerBound.Fill( statsImageFilter->GetMinimum() );
    // lowerBound.Fill( statsImageFilter->GetMaximum() );
    // histogramFilter->SetHistogramBinMinimum( lowerBound );
    // histogramFilter->SetHistogramBinMaximum( upperBound );

    histogramFilter->SetInput( image );
    histogramFilter->SetAutoMinimumMaximum( true );
    histogramFilter->SetHistogramSize( size );
    histogramFilter->Update();

    ComponentStats<U> stats;
    stats.m_minimum = static_cast<U>( statsFilter->GetMinimum() );
    stats.m_maximum = static_cast<U>( statsFilter->GetMaximum() );
    stats.m_mean = static_cast<U>( statsFilter->GetMean() );
    stats.m_stdDeviation = static_cast<U>( statsFilter->GetSigma() );
    stats.m_variance = static_cast<U>( statsFilter->GetVariance() );
    stats.m_sum = static_cast<U>( statsFilter->GetSum() );

    HistogramType* histogram = histogramFilter->GetOutput();
    if ( ! histogram )
    {
        throw_debug( "Unexpected error computing image histogram" )
    }

    auto itr = histogram->Begin();
    const auto end = histogram->End();

    stats.m_histogram.reserve( sk_numBins );

    while ( itr != end )
    {
        stats.m_histogram.push_back( static_cast<U>( itr.GetFrequency() ) );
        ++itr;
    }

    for ( size_t i = 0; i < sk_numBins; ++i )
    {
        stats.m_quantiles[i] = histogram->Quantile( 0, i / 100.0 );
    }

    return stats;
}


template< typename T, typename U, uint32_t NDim >
ComponentStats<U> createDefaultImageStatistics( T defaultValue, size_t numPixels )
{
    static constexpr size_t sk_numBins = 101;

    ComponentStats<U> stats;
    stats.m_minimum = static_cast<U>( defaultValue );
    stats.m_maximum = static_cast<U>( defaultValue );
    stats.m_mean = static_cast<U>( defaultValue );
    stats.m_stdDeviation = static_cast<U>( 0 );
    stats.m_variance = static_cast<U>( 0 );
    stats.m_sum = static_cast<U>( defaultValue * numPixels );
    stats.m_histogram.resize( sk_numBins, static_cast<U>( 1.0 / sk_numBins ) );

    for ( size_t i = 0; i < sk_numBins; ++i )
    {
        stats.m_quantiles[i] = defaultValue;
    }

    return stats;
}


template< class ComponentType, uint32_t NDim >
typename ::itk::Image< ComponentType, NDim >::Pointer
downcastImageBaseToImage( const typename ::itk::ImageBase< NDim >::Pointer& imageBase )
{
    typename ::itk::Image<ComponentType, NDim>::Pointer child =
            dynamic_cast< ::itk::Image<ComponentType, NDim>* >( imageBase.GetPointer() );

    if ( ! child.GetPointer() || child.IsNull() )
    {
        spdlog::error( "Unable to downcast ImageBase to Image with component type {}",
                       typeid( ComponentType ).name() );
        return nullptr;
    }

    return child;
}


template< class ComponentType, uint32_t NDim >
typename ::itk::VectorImage< ComponentType, NDim >::Pointer
downcastImageBaseToVectorImage( const typename ::itk::ImageBase< NDim >::Pointer& imageBase )
{
    typename ::itk::VectorImage< ComponentType, NDim >::Pointer child =
            dynamic_cast< ::itk::VectorImage< ComponentType, NDim > * >( imageBase.GetPointer() );

    if ( ! child.GetPointer() || child.IsNull() )
    {
        spdlog::error( "Unable to downcast ImageBase to VectorImage with component type {}",
                       typeid( ComponentType ).name() );
        return nullptr;
    }

    return child;
}


template< uint32_t NDim >
bool isVectorImage( const typename ::itk::ImageBase<NDim>::Pointer& imageBase )
{
    return ( imageBase && imageBase.IsNotNull() )
            ? ( imageBase->GetNumberOfComponentsPerPixel() > 1 )
            : false;
}


/**
 * @note Data of multi-component (vector) images gets duplicated by this function:
 * one copy pointed to by base class' \c m_imageBasePtr;
 * the other copy pointed to by this class' \c m_splitImagePtrs
 */
template< class ComponentType, uint32_t NDim >
std::vector< typename itk::Image<ComponentType, NDim>::Pointer >
splitImageIntoComponents( const typename ::itk::ImageBase<NDim>::Pointer& imageBase )
{
    using ImageType = itk::Image<ComponentType, NDim>;
    using VectorImageType = itk::VectorImage<ComponentType, NDim>;

    std::vector< typename ImageType::Pointer > splitImages;

    if ( isVectorImage<NDim>( imageBase ) )
    {
        const typename VectorImageType::Pointer vectorImage =
                downcastImageBaseToVectorImage<ComponentType, NDim>( imageBase );

        if ( ! vectorImage || vectorImage.IsNull() )
        {
            spdlog::error( "Error casting ImageBase to vector image" );
            return splitImages;
        }

        const size_t numPixels = vectorImage->GetBufferedRegion().GetNumberOfPixels();
        const uint32_t numComponents = vectorImage->GetVectorLength();

        splitImages.resize( numComponents );

        for ( uint32_t i = 0; i < numComponents; ++i )
        {
            splitImages[i] = ImageType::New();
            splitImages[i]->CopyInformation( vectorImage );
            splitImages[i]->SetRegions( vectorImage->GetBufferedRegion() );
            splitImages[i]->Allocate();

            ComponentType* source = vectorImage->GetBufferPointer() + i;
            ComponentType* dest = splitImages[i]->GetBufferPointer();

            const ComponentType* end = dest + numPixels;

            // Copy pixels from component i of vectorImage (the source),
            // which are offset from each other by a stride of numComponents,
            // into pixels of the i'th split image (the destination)
            while ( dest < end )
            {
                *dest = *source;
                ++dest;
                source += numComponents;
            }
        }
    }
    else
    {
        // Image has only one component
        const typename ImageType::Pointer image =
                downcastImageBaseToImage<ComponentType, NDim>( imageBase );

        if ( ! image || image.IsNull() )
        {
            spdlog::error( "Error casting ImageBase to image" );
            return splitImages;
        }

        splitImages.resize( 1 );
        splitImages[0] = image;
    }

    return splitImages;
}


template< class T >
typename itk::Image<T, 3>::Pointer
makeScalarImage(
        const std::array<uint32_t, 3>& imageDims,
        const std::array<double, 3>& imageOrigin,
        const std::array<double, 3>& imageSpacing,
        const std::array< std::array<double, 3>, 3 >&  imageDirection,
        const T* imageData )
{
    using ImportFilterType = itk::ImportImageFilter<T, 3>;

    if ( ! imageData )
    {
        spdlog::error( "Null data array provided when creating new scalar image" );
        return nullptr;
    }

    constexpr bool filterOwnsBuffer = false;

    typename ImportFilterType::IndexType start;
    typename ImportFilterType::SizeType size;
    typename ImportFilterType::DirectionType direction;

    itk::SpacePrecisionType origin[3];
    itk::SpacePrecisionType spacing[3];

    for ( uint32_t i = 0; i < 3; ++i )
    {
        start[i] = 0.0;
        size[i] = imageDims[i];
        origin[i] = imageOrigin[i];
        spacing[i] = imageSpacing[i];

        for ( uint32_t j = 0; j < 3; ++j )
        {
            direction[i][j] = imageDirection[i][j];
        }
    }

    const size_t numPixels = size[0] * size[1] * size[2];

    if ( 0 == numPixels )
    {
        spdlog::error( "Cannot create new scalar image with size zero" );
        return nullptr;
    }

    typename  ImportFilterType::RegionType region;
    region.SetIndex( start );
    region.SetSize( size );

    try
    {
        typename ImportFilterType::Pointer importer = ImportFilterType::New();
        importer->SetRegion( region );
        importer->SetOrigin( origin );
        importer->SetSpacing( spacing );
        importer->SetDirection( direction );
        importer->SetImportPointer( const_cast<T*>( imageData ), numPixels, filterOwnsBuffer );
        importer->Update();

        return importer->GetOutput();
    }
    catch ( const std::exception& e )
    {
        spdlog::error( "Exception creating new ITK scalar image from data array: {}", e.what() );
        return nullptr;
    }
}


template< class ComponentType, uint32_t NDim, bool PixelIsVector >
typename itk::ImageBase<NDim>::Pointer
readImage( const std::string& fileName )
{
    using ImageType = typename std::conditional< PixelIsVector,
        itk::VectorImage<ComponentType, NDim>,
        itk::Image<ComponentType, NDim> >::type;

    using ReaderType = itk::ImageFileReader<ImageType>;

    try
    {
        auto reader = ReaderType::New();

        if ( ! reader )
        {
            spdlog::error( "Null ITK ImageFileReader on reading image from {}", fileName );
            return nullptr;
        }

        reader->SetFileName( fileName.c_str() );
        reader->Update();
        return static_cast<itk::ImageBase<3>::Pointer>( reader->GetOutput() );
    }
    catch ( const std::exception& e )
    {
        spdlog::error( "Exception reading image from {}: {}", fileName, e.what() );
        return nullptr;
    }
}


template< class T, uint32_t NDim, bool PixelIsVector >
bool writeImage(
        typename itk::Image<T, NDim>::Pointer image,
        const std::string& fileName )
{
    using ImageType = typename std::conditional< PixelIsVector,
        itk::VectorImage<T, NDim>,
        itk::Image<T, NDim> >::type;

    using WriterType = itk::ImageFileWriter<ImageType>;

    if ( ! image )
    {
        spdlog::error( "Null image cannot be written to {}", fileName );
        return false;
    }

    try
    {
        auto writer = WriterType::New();

        if ( ! writer )
        {
            spdlog::error( "Null ITK ImageFileWriter on writing image to {}", fileName );
            return false;
        }

        writer->SetFileName( fileName.c_str() );
        writer->SetInput( image );
        writer->Update();
        return true;
    }
    catch ( const std::exception& e )
    {
        spdlog::error( "Exception writing image to {}: {}", fileName, e.what() );
        return false;
    }
}


template< class ComponentType >
std::vector<ComponentType> createBuffer( const float* buffer, size_t numElements )
{
    std::vector<ComponentType> data;
    data.resize( numElements );

    for ( size_t i = 0; i < numElements; ++i )
    {
        data[i] = static_cast<ComponentType>( buffer[i] );
    }

    return data;
}

#endif // IMAGE_UTILITY_TPP
