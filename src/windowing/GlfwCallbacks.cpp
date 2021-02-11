#include "windowing/GlfwCallbacks.h"

#include "AntropyApp.h"
#include "common/Types.h"

#include "logic/camera/CameraHelpers.h"
#include "logic/interaction/events/ButtonState.h"

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <imgui/imgui.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <optional>


namespace
{

static ButtonState s_mouseButtonState;
static ModifierState s_modifierState;

static std::optional<glm::vec2> s_lastWinPos;
static std::optional<glm::vec2> s_startWinPos;

}


void errorCallback( int error, const char* description )
{
    spdlog::error( "GLFW error #{}: '{}'", error, description );
}


void windowContentScaleCallback( GLFWwindow* window, float fbToWinScaleX, float fbToWinScaleY )
{
    auto app = reinterpret_cast<AntropyApp*>( glfwGetWindowUserPointer( window ) );
    if ( ! app )
    {
        spdlog::warn( "App is null in window content scale callback" );
        return;
    }

    app->windowData().setDeviceScaleRatio( glm::vec2{ fbToWinScaleX, fbToWinScaleY } );
}


void windowCloseCallback( GLFWwindow* window )
{
    glfwSetWindowShouldClose( window, GLFW_TRUE );
}


void windowSizeCallback( GLFWwindow* window, int winWidth, int winHeight )
{
    auto app = reinterpret_cast<AntropyApp*>( glfwGetWindowUserPointer( window ) );
    if ( ! app )
    {
        spdlog::warn( "App is null in window size callback" );
        return;
    }

//    float xscale, yscale;
//    glfwGetWindowContentScale( window, &xscale, &yscale );
//    app->windowData().setDeviceScaleRatio( { xscale, yscale } );

    app->resize( winWidth, winHeight );
    app->render();

    // The app sometimes crashes on macOS without this call
    glfwSwapBuffers( window );
}


