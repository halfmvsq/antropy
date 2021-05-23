#include "logic/states/AnnotationStateHelpers.h"
#include "logic/states/AnnotationStateMachine.h"
#include "logic/states/AnnotationStates.h"

#include "common/DataHelper.h"

#include "logic/annotation/Annotation.h"
#include "logic/app/Data.h"


namespace state
{

bool isInStateWhereAnnotationHighlightsAreVisible()
{
    if ( ASM::is_in_state<StandbyState>() ||
         ASM::is_in_state<VertexSelectedState>() )
    {
        return true;
    }
    return false;
}

bool isInStateWhereVertexHighlightsAreVisible()
{
    if ( isInStateWhereAnnotationHighlightsAreVisible() ||
         ASM::is_in_state<CreatingNewAnnotationState>() ||
         ASM::is_in_state<AddingVertexToNewAnnotationState>() )
    {
        return true;
    }
    return false;
}

bool isInStateWhereViewsCanScroll()
{
    if ( ASM::is_in_state<AnnotationOffState>() ||
         ASM::is_in_state<ViewBeingSelectedState>() ||
         ASM::is_in_state<StandbyState>() ||
         ASM::is_in_state<CreatingNewAnnotationState>() ||
         ASM::is_in_state<VertexSelectedState>() )
    {
        return true;
    }
    return false;
}

/**
 * @todo There are many edge cases to capture here.
 * For now, crosshairs movement is disabled while annotating.
  */
bool isInStateWhereCrosshairsCanMove()
{
    if ( ASM::is_in_state<AnnotationOffState>() )
    {
        return true;
    }
    return false;
}

bool isInStateWhereToolbarVisible()
{
    if ( ASM::is_in_state<AnnotationOffState>() ||
         ASM::is_in_state<ViewBeingSelectedState>() )
    {
        return false;
    }
    return true;
}

bool isInStateWhereViewSelectionsVisible()
{
    if ( ASM::is_in_state<AnnotationOffState>() )
    {
        return false;
    }
    return true;
}

bool showToolbarCreateButton()
{
    if ( ASM::is_in_state<StandbyState>() ||
         ASM::is_in_state<VertexSelectedState>() )
    {
        return true;
    }
    return false;
}

bool showToolbarCompleteButton()
{
    if ( ! ASM::growingAnnotUid() )
    {
        return false;
    }

    // Need a valid annotation with at least 1 vertex in order to complete it:
    Annotation* annot = ASM::appData()->annotation( *ASM::growingAnnotUid() );

    if ( ! annot )
    {
        return false;
    }

    if ( 0 == annot->polygon().numVertices() )
    {
        return false;
    }

    if ( ASM::is_in_state<CreatingNewAnnotationState>() ||
         ASM::is_in_state<AddingVertexToNewAnnotationState>() )
    {
        return true;
    }

    return false;
}

bool showToolbarCloseButton()
{
    if ( ! ASM::growingAnnotUid() )
    {
        return false;
    }

    // Need a valid annotation with at least 3 vertices in order to close it:
    Annotation* annot = ASM::appData()->annotation( *ASM::growingAnnotUid() );
    if ( ! annot )
    {
        return false;
    }

    if ( annot->polygon().numVertices() < 3 )
    {
        return false;
    }

    if ( ASM::is_in_state<CreatingNewAnnotationState>() ||
         ASM::is_in_state<AddingVertexToNewAnnotationState>() )
    {
        return true;
    }

    return false;
}

bool showToolbarFillButton()
{
    if ( ASM::is_in_state<StandbyState>() ||
         ASM::is_in_state<VertexSelectedState>() )
    {
        if ( ! ASM::appData() ) return false;
        const auto selectedAnnotUid = data::getSelectedAnnotation( *ASM::appData() );

        if ( const Annotation* annot = ASM::appData()->annotation( *selectedAnnotUid ) )
        {
            // Only show fill button for closed, non-smoothed annotation polygons.
            /// @todo Implement algorithm for filling smoothed polygons.
            return ( annot->isClosed() && ! annot->isSmoothed() );
        }
    }
    return false;
}

bool showToolbarUndoButton()
{
    return showToolbarCompleteButton();
}

bool showToolbarCancelButton()
{
    if ( ASM::is_in_state<CreatingNewAnnotationState>() ||
         ASM::is_in_state<AddingVertexToNewAnnotationState>() )
    {
        return true;
    }
    return false;
}

bool showToolbarInsertVertexButton()
{
    if ( ASM::is_in_state<VertexSelectedState>() )
    {
        return true;
    }
    return false;
}

bool showToolbarRemoveSelectedVertexButton()
{
    if ( ASM::is_in_state<VertexSelectedState>() )
    {
        return true;
    }
    return false;
}

bool showToolbarRemoveSelectedAnnotationButton()
{
    if ( ASM::is_in_state<StandbyState>() ||
         ASM::is_in_state<VertexSelectedState>() )
    {
        return ( ASM::appData() && data::getSelectedAnnotation( *ASM::appData() ) );
    }
    return false;
}

bool showToolbarCutSelectedAnnotationButton()
{
    if ( ASM::is_in_state<StandbyState>() ||
         ASM::is_in_state<VertexSelectedState>() )
    {
        return ( ASM::appData() && data::getSelectedAnnotation( *ASM::appData() ) );
    }
    return false;
}

bool showToolbarCopySelectedAnnotationButton()
{
    if ( ASM::is_in_state<StandbyState>() ||
         ASM::is_in_state<VertexSelectedState>() )
    {
        return ( ASM::appData() && data::getSelectedAnnotation( *ASM::appData() ) );
    }
    return false;
}

bool showToolbarPasteSelectedAnnotationButton()
{
    if ( ASM::is_in_state<StandbyState>() ||
         ASM::is_in_state<VertexSelectedState>() )
    {
        return true;
    }
    return false;
}

} // namespace state
