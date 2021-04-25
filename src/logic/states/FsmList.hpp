#ifndef FSMLIST_HPP
#define FSMLIST_HPP

#include "logic/states/AnnotationStateMachine.h"

#include <tinyfsm.hpp>


namespace state
{

// If we have multiple state machines, then the FsmList can be used.
// This allows us to dispatch events to multiple machines.
//using fsm_list = tinyfsm::FsmList< AnnotationStateMachine >;

// We have only one state machine:
using fsm_list = AnnotationStateMachine;


/**
 * @brief Dispatch event to the state machine(s)
 */
template<typename E>
void send_event( const E& event )
{
    fsm_list::template dispatch<E>( event );
}

} // namespace state

#endif // FSMLIST_HPP
