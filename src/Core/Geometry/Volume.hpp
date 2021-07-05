#pragma once

#include <Core/Geometry/AbstractGeometry.hpp>
#include <Core/RaCore.hpp>
#include <Core/Types.hpp>

#define RA_REQUIRE_OPTIONAL
#include <Core/Utils/StdOptional.hpp> // trigger an error if optional is not found
#undef RA_REQUIRE_OPTIONAL

#include <algorithm> // find_if

namespace Ra {
namespace Core {
namespace Geometry {

/**
 * AbstractVolume is an abstract class for Volumetric data (can be parametric or discrete, with
 * dense or sparse storage).
 *
 * \note Right now the encoded function \f$f\f$ is dimension 1, ie. \f$f:\Re^3->\Re\f$. If needed,
 *          the class can be templated so the built-in typedef #ValueType is replaced by a
 * parameter.
 *
 * \note This class could also be modified to encode n-dimensionnal textures.
 *          See this for an implementation of n-dimensionnal grids with neighbhorood queries
 *          (compile-time loop unrolling):
 *          http://github.com/STORM-IRIT/OpenGR/blob/master/src/gr/accelerators/normalset.h
 */
class RA_CORE_API AbstractVolume : public AbstractGeometry
{

  public:
    /// The type of geometry.
    enum VolumeStorageType {
        UNKNOWN         = 1 << 0,
        PARAMETRIC      = 1 << 1, /// <\brief volume defined as a function f : R^2 -> R
        DISCRETE_DENSE  = 1 << 2, /// <\brief volume defined as a function sampled in a regular grid
        DISCRETE_SPARSE = 1 << 3  /// <\brief volume defined as a function sampled in a regular grid
                                  /// with sparse storage
    };

    /// Type of value encoded in the volume
    using ValueType = Scalar;

  protected:
    AbstractVolume( const VolumeStorageType& type );

  public:
    AbstractVolume( const AbstractVolume& data ) = default;
    AbstractVolume& operator=( const AbstractVolume& ) = default;
    ~AbstractVolume()                                  = default;

    /// Return the type of geometry.
    inline VolumeStorageType getType() const { return m_type; }

  protected:
    /// Set the type of geometry.
    inline void setType( const VolumeStorageType& type ) { m_type = type; }

  public:
    /** Get the function value at a given position p
     *
     * Set to invalid when the query position p is out of bound
     */
    virtual Utils::optional<ValueType> getValue( Eigen::Ref<const Vector3> p ) const = 0;

    /// \name Status queries
    ///@{
    /// Return true if the volume is parametric
    bool isParametric() const;
    /// Return true if the volume is discrete. Can be cast as AbstractDiscreteVolume
    bool isDiscrete() const;
    /// Return true if the volume is dense (implies #isDiscrete to be true)
    bool isDense() const;
    /// Return true if the volume is sparse (implies #isDiscrete to be true)
    bool isSparse() const;
    ///@}

    /// Print info to the Debug output. Need to be extended by child classes
    void displayInfo() const;

  protected:
    /// The type of geometry for the object.
    VolumeStorageType m_type;
};
/**
 * General interface for discrete volume that store the information into a set of bins indexed in
 * 3D.
 *
 */
class RA_CORE_API AbstractDiscreteVolume : public AbstractVolume
{

  public:
    using ValueType = AbstractVolume::ValueType;
    using IndexType = Vector3i;

  protected:
    inline AbstractDiscreteVolume( const VolumeStorageType& type ) :
        AbstractVolume( type ), m_size( IndexType::Zero() ), m_binSize {1_ra, 1_ra, 1_ra} {}

  public:
    AbstractDiscreteVolume( const AbstractDiscreteVolume& data ) = default;
    AbstractDiscreteVolume& operator=( const AbstractDiscreteVolume& ) = default;
    ~AbstractDiscreteVolume() override                                 = default;

