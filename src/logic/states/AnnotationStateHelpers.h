#ifndef ANNOTATION_STATE_HELPERS_H
#define ANNOTATION_STATE_HELPERS_H

namespace state
{

/// Are annotation selections/highlights visible?
bool isInStateWhereAnnotationSelectionsAreVisible();

/// Are vertex selections/highlights visible?
bool isInStateWhereVertexSelectionsAreVisible();

/// Can views scroll in the current state?
bool isInStateWhereViewsCanScroll();

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

} // namespace state

#endif // ANNOTATION_STATE_HELPERS_H
