#include "rendering/TextureSetup.h"

#include "logic/app/Data.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>


std::unordered_map< uuids::uuid, std::vector<GLTexture> >
createImageTextures( const AppData& appData )
{
    static constexpr GLint sk_mipmapLevel = 0; // Load image data into first mipmap level
    static constexpr GLint sk_alignment = 1; // Pixel pack/unpack alignment is 1 byte
    static const tex::WrapMode sk_wrapModeClampToEdge = tex::WrapMode::ClampToEdge;
//    static const tex::WrapMode sk_wrapModeClampToBorder = tex::WrapMode::ClampToBorder;
    static const glm::vec4 sk_border{ 0.0f, 0.0f, 0.0f, 0.0f }; // Black border

    // Map from image UID to vector of textures for the image components.
    // Images with interleaved components will have one component texture.
    std::unordered_map< uuids::uuid, std::vector<GLTexture> > imageTextures;

    if ( 0 == appData.numImages() )
    {
        spdlog::warn( "No images are loaded for which to create textures" );
        return imageTextures;
    }

    spdlog::debug( "Begin creating 3D image textures" );

    GLTexture::PixelStoreSettings pixelPackSettings;
    pixelPackSettings.m_alignment = sk_alignment;
    GLTexture::PixelStoreSettings pixelUnpackSettings = pixelPackSettings;

    for ( const auto& imageUid : appData.imageUidsOrdered() )
    {
        spdlog::debug( "Begin creating texture(s) for components of image {}", imageUid );

        const auto* image = appData.image( imageUid );
        if ( ! image )
        {
            spdlog::warn( "Image {} is invalid", imageUid );
            continue;
        }

        const ComponentType compType = image->header().memoryComponentType();
        const uint32_t numComp = image->header().numComponentsPerPixel();

        std::vector<GLTexture> componentTextures;

        switch ( image->bufferType() )
        {
        case Image::MultiComponentBufferType::InterleavedImage:
        {
            spdlog::debug( "Image {} has {} interleaved components, so one texture will be created.",
                           imageUid, numComp );

            // For images with interleaved components, all components are at index 0
            constexpr uint32_t k_comp0 = 0;

            if ( 4 < numComp )
            {
                spdlog::warn( "Image {} has {} interleaved components, exceeding the maximum "
                              "of 4 allowed per texture; it will not be loaded as a texture",
                              imageUid, numComp );
                continue;
            }

            tex::MinificationFilter minFilter;
            tex::MagnificationFilter maxFilter;

            switch ( image->settings().interpolationMode( k_comp0 ) )
            {
            case InterpolationMode::NearestNeighbor:
            {
                minFilter = tex::MinificationFilter::Nearest;
                maxFilter = tex::MagnificationFilter::Nearest;
                break;
            }
            case InterpolationMode::Linear:
            {
                minFilter = tex::MinificationFilter::Linear;
                maxFilter = tex::MagnificationFilter::Linear;
                break;
            }
            }

            // The texture pixel format types depend on the number of components
            tex::SizedInternalFormat sizedInternalNormalizedFormat;
            tex::BufferPixelFormat bufferPixelNormalizedFormat;

            switch ( numComp )
            {
            case 1:
            {
                // Red:
                sizedInternalNormalizedFormat = GLTexture::getSizedInternalNormalizedRedFormat( compType );
                bufferPixelNormalizedFormat = GLTexture::getBufferPixelNormalizedRedFormat( compType );
                break;
            }
            case 2:
            {
                // Red, green:
                sizedInternalNormalizedFormat = GLTexture::getSizedInternalNormalizedRGFormat( compType );
                bufferPixelNormalizedFormat = GLTexture::getBufferPixelNormalizedRGFormat( compType );
                break;
            }
            case 3:
            {
                // Red, green, blue:
                sizedInternalNormalizedFormat = GLTexture::getSizedInternalNormalizedRGBFormat( compType );
                bufferPixelNormalizedFormat = GLTexture::getBufferPixelNormalizedRGBFormat( compType );
                break;
            }
            case 4:
            {
                // Red, green, blue, alpha:
                sizedInternalNormalizedFormat = GLTexture::getSizedInternalNormalizedRGBAFormat( compType );
                bufferPixelNormalizedFormat = GLTexture::getBufferPixelNormalizedRGBAFormat( compType );
                break;
            }
            default:
            {
                spdlog::warn( "Image {} has invalid number of components ({}); "
                              "it will not be loaded as a texture", imageUid, numComp );
                continue;
            }
            }

            GLTexture& T = componentTextures.emplace_back(
                        tex::Target::Texture3D,
                        GLTexture::MultisampleSettings(),
                        pixelPackSettings, pixelUnpackSettings );

            T.generate();
            T.setMinificationFilter( minFilter );
            T.setMagnificationFilter( maxFilter );
            T.setBorderColor( sk_border );
            T.setWrapMode( sk_wrapModeClampToEdge );
            T.setAutoGenerateMipmaps( true );
            T.setSize( image->header().pixelDimensions() );

            T.setData( sk_mipmapLevel,
                       sizedInternalNormalizedFormat,
                       bufferPixelNormalizedFormat,
                       GLTexture::getBufferPixelDataType( compType ),
                       image->bufferAsVoid( k_comp0 ) );

            spdlog::debug( "Done creating the texture for all interleaved components of image {}", imageUid );
            break;
        }
        case Image::MultiComponentBufferType::SeparateImages:
        {
            spdlog::debug( "Image {} has {} separate components, so {} textures will be created.",
                           imageUid, numComp, numComp );

            for ( uint32_t comp = 0; comp < numComp; ++comp )
            {
                tex::MinificationFilter minFilter;
                tex::MagnificationFilter maxFilter;

                switch ( image->settings().interpolationMode( comp ) )
                {
                case InterpolationMode::NearestNeighbor:
                {
                    minFilter = tex::MinificationFilter::Nearest;
                    maxFilter = tex::MagnificationFilter::Nearest;
                    break;
                }
                case InterpolationMode::Linear:
                {
                    minFilter = tex::MinificationFilter::Linear;
                    maxFilter = tex::MagnificationFilter::Linear;
                    break;
                }
                }

                // Use Red format for each component texture:
                const tex::SizedInternalFormat sizedInternalNormalizedFormat =
                        GLTexture::getSizedInternalNormalizedRedFormat( compType );

                const tex::BufferPixelFormat bufferPixelNormalizedFormat =
                        GLTexture::getBufferPixelNormalizedRedFormat( compType );

                GLTexture& T = componentTextures.emplace_back(
                            tex::Target::Texture3D,
                            GLTexture::MultisampleSettings(),
                            pixelPackSettings, pixelUnpackSettings );

                T.generate();
                T.setMinificationFilter( minFilter );
                T.setMagnificationFilter( maxFilter );
                T.setBorderColor( sk_border );
                T.setWrapMode( sk_wrapModeClampToEdge );
                T.setAutoGenerateMipmaps( true );
                T.setSize( image->header().pixelDimensions() );

                T.setData( sk_mipmapLevel,
                           sizedInternalNormalizedFormat,
                           bufferPixelNormalizedFormat,
                           GLTexture::getBufferPixelDataType( compType ),
                           image->bufferAsVoid( comp ) );
            }

            spdlog::debug( "Done creating {} image component textures", componentTextures.size() );
            break;
        }
        } // end switch ( image->bufferType() )

        imageTextures.emplace( imageUid, std::move( componentTextures ) );

        spdlog::debug( "Done creating texture(s) for image {} ('{}')",
                       imageUid, image->settings().displayName() );
    } // end for ( const auto& imageUid : appData.imageUidsOrdered() )

    spdlog::debug( "Done creating textures for {} image(s)", imageTextures.size() );
    return imageTextures;
}


