#include "AntropyApp.h"
#include "defines.h"

#include "common/DataHelper.h"
#include "common/Exception.hpp"
#include "common/MathFuncs.h"

#include "image/ImageUtility.h"

#include "logic/annotation/LandmarkGroup.h"
#include "logic/interaction/events/MouseEvent.h"
//#include "logic/ipc/IPCMessage.h"

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/color_space.hpp>
#include <glm/gtx/string_cast.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>

// Without undefining min and max, there are some errors compiling in Visual Studio
#undef min
#undef max


namespace
{

/// @todo put this function in another file
serialize::AntropyProject createProjectFromInputParams( const InputParams& params )
{
    serialize::AntropyProject project;

    if ( ! params.imageFiles.empty() )
    {
        // If images were provided, use them.

        // The reference image is at index 0:
        project.m_referenceImage.m_imageFileName = params.imageFiles[0].first;

        // Add reference segmentation, if provided:
        if ( params.imageFiles[0].second )
        {
            serialize::Segmentation seg;
            seg.m_segFileName = *( params.imageFiles[0].second );

            project.m_referenceImage.m_segmentations.emplace_back( std::move( seg ) );
        }

        // Additional images:
        for ( size_t i = 1; i < params.imageFiles.size(); ++i )
        {
            serialize::Image image;
            image.m_imageFileName = params.imageFiles[i].first;

            // Add image segmentation:
            if ( params.imageFiles[i].second )
            {
                serialize::Segmentation seg;
                seg.m_segFileName = *( params.imageFiles[i].second );

                image.m_segmentations.emplace_back( std::move( seg ) );
            }

            project.m_additionalImages.emplace_back( std::move( image ) );
        }
    }
    else if ( params.projectFile )
    {
        // A project file was provided, so open it:
        const bool valid = serialize::open( project, *params.projectFile );
        if ( ! valid )
        {
            spdlog::critical( "No images were provided in the project {}", *params.projectFile );
            exit( EXIT_FAILURE );
        }
    }
    else
    {
        spdlog::critical( "No project file or image arguments were provided" );
        exit( EXIT_FAILURE );
    }

    return project;
}

} // anonymous


AntropyApp::AntropyApp()
    :
      m_imagesReady( false ),
      m_imageLoadFailed( false ),

      // GLFW creates the OpenGL contex
      m_glfw( this, GL_VERSION_MAJOR, GL_VERSION_MINOR ),

      m_data(), // Requires OpenGL context
      m_imgui( m_glfw.window(), m_data ), // Requires OpenGL context
      m_rendering( m_data ), // Requires OpenGL context

      m_callbackHandler( m_data, m_rendering )

//      m_IPCHandler()
{   
    spdlog::debug( "Begin constructing application" );

    m_glfw.setCallbacks(
                [this](){ m_rendering.render(); },
                [this](){ m_imgui.render(); } );

    m_imgui.setCallbacks(
            [this] ( const uuids::uuid& viewUid )
            {
                m_callbackHandler.recenterView(
                            ImageSelection::AllLoadedImages, viewUid );
            },

            [this] ( bool recenterCrosshairs, bool recenterOnCurrentCrosshairsPosition )
            {
                m_callbackHandler.recenterViews(
                            ImageSelection::AllLoadedImages,
                            recenterCrosshairs,
                            recenterOnCurrentCrosshairsPosition );
            },

            [this] () { return m_callbackHandler.showOverlays(); },

            [this] ( bool show ) { m_callbackHandler.setShowOverlays( show ); },

            [this] ( const uuids::uuid& imageUid ) {
                m_rendering.updateImageUniforms( imageUid );
            },

            [this] ( const uuids::uuid& imageUid ) {
                m_rendering.updateImageInterpolation( imageUid );
            },

            [this] ( size_t labelColorTableIndex )
            {
                m_rendering.updateLabelColorTableTexture( labelColorTableIndex );
            },

            [this] ()
            {
                m_rendering.updateMetricUniforms();
            },

            [this] ()
            {
                return m_data.settings().worldCrosshairs().worldOrigin();
            },

            [this] ( size_t imageIndex ) -> std::optional<glm::vec3>
            {
                const auto imageUid = m_data.imageUid( imageIndex );
                const Image* image = imageUid ? m_data.image( *imageUid ) : nullptr;
                if ( ! image ) return std::nullopt;

                const auto& tx = image->transformations();
                const glm::vec4 subjectPos = tx.subject_T_worldDef() *
                        glm::vec4{ m_data.settings().worldCrosshairs().worldOrigin(), 1 };
                return glm::vec3{ subjectPos / subjectPos.w };
            },

            [this] ( size_t imageIndex )
            {
                return data::getImageVoxelCoordsAtCrosshairs( m_data, imageIndex );
            },

            [this] ( size_t imageIndex ) -> std::optional<double>
            {
                const auto imageUid = m_data.imageUid( imageIndex );
                const Image* image = imageUid ? m_data.image( *imageUid ) : nullptr;
                if ( ! image ) return std::nullopt;

                if ( const std::optional<glm::ivec3> coords =
                     data::getImageVoxelCoordsAtCrosshairs( m_data, imageIndex ) )
                {
                    const uint32_t activeComp = image->settings().activeComponent();
                    return image->valueAsDouble( activeComp, coords->x, coords->y, coords->z );
                }

                return std::nullopt;
            },

            // This gets the active segmentation value:
            [this] ( size_t imageIndex ) -> std::optional<int64_t>
            {
                const auto imageUid = m_data.imageUid( imageIndex );
                if ( ! imageUid ) return std::nullopt;

                const auto segUid = m_data.imageToActiveSegUid( *imageUid );
                const Image* seg = segUid ? m_data.seg( *segUid ) : nullptr;
                if ( ! seg ) return std::nullopt;

                if ( const std::optional<glm::ivec3> coords =
                     data::getImageVoxelCoordsAtCrosshairs( m_data, imageIndex ) )
                {
                    const uint32_t activeComp = seg->settings().activeComponent();
                    return seg->valueAsInt64( activeComp, coords->x, coords->y, coords->z );
                }

                return std::nullopt;
            },

            [this] ( const uuids::uuid& matchingImageUid, const std::string& segDisplayName )
            {
                return createBlankSegWithColorTable( matchingImageUid, segDisplayName );
            },

            [this] ( const uuids::uuid& segUid ) -> bool
            {
                return m_callbackHandler.clearSegVoxels( segUid );
            },

            [this] ( const uuids::uuid& segUid ) -> bool
            {
                bool success = false;
                success |= m_data.removeSeg( segUid );
                success |= m_rendering.removeSegTexture( segUid );
                return success;
            },

            [this] ( const uuids::uuid& imageUid, const uuids::uuid& seedSegUid, const uuids::uuid& resultSegUid ) -> bool
            {
                return m_callbackHandler.executeGridCutSegmentation( imageUid, seedSegUid, resultSegUid );
            },

            [this] ( const uuids::uuid& imageUid, bool locked ) -> bool
            {
                return m_callbackHandler.setLockManualImageTransformation( imageUid, locked );
            }
    );

    // Initialize the IPC handler
//    m_IPCHandler.Attach( IPCHandler::GetUserPreferencesFileName().c_str(),
//                         (short) IPCMessage::VERSION, sizeof( IPCMessage ) );

    spdlog::debug( "Done constructing application" );
}


