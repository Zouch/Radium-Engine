#pragma once

#include <Core/Utils/Timer.hpp>

namespace Ra {
namespace Core {
namespace Utils {

/**!
 * @brief The Chrono class represents a chronometer for timing generic functions
 * in an easy way.
 *
 * Example:
 *
 *      void foo( foo_args ... ) { ... }
 *      some_type bar( bar_args ... ) { ... }
 *
 *      Chrono foo_time;
 *      foo_time.run( foo, foo_args... );
 *
 *      Chrono bar_time;
 *      some_type bar_result = bar_time< same_type >.run( bar, foo_args... );
 *
 *      if( foo_time == bar_time ) {
 *          std::cout << "time is equal";
 *      } else {
 *          if( foo_time < bar_time ) {
 *              std::cout << "foo is faster";
 *          } else {
 *              std::cout << "bar is faster";
 *          }
 *      }
 *
 * \note Note that bar( bar_args ...) == bar_time< same_type >.run( bar, bar_args... )
 * \see Ra::Core::Utils::getIntervalMicro
 * \see Ra::Core::Utils::getIntervalSeconds
 */
class RA_CORE_API Chrono
{
  public:
    using MicroSeconds = long;
    using Seconds      = Scalar;
    /**
     *    \brief Default constructor.
     */
    Chrono() {}

    /**
     *    \brief Copy constructor.
     */
    Chrono( const Chrono& other ) = default;

    /**
     *    \brief Move constructor.
     */
    Chrono( Chrono&& other ) = default;

    /**
     *    \brief Destructor.
     */
    ~Chrono() {}

    /**!
     *    \brief Run the given void function f( args ... ) and times it.
     *    \param f                  The function to be timed.
     *    \param args               The parameters of f.
     *    \tparam Function          Type of the input function (automatically deduced)
     *    \tparam Args              Variadic pack with function parameters
     */
    template <class Function, class... Args>
    inline void run( Function&& f, Args&&... args ) {
        m_start = Clock::now();
        f( args... );
        m_end = Clock::now();
    }

    /**!
     *    \brief Run the given ReturnType function f( args ... ) and times it.
     *    \param f                  The function to be timed.
     *    \param args               The parameters of f.
     *    \tparam Times             Number of function run
     *    \tparam Function          Type of the input function (automatically deduced)
     *    \tparam Args              Variadic pack with function parameters
     *    \return The output of f( args ... ).
     */
    template <typename ReturnType, class Function, class... Args>
    inline ReturnType run( Function&& f, Args... args ) {
        // TODO //static_assert( /*check if ReturnType is equal to Function return type*/,
        // "RETURN_TYPE_DO_NOT_MATCH_FUNCTION_RETURN_TYPE" );
        m_start        = Clock::now();
        ReturnType res = f( args... );
        m_end          = Clock::now();
        return res;
    }

    /**!
     *    \brief Run the given function f( args ... ) n Times and compute the average timing.
     *    \param f                  The function to be timed.
     *    \param args               The parameters of f.
     *    \tparam Times             Number of function run
     *    \tparam Function          Type of the input function (automatically deduced)
     *    \tparam Args              Variadic pack with function parameters
     *    \return The average time of f( args ... ) in microseconds.
     */
    template <std::size_t Times, class Function, class... Args>
    inline MicroSeconds test( Function&& f, Args&&... args ) {
        MicroSeconds avg = 0;
        for ( std::size_t i = 0; i < Times; ++i ) {
            m_start = Clock::now();
            f( args... );
            m_end = Clock::now();
            avg += getIntervalMicro( m_start, m_end );
        }
        avg /= Times;
        m_start = Clock::now();
        m_end   = m_start + std::chrono::microseconds( avg );
        return avg;
    }

    /**!
     *    \brief Return the elapsed time for last call of run in microseconds.
     *    \return The elapsed time in microseconds.
     *    \see Ra::Core::Utils::getIntervalMicro
     */
    inline MicroSeconds elapsedMicroSeconds() const { return getIntervalMicro( m_start, m_end ); }

    /**!
     *    \brief Return the elapsed time for last call of run in seconds.
     *    \return The elapsed time in seconds.
     *    \see Ra::Core::Utils::getIntervalSeconds
     */
    inline Seconds elapsedSeconds() const { return getIntervalSeconds( m_start, m_end ); }

    /**!
     *    \brief Copy assignment operator.
     */
    inline Chrono& operator=( const Chrono& other ) = default;

    /**!
     *    \brief Move assignment operator.
     */
    inline Chrono& operator=( Chrono&& other ) = default;

    /**!
     *    \brief Equal operator.
     */
    inline bool operator==( const Chrono& other ) const {
        return ( elapsedMicroSeconds() == other.elapsedMicroSeconds() );
    }

    /**!
     *    \brief Less operator.
     */
    inline bool operator<( const Chrono& other ) const {
        return ( elapsedMicroSeconds() < other.elapsedMicroSeconds() );
    }

  protected:
    /// VARIABLE
    TimePoint m_start; ///< Time at the beginning of the function.
    TimePoint m_end;   ///< Time after running the function.
};

} // namespace Utils
} // namespace Core
} // namespace Ra
