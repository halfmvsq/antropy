#include "logic/states/AnnotationStates.h"
#include "logic/states/AnnotationStateHelpers.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>


namespace state
{

void AnnotationOffState::entry()
{
    if ( ! ms_appData )
    {
        // The AppData pointer has not yet been set
        return;
    }

    ms_hoveredViewUid = std::nullopt;
    ms_selectedViewUid = std::nullopt;
    ms_growingAnnotUid = std::nullopt;

    unhoverAnnotation();
    deselect( true, false );
}

void AnnotationOffState::react( const TurnOnAnnotationModeEvent& )
{
    transit<ViewBeingSelectedState>();
}


/**************** ViewBeingSelectedState *******************/

void ViewBeingSelectedState::entry()
{
    ms_hoveredViewUid = std::nullopt;
    ms_selectedViewUid = std::nullopt;
    ms_growingAnnotUid = std::nullopt;
    unhoverAnnotation();
}

void ViewBeingSelectedState::react( const MousePressEvent& e )
{
    selectView( e.hit );
    transit<StandbyState>();
}

void ViewBeingSelectedState::react( const MouseMoveEvent& e )
{
    hoverView( e.hit );
}

void ViewBeingSelectedState::react( const MouseReleaseEvent& )
{
    /// @note If this is not call, the UI may not update until the next mouse event following the
    /// \c MousePressEvent that should trigger the UI change
    if ( ms_renderUiCallback ) ms_renderUiCallback();
}

void ViewBeingSelectedState::react( const TurnOffAnnotationModeEvent& )
{
    transit<AnnotationOffState>();
}


/**************** StandbyState *******************/

void StandbyState::entry()
{
    if ( ! ms_selectedViewUid )
    {
        spdlog::error( "Entered StandbyState without a selected view" );
        transit<ViewBeingSelectedState>();
        return;
    }

    ms_growingAnnotUid = std::nullopt;
    unhoverAnnotation();
}

void StandbyState::exit()
{
}

void StandbyState::react( const MousePressEvent& e )
{
    selectView( e.hit );

    if ( e.buttonState.left )
    {
        if ( selectAnnotationAndVertex( e.hit ) )
        {
            transit<VertexSelectedState>();
        }
    }
}

void StandbyState::react( const MouseReleaseEvent& /*e*/ )
{
    /// @note If this is not call, the UI may not update until the next mouse event following the
    /// \c MousePressEvent that should trigger the UI change
    if ( ms_renderUiCallback ) ms_renderUiCallback();
}

void StandbyState::react( const MouseMoveEvent& e )
{
    hoverView( e.hit );
    hoverAnnotationAndVertex( e.hit );
}

void StandbyState::react( const TurnOffAnnotationModeEvent& )
{
    transit<AnnotationOffState>();
}

void StandbyState::react( const CreateNewAnnotationEvent& )
{
    transit<CreatingNewAnnotationState>();
}


/**************** CreatingNewAnnotationState *******************/

void CreatingNewAnnotationState::entry()
{
    if ( ! ms_selectedViewUid )
    {
        spdlog::error( "Attempting to create a new annotation without a selected view" );
        transit<ViewBeingSelectedState>();
        return;
    }

    ms_growingAnnotUid = std::nullopt;
    unhoverAnnotation();
    deselect( true, true );
}

void CreatingNewAnnotationState::exit()
{
}

void CreatingNewAnnotationState::react( const MousePressEvent& e )
{
    if ( e.buttonState.left )
    {
        if ( createNewGrowingPolygon( e.hit ) &&
             addVertexToGrowingPolygon( e.hit ) )
        {
            transit<AddingVertexToNewAnnotationState>();
        }
    }
}

void CreatingNewAnnotationState::react( const MouseMoveEvent& e )
{
    hoverAnnotationAndVertex( e.hit );

    /*
    // Only create/edit points on the outer polygon boundary for now
    static constexpr size_t OUTER_BOUNDARY = 0;

    if ( ! ms_annotBeingCreatedUid )
    {
        spdlog::warn( "No active annotation for which to move first vertex" );
        return;
    }

    spdlog::trace( "ReadyToCreateState::react( const MousePressEvent& e )" );

    if ( ! checkAppData() ) return false;

    if ( ! ms_selectedViewUid )
    {
        spdlog::error( "No selected in which to create annotation" );
        transit<ViewBeingSelectedState>();
        return;
    }

    if ( *ms_selectedViewUid != e.hit.viewUid )
    {
        // Mouse pointer is not in the view selected for creating annotation
        spdlog::trace( "Mouse pointer is not in the view selected for creating annotation" );
        return;
    }
    */
}

void CreatingNewAnnotationState::react( const MouseReleaseEvent& /*e*/ )
{
    /// @note If this is not call, the UI may not update until the next mouse event following the
    /// \c MousePressEvent that should trigger the UI change
    if ( ms_renderUiCallback ) ms_renderUiCallback();
}

void CreatingNewAnnotationState::react( const TurnOffAnnotationModeEvent& )
{
    transit<AnnotationOffState>();
}

void CreatingNewAnnotationState::react( const CompleteNewAnnotationEvent& )
{
    static constexpr bool sk_doNotClosePolygon = false;
    completeGrowingPoylgon( sk_doNotClosePolygon );
}

void CreatingNewAnnotationState::react( const CancelNewAnnotationEvent& )
{
    removeGrowingPolygon();
}


/**************** AddingVertexToNewAnnotationState *******************/

void AddingVertexToNewAnnotationState::entry()
{
    if ( ! ms_selectedViewUid )
    {
        spdlog::error( "Entered AddingVertexToNewAnnotationState without a selected view" );
        transit<ViewBeingSelectedState>();
        return;
    }

    if ( ! ms_growingAnnotUid )
    {
        spdlog::error( "Entered AddingVertexToNewAnnotationState without "
                       "an annotation having been created" );
        transit<CreatingNewAnnotationState>();
        return;
    }
}

void AddingVertexToNewAnnotationState::exit()
{
    ms_growingAnnotUid = std::nullopt;
}

void AddingVertexToNewAnnotationState::react( const MousePressEvent& e )
{
    if ( e.buttonState.left )
    {
        addVertexToGrowingPolygon( e.hit );
    }
}

void AddingVertexToNewAnnotationState::react( const MouseMoveEvent& e )
{
    hoverAnnotationAndVertex( e.hit );

    if ( e.buttonState.left )
    {
        addVertexToGrowingPolygon( e.hit );
    }
}

void AddingVertexToNewAnnotationState::react( const MouseReleaseEvent& /*e*/ )
{
    /// @note If this is not call, the UI may not update until the next mouse event following the
    /// \c MousePressEvent that should trigger the UI change
    if ( ms_renderUiCallback ) ms_renderUiCallback();
}

void AddingVertexToNewAnnotationState::react( const TurnOffAnnotationModeEvent& )
{
    transit<AnnotationOffState>();
}

void AddingVertexToNewAnnotationState::react( const CompleteNewAnnotationEvent& )
{
    static constexpr bool sk_doNotClosePolygon = false;
    completeGrowingPoylgon( sk_doNotClosePolygon );
}

void AddingVertexToNewAnnotationState::react( const CloseNewAnnotationEvent& )
{
    static constexpr bool sk_closePolygon = true;
    completeGrowingPoylgon( sk_closePolygon );
}

void AddingVertexToNewAnnotationState::react( const UndoVertexEvent& )
{
    undoLastVertexOfGrowingPolygon();
}

void AddingVertexToNewAnnotationState::react( const CancelNewAnnotationEvent& )
{
    removeGrowingPolygon();
}


/**************** VertexSelectedState *******************/

void VertexSelectedState::entry()
{
//    spdlog::debug( "enter selected" );
}

void VertexSelectedState::exit()
{
//    spdlog::debug( "exit selected" );
}

void VertexSelectedState::react( const MousePressEvent& e )
{
    if ( e.buttonState.left )
    {
        if ( selectAnnotationAndVertex( e.hit ) )
        {
            transit<VertexSelectedState>();
        }
        else
        {
            transit<StandbyState>();
        }
    }
}

void VertexSelectedState::react( const MouseReleaseEvent& /*e*/ )
{
    /// @note If this is not call, the UI may not update until the next mouse event following the
    /// \c MousePressEvent that should trigger the UI change
    if ( ms_renderUiCallback ) ms_renderUiCallback();
}

void VertexSelectedState::react( const MouseMoveEvent& e )
{
    hoverAnnotationAndVertex( e.hit );
}

void VertexSelectedState::react( const TurnOffAnnotationModeEvent& )
{
    transit<AnnotationOffState>();
}

void VertexSelectedState::react( const RemoveSelectedVertexEvent& )
{
    removeSelectedVertex();
}

} // namespace state
