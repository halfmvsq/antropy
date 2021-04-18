﻿#include "ui/ImGuiWrapper.h"

#include "ui/Helpers.h"
#include "ui/Style.h"
#include "ui/Toolbars.h"
#include "ui/Widgets.h"
#include "ui/Windows.h"

#include "logic/app/CallbackHandler.h"
#include "logic/app/Data.h"
#include "logic/camera/CameraHelpers.h"

#include <IconFontCppHeaders/IconsForkAwesome.h>

#include <imgui/imgui.h>

// GLFW and OpenGL 3 bindings for ImGui:
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cmrc/cmrc.hpp>

#include <glm/glm.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

CMRC_DECLARE(fonts);


namespace
{

static const glm::quat sk_identityRotation{ 1.0f, 0.0f, 0.0f, 0.0f };
static const glm::vec3 sk_zeroVec{ 0.0f, 0.0f, 0.0f };

}


ImGuiWrapper::ImGuiWrapper(
        GLFWwindow* window,
        AppData& appData,
        CallbackHandler& callbackHandler )
    :
      m_appData( appData ),
      m_callbackHandler( callbackHandler ),

      m_recenterView( nullptr ),
      m_recenterAllViews( nullptr ),

      m_getOverlayVisibility( nullptr ),
      m_setOverlayVisibility( nullptr ),

      m_updateImageUniforms( nullptr ),
      m_updateImageInterpolationMode( nullptr ),
      m_updateMetricUniforms( nullptr ),

      m_getWorldDeformedPos( nullptr ),
      m_getSubjectPos( nullptr ),
      m_getVoxelPos( nullptr ),
      m_setSubjectPos( nullptr ),
      m_setVoxelPos( nullptr ),
      m_getImageValue( nullptr ),
      m_getSegLabel( nullptr ),

      m_createBlankSeg( nullptr ),
      m_clearSeg( nullptr ),
      m_removeSeg( nullptr ),

      m_executeGridCutsSeg( nullptr ),
      m_setLockManualImageTransformation( nullptr )
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    spdlog::debug( "Created ImGui context" );

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "antropy_ui.ini";
    io.LogFilename = "logs/antropy_ui.log";

    // Apply a custom dark style:
    applyCustomDarkStyle();

    // Setup ImGui platform/renderer bindings:
    static const char* glsl_version = "#version 150";
    ImGui_ImplGlfw_InitForOpenGL( window, true );
    ImGui_ImplOpenGL3_Init( glsl_version );

    spdlog::debug( "Done setup of ImGui platform and renderer bindings" );

    initializeData();
}


ImGuiWrapper::~ImGuiWrapper()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    spdlog::debug( "Shutdown ImGui" );
    ImGui::DestroyContext();
    spdlog::debug( "Destroyed ImGui context" );
}


