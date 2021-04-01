#include "windowing/GlfwWrapper.h"

#include "AntropyApp.h"
#include "common/Exception.hpp"
#include "windowing/GlfwCallbacks.h"

#include <spdlog/spdlog.h>

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>


namespace
{

static const std::string s_antropy( "Antropy" );

}


GlfwWrapper::GlfwWrapper( AntropyApp* app, int glMajorVersion, int glMinorVersion )
    :
      m_eventProcessingMode( EventProcessingMode::Wait ),
      m_waitTimoutSeconds( 1.0 / 30.0 ),

      m_renderScene( nullptr ),
      m_renderGui( nullptr )
{
    if ( ! app )
    {
        spdlog::critical( "The application is null on GLFW creation" );
        throw_debug( "The application is null" )
    }

    spdlog::debug( "OpenGL Core profile version {}.{}", glMajorVersion, glMinorVersion );

    if ( ! glfwInit() )
    {
        spdlog::critical( "Failed to initialize the GLFW windowing library" );
        throw_debug( "Failed to initialize the GLFW windowing library" )
    }

    spdlog::debug( "Initialized GLFW windowing library" );

    // Set OpenGL version
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, glMajorVersion );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, glMinorVersion );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

    // Desired bit depths of the components of the window's default framebuffer
    glfwWindowHint( GLFW_RED_BITS, 8 );
    glfwWindowHint( GLFW_GREEN_BITS, 8 );
    glfwWindowHint( GLFW_BLUE_BITS, 8 );
    glfwWindowHint( GLFW_ALPHA_BITS, 8 );
    glfwWindowHint( GLFW_DEPTH_BITS, 24 );
    glfwWindowHint( GLFW_STENCIL_BITS, 8 );

    // Desired number of samples to use for multisampling
    glfwWindowHint( GLFW_SAMPLES, 4 );

    glfwWindowHint( GLFW_DOUBLEBUFFER, GLFW_TRUE );
    glfwWindowHint( GLFW_MAXIMIZED, GLFW_TRUE );

#ifdef __APPLE__
    // Window's context is an OpenGL forward-compatible, i.e. one where all functionality deprecated
    // in the requested version of OpenGL is removed (required on macOS)
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );

    // Use full resolution framebuffers on Retina displays
    glfwWindowHint( GLFW_COCOA_RETINA_FRAMEBUFFER, GL_TRUE );

    // Disable Automatic Graphics Switching, i.e. do not allow the system to choose the integrated GPU
    // for the OpenGL context and move it between GPUs if necessary. Forces it to always run on the discrete GPU.
    glfwWindowHint( GLFW_COCOA_GRAPHICS_SWITCHING, GL_FALSE );

    spdlog::debug( "Initialized GLFW window and context for Apple macOS platform" );
#endif

    // Get window dimensions, using monitor work area if available
    int width = static_cast<int>( app->windowData().viewport().width() );
    int height = static_cast<int>( app->windowData().viewport().height() );

    if ( GLFWmonitor* monitor = glfwGetPrimaryMonitor() )
    {
        int xpos, ypos;
        glfwGetMonitorWorkarea( monitor, &xpos, &ypos, &width, &height );
    }

//    if ( const GLFWvidmode* videoMode = monitor ? glfwGetVideoMode( monitor ) : nullptr )
//    {
//        width = videoMode->width;
//        height = videoMode->height;
//    }

    m_window = glfwCreateWindow( width, height, "Antropy", nullptr, nullptr );

    if ( ! m_window )
    {
        glfwTerminate();
        throw_debug( "Failed to create GLFW window and context" )
    }

    spdlog::debug( "Created GLFW window and context" );

    // Embed pointer to application data in GLFW window
    glfwSetWindowUserPointer( m_window, reinterpret_cast<void*>( app ) );

    // Make window's context current on this thread
    glfwMakeContextCurrent( m_window );

    // Set callbacks
    glfwSetErrorCallback( errorCallback );
    glfwSetWindowContentScaleCallback( m_window, windowContentScaleCallback );
    glfwSetWindowCloseCallback( m_window, windowCloseCallback );
    glfwSetWindowSizeCallback( m_window, windowSizeCallback );
    glfwSetCursorPosCallback( m_window, cursorPosCallback );
    glfwSetMouseButtonCallback( m_window, mouseButtonCallback );
    glfwSetScrollCallback( m_window, scrollCallback );
    glfwSetKeyCallback( m_window, keyCallback );
    glfwSetDropCallback( m_window, dropCallback );

    spdlog::debug( "Set GLFW callbacks" );

    // Create cursors: not currently used
    m_mouseModeToCursor.emplace(
                MouseMode::WindowLevel, glfwCreateStandardCursor( GLFW_RESIZE_ALL_CURSOR ) );
    spdlog::debug( "Created GLFW cursors" );

