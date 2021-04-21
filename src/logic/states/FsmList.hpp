#ifndef FSMLIST_HPP
#define FSMLIST_HPP

#include "logic/states/AnnotationStateMachine.h"

#include <tinyfsm.hpp>


namespace state
{

//using fsm_list = tinyfsm::FsmList< AnnotationStateMachine >;
using fsm_list = AnnotationStateMachine;

/**
 * @brief Dispatch event to AnnotationStateMachine.
 *
 * @note We can list other state machines in the template in order to
 * dispatch events to them.
 */
template<typename E>
void send_event( const E& event )
{
    fsm_list::template dispatch<E>( event );
}

} // namespace state

#endif // FSMLIST_HPP