AntropyApp::~AntropyApp()
{
    m_futureLoadProject.wait();

//    if ( m_IPCHandler.IsAttached() )
//    {
//        m_IPCHandler.Close();
//    }
}


void AntropyApp::init()
{
    spdlog::debug( "Begin initializing application" );

    m_rendering.init();
    m_glfw.init(); // Trigger initial windowing callbacks

    spdlog::debug( "Done initializing application" );
}


void AntropyApp::run()
{
    spdlog::debug( "Begin application run loop" );

    auto onImagesReady = [this] ()
    {
        const Image* refImg = m_data.refImage();

        if ( ! refImg )
        {
            // At a minimum, we need a reference image to do anything.
            spdlog::critical( "The reference image is null" );
            throw_debug( "The reference image is null" )
        }

        m_rendering.initTextures();
        m_rendering.updateImageUniforms( m_data.imageUidsOrdered() );
//        m_rendering.setRenderImages( true );

        m_glfw.setEventProcessingMode( EventProcessingMode::Wait ); // Render only on events
        m_glfw.setWindowTitleStatus( m_data.getAllImageDisplayNames() );

        m_data.guiData().m_renderUiWindows = true;
        m_data.guiData().m_renderUiOverlays = true;
        m_data.settings().setAnimating( false );
        m_data.settings().setOverlays( true );

        m_data.windowData().addAxCorSagLayout( m_data.numImages() );
        m_data.windowData().addLightboxLayoutForImage( refImg->header().pixelDimensions().z );

        m_data.windowData().setDefaultRenderedImagesForAllLayouts( m_data.imageUidsOrdered() );

        m_imgui.render(); // Render one frame of the UI

        constexpr bool k_recenterCrosshairs = true;
        constexpr bool k_recenterOnCurrentCrosshairsPos = false;

        m_callbackHandler.recenterViews( ImageSelection::AllLoadedImages,
                                         k_recenterCrosshairs,
                                         k_recenterOnCurrentCrosshairsPos );

        m_callbackHandler.setMouseMode( MouseMode::Pointer );

        spdlog::debug( "Textures ready; rendering enabled" );
    };

    m_glfw.renderLoop( m_imagesReady, m_imageLoadFailed, onImagesReady );
    spdlog::debug( "Done application run loop" );
}


void AntropyApp::resize( int width, int height )
{
    m_rendering.resize( width, height );
}


void AntropyApp::render()
{
    m_glfw.renderOnce();
}


CallbackHandler& AntropyApp::callbackHandler() { return m_callbackHandler; }

const AppData& AntropyApp::appData() const { return m_data; }
AppData& AntropyApp::appData() { return m_data; }

