#include "logic/states/AnnotationStateMachine.h"
#include "logic/states/AnnotationStates.h"
#include "logic/states/FsmList.hpp"

#include "common/DataHelper.h"
#include "logic/app/Data.h"
#include "logic/camera/CameraHelpers.h"
#include "logic/camera/MathUtility.h"

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

std::optional<size_t> ASM::ms_selectedVertex = std::nullopt;
std::optional<uuids::uuid> ASM::ms_hoveredAnnotUid = std::nullopt;
std::optional<size_t> ASM::ms_hoveredVertex = std::nullopt;


void AnnotationStateMachine::react( const tinyfsm::Event& )
{
    spdlog::warn( "Unhandled event sent to AnnotationStateMachine" );
}


bool AnnotationStateMachine::checkAppData()
{
    if ( ! ms_appData )
    {
        spdlog::error( "AppData is null" );
        return false;
    }
    return true;
}

Image* AnnotationStateMachine::checkActiveImage( const ViewHit& hit )
{
    if ( ! checkAppData() ) return nullptr;

    const auto activeImageUid = ms_appData->activeImageUid();
    if ( ! activeImageUid )
    {
        spdlog::info( "There is no active image to annotate" );
        return nullptr;
    }

    Image* activeImage = ms_appData->image( *activeImageUid );
    if ( ! activeImage )
    {
        spdlog::error( "Active image {} is null", *activeImageUid );
        return nullptr;
    }

    if ( ! std::count( std::begin( hit.view->visibleImages() ),
                       std::end( hit.view->visibleImages() ),
                       *activeImageUid ) )
    {
        // The active image is not visible in the view hit by the mouse
        return nullptr;
    }

    return activeImage;
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
        // Mouse pointer is not in the view selected for annotating
        return false;
    }

    return true;
}

void AnnotationStateMachine::hoverView( const ViewHit& hit )
{
    ms_hoveredViewUid = hit.viewUid;
}

void AnnotationStateMachine::selectView( const ViewHit& hit )
{
    if ( ms_selectedViewUid && *ms_selectedViewUid != hit.viewUid )
    {
        deselectAnnotation();
        unhoverAnnotation();
    }

    ms_selectedViewUid = hit.viewUid;
}

void AnnotationStateMachine::deselectAnnotation()
{
    if ( ! checkAppData() ) return;

    ms_selectedVertex = std::nullopt;

    const auto activeImageUid = ms_appData->activeImageUid();
    if ( ! activeImageUid ) return;

    if ( ! ms_appData->assignActiveAnnotationUidToImage( *activeImageUid, std::nullopt ) )
    {
        spdlog::error( "Unable to remove active annotation from image {}", *activeImageUid );
    }

    synchronizeAnnotationHighlights();
}

void AnnotationStateMachine::unhoverAnnotation()
{
    ms_hoveredAnnotUid = std::nullopt;
    ms_hoveredVertex = std::nullopt;
    synchronizeAnnotationHighlights();
}

bool AnnotationStateMachine::createNewGrowingPolygon( const ViewHit& hit )
{
    if ( ! checkAppData() ) return false;
    if ( ! checkViewSelection( hit ) ) return false;

    // Annotate on the active image
    const Image* activeImage = checkActiveImage( hit );
    if ( ! activeImage ) return false;

    const auto activeImageUid = ms_appData->activeImageUid();

    // Compute the plane equation in Subject space. Use the World position after
    // the view offset has been applied, so that the user can annotate in any view
    // of a lightbox layout.
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

    // Select the annotation
    const std::optional<size_t> noSelectedVertex;
    selectAnnotationAndVertex( *annotUid, noSelectedVertex );

    return true;
}

