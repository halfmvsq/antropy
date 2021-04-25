#include "windowing/GlfwCallbacks.h"

#include "AntropyApp.h"
#include "common/Types.h"

#include "logic/camera/CameraHelpers.h"
#include "logic/interaction/ViewHit.h"
#include "logic/interaction/events/ButtonState.h"
#include "logic/states/FsmList.hpp"

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

// The last cursor position in Window space
static std::optional<ViewHit> s_prevHit;

// The start cursor position in Window space: where the cursor was clicked prior to dragging
static std::optional<ViewHit> s_startHit;

// Should zooms be synchronized for all views?
bool syncZoomsForAllViews( const ModifierState& modState )
{
    return ( modState.shift );
}

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

void windowPositionCallback( GLFWwindow* window, int screenWindowPosX, int screenWindowPosY )
{
    auto app = reinterpret_cast<AntropyApp*>( glfwGetWindowUserPointer( window ) );
    if ( ! app )
    {
        spdlog::warn( "App is null in window size callback" );
        return;
    }

    // Save the window position. This does not affect rendering at all, so no render is required.
    app->windowData().setWindowPos( screenWindowPosX, screenWindowPosY );
}

void windowSizeCallback( GLFWwindow* window, int windowWidth, int windowHeight )
{
    auto app = reinterpret_cast<AntropyApp*>( glfwGetWindowUserPointer( window ) );
    if ( ! app )
    {
        spdlog::warn( "App is null in window size callback" );
        return;
    }

    app->resize( windowWidth, windowHeight );
    app->render();

    // The app sometimes crashes on macOS without this call
    glfwSwapBuffers( window );
}