//    glfwSetWindowIcon( window, 1, GLFWimage() );

    // Load all OpenGL function pointers with GLAD
    if ( ! gladLoadGLLoader( (GLADloadproc) glfwGetProcAddress ) )
    {
        glfwTerminate();
        spdlog::critical( "Failed to load OpenGL function pointers with GLAD" );
        throw_debug( "Failed to load OpenGL function pointers with GLAD" )
    }

    spdlog::debug( "Loaded OpenGL function pointers with GLAD" );
}


GlfwWrapper::~GlfwWrapper()
{
    for ( auto& cursor : m_mouseModeToCursor )
    {
        if ( cursor.second ) glfwDestroyCursor( cursor.second );
    }

    glfwDestroyWindow( m_window );
    glfwTerminate();
    spdlog::debug( "Destroyed window and terminated GLFW" );
}


void GlfwWrapper::setCallbacks(
        std::function< void() > renderScene,
        std::function< void() > renderGui )
{
    m_renderScene = std::move( renderScene );
    m_renderGui = std::move( renderGui );
}

void GlfwWrapper::setEventProcessingMode( EventProcessingMode mode )
{
    m_eventProcessingMode = std::move( mode );
}

void GlfwWrapper::setWaitTimeout( double waitTimoutSeconds )
{
    m_waitTimoutSeconds = waitTimoutSeconds;
}


void GlfwWrapper::init()
{
    int width, height;
    glfwGetWindowSize( m_window, &width, &height );
    windowSizeCallback( m_window, width, height );

    float xscale, yscale;
    glfwGetWindowContentScale( m_window, &xscale, &yscale );
    windowContentScaleCallback( m_window, xscale, yscale );

    spdlog::debug( "Initialized GLFW wrapper" );
}


void GlfwWrapper::renderLoop(
        std::atomic<bool>& imagesReady,
        const std::atomic<bool>& imageLoadFailed,
        const std::function<void(void)>& onImagesReady )
{
    if ( ! m_renderScene || ! m_renderGui )
    {
        spdlog::critical( "Rendering callbacks not initialized" );
        throw_debug( "Rendering callbacks not initialized" )
    }

    spdlog::debug( "Starting GLFW rendering loop" );

    while ( ! glfwWindowShouldClose( m_window ) )
    {
        if ( imagesReady )
        {
            imagesReady = false;
            onImagesReady();
        }

        if ( imageLoadFailed )
        {
            spdlog::critical( "Render loop exiting due to failure to load images" );
            exit( EXIT_FAILURE );
        }

        processInput();
        renderOnce();

        glfwSwapBuffers( m_window );

        switch ( m_eventProcessingMode )
        {
        case EventProcessingMode::Poll:
        {
            glfwPollEvents();
            break;
        }
        case EventProcessingMode::Wait:
        {
            glfwWaitEvents();
            break;
        }
        case EventProcessingMode::WaitTimeout:
        {
            glfwWaitEventsTimeout( m_waitTimoutSeconds );
            break;
        }
        }
    }

    spdlog::debug( "Done GLFW rendering loop" );
}


void GlfwWrapper::renderOnce()
{
    m_renderScene();
    m_renderGui();
}

void GlfwWrapper::postEmptyEvent()
{
    glfwPostEmptyEvent();
}

void GlfwWrapper::processInput()
{
    // No inputs are currently being processed here.

    // Could check inputs and react as shown below:
    //    if ( GLFW_PRESS == glfwGetKey( m_window, GLFW_KEY_ESCAPE ) )
    //    {
    //        glfwSetWindowShouldClose( m_window, true );
    //    }
}


const GLFWwindow* GlfwWrapper::window() const { return m_window; }
GLFWwindow* GlfwWrapper::window() { return m_window; }

GLFWcursor* GlfwWrapper::cursor( MouseMode mode )
{
    const auto it = m_mouseModeToCursor.find( mode );
    if ( std::end( m_mouseModeToCursor ) != it )
    {
        return it->second;
    }
    return nullptr;
}

void GlfwWrapper::setWindowTitleStatus( const std::string& status )
{
    if ( status.empty() )
    {
        glfwSetWindowTitle( m_window, s_antropy.c_str() );
    }
    else
    {
        const std::string statusString( s_antropy + std::string(" [") + status + std::string("]") );
        glfwSetWindowTitle( m_window, statusString.c_str() );
    }
}