bool AnnotationStateMachine::addVertexToGrowingPolygon( const ViewHit& hit )
{
    static constexpr size_t FIRST_VERTEX_INDEX = 0;

    if ( ! checkAppData() ) return false;
    if ( ! checkViewSelection( hit ) ) return false;

    if ( ! ms_growingAnnotUid )
    {
        spdlog::error( "There is no new annotation for which to add a vertex" );
        transit<AnnotationOffState>();
        return false;
    }

    // Annotate on the active image
    const Image* activeImage = checkActiveImage( hit );
    if ( ! activeImage ) return false;

    const auto [subjectPlaneEquation, subjectPlanePoint] =
            math::computeSubjectPlaneEquation(
                activeImage->transformations().subject_T_worldDef(),
                -hit.worldFrontAxis, glm::vec3{ hit.worldPos_offsetApplied } );

    Annotation* growingAnnot = ms_appData->annotation( *ms_growingAnnotUid );
    if ( ! growingAnnot )
    {
        spdlog::warn( "Growing annotation {} is no longer valid", *ms_growingAnnotUid );
        transit<StandbyState>();
        return false;
    }

    const auto hitVertices = findHitVertices( hit );

    if ( growingAnnot->numBoundaries() > 0 )
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

        // Loop over vertices near the mouse hit
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
                transit<StandbyState>();
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

void AnnotationStateMachine::completeGrowingPoylgon( bool closePolgon )
{
    if ( ! checkAppData() ) return;

    if ( ! ms_growingAnnotUid )
    {
        // No growing annotation to complete
        return;
    }

    if ( closePolgon )
    {
        Annotation* growingAnnot = ms_appData->annotation( *ms_growingAnnotUid );
        if ( ! growingAnnot )
        {
            spdlog::warn( "Growing annotation {} is no longer valid", *ms_growingAnnotUid );
            transit<StandbyState>();
            return;
        }

        const bool hasThreeOrMoreVertices =
                ( growingAnnot->getBoundaryVertices( OUTER_BOUNDARY ).size() >= 3 );

        if ( hasThreeOrMoreVertices )
        {
            growingAnnot->setClosed( true );
            growingAnnot->setFilled( true );
        }
    }

    // Done with growing this annotation:
    ms_growingAnnotUid = std::nullopt;

    transit<StandbyState>();
}

void AnnotationStateMachine::undoLastVertexOfGrowingPolygon()
{
    if ( ! checkAppData() ) return;

    if ( ! ms_growingAnnotUid )
    {
        // No growing annotation to undo
        return;
    }

    Annotation* growingAnnot = ms_appData->annotation( *ms_growingAnnotUid );
    if ( ! growingAnnot )
    {
        spdlog::warn( "Growing annotation {} is no longer valid", *ms_growingAnnotUid );
        transit<StandbyState>();
        return;
    }

    if ( growingAnnot->numBoundaries() > 0 )
    {
        const size_t numVertices = growingAnnot->getBoundaryVertices( OUTER_BOUNDARY ).size();

        if ( numVertices >= 2 )
        {
            // There are at least two vertices, so remove the last one
            growingAnnot->polygon().removeVertexFromBoundary( OUTER_BOUNDARY, numVertices - 1 );
        }
        else
        {
            // There are 0 or 1 vertices, so remove the whole annotation
            removeGrowingPolygon();
        }
    }
}

void AnnotationStateMachine::removeGrowingPolygon()
{
    if ( ! checkAppData() ) return;

    if ( ! ms_growingAnnotUid )
    {
        transit<StandbyState>();
        return;
    }

    if ( ! ms_appData->removeAnnotation( *ms_growingAnnotUid ) )
    {
        spdlog::error( "Unable to remove annotation {}", *ms_growingAnnotUid );
    }

    ms_growingAnnotUid = std::nullopt;
    deselectAnnotation();
    transit<StandbyState>();
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

    const Image* activeImage = checkActiveImage( hit );
    if ( ! activeImage ) return sk_empty;

    const auto activeImageUid = ms_appData->activeImageUid();

    // Number of mm per pixel in the x and y directions:
    const glm::vec2 mmPerPixel = camera::worldPixelSize(
                ms_appData->windowData().viewport(),
                hit.view->camera(), hit.view->viewClip_T_windowClip() );

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


    // Closest distance found to a vertex so far
    float closestDistance_inPixels = std::numeric_limits<float>::max();

    // Pairs of {annotationUid, vertex index}
    std::vector< std::pair<uuids::uuid, size_t> > annotAndVertex;

    // Loop over all annotations and determine whether we're atop a vertex:
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

        if ( 0 == annot->numBoundaries() ) continue;

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

void AnnotationStateMachine::synchronizeAnnotationHighlights()
{
    if ( ! checkAppData() ) return;

    const auto activeImageUid = ms_appData->activeImageUid();
    if ( ! activeImageUid ) return;

    for ( const auto& imageUid : ms_appData->imageUidsOrdered() )
    {
        const auto activeAnnotUid = ms_appData->imageToActiveAnnotationUid( imageUid );

        for ( const auto& annotUid : ms_appData->annotationsForImage( imageUid ) )
        {
            Annotation* annot = ms_appData->annotation( annotUid );
            if ( ! annot )
            {
                spdlog::error( "Null annotation {}", annotUid );
                continue;
            }

            // Remove all highlights
            annot->setHighlighted( false );
            annot->removeVertexHighlights();
            annot->removeEdgeHighlights();

            if ( imageUid == *activeImageUid )
            {
                if ( activeAnnotUid && ( *activeAnnotUid == annotUid ) )
                {
                    // Highlight the active annotation (of the active image)
                    annot->setHighlighted( true );

                    if ( ms_selectedVertex )
                    {
                        annot->setVertexHighlight( { OUTER_BOUNDARY, *ms_selectedVertex }, true );
                    }
                }

                if ( ms_hoveredAnnotUid && ( *ms_hoveredAnnotUid == annotUid ) )
                {
                    // Do not highlight the hovered annotation; only the hovered vertex
                    // annot->setHighlighted( true );

                    if ( ms_hoveredVertex )
                    {
                        annot->setVertexHighlight( { OUTER_BOUNDARY, *ms_hoveredVertex }, true );
                    }
                }
            }
        }
    }
}

void AnnotationStateMachine::hoverAnnotationAndVertex( const ViewHit& hit )
{
    if ( ! checkViewSelection( hit ) ) return;

    // Clear current hover
    ms_hoveredAnnotUid = std::nullopt;
    ms_hoveredVertex = std::nullopt;
    synchronizeAnnotationHighlights();

    const auto hitVertices = findHitVertices( hit );

    if ( ! hitVertices.empty() )
    {
        // Set hover for first vertex that was hit
        ms_hoveredAnnotUid = hitVertices[0].first;
        ms_hoveredVertex = hitVertices[0].second;
        synchronizeAnnotationHighlights();
    }
}

void AnnotationStateMachine::selectAnnotationAndVertex(
        const uuids::uuid& annotUid,
        const std::optional<size_t>& vertexIndex )
{
    if ( ! checkAppData() ) return;

    const auto activeImageUid = ms_appData->activeImageUid();
    if ( ! activeImageUid ) return;

    if ( ms_appData->assignActiveAnnotationUidToImage( *activeImageUid, annotUid ) )
    {
        if ( vertexIndex )
        {
            ms_selectedVertex = *vertexIndex;
        }
    }
    else
    {
        spdlog::error( "Unable to assign active annotation {} to image {}",
                       annotUid, *activeImageUid );
    }

    synchronizeAnnotationHighlights();
}

} // namespace state


/// Initial state definition
FSM_INITIAL_STATE( state::AnnotationStateMachine, state::AnnotationOffState )
