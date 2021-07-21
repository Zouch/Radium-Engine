#pragma once

#include <Engine/RaEngine.hpp>

#include <mutex>

#include <Core/Utils/Observable.hpp>

#include <Engine/Scene/ItemEntry.hpp>

namespace Ra {
namespace Engine {
namespace Scene {

/** This class is used to inform the external world of events happening in the engine.
 * You can register a callback function which will be called any time the event happens.
 * Signals are fired when an object of the engine (entity, component or render object)
 * is either added or removed.
 * Signals on entities, components and render objects send the item entry indicating the object
 * that was added or removed.
 *
 * Signals for new objects are fired just after they have been initialized and acquired
 * by the object owning them.
 * Signals for destroyed objects are fired just before they are removed from the object
 * owning them and destroyed
 *
 * Signals of end of frame send no parameters.
 **/
class RA_ENGINE_API SignalManager
{

  public:
    /// Notifies all observers of an entity creation
    void fireEntityCreated( const ItemEntry& entity ) const;
    /// Notifies all observers of an entity removal
    void fireEntityDestroyed( const ItemEntry& entity ) const;

    /// Notifies all observers of a component creation
    void fireComponentAdded( const ItemEntry& component ) const;
    /// Notifies all observers of a component removal
    void fireComponentRemoved( const ItemEntry& component ) const;

    /// Notifies all observers of a render object creation
    void fireRenderObjectAdded( const ItemEntry& ro ) const;
    /// Notifies all observers of a render object removal
    void fireRenderObjectRemoved( const ItemEntry& ro ) const;

    /// Notifies all observers of a end of frame event
    void fireFrameEnded() const;

    /// Enable/disable the notification of observers
    void setOn( bool on ) { m_isOn = on; }

    /// Type for item (entity, component or render object) observable
    using ItemObservable = Ra::Core::Utils::Observable<const ItemEntry&>;
    /// Type for frame observable
    using FrameObservable = Ra::Core::Utils::Observable<>;

    /// Access to the observable members
    ///@{
    ItemObservable& getEntityCreatedNotifier();
    ItemObservable& getEntityDestroyedNotifier();
    ItemObservable& getComponentCreatedNotifier();
    ItemObservable& getComponentDestroyedNotifier();
    ItemObservable& getRenderObjectCreatedNotifier();
    ItemObservable& getRenderObjectDestroyedNotifier();
    FrameObservable& getEndFrameNotifier();
    ///@}

  private:
    /// lock to prevent race conditions on observables
    mutable std::mutex m_mutex;

    /// State of the signal manager
    bool m_isOn { true };

    /// Item observables
    ///@{
    ItemObservable m_entityCreatedCallbacks;
    ItemObservable m_entityDestroyedCallbacks;
    ItemObservable m_componentAddedCallbacks;
    ItemObservable m_componentRemovedCallbacks;
    ItemObservable m_roAddedCallbacks;
    ItemObservable m_roRemovedCallbacks;
    ///@}

    /// Frame observable
    ///@{
    FrameObservable m_frameEndCallbacks;
    ///@}

    /// Helper function to notify observers
    template <typename... TArgs>
    void notify( const Ra::Core::Utils::Observable<TArgs...>& o, TArgs... args ) const {
        if ( m_isOn ) {
            std::lock_guard<std::mutex> lock( m_mutex );
            o.notify( args... );
        }
    }
};
} // namespace Scene
} // namespace Engine
} // namespace Ra