    /// Erases all data, making the geometry empty.
    void clear() override;

    /// Compute the aabb of the volume
    Aabb computeAabb() const override;

    /// return the size (number of bins ni each dimension) of the volume
    const Vector3i& size() const { return m_size; }
    /// \warning Clears existing data
    void setSize( Eigen::Ref<const Vector3i> size ) {
        m_size = size;
        updateStorage();
        invalidateAabb();
    }
    /// return the bin size
    const Vector3& binSize() const { return m_binSize; }

    /// Set the bin size
    void setBinSize( Eigen::Ref<const Vector3> binSize ) {
        CORE_ASSERT( binSize != Vector3::Zero(), "Volume bin size can't be zero." );
        m_binSize = binSize;
        invalidateAabb();
    }

    /// Get the value of the given bin
    inline Utils::optional<ValueType> getBinValue( Eigen::Ref<const IndexType> p ) const {
        if ( auto res = linearIndex( p ) ) return getBinValue( *res );
        return {};
    }

    /** Get the function value at a given position p (discrete implementation).
     * \warning no bounds checking on the parameter p
     * \see #getBinValue
     */
    inline Utils::optional<ValueType> getValue( Eigen::Ref<const Vector3> p ) const override final {
        return getBinValue( ( p.cwiseQuotient( m_binSize ) ).cast<typename IndexType::Scalar>() );
    }

    /**
     * Increment bin p by value.
     * \note : does nothing if p is out of bounds.
     */
    inline bool addToBin( const ValueType& value, Eigen::Ref<const IndexType> p ) {
        if ( auto res = linearIndex( p ) )
        {
            addToBin( value, *res );
            invalidateAabb();
            return true;
        }
        return false;
    }

  protected:
    /// Convert the 3D position into a linear index on the bin set
    inline Utils::optional<typename IndexType::Scalar>
    linearIndex( Eigen::Ref<const IndexType> p ) const {
        using Integer = typename IndexType::Scalar;
        if ( ( p.array() >= m_size.array() ).any() ) return {};
        return p.dot( IndexType( Integer( 1 ), m_size( 0 ), m_size( 0 ) * m_size( 1 ) ) );
    }
    /// Get the bin value
    virtual Utils::optional<ValueType> getBinValue( typename IndexType::Scalar idx ) const = 0;
    /// Add a value to the bin.
    virtual void addToBin( const ValueType& value, typename IndexType::Scalar idx ) = 0;

    /// Method called when size as been updated
    virtual void updateStorage() = 0;

  private:
    IndexType m_size;  /// <\brief number of bins per dimension
    Vector3 m_binSize; /// <\brief size of a bin per dimension
};

/// Discrete volume data storing values in a regular grid
class RA_CORE_API VolumeGrid : public AbstractDiscreteVolume
{
  public:
    using ValueType = AbstractDiscreteVolume::ValueType;
    using IndexType = AbstractDiscreteVolume::IndexType;

    using Container = std::vector<ValueType>;

    // For now, assume that AbstractDiscreteVolume::ValueType is Scalar
    using Gradientcontainer = std::vector<Vector3>;

  public:
    inline VolumeGrid( const ValueType& defaultValue = ValueType( 0. ) ) :
        AbstractDiscreteVolume( DISCRETE_DENSE ), m_defaultValue( defaultValue ) {}
    VolumeGrid( const VolumeGrid& data ) = default;
    VolumeGrid& operator=( const VolumeGrid& ) = default;
    ~VolumeGrid() override                     = default;

    using AbstractDiscreteVolume::addToBin;
    using AbstractDiscreteVolume::getBinValue;

    /// Direct access to the managed data
    inline const Container& data() const { return m_data; }
    /// Direct access, with modification allowed to the managed data
    inline Container& data() { return m_data; }

    /// Add a value to all bins
    inline void addToAllBins( const ValueType& value ) {
        for ( auto& v : m_data )
        {
            v += value;
        }
    }

