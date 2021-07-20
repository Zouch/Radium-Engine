#pragma once

#include <Core/Types.hpp>
#include <Engine/RaEngine.hpp>

namespace Ra {
namespace Engine {
namespace Data {
/// the set of viewing parameters extracted from the camera and given to the renderer
struct RA_ENGINE_API ViewingParameters {
    Core::Matrix4 viewMatrix { Core::Matrix4::Identity() };
    Core::Matrix4 projMatrix { Core::Matrix4::Identity() };
    Scalar dt { 0 };
};
} // namespace Data
} // namespace Engine
} // namespace Ra
