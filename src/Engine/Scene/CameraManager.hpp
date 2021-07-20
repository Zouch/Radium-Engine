#pragma once

#include <Engine/RaEngine.hpp>
#include <Engine/Scene/CameraStorage.hpp>
#include <Engine/Scene/System.hpp>

#include <memory>

namespace Ra {
namespace Engine {
namespace Scene {

/**
 * Interface providing functions to manage a group or type of Cameras
 * in a specific way.
 * CameraManager handle one "active camera" for rendering as m_activeCamera;
 * this rendering camera has no Component associated, and it can retrieve it's data from any managed
 * camera with activate(index).
 * Manipulator and renderer can access (and even store as a visiting ptr) this data with
 * getActiveCamera()
 * Default camera data is used to intializad camera. Client code can set this static data to control
 * default camera initialization.
 */
class RA_ENGINE_API CameraManager : public System
{
    // Radium-V2 : make Camera manager compatible with range for ...
  public:
    /// Constructor
    CameraManager();

    // Make copies impossible
    CameraManager( const CameraManager& ) = delete;
    CameraManager& operator=( const CameraManager& ) = delete;

    /// Virtual destructor
    ~CameraManager() override = default;

    /// Get a pointer to the i-th Camera.
    virtual const CameraComponent* getCamera( size_t i ) const = 0;

    /// Add a Camera to the manager ...
    virtual void addCamera( CameraComponent* cam ) = 0;

    /// get camera component's index, if this component is managed.
    /// \return valid index if component is managed, invalid index in other case.
    virtual Core::Utils::Index getCameraIndex( const CameraComponent* cam );

    /// Add a default camera
    virtual void initialize();

    /// activate camera at index given its index.
    /// index-th camera data is copied to the "default camera" at index 0.
    /// width and height of camera 0 is kept during this process, since it's likely to be screen
    /// size, while index-th camera width height might be something else.
    /// \param index: camera's index to activate, if invalid or out of bound, activation is ignored.
    void activate( Core::Utils::Index index );

    //
    // Calls for the Renderer
    //

    /**
     * @brief Number of Cameras.
     * This is still a work in progress. The idea is to make it possible for a
     * CameraManager to tell it has only one Camera, for example if it wants to send
     * a lot of sources at once in a single RenderParams, let's say a texture.
     */
    // I dont' get the idea of texture camera storage here, event if it's possible for some kind of
    // multi view synthesis, the camera manager manages Components, do this idea of texture means
    // the components share some data where the know how to read/write the chunk ? (dlyr.)
    virtual size_t count() const;

    //
    // System methods
    //
    void generateTasks( Core::TaskQueue* taskQueue, const Engine::FrameInfo& frameInfo ) override;

    void handleAssetLoading( Entity* entity, const Core::Asset::FileData* data ) override;

    /// this static data member handles default camera values.
    /// can be tweaked after ctor and before initalization, or for any kind of reset.
    static Ra::Core::Asset::Camera defaultCamera;

    Ra::Core::Asset::Camera* getActiveCamera() { return &m_activeCamera; }
    void resetActiveCamera() { m_activeCamera = defaultCamera; }

  protected:
    /** Inherited method marked as final to ensure correct memory management
     *  even in child classes (e.g. CameraStorage).
     */
    void registerComponent( const Entity* entity, Component* component ) final;

    /// Inherited method marked as final to ensure correct memory management
    /// even in child classes (e.g. CameraStorage).
    void unregisterComponent( const Entity* entity, Component* component ) final;

    /// Inherited method marked as final to ensure correct memory management
    /// even in child classes (e.g. CameraStorage).
    void unregisterAllComponents( const Entity* entity ) final;

  protected:
    /// Stores the object that stores the Cameras...
    std::unique_ptr<CameraStorage> m_data { nullptr };

    /// active camera data, active camera hasn't any component just pure data.
    Ra::Core::Asset::Camera m_activeCamera;
};

} // namespace Scene
} // namespace Engine
} // namespace Ra
