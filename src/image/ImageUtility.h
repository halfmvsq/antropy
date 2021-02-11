#ifndef IMAGE_UTILITY_H
#define IMAGE_UTILITY_H

#include "common/Types.h"
#include "image/ImageIoInfo.h"

#include <itkCommonEnums.h>
#include <itkImage.h>
#include <itkImageIOBase.h>

#include <string>
#include <utility>


std::string getFileName( const std::string& filePath, bool withExtension = false );

PixelType fromItkPixelType( const ::itk::IOPixelEnum& pixelType );

ComponentType fromItkComponentType( const ::itk::IOComponentEnum& componentType );

std::pair< itk::CommonEnums::IOComponent, std::string >
sniffComponentType( const char* fileName );

typename itk::ImageIOBase::Pointer
createStandardImageIo( const char* fileName );

/**
 * @brief Get the range of values that can be held in components of a given type.
 * Only for components supported by Antropy.
 *
 * @param componentType
 * @return
 */
std::pair< double, double > componentRange( const ComponentType& componentType );

#endif // IMAGE_UTILITY_H