    /// Test if gradients are defined
    bool hasGradients() const { return m_data.size() == m_gradient.size(); }

    /// Generate gradients from data
    void computeGradients();

    /// Direct access to the managed gradients
    inline const Gradientcontainer& gradient() const { return m_gradient; }
    /// Direct access, with modification allowed to the managed gradients
    inline Gradientcontainer& gradient() { return m_gradient; }

  protected:
    /// Get the function value a given position p
    /// \warning no bounds checking on the parameter p
    inline Utils::optional<ValueType> getBinValue( typename IndexType::Scalar idx ) const override {
        return m_data[size_t( idx )];
    }

    /// Add a value to the given bin
    /// \warning no bounds checking on the parameter idx
    inline void addToBin( const ValueType& value, typename IndexType::Scalar idx ) override {
        m_data[size_t( idx )] += value;
    }

    /// \warning This method needs to be updated in case we switch to multidimensionnal functions
    inline void updateStorage() override {
        m_data.resize( size_t( size().prod() ), m_defaultValue );
    }

  private:
    /// sample the volume at (i, j, k) with "clamp to border" behavior
    ValueType sample( const IndexType& i );

    ValueType m_defaultValue;
    Container m_data;
    Gradientcontainer m_gradient;
}; // class VolumeGrid

/** Discrete volume data with sparse storage
 *
 * Samples are stored as SparseVolumeData::sample, which stores the bin linear
 * index, and the function value
 */
class RA_CORE_API VolumeSparse : public AbstractDiscreteVolume
{
  public:
    using ValueType = AbstractDiscreteVolume::ValueType;
    using IndexType = AbstractDiscreteVolume::IndexType;
    struct SampleType {
        int index;
        ValueType value;

        inline SampleType( int idx, const ValueType& v ) : index( idx ), value( v ) {}
    };
    using Container = std::vector<SampleType>;

  public:
    inline VolumeSparse() : AbstractDiscreteVolume( DISCRETE_SPARSE ) {}
    VolumeSparse( const VolumeSparse& data ) = default;
    VolumeSparse& operator=( const VolumeSparse& ) = default;
    ~VolumeSparse() override                       = default;

    using AbstractDiscreteVolume::addToBin;
    using AbstractDiscreteVolume::getBinValue;

  protected:
    /** Get the function value at a given position p (if the bin exists)
     *
     * Returns an invalid value when no sample is registered in the targeted bin.
     *
     * \complexity At most last - first applications of the predicate
     * (http://en.cppreference.com/w/cpp/algorithm/find)
     */
    inline Utils::optional<ValueType> getBinValue( typename IndexType::Scalar idx ) const override {
        auto res = findBin( idx );
        if ( res != std::end( m_data ) ) return res->value;
        return {};
    }

    /// Increment bin p by value
    ///
    /// Create the bin if not already existing
    ///
    /// \complexity At most last - first applications of the predicate
    /// (http://en.cppreference.com/w/cpp/algorithm/find)
    inline void addToBin( const ValueType& value, typename IndexType::Scalar idx ) override {
        auto res = findBin( idx );
        if ( res != std::end( m_data ) )
            res->value += value;
        else
            m_data.emplace_back( idx, value );
    }

    inline void updateStorage() override { m_data.clear(); }

  private:
    inline Container::iterator findBin( typename IndexType::Scalar idx ) {
        return std::find_if( std::begin( m_data ),
                             std::end( m_data ),
                             [&idx]( const SampleType& s ) { return s.index == idx; } );
    }
    inline Container::const_iterator findBin( typename IndexType::Scalar idx ) const {
        return std::find_if( std::begin( m_data ),
                             std::end( m_data ),
                             [&idx]( const SampleType& s ) { return s.index == idx; } );
    }

  private:
    Container m_data;

}; // class VolumeSparse

} // namespace Geometry
} // namespace Core
} // namespace Ra
