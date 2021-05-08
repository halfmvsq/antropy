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

    static void setAppData( AppData* appData ) { ms_appData = appData; }
    static AppData* appData() { return ms_appData; }

    /// Hovered (putatively selected) view UID
    static std::optional<uuids::uuid> hoveredViewUid() { return ms_hoveredViewUid; }

    /// Selected view UID, in which the user is currently annotating
    static std::optional<uuids::uuid> selectedViewUid() { return ms_selectedViewUid; }

    /// The active annotation that is being created and growing
    static std::optional<uuids::uuid> growingAnnotUid() { return ms_growingAnnotUid; }


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

    virtual void react( const TurnOnAnnotationModeEvent& ) {}
    virtual void react( const TurnOffAnnotationModeEvent& ) {}
    virtual void react( const CreateNewAnnotationEvent& ) {}
    virtual void react( const CompleteNewAnnotationEvent& ) {}
    virtual void react( const CloseNewAnnotationEvent& ) {}
    virtual void react( const UndoVertexEvent& ) {}
    virtual void react( const CancelNewAnnotationEvent& ) {}


    /***** Start functions commonly used in various states *****/

    /// @return false iff not ok
    bool checkViewSelection( const ViewHit& );

    /// @return false iff not ok
    bool checkAppData();

    void turnAnnotatingOff();
    void hoverOverView( const ViewHit& );
    void selectView( const ViewHit& );
    bool createNewGrowingAnnotation( const ViewHit& );
    bool addVertexToGrowingAnnotation( const ViewHit& );
    void completeGrowingAnnotation( bool closeAnnotation );
    void undoLastVertexOfGrowingAnnotation();
    void removeGrowingAnnotation();

    void selectVertex( const uuids::uuid& annotUid, size_t vertexIndex );
    void deselectVertex();

    void deselectAllAnnotationVertices();

    /// @return Vector of pairs of { annotationUid, vertex index }
    std::vector< std::pair<uuids::uuid, size_t> > findHitVertices( const ViewHit& );

    void highlightHoveredVertex( const ViewHit& );

    /***** End functions commonly used in various states *****/


    /// Hold a pointer to the application data object
    static AppData* ms_appData;

    /// Hovered (putatively selected) view UID
    static std::optional<uuids::uuid> ms_hoveredViewUid;

    /// Selected view UID, in which the user is currently annotating
    static std::optional<uuids::uuid> ms_selectedViewUid;

    /// The annotation that is being created and growing
    static std::optional<uuids::uuid> ms_growingAnnotUid;

    /// The annotation that is selected and being edited
    static std::optional<uuids::uuid> ms_selectedAnnotUid;

    /// The vertex index that is selected and being edited
    static std::optional<size_t> ms_selectedVertex;
};



/********** Begin state declarations **********/

/// @todo Create AnnotatingState: { Nothing, PickingPoint, MovingPoint, SelectingPoint }

/**
 * @brief State where annotating is turned off.
 */
class AnnotationOffState : public AnnotationStateMachine
{
    void entry() override;

    void react( const TurnOnAnnotationModeEvent& ) override;
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

    void react( const TurnOffAnnotationModeEvent& ) override;
};

/**
 * @brief State where the user has turned annotating on
 * and has also selected a view in which to perform annotation.
 * They are ready to edit existing annotations.
 */
/// @todo rename to ReadyState or IdleState
class ReadyToEditState : public AnnotationStateMachine
{
    void entry() override;
    void exit() override;

    void react( const MousePressEvent& ) override;
    void react( const MouseMoveEvent& ) override;
    void react( const MouseReleaseEvent& ) override;

    void react( const TurnOffAnnotationModeEvent& ) override;
    void react( const CreateNewAnnotationEvent& ) override;
};

/**
 * @brief State where the user has decided to create a new annotation.
 */
class CreatingNewAnnotationState : public AnnotationStateMachine
{
    void entry() override;
    void exit() override;

    void react( const MousePressEvent& ) override;
    void react( const MouseMoveEvent& ) override;
    void react( const MouseReleaseEvent& ) override;

    void react( const TurnOffAnnotationModeEvent& ) override;
    void react( const CompleteNewAnnotationEvent& ) override;
    void react( const CancelNewAnnotationEvent& ) override;
};

/**
 * @brief State where the user is adding vertices to the new annotation.
 */
class AddingVertexToNewAnnotationState : public AnnotationStateMachine
{
    void entry() override;
    void exit() override;

    void react( const MousePressEvent& ) override;
    void react( const MouseMoveEvent& ) override;
    void react( const MouseReleaseEvent& ) override;

    void react( const TurnOffAnnotationModeEvent& ) override;
    void react( const CompleteNewAnnotationEvent& ) override;
    void react( const CloseNewAnnotationEvent& ) override;
    void react( const UndoVertexEvent& ) override;
    void react( const CancelNewAnnotationEvent& ) override;
};

class VertexSelectedState : public AnnotationStateMachine
{
    void entry() override;
    void exit() override;

    void react( const MousePressEvent& ) override;
    void react( const MouseReleaseEvent& ) override;
    void react( const MouseMoveEvent& ) override;

    void react( const TurnOffAnnotationModeEvent& ) override;
};

/********** End state declarations **********/



/********** Start helper functions **********/

/// Can views scroll in the current state?
bool isInStateWhereViewsCanScroll();

/// Is the toolbar visible in the current state?
bool isInStateWhereToolbarVisible();

/// Check whether Annotation toolbar buttons are visible in the current state
bool showToolbarCreateButton(); // Create new annotation
bool showToolbarCompleteButton(); // Complete current annotation
bool showToolbarCloseButton(); // Close current annotation
bool showToolbarCancelButton(); // Cancel current annotation
bool showToolbarUndoButton(); // Undo last vertex

/********** End helper functions **********/

} // namespace state


// Shortcut for referring to the state machine:
using ASM = state::AnnotationStateMachine;

#endif // VIEW_SELECTION_STATE_MACHINE_H
