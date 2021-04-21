#ifndef FSMLIST_HPP
#define FSMLIST_HPP

#include "logic/states/AnnotationStateMachine.h"

#include <tinyfsm.hpp>


namespace state
{

using fsm_list = tinyfsm::FsmList< AnnotationStateMachine >;

/**
 * @brief Dispatch event to AnnotationStateMachine.
 * Can list other state machines in the template.
 */
template<typename E>
void send_event( const E& event )
{
    fsm_list::template dispatch<E>( event );
}

} // namespace state

#endif // FSMLIST_HPP