void ImGuiWrapper::setCallbacks(
        std::function< void ( const uuids::uuid& viewUid )> recenterView,
        std::function< void ( bool recenterCrosshairs, bool recenterOnCurrentCrosshairsPosition, bool resetObliqueOrientation )> recenterCurrentViews,
        std::function< bool ( void ) > getOverlayVisibility,
        std::function< void ( bool ) > setOverlayVisibility,
        std::function< void ( const uuids::uuid& viewUid )> updateImageUniforms,
        std::function< void ( const uuids::uuid& viewUid )> updateImageInterpolationMode,
        std::function< void ( size_t tableIndex ) > updateLabelColorTableTexture,
        std::function< void ()> updateMetricUniforms,
        std::function< glm::vec3 () > getWorldDeformedPos,
        std::function< std::optional<glm::vec3> ( size_t imageIndex ) > getSubjectPos,
        std::function< std::optional<glm::ivec3> ( size_t imageIndex ) > getVoxelPos,
        std::function< void ( size_t imageIndex, const glm::vec3& subjectPos ) > setSubjectPos,
        std::function< void ( size_t imageIndex, const glm::ivec3& voxelPos ) > setVoxelPos,
        std::function< std::optional<double> ( size_t imageIndex ) > getImageValue,
        std::function< std::optional<int64_t> ( size_t imageIndex ) > getSegLabel,
        std::function< std::optional<uuids::uuid>( const uuids::uuid& matchingImageUid, const std::string& segDisplayName ) > createBlankSeg,
        std::function< bool ( const uuids::uuid& segUid ) > clearSeg,
        std::function< bool ( const uuids::uuid& segUid ) > removeSeg,
        std::function< bool ( const uuids::uuid& imageUid, const uuids::uuid& seedSegUid, const uuids::uuid& resultSegUid ) > executeGridCutsSeg,
        std::function< bool ( const uuids::uuid& imageUid, bool locked ) > setLockManualImageTransformation )
{
    m_recenterView = recenterView;
    m_recenterAllViews = recenterCurrentViews;
    m_getOverlayVisibility = getOverlayVisibility;
    m_setOverlayVisibility = setOverlayVisibility;
    m_updateImageUniforms = updateImageUniforms;
    m_updateImageInterpolationMode = updateImageInterpolationMode;
    m_updateLabelColorTableTexture = updateLabelColorTableTexture;
    m_updateMetricUniforms = updateMetricUniforms;
    m_getWorldDeformedPos = getWorldDeformedPos;
    m_getSubjectPos = getSubjectPos;
    m_getVoxelPos = getVoxelPos;
    m_setSubjectPos = setSubjectPos;
    m_setVoxelPos = setVoxelPos;
    m_getImageValue = getImageValue;
    m_getSegLabel = getSegLabel;
    m_createBlankSeg = createBlankSeg;
    m_clearSeg = clearSeg;
    m_removeSeg = removeSeg;
    m_executeGridCutsSeg = executeGridCutsSeg;
    m_setLockManualImageTransformation = setLockManualImageTransformation;
}


void ImGuiWrapper::initializeData()
{   
    static const std::string cousineFontPath( "resources/fonts/Cousine/Cousine-Regular.ttf" );
    static const std::string forkAwesomeFontPath = std::string( "resources/fonts/ForkAwesome/" ) + FONT_ICON_FILE_NAME_FK;

    static constexpr double sk_fontSize = 15.0;
    static const ImWchar forkAwesomeIconGlyphRange[] = { ICON_MIN_FK, ICON_MAX_FK, 0 };

    ImGuiIO& io = ImGui::GetIO();

    // Load fonts: If no fonts are loaded, dear imgui will use the default font.
    // You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the
    // font among multiple. If the file cannot be loaded, the function will return NULL.
    // Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when
    // calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.

//    auto font_default = io.Fonts->AddFontDefault();

    auto filesystem = cmrc::fonts::get_filesystem();

    /// @see For details about Fork Awesome fonts: https://forkaweso.me/Fork-Awesome/icons/

    cmrc::file cousineFontFile = filesystem.open( cousineFontPath );
    cmrc::file forkAwesomeWebFontFile = filesystem.open( forkAwesomeFontPath );

    // ImGui will take ownership of the font, so make a copy:
    m_appData.guiData().m_cousineFontData = new char[cousineFontFile.size()];
    m_appData.guiData().m_forkAwesomeFontData = new char[forkAwesomeWebFontFile.size()];

    for ( size_t i = 0; i < cousineFontFile.size(); ++i )
    {
        m_appData.guiData().m_cousineFontData[i] = cousineFontFile.cbegin()[i];
    }

    for ( size_t i = 0; i < forkAwesomeWebFontFile.size(); ++i )
    {
        m_appData.guiData().m_forkAwesomeFontData[i] = forkAwesomeWebFontFile.cbegin()[i];
    }

    ImFontConfig cousineFontConfig;

    ImFontConfig forkAwesomeConfig;
    forkAwesomeConfig.MergeMode = true;
    forkAwesomeConfig.PixelSnapH = true;

    myImFormatString( cousineFontConfig.Name, IM_ARRAYSIZE(cousineFontConfig.Name),
                      "%s, %.0fpx", "Cousine", sk_fontSize );

    myImFormatString( forkAwesomeConfig.Name, IM_ARRAYSIZE(forkAwesomeConfig.Name),
                      "%s, %.0fpx", "Fork Awesome", sk_fontSize );

    m_appData.guiData().m_cousineFont = io.Fonts->AddFontFromMemoryTTF(
                static_cast<void*>( m_appData.guiData().m_cousineFontData ),
                static_cast<int32_t>( cousineFontFile.size() ),
                static_cast<float>( sk_fontSize ), &cousineFontConfig );

    // Merge in icons from Fork Awesome:
    m_appData.guiData().m_forkAwesomeFont = io.Fonts->AddFontFromMemoryTTF(
                static_cast<void*>( m_appData.guiData().m_forkAwesomeFontData ),
                static_cast<int32_t>( forkAwesomeWebFontFile.size() ),
                static_cast<float>( sk_fontSize ), &forkAwesomeConfig,
                forkAwesomeIconGlyphRange );

    if ( m_appData.guiData().m_cousineFont )
    {
        spdlog::debug( "Loaded font {}", cousineFontPath );
    }
    else
    {
        spdlog::error( "Could not load font {}", cousineFontPath );
    }

    if ( m_appData.guiData().m_forkAwesomeFont )
    {
        spdlog::debug( "Loaded font {}", forkAwesomeFontPath );
    }
    else
    {
        spdlog::error( "Could not load font {}", forkAwesomeFontPath );
    }

    spdlog::debug( "Initialized ImGui data" );
}


