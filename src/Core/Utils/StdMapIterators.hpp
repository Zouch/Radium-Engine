#pragma once

#include <map>
#include <type_traits> // conditionals
#include <unordered_map>

/// \file This file contains utilities and wrapper to iterate over Map and OrderedMap values/keys
/// Source: https://gist.github.com/eruffaldi/93d09ed6644ae3fa279f
namespace Ra {
namespace Core {
namespace Utils {

/// \brief Generate a range to iterate over the keys of a map
///
/// \tparam Mapclass Specialized Map or OrderedMap
///
/// Usage:
/// \snippet tests/unittest/Core/mapiterators.cpp Iterating over keys
template <class Mapclass>
struct map_keys {
    using map_t = Mapclass;

    struct iterator {
        using realiterator_t = typename std::conditional<std::is_const<map_t>::value,
                                                         typename map_t::const_iterator,
                                                         typename map_t::iterator>::type;
        using value_t        = typename std::add_const<typename map_t::key_type>::type;
        realiterator_t under;

        iterator( realiterator_t x ) : under( x ) {}

        auto operator*() -> typename std::add_lvalue_reference<value_t>::type {
            return under->first;
        }
        auto operator->() -> typename std::add_pointer<value_t>::type { return &under->first; }

        bool operator!=( const iterator& o ) const { return under != o.under; }

        iterator& operator++() {
            ++under;
            return *this;
        }
        iterator operator++( int ) {
            iterator x( *this );
            ++under;
            return x;
        }
    };

    map_keys( map_t& x ) : x_( x ) {}
    virtual inline ~map_keys() = default;

    iterator begin() { return iterator( x_.begin() ); }
    iterator end() { return iterator( x_.end() ); }
    unsigned int size() const { return x_.size(); }

    map_t& x_;
};

/// \brief Generate a range to iterate over the values of a map
///
/// \tparam Mapclass Specialized Map or OrderedMap
///
/// Usage:
/// \snippet tests/unittest/Core/mapiterators.cpp Iterating over values
template <class Mapclass>
struct map_values {
    using map_t = Mapclass;

    struct iterator {
        using realiterator_t = typename std::conditional<std::is_const<map_t>::value,
                                                         typename map_t::const_iterator,
                                                         typename map_t::iterator>::type;
        using value_t =
            typename std::conditional<std::is_const<map_t>::value,
                                      typename std::add_const<typename map_t::mapped_type>::type,
                                      typename map_t::mapped_type>::type;
        realiterator_t under;

        iterator( realiterator_t x ) : under( x ) {}

        auto operator*() -> typename std::add_lvalue_reference<value_t>::type {
            return under->second;
        }
        auto operator->() -> typename std::add_pointer<value_t>::type { return &under->second; }

        bool operator!=( const iterator& o ) const { return under != o.under; }

        iterator& operator++() {
            ++under;
            return *this;
        }
        iterator operator++( int ) {
            iterator x( *this );
            ++under;
            return x;
        }
    };

    map_values( map_t& x ) : x_( x ) {}

    iterator begin() { return iterator( x_.begin() ); }
    iterator end() { return iterator( x_.end() ); }
    unsigned int size() const { return x_.size(); }

    map_t& x_;
};
} // namespace Utils
} // namespace Core
} // namespace Ra
