#ifndef ANNOTATION_GROUP_H
#define ANNOTATION_GROUP_H

/// This class holds annotations that belong to the same subject plane.
/// This way, annotations on the same slice are group together in the UI.
/// It also makes searching for annotations by normal/distance (plane equation) faster, since all annotations
/// on the same slice are grouped.
/// Holds list of annotations in list. Order in list corresponds to rendering order (bottom to top).
/// @todo Put layering functions in here.

/// add/remove/modify annotation in group
/// if group becomes empty, then delete it

#endif // ANNOTATION_GROUP_H
