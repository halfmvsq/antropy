#ifndef IMGUI_WRAPPER_H
#define IMGUI_WRAPPER_H

#include <glm/vec3.hpp>

#include <uuid.h>

#include <functional>

class AppData;
struct GLFWwindow;


/**
 * @brief A simple wrapper for ImGui
 */
class ImGuiWrapper
{
public:

    ImGuiWrapper( GLFWwindow* window, AppData& appData );
    ~ImGuiWrapper();

    void setCallbacks(
            std::function< void ( const uuids::uuid& viewUid ) > recenterView,
            std::function< void ( bool recenterCrosshairs, bool recenterOnCurrentCrosshairsPosition ) > recenterCurrentViews,
            std::function< bool ( void ) > getOverlayVisibility,
            std::function< void ( bool ) > setOverlayVisibility,
            std::function< void ( const uuids::uuid& viewUid ) > updateImageUniforms,
            std::function< void ( const uuids::uuid& viewUid ) > updateImageInterpolationMode,
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
            std::function< bool ( const uuids::uuid& imageUid, const uuids::uuid& seedSegUid, const uuids::uuid& resultSegUid ) > m_executeGridCutsSeg,
            std::function< bool ( const uuids::uuid& imageUid, bool locked ) > setLockManualImageTransformation );

    void render();


private:

    void initializeData();

    AppData& m_appData;

    // Callbacks:
    std::function< void ( const uuids::uuid& viewUid ) > m_recenterView;
    std::function< void ( bool recenterCrosshairs, bool recenterOnCurrentCrosshairsPosition ) > m_recenterAllViews;
    std::function< bool ( void ) > m_getOverlayVisibility;
    std::function< void ( bool ) > m_setOverlayVisibility;
    std::function< void ( const uuids::uuid& viewUid ) > m_updateImageUniforms;
    std::function< void ( const uuids::uuid& viewUid ) > m_updateImageInterpolationMode;
    std::function< void ( size_t tableIndex ) > m_updateLabelColorTableTexture;
    std::function< void ( void ) > m_updateMetricUniforms;
    std::function< glm::vec3 () > m_getWorldDeformedPos;
    std::function< std::optional<glm::vec3> ( size_t imageIndex ) > m_getSubjectPos;
    std::function< std::optional<glm::ivec3> ( size_t imageIndex ) > m_getVoxelPos;
    std::function< std::optional<double> ( size_t imageIndex ) > m_getImageValue;
    std::function< std::optional<int64_t> ( size_t imageIndex ) > m_getSegLabel;
    std::function< std::optional<uuids::uuid>( const uuids::uuid& matchingImageUid, const std::string& segDisplayName ) > m_createBlankSeg;
    std::function< bool ( const uuids::uuid& segUid ) > m_clearSeg;
    std::function< bool ( const uuids::uuid& segUid ) > m_removeSeg;
    std::function< bool ( const uuids::uuid& imageUid, const uuids::uuid& seedSegUid, const uuids::uuid& resultSegUid ) > m_executeGridCutsSeg;
    std::function< bool ( const uuids::uuid& imageUid, bool locked ) > m_setLockManualImageTransformation;
};

#endif // IMGUI_WRAPPER_H
