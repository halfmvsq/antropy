#include "logic/states/AnnotationStateHelpers.h"
#include "logic/states/AnnotationStateMachine.h"
#include "logic/states/AnnotationStates.h"

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
    if ( ASM::is_in_state<StandbyState>() ||
         ASM::is_in_state<CreatingNewAnnotationState>() ||
         ASM::is_in_state<AddingVertexToNewAnnotationState>() ||
         ASM::is_in_state<VertexSelectedState>() )
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
         ASM::is_in_state<CreatingNewAnnotationState>() )
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
    if ( ASM::is_in_state<StandbyState>() )
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

bool showToolbarRemoveSelectedVertexButton()
{
    if ( ASM::is_in_state<VertexSelectedState>() )
    {
        return true;
    }
    return false;
}

} // namespace state
