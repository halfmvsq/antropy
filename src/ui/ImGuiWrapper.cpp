#include "ui/ImGuiWrapper.h"

#include "ui/Helpers.h"
#include "ui/Style.h"
#include "ui/Toolbars.h"
#include "ui/Widgets.h"
#include "ui/Windows.h"

#include "logic/app/Data.h"

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


ImGuiWrapper::ImGuiWrapper( GLFWwindow* window, AppData& appData )
    :
      m_appData( appData ),

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

    // This is the standard ImGui dark style:
//    ImGui::StyleColorsDark();

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
        std::function< void ( bool recenterCrosshairs, bool recenterOnCurrentCrosshairsPosition )> recenterCurrentViews,
        std::function< bool ( void ) > getOverlayVisibility,
        std::function< void ( bool ) > setOverlayVisibility,
        std::function< void ( const uuids::uuid& viewUid )> updateImageUniforms,
        std::function< void ( const uuids::uuid& viewUid )> updateImageInterpolationMode,
        std::function< void ( size_t tableIndex ) > updateLabelColorTableTexture,
        std::function< void ()> updateMetricUniforms,
        std::function< glm::vec3 () > getWorldDeformedPos,
        std::function< std::optional<glm::vec3> ( size_t imageIndex ) > getSubjectPos,
        std::function< std::optional<glm::ivec3> ( size_t imageIndex ) > getVoxelPos,
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
    ImGuiIO& io = ImGui::GetIO();
    //    io.WantCaptureMouse = false;
    //    io.WantCaptureKeyboard = true;
    //    io.WantTextInput = true;

    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
//    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
//    io.ConfigFlags |= ImGuiConfigFlags_NoMouse;

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
//    io.Fonts->AddFontDefault();

//    auto font_default = io.Fonts->AddFontDefault();

//    m_guiData.font_cousine = io.Fonts->AddFontFromFileTTF( "../externals/imgui/misc/fonts/Cousine-Regular.ttf", 15.0f);
//    m_guiData.font_karla   = io.Fonts->AddFontFromFileTTF( "../externals/imgui/misc/fonts/Karla-Regular.ttf", 18.0f);

    {
        auto filesystem = cmrc::fonts::get_filesystem();

        cmrc::file cousineFont = filesystem.open( "resources/fonts/Cousine/Cousine-Regular.ttf" );

        // ImGui will take ownership of the font, so make a copy.
        char* fontCousineCopy = new char[cousineFont.size()];
        for ( size_t i = 0; i < cousineFont.size(); ++i )
        {
            fontCousineCopy[i] = cousineFont.begin()[i];
        }

        /// @see For details about Fork Awesome fonts:
        /// https://forkaweso.me/Fork-Awesome/icons/
#if 0
        std::ostringstream ssFontsAwesomePath;
        ssFontsAwesomePath << "resources/fonts/FontAwesome/" << FONT_ICON_FILE_NAME_FAS; // FAR
        cmrc::file fontAwesomeSolid = filesystem.open( ssFontsAwesomePath.str() );

        char* fontAwesomeCopy = new char[fontAwesomeSolid.size()];
        for ( size_t i = 0; i < fontAwesomeSolid.size(); ++i )
        {
            fontAwesomeCopy[i] = fontAwesomeSolid.begin()[i];
        }
#endif

        std::ostringstream ssForkAwesomePath;
        ssForkAwesomePath << "resources/fonts/ForkAwesome/" << FONT_ICON_FILE_NAME_FK;
        cmrc::file forkAwesomeWeb = filesystem.open( ssForkAwesomePath.str() );

        char* forkAwesomeCopy = new char[forkAwesomeWeb.size()];
        for ( size_t i = 0; i < forkAwesomeWeb.size(); ++i )
        {
            forkAwesomeCopy[i] = forkAwesomeWeb.begin()[i];
        }

        static ImFontConfig cousineFontConfig;
        myImFormatString( cousineFontConfig.Name, IM_ARRAYSIZE(cousineFontConfig.Name), "%s, %.0fpx", "Cousine", 15.0 );

//         NB: Transfer ownership of 'ttf_data' to ImFontAtlas, unless font_cfg_template->FontDataOwnedByAtlas == false.
//         Owned TTF buffer will be deleted after Build().

//        spdlog::critical( "name = {}", cousineFontConfig.Name );

        m_appData.guiData().m_cousineFont = io.Fonts->AddFontFromMemoryTTF(
                    static_cast<void*>( fontCousineCopy ),
                    static_cast<int32_t>( cousineFont.size() ),
                    15.0f, &cousineFontConfig );

//        if ( m_appData.guiData().cousineFont )
//        {
//            spdlog::info( "Loaded cousineFont: {}", m_appData.guiData().cousineFont->IsLoaded() );
//        }
//        else
//        {
//            spdlog::warn( "Did not load cousineFont" );
//        }

#if 0
        // merge in icons from Font Awesome
        static const ImWchar fontsAwesomeIconRange[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
        ImFontConfig fontsAwesomeConfig;
        fontsAwesomeConfig.MergeMode = true;
        fontsAwesomeConfig.PixelSnapH = true;
        MyImFormatString( fontsAwesomeConfig.Name, IM_ARRAYSIZE(fontsAwesomeConfig.Name), "%s, %.0fpx", "Font Awesome 5 Solid", 15.0 );

        io.Fonts->AddFontFromMemoryTTF(
                    static_cast<void*>( fontAwesomeCopy ),
                    static_cast<int32_t>( fontAwesomeSolid.size() ),
                    15.0f, &fontsAwesomeConfig, fontsAwesomeIconRange );
#endif

        // merge in icons from Fork Awesome
        static const ImWchar forkAwesomeIconRange[] = { ICON_MIN_FK, ICON_MAX_FK, 0 };
        ImFontConfig forkAwesomeConfig;
        forkAwesomeConfig.MergeMode = true;
        forkAwesomeConfig.PixelSnapH = true;
        myImFormatString( forkAwesomeConfig.Name, IM_ARRAYSIZE(forkAwesomeConfig.Name), "%s, %.0fpx", "Fork Awesome", 15.0 );

        m_appData.guiData().m_forkAwesomeFont =
                io.Fonts->AddFontFromMemoryTTF(
                    static_cast<void*>( forkAwesomeCopy ),
                    static_cast<int32_t>( forkAwesomeWeb.size() ),
                    15.0f, &forkAwesomeConfig, forkAwesomeIconRange );
    }

//    if ( m_appData.guiData().forkAwesomeFont )
//    {
//        spdlog::info( "Loaded forkAwesomeFont: {}", m_appData.guiData().forkAwesomeFont->IsLoaded() );
//    }
//    else
//    {
//        spdlog::warn( "Did not load forkAwesomeFont" );
//    }

    //    std::cout << font_cousine->IsLoaded() << std::endl;
    //    std::cout << font_karla->IsLoaded() << std::endl;

    spdlog::debug( "Initialized ImGui data" );
}


