#pragma once

#include <Engine/RaEngine.hpp>

namespace Ra {
namespace Engine {
/// Structure passed to each system before they fill the task queue.
struct RA_ENGINE_API FrameInfo {
    /// The current animation time.
    Scalar m_animationTime { 0 };

    /// Time elapsed since the last frame in seconds.
    Scalar m_dt { 0 };

    /// Number of frames since the start of the application.
    // (NB : at 60 FPS on a 32 bits machine this will cycle every two years... ;) )
    uint m_numFrame { 0 };

    // Other stuff (e.g. which systems are present, etc).
};
} // namespace Engine
} // namespace Ra
