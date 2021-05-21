#ifndef ANNOTATION_STATE_HELPERS_H
#define ANNOTATION_STATE_HELPERS_H

namespace state
{

/// Are annotation selections/highlights visible?
bool isInStateWhereAnnotationHighlightsAreVisible();

/// Are vertex selections/highlights visible?
bool isInStateWhereVertexHighlightsAreVisible();

/// Can views scroll in the current state?
bool isInStateWhereViewsCanScroll();

/// Can crosshairs move with the mouse in the current state?
bool isInStateWhereCrosshairsCanMove();

/// Is the toolbar visible in the current state?
bool isInStateWhereToolbarVisible();

/// Are view highlights and selections visible in the current state?
bool isInStateWhereViewSelectionsVisible();

/// Check whether Annotation toolbar buttons are visible in the current state
bool showToolbarCreateButton(); // Create new annotation
bool showToolbarCompleteButton(); // Complete current annotation
bool showToolbarCloseButton(); // Close current annotation
bool showToolbarCancelButton(); // Cancel current annotation
bool showToolbarUndoButton(); // Undo last vertex
bool showToolbarInsertVertexButton(); // Insert vertex
bool showToolbarRemoveSelectedVertexButton(); // Remove selected vertex
bool showToolbarRemoveSelectedAnnotationButton(); // Remove selected annotation

} // namespace state

#endif // ANNOTATION_STATE_HELPERS_H
