#include "logic/states/AnnotationStateMachine.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>


void AnnotationStateMachine::react( const tinyfsm::Event& )
{
    spdlog::warn( "Unhandled event sent to AnnotationViewSelectionStateMachine" );
}


void AnnotationOffState::entry()
{
    spdlog::trace( "AnnotationOff::entry()" );
}

void AnnotationOffState::react( const TurnOnAnnotationMode& )
{
    transit<ViewSelectionState>();
}


void ViewSelectionState::entry()
{
    spdlog::trace( "AnnotationOn_viewIsNotSelected::entry()" );
}

void ViewSelectionState::react( const SelectViewEvent& e )
{
    m_selectedViewUid = e.selectedViewUid;
    transit<ViewSelectedState>();
}

void ViewSelectionState::react( const TurnOffAnnotationMode& )
{
    m_selectedViewUid = std::nullopt;
    transit<AnnotationOffState>();
}


void ViewSelectedState::entry()
{
    spdlog::trace( "AnnotationOn_viewIsSelected::entry()" );

    if ( m_selectedViewUid )
    {
        spdlog::trace( "Selected view is {}", *m_selectedViewUid );
    }
    else
    {
        //ERRROR
    }
}

void ViewSelectedState::react( const TurnOffAnnotationMode& )
{
    m_selectedViewUid = std::nullopt;
    transit<AnnotationOffState>();
}

/// Initial state definition
FSM_INITIAL_STATE( AnnotationStateMachine, AnnotationOffState )
