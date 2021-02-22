#include "ui/Toolbars.h"
#include "ui/GuiData.h"
#include "ui/Helpers.h"
#include "ui/Popups.h"
#include "ui/Widgets.h"

#include "logic/app/Data.h"

#include <IconFontCppHeaders/IconsForkAwesome.h>

#include <imgui/imgui.h>

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/color_space.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>


namespace
{

static const ImVec2 sk_toolbarButtonSize( 32, 32 );
static constexpr float sk_pad = 10.0f;

static const ImVec4 sk_darkTextColor( 0.0f, 0.0f, 0.0f, 1.0f );
static const ImVec4 sk_lightTextColor( 1.0f, 1.0f, 1.0f, 1.0f );


void renderContextMenu( int& corner, bool& isHoriz )
{
    if ( ImGui::BeginMenu( "Position" ) )
    {
        if ( ImGui::MenuItem( "Custom", nullptr, corner == -1 ) ) corner = -1;
        if ( ImGui::MenuItem( "Top-left", nullptr, corner == 0 ) ) corner = 0;
        if ( ImGui::MenuItem( "Top-right", nullptr, corner == 1 ) ) corner = 1;
        if ( ImGui::MenuItem( "Bottom-left", nullptr, corner == 2 ) ) corner = 2;
        if ( ImGui::MenuItem( "Bottom-right", nullptr, corner == 3 ) ) corner = 3;
        ImGui::EndMenu();
    }

    if ( ImGui::BeginMenu( "Orientation" ) )
    {
        if ( ImGui::MenuItem( "Horizontal", nullptr, ( isHoriz ) ) ) isHoriz = ! isHoriz;
        if ( ImGui::MenuItem( "Vertical", nullptr, ( ! isHoriz ) ) ) isHoriz = ! isHoriz;
        ImGui::EndMenu();
    }

    ImGui::EndPopup();
}

} // anonymous