std::unordered_map< uuids::uuid, GLTexture >
createSegTextures( const AppData& appData )
{
    // Load the first pixel component of the segmentation image.
    // (Segmentations should have only one component.)
    constexpr uint32_t k_comp0 = 0;

    constexpr GLint k_mipmapLevel = 0; // Load seg data into first mipmap level
    constexpr GLint k_alignment = 1; // Pixel pack/unpack alignment is 1 byte

    static const tex::WrapMode sk_wrapMode = tex::WrapMode::ClampToBorder;
    static const glm::vec4 sk_border{ 0.0f, 0.0f, 0.0f, 0.0f }; // Black border

    // Nearest-neighbor interpolation is used for segmentation textures:
    static const tex::MinificationFilter sk_minFilter = tex::MinificationFilter::Nearest;
    static const tex::MagnificationFilter sk_maxFilter = tex::MagnificationFilter::Nearest;

    std::unordered_map< uuids::uuid, GLTexture > textures;

    if ( 0 == appData.numSegs() )
    {
        spdlog::info( "No image segmentations loaded for which to create textures" );
        return textures;
    }

    spdlog::debug( "Begin creating 3D segmentation textures" );

    GLTexture::PixelStoreSettings pixelPackSettings;
    pixelPackSettings.m_alignment = k_alignment;
    GLTexture::PixelStoreSettings pixelUnpackSettings = pixelPackSettings;

    // Loop through images in order of index
    for ( const auto& segUid : appData.segUidsOrdered() )
    {
        const auto* seg = appData.seg( segUid );
        if ( ! seg )
        {
            spdlog::warn( "Segmentation {} is invalid", segUid );
            continue;
        }

        const ComponentType compType = seg->header().memoryComponentType();

        auto it = textures.try_emplace(
                    segUid,
                    tex::Target::Texture3D,
                    GLTexture::MultisampleSettings(),
                    pixelPackSettings, pixelUnpackSettings );

        if ( ! it.second ) continue;
        GLTexture& T = it.first->second;

        T.generate();
        T.setMinificationFilter( sk_minFilter );
        T.setMagnificationFilter( sk_maxFilter );
        T.setBorderColor( sk_border );
        T.setWrapMode( sk_wrapMode );
        T.setAutoGenerateMipmaps( true );
        T.setSize( seg->header().pixelDimensions() );

        T.setData( k_mipmapLevel,
                   GLTexture::getSizedInternalRedFormat( compType ),
                   GLTexture::getBufferPixelRedFormat( compType ),
                   GLTexture::getBufferPixelDataType( compType ),
                   seg->bufferAsVoid( k_comp0 ) );

        spdlog::debug( "Created texture for segmentation {} ('{}')",
                       segUid, seg->settings().displayName() );
    }

    spdlog::debug( "Done creating {} segmentation textures", textures.size() );
    return textures;
}


