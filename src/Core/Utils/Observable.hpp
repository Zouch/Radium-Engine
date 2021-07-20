#pragma once

#include <Core/Utils/Log.hpp>

#include <functional>
#include <map>

namespace Ra {
namespace Core {
namespace Utils {

/// Simple observable implementation with void observer(Args...) notification.
/// Typical usage is to derive from Observer with the intended \p Args as in
/// \code{.cpp}
/// class MyClass : public Oberservable<int> [...]
/// MyClass c;
/// c.attach(functor)
/// \endcode
/// Also a class can have Observable members, than act as slots
/// \code{.cpp}
/// class MyClass {
/// public :
///   Oberservable<int> slot0;
///   Oberservable<> slot1;
///   void f(){ slot0.notify(10); }
/// };
/// MyClass c;
/// c.slot0.attach(functor)
/// c.slot1.attach(functor)
/// \endcode

template <typename... Args>
class Observable
{
  public:
    /// Observer functor type
    using Observer = std::function<void( Args... )>;

    /// Default constructor ... do nothing ;)
    Observable()                    = default;
    Observable( const Observable& ) = delete;
    Observable& operator=( const Observable& ) = delete;
    virtual ~Observable()                      = default;

    /// explicit copy of all attached observers the \p other Observable
    void copyObserversTo( Observable& other ) const {
        for ( const auto& o : m_observers )
        { other.attach( o.second ); }
    }

    /// Attach an \p observer that will be call on subsecant call to notify()
    /// \return An unique int to identify the observer, could be used to pass to Obeservable::detach
    inline int attach( Observer observer ) {
        m_observers.insert( { ++m_currentId, observer } );
        return m_currentId;
    }

    /// Utility function that extract the observer from a class member.
    /// Attach an \p observer that will be call on subsecant call to notify()
    /// \return An unique int to identify the observer, could be used to pass to Obeservable::detach
    template <typename T>
    int attachMember( T* object, void ( T::*observer )( Args... ) ) {
        return attach( bind( observer, object ) );
    }
    /// Notify (i.e. call) each attached observer with argument \p p
    inline void notify( Args... p ) const {
        for ( const auto& o : m_observers )
            o.second( std::forward<Args>( p )... );
    }

    /// Detach all observers
    inline void detachAll() { m_observers.clear(); }

    /// Detach the \p observerId, observerId must have been saved from a
    /// previous call to attach
    inline void detach( int observerId ) { m_observers.erase( observerId ); }

  private:
    // from https://stackoverflow.com/questions/21192659/variadic-templates-and-stdbind
    template <typename R, typename T, typename U>
    std::function<R( Args... )> bind( R ( T::*f )( Args... ), U p ) {
        return [p, f]( Args... args ) -> R { return ( p->*f )( args... ); };
    }

    std::map<int, Observer> m_observers;
    int m_currentId { 0 };
};

class RA_CORE_API ObservableVoid : public Observable<>
{};

} // namespace Utils
} // namespace Core
} // namespace Ra