void renderToolbar(
        AppData& appData,
        const std::function< MouseMode (void) >& getMouseMode,
        const std::function< void (MouseMode) >& setMouseMode,
        const std::function< void (void) >& recenterViews,
        const std::function< bool (void) >& getOverlayVisibility,
        const std::function< void (bool) >& setOverlayVisibility,
        const std::function< void (int step) >& cycleViews,

        const std::function< size_t (void) >& getNumImages,
        const std::function< std::pair<const char*, const char* >( size_t index ) >& getImageDisplayAndFileName,
        const std::function< size_t (void) >& getActiveImageIndex,
        const std::function< void (size_t) >& setActiveImageIndex )
{
    // Always keep the toolbar open by setting this to null
    static bool* toolbarWindowOpen = nullptr;

    static int corner = 1;
    static bool isHoriz = false;

    static const ImGuiWindowFlags toolbarWindowFlags = 0
            | ImGuiWindowFlags_AlwaysAutoResize
            | ImGuiWindowFlags_NoFocusOnAppearing
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_NoNav
            ;

    const MouseMode activeMouseMode = getMouseMode();

    const ImVec2 buttonSpace = ( isHoriz ? ImVec2( 2.0f, 0.0f ) : ImVec2( 0.0f, 2.0f ) );

    bool openAddLayoutPopup = false;

    const ImVec4* colors = ImGui::GetStyle().Colors;
    ImVec4 activeColor = colors[ImGuiCol_ButtonActive];
    ImVec4 inactiveColor = colors[ImGuiCol_Button];
    ImVec4 highlightColor( 0.64f, 0.44f, 0.64f, 0.40f );

    activeColor.w = 0.94f;
    inactiveColor.w = 0.7f;


    GuiData& guiData = appData.guiData();

    ImGui::PushID( "toolbar" );

    ImGuiIO& io = ImGui::GetIO();

    ImGuiWindowFlags windowFlags = toolbarWindowFlags;

    if ( corner != -1 )
    {
        windowFlags |= ImGuiWindowFlags_NoMove;

        const ImVec2 windowPos = ImVec2( ( corner & 1 ) ? io.DisplaySize.x - sk_pad : sk_pad,
                                         ( corner & 2 ) ? io.DisplaySize.y - sk_pad : sk_pad );

        const ImVec2 windowPosPivot = ImVec2( ( corner & 1 ) ? 1.0f : 0.0f,
                                              ( corner & 2 ) ? 1.0f : 0.0f );

        ImGui::SetNextWindowPos( windowPos, ImGuiCond_Always, windowPosPivot );
    }


    ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
    ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0.0f, 0.0f ) );

    //    static bool isCollapsed = false;

    const char* title = ( ( isHoriz /*| isCollapsed*/ )
                          ? "Tools###ToolbarWindow"
                          : "###ToolbarWindow" );

    if ( ImGui::Begin( title, toolbarWindowOpen, toolbarWindowFlags ) )
    {
        //        isCollapsed = false;

        int id = 0;

        for ( const MouseMode& mouseMode : AllMouseModes )
        {
            ImGui::PushID( id );
            {
                bool isModeActive = ( activeMouseMode == mouseMode );

                if ( isHoriz ) ImGui::SameLine();
                ImGui::PushStyleColor( ImGuiCol_Button, ( isModeActive ? activeColor : inactiveColor ) );
                {
                    if ( ImGui::Button( toolbarButtonIcon( mouseMode ), sk_toolbarButtonSize) )
                    {
                        isModeActive = ! isModeActive;
                        if ( isModeActive )
                        {
                            setMouseMode( mouseMode );
                        }
                    }

                    if ( ImGui::IsItemHovered() )
                    {
                        ImGui::SetTooltip( "%s", typeString( mouseMode ).c_str() );
                    }

                    if ( MouseMode::CameraZoom == mouseMode ||
                         MouseMode::Annotate == mouseMode )
                    {
                        // Put a small dummy space after these buttons
                        if ( isHoriz ) ImGui::SameLine();
                        ImGui::Dummy( buttonSpace );
                    }
                }
                ImGui::PopStyleColor( 1 ); // ImGuiCol_Button
                ++id;
            }
            ImGui::PopID();
        }


        // These are not checkable (toggle) buttons, so style them using the
        // inactive button color
        ImGui::PushStyleColor( ImGuiCol_Button, inactiveColor );
        {
            if ( isHoriz ) ImGui::SameLine();
            ImGui::Dummy( buttonSpace );

            if ( isHoriz ) ImGui::SameLine();
            if ( ImGui::Button( ICON_FK_PICTURE_O, sk_toolbarButtonSize ) )
            {
                ImGui::OpenPopup( "imagePopup" );
            }

            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "%s", "Set active image" );
            }

            if ( ImGui::BeginPopup( "imagePopup" ) )
            {
                const size_t activeIndex = getActiveImageIndex();

                for ( size_t i = 0; i < getNumImages(); ++i )
                {
                    ImGui::PushID( static_cast<int>( i ) );
                    {
                        const auto displayAndFileName = getImageDisplayAndFileName(i);

                        bool isSelected = ( i == activeIndex );
                        if ( ImGui::MenuItem( displayAndFileName.first, "", &isSelected ) )
                        {
                            if ( isSelected )
                            {
                                setActiveImageIndex( i );
                                ImGui::SetItemDefaultFocus();
                            }
                        }

                        if ( ImGui::IsItemHovered() )
                        {
                            ImGui::SetTooltip( "%s", displayAndFileName.second );
                        }
                    }
                    ImGui::PopID(); // i
                }

                ImGui::EndPopup();
            }


            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushID( id );
            {
                ImGui::PushStyleColor( ImGuiCol_Button, ( guiData.m_showImagePropertiesWindow ? activeColor : inactiveColor ) );
                {
                    if ( ImGui::Button( ICON_FK_SLIDERS, sk_toolbarButtonSize) )
                    {
                        guiData.m_showImagePropertiesWindow = ! guiData.m_showImagePropertiesWindow;
                    }

                    if ( ImGui::IsItemHovered() ) {
                        ImGui::SetTooltip( "%s", "Show image properties" );
                    }
                }
                ImGui::PopStyleColor( 1 ); // ImGuiCol_Button

                ++id;
            }
            ImGui::PopID();


            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushID( id );
            {
                ImGui::PushStyleColor( ImGuiCol_Button, ( guiData.m_showSegmentationsWindow ? activeColor : inactiveColor ) );
                {
                    //if ( ImGui::Button( ICON_FK_TH, sk_toolbarButtonSize) )
                    if ( ImGui::Button( ICON_FK_LIST_OL, sk_toolbarButtonSize) )
                    {
                        guiData.m_showSegmentationsWindow = ! guiData.m_showSegmentationsWindow;
                    }

                    if ( ImGui::IsItemHovered() ) {
                        ImGui::SetTooltip( "%s", "Show segmentation properties" );
                    }
                }
                ImGui::PopStyleColor( 1 ); // ImGuiCol_Button

                ++id;
            }
            ImGui::PopID();


            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushID( id );
            {
                ImGui::PushStyleColor( ImGuiCol_Button, ( guiData.m_showLandmarksWindow ? activeColor : inactiveColor ) );
                {
                    if ( ImGui::Button( ICON_FK_MAP_MARKER, sk_toolbarButtonSize) )
                    {
                        guiData.m_showLandmarksWindow = ! guiData.m_showLandmarksWindow;
                    }

                    if ( ImGui::IsItemHovered() ) {
                        ImGui::SetTooltip( "%s", "Show landmark properties" );
                    }
                }
                ImGui::PopStyleColor( 1 ); // ImGuiCol_Button

                ++id;
            }
            ImGui::PopID();


            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushID( id );
            {
                ImGui::PushStyleColor( ImGuiCol_Button, ( guiData.m_showAnnotationsWindow ? activeColor : inactiveColor ) );
                {
                    if ( ImGui::Button( ICON_FK_STAR_O, sk_toolbarButtonSize) )
                    {
                        guiData.m_showAnnotationsWindow = ! guiData.m_showAnnotationsWindow;
                    }

                    if ( ImGui::IsItemHovered() ) {
                        ImGui::SetTooltip( "%s", "Show annotation properties" );
                    }
                }
                ImGui::PopStyleColor( 1 ); // ImGuiCol_Button

                ++id;
            }
            ImGui::PopID();


            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushID( id );
            {
                ImGui::PushStyleColor( ImGuiCol_Button, ( guiData.m_showSettingsWindow ? activeColor : inactiveColor ) );
                {
                    if ( ImGui::Button( ICON_FK_COGS, sk_toolbarButtonSize) )
                    {
                        guiData.m_showSettingsWindow = ! guiData.m_showSettingsWindow;
                    }

                    if ( ImGui::IsItemHovered() ) {
                        ImGui::SetTooltip( "%s", "Show settings" );
                    }
                }
                ImGui::PopStyleColor( 1 ); // ImGuiCol_Button

                ++id;
            }
            ImGui::PopID();


            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushID( id );
            {
                ImGui::PushStyleColor( ImGuiCol_Button, ( guiData.m_showInspectionWindow ? activeColor : inactiveColor ) );
                {
                    if ( ImGui::Button( ICON_FK_EYEDROPPER, sk_toolbarButtonSize) )
                    {
                        guiData.m_showInspectionWindow = ! guiData.m_showInspectionWindow;
                    }

                    if ( ImGui::IsItemHovered() ) {
                        ImGui::SetTooltip( "%s", "Show cursor inspector" );
                    }
                }
                ImGui::PopStyleColor( 1 ); // ImGuiCol_Button

                ++id;
            }
            ImGui::PopID();


            if ( isHoriz ) ImGui::SameLine();
            ImGui::Dummy( buttonSpace );

            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushID( id );
            {
                if ( ImGui::Button( ICON_FK_CROSSHAIRS, sk_toolbarButtonSize) ) {
                    recenterViews();
                }
                if ( ImGui::IsItemHovered() ) {
                    ImGui::SetTooltip( "%s", "Recenter views (C)" );
                }
                ++id;
            }
            ImGui::PopID();


            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushID( id );
            {
                bool isOverlayVisible = getOverlayVisibility();

                ImGui::PushStyleColor( ImGuiCol_Button, ( isOverlayVisible ? activeColor : inactiveColor ) );
                {
                    if ( ImGui::Button( ICON_FK_CLONE, sk_toolbarButtonSize) )
                    {
                        isOverlayVisible = ! isOverlayVisible;
                        setOverlayVisibility( isOverlayVisible );
                    }

                    if ( ImGui::IsItemHovered() ) {
                        ImGui::SetTooltip( "%s", "Toggle view overlays (O)" );
                    }
                }
                ImGui::PopStyleColor( 1 ); // ImGuiCol_Button

                ++id;
            }
            ImGui::PopID();


            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushID( id );
            {
//                ImGui::PushStyleColor( ImGuiCol_Button, highlightColor );
                {
                    if ( ImGui::Button( ICON_FK_CARET_SQUARE_O_LEFT, sk_toolbarButtonSize) ) {
                        cycleViews( -1 );
                    }
                    if ( ImGui::IsItemHovered() ) {
                        ImGui::SetTooltip( "%s", "Previous layout ([)" );
                    }
                }
//                ImGui::PopStyleColor( 1 ); // ImGuiCol_Button
                ++id;
            }
            ImGui::PopID();


            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushID( id );
            {
//                ImGui::PushStyleColor( ImGuiCol_Button, highlightColor );
                {
                    if ( ImGui::Button( ICON_FK_CARET_SQUARE_O_RIGHT, sk_toolbarButtonSize) ) {
                        cycleViews( 1 );
                    }
                    if ( ImGui::IsItemHovered() ) {
                        ImGui::SetTooltip( "%s", "Next layout (])" );
                    }
                }
//                ImGui::PopStyleColor( 1 ); // ImGuiCol_Button
                ++id;
            }
            ImGui::PopID();


            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushID( id );
            {
                if ( ImGui::Button( ICON_FK_TH, sk_toolbarButtonSize) )
                {
                    openAddLayoutPopup = true;
                }

                if ( ImGui::IsItemHovered() ) {
                    ImGui::SetTooltip( "%s", "Add new layout" );
                }

                ++id;
            }
            ImGui::PopID();


            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushID( id );
            {
                if ( ImGui::Button( ICON_FK_WINDOW_CLOSE_O, sk_toolbarButtonSize) )
                {
                    auto& wd = appData.windowData();
                    if ( wd.numLayouts() >= 2 )
                    {
                        // Only delete a layout if there are at least two, so that one is left.
                        const size_t layoutToDelete = wd.currentLayoutIndex();
                        wd.cycleCurrentLayout( -1 );
                        wd.removeLayout( layoutToDelete );
                    }
                }

                if ( ImGui::IsItemHovered() ) {
                    ImGui::SetTooltip( "%s", "Remove current layout" );
                }

                ++id;
            }
            ImGui::PopID();


            if ( isHoriz ) ImGui::SameLine();
            ImGui::Dummy( buttonSpace );

            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushID( id );
            {
                if ( ImGui::Button( ICON_FK_INFO, sk_toolbarButtonSize) )
                {
                }

                if ( ImGui::IsItemHovered() ) {
                    ImGui::SetTooltip( "%s", "About Antropy" );
                }

                ++id;
            }
            ImGui::PopID();
        }
        ImGui::PopStyleColor( 1 ); // ImGuiCol_Button


        if ( ImGui::BeginPopupContextWindow() )
        {
            renderContextMenu( corner, isHoriz );
        }

        ImGui::End(); // End toolbar
    }
