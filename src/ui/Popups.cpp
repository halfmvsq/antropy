#include "ui/Popups.h"
#include "ui/Helpers.h"

#include "defines.h"
#include "logic/app/Data.h"

#include <imgui/imgui.h>


void renderAddLayoutModalPopup(
        AppData& appData,
        bool openAddLayoutPopup,
        const std::function< void (void) >& recenterViews )
{
    bool addLayout = false;

    static int width = 3;
    static int height = 3;
    static bool isLightbox = false;

    if ( openAddLayoutPopup && ! ImGui::IsPopupOpen( "Add Layout" ) )
    {
        ImGui::OpenPopup( "Add Layout", ImGuiWindowFlags_AlwaysAutoResize );
    }

    const ImVec2 center( ImGui::GetIO().DisplaySize.x * 0.5f,
                         ImGui::GetIO().DisplaySize.y * 0.5f );

    ImGui::SetNextWindowPos( center, ImGuiCond_Appearing, ImVec2( 0.5f, 0.5f ) );

    if ( ImGui::BeginPopupModal( "Add Layout", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
    {
        ImGui::Text( "Please set the number of views in the new layout:" );

        if ( ImGui::InputInt( "Horizontal", &width ) )
        {
            width = std::max( width, 1 );
        }

        if ( ImGui::InputInt( "Vertical", &height ) )
        {
            height = std::max( height, 1 );
        }

        if ( width >= 5 && height >= 5 )
        {
            isLightbox = true;
        }

        ImGui::Checkbox( "Lightbox mode", &isLightbox );
        ImGui::SameLine();
        helpMarker( "Should all views in the layout share a common view type?" );
        ImGui::Separator();

        ImGui::SetNextItemWidth( -1.0f );

        if ( ImGui::Button( "OK", ImVec2( 80, 0 ) ) )
        {
            addLayout = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();

        ImGui::SameLine();
        if ( ImGui::Button( "Cancel", ImVec2( 80, 0 ) ) )
        {
            addLayout = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if ( addLayout )
    {
        // Apply offsets to views if using lightbox mode
        const bool offsetViews = ( isLightbox );

        auto& wd = appData.windowData();
        wd.addGridLayout( width, height, offsetViews, isLightbox );
        wd.setCurrentLayoutIndex( wd.numLayouts() - 1 );
        wd.setDefaultRenderedImagesForLayout( wd.currentLayout(), appData.imageUidsOrdered() );

        recenterViews();
    }
}


void renderAboutDialogModalPopup( bool open )
{
    static const std::string sk_gitInfo =
            std::string( "Git commit hash: " ) + ANTROPY_GIT_COMMIT_SHA1 + "\n" +
            std::string( "Git commit timestamp: " ) + ANTROPY_GIT_COMMIT_TIMESTAMP + "\n" +
            std::string( "Git branch: " ) + ANTROPY_GIT_BRANCH + "\n\n" +
            std::string( "Build timestamp: ") + ANTROPY_BUILD_TIMESTAMP + " (UTC)" + "\n" +
            std::string( "Build OS: ") + SYSTEM_NAME + " (" + SYSTEM_VERSION + ")\n" +
            std::string( "Build processor: ") + SYSTEM_PROCESSOR;

    if ( open && ! ImGui::IsPopupOpen( "About Antropy" ) )
    {
        ImGui::OpenPopup( "About Antropy", ImGuiWindowFlags_AlwaysAutoResize );
    }

    const ImVec2 center( ImGui::GetIO().DisplaySize.x * 0.5f,
                         ImGui::GetIO().DisplaySize.y * 0.5f );

    ImGui::SetNextWindowPos( center, ImGuiCond_Appearing, ImVec2( 0.5f, 0.5f ) );

    ImGui::SetNextWindowSize( ImVec2( 500.0f, 0 ) );

    if ( ImGui::BeginPopupModal( "About Antropy", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
    {
        ImGui::Text( "%s (version %s)", ANTROPY_APPNAME_FULL, ANTROPY_VERSION_FULL );

        ImGui::Spacing();
        ImGui::Text( "Copyright 2021" );
        ImGui::Text( "%s,", ANTROPY_ORGNAME_LINE1 );
        ImGui::Text( "%s,", ANTROPY_ORGNAME_LINE2 );
        ImGui::Text( "%s", ANTROPY_ORGNAME_LINE3 );

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Text( "Build information:" );

        ImGui::InputTextMultiline(
                    "##gitInfo",
                    const_cast<char*>( sk_gitInfo.c_str() ),
                    sk_gitInfo.length(),
                    ImVec2( -FLT_MIN, ImGui::GetTextLineHeight() * 8 ),
                    ImGuiInputTextFlags_ReadOnly );

        if ( ImGui::Button( "OK", ImVec2( 80, 0 ) ) )
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();

        ImGui::EndPopup();
    }
}
