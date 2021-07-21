#pragma once

#include <Engine/RaEngine.hpp>
#include <Engine/Scene/LightStorage.hpp>
#include <Engine/Scene/System.hpp>

#include <memory>

namespace Ra {
namespace Engine {
namespace Scene {

/**
 * Interface providing functions to manage a group or type of lights
 * in a specific way.
 */
class RA_ENGINE_API LightManager : public System
{
    // Radium-V2 : make light manager compatible with range for ...
  public:
    /// Constructor
    LightManager() = default;

    // Make copies impossible
    LightManager( const LightManager& ) = delete;
    LightManager& operator=( const LightManager& ) = delete;

    /// Virtual destructor
    ~LightManager() override;

    /// Get a pointer to the li-th Light.
    virtual const Scene::Light* getLight( size_t li ) const = 0;

    /** Add a light to the manager ...
     * Consider the component is already registered. The light manager will not take ownership of
     * the added light, it will just push the light on the storage ...
     * @param li The (already registered) light to add.
     */
    virtual void addLight( const Scene::Light* li ) = 0;

    //
    // Calls for the Renderer. Note that
    //

    /**
     * @brief Number of lights.
     * This is still a work in progress. The idea is to make it possible for a
     * LightManager to tell it has only one Light, for example if it wants to send
     * a lot of sources at once in a single RenderParams, let's say a texture.
     */
    virtual size_t count() const;

    //
    // System methods
    //
    /// Do nothing as this system only manage light related asset loading
    void generateTasks( Core::TaskQueue* taskQueue, const Engine::FrameInfo& frameInfo ) override;

    /// Transform loaded file data to usable entities and component in the engine
    void handleAssetLoading( Entity* entity, const Core::Asset::FileData* data ) override;

  protected:
    /// Inherited method marked as final to ensure correct memory management
    /// even in child classes (e.g. LightStorage).
    void registerComponent( const Entity* entity, Component* component ) final;

    /// Inherited method marked as final to ensure correct memory management
    /// even in child classes (e.g. LightStorage).
    void unregisterComponent( const Entity* entity, Component* component ) final;

    /// Inherited method marked as final to ensure correct memory management
    /// even in child classes (e.g. LightStorage).
    void unregisterAllComponents( const Entity* entity ) final;

  protected:
    /// Stores the object that stores the lights...
    std::unique_ptr<LightStorage> m_data { nullptr };
};

} // namespace Scene
} // namespace Engine
} // namespace Ra