void cursorPosCallback( GLFWwindow* window, double mindowCursorPosX, double mindowCursorPosY )
{
    static constexpr bool inPlane = true;
    static constexpr bool outOfPlane = false;

    auto app = reinterpret_cast<AntropyApp*>( glfwGetWindowUserPointer( window ) );
    if ( ! app )
    {
        spdlog::warn( "App is null in cursor position callback" );
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();

    if ( io.WantCaptureMouse )
    {
        // Poll events, so that the UI is responsive:
        app->glfw().setEventProcessingMode( EventProcessingMode::Poll );

        // Since ImGui has captured the event, do not send it to the app:
        return;
    }
    else if ( ! app->appData().state().animating() )
    {
        // Mouse is not captured by the UI and the app is not animating,
        // so wait for events to save processing power:
        app->glfw().setEventProcessingMode( EventProcessingMode::Wait );
    }

    const glm::vec2 windowCurrentPos = camera::window_T_mindow(
                static_cast<float>( app->windowData().getWindowSize().y ),
                { mindowCursorPosX, mindowCursorPosY } );

    if ( ! s_startHit )
    {
        s_startHit = getViewHit( app->appData(), windowCurrentPos );
    }

    if ( ! s_prevHit )
    {
        s_prevHit = getViewHit( app->appData(), windowCurrentPos );
    }

    // Compute current hit based on the transformation of the starting view.
    // This preserves continuity between previous and current coordinates.
    // It also allows the hit to be valid outside of a view.
    const auto currHit_withOverride =
            getViewHit( app->appData(), windowCurrentPos, s_startHit->viewUid );

    // Compute current hit without any override provided. This prevents the
    // hit from being valid if the cursor hits outside of a view.
    const auto currHit_invalidOutsideView =
            getViewHit( app->appData(), windowCurrentPos );

    // Send event to annotation state machine
    send_event( state::MouseMoveEvent( *currHit_invalidOutsideView ) );

    CallbackHandler& handler = app->callbackHandler();

    switch ( app->appData().state().mouseMode() )
    {
    case MouseMode::Pointer:
    {
        if ( s_mouseButtonState.left )
        {
            if ( ! currHit_invalidOutsideView ) break;
            handler.doCrosshairsMove( *currHit_invalidOutsideView );
        }
        else if ( s_mouseButtonState.right )
        {
            if ( ! currHit_withOverride ) break;
            handler.doCameraZoomDrag(
                        *s_startHit, *s_prevHit, *currHit_withOverride,
                        ZoomBehavior::ToCrosshairs,
                        syncZoomsForAllViews( s_modifierState ) );
        }
        else if ( s_mouseButtonState.middle )
        {
            if ( ! currHit_withOverride ) break;
            handler.doCameraTranslate2d( *s_startHit, *s_prevHit, *currHit_withOverride );
        }
        break;
    }
    case MouseMode::Segment:
    {
        if ( ! currHit_invalidOutsideView ) break;

        if ( s_mouseButtonState.left || s_mouseButtonState.right )
        {
            if ( app->appData().settings().crosshairsMoveWithBrush() )
            {
                handler.doCrosshairsMove( *currHit_invalidOutsideView );
            }

            const bool swapFgAndBg = ( s_mouseButtonState.right );
            handler.doSegment( *currHit_invalidOutsideView, swapFgAndBg );
        }

        break;
    }
    case MouseMode::Annotate:
    {       
        if ( ! currHit_invalidOutsideView ) break;

        if ( s_mouseButtonState.left )
        {
            if ( app->appData().settings().crosshairsMoveWithAnnotationPointCreation() )
            {
                handler.doCrosshairsMove( *currHit_invalidOutsideView );
            }

            handler.doAnnotate( *currHit_invalidOutsideView );
        }
        break;
    }
    case MouseMode::WindowLevel:
    {
        if ( ! currHit_withOverride ) break;

        if ( s_mouseButtonState.left )
        {
            handler.doWindowLevel( *s_prevHit, *currHit_withOverride );
        }
        else if ( s_mouseButtonState.right )
        {
            handler.doOpacity( *s_prevHit, *currHit_withOverride );
        }
        break;
    }
    case MouseMode::CameraZoom:
    {
        if ( ! currHit_withOverride ) break;

        if ( s_mouseButtonState.left )
        {           
            handler.doCameraZoomDrag(
                        *s_startHit, *s_prevHit, *currHit_withOverride,
                        ZoomBehavior::ToCrosshairs,
                        syncZoomsForAllViews( s_modifierState ) );
        }
        else if ( s_mouseButtonState.right )
        {
            handler.doCameraZoomDrag(
                        *s_startHit, *s_prevHit, *currHit_withOverride,
                        ZoomBehavior::ToStartPosition,
                        syncZoomsForAllViews( s_modifierState ) );
        }
        else if ( s_mouseButtonState.middle )
        {
            handler.doCameraTranslate2d( *s_startHit, *s_prevHit, *currHit_withOverride );
        }
        break;
    }
    case MouseMode::CameraTranslate:
    {
        if ( ! currHit_withOverride ) break;

        if ( s_mouseButtonState.left )
        {
            handler.doCameraTranslate2d( *s_startHit, *s_prevHit, *currHit_withOverride );
        }
        else if ( s_mouseButtonState.right )
        {
            // do 3D translate
        }
        break;
    }
    case MouseMode::CameraRotate:
    {
        if ( ! currHit_withOverride ) break;

        if ( s_mouseButtonState.left )
        {
            handler.doCameraRotate2d( *s_startHit, *s_prevHit, *currHit_withOverride );
        }
        else if ( s_mouseButtonState.right )
        {
            if ( s_modifierState.shift )
            {
                handler.doCameraRotate3d( *s_startHit, *s_prevHit, *currHit_withOverride,
                                          AxisConstraint::X );
            }
            else if ( s_modifierState.control )
            {
                handler.doCameraRotate3d( *s_startHit, *s_prevHit, *currHit_withOverride,
                                          AxisConstraint::Y );
            }
            else if ( s_modifierState.alt )
            {
                handler.doCameraRotate2d( *s_startHit, *s_prevHit, *currHit_withOverride );
            }
            else
            {
                handler.doCameraRotate3d( *s_startHit, *s_prevHit, *currHit_withOverride,
                                          AxisConstraint::None );
            }
        }
        break;
    }
    case MouseMode::ImageTranslate:
    {
        if ( ! currHit_withOverride ) break;

        if ( s_mouseButtonState.left )
        {
            handler.doImageTranslate( *s_startHit, *s_prevHit, *currHit_withOverride, inPlane );
        }
        else if ( s_mouseButtonState.right )
        {
            handler.doImageTranslate( *s_startHit, *s_prevHit, *currHit_withOverride, outOfPlane );
        }
        break;
    }
    case MouseMode::ImageRotate:
    {
        if ( ! currHit_withOverride ) break;

        if ( s_mouseButtonState.left )
        {
            handler.doImageRotate( *s_startHit, *s_prevHit, *currHit_withOverride, inPlane );
        }
        else if ( s_mouseButtonState.right )
        {
            handler.doImageRotate( *s_startHit, *s_prevHit, *currHit_withOverride, outOfPlane );
        }
        break;
    }
    case MouseMode::ImageScale:
    {
        if ( ! currHit_withOverride ) break;

        if ( s_mouseButtonState.left )
        {
            const bool constrainIsotropic = s_modifierState.shift;
            handler.doImageScale( *s_startHit, *s_prevHit, *currHit_withOverride, constrainIsotropic );
        }
        break;
    }
    }

    s_prevHit = currHit_withOverride;
}

void mouseButtonCallback( GLFWwindow* window, int button, int action, int mods )
{
    auto app = reinterpret_cast<AntropyApp*>( glfwGetWindowUserPointer( window ) );
    if ( ! app )
    {
        spdlog::warn( "App is null in mouse button callback" );
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    if ( io.WantCaptureMouse ) return; // ImGui has captured event

    // Update button state
    s_mouseButtonState.updateFromGlfwEvent( button, action );
    s_modifierState.updateFromGlfwEvent( mods );

    // Reset start and previous hits
    s_startHit = std::nullopt;
    s_prevHit = std::nullopt;

    double mindowCursorPosX, mindowCursorPosY;
    glfwGetCursorPos( window, &mindowCursorPosX, &mindowCursorPosY );

    const glm::vec2 windowCursorPos = camera::window_T_mindow(
                static_cast<float>( app->windowData().getWindowSize().y ),
                { mindowCursorPosX, mindowCursorPosY } );

    const auto hit = getViewHit( app->appData(), windowCursorPos );
    if ( ! hit ) return;

    // Send event to the annotation state machine
    switch ( action )
    {
    case GLFW_PRESS:
    {
        send_event( state::MousePressEvent( *hit ) );
        break;
    }
    case GLFW_RELEASE:
    {
        // app->state().setMouseMode( MouseMode::Nothing );
        app->appData().windowData().setActiveViewUid( std::nullopt );

        send_event( state::MouseReleaseEvent( *hit ) );
        break;
    }
    default: break;
    }

    // Trigger the cursor position callback
    cursorPosCallback( window, mindowCursorPosX, mindowCursorPosY );
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

    double mindowCursorPosX, mindowCursorPosY;
    glfwGetCursorPos( window, &mindowCursorPosX, &mindowCursorPosY );
    cursorPosCallback( window, mindowCursorPosX, mindowCursorPosY );

    const glm::vec2 windowCursorPos = camera::window_T_mindow(
                static_cast<float>( app->windowData().getWindowSize().y ),
                { mindowCursorPosX, mindowCursorPosY } );

    const auto hit = getViewHit( app->appData(), windowCursorPos, std::nullopt );
    if ( ! hit ) return;

    CallbackHandler& handler = app->callbackHandler();

    switch ( app->appData().state().mouseMode() )
    {
    case MouseMode::Pointer:
    case MouseMode::Segment:
    case MouseMode::CameraTranslate:
    case MouseMode::CameraRotate:
    case MouseMode::ImageRotate:
    case MouseMode::ImageTranslate:
    case MouseMode::ImageScale:
    case MouseMode::WindowLevel:
    {
        handler.doCrosshairsScroll( *hit, { scrollOffsetX, scrollOffsetY } );
        break;
    }
    case MouseMode::CameraZoom:
    {
        handler.doCameraZoomScroll( *hit, { scrollOffsetX, scrollOffsetY },
                                    ZoomBehavior::ToCrosshairs,
                                    syncZoomsForAllViews( s_modifierState ) );
        break;
    }
    case MouseMode::Annotate:
    {
        // Disable scrolling while annotating:
        if ( ! ASM::is_in_state<state::AnnotationOffState>() )
        {
            break;
        }

        handler.doCrosshairsScroll( *hit, { scrollOffsetX, scrollOffsetY } );
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

    double mindowCursorPosX, mindowCursorPosY;
    glfwGetCursorPos( window, &mindowCursorPosX, &mindowCursorPosY );

    const glm::vec2 windowCursorPos = camera::window_T_mindow(
                static_cast<float>( app->windowData().getWindowSize().y ),
                { mindowCursorPosX, mindowCursorPosY } );

    const auto hit = getViewHit( app->appData(), windowCursorPos );

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
    case GLFW_KEY_X: handler.setMouseMode( MouseMode::CameraTranslate ); break;

    case GLFW_KEY_A: handler.decreaseSegOpacity(); break;
    case GLFW_KEY_S: handler.toggleSegVisibility(); break;
    case GLFW_KEY_D: handler.increaseSegOpacity(); break;

    case GLFW_KEY_W: handler.toggleImageVisibility(); break;
    case GLFW_KEY_E: handler.toggleImageEdges(); break;
    case GLFW_KEY_O: handler.cycleOverlayAndUiVisibility(); break;

    case GLFW_KEY_C:
    {
        handler.recenterViews( app->appData().state().recenteringMode(), true, false, true );
        break;
    }

    case GLFW_KEY_F4: handler.toggleFullScreenMode(); break;
    case GLFW_KEY_ESCAPE: handler.toggleFullScreenMode( true ); break;

    case GLFW_KEY_PAGE_DOWN:
    {
        if ( s_modifierState.shift )
        {
            handler.cycleImageComponent( -1 );
        }
        else
        {
            if ( ! hit ) break;
            handler.scrollViewSlice( *hit, -1 );
        }

        break;
    }
    case GLFW_KEY_PAGE_UP:
    {
        if ( s_modifierState.shift )
        {
            handler.cycleImageComponent( 1 );
        }
        else
        {
            if ( ! hit ) break;
            handler.scrollViewSlice( *hit, 1 );
        }

        break;
    }
    case GLFW_KEY_LEFT:
    {
        if ( ! hit ) break;
        handler.moveCrosshairsOnViewSlice( *hit, -1, 0 );
        break;
    }
    case GLFW_KEY_RIGHT:
    {
        if ( ! hit ) break;
        handler.moveCrosshairsOnViewSlice( *hit, 1, 0 );
        break;
    }
    case GLFW_KEY_UP:
    {
        if ( ! hit ) break;
        handler.moveCrosshairsOnViewSlice( *hit, 0, 1 );
        break;
    }
    case GLFW_KEY_DOWN:
    {
        if ( ! hit ) break;
        handler.moveCrosshairsOnViewSlice( *hit, 0, -1 );
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
            const auto mouseMode = app->appData().state().mouseMode();

            if ( MouseMode::Pointer == mouseMode ||
                 MouseMode::ImageRotate == mouseMode ||
                 MouseMode::ImageScale == mouseMode )
            {
                app->appData().state().setWorldRotationCenter(
                            app->appData().state().worldCrosshairs().worldOrigin() );
            }
            break;
        }

        case GLFW_KEY_SPACE:
        {
            const auto mouseMode = app->appData().state().mouseMode();

            if ( MouseMode::Pointer == mouseMode ||
                 MouseMode::ImageRotate == mouseMode ||
                 MouseMode::ImageScale == mouseMode )
            {
                app->appData().state().setWorldRotationCenter( std::nullopt );
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

            serialize::Image serializedImage;
            serializedImage.m_imageFileName = paths[i];
            app->loadSerializedImage( serializedImage );
        }
    }
}
