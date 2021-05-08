#include "logic/states/AnnotationStateMachine.h"

#include "common/DataHelper.h"
#include "logic/app/Data.h"
#include "logic/camera/CameraHelpers.h"
#include "logic/camera/MathUtility.h"
#include "logic/states/FsmList.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>


namespace
{

// Only create/edit points on the outer polygon boundary for now
static constexpr size_t OUTER_BOUNDARY = 0;

}


namespace state
{

AppData* ASM::ms_appData = nullptr;
std::optional<uuids::uuid> ASM::ms_hoveredViewUid = std::nullopt;
std::optional<uuids::uuid> ASM::ms_selectedViewUid = std::nullopt;
std::optional<uuids::uuid> ASM::ms_growingAnnotUid = std::nullopt;
std::optional<uuids::uuid> ASM::ms_selectedAnnotUid = std::nullopt;
std::optional<size_t> ASM::ms_selectedVertex = std::nullopt;


void AnnotationStateMachine::react( const tinyfsm::Event& )
{
    spdlog::warn( "Unhandled event sent to AnnotationStateMachine" );
}


bool AnnotationStateMachine::checkAppData()
{
    if ( ! ms_appData )
    {
        spdlog::error( "AppData is null" );
        turnAnnotatingOff();
        return false;
    }

    return true;
}

bool AnnotationStateMachine::checkViewSelection( const ViewHit& hit )
{
    if ( ! checkAppData() ) return false;

    if ( ! ms_selectedViewUid )
    {
        spdlog::error( "No selected view in which to annotate" );
        transit<ViewBeingSelectedState>();
        return false;
    }

    if ( *ms_selectedViewUid != hit.viewUid )
    {
        // Mouse pointer is not in the view selected for editing annotation
//        spdlog::trace( "Mouse pointer is not in the view selected for annotating" );
        return false;
    }

    return true;
}

void AnnotationStateMachine::turnAnnotatingOff()
{
    ms_selectedViewUid = std::nullopt;
    ms_hoveredViewUid = std::nullopt;
    ms_growingAnnotUid = std::nullopt;
    deselectVertex();
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

bool AnnotationStateMachine::createNewGrowingAnnotation( const ViewHit& hit )
{
    if ( ! checkViewSelection( hit ) ) return false;

    // Annotate on the active image
    const auto activeImageUid = ms_appData->activeImageUid();
    const Image* activeImage = ( activeImageUid ? ms_appData->image( *activeImageUid ) : nullptr );

    if ( ! activeImage )
    {
        spdlog::warn( "There is no active image when attempting to annotate" );
        return false;
    }

    if ( 0 == std::count( std::begin( hit.view->visibleImages() ),
                          std::end( hit.view->visibleImages() ),
                          *activeImageUid ) )
    {
        // The active image is not visible
        return false;
    }

    deselectVertex();

    // Compute the plane equation in Subject space. Use the World position after the view
    // offset has been applied, so that the user can annotate in any view of a lightbox layout.
    const auto [subjectPlaneEquation, subjectPlanePoint] =
            math::computeSubjectPlaneEquation(
                activeImage->transformations().subject_T_worldDef(),
                -hit.worldFrontAxis, glm::vec3{ hit.worldPos_offsetApplied } );

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
            return false;
        }

        ms_appData->assignActiveAnnotationUidToImage( *activeImageUid, *annotUid );

        spdlog::debug( "Added new annotation {} (subject plane: {}) for image {}",
                       *annotUid, glm::to_string( subjectPlaneEquation ), *activeImageUid );
    }
    catch ( const std::exception& ex )
    {
        spdlog::error( "Unable to create new annotation (subject plane: {}) for image {}: {}",
                       glm::to_string( subjectPlaneEquation ), *activeImageUid, ex.what() );
        return false;
    }

    Annotation* annot = ms_appData->annotation( *annotUid );
    if ( ! annot )
    {
        spdlog::error( "Null annotation {}", *annotUid );
        return false;
    }

    // Mark this annotation as the one being created:
    ms_growingAnnotUid = annotUid;

    return true;
}

