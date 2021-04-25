#include "logic/states/AnnotationStateMachine.h"
#include "logic/states/FsmList.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>


namespace state
{

std::optional<uuids::uuid> AnnotationStateMachine::m_selectedViewUid = std::nullopt;


void AnnotationStateMachine::react( const tinyfsm::Event& )
{
    spdlog::warn( "Unhandled event sent to AnnotationStateMachine" );
}


void AnnotationOffState::entry()
{
    spdlog::trace( "AnnotationOff::entry()" );
}

void AnnotationOffState::react( const TurnOnAnnotationMode& )
{
    spdlog::trace( "AnnotationOffState::react( const TurnOnAnnotationMode& )::entry()" );
    transit<ViewBeingSelectedState>();
}


void ViewBeingSelectedState::entry()
{
    spdlog::trace( "ViewBeingSelectedState::entry()" );
}

void ViewBeingSelectedState::react( const MousePressEvent& e )
{
    spdlog::trace( "ViewBeingSelectedState::react( const SelectViewEvent& e )" );
    m_selectedViewUid = e.hit.viewUid;
    transit<ViewSelectedState>();
}

void ViewBeingSelectedState::react( const TurnOffAnnotationMode& )
{
    spdlog::trace( "ViewBeingSelectedState::react( const TurnOffAnnotationMode& )" );
    m_selectedViewUid = std::nullopt;
    transit<AnnotationOffState>();
}


void ViewSelectedState::entry()
{
    spdlog::trace( "ViewSelectedState::entry()" );

    if ( m_selectedViewUid )
    {
        spdlog::trace( "Selected view {} for annotating", *m_selectedViewUid );
    }
    else
    {
        spdlog::error( "Entered ViewSelectedState without a selected view" );
        send_event( TurnOffAnnotationMode() );
    }
}

void ViewSelectedState::exit()
{
    spdlog::trace( "ViewSelectedState::exit()" );
    m_selectedViewUid = std::nullopt;
}

void ViewSelectedState::react( const MousePressEvent& e )
{
    spdlog::trace( "ViewSelectedState::react( const SelectViewEvent& e )" );
    m_selectedViewUid = e.hit.viewUid;
}

void ViewSelectedState::react( const TurnOffAnnotationMode& )
{
    spdlog::trace( "ViewSelectedState::react( const TurnOffAnnotationMode& )" );
    m_selectedViewUid = std::nullopt;
    transit<AnnotationOffState>();
}

} // namespace state


/// Initial state definition
FSM_INITIAL_STATE( state::AnnotationStateMachine, state::AnnotationOffState )
