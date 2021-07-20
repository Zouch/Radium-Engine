#pragma once

#include <Core/RaCore.hpp>
#include <Core/Utils/Index.hpp>

namespace Ra {
namespace Core {
namespace Utils {

/**
 * Base class for objects that will be used through an index.
 */
class IndexedObject
{
  public:
    /// CONSTRUCTOR
    explicit inline IndexedObject( Index idx = Index::Invalid() ) : m_idx { idx } {}
    inline IndexedObject( const IndexedObject& id_obj ) = default;
    virtual inline ~IndexedObject()                     = default;

    inline void setIndex( const Index& idx ) { m_idx = idx; }
    inline const Index& getIndex() const { return m_idx; }

  protected:
    /// VARIABLE
    Index m_idx;
};

} // namespace Utils
} // namespace Core
} // namespace Ra