bool AnnotationStateMachine::addVertexToGrowingAnnotation( const ViewHit& hit )
{
    static constexpr size_t FIRST_VERTEX_INDEX = 0;

    if ( ! checkViewSelection( hit ) ) return false;

    if ( ! ms_growingAnnotUid )
    {
        spdlog::error( "There is no new annotation for which to add a vertex" );
        turnAnnotatingOff();
        return false;
    }

    // Annotate on the active image
    const auto activeImageUid = ms_appData->activeImageUid();
    const Image* activeImage = ( activeImageUid ? ms_appData->image( *activeImageUid ) : nullptr );

    if ( ! activeImage )
    {
        spdlog::warn( "There is no active image when attempting to annotate" );
        return false;
    }

    if ( 0 == std::count( std::begin( hit.view->visibleImages() ),
                          std::end( hit.view->visibleImages() ),
                          *activeImageUid ) )
    {
        // The active image is not visible
        return false;
    }

    // Compute the plane equation in Subject space. Use the World position after the view
    // offset has been applied, so that the user can annotate in any view of a lightbox layout.
    const auto [subjectPlaneEquation, subjectPlanePoint] =
            math::computeSubjectPlaneEquation(
                activeImage->transformations().subject_T_worldDef(),
                -hit.worldFrontAxis, glm::vec3{ hit.worldPos_offsetApplied } );

    Annotation* growingAnnot = ms_appData->annotation( *ms_growingAnnotUid );

    if ( ! growingAnnot )
    {
        spdlog::error( "Null annotation {}", *ms_growingAnnotUid );
        return false;
    }

    const auto hitVertices = findHitVertices( hit );

    if ( growingAnnot->polygon().numBoundaries() > 0 )
    {
        // If the growing annotation has a boundary polygon, then perform checks
        // to see if we should
        // 1) ignore the current vertex due to being too close to the last vertex
        // 2) close the polygon due to the current vertex being close to the first vertex

        const bool hasMoreThanTwoVertices =
                ( growingAnnot->getBoundaryVertices( OUTER_BOUNDARY ).size() >= 2 );

        // Index of the vertex that is going to be added:
        const size_t currentVertexIndex =
                growingAnnot->getBoundaryVertices( OUTER_BOUNDARY ).size();

        // Loop over vertices near this hit
        for ( const auto& vertex : hitVertices )
        {
            if ( *ms_growingAnnotUid == vertex.first &&
                 currentVertexIndex == ( vertex.second + 1 ) )
            {
                // The current vertex to add is close to the last vertex, so do not add it.
                return false;
            }

            if ( *ms_growingAnnotUid == vertex.first &&
                 FIRST_VERTEX_INDEX == vertex.second &&
                 hasMoreThanTwoVertices )
            {
                // The point is near the annotation's first vertex and the annotation
                // has more than two vertices, so close the polygon and do not add a new vertex.
                growingAnnot->setClosed( true );
                growingAnnot->setFilled( true );
                transit<ReadyToEditState>();
                return true;
            }
        }
    }

    // Check if the point is near another vertex of an annotation. Use this existing
    // vertex position for the new one, so that we can create sealed annotations.

    for ( const auto& vertex : hitVertices )
    {
        Annotation* otherAnnot = ms_appData->annotation( vertex.first );

        if ( ! otherAnnot )
        {
            spdlog::error( "Null annotation {}", vertex.first );
            continue;
        }

        if ( const auto planePoint2d = otherAnnot->polygon().getBoundaryVertex(
                 OUTER_BOUNDARY, vertex.second ) )
        {
            // Add the existing point
            growingAnnot->addPlanePointToBoundary( OUTER_BOUNDARY, *planePoint2d );
            return true;
        }
    }

    // The prior checks fell through, so add the new point to the polygon:
    if ( ! growingAnnot->addSubjectPointToBoundary( OUTER_BOUNDARY, subjectPlanePoint ) )
    {
        spdlog::error( "Unable to add point {} to annotation {}",
                       glm::to_string( hit.worldPos_offsetApplied ),
                       *ms_growingAnnotUid );
    }

    return true;
}

void AnnotationStateMachine::completeGrowingAnnotation( bool closeAnnotation )
{
    if ( ! ms_growingAnnotUid )
    {
        // No growing annotation to complete/close
        return;
    }

    if ( closeAnnotation )
    {
        Annotation* growingAnnot = ms_appData->annotation( *ms_growingAnnotUid );

        if ( ! growingAnnot )
        {
            spdlog::error( "Null annotation {}", *ms_growingAnnotUid );
            return;
        }

        const bool hasMoreThanThreeVertices =
                ( growingAnnot->getBoundaryVertices( OUTER_BOUNDARY ).size() >= 3 );

        if ( hasMoreThanThreeVertices )
        {
            growingAnnot->setClosed( true );
            growingAnnot->setFilled( true );
        }
    }

    // Done with growing this annotation:
    ms_growingAnnotUid = std::nullopt;
    transit<ReadyToEditState>();
}