//    else
//    {
//        isCollapsed = true;
//    }

    // Pop: ImGuiStyleVar_WindowRounding, ImGuiStyleVar_WindowBorderSize,
    // ImGuiStyleVar_WindowPadding, ImGuiStyleVar_ItemSpacing
    ImGui::PopStyleVar( 4 );

    ImGui::PopID();

    renderAddLayoutModalPopup(
                appData,
                openAddLayoutPopup,
                recenterViews );
}


void renderSegToolbar(
        AppData& appData,
        const std::function< size_t (void) >& getNumImages,
        const std::function< std::pair<const char*, const char* >( size_t index ) >& getImageDisplayAndFileName,
        const std::function< size_t (void) >& getActiveImageIndex,
        const std::function< void (size_t) >& setActiveImageIndex,
        const std::function< bool ( size_t imageIndex ) >& getImageHasActiveSeg,
        const std::function< void ( size_t imageIndex, bool set ) >& setImageHasActiveSeg,
        const std::function< void( const uuids::uuid& imageUid ) >& updateImageUniforms,
        const std::function< std::optional<uuids::uuid>( const uuids::uuid& matchingImageUid, const std::string& segDisplayName ) >& createBlankSeg,
        const std::function< bool ( const uuids::uuid& imageUid, const uuids::uuid& seedSegUid, const uuids::uuid& resultSegUid ) >& executeGridCutsSeg )
{
    if ( MouseMode::Segment != appData.state().mouseMode() )
    {
        return;
    }

    // Always keep the toolbar open by setting this to null
    static bool* toolbarWindowOpen = nullptr;

    static int corner = 3;
    static bool isHoriz = false;

    static const ImGuiWindowFlags toolbarWindowFlags = 0
            | ImGuiWindowFlags_AlwaysAutoResize
            | ImGuiWindowFlags_NoFocusOnAppearing
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_NoNav
            ;


    const auto activeImageUid = appData.activeImageUid();
    if ( ! activeImageUid )
    {
        spdlog::error( "There is no active image to segment" );
        return;
    }

    const auto activeSegUid = appData.imageToActiveSegUid( *activeImageUid );
    if ( ! activeSegUid )
    {
        spdlog::error( "There is no active segmentation for image {}", *activeImageUid );
        return;
    }

    const Image* activeSeg = appData.seg( *activeSegUid );
    if ( ! activeSeg )
    {
        spdlog::error( "The active segmentation {} is null for image {}", *activeSegUid, *activeImageUid );
        return;
    }

    const size_t activeLabelTableIndex = activeSeg->settings().labelTableIndex();
    const auto activeLabelTableUid = appData.labelTableUid( activeLabelTableIndex );
    if ( ! activeLabelTableUid )
    {
        spdlog::error( "There is no label table for active segmentation {}", *activeSegUid );
        return;
    }

    const ParcellationLabelTable* activeLabelTable = appData.labelTable( *activeLabelTableUid );
    if ( ! activeLabelTable )
    {
        spdlog::error( "The label table {} for active segmentation {} is null", *activeLabelTableUid, *activeSegUid );
        return;
    }

    const ImVec2 buttonSpace = ( isHoriz ? ImVec2( 2.0f, 0.0f ) : ImVec2( 0.0f, 2.0f ) );

    const ImVec4* colors = ImGui::GetStyle().Colors;
    ImVec4 activeColor = colors[ImGuiCol_ButtonActive];
    ImVec4 inactiveColor = colors[ImGuiCol_Button];
    ImVec4 highlightColor( 0.64f, 0.44f, 0.64f, 0.40f );

    activeColor.w = 0.94f;
    inactiveColor.w = 0.7f;

    ImGui::PushID( "segtoolbar" );

    ImGuiIO& io = ImGui::GetIO();

    ImGuiWindowFlags windowFlags = toolbarWindowFlags;

    if ( corner != -1 )
    {
        windowFlags |= ImGuiWindowFlags_NoMove;

        const ImVec2 windowPos = ImVec2(
                    ( corner & 1 ) ? io.DisplaySize.x - sk_pad : sk_pad,
                    ( corner & 2 ) ? io.DisplaySize.y - sk_pad : sk_pad );

        const ImVec2 windowPosPivot = ImVec2( ( corner & 1 ) ? 1.0f : 0.0f,
                                              ( corner & 2 ) ? 1.0f : 0.0f );

        ImGui::SetNextWindowPos( windowPos, ImGuiCond_Always, windowPosPivot );
    }


    ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
    ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0.0f, 0.0f ) );

    const char* title = ( ( isHoriz /*| isCollapsed*/ )
                          ? "Segmentation###SegToolbarWindow"
                          : "###SegToolbarWindow" );

    if ( ImGui::Begin( title, toolbarWindowOpen, toolbarWindowFlags ) )
    {
        int id = 0;

        const size_t fgLabel = appData.settings().foregroundLabel();
        const size_t bgLabel = appData.settings().backgroundLabel();

        const glm::vec3 fgColor = activeLabelTable->getColor( fgLabel );
        const glm::vec3 bgColor = activeLabelTable->getColor( bgLabel );

        ImVec4 fgImGuiColor( fgColor.r, fgColor.g, fgColor.b, 1.0f );
        ImVec4 bgImGuiColor( bgColor.r, bgColor.g, bgColor.b, 1.0f );

        const bool useDarkTextForFgColor = ( glm::luminosity( fgColor ) > 0.5f );
        const bool useDarkTextForBgColor = ( glm::luminosity( bgColor ) > 0.5f );

        const std::string fgButtonLabel = std::to_string( fgLabel ) + "###fgButton";
        const std::string bgButtonLabel = std::to_string( bgLabel ) + "###bgButton";

        ImGui::PushStyleColor( ImGuiCol_Button, inactiveColor );
        {
            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushStyleColor( ImGuiCol_Button, fgImGuiColor );
            ImGui::PushStyleColor( ImGuiCol_Text, ( useDarkTextForFgColor ? sk_darkTextColor : sk_lightTextColor ) );
            {
                if ( ImGui::Button( fgButtonLabel.c_str(), sk_toolbarButtonSize) )
                {
                    ImGui::OpenPopup( "foregroundLabelPopup" );
                }
            }
            ImGui::PopStyleColor( 2 ); // ImGuiCol_Button, ImGuiCol_Text
            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "%s", "Select foreground label (<,>)" );
            }


            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushStyleColor( ImGuiCol_Button, bgImGuiColor );
            ImGui::PushStyleColor( ImGuiCol_Text, ( useDarkTextForBgColor ? sk_darkTextColor : sk_lightTextColor ) );
            {
                if ( ImGui::Button( bgButtonLabel.c_str(), sk_toolbarButtonSize) )
                {
                    ImGui::OpenPopup( "backgroundLabelPopup" );
                }
            }
            ImGui::PopStyleColor( 2 ); // ImGuiCol_Button, ImGuiCol_Text
            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "%s", "Select background label (shift + <,>)" );
            }


            if ( activeLabelTable && ImGui::BeginPopup( "foregroundLabelPopup" ) )
            {
                const float sz = ImGui::GetTextLineHeight();

                for ( size_t i = 0; i < activeLabelTable->numLabels(); ++i )
                {
                    const std::string labelName = std::to_string( i ) + ") " + activeLabelTable->getName( i );
                    const glm::vec3 labelColor = activeLabelTable->getColor( i );
                    const ImU32 labelColorU32 = ImGui::ColorConvertFloat4ToU32( ImVec4( labelColor.r, labelColor.g, labelColor.b, 1.0f ) );

                    const ImVec2 p = ImGui::GetCursorScreenPos();
                    ImGui::GetWindowDrawList()->AddRectFilled( p, ImVec2( p.x + sz, p.y + sz ), labelColorU32 );
                    ImGui::Dummy( ImVec2( sz, sz ) );
                    ImGui::SameLine();

                    bool isSelected = ( fgLabel == i );
                    if ( ImGui::MenuItem( labelName.c_str(), "", &isSelected ) )
                    {
                        if ( isSelected )
                        {
                            appData.settings().setForegroundLabel( i, *activeLabelTable );
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                }

                ImGui::EndPopup();
            }


            if ( activeLabelTable && ImGui::BeginPopup( "backgroundLabelPopup" ) )
            {
                const float sz = ImGui::GetTextLineHeight();

                for ( size_t i = 0; i < activeLabelTable->numLabels(); ++i )
                {
                    const std::string labelName = std::to_string( i ) + ") " + activeLabelTable->getName( i );
                    const glm::vec3 labelColor = activeLabelTable->getColor( i );
                    const ImU32 labelColorU32 = ImGui::ColorConvertFloat4ToU32( ImVec4( labelColor.r, labelColor.g, labelColor.b, 1.0f ) );

                    const ImVec2 p = ImGui::GetCursorScreenPos();
                    ImGui::GetWindowDrawList()->AddRectFilled( p, ImVec2( p.x + sz, p.y + sz ), labelColorU32 );
                    ImGui::Dummy( ImVec2( sz, sz ) );
                    ImGui::SameLine();

                    bool isSelected = ( bgLabel == i );
                    if ( ImGui::MenuItem( labelName.c_str(), "", &isSelected ) )
                    {
                        if ( isSelected )
                        {
                            appData.settings().setBackgroundLabel( i, *activeLabelTable );
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                }

                ImGui::EndPopup();
            }


            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushID( id );
            {
                if ( ImGui::Button( ICON_FK_RANDOM, sk_toolbarButtonSize ) )
                {
                    appData.settings().swapForegroundAndBackgroundLabels( *activeLabelTable );
                }
                if ( ImGui::IsItemHovered() )
                {
                    ImGui::SetTooltip( "%s", "Swap foreground and background labels" );
                }
                ++id;
            }
            ImGui::PopID();


            if ( isHoriz ) ImGui::SameLine();
            ImGui::Dummy( buttonSpace );


            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushID( id );
            {
                bool replaceBgWithFg = appData.settings().replaceBackgroundWithForeground();
                ImGui::PushStyleColor( ImGuiCol_Button, ( replaceBgWithFg ? activeColor : inactiveColor ) );
                {
                    if ( ImGui::Button( ICON_FK_PENCIL_SQUARE, sk_toolbarButtonSize ) )
                    {
                        replaceBgWithFg = ! replaceBgWithFg;
                        appData.settings().setReplaceBackgroundWithForeground( replaceBgWithFg );
                    }

                    if ( ImGui::IsItemHovered() ) {
                        ImGui::SetTooltip( "%s", "Draw foreground label only on top of background label" );
                    }
                }
                ImGui::PopStyleColor( 1 ); // ImGuiCol_Button

                ++id;
            }
            ImGui::PopID();


            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushID( id );
            {
                bool use3d = appData.settings().use3dBrush();
                ImGui::PushStyleColor( ImGuiCol_Button, ( use3d ? activeColor : inactiveColor ) );
                {
                    if ( ImGui::Button( ICON_FK_CUBE, sk_toolbarButtonSize) )
                    {
                        use3d = ! use3d;
                        appData.settings().setUse3dBrush( use3d );
                    }

                    if ( ImGui::IsItemHovered() )
                    {
                        ImGui::SetTooltip( "%s", "Set 2D/3D brush" );
                    }
                }
                ImGui::PopStyleColor( 1 ); // ImGuiCol_Button

                ++id;
            }
            ImGui::PopID();


            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushID( id );
            {
                bool roundBrush = appData.settings().useRoundBrush();

                if ( ImGui::Button( roundBrush ? ICON_FK_CIRCLE_THIN : ICON_FK_SQUARE_O, sk_toolbarButtonSize ) )
                {
                    roundBrush = ! roundBrush;
                    appData.settings().setUseRoundBrush( roundBrush );
                }

                if ( ImGui::IsItemHovered() )
                {
                    ImGui::SetTooltip( "%s", "Set round/square brush shape" );
                }

                ++id;
            }
            ImGui::PopID();


            if ( isHoriz ) ImGui::SameLine();
            ImGui::Dummy( buttonSpace );

            constexpr uint32_t minBrushVox = 1;
            constexpr uint32_t maxBrushVox = 101;

            if ( isHoriz ) ImGui::SameLine();
            if ( ImGui::Button( ICON_FK_PLUS_CIRCLE, sk_toolbarButtonSize) )
            {
                /// @todo replace with AntropyApp::cycleBrushSize
                uint32_t brushSizeVox = appData.settings().brushSizeInVoxels();
                brushSizeVox = glm::clamp( brushSizeVox + 1, minBrushVox, maxBrushVox );
                appData.settings().setBrushSizeInVoxels( brushSizeVox );
            }
            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "%s", "Increase brush size (+)" );
            }

            if ( isHoriz ) ImGui::SameLine();
            if ( ImGui::Button( ICON_FK_MINUS_CIRCLE, sk_toolbarButtonSize) )
            {
                /// @todo replace with AntropyApp::cycleBrushSize
                uint32_t brushSizeVox = appData.settings().brushSizeInVoxels();
                brushSizeVox = glm::clamp( brushSizeVox - 1, minBrushVox, maxBrushVox );
                appData.settings().setBrushSizeInVoxels( brushSizeVox );
            }
            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "%s", "Decrease brush size (-)" );
            }


            if ( isHoriz ) ImGui::SameLine();
            if ( ImGui::Button( ICON_FK_BULLSEYE, sk_toolbarButtonSize) )
            {
                ImGui::OpenPopup( "brushSizePopup" );
            }

            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "%s", "Brush options" );
            }

            if ( ImGui::BeginPopup( "brushSizePopup" ) )
            {
                bool useVoxels = appData.settings().useVoxelBrushSize();
                bool replaceBgWithFg = appData.settings().replaceBackgroundWithForeground();
                bool use3d = appData.settings().use3dBrush();
                bool useIso = appData.settings().useIsotropicBrush();
                bool useRound = appData.settings().useRoundBrush();
                bool xhairsMove = appData.settings().crosshairsMoveWithBrush();

                ImGui::Text( "Brush options:" );
                ImGui::Separator();
                ImGui::Dummy( ImVec2( 0.0f, 4.0f ) );

                ImGui::Spacing();

                if ( useVoxels )
                {
                    uint32_t brushSizeVox = appData.settings().brushSizeInVoxels();
                    uint32_t stepSmall = 1;
                    uint32_t stepBig = 5;

                    if ( ImGui::InputScalar( "width (vox)##brushSizeVox", ImGuiDataType_U32, &brushSizeVox, &stepSmall, &stepBig ) )
                    {
                        brushSizeVox = glm::clamp( brushSizeVox, minBrushVox, maxBrushVox );
                        appData.settings().setBrushSizeInVoxels( brushSizeVox );
                    }
                }
//                else
//                {
//                    float brushSizeMm = appData.brushSizeInMm();

//                    if ( ImGui::SliderFloat( "size (mm)##brushSizeMmSlider", &brushSizeMm, 0.001f, 100.000f, "%.3f" ) )
//                    {
//                        appData.setBrushSizeInMm( brushSizeMm );
//                    }
//                }
                ImGui::SameLine(); ImGui::Dummy( ImVec2( 2.0f, 0.0f ) );
                ImGui::SameLine(); helpMarker( "Brush width in voxels" );
                ImGui::Dummy( ImVec2( 0.0f, 2.0f ) );


//                if ( ImGui::RadioButton( "Voxels", useVoxels ) )
//                {
//                    useVoxels = true;
//                    appData.setUseVoxelBrushSize( useVoxels );
//                }

//                ImGui::SameLine();
//                if ( ImGui::RadioButton( "Millimeters", ! useVoxels ) )
//                {
//                    useVoxels = false;
//                    appData.setUseVoxelBrushSize( useVoxels );
//                }
//                ImGui::SameLine(); ImGui::Dummy( ImVec2( 2.0f, 0.0f ) );
//                ImGui::SameLine(); HelpMarker( "Set brush size in units of either voxels or millimeters" );


                if ( ImGui::RadioButton( "Round", useRound ) )
                {
                    useRound = true;
                    appData.settings().setUseRoundBrush( useRound );
                }

                ImGui::SameLine();
                if ( ImGui::RadioButton( "Square", ! useRound ) )
                {
                    useRound = false;
                    appData.settings().setUseRoundBrush( useRound );
                }
                ImGui::SameLine(); ImGui::Dummy( ImVec2( 2.0f, 0.0f ) );
                ImGui::SameLine(); helpMarker( "Set either round or square brush shape" );
                ImGui::Dummy( ImVec2( 0.0f, 2.0f ) );


                if ( ImGui::RadioButton( "2D", ! use3d ) )
                {
                    use3d = false;
                    appData.settings().setUse3dBrush( use3d );
                }

                ImGui::SameLine();
                if ( ImGui::RadioButton( "3D", use3d ) )
                {
                    use3d = true;
                    appData.settings().setUse3dBrush( use3d );
                }
                ImGui::SameLine(); ImGui::Dummy( ImVec2( 2.0f, 0.0f ) );
                ImGui::SameLine(); helpMarker( "Set either 2D (planar) or 3D (volumetric) brush shape" );
                ImGui::Dummy( ImVec2( 0.0f, 2.0f ) );


                if ( ImGui::Checkbox( "Isotropic brush", &useIso ) )
                {
                    appData.settings().setUseIsotropicBrush( useIso );
                }
                ImGui::SameLine(); ImGui::Dummy( ImVec2( 2.0f, 0.0f ) );
                ImGui::SameLine(); helpMarker( "Set either anisotropic or isotropic brush dimensions" );
                ImGui::Dummy( ImVec2( 0.0f, 2.0f ) );



                if ( ImGui::Checkbox( "Replace background with foreground", &replaceBgWithFg ) )
                {
                    appData.settings().setReplaceBackgroundWithForeground( replaceBgWithFg );
                }
                ImGui::SameLine(); ImGui::Dummy( ImVec2( 2.0f, 0.0f ) );
                ImGui::SameLine(); helpMarker( "When enabled, the brush only draws the foreground label on top of the background label" );
                ImGui::Dummy( ImVec2( 0.0f, 2.0f ) );



                if ( ImGui::Checkbox( "Crosshairs move with brush", &xhairsMove ) )
                {
                    appData.settings().setCrosshairsMoveWithBrush( xhairsMove );
                }
                ImGui::SameLine(); ImGui::Dummy( ImVec2( 2.0f, 0.0f ) );
                ImGui::SameLine(); helpMarker( "Crosshairs movement is linked with brush movement" );

                ImGui::EndPopup();
            }


            if ( isHoriz ) ImGui::SameLine();
            ImGui::Dummy( buttonSpace );


            if ( isHoriz ) ImGui::SameLine();
            ImGui::PushID( id );
            {
                bool xhairsMove = appData.settings().crosshairsMoveWithBrush();

                ImGui::PushStyleColor( ImGuiCol_Button, ( xhairsMove ? activeColor : inactiveColor ) );
                {
                    if ( ImGui::Button( xhairsMove ? ICON_FK_LINK : ICON_FK_CHAIN_BROKEN, sk_toolbarButtonSize ) )
                    {
                        xhairsMove = ! xhairsMove;
                        appData.settings().setCrosshairsMoveWithBrush( xhairsMove );
                    }
                }
                ImGui::PopStyleColor( 1 ); // ImGuiCol_Button

                if ( ImGui::IsItemHovered() )
                {
                    ImGui::SetTooltip( "%s", "Crosshairs linked to brush" );
                }

                ++id;
            }
            ImGui::PopID();


            if ( isHoriz ) ImGui::SameLine();
            if ( ImGui::Button( ICON_FK_RSS, sk_toolbarButtonSize ) )
            {
                ImGui::OpenPopup( "segSyncPopup" );
            }

            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "%s", "Synchronize drawing of segmentations on multiple images" );
            }


            if ( isHoriz ) ImGui::SameLine();
            if ( ImGui::Button( ICON_FK_CUBES, sk_toolbarButtonSize) )
            {
                const auto imageUid = appData.activeImageUid();
                const Image* image = appData.activeImage();

                if ( imageUid && image )
                {
                    if ( const auto seedSegUid = appData.imageToActiveSegUid( *imageUid ) )
                    {
                        const size_t numSegsForImage = appData.imageToSegUids( *imageUid ).size();

                        std::string segDisplayName =
                                std::string( "Graph Cuts segmentation " ) +
                                std::to_string( numSegsForImage + 1 ) +
                                " for image '" +
                                image->settings().displayName() + "'";

                        if ( const auto blankSegUid = createBlankSeg( *imageUid, std::move( segDisplayName ) ) )
                        {
                            updateImageUniforms( *imageUid );
                            executeGridCutsSeg( *imageUid, *seedSegUid, *blankSegUid );
                        }
                    }
                }
            }
            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "%s", "Execute Graph Cuts segmentation" );
            }


            if ( ImGui::BeginPopup( "segSyncPopup" ) )
            {
                const size_t activeIndex = getActiveImageIndex();

                /*
                for ( size_t i = 0; i < getNumImages(); ++i )
                {
                    ImGui::PushID( static_cast<int>( i ) );
                    {
                        const auto displayAndFileName = getImageDisplayAndFileName(i);

                        // Active image is selected
                        bool isSelected = ( i == activeIndex );

                        // Image is selected if its seg is active
                        isSelected |= getImageHasActiveSeg( i );

                        if ( ImGui::MenuItem( displayAndFileName.first, "", &isSelected ) )
                        {
                            if ( isSelected )
                            {
                                setImageHasActiveSeg( i, true );
                                ImGui::SetItemDefaultFocus();
                            }
                            else
                            {
                                setImageHasActiveSeg( i, false );
                            }
                        }

                        if ( ImGui::IsItemHovered() )
                        {
                            ImGui::SetTooltip( "%s", displayAndFileName.second );
                        }
                    }
                    ImGui::PopID(); // i
                }
                */

                ImGui::Text( "Select the active image to segment:" );
                ImGui::Dummy( ImVec2( 0.0f, 4.0f ) );

                renderActiveImageSelectionCombo(
                            getNumImages,
                            getImageDisplayAndFileName,
                            getActiveImageIndex,
                            setActiveImageIndex,
                            false );

                ImGui::Dummy( ImVec2( 0.0f, 4.0f ) );
                ImGui::Separator();
                ImGui::Dummy( ImVec2( 0.0f, 4.0f ) );

                ImGui::SetNextItemOpen( true, ImGuiCond_Appearing );

                if ( ImGui::TreeNode( "Synchronize drawing on additional images:" ) )
                {
                    ImGui::Dummy( ImVec2( 0.0f, 4.0f ) );

                    for ( size_t i = 0; i < getNumImages(); ++i )
                    {
                        // Active image is not shown
                        if ( i == activeIndex ) continue;

                        const auto displayAndFileName = getImageDisplayAndFileName(i);

                        // Image is selected if its seg is active
                        bool isSelected = getImageHasActiveSeg( i );

                        if ( ImGui::Selectable( displayAndFileName.first, &isSelected ) )
                        {
                            if ( isSelected )
                            {
                                setImageHasActiveSeg( i, true );
                                ImGui::SetItemDefaultFocus();
                            }
                            else
                            {
                                setImageHasActiveSeg( i, false );
                            }
                        }

                        if ( ImGui::IsItemHovered() )
                        {
                            ImGui::SetTooltip( "%s", displayAndFileName.second );
                        }

                        ImGui::Dummy( ImVec2( 0.0f, 4.0f ) );
                    }

                    ImGui::TreePop();
                }

                ImGui::EndPopup();
            }
        }
        ImGui::PopStyleColor( 1 ); // ImGuiCol_Button


        if ( ImGui::BeginPopupContextWindow() )
        {
            renderContextMenu( corner, isHoriz );
        }

        ImGui::End(); // End toolbar
    }

    // Pop: ImGuiStyleVar_WindowRounding, ImGuiStyleVar_WindowBorderSize,
    // ImGuiStyleVar_WindowPadding, ImGuiStyleVar_ItemSpacing
    ImGui::PopStyleVar( 4 );

    ImGui::PopID();
}