std::unordered_map< uuids::uuid, GLTexture >
createImageColorMapTextures( const AppData& appData )
{
    std::unordered_map< uuids::uuid, GLTexture > textures;

    if ( 0 == appData.numImageColorMaps() )
    {
        spdlog::warn( "No image color maps loaded for which to create textures" );
        return textures;
    }

    spdlog::debug( "Begin creating image color map textures" );

    // Loop through color maps in order of index
    for ( size_t i = 0; i < appData.numImageColorMaps(); ++i )
    {
        const auto cmapUid = appData.imageColorMapUid( i );
        if ( ! cmapUid )
        {
            spdlog::warn( "Image color map index {} is invalid", i );
            continue;
        }

        const auto* map = appData.imageColorMap( *cmapUid );
        if ( ! map )
        {
            spdlog::warn( "Image color map {} is invalid", *cmapUid );
            continue;
        }

        auto it = textures.emplace( *cmapUid, tex::Target::Texture1D );
        if ( ! it.second ) continue;

        GLTexture& T = it.first->second;

        T.generate();
        T.setSize( glm::uvec3{ map->numColors(), 1, 1 } );

        T.setData( 0, ImageColorMap::textureFormat_RGBA_F32(),
                   tex::BufferPixelFormat::RGBA,
                   tex::BufferPixelDataType::Float32,
                   map->data_RGBA_F32() );

        // We should never sample outside the texture coordinate range [0.0, 1.0], anyway
        T.setWrapMode( tex::WrapMode::ClampToEdge );

        // All sampling of color maps uses linearly interpolation
        T.setAutoGenerateMipmaps( false );
        T.setMinificationFilter( tex::MinificationFilter::Linear );
        T.setMagnificationFilter( tex::MagnificationFilter::Linear );

        spdlog::trace( "Generated texture for image color map {}", *cmapUid );
    }

    spdlog::debug( "Done creating {} image color map textures", textures.size() );
    return textures;
}


std::unordered_map< uuids::uuid, GLTexture >
createLabelColorTableTextures( const AppData& appData )
{
    static const glm::vec4 sk_border{ 0.0f, 0.0f, 0.0f, 0.0f };

    std::unordered_map< uuids::uuid, GLTexture > textures;

    if ( 0 == appData.numLabelTables() )
    {
        spdlog::warn( "No parcellation label color tables loaded for which to create textures" );
        return textures;
    }

    spdlog::debug( "Begin creating 1D label color map textures" );

    // Loop through label tables in order of index
    for ( size_t i = 0; i < appData.numLabelTables(); ++i )
    {
        const auto tableUid = appData.labelTableUid( i );
        if ( ! tableUid )
        {
            spdlog::warn( "Label table index {} is invalid", i );
            continue;
        }

        const auto* table = appData.labelTable( *tableUid );
        if ( ! table )
        {
            spdlog::warn( "Label table {} is invalid", *tableUid );
            continue;
        }

        auto it = textures.emplace( *tableUid, tex::Target::Texture1D );
        if ( ! it.second ) continue;

        GLTexture& T = it.first->second;

        T.generate();
        T.setSize( glm::uvec3{ table->numLabels(), 1, 1 } );

        T.setData( 0, ImageColorMap::textureFormat_RGBA_F32(),
                   tex::BufferPixelFormat::RGBA,
                   tex::BufferPixelDataType::Float32,
                   table->colorData_RGBA_premult_F32() );

        // We should never sample outside the texture coordinate range [0.0, 1.0], anyway
        T.setBorderColor( sk_border );
        T.setWrapMode( tex::WrapMode::ClampToBorder );

        // All sampling of color maps uses linearly interpolation
        T.setAutoGenerateMipmaps( false );
        T.setMinificationFilter( tex::MinificationFilter::Nearest );
        T.setMagnificationFilter( tex::MagnificationFilter::Nearest );

        spdlog::debug( "Generated texture for label color table {}", *tableUid );
    }

    spdlog::debug( "Done creating {} label color map textures", textures.size() );
    return textures;
}