void AnnotationStateMachine::undoLastVertexOfGrowingAnnotation()
{
    if ( ! ms_growingAnnotUid )
    {
        // No growing annotation to complete/close
        return;
    }

    Annotation* growingAnnot = ms_appData->annotation( *ms_growingAnnotUid );

    if ( ! growingAnnot )
    {
        spdlog::error( "Null annotation {}", *ms_growingAnnotUid );
        return;
    }

    const size_t numVertices = growingAnnot->getBoundaryVertices( OUTER_BOUNDARY ).size();

    if ( numVertices >= 1 )
    {
        growingAnnot->polygon().removeVertexFromBoundary( OUTER_BOUNDARY, numVertices - 1 );
    }
}

void AnnotationStateMachine::removeGrowingAnnotation()
{
    if ( ! checkAppData() ) return;

    if ( ! ms_growingAnnotUid )
    {
        transit<ReadyToEditState>();
        return;
    }

    if ( ! ms_appData->removeAnnotation( *ms_growingAnnotUid ) )
    {
        spdlog::error( "Unable to remove annotation {}", *ms_growingAnnotUid );
    }

    ms_growingAnnotUid = std::nullopt;
    transit<ReadyToEditState>();
}

std::vector< std::pair<uuids::uuid, size_t> >
AnnotationStateMachine::findHitVertices( const ViewHit& hit )
{
    // Distance threshold for hitting a vertex (in pixels)
    static constexpr float sk_distThresh_inPixels = 6.0f;

    static const std::vector< std::pair<uuids::uuid, size_t> > sk_empty;

    if ( ! checkAppData() ) return sk_empty;

    if ( ! hit.view )
    {
        spdlog::error( "Null view" );
        return sk_empty;
    }

    const auto activeImageUid = ms_appData->activeImageUid();
    const Image* activeImage = ( activeImageUid ? ms_appData->image( *activeImageUid ) : nullptr );

    if ( ! activeImage )
    {
        spdlog::warn( "There is no active image when attempting to annotate" );
        return sk_empty;
    }

    if ( 0 == std::count( std::begin( hit.view->visibleImages() ),
                          std::end( hit.view->visibleImages() ),
                          *activeImageUid ) )
    {
        // The active image is not visible
        return sk_empty;
    }

    // Number of mm per pixel in the x and y directions:
    const glm::vec2 mmPerPixel = camera::worldPixelSize(
                ms_appData->windowData().viewport(),
                hit.view->camera(), hit.view->viewClip_T_windowClip() );

    // Compute the plane equation in Subject space. Use the World position after the view
    // offset has been applied, so that the user can annotate in any view of a lightbox layout.
    const auto [subjectPlaneEquation, subjectPlanePoint] =
            math::computeSubjectPlaneEquation(
                activeImage->transformations().subject_T_worldDef(),
                -hit.worldFrontAxis, glm::vec3{ hit.worldPos_offsetApplied } );

    // Use the image slice scroll distance as the threshold for plane distances
    const float planeDistanceThresh = 0.5f * data::sliceScrollDistance(
                hit.worldFrontAxis, *activeImage );

    // Find all annotations for the active image that match this plane
    const auto uidsOfAnnotsOnImageSlice = data::findAnnotationsForImage(
                *ms_appData, *activeImageUid,
                subjectPlaneEquation, planeDistanceThresh );


    // Loop over all annotations and determine whether we're atop a vertex.

    // Closest distance found to a vertex so far
    float closestDistance_inPixels = std::numeric_limits<float>::max();

    // Pairs of { annotationUid, vertex index }
    std::vector< std::pair<uuids::uuid, size_t> > annotAndVertex;

    for ( const auto& annotUid : uidsOfAnnotsOnImageSlice )
    {
        Annotation* annot = ms_appData->annotation( annotUid );
        if ( ! annot )
        {
            spdlog::error( "Null annotation {}", annotUid );
            continue;
        }

        const glm::vec2 hoveredPoint =
                annot->projectSubjectPointToAnnotationPlane( subjectPlanePoint );

        size_t vertexIndex = 0;

        for ( const glm::vec2& annotPoint : annot->getBoundaryVertices( OUTER_BOUNDARY ) )
        {
            const glm::vec2 dist_inMM = glm::abs( annotPoint - hoveredPoint );
            const float dist_inPixels = glm::length( dist_inMM / mmPerPixel );

            if ( dist_inPixels < sk_distThresh_inPixels &&
                 dist_inPixels < closestDistance_inPixels )
            {
                annotAndVertex.emplace_back( std::make_pair( annotUid, vertexIndex ) );
                closestDistance_inPixels = dist_inPixels;
            }

            ++vertexIndex;
        }
    }

    return annotAndVertex;
}

