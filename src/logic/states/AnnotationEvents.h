#ifndef ANNOTATION_EVENTS_H
#define ANNOTATION_EVENTS_H

#include "logic/interaction/ViewHit.h"
#include "logic/interaction/events/ButtonState.h"

#include <tinyfsm.hpp>


namespace state
{

/// MouseEvent is a base class for mouse press, release, and move events.
struct MouseEvent : public tinyfsm::Event
{
    MouseEvent( const ViewHit& h, const ButtonState& b, const ModifierState& m )
        : hit( h ), buttonState( b ), modifierState( m ) {}

    virtual ~MouseEvent() = default;

    const ViewHit hit; //!< View hit information for this event
    const ButtonState buttonState; //!< Mouse button state
    const ModifierState modifierState; //!< Keyboard modifier state
};


/// Mouse pointer pressed
struct MousePressEvent : public MouseEvent
{
    MousePressEvent( const ViewHit& h, const ButtonState& b, const ModifierState& m )
        : MouseEvent( h, b, m ) {}

    ~MousePressEvent() override = default;
};

/// Mouse pointer released
struct MouseReleaseEvent : public MouseEvent
{
    MouseReleaseEvent( const ViewHit& h, const ButtonState& b, const ModifierState& m )
        : MouseEvent( h, b, m ) {}

    ~MouseReleaseEvent() override = default;
};

/// Mouse pointer moved
struct MouseMoveEvent : public MouseEvent
{
    MouseMoveEvent( const ViewHit& h, const ButtonState& b, const ModifierState& m )
        : MouseEvent( h, b, m ) {}

    ~MouseMoveEvent() override = default;
};


/// User has turned on annotation mode: they want to create or edit annotations
struct TurnOnAnnotationModeEvent : public tinyfsm::Event {};

/// User has turned off annotation mode: they want to stop annotating
struct TurnOffAnnotationModeEvent : public tinyfsm::Event {};

/// User wants to create a new annotation
struct CreateNewAnnotationEvent : public tinyfsm::Event {};

/// User wants to complete the new annotation that is currently in progress
struct CompleteNewAnnotationEvent : public tinyfsm::Event {};

/// User wants to close the new annotation that is currently in progress
struct CloseNewAnnotationEvent : public tinyfsm::Event {};

/// User wants to undo the last annotation vertex that was created for the current
/// annotation in progress
struct UndoVertexEvent : public tinyfsm::Event {};

/// User wants to cancel creating the new annotation that is currently in progress
struct CancelNewAnnotationEvent : public tinyfsm::Event {};

} // namespace state

#endif // ANNOTATION_EVENTS_H
