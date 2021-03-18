#ifndef ANTROPY_APP_H
#define ANTROPY_APP_H

#include "common/InputParams.h"
#include "common/Types.h"

#include "logic/app/CallbackHandler.h"
#include "logic/app/Data.h"
#include "logic/app/Settings.h"
#include "logic/app/State.h"

#include "rendering/Rendering.h"
#include "ui/ImGuiWrapper.h"

#include "windowing/GlfwWrapper.h"

#include <glm/vec3.hpp>

#include <uuid.h>

#include <atomic>
#include <future>
#include <optional>
#include <string>

struct GLFWcursor;


/**
 * @brief This class basically runs the show. Its responsibilities are
 * 1) Hold the OpenGL context and all application data, including for the UI, rendering, and windowing
 * 2) Run the rendering loop
 * 3) Load images
 * 4) Execute callbacks from the UI
 *
 * @note Might be nice to split this class apart.
 */
class AntropyApp
{
public:

    AntropyApp();
    ~AntropyApp();

    /// Initialize rendering functions, OpenGL context, and windowing (GLFW)
    void init();

    /// Run the render loop
    void run();

    /// Resize the framebuffer
    void resize( int width, int height );

    /// Render one frame
    void render();

    /// Asynchronously load images and notify render loop when done
    void loadImagesFromParams( const InputParams& );


    /// Load an image from disk.
    /// @return Uid and flag if loaded.
    /// False indcates that it was already loaded and that we are returning an existing image.
    std::pair< std::optional<uuids::uuid>, bool >
    loadImage( const std::string& fileName, bool ignoreIfAlreadyLoaded );

    /// Load a segmentation from disk. If its header does not match the given image, then it is not loaded
    /// @return Uid and flag if loaded.
    /// False indcates that it was already loaded and that we are returning an existing image.
    std::pair< std::optional<uuids::uuid>, bool >
    loadSegmentation( const std::string& fileName,
                      const std::optional<uuids::uuid>& imageUid = std::nullopt );

    /// Load a deformation field from disk.
    /// @todo If its header does not match the given image, then it is not loaded
    /// @return Uid and flag if loaded.
    /// False indcates that it was already loaded and that we are returning an existing image.
    std::pair< std::optional<uuids::uuid>, bool >
    loadDeformationField( const std::string& fileName );


    CallbackHandler& callbackHandler();

    const AppData& appData() const;
    AppData& appData();

    const AppSettings& appSettings() const;
    AppSettings& appSettings();

    const AppState& appState() const;
    AppState& appState();

    const GuiData& guiData() const;
    GuiData& guiData();

    const GlfwWrapper& glfw() const;
    GlfwWrapper& glfw();

    const RenderData& renderData() const;
    RenderData& renderData();

    const WindowData& windowData() const;
    WindowData& windowData();

    static void logPreamble();


private:

//    void broadcastCrosshairsPosition();

    void setCallbacks();

    bool loadSerializedImage( const serialize::Image& );

    /// Create a blank segmentation with the same header as the given image
    std::optional<uuids::uuid> createBlankSeg(
            const uuids::uuid& matchImageUid,
            std::string segDisplayName );

    /// Create a blank segmentation with the same header as the given image
    std::optional<uuids::uuid> createBlankSegWithColorTable(
            const uuids::uuid& matchImageUid,
            std::string segDisplayName );

    std::future<void> m_futureLoadProject;

    // Set true when images are loaded from disk and ready to be loaded into textures
    std::atomic<bool> m_imagesReady;

    // Set true if images could not be loaded
    std::atomic<bool> m_imageLoadFailed;

    GlfwWrapper m_glfw;
    AppData m_data;
    ImGuiWrapper m_imgui;
    Rendering m_rendering;

    CallbackHandler m_callbackHandler;
};

#endif // ANTROPY_APP_H
