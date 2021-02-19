#include "ui/Popups.h"
#include "ui/Helpers.h"

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
