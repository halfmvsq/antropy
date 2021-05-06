#include "logic/states/AnnotationStateMachine.h"
#include "logic/states/FsmList.hpp"
#include "logic/app/Data.h"
#include "logic/camera/MathUtility.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>


namespace state
{

AppData* ASM::ms_appData = nullptr;
std::optional<uuids::uuid> ASM::ms_hoveredViewUid = std::nullopt;
std::optional<uuids::uuid> ASM::ms_selectedViewUid = std::nullopt;
std::optional<uuids::uuid> ASM::ms_growingAnnotUid = std::nullopt;


void AnnotationStateMachine::react( const tinyfsm::Event& )
{
    spdlog::warn( "Unhandled event sent to AnnotationStateMachine" );
}

void AnnotationStateMachine::turnAnnotatingOff()
{
    ms_selectedViewUid = std::nullopt;
    ms_hoveredViewUid = std::nullopt;
    ms_growingAnnotUid = std::nullopt;
    transit<AnnotationOffState>();
}

void AnnotationStateMachine::hoverOverView( const ViewHit& hit )
{
    if ( ms_selectedViewUid && ( *ms_selectedViewUid != hit.viewUid ) )
    {
        ms_hoveredViewUid = hit.viewUid;
    }
    else if ( ! ms_selectedViewUid )
    {
        ms_hoveredViewUid = hit.viewUid;
    }
}

void AnnotationStateMachine::selectView( const ViewHit& hit )
{
    ms_hoveredViewUid = std::nullopt;
    ms_selectedViewUid = hit.viewUid;
}

void AnnotationStateMachine::addVertexToGrowingAnnotation( const ViewHit& hit )
{
    // Only create/edit points on the outer polygon boundary for now
    static constexpr size_t OUTER_BOUNDARY = 0;

    spdlog::trace( "addVertexToActiveAnnotation" );

    if ( ! ms_appData )
    {
        spdlog::error( "AppData is null when growing annotation" );
        transit<AnnotationOffState>();
        return;
    }

    if ( ! ms_selectedViewUid )
    {
        spdlog::error( "No selected view in which to grow annotation" );
        transit<AnnotationOffState>();
        return;
    }

    if ( *ms_selectedViewUid != hit.viewUid )
    {
        // Mouse pointer is not in the view selected for expanding annotation
        spdlog::trace( "Mouse pointer is not in the view selected for growing the annotation" );
        return;
    }

    if ( ! ms_growingAnnotUid )
    {
        spdlog::error( "addVertexToGrowingAnnotation without a growing annotation" );
        transit<AnnotationOffState>();
        return;
    }

    // Annotate on the active image
    const auto activeImageUid = ms_appData->activeImageUid();
    const Image* activeImage = ( activeImageUid ? ms_appData->image( *activeImageUid ) : nullptr );

    if ( ! activeImage )
    {
        spdlog::warn( "There is no active image when attempting to annotate" );
        return;
    }

    if ( 0 == std::count( std::begin( hit.view->visibleImages() ),
                          std::end( hit.view->visibleImages() ),
                          *activeImageUid ) )
    {
        // The active image is not visible
        return;
    }

    // Ok: the pointer is in the view bounds and we're good to go. Make this the active view.
    /// @todo Is this needed?
    ms_appData->windowData().setActiveViewUid( hit.viewUid );

    // Compute the plane equation in Subject space. Use the World position after the view
    // offset has been applied, so that the user can annotate in any view of a lightbox layout.
    const auto [subjectPlaneEquation, subjectPlanePoint] =
            math::computeSubjectPlaneEquation(
                activeImage->transformations().subject_T_worldDef(),
                -hit.worldFrontAxis, glm::vec3{ hit.worldPos_offsetApplied } );

    // Get the annotation
    Annotation* annot = ms_appData->annotation( *ms_growingAnnotUid );
    if ( ! annot )
    {
        spdlog::error( "Null annotation {}", *ms_growingAnnotUid );
        return;
    }

    const auto projectedPoint = annot->addSubjectPointToBoundary(
                OUTER_BOUNDARY, subjectPlanePoint );

    if ( ! projectedPoint )
    {
        spdlog::error( "Unable to add point {} to annotation {}",
                       glm::to_string( hit.worldPos_offsetApplied ),
                       *ms_growingAnnotUid );
    }
}



/**************** AnnotationOffState *******************/

void AnnotationOffState::entry()
{
//    spdlog::trace( "AnnotationOff::entry()" );
}

void AnnotationOffState::react( const TurnOnAnnotationModeEvent& )
{
//    spdlog::trace( "AnnotationOffState::react( const TurnOnAnnotationMode& )::entry()" );
    transit<ViewBeingSelectedState>();
}



/**************** ViewBeingSelectedState *******************/

void ViewBeingSelectedState::entry()
{
//    spdlog::trace( "ViewBeingSelectedState::entry()" );
}

void ViewBeingSelectedState::react( const MousePressEvent& e )
{
//    spdlog::trace( "ViewBeingSelectedState::react( const MousePressEvent& e )" );
    selectView( e.hit );
    transit<ReadyToEditState>();
}

void ViewBeingSelectedState::react( const MouseMoveEvent& e )
{
//    spdlog::trace( "ViewBeingSelectedState::react( const MouseMoveEvent& e )" );
    hoverOverView( e.hit );
}

void ViewBeingSelectedState::react( const TurnOffAnnotationModeEvent& )
{
//    spdlog::trace( "ViewBeingSelectedState::react( const TurnOffAnnotationMode& )" );
    turnAnnotatingOff();
}



/**************** ReadyToEditState *******************/

void ReadyToEditState::entry()
{
//    spdlog::trace( "ReadyToEditState::entry()" );

    if ( ! ms_selectedViewUid )
    {
        spdlog::error( "Entered ReadyToEditState without a selected view" );
        transit<ViewBeingSelectedState>();
        return;
    }
    else
    {
//        spdlog::trace( "Selected view {} for editing annotations", *ms_selectedViewUid );
    }
}

void ReadyToEditState::exit()
{
//    spdlog::trace( "ReadyToEditState::exit()" );
}

void ReadyToEditState::react( const MousePressEvent& e )
{
//    spdlog::trace( "ReadyToEditState::react( const MousePressEvent& e )" );
    selectView( e.hit );
}

void ReadyToEditState::react( const MouseReleaseEvent& /*e*/ )
{
    spdlog::trace( "ReadyToEditState::react( const MouseReleaseEvent& e )" );
}

void ReadyToEditState::react( const MouseMoveEvent& e )
{
//    spdlog::trace( "ReadyToEditState::react( const MouseMoveEvent& e )" );
    hoverOverView( e.hit );
}

void ReadyToEditState::react( const TurnOffAnnotationModeEvent& )
{
//    spdlog::trace( "ReadyToEditState::react( const TurnOffAnnotationMode& )" );
    turnAnnotatingOff();
}

void ReadyToEditState::react( const CreateNewAnnotationEvent& )
{
    spdlog::trace( "ReadyToEditState::react( const CreateNewAnnotation& )" );
    transit<CreatingNewAnnotationState>();
}



/**************** CreatingNewAnnotationState *******************/

void CreatingNewAnnotationState::entry()
{
    spdlog::trace( "ReadyToCreateState::entry()" );

    if ( ms_selectedViewUid )
    {
        spdlog::trace( "Selected view {} for creating annotation", *ms_selectedViewUid );
    }
    else
    {
        spdlog::error( "Entered ReadyToCreateState without a selected view" );
        transit<ViewBeingSelectedState>();
        return;
    }

    ms_growingAnnotUid = std::nullopt;
}

void CreatingNewAnnotationState::exit()
{
    spdlog::trace( "ReadyToCreateState::exit()" );
}

/**
 * @brief ReadyToCreateState::react Create a new annotation with its first vertex
 * @param e
 */
void CreatingNewAnnotationState::react( const MousePressEvent& e )
{
    // Only create/edit points on the outer polygon boundary for now
    static constexpr size_t OUTER_BOUNDARY = 0;

    spdlog::trace( "ReadyToCreateState::react( const MousePressEvent& e )" );

    if ( ! ms_appData )
    {
        spdlog::error( "AppData is null" );
        transit<AnnotationOffState>();
        return;
    }

    if ( ! ms_selectedViewUid )
    {
        spdlog::error( "No selected in which to create annotation" );
        transit<ViewBeingSelectedState>();
        return;
    }

    if ( *ms_selectedViewUid != e.hit.viewUid )
    {
        // Mouse pointer is not in the view selected for creating annotation
        spdlog::trace( "Mouse pointer is not in the view selected for creating annotation" );
        return;
    }

    // Annotate on the active image
    const auto activeImageUid = ms_appData->activeImageUid();
    const Image* activeImage = ( activeImageUid ? ms_appData->image( *activeImageUid ) : nullptr );

    if ( ! activeImage )
    {
        spdlog::warn( "There is no active image when attempting to annotate" );
        return;
    }

    if ( 0 == std::count( std::begin( e.hit.view->visibleImages() ),
                          std::end( e.hit.view->visibleImages() ),
                          *activeImageUid ) )
    {
        // The active image is not visible
        return;
    }

    // Ok: the pointer is in the view bounds and we're good to go. Make this the active view.
    /// @todo Is this needed?
    ms_appData->windowData().setActiveViewUid( e.hit.viewUid );

    // Compute the plane equation in Subject space. Use the World position after the view
    // offset has been applied, so that the user can annotate in any view of a lightbox layout.
    const auto [subjectPlaneEquation, subjectPlanePoint] =
            math::computeSubjectPlaneEquation(
                activeImage->transformations().subject_T_worldDef(),
                -e.hit.worldFrontAxis, glm::vec3{ e.hit.worldPos_offsetApplied } );

    // Create new annotation for this image:
    std::optional<uuids::uuid> annotUid;

    try
    {
        const std::string name = std::string( "Annotation " ) +
                std::to_string( ms_appData->annotationsForImage( *activeImageUid ).size() );

        const glm::vec4 color{ activeImage->settings().borderColor(), 1.0f };

        annotUid = ms_appData->addAnnotation(
                    *activeImageUid, Annotation( name, color, subjectPlaneEquation ) );

        if ( ! annotUid )
        {
            spdlog::error( "Unable to add new annotation (subject plane: {}) for image {}",
                           glm::to_string( subjectPlaneEquation ), *activeImageUid );
            return;
        }

        ms_appData->assignActiveAnnotationUidToImage( *activeImageUid, *annotUid );

        spdlog::debug( "Added new annotation {} (subject plane: {}) for image {}",
                       *annotUid, glm::to_string( subjectPlaneEquation ), *activeImageUid );
    }
    catch ( const std::exception& ex )
    {
        spdlog::error( "Unable to create new annotation (subject plane: {}) for image {}: {}",
                       glm::to_string( subjectPlaneEquation ), *activeImageUid, ex.what() );
        return;
    }

    Annotation* annot = ms_appData->annotation( *annotUid );
    if ( ! annot )
    {
        spdlog::error( "Null annotation {}", *annotUid );
        return;
    }

    const auto projectedPoint = annot->addSubjectPointToBoundary(
                OUTER_BOUNDARY, subjectPlanePoint );

    if ( ! projectedPoint )
    {
        spdlog::error( "Unable to add point {} to annotation {}",
                       glm::to_string( e.hit.worldPos_offsetApplied ), *annotUid );

        /// @todo Delete the annotation!!!
    }

    // Mark this annotation as the one being created:
    ms_growingAnnotUid = annotUid;

    transit<AddingVertexToNewAnnotationState>();
}

/// @todo Make a function for moving a vertex
/// @todo This function should move the vertex that was added above
void CreatingNewAnnotationState::react( const MouseMoveEvent& /*e*/ )
{
    /*
    // Only create/edit points on the outer polygon boundary for now
    static constexpr size_t OUTER_BOUNDARY = 0;

    if ( ! ms_annotBeingCreatedUid )
    {
        spdlog::warn( "No active annotation for which to move first vertex" );
        return;
    }

    spdlog::trace( "ReadyToCreateState::react( const MousePressEvent& e )" );

    if ( ! ms_appData )
    {
        spdlog::error( "AppData is null" );
        transit<AnnotationOffState>();
        return;
    }

    if ( ! ms_selectedViewUid )
    {
        spdlog::error( "No selected in which to create annotation" );
        transit<ViewBeingSelectedState>();
        return;
    }

    if ( *ms_selectedViewUid != e.hit.viewUid )
    {
        // Mouse pointer is not in the view selected for creating annotation
        spdlog::trace( "Mouse pointer is not in the view selected for creating annotation" );
        return;
    }
    */
}

void CreatingNewAnnotationState::react( const MouseReleaseEvent& /*e*/ )
{
    /// @todo Transition states here
    //transit<AddingVertexToNewAnnotationState>();
}

void CreatingNewAnnotationState::react( const TurnOffAnnotationModeEvent& )
{
//    spdlog::trace( "CreatingNewAnnotationState::react( const TurnOffAnnotationMode& )" );
    turnAnnotatingOff();
}

void CreatingNewAnnotationState::react( const CompleteNewAnnotationEvent& )
{
    spdlog::trace( "CreatingNewAnnotationState::react( const CompleteNewAnnotationEvent& )" );
    ms_growingAnnotUid = std::nullopt;
    transit<ReadyToEditState>();
}


/**************** AddingVertexToNewAnnotationState *******************/

void AddingVertexToNewAnnotationState::entry()
{
    spdlog::trace( "AddingVertexToNewAnnotationState::entry()" );

    if ( ms_selectedViewUid )
    {
        spdlog::trace( "Selected view {} for adding vertex annotation", *ms_selectedViewUid );
    }
    else
    {
        spdlog::error( "Entered AddingVertexToNewAnnotationState without a selected view" );
        transit<ViewBeingSelectedState>();
        return;
    }

    if ( ! ms_growingAnnotUid )
    {
        spdlog::error( "Entered AddingVertexToNewAnnotationState without an annotation having been created" );
        transit<CreatingNewAnnotationState>();
        return;
    }
}

void AddingVertexToNewAnnotationState::exit()
{
    spdlog::trace( "AddingVertexToNewAnnotationState::exit()" );
    ms_growingAnnotUid = std::nullopt;
}

/**
 * @brief AddingVertexToNewAnnotationState::react Add a vertex to the active annotation
 * @param e
 */


void AddingVertexToNewAnnotationState::react( const MousePressEvent& e )
{
    addVertexToGrowingAnnotation( e.hit );
}

void AddingVertexToNewAnnotationState::react( const MouseMoveEvent& e )
{
//    spdlog::trace( "AddingVertexToNewAnnotationState::react( const MouseMoveEvent& e )" );
    // Check if this closes the polygon!!!

    if ( e.buttonState.left )
    {
        addVertexToGrowingAnnotation( e.hit );
    }
}

void AddingVertexToNewAnnotationState::react( const MouseReleaseEvent& /*e*/ )
{
//    spdlog::trace( "AddingVertexToNewAnnotationState::react( const MouseReleaseEvent& e )" );
}

void AddingVertexToNewAnnotationState::react( const TurnOffAnnotationModeEvent& )
{
//    spdlog::trace( "AddingVertexToNewAnnotationState::react( const TurnOffAnnotationMode& )" );
    turnAnnotatingOff();
}

void AddingVertexToNewAnnotationState::react( const CompleteNewAnnotationEvent& )
{
    spdlog::trace( "AddingVertexToNewAnnotationState::react( const CompleteNewAnnotationEvent& )" );
    ms_growingAnnotUid = std::nullopt;
    transit<ReadyToEditState>();
}


/**************** VertexSelectedState *******************/

void VertexSelectedState::entry()
{
    spdlog::trace( "VertexSelectedState::entry()" );
}

void VertexSelectedState::exit()
{
    spdlog::trace( "VertexSelectedState::exit()" );
}

void VertexSelectedState::react( const MousePressEvent& /*e*/ )
{
    spdlog::trace( "VertexSelectedState::react( const MousePressEvent& e )" );
}

void VertexSelectedState::react( const MouseReleaseEvent& /*e*/ )
{
    spdlog::trace( "VertexSelectedState::react( const MouseReleaseEvent& e )" );
}

void VertexSelectedState::react( const MouseMoveEvent& /*e*/ )
{
//    spdlog::trace( "VertexSelectedState::react( const MouseMoveEvent& e )" );
}

void VertexSelectedState::react( const TurnOffAnnotationModeEvent& )
{
    spdlog::trace( "VertexSelectedState::react( const TurnOffAnnotationMode& )" );
    turnAnnotatingOff();
}



bool isInStateWhereViewsCanScroll()
{
    if ( ASM::is_in_state<AnnotationOffState>() ||
         ASM::is_in_state<ViewBeingSelectedState>() ||
         ASM::is_in_state<ReadyToEditState>() ||
         ASM::is_in_state<CreatingNewAnnotationState>() )
    {
        return true;
    }

    return false;
}

bool isInStateWhereToolbarVisible()
{
    if ( ASM::is_in_state<AnnotationOffState>() ||
         ASM::is_in_state<ViewBeingSelectedState>() )
    {
        return false;
    }

    return true;
}

bool showToolbarCreateButton()
{
    if ( ASM::is_in_state<state::ReadyToEditState>() )
    {
        return true;
    }

    return false;
}

bool showToolbarCompleteButton()
{
    if ( ! ASM::growingAnnotUid() )
    {
        return false;
    }

    Annotation* annot = ASM::appData()->annotation( *ASM::growingAnnotUid() );

    if ( ! annot )
    {
        return false;
    }

    if ( ASM::is_in_state<state::CreatingNewAnnotationState>() ||
         ASM::is_in_state<state::AddingVertexToNewAnnotationState>() )
    {
//        if ( annot->polygon().numVertices() > 0 )
        {
            return true;
        }
    }

    return false;
}

} // namespace state


/// Initial state definition
FSM_INITIAL_STATE( state::AnnotationStateMachine, state::AnnotationOffState )
