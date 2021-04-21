#ifndef VIEW_SELECTION_STATE_MACHINE_H
#define VIEW_SELECTION_STATE_MACHINE_H

#include <tinyfsm.hpp>
#include <uuid.h>
#include <optional>


namespace state
{

/*** Begin event declarations ***/

/// User has selected a view to do annotation in
struct SelectViewEvent : public tinyfsm::Event
{
    /// Uid of the view that was selected
    uuids::uuid selectedViewUid;
};

/// User has turned on annotation mode: they want to annotate
struct TurnOnAnnotationMode : public tinyfsm::Event {};

/// User has turned off annotation mode: they do not want to annotate
struct TurnOffAnnotationMode : public tinyfsm::Event {};

/*** End event declarations ***/



/**
 * @brief State machine for annotation view selection
 */
class AnnotationStateMachine : public tinyfsm::Fsm<AnnotationStateMachine>
{
    friend class tinyfsm::Fsm<AnnotationStateMachine>;

public:

    AnnotationStateMachine() = default;
    virtual ~AnnotationStateMachine() = default;

protected:

    /// Default reaction for unhandled events
    void react( const tinyfsm::Event& );

    /// Default reactions for handled events
    virtual void react( const SelectViewEvent& ) {}
    virtual void react( const TurnOnAnnotationMode& ) {}
    virtual void react( const TurnOffAnnotationMode& ) {}

    /// Default action when entering a state
    virtual void entry() {}

    /// Default action when exiting a state
    virtual void exit() {}

    /// Selected view UID
    static std::optional<uuids::uuid> m_selectedViewUid;
};



/*** Begin state declarations ***/

/// @brief State where the user has turned annotating off
class AnnotationOffState : public AnnotationStateMachine
{
    void entry() override;
    void react( const TurnOnAnnotationMode& ) override;
};

/// @brief State where the user has turned annotating on,
/// but no view has yet been selected in which to annotate
class ViewBeingSelectedState : public AnnotationStateMachine
{
    void entry() override;
    void react( const SelectViewEvent& ) override;
    void react( const TurnOffAnnotationMode& ) override;
};

/// @brief State where the user has turned annotating on
/// and has also selected a view in which to perform annotation
class ViewSelectedState : public AnnotationStateMachine
{
    void entry() override;
    void react( const TurnOffAnnotationMode& ) override;
};

/*** End state declarations ***/

} // namespace state

#endif // VIEW_SELECTION_STATE_MACHINE_H