void cursorPosCallback( GLFWwindow* window, double mousePosX, double mousePosY )
{
    auto app = reinterpret_cast<AntropyApp*>( glfwGetWindowUserPointer( window ) );
    if ( ! app )
    {
        spdlog::warn( "App is null in cursor position callback" );
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();

    if ( io.WantCaptureMouse )
    {
        // Poll events, so that the UI is responsive
        app->glfw().setEventProcessingMode( EventProcessingMode::Poll );
        return; // ImGui has captured event
    }
    else if ( ! app->appData().settings().animating() )
    {
        // Mouse is not captured by the UI, and the app is not animating,
        // so wait for events to save processing
        app->glfw().setEventProcessingMode( EventProcessingMode::Wait );
    }

    const glm::vec2 currWinPos = camera::view_T_mouse(
                app->windowData().viewport(), { mousePosX, mousePosY } );

    if ( ! s_lastWinPos )
    {
        s_lastWinPos = currWinPos;
    }

    if ( ! s_startWinPos )
    {
        s_startWinPos = currWinPos;
    }

    CallbackHandler& handler = app->callbackHandler();

    switch ( app->appData().settings().mouseMode() )
    {
    case MouseMode::Pointer:
    {
        if ( s_mouseButtonState.left )
        {
            handler.doCrosshairsMove( *s_lastWinPos, currWinPos );
        }
        else if ( s_mouseButtonState.right )
        {
            const bool syncZoomsForAllViews = s_modifierState.shift;

            handler.doCameraZoomDrag( *s_lastWinPos, currWinPos, *s_startWinPos,
                                      ZoomBehavior::ToCrosshairs, syncZoomsForAllViews );
        }
        else if ( s_mouseButtonState.middle )
        {
            handler.doCameraTranslate2d( *s_lastWinPos, currWinPos, *s_startWinPos );
        }
        break;
    }
    case MouseMode::Segment:
    {
        if ( s_mouseButtonState.left )
        {
            if ( app->appData().settings().crosshairsMoveWithBrush() )
            {
                handler.doCrosshairsMove( *s_lastWinPos, currWinPos );
            }

            handler.doSegment( *s_lastWinPos, currWinPos, true );
        }
        else if ( s_mouseButtonState.right )
        {
            if ( app->appData().settings().crosshairsMoveWithBrush() )
            {
                handler.doCrosshairsMove( *s_lastWinPos, currWinPos );
            }

            handler.doSegment( *s_lastWinPos, currWinPos, false );
        }
        break;
    }
    case MouseMode::Annotate:
    {
        break;
    }
    case MouseMode::WindowLevel:
    {
        if ( s_mouseButtonState.left )
        {
            handler.doWindowLevel( *s_lastWinPos, currWinPos );
        }
        else if ( s_mouseButtonState.right )
        {
            handler.doOpacity( *s_lastWinPos, currWinPos );
        }
        break;
    }
    case MouseMode::CameraZoom:
    {
        if ( s_mouseButtonState.left )
        {
            const bool syncZoomsForAllViews = s_modifierState.shift;

            handler.doCameraZoomDrag( *s_lastWinPos, currWinPos, *s_startWinPos,
                                      ZoomBehavior::ToCrosshairs, syncZoomsForAllViews );
        }
        else if ( s_mouseButtonState.right )
        {
            const bool syncZoomsForAllViews = s_modifierState.shift;

            handler.doCameraZoomDrag( *s_lastWinPos, currWinPos, *s_startWinPos,
                                   ZoomBehavior::ToStartPosition, syncZoomsForAllViews );
        }
        else if ( s_mouseButtonState.middle )
        {
            handler.doCameraTranslate2d( *s_lastWinPos, currWinPos, *s_startWinPos );
        }
        break;
    }
    case MouseMode::CameraTranslate2D:
    {
        if ( s_mouseButtonState.left )
        {
            handler.doCameraTranslate2d( *s_lastWinPos, currWinPos, *s_startWinPos );
        }
        break;
    }
    case MouseMode::ImageTranslate:
    {
        if ( s_mouseButtonState.left )
        {
            handler.doImageTranslate( *s_lastWinPos, currWinPos, *s_startWinPos, true );
        }
        else if ( s_mouseButtonState.right )
        {
            handler.doImageTranslate( *s_lastWinPos, currWinPos, *s_startWinPos, false );
        }
        break;
    }
    case MouseMode::ImageRotate:
    {
        if ( s_mouseButtonState.left )
        {
            handler.doImageRotate( *s_lastWinPos, currWinPos, *s_startWinPos, true );
        }
        else if ( s_mouseButtonState.right )
        {
            handler.doImageRotate( *s_lastWinPos, currWinPos, *s_startWinPos, false );
        }
        break;
    }
    case MouseMode::ImageScale:
    {
        if ( s_mouseButtonState.left )
        {
            const bool constrainIsotropic = s_modifierState.shift;
            handler.doImageScale( *s_lastWinPos, currWinPos, *s_startWinPos, constrainIsotropic );
        }
        break;
    }
    }

    s_lastWinPos = currWinPos;
}


void mouseButtonCallback( GLFWwindow* window, int button, int action, int mods )
{
    const ImGuiIO& io = ImGui::GetIO();
    if ( io.WantCaptureMouse ) return; // ImGui has captured event

    s_mouseButtonState.updateFromGlfwEvent( button, action );
    s_modifierState.updateFromGlfwEvent( mods );

    s_lastWinPos = std::nullopt;
    s_startWinPos = std::nullopt;

    auto app = reinterpret_cast<AntropyApp*>( glfwGetWindowUserPointer( window ) );
    if ( ! app )
    {
        spdlog::warn( "App is null in mouse button callback" );
        return;
    }   

    switch ( action )
    {
    case GLFW_PRESS:
    {
        double mousePosX, mousePosY;
        glfwGetCursorPos( window, &mousePosX, &mousePosY );
        cursorPosCallback( window, mousePosX, mousePosY );
        break;
    }
    case GLFW_RELEASE:
    {
        // app->appData().setMouseMode( MouseMode::Nothing );
        app->appData().windowData().setActiveViewUid( std::nullopt );
        break;
    }
    default: break;
    }
}


void scrollCallback( GLFWwindow* window, double scrollOffsetX, double scrollOffsetY )
{
    const ImGuiIO& io = ImGui::GetIO();
    if ( io.WantCaptureMouse ) return; // ImGui has captured event

    auto app = reinterpret_cast<AntropyApp*>( glfwGetWindowUserPointer( window ) );
    if ( ! app )
    {
        spdlog::warn( "App is null in scroll callback" );
        return;
    }

    double mousePosX, mousePosY;
    glfwGetCursorPos( window, &mousePosX, &mousePosY );
    cursorPosCallback( window, mousePosX, mousePosY );

    CallbackHandler& handler = app->callbackHandler();

    const glm::vec2 winPos = camera::view_T_mouse( app->windowData().viewport(), { mousePosX, mousePosY } );

    switch ( app->appData().settings().mouseMode() )
    {
    case MouseMode::Pointer:
    case MouseMode::Segment:
    case MouseMode::Annotate:
    case MouseMode::CameraTranslate2D:
    case MouseMode::ImageRotate:
    case MouseMode::ImageTranslate:
    case MouseMode::ImageScale:
    case MouseMode::WindowLevel:
    {
        handler.doCrosshairsScroll( winPos, { scrollOffsetX, scrollOffsetY } );
        break;
    }
    case MouseMode::CameraZoom:
    {
        const bool syncZoomsForAllViews = s_modifierState.shift;

        handler.doCameraZoomScroll( { scrollOffsetX, scrollOffsetY }, winPos,
                                    ZoomBehavior::ToCrosshairs, syncZoomsForAllViews );
        break;
    }
    }
}


void keyCallback( GLFWwindow* window, int key, int /*scancode*/, int action, int mods )
{
    const ImGuiIO& io = ImGui::GetIO();
    if ( io.WantCaptureKeyboard ) return; // ImGui has captured event

    s_modifierState.updateFromGlfwEvent( mods );

    auto app = reinterpret_cast<AntropyApp*>( glfwGetWindowUserPointer( window ) );

    if ( ! app )
    {
        spdlog::warn( "App is null in key callback" );
        return;
    }

    // Do actions on GLFW_PRESS and GLFW_REPEAT only
    if ( GLFW_RELEASE == action ) return;

    const Viewport& viewport = app->windowData().viewport();

    double mousePosX, mousePosY;
    glfwGetCursorPos( window, &mousePosX, &mousePosY );

    CallbackHandler& handler = app->callbackHandler();

    switch ( key )
    {
    case GLFW_KEY_Q:
    {
        if ( s_modifierState.control )
        {
            glfwSetWindowShouldClose( window, true );
        }
        break;
    }

    case GLFW_KEY_V: handler.setMouseMode( MouseMode::Pointer ); break;
    case GLFW_KEY_B: handler.setMouseMode( MouseMode::Segment ); break;
    case GLFW_KEY_L: handler.setMouseMode( MouseMode::WindowLevel ); break;

    case GLFW_KEY_R: handler.setMouseMode( MouseMode::ImageRotate ); break;
    case GLFW_KEY_T: handler.setMouseMode( MouseMode::ImageTranslate ); break;
//    case GLFW_KEY_Y: handler.setMouseMode( MouseMode::ImageScale ); break;

    case GLFW_KEY_Z: handler.setMouseMode( MouseMode::CameraZoom ); break;
    case GLFW_KEY_X: handler.setMouseMode( MouseMode::CameraTranslate2D ); break;

    case GLFW_KEY_A: handler.decreaseSegOpacity(); break;
    case GLFW_KEY_S: handler.toggleSegVisibility(); break;
    case GLFW_KEY_D: handler.increaseSegOpacity(); break;

    case GLFW_KEY_W: handler.toggleImageVisibility(); break;
    case GLFW_KEY_E: handler.toggleImageEdges(); break;
    case GLFW_KEY_O: handler.cycleOverlayAndUiVisibility(); break;

    case GLFW_KEY_C: handler.recenterViews( ImageSelection::AllLoadedImages, true, false ); break;

    case GLFW_KEY_PAGE_DOWN:
    {
        cursorPosCallback( window, mousePosX, mousePosY );

        if ( s_modifierState.shift )
        {
            handler.cycleImageComponent( -1 );
        }
        else
        {
            const glm::vec2 winPos = camera::view_T_mouse( viewport, { mousePosX, mousePosY } );
            handler.scrollViewSlice( winPos, -1 );
        }

        break;
    }
    case GLFW_KEY_PAGE_UP:
    {
        cursorPosCallback( window, mousePosX, mousePosY );

        if ( s_modifierState.shift )
        {
            handler.cycleImageComponent( 1 );
        }
        else
        {
            const glm::vec2 winPos = camera::view_T_mouse( viewport, { mousePosX, mousePosY } );
            handler.scrollViewSlice( winPos, 1 );
        }

        break;
    }
    case GLFW_KEY_LEFT:
    {
        cursorPosCallback( window, mousePosX, mousePosY );

        const glm::vec2 winPos = camera::view_T_mouse( viewport, { mousePosX, mousePosY } );
        handler.moveCrosshairsOnViewSlice( winPos, -1, 0 );
        break;
    }
    case GLFW_KEY_RIGHT:
    {
        cursorPosCallback( window, mousePosX, mousePosY );

        const glm::vec2 winPos = camera::view_T_mouse( viewport, { mousePosX, mousePosY } );
        handler.moveCrosshairsOnViewSlice( winPos, 1, 0 );
        break;
    }
    case GLFW_KEY_UP:
    {
        cursorPosCallback( window, mousePosX, mousePosY );

        const glm::vec2 winPos = camera::view_T_mouse( viewport, { mousePosX, mousePosY } );
        handler.moveCrosshairsOnViewSlice( winPos, 0, 1 );
        break;
    }
    case GLFW_KEY_DOWN:
    {
        cursorPosCallback( window, mousePosX, mousePosY );

        const glm::vec2 winPos = camera::view_T_mouse( viewport, { mousePosX, mousePosY } );
        handler.moveCrosshairsOnViewSlice( winPos, 0, -1 );
        break;
    }

    case GLFW_KEY_LEFT_BRACKET:
    {
        handler.cyclePrevLayout();
        break;
    }
    case GLFW_KEY_RIGHT_BRACKET:
    {
        handler.cycleNextLayout();
        break;
    }
    case GLFW_KEY_COMMA:
    {
        if ( s_modifierState.shift )
        {
            handler.cycleBackgroundSegLabel( -1 );
        }
        else
        {
            handler.cycleForegroundSegLabel( -1 );
        }
        break;
    }
    case GLFW_KEY_PERIOD:
    {
        if ( s_modifierState.shift )
        {
            handler.cycleBackgroundSegLabel( 1 );
        }
        else
        {
            handler.cycleForegroundSegLabel( 1 );
        }
        break;
    }

    case GLFW_KEY_KP_ADD:
    case GLFW_KEY_EQUAL:
    {
        handler.cycleBrushSize( 1 );
        break;
    }

    case GLFW_KEY_KP_SUBTRACT:
    case GLFW_KEY_MINUS:
    {
        handler.cycleBrushSize( -1 );
        break;
    }

        /*
        case GLFW_KEY_ENTER:
        {
            const auto mouseMode = app->appData().settings().mouseMode();

            if ( MouseMode::Pointer == mouseMode ||
                 MouseMode::ImageRotate == mouseMode ||
                 MouseMode::ImageScale == mouseMode )
            {
                app->appData().settings().setWorldRotationCenter(
                            app->appData().settings().worldCrosshairs().worldOrigin() );
            }
            break;
        }

        case GLFW_KEY_SPACE:
        {
            const auto mouseMode = app->appData().settings().mouseMode();

            if ( MouseMode::Pointer == mouseMode ||
                 MouseMode::ImageRotate == mouseMode ||
                 MouseMode::ImageScale == mouseMode )
            {
                app->appData().settings().setWorldRotationCenter( std::nullopt );
            }
            break;
        }
        */

        /*
        case GLFW_KEY_C:
        {
            if ( GLFW_MOD_CONTROL & mods )
            {
                glfwSetClipboardString( nullptr, "A string with words in it" );
            }
            break;
        }

        case GLFW_KEY_V:
        {
            if ( GLFW_MOD_CONTROL & mods )
            {
                const char* text = glfwGetClipboardString( nullptr );
            }
            break;
        }
        */
    }
}


void dropCallback( GLFWwindow* window, int count, const char** paths )
{
    if ( 0 == count || ! paths ) return;

    auto app = reinterpret_cast<AntropyApp*>( glfwGetWindowUserPointer( window ) );
    if ( ! app )
    {
        spdlog::warn( "App is null in drop callback" );
        return;
    }

    for ( int i = 0;  i < count; ++i )
    {
        if ( paths[i] )
        {
            spdlog::info( "Dropped file {}: {}", i, paths[i] );

            /// @todo Could attempt to load the dropped image:
//            app->loadImage( paths[i], false );
        }
    }
}