void ImGuiWrapper::render()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    /// @todo Move these functions elsewhere

    auto getImageDisplayAndFileNames = [this] ( size_t imageIndex )
            -> std::pair<const char*, const char*>
    {
        static const std::string s_empty( "<unknown>" );

        if ( const auto imageUid = m_appData.imageUid( imageIndex ) )
        {
            if ( const Image* image = m_appData.image( *imageUid ) )
            {
                return { image->settings().displayName().c_str(),
                         image->header().fileName().c_str() };
            }
        }

        return { s_empty.c_str(), s_empty.c_str() };
    };

    /// @todo remove this
    auto getActiveImageIndex = [this] () -> size_t
    {
        if ( const auto imageUid = m_appData.activeImageUid() )
        {
            if ( const auto index = m_appData.imageIndex( *imageUid ) )
            {
                return *index;
            }
        }

        spdlog::warn( "No valid active image" );
        return 0;
    };

    auto setActiveImageIndex = [this] ( size_t index )
    {
        if ( const auto imageUid = m_appData.imageUid( index ) )
        {
            if ( ! m_appData.setActiveImageUid( *imageUid ) )
            {
                spdlog::warn( "Cannot set active image to {}", *imageUid );
            }
        }
        else
        {
            spdlog::warn( "Cannot set active image to invalid index {}", index );
        }
    };

    auto getImageHasActiveSeg = [this] ( size_t index ) -> bool
    {
        if ( const auto imageUid = m_appData.imageUid( index ) )
        {
            return m_appData.isImageBeingSegmented( *imageUid );
        }
        else
        {
            spdlog::warn( "Cannot get whether seg is active for invalid image index {}", index );
            return false;
        }
    };

    auto setImageHasActiveSeg = [this] ( size_t index, bool set )
    {
        if ( const auto imageUid = m_appData.imageUid( index ) )
        {
            m_appData.setImageBeingSegmented( *imageUid, set );
        }
        else
        {
            spdlog::warn( "Cannot set whether seg is active for invalid image index {}", index );
        }
    };

    /// @todo remove this
    auto getMouseMode = [this] ()
    {
        return m_appData.state().mouseMode();
    };

    auto setMouseMode = [this] ( MouseMode mouseMode )
    {
        m_appData.state().setMouseMode( mouseMode );
    };

    auto cycleViewLayout = [this] ( int step )
    {
        m_appData.windowData().cycleCurrentLayout( step );
    };

    /// @todo remove this
    auto getNumImageColorMaps = [this] () -> size_t
    {
        return m_appData.numImageColorMaps();
    };

    auto getImageColorMap = [this] ( size_t cmapIndex ) -> const ImageColorMap*
    {
        if ( const auto cmapUid = m_appData.imageColorMapUid( cmapIndex ) )
        {
            return m_appData.imageColorMap( *cmapUid );
        }
        return nullptr;
    };

    auto getLabelTable = [this] ( size_t tableIndex ) -> ParcellationLabelTable*
    {
        if ( const auto tableUid = m_appData.labelTableUid( tableIndex ) )
        {
            return m_appData.labelTable( *tableUid );
        }
        return nullptr;
    };

    auto getImageIsVisibleSetting = [this] ( size_t imageIndex ) -> bool
    {
        if ( const auto imageUid = m_appData.imageUid( imageIndex ) )
        {
            if ( const Image* image = m_appData.image( *imageUid ) )
            {
                return image->settings().visibility();
            }
        }
        return false;
    };

    auto moveImageBackward = [this] ( const uuids::uuid& imageUid ) -> bool
    {
        if ( m_appData.moveImageBackwards( imageUid ) )
        {
            m_appData.windowData().updateImageOrdering( m_appData.imageUidsOrdered() );
            return true;
        }
        return false;
    };

    auto moveImageForward = [this] ( const uuids::uuid& imageUid ) -> bool
    {
        if ( m_appData.moveImageForwards( imageUid ) )
        {
            m_appData.windowData().updateImageOrdering( m_appData.imageUidsOrdered() );
            return true;
        }
        return false;
    };

    auto moveImageToBack = [this] ( const uuids::uuid& imageUid ) -> bool
    {
        if ( m_appData.moveImageToBack( imageUid ) )
        {
            m_appData.windowData().updateImageOrdering( m_appData.imageUidsOrdered() );
            return true;
        }
        return false;
    };

    auto moveImageToFront = [this] ( const uuids::uuid& imageUid ) -> bool
    {
        if ( m_appData.moveImageToFront( imageUid ) )
        {
            m_appData.windowData().updateImageOrdering( m_appData.imageUidsOrdered() );
            return true;
        }
        return false;
    };

    auto applyImageSelectionAndShaderToAllViews = [this] ( const uuids::uuid& viewUid )
    {
        m_appData.windowData().applyImageSelectionToAllCurrentViews( viewUid );
        m_appData.windowData().applyViewShaderToAllCurrentViews( viewUid );
    };

    auto getViewCameraRotation = [this] ( const uuids::uuid& viewUid ) -> glm::quat
    {
        const View* view = m_appData.windowData().getCurrentView( viewUid );
        if ( ! view ) return sk_identityRotation;

        return camera::computeCameraRotationRelativeToWorld( view->camera() );
    };

    auto setViewCameraRotation = [this] (
            const uuids::uuid& viewUid,
            const glm::quat& camera_T_world_rotationDelta )
    {
        m_callbackHandler.doCameraRotate3d( viewUid, camera_T_world_rotationDelta );
    };

    auto setViewCameraDirection = [this] (
            const uuids::uuid& viewUid,
            const glm::vec3& worldFwdDirection )
    {
        m_callbackHandler.handleSetViewForwardDirection( viewUid, worldFwdDirection );

        // This experimenting with this simpler function:
//        View* view = m_appData.windowData().getView( viewUid );
//        if ( ! view ) return;
//        camera::orientCameraToWorldTargetNormalDirection( view->camera(), worldFwdDirection );
    };

    auto getViewNormal = [this] ( const uuids::uuid& viewUid )
    {
        View* view = m_appData.windowData().getCurrentView( viewUid );
        if ( ! view ) return sk_zeroVec;
        return camera::worldDirection( view->camera(), Directions::View::Back );
    };

    auto getObliqueViewDirections = [this] ( const uuids::uuid& viewUidToExclude )
            -> std::vector< glm::vec3 >
    {
        std::vector< glm::vec3 > obliqueViewDirections;

        for ( size_t i = 0; i < m_appData.windowData().numLayouts(); ++i )
        {
            const Layout* layout = m_appData.windowData().layout( i );
            if ( ! layout ) continue;

            for ( const auto& view : layout->views() )
            {
                if ( view.first == viewUidToExclude ) continue;
                if ( ! view.second ) continue;

                if ( ! camera::looksAlongOrthogonalAxis( view.second->camera() ) )
                {
                    obliqueViewDirections.emplace_back(
                                camera::worldDirection( view.second->camera(),
                                                        Directions::View::Front ) );
                }
            }
        }

        return obliqueViewDirections;
    };


    ImGui::NewFrame();

    if ( m_appData.guiData().m_renderUiWindows )
    {
        if ( m_appData.guiData().m_showDemoWindow )
        {
            ImGui::ShowDemoWindow( &m_appData.guiData().m_showDemoWindow );
        }

        /*
        if ( true )
        {
            if ( ImGui::BeginMainMenuBar() )
            {
                const auto s = ImGui::GetWindowSize();
                const std::string sizeString = std::to_string(s.x) + ", " + std::to_string(s.y);

                if ( ImGui::BeginMenu( "File" ) )
                {
                    if ( ImGui::MenuItem( sizeString.c_str() ) )
                    {

                    }
                    ImGui::EndMenu();
                }

                if ( ImGui::BeginMenu( "Edit" ) )
                {
                    if ( ImGui::MenuItem( "Item" ) )
                    {

                    }
                    ImGui::EndMenu();
                }

                ImGui::EndMainMenuBar();
            }
        }
        */

        if ( m_appData.guiData().m_showSettingsWindow )
        {
            renderSettingsWindow(
                        m_appData,
                        getNumImageColorMaps,
                        getImageColorMap,
                        m_updateMetricUniforms,
                        m_recenterAllViews );
        }

        if ( m_appData.guiData().m_showInspectionWindow )
        {
            renderInspectionWindowWithTable(
                        m_appData,
                        getImageDisplayAndFileNames,
                        m_getSubjectPos,
                        m_getVoxelPos,
                        m_setSubjectPos,
                        m_setVoxelPos,
                        m_getImageValue,
                        m_getSegLabel,
                        getLabelTable );
        }

        if ( m_appData.guiData().m_showImagePropertiesWindow )
        {
            renderImagePropertiesWindow(
                        m_appData,
                        m_appData.numImages(),
                        getImageDisplayAndFileNames,
                        getActiveImageIndex,
                        setActiveImageIndex,
                        getNumImageColorMaps,
                        getImageColorMap,
                        moveImageBackward,
                        moveImageForward,
                        moveImageToBack,
                        moveImageToFront,
                        m_updateImageUniforms,
                        m_updateImageInterpolationMode,
                        m_setLockManualImageTransformation );
        }

        if ( m_appData.guiData().m_showSegmentationsWindow )
        {
            renderSegmentationPropertiesWindow(
                        m_appData,
                        getLabelTable,
                        m_updateImageUniforms,
                        m_updateLabelColorTableTexture,
                        m_createBlankSeg,
                        m_clearSeg,
                        m_removeSeg );
        }

        if ( m_appData.guiData().m_showLandmarksWindow )
        {
            renderLandmarkPropertiesWindow(
                        m_appData,
                        m_recenterAllViews );
        }

        if ( m_appData.guiData().m_showAnnotationsWindow )
        {
            renderAnnotationWindow(
                        m_appData,
                        setViewCameraDirection,
                        m_recenterAllViews );
        }

        if ( m_appData.guiData().m_showOpacityBlenderWindow )
        {
            renderOpacityBlenderWindow( m_appData, m_updateImageUniforms );
        }

        renderToolbar(
                    m_appData,
                    getMouseMode,
                    setMouseMode,
                    m_recenterAllViews,
                    m_getOverlayVisibility,
                    m_setOverlayVisibility,
                    cycleViewLayout,

                    m_appData.numImages(),
                    getImageDisplayAndFileNames,
                    getActiveImageIndex,
                    setActiveImageIndex );

        renderSegToolbar(
                    m_appData,
                    m_appData.numImages(),
                    getImageDisplayAndFileNames,
                    getActiveImageIndex,
                    setActiveImageIndex,
                    getImageHasActiveSeg,
                    setImageHasActiveSeg,
                    m_updateImageUniforms,
                    m_createBlankSeg,
                    m_executeGridCutsSeg );
    }


    Layout& currentLayout = m_appData.windowData().currentLayout();

    const float wholeWindowHeight = static_cast<float>( m_appData.windowData().getWindowSize().y );

    if ( m_appData.guiData().m_renderUiOverlays && currentLayout.isLightbox() )
    {
        // Per-layout UI controls:

        static constexpr bool sk_recenterCrosshairs = false;
        static constexpr bool sk_recenterOnCurrentCrosshairsPosition = false;
        static constexpr bool sk_resetObliqueOrientation = false;

        const auto mindowFrameBounds = camera::computeMindowFrameBounds(
            currentLayout.windowClipViewport(),
            m_appData.windowData().viewport().getAsVec4(),
            wholeWindowHeight );

        renderViewSettingsComboWindow(
                    currentLayout.uid(),
                    mindowFrameBounds,
                    currentLayout.uiControls(),
                    true,
                    false,

                    m_appData.numImages(),
                    [this, &currentLayout] ( size_t index ) { return currentLayout.isImageRendered( m_appData, index ); },
                    [this, &currentLayout] ( size_t index, bool visible ) { currentLayout.setImageRendered( m_appData, index, visible ); },

                    [this, &currentLayout] ( size_t index ) { return currentLayout.isImageUsedForMetric( m_appData, index ); },
                    [this, &currentLayout] ( size_t index, bool visible ) { currentLayout.setImageUsedForMetric( m_appData, index, visible ); },
                    getImageDisplayAndFileNames,

                    getImageIsVisibleSetting,

                    currentLayout.cameraType(),
                    currentLayout.renderMode(),
                    [&currentLayout] ( const camera::CameraType& cameraType ) { return currentLayout.setCameraType( cameraType ); },
                    [&currentLayout] ( const camera::ViewRenderMode& shaderType ) { return currentLayout.setRenderMode( shaderType ); },

                    [this]() { m_recenterAllViews( sk_recenterCrosshairs, sk_recenterOnCurrentCrosshairsPosition, sk_resetObliqueOrientation ); },
                    nullptr );

        renderViewOrientationToolWindow(
                    currentLayout.uid(),
                    mindowFrameBounds,
                    currentLayout.uiControls(),
                    true,
                    currentLayout.cameraType(),
                    [&getViewCameraRotation, &currentLayout] () { return getViewCameraRotation( currentLayout.uid() ); },
                    [&setViewCameraRotation, &currentLayout] ( const glm::quat& q ) { return setViewCameraRotation( currentLayout.uid(), q ); },
                    [&setViewCameraDirection, &currentLayout] ( const glm::vec3& dir ) { return setViewCameraDirection( currentLayout.uid(), dir ); },
                    [&getViewNormal, &currentLayout] () { return getViewNormal( currentLayout.uid() ); },
                    getObliqueViewDirections );
    }
    else if ( m_appData.guiData().m_renderUiOverlays && ! currentLayout.isLightbox() )
    {
        // Per-view UI controls:

        for ( const auto& viewUid : m_appData.windowData().currentViewUids() )
        {
            View* view = m_appData.windowData().getCurrentView( viewUid );
            if ( ! view ) return;

            auto setCameraType = [view] ( const camera::CameraType& cameraType )
            {
                if ( view ) view->setCameraType( cameraType );
            };

            auto setRenderMode = [view] ( const camera::ViewRenderMode& shaderType )
            {
                if ( view ) view->setRenderMode( shaderType );
            };

            auto recenter = [this, &viewUid] ()
            {
                m_recenterView( viewUid );
            };

            const auto mindowFrameBounds = camera::computeMindowFrameBounds(
                view->windowClipViewport(),
                m_appData.windowData().viewport().getAsVec4(),
                wholeWindowHeight );

            renderViewSettingsComboWindow(
                        viewUid,
                        mindowFrameBounds,
                        view->uiControls(),
                        false,
                        true,

                        m_appData.numImages(),
                        [this, view] ( size_t index ) { return view->isImageRendered( m_appData, index ); },
                        [this, view] ( size_t index, bool visible ) { view->setImageRendered( m_appData, index, visible ); },

                        [this, view] ( size_t index ) { return view->isImageUsedForMetric( m_appData, index ); },
                        [this, view] ( size_t index, bool visible ) { view->setImageUsedForMetric( m_appData, index, visible ); },

                        getImageDisplayAndFileNames,
                        getImageIsVisibleSetting,

                        view->cameraType(),
                        view->renderMode(),
                        setCameraType,
                        setRenderMode,

                        recenter,
                        applyImageSelectionAndShaderToAllViews );

            renderViewOrientationToolWindow(
                    viewUid,
                    mindowFrameBounds,
                    view->uiControls(),
                    false,
                    view->cameraType(),
                    [&getViewCameraRotation, &viewUid] () { return getViewCameraRotation( viewUid ); },
                    [&setViewCameraRotation, &viewUid] ( const glm::quat& q ) { return setViewCameraRotation( viewUid, q ); },
                    [&setViewCameraDirection, &viewUid] ( const glm::vec3& dir ) { return setViewCameraDirection( viewUid, dir ); },
                    [&getViewNormal, &viewUid] () { return getViewNormal( viewUid ); },
                    getObliqueViewDirections );
        }
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
}