const AppSettings& AntropyApp::appSettings() const { return m_data.settings(); }
AppSettings& AntropyApp::appSettings() { return m_data.settings(); }

const GuiData& AntropyApp::guiData() const { return m_data.guiData(); }
GuiData& AntropyApp::guiData() { return m_data.guiData(); }

const GlfwWrapper& AntropyApp::glfw() const { return m_glfw; }
GlfwWrapper& AntropyApp::glfw() { return m_glfw; }

const WindowData& AntropyApp::windowData() const { return m_data.windowData(); }
WindowData& AntropyApp::windowData() { return m_data.windowData(); }


void AntropyApp::logPreamble()
{
    spdlog::info( "{} (version {})", ANTROPY_APPNAME_FULL, ANTROPY_VERSION_FULL );
    spdlog::info( "{}", ANTROPY_ORGNAME_FULL );
}


std::pair< std::optional<uuids::uuid>, bool >
AntropyApp::loadImage( const std::string& fileName, bool ignoreIfAlreadyLoaded )
{
    if ( ignoreIfAlreadyLoaded )
    {
        // Has this image already been loaded? Search for its file name:
        for ( const auto& imageUid : m_data.imageUidsOrdered() )
        {
            if ( const Image* image = m_data.image( imageUid ) )
            {
                if ( image->header().fileName() == fileName )
                {
                    spdlog::info( "Image {} has already been loaded as {}", fileName, imageUid );
                    return { imageUid, false };
                }
            }
        }
    }

    Image image( fileName, Image::ImageRepresentation::Image,
                 Image::MultiComponentBufferType::SeparateImages );

    spdlog::info( "Read image from file {}", fileName );

    std::ostringstream ss;
    image.metaData( ss );

    spdlog::trace( "Meta data:\n{}", ss.str() );
    spdlog::info( "Header:\n{}", image.header() );
    spdlog::info( "Transformation:\n{}", image.transformations() );
    spdlog::info( "Settings:\n{}", image.settings() );

    return { m_data.addImage( std::move(image) ), true };
}


std::pair< std::optional<uuids::uuid>, bool >
AntropyApp::loadSegmentation(
        const std::string& fileName,
        const std::optional<uuids::uuid>& matchingImageUid )
{
    // Return value indicating that segmentation was not loaded:
    static const std::pair< std::optional<uuids::uuid>, bool > sk_noSegLoaded{ std::nullopt, false };

    // Has this segmentation already been loaded? Search for its file name:
    for ( const auto& segUid : m_data.segUidsOrdered() )
    {
        if ( const Image* seg = m_data.seg( segUid ) )
        {
            if ( seg->header().fileName() == fileName )
            {
                spdlog::info( "Segmentation from file {} has already been loaded as {}",
                              fileName, segUid );
                return { segUid, false };
            }
        }
    }

    Image seg( fileName,
               Image::ImageRepresentation::Segmentation,
               Image::MultiComponentBufferType::SeparateImages );

    spdlog::info( "Read segmentation image from file {}", fileName );

    std::ostringstream ss;
    seg.metaData( ss );

    spdlog::trace( "Meta data:\n{}", ss.str() );
    spdlog::info( "Header:\n{}", seg.header() );
    spdlog::info( "Transformation:\n{}", seg.transformations() );

    const Image* matchImg = ( matchingImageUid )
            ? m_data.image( *matchingImageUid ) : nullptr;

    if ( ! matchImg )
    {
        // No valid image was provided to match with this segmentation.
        // Just try to add the segmentation.
        const auto segUid = m_data.addSeg( std::move(seg) );

        if ( segUid )
        {
            return { *segUid, true };
        }
        else
        {
            return sk_noSegLoaded;
        }
    }

    // Set the default opacity:
    seg.settings().setOpacity( 0.5 );

    // Compare header of segmentation with header of its matching image:
    const auto& imgTx = matchImg->transformations();
    const auto& segTx = seg.transformations();


    // This check should be performed after affine_T_subject has been set for both the image
    // and its segmentation:
//    if ( imgTx.get_affine_T_subject() != segTx.get_affine_T_subject() )
//    {
//        spdlog::warn( "The affine_T_subject transformations for image {} "
//                      "and segmentation do not match", *matchingImageUid );
//        spdlog::error( "The segmentation from file {} will not be loaded", fileName );
//        return sk_noSegAdded;
//    }


    /// @todo Turn these equality checks into glm::epsilonEquals checks
    if ( imgTx.subject_T_texture() != segTx.subject_T_texture() )
    {
        spdlog::warn( "The subject_T_texture transformations for image {} "
                      "and segmentation from file {} do not match", *matchingImageUid, fileName );

        const auto& imgHdr = matchImg->header();
        const auto& segHdr = seg.header();

        if ( imgHdr.origin() != segHdr.origin() )
        {
            spdlog::warn( "The origins of image ({}) and segmentation ({}) do not match",
                          glm::to_string( imgHdr.origin() ),
                          glm::to_string( segHdr.origin() ) );
        }

        if ( imgHdr.spacing() != segHdr.spacing() )
        {
            spdlog::warn( "The voxel spacings of image ({}) and segmentation ({}) do not match",
                          glm::to_string( imgHdr.spacing() ),
                          glm::to_string( segHdr.spacing() ) );
        }

        if ( imgHdr.directions() != segHdr.directions() )
        {
            spdlog::warn( "The direction vectors of image ({}) and segmentation ({}) do not match",
                          glm::to_string( imgHdr.directions() ),
                          glm::to_string( segHdr.directions() ) );
        }

        if ( imgHdr.pixelDimensions() != segHdr.pixelDimensions() )
        {
            spdlog::warn( "The pixel dimensions of image ({}) and segmentation ({}) do not match",
                          glm::to_string( imgHdr.pixelDimensions() ),
                          glm::to_string( segHdr.pixelDimensions() ) );
        }

        spdlog::error( "The segmentation from file {} will not be loaded due to "
                       "subject_T_texture mismatch", fileName );

        return sk_noSegLoaded;
    }

    // The image and segmentation transformations match!

    if ( ! isComponentUnsignedInt( seg.header().memoryComponentType() ) )
    {
        spdlog::error( "The segmentation from {} does not have unsigned integer pixel component type "
                       "and so will not be loaded.", fileName );
        return sk_noSegLoaded;
    }

    // Synchronize transformation on all segmentations of the image:
    m_callbackHandler.syncManualImageTransformationOnSegs( *matchingImageUid );

    if ( const auto segUid = m_data.addSeg( std::move(seg) ) )
    {
        spdlog::info( "Loaded segmentation from file {}", fileName );
        return { *segUid, true };
    }

    return sk_noSegLoaded;
}