void ImGuiWrapper::render()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    auto getNumImages = [this] ()
    {
        return m_appData.numImages();
    };

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

    auto recenterAllViews = [this] ()
    {
        static constexpr bool s_recenterCrosshairs = true;
        static constexpr bool s_recenterOnCurrentCrosshairsPosition = false;
        m_recenterAllViews( s_recenterCrosshairs, s_recenterOnCurrentCrosshairsPosition );
    };

    auto recenterAllViewsOnCurrentCrosshairsPosition = [this] ( bool recenterOnCurrentCrosshairsPosition )
    {
        static constexpr bool s_recenterCrosshairs = false;
        m_recenterAllViews( s_recenterCrosshairs, recenterOnCurrentCrosshairsPosition );
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


    ImGui::NewFrame();

    if ( m_appData.guiData().m_renderUiWindows )
    {
        if ( m_appData.guiData().m_showDemoWindow )
        {
            ImGui::ShowDemoWindow( &m_appData.guiData().m_showDemoWindow );
        }

        if ( m_appData.guiData().m_showSettingsWindow )
        {
            renderSettingsWindow(
                        m_appData,
                        getNumImageColorMaps,
                        getImageColorMap,
                        m_updateMetricUniforms,
                        recenterAllViews );
        }

        if ( m_appData.guiData().m_showInspectionWindow )
        {
            renderInspectionWindowWithTable(
                        m_appData,
                        getImageDisplayAndFileNames,
                        m_getSubjectPos,
                        m_getVoxelPos,
                        m_getImageValue,
                        m_getSegLabel,
                        getLabelTable );
        }

        if ( m_appData.guiData().m_showImagePropertiesWindow )
        {
            renderImagePropertiesWindow(
                        m_appData,
                        getNumImages,
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
                        recenterAllViewsOnCurrentCrosshairsPosition );
        }

        if ( m_appData.guiData().m_showAnnotationsWindow )
        {
            renderAnnotationWindow(
                        m_appData,
                        recenterAllViewsOnCurrentCrosshairsPosition );
        }

        if ( m_appData.guiData().m_showOpacityBlenderWindow )
        {
            renderOpacityBlenderWindow( m_appData, m_updateImageUniforms );
        }

        renderToolbar(
                    m_appData,
                    getMouseMode,
                    setMouseMode,
                    recenterAllViews,
                    m_getOverlayVisibility,
                    m_setOverlayVisibility,
                    cycleViewLayout,

                    getNumImages,
                    getImageDisplayAndFileNames,
                    getActiveImageIndex,
                    setActiveImageIndex );

        renderSegToolbar(
                    m_appData,
                    getNumImages,
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

    if ( m_appData.guiData().m_renderUiOverlays && currentLayout.isLightbox() )
    {
        // Per-layout UI controls:

        static constexpr bool s_recenterCrosshairs = false;
        static constexpr bool s_recenterOnCurrentCrosshairsPosition = false;

        renderViewSettingsComboWindow(
                    currentLayout.uid(),
                    currentLayout.winMouseMinMaxCoords(),
                    currentLayout.uiControls(),
                    true,
                    false,

                    getNumImages,
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

                    [this]() { m_recenterAllViews( s_recenterCrosshairs, s_recenterOnCurrentCrosshairsPosition ); },
                    nullptr );
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

            renderViewSettingsComboWindow(
                        viewUid,
                        view->winMouseMinMaxCoords(),
                        view->uiControls(),
                        false,
                        true,

                        getNumImages,
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
        }
    }


    /// @todo Add "Simple overlay" from Examples menu. Use it to show mouse position, image value, seg value etc.

    // 4 Render a texture
    //        ImGui::Begin("GameWindow");
    //        {
    //          // Using a Child allow to fill all the space of the window.
    //          // It also alows customization
    //          ImGui::BeginChild("GameRender");
    //          // Get the size of the child (i.e. the whole draw size of the windows).
    //          ImVec2 wsize = ImGui::GetWindowSize();
    //          // Because I use the texture from OpenGL, I need to invert the V from the UV.
    //          ImGui::Image((ImTextureID)tex, wsize, ImVec2(0, 1), ImVec2(1, 0));
    //          ImGui::EndChild();
    //        }
    //        ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
}
