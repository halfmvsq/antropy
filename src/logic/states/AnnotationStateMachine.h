#ifndef VIEW_SELECTION_STATE_MACHINE_H
#define VIEW_SELECTION_STATE_MACHINE_H

#include <tinyfsm.hpp>

#include <uuid.h>

#include <optional>


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
class AnnotationStateMachine :
        public tinyfsm::Fsm<AnnotationStateMachine>
{
    friend class tinyfsm::Fsm<AnnotationStateMachine>;

public:

    AnnotationStateMachine() = default;

    /// @note tinyfsm::Fsm is missing virtual destructor
    virtual ~AnnotationStateMachine() = default;

protected:

    /// Default reaction for unhandled events
    void react( const tinyfsm::Event& );

    virtual void react( const SelectViewEvent& ) {}
    virtual void react( const TurnOnAnnotationMode& ) {}
    virtual void react( const TurnOffAnnotationMode& ) {}

    /// Entry actions for states
    virtual void entry() {}

    /// Exit actions for states
    virtual void exit() {}

    std::optional<uuids::uuid> m_selectedViewUid = std::nullopt;
};



/*** Begin states of the machine ***/

/// @brief State where the user has turned annotating off
class AnnotationOffState : public AnnotationStateMachine
{
    void entry() override;
    void react( const TurnOnAnnotationMode& ) override;
};

/// @brief State where the user has turned annotating on,
/// but no view has yet been selected in which to annotate
class ViewSelectionState : public AnnotationStateMachine
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

/*** End states of the machine ***/

#endif // VIEW_SELECTION_STATE_MACHINE_H