std::pair< std::optional<uuids::uuid>, bool >
AntropyApp::loadDeformationField( const std::string& fileName )
{
    // Return value indicating that deformation field was not loaded:
    static const std::pair< std::optional<uuids::uuid>, bool > sk_noDefLoaded{ std::nullopt, false };

    // Has this deformation field already been loaded? Search for its file name:
    for ( const auto& defUid : m_data.defUidsOrdered() )
    {
        if ( const Image* def = m_data.def( defUid ) )
        {
            if ( def->header().fileName() == fileName )
            {
                spdlog::info( "Deformation field {} has already been loaded as {}", fileName, defUid );
                return { defUid, false };
            }
        }
    }

    // Components of a deformation field image are loaded as interleaved images
    Image def( fileName, Image::ImageRepresentation::Image,
               Image::MultiComponentBufferType::InterleavedImage );

    spdlog::info( "Read deformation field image from file {}", fileName );

    std::ostringstream ss;
    def.metaData( ss );

    spdlog::trace( "Meta data:\n{}", ss.str() );
    spdlog::info( "Header:\n{}", def.header() );
    spdlog::info( "Transformation:\n{}", def.transformations() );
    spdlog::info( "Settings:\n{}", def.settings() );

    /// @todo Do check of deformation field header against the reference image header?

    if ( def.header().numComponentsPerPixel() < 3 )
    {
        spdlog::error( "The deformation field from {} has fewer than three components per pixel "
                       "and so will not be loaded.", fileName );
        return sk_noDefLoaded;
    }

    if ( const auto defUid = m_data.addDef( std::move(def) ) )
    {
        spdlog::info( "Loaded deformation field image from file {} as {}", fileName, *defUid );
        return { *defUid, true };
    }

    return sk_noDefLoaded;
}


std::optional<uuids::uuid> AntropyApp::createBlankSeg(
        const uuids::uuid& matchImageUid,
        std::string segDisplayName )
{
    const Image* matchImg = m_data.image( matchImageUid );

    if ( ! matchImg )
    {
        spdlog::debug( "Cannot create blank segmentation for invalid matching image {}", matchImageUid );
        return std::nullopt; // Invalid image provided
    }

    // Copy the image header, changing it to scalar with uint8_t components:
    ImageHeader segHeader = matchImg->header();

    segHeader.setExistsOnDisk( false );
    segHeader.setFileName( "" );
    segHeader.adjustToScalarUCharFormat();

    Image seg( segHeader, segDisplayName,
               Image::ImageRepresentation::Segmentation,
               Image::MultiComponentBufferType::SeparateImages );

    // Set the default opacity:
    seg.settings().setOpacity( 0.5 );

    spdlog::info( "Created segmentation matching header of image {}", matchImageUid );
    spdlog::debug( "Header:\n{}", seg.header() );
    spdlog::debug( "Transformation:\n{}", seg.transformations() );

    const auto segUid = m_data.addSeg( std::move( seg ) );

    // Synchronize transformation on all segmentations of the image:
    m_callbackHandler.syncManualImageTransformationOnSegs( matchImageUid );

    // Update uniforms for all images
    m_rendering.updateImageUniforms( m_data.imageUidsOrdered() );

    return segUid;
}


