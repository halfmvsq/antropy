#include "logic/AppSettings.h"
//#include "logic/ipc/IPCMessage.h"

#include <glm/glm.hpp>


AppSettings::AppSettings()
    :
      m_mouseMode( MouseMode::Pointer ),
      m_buttonState(),
      m_recenteringMode( ImageSelection::AllLoadedImages ),

      m_synchronizeZoom( true ),
      m_animating( false ),
      m_overlays( true ),

      m_foregroundLabel( 1u ),
      m_backgroundLabel( 0u ),
      m_replaceBackgroundWithForeground( false ),
      m_use3dBrush( false ),
      m_useIsotropicBrush( true ),
      m_useVoxelBrushSize( true ),
      m_useRoundBrush( true ),
      m_crosshairsMoveWithBrush( false ),
      m_brushSizeInVoxels( 1 ),
      m_brushSizeInMm( 1.0f ),

      m_worldCrosshairs(),
      m_worldRotationCenter( std::nullopt )

      //m_ipcHandler()
{
}


void AppSettings::adjustActiveSegmentationLabels(
        const ParcellationLabelTable& activeLabelTable )
{
    m_foregroundLabel = std::min( m_foregroundLabel, activeLabelTable.numLabels() - 1 );
    m_backgroundLabel = std::min( m_backgroundLabel, activeLabelTable.numLabels() - 1 );
}

void AppSettings::swapForegroundAndBackgroundLabels(
        const ParcellationLabelTable& activeLabelTable )
{
    const auto fg = foregroundLabel();
    const auto bg = backgroundLabel();

    setForegroundLabel( bg, activeLabelTable );
    setBackgroundLabel( fg, activeLabelTable );
}

void AppSettings::setWorldRotationCenter(
        const std::optional< glm::vec3 >& worldRotationCenter )
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

const std::optional< glm::vec3 >& AppSettings::worldRotationCenter() const
{
    return m_worldRotationCenter;
}

void AppSettings::setWorldCrosshairsPos( const glm::vec3& worldCrosshairsPos )
{
    /// @todo Should the crosshairs be confined to the AABB of the images?
    m_worldCrosshairs.setWorldOrigin( worldCrosshairsPos );
//    broadcastCrosshairsPosition();
}

const CoordinateFrame& AppSettings::worldCrosshairs() const
{
    return m_worldCrosshairs;
}

MouseMode AppSettings::mouseMode() const { return m_mouseMode; }
void AppSettings::setMouseMode( MouseMode mode ) { m_mouseMode = mode; }

ButtonState& AppSettings::buttonState() { return m_buttonState; }

ImageSelection AppSettings::recenteringMode() const { return m_recenteringMode; }
void AppSettings::setRecenteringMode( ImageSelection mode ) { m_recenteringMode = mode; }

bool AppSettings::synchronizeZooms() const { return m_synchronizeZoom; }
void AppSettings::setSynchronizeZooms( bool sync ) { m_synchronizeZoom = sync; }

bool AppSettings::animating() const { return m_animating; }
void AppSettings::setAnimating( bool set ) { m_animating = set; }

bool AppSettings::overlays() const { return m_overlays; }
void AppSettings::setOverlays( bool set ) { m_overlays = set; }


size_t AppSettings::foregroundLabel() const { return m_foregroundLabel; }

void AppSettings::setForegroundLabel( size_t label, const ParcellationLabelTable& activeLabelTable )
{
    m_foregroundLabel = label;
    adjustActiveSegmentationLabels( activeLabelTable );
}

size_t AppSettings::backgroundLabel() const { return m_backgroundLabel; }

void AppSettings::setBackgroundLabel( size_t label, const ParcellationLabelTable& activeLabelTable )
{
    m_backgroundLabel = label;
    adjustActiveSegmentationLabels( activeLabelTable );
}

bool AppSettings::replaceBackgroundWithForeground() const { return m_replaceBackgroundWithForeground; }
void AppSettings::setReplaceBackgroundWithForeground( bool set ) { m_replaceBackgroundWithForeground = set; }

bool AppSettings::use3dBrush() const { return m_use3dBrush; }
void AppSettings::setUse3dBrush( bool set ) { m_use3dBrush = set; }

bool AppSettings::useIsotropicBrush() const { return m_useIsotropicBrush; }
void AppSettings::setUseIsotropicBrush( bool set ) { m_useIsotropicBrush = set; }

bool AppSettings::useVoxelBrushSize() const { return m_useVoxelBrushSize; }
void AppSettings::setUseVoxelBrushSize( bool set ) { m_useVoxelBrushSize = set; }

bool AppSettings::useRoundBrush() const { return m_useRoundBrush; }
void AppSettings::setUseRoundBrush( bool set ) { m_useRoundBrush = set; }

bool AppSettings::crosshairsMoveWithBrush() const { return m_crosshairsMoveWithBrush; }
void AppSettings::setCrosshairsMoveWithBrush( bool set ) { m_crosshairsMoveWithBrush = set; }

uint32_t AppSettings::brushSizeInVoxels() const { return m_brushSizeInVoxels; }
void AppSettings::setBrushSizeInVoxels( uint32_t size ) { m_brushSizeInVoxels = size; }

float AppSettings::brushSizeInMm() const { return m_brushSizeInMm; }
void AppSettings::setBrushSizeInMm( float size ) { m_brushSizeInMm = size; }


/*
void AppSettings::broadcastCrosshairsPosition()
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
