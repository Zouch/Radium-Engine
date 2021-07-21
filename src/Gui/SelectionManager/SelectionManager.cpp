#include <Gui/SelectionManager/SelectionManager.hpp>

#include <Core/Utils/Log.hpp>
#include <Engine/RadiumEngine.hpp>

using Ra::Engine::Scene::ItemEntry;

namespace Ra {
namespace Gui {

using namespace Core::Utils; // log

SelectionManager::SelectionManager( ItemModel* model, QObject* parent ) :
    QItemSelectionModel( model, parent ) {
    connect( this, &SelectionManager::selectionChanged, this, &SelectionManager::printSelection );
    connect( model, &ItemModel::modelRebuilt, this, &SelectionManager::onModelRebuilt );
}

bool SelectionManager::isSelected( const ItemEntry& ent ) const {
    QModelIndex idx = itemModel()->findEntryIndex( ent );
    if ( idx.isValid() ) {
        const auto& pos = std::find( selectedIndexes().begin(), selectedIndexes().end(), idx );
        return ( pos != selectedIndexes().end() );
    }
    return false;
}

std::vector<ItemEntry> SelectionManager::selectedEntries() const {
    std::vector<ItemEntry> result;
    result.reserve( uint( selectedIndexes().size() ) );
    for ( const auto& idx : selectedIndexes() ) {
        result.push_back( itemModel()->getEntry( idx ) );
        CORE_ASSERT( result.back().isValid(), "Invalid entry in selection" );
    }
    return result;
}

const ItemEntry& SelectionManager::currentItem() const {
    // getEntry returns an invalid entry when given an invalid index.
    return itemModel()->getEntry( currentIndex() );
}

void SelectionManager::select( const ItemEntry& ent, QItemSelectionModel::SelectionFlags command ) {
    QModelIndex idx = itemModel()->findEntryIndex( ent );
    if ( idx.isValid() && ent.isSelectable() ) { QItemSelectionModel::select( idx, command ); }
}

void SelectionManager::setCurrentEntry( const ItemEntry& ent,
                                        QItemSelectionModel::SelectionFlags command ) {
    QModelIndex idx = itemModel()->findEntryIndex( ent );
    if ( idx.isValid() ) {
        QItemSelectionModel::setCurrentIndex( idx, command );
        emit currentChanged( idx, idx );
    }
}

void SelectionManager::onModelRebuilt() {
    clear();
}

void SelectionManager::printSelection() const {
    LOG( logDEBUG ) << "Selected entries : ";
    for ( const auto& ent : selectedEntries() ) {
        LOG( logDEBUG ) << getEntryName( Ra::Engine::RadiumEngine::getInstance(), ent );
    }
    LOG( logDEBUG ) << "Current : "
                    << getEntryName( Ra::Engine::RadiumEngine::getInstance(), currentItem() );
}
} // namespace Gui
} // namespace Ra
