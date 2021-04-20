#include <Core/Asset/HandleData.hpp>

namespace Ra {
namespace Core {
namespace Asset {

HandleComponentData::HandleComponentData() : m_name( "" ) {}

HandleData::HandleData( const std::string& name, const HandleType& type ) :
    AssetData( name ), m_type( type ) {}

HandleData::~HandleData() {}

} // namespace Asset
} // namespace Core
} // namespace Ra