std::optional<uuids::uuid> AntropyApp::createBlankSegWithColorTable(
        const uuids::uuid& matchImageUid,
        std::string segDisplayName )
{
    const Image* matchImage = m_data.image( matchImageUid );
    if ( ! matchImage )
    {
        spdlog::error( "Cannot create blank segmentation for invalid image {}", matchImageUid );
        return std::nullopt;
    }

    auto segUid = createBlankSeg( matchImageUid, segDisplayName );
    if ( ! segUid )
    {
        spdlog::error( "Error creating blank segmentation for image {}", matchImageUid );
        return std::nullopt;
    }

    spdlog::debug( "Created blank segmentation {} ('{}') for image {}",
                   matchImageUid, segDisplayName, matchImageUid );

    Image* seg = m_data.seg( *segUid );
    if ( ! seg )
    {
        spdlog::error( "Null segmentation created {}", *segUid );
        m_data.removeSeg( *segUid );
        return std::nullopt;
    }

    const auto tableUid = data::createLabelColorTableForSegmentation( m_data, *segUid );
    bool createdTableTexture = false;

    if ( tableUid )
    {
        createdTableTexture = m_rendering.createLabelColorTableTexture( *tableUid );
    }

    if ( ! tableUid || ! createdTableTexture )
    {
        constexpr size_t k_defaultTableIndex = 0;

        spdlog::error( "Unable to create label color table for segmentation {}. "
                       "Defaulting to table index {}.", *segUid, k_defaultTableIndex );

        seg->settings().setLabelTableIndex( k_defaultTableIndex );
    }

    if ( m_data.assignSegUidToImage( matchImageUid, *segUid ) )
    {
        spdlog::info( "Assigned segmentation {} to image {}", *segUid, matchImageUid );
    }
    else
    {
        spdlog::error( "Unable to assign segmentation {} to image {}", *segUid, matchImageUid );
        m_data.removeSeg( *segUid );
        return std::nullopt;
    }

    // Make it the active segmentation
    m_data.assignActiveSegUidToImage( matchImageUid, *segUid );

    if ( ! m_rendering.createSegTexture( *segUid ) )
    {
        spdlog::error( "Unable to create texture for segmentation {}", *segUid );
        m_data.removeSeg( *segUid );
        return std::nullopt;
    }

    // Assign the image's affine_T_subject transformation to its segmentation:
    seg->transformations().set_affine_T_subject(
                matchImage->transformations().get_affine_T_subject() );

    // Synchronize transformation on all segmentations of the image:
    m_callbackHandler.syncManualImageTransformationOnSegs( matchImageUid );

    // Update uniforms for all images
    m_rendering.updateImageUniforms( m_data.imageUidsOrdered() );

    return *segUid;
}


