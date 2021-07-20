#include <Gui/TreeModel/EntityTreeModel.hpp>

#include <stack>

#include <Engine/Scene/Component.hpp>
#include <Engine/Scene/Entity.hpp>

#include <Engine/Rendering/RenderObject.hpp>
#include <Engine/Rendering/RenderObjectManager.hpp>
#include <Engine/Scene/EntityManager.hpp>

using Ra::Engine::Scene::ItemEntry;

namespace Ra {
namespace Gui {

void ItemModel::buildModel() {
    m_rootItem.reset( new EngineTreeItem );
    m_rootItem->m_parent = nullptr;

    auto roManager = m_engine->getRenderObjectManager();
    for ( const auto& ent : m_engine->getEntityManager()->getEntities() ) {
        EngineTreeItem* entityItem = new EngineTreeItem;
        entityItem->m_entry        = ItemEntry( ent );
        entityItem->m_parent       = m_rootItem.get();
        for ( const auto& comp : ent->getComponents() ) {
            if ( comp ) {
                EngineTreeItem* componentItem = new EngineTreeItem;
                componentItem->m_entry        = ItemEntry( ent, comp.get() );
                componentItem->m_parent       = entityItem;
                for ( const auto& roIdx : comp->m_renderObjects ) {
                    EngineTreeItem* roItem = new EngineTreeItem;
                    roItem->m_entry        = ItemEntry( ent, comp.get(), roIdx );
                    roItem->m_parent       = componentItem;
                    roItem->setChecked( roManager->getRenderObject( roIdx )->isVisible() );
                    componentItem->m_children.emplace_back( roItem );
                }
                entityItem->m_children.emplace_back( componentItem );
            }
        }
        m_rootItem->m_children.emplace_back( entityItem );
    }
}

const ItemEntry& ItemModel::getEntry( const QModelIndex& index ) const {
    return static_cast<EngineTreeItem*>( getItem( index ) )->m_entry;
}

QModelIndex ItemModel::findEntryIndex( const ItemEntry& entry ) const {
    if ( entry.isValid() ) {
        std::stack<TreeItem*> stack;
        stack.push( m_rootItem.get() );
        while ( !stack.empty() ) {
            EngineTreeItem* item = static_cast<EngineTreeItem*>( stack.top() );
            stack.pop();
            // Found item, so build the index corresponding to it.
            if ( item->m_entry == entry ) {
                return createIndex( item->getIndexInParent(), 0, item );
            }

            // Add children to the stack
            for ( const auto& child : item->m_children ) {
                stack.push( child.get() );
            }
        }
    }

    return QModelIndex();
}

void ItemModel::addItem( const Engine::Scene::ItemEntry& ent ) {
    CORE_ASSERT( ent.isValid(), "Inserting invalid entry" );
    CORE_ASSERT( !findEntryIndex( ent ).isValid(), "Entry already in model" );
    auto roManager = m_engine->getRenderObjectManager();

    if ( !findEntryIndex( ent ).isValid() ) {
        TreeItem* parentItem;
        QModelIndex parentIdx;
        if ( ent.isEntityNode() ) { parentItem = m_rootItem.get(); }
        else {
            ItemEntry parentEntry;
            if ( ent.isRoNode() ) { parentEntry = ItemEntry( ent.m_entity, ent.m_component ); }
            else if ( ent.isComponentNode() ) {
                parentEntry = ItemEntry( ent.m_entity );
            }
            parentIdx = findEntryIndex( parentEntry );
            CORE_ASSERT( parentIdx.isValid(), "Parent does not exist" );
            parentItem = getItem( parentIdx );
        }

        CORE_ASSERT( getItem( parentIdx ) == parentItem, "Model inconsistency" );

        EngineTreeItem* childItem = new EngineTreeItem;
        childItem->m_entry        = ent;
        childItem->m_parent       = parentItem;
        if ( ent.isRoNode() ) {
            childItem->setChecked( roManager->getRenderObject( ent.m_roIndex )->isVisible() );
            // update parent checkbox if every ro aren't visible
            bool checked = roManager->getRenderObject( ent.m_roIndex )->isVisible();
            auto pent    = static_cast<EngineTreeItem*>( parentItem )->m_entry.m_component;
            for ( const auto& roIdx : pent->getRenderObjects() ) {
                checked = checked || roManager->getRenderObject( roIdx )->isVisible();
            }

            parentItem->setChecked( checked );
        }
        else if ( ent.isComponentNode() ) {
            //
            bool checked = false;
            for ( const auto& roIdx : ent.m_component->getRenderObjects() ) {
                checked = checked || roManager->getRenderObject( roIdx )->isVisible();
            }
            childItem->setChecked( checked );
        }

        int row = int( parentItem->m_children.size() );
        beginInsertRows( parentIdx, row, row );
        parentItem->m_children.emplace_back( childItem );
        endInsertRows();
    }
}

void ItemModel::removeItem( const Engine::Scene::ItemEntry& ent ) {
    QModelIndex entryIndex = findEntryIndex( ent );
    CORE_ASSERT( ent.isValid(), "Removing invalid entry" );
    CORE_ASSERT( entryIndex.isValid(), "Entry not in model" );
    if ( entryIndex.isValid() ) {
        EngineTreeItem* toRemove = static_cast<EngineTreeItem*>( getItem( entryIndex ) );
        TreeItem* parentItem     = toRemove->m_parent;
        auto& childList          = parentItem->m_children;
        const auto childPos =
            std::find_if( childList.begin(), childList.end(), [toRemove]( const auto& ptr ) {
                return ptr.get() == toRemove;
            } );

        CORE_ASSERT( childPos != childList.end(), "Child not in parent's list" );
        int row = toRemove->getIndexInParent();
        CORE_ASSERT( childPos - childList.begin() == row, "Iterator consistency error" );
        beginRemoveRows( parent( entryIndex ), row, row );
        parentItem->m_children.erase( childPos );
        endRemoveRows();
    }
}

void ItemModel::onDataChanged( const QModelIndex& topLeft,
                               const QModelIndex& bottomRight,
                               const QVector<int>& roles ) {
    if ( topLeft == bottomRight && roles.size() == 1 && roles.first() == Qt::CheckStateRole ) {
        auto index = topLeft;
        if ( index.isValid() ) {
            bool visible = getItem( index )->isChecked();
            auto ent     = getEntry( index );

            if ( ent.isValid() && ent.isRoNode() ) {
                emit visibilityROChanged( ent.m_roIndex, visible );
            }
        }
    }
}

int TreeItem::getIndexInParent() const {
    CORE_ASSERT( m_parent, "Looking for the root item's index." );
    for ( uint i = 0; i < m_parent->m_children.size(); ++i ) {
        if ( m_parent->m_children[i].get() == this ) { return i; }
    }
    // If we reached here, it means that the item
    // was not found in its parent's child list.
    // indicating the tree is corrupted.
    CORE_ASSERT( false, " Did not find child in parent" );
    return -1;
}

} // namespace Gui
} // namespace Ra
