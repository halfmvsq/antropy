#include "logic/app/State.h"
//#include "logic/ipc/IPCMessage.h"


AppState::AppState()
    :
      m_mouseMode( MouseMode::Pointer ),
      m_buttonState(),
      m_recenteringMode( ImageSelection::AllLoadedImages ),
      m_animating( false ),
      m_annotating( false ),
      m_worldCrosshairs(),
      m_worldRotationCenter( std::nullopt )
      //m_ipcHandler()
{}

void AppState::setWorldRotationCenter( const std::optional< glm::vec3 >& worldRotationCenter )
{
    if ( worldRotationCenter )
    {
        m_worldRotationCenter = *worldRotationCenter;
    }
    else
    {
        m_worldRotationCenter = std::nullopt;
    }
}

const std::optional< glm::vec3 >& AppState::worldRotationCenter() const
{
    return m_worldRotationCenter;
}

void AppState::setWorldCrosshairsPos( const glm::vec3& worldCrosshairsPos )
{
    m_worldCrosshairs.setWorldOrigin( worldCrosshairsPos );
//    broadcastCrosshairsPosition();
}

const CoordinateFrame& AppState::worldCrosshairs() const
{
    return m_worldCrosshairs;
}

void AppState::setMouseMode( MouseMode mode ) { m_mouseMode = mode; }
MouseMode AppState::mouseMode() const { return m_mouseMode; }

void AppState::setButtonState( ButtonState state ) { m_buttonState = state; }
ButtonState AppState::buttonState() const { return m_buttonState; }

void AppState::setRecenteringMode( ImageSelection mode ) { m_recenteringMode = mode; }
ImageSelection AppState::recenteringMode() const { return m_recenteringMode; }

void AppState::setAnimating( bool set ) { m_animating = set; }
bool AppState::animating() const { return m_animating; }

void AppState::setAnnotating( bool set ) { m_annotating = set; }
bool AppState::annotating() const { return m_annotating; }


/*
void AppState::broadcastCrosshairsPosition()
{
    static const glm::vec3 ZERO( 0.0f );
    static const glm::vec3 ONE( 1.0f );

    // We need a reference image to broadcast the crosshairs
    const auto imgUid = refImageUid();
    if ( ! imgUid ) return;

    const Image* refImg = image( *imgUid );
    if ( ! refImg ) return;

    const auto& refTx = refImg->transformations();

    // Convert World to reference Subject and texture positions:
    glm::vec4 refSubjectPos = refTx.subject_T_worldDef() * glm::vec4{ worldCrosshairs().worldOrigin(), 1.0f };
    glm::vec4 refTexturePos = refTx.texture_T_subject() * refSubjectPos;

    refSubjectPos /= refSubjectPos.w;
    refTexturePos /= refTexturePos.w;

    if ( glm::any( glm::lessThan( glm::vec3{refTexturePos}, ZERO ) ) ||
         glm::any( glm::greaterThan( glm::vec3{refTexturePos}, ONE ) ) )
    {
        return;
    }

    // Read the contents of shared memory into the local message object
    IPCMessage message;
    m_ipcHandler.Read( static_cast<void*>( &message ) );

    // Convert LPS to RAS
    message.cursor[0] = -refSubjectPos[0];
    message.cursor[1] = -refSubjectPos[1];
    message.cursor[2] = refSubjectPos[2];

    if ( ! m_ipcHandler.Broadcast( static_cast<void*>( &message ) ) )
    {
        spdlog::debug( "Failed to broadcast IPC to ITK-SNAP" );
    }
}
*/
