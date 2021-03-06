#ifndef VIEW_HIT_H
#define VIEW_HIT_H

#include "windowing/View.h"

#include <glm/glm.hpp>
#include <uuid.h>

class AppData;


/**
 * @brief When a view is hit by a mouse/pointer click, this structure is used to
 * return data about the view that was hit, including its ID, a reference to the view,
 * and the hit position in Clip space of the view.
 */
struct ViewHit
{
    /// @todo Make this a reference so that we never have to check for null!!!
    /// This requires a copy constructor, etc.
    View* view = nullptr;
    uuids::uuid viewUid;

    glm::vec2 windowClipPos;
    glm::vec2 viewClipPos;
    glm::vec4 worldPos;
    glm::vec4 worldPos_offsetApplied;
    glm::vec3 worldFrontAxis;
};


/// @todo Interface instead of appData
std::optional<ViewHit> getViewHit(
        AppData& appData,
        const glm::vec2& windowPos,
        const std::optional<uuids::uuid>& viewUidForOverride = std::nullopt );

#endif // VIEW_HIT_H