bool AntropyApp::loadSerializedImage( const serialize::Image& serializedImage )
{
    constexpr size_t sk_defaultImageColorMapIndex = 0;

    // Do NOT ignore images if they have already been loaded:
    // (i.e. load duplicate images again anyway):
    constexpr bool k_ignoreImageIfAlreadyLoaded = false;

    // Load image:
    std::optional<uuids::uuid> imageUid;
    bool isNewImage = false;

    try
    {
        spdlog::debug( "Attempting to load image from {}", serializedImage.m_imageFileName );

        std::tie( imageUid, isNewImage ) = loadImage(
                    serializedImage.m_imageFileName, k_ignoreImageIfAlreadyLoaded );
    }
    catch ( const std::exception& e )
    {
        spdlog::error( "Exception loading image from {}: {}",
                       serializedImage.m_imageFileName, e.what() );
        return false;
    }

    if ( ! imageUid )
    {
        spdlog::error( "Unable to load image from {}", serializedImage.m_imageFileName );
        return false;
    }

    if ( ! isNewImage )
    {
        spdlog::info( "Image from {} already exists in this project as {}",
                      serializedImage.m_imageFileName, *imageUid );

        if ( k_ignoreImageIfAlreadyLoaded )
        {
            // Because this setting is true, cancel loading the rest of the data for this image:
            return true;
        }
    }

    Image* image = m_data.image( *imageUid );
    if ( ! image )
    {
        spdlog::error( "Null image {}", *imageUid );
        return false;
    }

    spdlog::info( "Loaded image from {} as {}", serializedImage.m_imageFileName, *imageUid );

    /// @todo Load from project settings
    for ( uint32_t i = 0; i < image->header().numComponentsPerPixel(); ++i )
    {
        image->settings().setColorMapIndex( i, sk_defaultImageColorMapIndex );
    }


    // Load and set affine transformation from file:
    if ( serializedImage.m_affineTxFileName )
    {
        glm::dmat4 affine_T_subject( 1.0 );

        if ( ! serialize::openAffineTxFile( affine_T_subject, *serializedImage.m_affineTxFileName ) )
        {
            image->transformations().set_affine_T_subject_fileName( std::nullopt );

            spdlog::error( "Unable to read affine transformation from file {} for image {}",
                           *serializedImage.m_affineTxFileName, *imageUid );
        }

        image->transformations().set_affine_T_subject_fileName( serializedImage.m_affineTxFileName );
        image->transformations().set_affine_T_subject( glm::mat4{ affine_T_subject } );
    }
    else
    {
        // No affine transformation provided:
        image->transformations().set_affine_T_subject_fileName( std::nullopt );
    }



    if ( serializedImage.m_deformationFileName )
    {
        std::optional<uuids::uuid> deformationUid;
        bool isDeformationNewImage = false;

        try
        {
            spdlog::debug( "Attempting to load deformation field image from {}",
                           *serializedImage.m_deformationFileName );

            std::tie( deformationUid, isDeformationNewImage ) =
                    loadDeformationField( *serializedImage.m_deformationFileName );
        }
        catch ( const std::exception& e )
        {
            spdlog::error( "Exception loading deformation field from {}: {}",
                           *serializedImage.m_deformationFileName, e.what() );
        }

        do
        {
            if ( ! deformationUid )
            {
                spdlog::error( "Unable to load deformation field from {} for image {}",
                               *serializedImage.m_deformationFileName, *imageUid );
                break;
            }

            if ( ! isDeformationNewImage )
            {
                spdlog::info( "Deformation field from {} already exists in this project as image {}",
                              *serializedImage.m_deformationFileName, *deformationUid );
                break;
            }

            Image* deformation = m_data.def( *deformationUid );

            if ( ! deformation )
            {
                spdlog::error( "Null deformation field image {}", *deformationUid );
                break;
            }

            deformation->settings().setDisplayName(
                        deformation->settings().displayName() + " (deformation)" );

            /// @todo Load this from project settings
            for ( uint32_t i = 0; i < deformation->header().numComponentsPerPixel(); ++i )
            {
                deformation->settings().setColorMapIndex( i, 25 );
            }

            if ( m_data.assignDefUidToImage( *imageUid, *deformationUid ) )
            {
                spdlog::info( "Assigned deformation field {} to image {}", *deformationUid, *imageUid );
            }
            else
            {
                spdlog::error( "Unable to assign deformation field {} to image {}",
                               *deformationUid, *imageUid );
                m_data.removeDef( *deformationUid );
                break;
            }

            break;
        } while ( 1 );


        /// @todo Deformation field images are special:
        /// 1) no segmentation is created
        /// 2) no affine transformation can be applied: it copies the affine tx of its image
        /// 3) need warning when header tx doesn't match that of reference
        /// 4) even if all components are loaded as RGB texure, we should be able to view each component separately in a shader that takes
        /// in as a uniform the active component
    }



    static const auto sk_hueMinMax = std::make_pair( 0.0f, 360.0f );
    static const auto sk_satMinMax = std::make_pair( 0.6f, 1.0f );
    static const auto sk_valMinMax = std::make_pair( 0.6f, 1.0f );


    // Set landmarks from file:
    for ( const auto& lm : serializedImage.m_landmarks )
    {
        std::map< size_t, PointRecord<glm::vec3> > landmarks;

        if ( serialize::openLandmarksFile( landmarks, lm.m_csvFileName ) )
        {
            spdlog::info( "Loaded landmarks from CSV file {} for image {}", lm.m_csvFileName, *imageUid );

            // Assign random colors to the landmarks. Make sure that landmarks with the same index
            // in different groups have the same color. This is done by seeding the random number
            // generator with the landmark index.
            for ( auto& p : landmarks )
            {
                const auto colors = math::generateRandomHsvSamples(
                            1, sk_hueMinMax, sk_satMinMax, sk_valMinMax,
                            static_cast<uint32_t>( p.first ) );

                if ( ! colors.empty() )
                {
                    p.second.setColor( glm::rgbColor( colors[0] ) );
                }
            }


            LandmarkGroup lmGroup;

            lmGroup.setFileName( lm.m_csvFileName );
            lmGroup.setName( getFileName( lm.m_csvFileName, false ) );
            lmGroup.setPoints( landmarks );
            lmGroup.setRenderLandmarkNames( false );

            if ( lm.m_inVoxelSpace )
            {
                lmGroup.setInVoxelSpace( true );
                spdlog::info( "Landmarks are defined in Voxel space" );
            }
            else
            {
                lmGroup.setInVoxelSpace( false );
                spdlog::info( "Landmarks are defined in physical Subject space" );
            }

            for ( const auto& p : landmarks )
            {
                spdlog::trace( "Landmark {} ('{}') : {}", p.first, p.second.getName(),
                               glm::to_string( p.second.getPosition() ) );
            }

            const auto lmGroupUid = m_data.addLandmarkGroup( lmGroup );
            const bool linked = m_data.assignLandmarkGroupUidToImage( *imageUid, lmGroupUid );

            if ( ! linked )
            {
                spdlog::error( "Unable to assigned landmark group {} to image {}",
                               lmGroupUid, *imageUid );
            }
        }
        else
        {
            spdlog::error( "Unable to open landmarks from CSV file {} for image {}",
                           lm.m_csvFileName, *imageUid );
        }
    }



    // Load segmentation images:

    // Structure for holding information about a segmentation being loaded
    struct SegInfo
    {
        std::optional<uuids::uuid> uid; // Seg UID assigned by AppData after the image is loaded from disk
        bool isNewSeg = false; // Is this a new segmentation (true) or a repeat of a previous one (false)?
        bool needsNewLabelColorTable = true; // Does the segmentation need a new label color table?
    };

    // Holds info about all segmentations being loaded:
    std::vector< SegInfo > allSegInfos;

    for ( const auto& serializedSeg : serializedImage.m_segmentations )
    {
        SegInfo segInfo;

        try
        {
            spdlog::debug( "Attempting to load segmentation image from {}",
                           serializedSeg.m_segFileName );

            std::tie( segInfo.uid, segInfo.isNewSeg ) =
                    loadSegmentation( serializedSeg.m_segFileName, *imageUid );
        }
        catch ( const std::exception& e )
        {
            spdlog::error( "Exception loading segmentation from {}: {}",
                           serializedSeg.m_segFileName, e.what() );
            continue; // Skip this segmentation
        }

        if ( segInfo.uid )
        {
            if ( segInfo.isNewSeg )
            {
                spdlog::info( "Loaded segmentation from {} for image {} as {}",
                              serializedSeg.m_segFileName, *imageUid, *segInfo.uid );

                // New segmentation needs a new table
                segInfo.needsNewLabelColorTable = true;
            }
            else
            {
                spdlog::info( "Segmentation from {} already exists as {}, so it was not loaded again. "
                              "This segmentation will be shared across all images that reference it.",
                              serializedSeg.m_segFileName, *segInfo.uid );

                // Existing segmentation does not need a new table
                segInfo.needsNewLabelColorTable = false;
            }

            allSegInfos.push_back( segInfo );
        }
    }


    if ( allSegInfos.empty() )
    {
        // No segmentation was loaded!
        spdlog::debug( "No segmentation loaded for image {}; creating blank segmentation.", *imageUid );

        try
        {
            const std::string segDisplayName =
                    std::string( "Untitled segmentation for image '" ) +
                    image->settings().displayName() + "'";

            SegInfo segInfo;
            segInfo.uid = createBlankSeg( *imageUid, segDisplayName );
            segInfo.isNewSeg = true;
            segInfo.needsNewLabelColorTable = true;

            if ( segInfo.uid )
            {
                spdlog::debug( "Created blank segmentation {} ('{}') for image {}",
                               *segInfo.uid, segDisplayName, *imageUid );
            }
            else
            {
                // This is a problem that we can't recover from:
                spdlog::error( "Error creating blank segmentation for image {}. "
                               "No segmentation will be assigned to the image.", *imageUid );
                return false;
            }

            allSegInfos.push_back( segInfo );
        }
        catch ( const std::exception& e )
        {
            spdlog::error( "Exception creating blank segmentation for image {}: {}", *imageUid, e.what() );
            spdlog::error( "No segmentation will be assigned to the image." );
            return false;
        }
    }


    /// @todo Put all this into the loadSeg function?
    for ( const SegInfo& segInfo : allSegInfos )
    {
        Image* seg = m_data.seg( *segInfo.uid );

        if ( ! seg )
        {
            spdlog::error( "Null segmentation {}", *segInfo.uid );
            m_data.removeSeg( *segInfo.uid );
            continue;
        }

        if ( segInfo.needsNewLabelColorTable )
        {
            if ( ! data::createLabelColorTableForSegmentation( m_data, *segInfo.uid ) )
            {
                constexpr size_t k_defaultTableIndex = 0;

                spdlog::error( "Unable to create label color table for segmentation {}. "
                               "Defaulting to table index {}.", *segInfo.uid, k_defaultTableIndex );

                seg->settings().setLabelTableIndex( k_defaultTableIndex );
            }
        }

        if ( m_data.assignSegUidToImage( *imageUid, *segInfo.uid ) )
        {
            spdlog::info( "Assigned segmentation {} to image {}", *segInfo.uid, *imageUid );
        }
        else
        {
            spdlog::error( "Unable to assign segmentation {} to image {}", *segInfo.uid, *imageUid );
            m_data.removeSeg( *segInfo.uid );
            continue;
        }

        // Assign the image's affine_T_subject transformation to its segmentation:
        seg->transformations().set_affine_T_subject( image->transformations().get_affine_T_subject() );
    }


    // Checks that the image has at least one segmentation:
    if ( m_data.imageToSegUids( *imageUid ).empty() )
    {
        spdlog::error( "Image {} has no segmentation", *imageUid );
        return false;
    }
    else if ( ! m_data.imageToActiveSegUid( *imageUid ) )
    {
        // The image has no active segmentation, so assign the first seg as the active one:
        const auto firstSegUid = m_data.imageToSegUids( *imageUid ).front();
        m_data.assignActiveSegUidToImage( *imageUid, firstSegUid );
    }

    return true;
}


