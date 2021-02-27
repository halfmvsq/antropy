#include "common/Types.h"

#include <IconFontCppHeaders/IconsForkAwesome.h>

#include <unordered_map>


bool isComponentFloatingPoint( const ComponentType& compType )
{
    switch ( compType )
    {
    case ComponentType::Int8:
    case ComponentType::UInt8:
    case ComponentType::Int16:
    case ComponentType::UInt16:
    case ComponentType::Int32:
    case ComponentType::UInt32:
    case ComponentType::ULong:
    case ComponentType::Long:
    case ComponentType::ULongLong:
    case ComponentType::LongLong:
    {
        return false;
    }

    case ComponentType::Float32:
    case ComponentType::Float64:
    case ComponentType::LongDouble:
    {
        return true;
    }

    case ComponentType::Undefined:
    {
        return false;
    }
    }

    return false;
}


bool isComponentUnsignedInt( const ComponentType& compType )
{
    switch ( compType )
    {
    case ComponentType::Int8:
    case ComponentType::Int16:
    case ComponentType::Int32:
    case ComponentType::Long:
    case ComponentType::LongLong:
    case ComponentType::Float32:
    case ComponentType::Float64:
    case ComponentType::LongDouble:
    {
        return false;
    }

    case ComponentType::UInt8:
    case ComponentType::UInt16:
    case ComponentType::UInt32:
    case ComponentType::ULong:
    case ComponentType::ULongLong:
    {
        return true;
    }

    case ComponentType::Undefined:
    {
        return false;
    }
    }

    return false;
}


std::string typeString( const MouseMode& mouseMode )
{
    static const std::unordered_map< MouseMode, std::string > s_typeToStringMap
    {
        { MouseMode::Pointer, "Pointer (V)\nMove the crosshairs" },
        { MouseMode::WindowLevel, "Window/level and opacity (L)\nLeft button: window/level\nRight button: opacity" },
        { MouseMode::CameraTranslate2D, "Pan view (X)" },
        { MouseMode::CameraRotate, "Rotate view\nLeft button: rotate in plane\nRight button: rotate out of plane" },
        { MouseMode::CameraZoom, "Zoom view (Z)\nLeft button: zoom to crosshairs\nRight button: zoom to cursor" },
        { MouseMode::Segment, "Segment (B)\nLeft button: paint foreground label\nRight button: paint background label" },
        { MouseMode::Annotate, "Annotate (N)" },
        { MouseMode::ImageTranslate, "Translate image (T)\nLeft button: translate in plane\nRight button: translate out of plane" },
        { MouseMode::ImageRotate, "Rotate image (R)\nLeft button: rotate in plane\nRight button: rotate out of plane" },
        { MouseMode::ImageScale, "Scale image (Y)" }
    };

    return s_typeToStringMap.at( mouseMode );
}


const char* toolbarButtonIcon( const MouseMode& mouseMode )
{
    static const std::unordered_map< MouseMode, const char* > s_typeToIconMap
    {
        { MouseMode::Pointer, ICON_FK_MOUSE_POINTER },
        { MouseMode::Segment, ICON_FK_PAINT_BRUSH },
        { MouseMode::Annotate, ICON_FK_PENCIL },
        { MouseMode::WindowLevel, ICON_FK_ADJUST },
        { MouseMode::CameraTranslate2D, ICON_FK_HAND_PAPER_O },
        { MouseMode::CameraRotate, ICON_FK_FUTBOL_O },
        { MouseMode::CameraZoom, ICON_FK_SEARCH },
        { MouseMode::ImageTranslate, ICON_FK_ARROWS },
        { MouseMode::ImageRotate, ICON_FK_UNDO },
        { MouseMode::ImageScale, ICON_FK_EXPAND }
    };

    return s_typeToIconMap.at( mouseMode );
}