void AnnotationStateMachine::highlightHoveredVertex( const ViewHit& hit )
{
    if ( ! checkAppData() ) return;

    deselectAllAnnotationVertices();

    const auto hitVertices = findHitVertices( hit );

    if ( hitVertices.empty() )
    {
        return; // No vertex hit
    }

    // Let's select the first vertex that was hit:
    const auto hitVertex = hitVertices.front();

    Annotation* annot = ms_appData->annotation( hitVertex.first );

    if ( ! annot )
    {
        spdlog::error( "Null annotation {}", hitVertex.first );
        return;
    }

    annot->polygon().setSelectedVertex( std::make_pair( OUTER_BOUNDARY, hitVertex.second ) );
}

void AnnotationStateMachine::selectVertex(
        const uuids::uuid& annotUid, size_t vertexIndex )
{
    ms_selectedAnnotUid = annotUid;
    ms_selectedVertex = vertexIndex;
}

void AnnotationStateMachine::deselectVertex()
{
    ms_selectedAnnotUid = std::nullopt;
    ms_selectedVertex = std::nullopt;
}

void AnnotationStateMachine::deselectAllAnnotationVertices()
{
    if ( ! checkAppData() ) return;

    for ( const auto& imageUid : ms_appData->imageUidsOrdered() )
    {
        for ( const auto& annotUid : ms_appData->annotationsForImage( imageUid ) )
        {
            Annotation* annot = ms_appData->annotation( annotUid );
            if ( ! annot )
            {
                spdlog::error( "Null annotation {}", annotUid );
                continue;
            }

            annot->polygon().setSelectedVertex( std::nullopt );
        }
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
//    spdlog::trace( "ReadyToEditState::react( const MouseReleaseEvent& e )" );
}

void ReadyToEditState::react( const MouseMoveEvent& e )
{
    hoverOverView( e.hit );

    if ( ! checkViewSelection( e.hit ) ) return;

    highlightHoveredVertex( e.hit );

    /// Don't do this just for hover. Do it when clicking
//    deselect();

    /// Only do this when clicked!
//    selectVertex( vertex->first, vertex->second );
}

void ReadyToEditState::react( const TurnOffAnnotationModeEvent& )
{
//    spdlog::trace( "ReadyToEditState::react( const TurnOffAnnotationMode& )" );
    turnAnnotatingOff();
}

void ReadyToEditState::react( const CreateNewAnnotationEvent& )
{
//    spdlog::trace( "ReadyToEditState::react( const CreateNewAnnotation& )" );
    transit<CreatingNewAnnotationState>();
}



/**************** CreatingNewAnnotationState *******************/

void CreatingNewAnnotationState::entry()
{
//    spdlog::trace( "ReadyToCreateState::entry()" );

    if ( ms_selectedViewUid )
    {
//        spdlog::trace( "Selected view {} for creating annotation", *ms_selectedViewUid );
    }
    else
    {
        spdlog::error( "Entered CreatingNewAnnotationState without a selected view" );
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
 * @todo Make the point selection STICKY, so that we can link annotations!
 */
void CreatingNewAnnotationState::react( const MousePressEvent& e )
{
//    spdlog::trace( "CreatingNewAnnotationState::react( const MousePressEvent& e )" );

    if ( e.buttonState.left )
    {
        if ( createNewGrowingAnnotation( e.hit ) &&
             addVertexToGrowingAnnotation( e.hit ) )
        {
            transit<AddingVertexToNewAnnotationState>();
        }
    }
}

/// @todo Make a function for moving a vertex
/// @todo This function should move the vertex that was added above
void CreatingNewAnnotationState::react( const MouseMoveEvent& e )
{
    highlightHoveredVertex( e.hit );

    /*
    // Only create/edit points on the outer polygon boundary for now
    static constexpr size_t OUTER_BOUNDARY = 0;

    if ( ! ms_annotBeingCreatedUid )
    {
        spdlog::warn( "No active annotation for which to move first vertex" );
        return;
    }

    spdlog::trace( "ReadyToCreateState::react( const MousePressEvent& e )" );

    if ( ! checkAppData() ) return false;

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
//    spdlog::trace( "CreatingNewAnnotationState::react( const CompleteNewAnnotationEvent& )" );
    completeGrowingAnnotation( false );
}

void CreatingNewAnnotationState::react( const CancelNewAnnotationEvent& )
{
//    spdlog::trace( "CreatingNewAnnotationState::react( const CancelNewAnnotationEvent& )" );
    removeGrowingAnnotation();
}


/**************** AddingVertexToNewAnnotationState *******************/

void AddingVertexToNewAnnotationState::entry()
{
    spdlog::trace( "AddingVertexToNewAnnotationState::entry()" );

    if ( ms_selectedViewUid )
    {
//        spdlog::trace( "Selected view {} for adding vertex annotation", *ms_selectedViewUid );
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
    if ( e.buttonState.left )
    {
        addVertexToGrowingAnnotation( e.hit );
    }
}

void AddingVertexToNewAnnotationState::react( const MouseMoveEvent& e )
{
    spdlog::trace( "AddingVertexToNewAnnotationState::react( const MouseMoveEvent& e )" );

    highlightHoveredVertex( e.hit );

    if ( e.buttonState.left )
    {
        deselectVertex();
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
//    spdlog::trace( "AddingVertexToNewAnnotationState::react( const CompleteNewAnnotationEvent& )" );
    completeGrowingAnnotation( false );
}

void AddingVertexToNewAnnotationState::react( const CloseNewAnnotationEvent& )
{
//    spdlog::trace( "AddingVertexToNewAnnotationState::react( const CloseNewAnnotationEvent& )" );
    completeGrowingAnnotation( true );
}

void AddingVertexToNewAnnotationState::react( const UndoVertexEvent& )
{
//    spdlog::trace( "AddingVertexToNewAnnotationState::react( const UndoVertexEvent& )" );
    undoLastVertexOfGrowingAnnotation();
}

void AddingVertexToNewAnnotationState::react( const CancelNewAnnotationEvent& )
{
//    spdlog::trace( "AddingVertexToNewAnnotationState::react( const CancelNewAnnotationEvent& )" );
    removeGrowingAnnotation();
}



/**************** VertexSelectedState *******************/

void VertexSelectedState::entry()
{
//    spdlog::trace( "VertexSelectedState::entry()" );
}

void VertexSelectedState::exit()
{
//    spdlog::trace( "VertexSelectedState::exit()" );
}

void VertexSelectedState::react( const MousePressEvent& /*e*/ )
{
//    spdlog::trace( "VertexSelectedState::react( const MousePressEvent& e )" );
}

void VertexSelectedState::react( const MouseReleaseEvent& /*e*/ )
{
//    spdlog::trace( "VertexSelectedState::react( const MouseReleaseEvent& e )" );
}

void VertexSelectedState::react( const MouseMoveEvent& /*e*/ )
{
//    spdlog::trace( "VertexSelectedState::react( const MouseMoveEvent& e )" );
}

void VertexSelectedState::react( const TurnOffAnnotationModeEvent& )
{
//    spdlog::trace( "VertexSelectedState::react( const TurnOffAnnotationMode& )" );
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

    // Need a valid annotation with at least 1 vertex in order to complete it:
    Annotation* annot = ASM::appData()->annotation( *ASM::growingAnnotUid() );

    if ( ! annot )
    {
        return false;
    }

    if ( 0 == annot->polygon().numVertices() )
    {
        return false;
    }

    if ( ASM::is_in_state<state::CreatingNewAnnotationState>() ||
         ASM::is_in_state<state::AddingVertexToNewAnnotationState>() )
    {
        return true;
    }

    return false;
}

bool showToolbarCloseButton()
{
    if ( ! ASM::growingAnnotUid() )
    {
        return false;
    }

    // Need a valid annotation with at least 3 vertices in order to close it:
    Annotation* annot = ASM::appData()->annotation( *ASM::growingAnnotUid() );

    if ( ! annot )
    {
        return false;
    }

    if ( annot->polygon().numVertices() < 3 )
    {
        return false;
    }

    if ( ASM::is_in_state<state::CreatingNewAnnotationState>() ||
         ASM::is_in_state<state::AddingVertexToNewAnnotationState>() )
    {
        return true;
    }

    return false;
}

bool showToolbarUndoButton()
{
    return showToolbarCompleteButton();
}

bool showToolbarCancelButton()
{
    if ( ASM::is_in_state<state::CreatingNewAnnotationState>() ||
         ASM::is_in_state<state::AddingVertexToNewAnnotationState>() )
    {
        return true;
    }

    return false;
}

} // namespace state


/// Initial state definition
FSM_INITIAL_STATE( state::AnnotationStateMachine, state::AnnotationOffState )