void AntropyApp::loadImagesFromParams( const InputParams& params )
{
    spdlog::debug( "Begin loading images from parameters" );

    // The image loader function is called from a new thread
    auto projectLoader = [this]
            ( const serialize::AntropyProject& project,
              const std::function< void( bool projectLoadedSuccessfully ) >& onProjectLoadingDone )
    {
        static constexpr size_t sk_defaultReferenceImageIndex = 0;
        static constexpr size_t sk_defaultActiveImageIndex = 1;

        // Set event processing mode to poll, so that we can get continuous animations while loading
        m_glfw.setEventProcessingMode( EventProcessingMode::Poll );
        m_data.settings().setAnimating( true );

        spdlog::debug( "Begin loading images" );

        const bool loadedReference = this->loadSerializedImage( project.m_referenceImage );

        if ( ! loadedReference )
        {
            spdlog::critical( "Could not load reference image {}",
                              project.m_referenceImage.m_imageFileName );
            onProjectLoadingDone( false );
        }

        for ( const auto& additionalImage : project.m_additionalImages )
        {
            const bool loadedImage = this->loadSerializedImage( additionalImage );

            if ( ! loadedImage )
            {
                spdlog::error( "Could not load additional image {}; skipping it",
                               additionalImage.m_imageFileName );
            }
        }

        if ( 0 == m_data.numImages() )
        {
            spdlog::critical( "There are no images loaded" );
            onProjectLoadingDone( false );
        }

        const auto refImageUid = m_data.imageUid( sk_defaultReferenceImageIndex );

        if ( refImageUid && m_data.setRefImageUid( *refImageUid ) )
        {           
            spdlog::info( "Set {} as the reference image", *refImageUid );
        }
        else
        {
            spdlog::critical( "Unable to set {} as the reference image", *refImageUid );
            onProjectLoadingDone( false );
        }

        if ( sk_defaultActiveImageIndex < m_data.numImages() )
        {
            const auto actImgUid = m_data.imageUid( sk_defaultActiveImageIndex );

            if ( actImgUid && m_data.setActiveImageUid( *actImgUid ) )
            {
                spdlog::info( "Set {} as the active image", *actImgUid );
            }
            else
            {
                spdlog::error( "Unable to set {} as the active image", *actImgUid );
//                onProjectLoadingDone( false );
            }
        }
        else
        {
            if ( m_data.setActiveImageUid( *refImageUid ) )
            {
                spdlog::info( "Set reference image ({}) as the active image", *refImageUid );
            }
            else
            {
                spdlog::error( "Unable to set {} as the active image", *refImageUid );
//                onProjectLoadingDone( false );
            }
        }

        /// @note The brush size in mm is not currently being used anywhere
//      // Set brush size to min voxel size
//      if ( const Image* image = m_data.refImage() )
//      {
//        m_data.setBrushSizeInMm( glm::compMin( image->header().spacing() ) );
//      }


        // Assign nice rainbow colors
        m_data.setRainbowColorsForImageEdges();
        m_data.setRainbowColorsForLandmarkGroups();

        // If there is only one image, show the tri-view layout:
        if ( 1 == m_data.numImages() )
        {
            m_data.windowData().setCurrentLayoutIndex( 1 );
        }

        onProjectLoadingDone( true );
    };


    auto onProjectLoadingDone = [this]( bool projectLoadedSuccessfully )
    {
        if ( projectLoadedSuccessfully )
        {
            // Set flag that images are ready and post an empty event to notify render thread
            // that there is an event
            m_imagesReady = true;
            m_glfw.postEmptyEvent();
            spdlog::debug( "Done loading images" );
        }
        else
        {
            m_imagesReady = false;
            m_imageLoadFailed = true; // This flag will cause the render loop to exit
            spdlog::critical( "Failed to load images" );
        }
    };


    m_glfw.setWindowTitleStatus( "Loading images..." );

    // Create a project to be loaded in from the input parameters
    m_data.setProject( createProjectFromInputParams( params ) );

    m_futureLoadProject = std::async(
                std::launch::async, projectLoader,
                m_data.project(), onProjectLoadingDone );

    spdlog::debug( "Done loading images from parameters" );
}


//void AntropyApp::broadcastCrosshairsPosition()
//{
//    // We need a reference image to move the crosshairs
//    const Image* refImg = m_data.refImage();
//    if ( ! refImg ) return;

//    const auto& refTx = refImg->transformations();

//    // Convert World to reference Subject position:
//    glm::vec4 refSubjectPos = refTx.subject_T_world() *
//        glm::vec4{ m_data.worldCrosshairs().worldOrigin(), 1.0f };

//    refSubjectPos /= refSubjectPos.w;

//    // Read the contents of shared memory into the local message object
//    IPCMessage message;
//    m_IPCHandler.Read( static_cast<void*>( &message ) );

////    spdlog::info( "cursor = {}, {}, {}", message.cursor[0], message.cursor[1], message.cursor[2] );

//    // Convert LPS to RAS
//    message.cursor[0] = -refSubjectPos[0];
//    message.cursor[1] = -refSubjectPos[1];
//    message.cursor[2] = refSubjectPos[2];

//    if ( ! m_IPCHandler.Broadcast( static_cast<void*>( &message ) ) )
//    {
//        spdlog::warn( "Failed to broadcast IPC" );
//    }
//}

