#ifndef VIEW_SELECTION_STATE_MACHINE_H
#define VIEW_SELECTION_STATE_MACHINE_H

#include "logic/interaction/ViewHit.h"
#include "logic/interaction/events/ButtonState.h"

#include <tinyfsm.hpp>
#include <uuid.h>
#include <optional>

class AppData;


namespace state
{

/********** Begin event declarations **********/

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


/// User has turned on annotation mode: they want to create/modify annotations
struct TurnOnAnnotationMode : public tinyfsm::Event {};

/// User has turned off annotation mode: they do not want to annotate
struct TurnOffAnnotationMode : public tinyfsm::Event {};

/********** End event declarations **********/



/**
 * @brief State machine for annotations
 *
 * @note Access the current sate with current_state_ptr()
 * @note Check if in state with is_in_state<STATE>()
 */
class AnnotationStateMachine : public tinyfsm::Fsm<AnnotationStateMachine>
{
    friend class tinyfsm::Fsm<AnnotationStateMachine>;

public:

    AnnotationStateMachine() = default;
    virtual ~AnnotationStateMachine() = default;

    void setAppData( AppData* appData )
    {
        m_appData = appData;
    }

    /// Hovered (putatively selected) view UID
    static std::optional<uuids::uuid> m_hoveredViewUid;

    /// Selected view UID, in which the user is currently annotating
    static std::optional<uuids::uuid> m_selectedViewUid;


protected:

    /// Default action when entering a state
    virtual void entry() {}

    /// Default action when exiting a state
    virtual void exit() {}

    /// Default reaction for unhandled events
    void react( const tinyfsm::Event& );

    /// Default reactions for handled events
    virtual void react( const MousePressEvent& ) {}
    virtual void react( const MouseReleaseEvent& ) {}
    virtual void react( const MouseMoveEvent& ) {}

    virtual void react( const TurnOnAnnotationMode& ) {}
    virtual void react( const TurnOffAnnotationMode& ) {}

    AppData* m_appData = nullptr;
};



/********** Begin state declarations **********/

/// @todo Create AnnotatingState: { Nothing, PickingPoint, MovingPoint, SelectingPoint }

/**
 * @brief State where annotating is turned off.
 */
class AnnotationOffState : public AnnotationStateMachine
{
    void entry() override;

    void react( const TurnOnAnnotationMode& ) override;
};

/**
 * @brief State where the user has turned annotating on,
 * but no view has yet been selected in which to annotate.
 */
class ViewBeingSelectedState : public AnnotationStateMachine
{
    void entry() override;

    void react( const MousePressEvent& ) override;
    void react( const MouseMoveEvent& ) override;
    void react( const TurnOffAnnotationMode& ) override;
};

/**
 * @brief State where the user has turned annotating on
 * and has also selected a view in which to perform annotation.
 * They are ready to create and edit annotations.
 */
class ReadyState : public AnnotationStateMachine
{
    void entry() override;
    void exit() override;

    void react( const MousePressEvent& ) override;
    void react( const MouseReleaseEvent& ) override;
    void react( const MouseMoveEvent& ) override;
    void react( const TurnOffAnnotationMode& ) override;
};

/**
 * @brief State where the user is actively annotating.
 */
//class ReadyState : public AnnotationStateMachine
//{
//    void entry() override;
//    void exit() override;

//    void react( const MousePressEvent& ) override;
//    void react( const MouseReleaseEvent& ) override;
//    void react( const MouseMoveEvent& ) override;
//    void react( const TurnOffAnnotationMode& ) override;
//};

class VertexSelectedState : public AnnotationStateMachine
{
    void entry() override;
    void exit() override;

    void react( const MousePressEvent& ) override;
    void react( const MouseReleaseEvent& ) override;
    void react( const MouseMoveEvent& ) override;
    void react( const TurnOffAnnotationMode& ) override;
};

/********** End state declarations **********/


/********** Start helper functions **********/

/// Can views scroll in the current state?
bool isInStateWhereViewsCanScroll();

/// Is the toolbar visible in the current state?
bool isInStateWhereToolbarVisible();

/********** End helper functions **********/

} // namespace state


// Shortcut for referring to the state machine:
using ASM = state::AnnotationStateMachine;

#endif // VIEW_SELECTION_STATE_MACHINE_H
