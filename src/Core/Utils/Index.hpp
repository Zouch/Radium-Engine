#ifndef RADIUMENGINE_INDEX_HPP
#define RADIUMENGINE_INDEX_HPP

#include <Core/RaCore.hpp>
#include <limits>

namespace Ra {
namespace Core {
namespace Utils {

class RA_CORE_API Index
{
  public:
    /// CONSTRUCTOR
    /// Default constructor that allow implicit conversion from integer to Index
    constexpr explicit Index( int i = s_invalid );
    constexpr Index( const Index& i );
    constexpr Index& operator=( long int i );

    /// DESTRUCTOR: Must not be defined, we need it trivial to be
    /// constexpr
    // ~Index() { }

    /// VALID
    constexpr bool isValid() const;

    /// INVALID
    constexpr bool isInvalid() const;
    constexpr void setInvalid();
    static constexpr Index Invalid();
    static constexpr Index Max();

    /// INDEX
    constexpr int getValue() const;
    constexpr void setValue( const int i );

    /// CAST
    constexpr operator int() const; // required for `array[idx]`

    /// OPERATOR
    constexpr Index& operator=( const Index& id );
    constexpr Index& operator++();
    constexpr Index& operator--();

    constexpr Index operator+( const Index& id );
    template <typename Integer>
    constexpr Index operator+( const Integer& i );

    constexpr Index operator-( const Index& id );
    template <typename Integer>
    constexpr Index operator-( const Integer& i );

    constexpr bool operator==( const Index& id );
    template <typename Integer>
    constexpr bool operator==( const Integer& i );

    constexpr bool operator!=( const Index& id );
    template <typename Integer>
    constexpr bool operator!=( const Integer& i );

    constexpr bool operator<( const Index& id1 );
    template <typename Integer>
    constexpr bool operator<( const Integer& i );

    constexpr bool operator<=( const Index& id1 );
    template <typename Integer>
    constexpr bool operator<=( const Integer& i );

    constexpr bool operator>( const Index& id1 );
    template <typename Integer>
    constexpr bool operator>( const Integer& i );

    constexpr bool operator>=( const Index& id1 );
    template <typename Integer>
    constexpr bool operator>=( const Integer& i );

  protected:
    /// VARIABLE
    int m_idx{s_invalid};

  private:
    /// CONSTANT
    static const int s_invalid{-1};
    static const int s_maxIdx{std::numeric_limits<int>::max()};
};
} // namespace Utils
} // namespace Core
} // namespace Ra

#include <Core/Utils/Index.inl>

#endif // RADIUMENGINE_INDEX_HPP
