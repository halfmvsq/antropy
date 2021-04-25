#ifndef VIEW_SELECTION_STATE_MACHINE_H
#define VIEW_SELECTION_STATE_MACHINE_H

#include "logic/interaction/ViewHit.h"

#include <tinyfsm.hpp>
#include <uuid.h>

#include <memory>
#include <optional>

class AppData;


namespace state
{

/*** Begin event declarations ***/

struct MousePressEvent : public tinyfsm::Event
{
    MousePressEvent( const ViewHit& h ) : hit( h ) {}
    const ViewHit& hit; //!< View hit information for this event
};

/// User has turned on annotation mode: they want to create/modify annotations
struct TurnOnAnnotationMode : public tinyfsm::Event {};

/// User has turned off annotation mode: they do not want to annotate
struct TurnOffAnnotationMode : public tinyfsm::Event {};

/*** End event declarations ***/



/**
 * @brief State machine for annotation
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

    const std::optional<uuids::uuid>& selectedViewUid() const
    {
        return m_selectedViewUid;
    }


protected:

    /// Default action when entering a state
    virtual void entry() {}

    /// Default action when exiting a state
    virtual void exit() {}

    /// Default reaction for unhandled events
    void react( const tinyfsm::Event& );

    /// Default reactions for handled events
    virtual void react( const MousePressEvent& ) {}
    virtual void react( const TurnOnAnnotationMode& ) {}
    virtual void react( const TurnOffAnnotationMode& ) {}

    AppData* m_appData = nullptr;

    /// Selected view UID
    static std::optional<uuids::uuid> m_selectedViewUid;
};



/*** Begin state declarations ***/

/// @todo Create AnnotatingState: { Nothing, PickingPoint, MovingPoint, SelectingPoint }

/**
 * @brief State where the user has turned annotating off
 */
class AnnotationOffState : public AnnotationStateMachine
{
    void entry() override;

    void react( const TurnOnAnnotationMode& ) override;
};

/**
 * @brief State where the user has turned annotating on,
 * but no view has yet been selected in which to annotate
 */
class ViewBeingSelectedState : public AnnotationStateMachine
{
    void entry() override;

    void react( const MousePressEvent& ) override;
    void react( const TurnOffAnnotationMode& ) override;
};

/**
 * @brief State where the user has turned annotating on
 * and has also selected a view in which to perform annotation
 */
class ViewSelectedState : public AnnotationStateMachine
{
    void entry() override;
    void exit() override;

    void react( const MousePressEvent& ) override;
    void react( const TurnOffAnnotationMode& ) override;
};

/*** End state declarations ***/

} // namespace state

using ASM = state::AnnotationStateMachine;

#endif // VIEW_SELECTION_STATE_MACHINE_H
