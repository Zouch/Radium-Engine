#pragma once

#include <Gui/RaGui.hpp>

#include <memory>
#include <vector>

#include <QAbstractItemModel>
#include <QVariant>

/// This file contains a basic implementation of a tree model object with QAbstractItemModel
/// See http://doc.qt.io/qt-5/qtwidgets-itemviews-simpletreemodel-example.html
/// This could have been a template with the object type, but QObjects cannot be
/// templated, which is very sad.
/// To use this base class you must :
/// * create a derived class from TreeItem which links the model with actual objects
/// for example by storing a handle or a pointer.
/// * create a derived class from TreeModel. Add items of the derived TreeItem class
/// to the tree, and static_cast the result of getItem to the concrete class to use it.

namespace Ra {
namespace Gui {

/**
 * Base class for element of the tree representation.
 * It just maintains a tree structure leaving the data storage to derived classes.
 */
class RA_GUI_API TreeItem
{
  public:
    TreeItem() {}
    virtual ~TreeItem() {}

    /// TreeItems are non-copyable
    TreeItem( const TreeItem& ) = delete;
    TreeItem& operator=( const TreeItem& ) = delete;

    /// \name Interface
    /// \{
    /// Returns the name of the represented item to be
    /// displayed in the Qt tree.
    virtual std::string getName() const = 0;

    /// Return true if  the represented object is valid.
    virtual bool isValid() const = 0;

    /// Return true if the represented object can be selected.
    virtual bool isSelectable() const = 0;
    /// \}

    /// \name Other functions
    /// \{

    /// Return the index of this object in the parents' child list.
    /// Of course this won't work for the root item.
    int getIndexInParent() const;

    bool isChecked() const { return m_checked; }
    void setChecked( bool checked = true ) { m_checked = checked; }
    /// \}
  public:
    /// \name Tree structure variables
    /// \{
    /// Parent object of item.
    TreeItem* m_parent { nullptr };

    /// Children of item in the tree.
    std::vector<std::unique_ptr<TreeItem>> m_children;
    /// \}
  private:
    bool m_checked { true };
};

/// This class implement QAbstractItem model with the TreeItem as its model
/// Derived class must use some kind of cast to access the derived TreeItems.
class RA_GUI_API TreeModel : public QAbstractItemModel
{
    Q_OBJECT

  public:
    TreeModel( QObject* parent = nullptr ) : QAbstractItemModel( parent ) {}

    /// \name QAbstractItemModel interface
    /// \{
    /** Returns the number of rows, i.e .the number of children of a given parent item.
     *
     * @param parent
     * @return
     */
    int rowCount( const QModelIndex& parent = QModelIndex() ) const override;

    /** Returns the number of columns (only one in our case).
     *
     * @param parent
     * @return
     */
    int columnCount( const QModelIndex& parent = QModelIndex() ) const override {
        CORE_UNUSED( parent );
        return 1;
    }

    /** Return the data corresponding to the given index : in this case a string with the item name.
     *
     * @param index
     * @param role
     * @return
     */
    QVariant data( const QModelIndex& index, int role ) const override;

    bool setData( const QModelIndex& index, const QVariant& value, int role ) override;

    /** Return the header data of the given section.
     *
     * @param section
     * @param orientation
     * @param role
     * @return
     */
    QVariant headerData( int section,
                         Qt::Orientation orientation,
                         int role = Qt::DisplayRole ) const override;

    /** Create an index for the given row, column and parent.
     *
     * @param row
     * @param column
     * @param parent
     * @return
     */
    QModelIndex
    index( int row, int column, const QModelIndex& parent = QModelIndex() ) const override;

    /** Get the parent of a given child item.
     *
     * @param child
     * @return
     */
    QModelIndex parent( const QModelIndex& child ) const override;

    /** Get the flags of the object.
     *
     * @param index
     * @return
     */
    Qt::ItemFlags flags( const QModelIndex& index ) const override;

    /// \}
    /** Call this function to update the tree model to the current content of the engine.
     *
     */
    void rebuildModel();

  signals:
    /// Emitted when the model has been rebuilt.
    void modelRebuilt();

  protected:
    virtual std::string getHeaderString() const = 0;

    /// Function to build the model from scratch. Call either at initialization or through
    /// rebuildModel().
    virtual void buildModel() = 0;

    /// Get the tree item corresponding to the given index.
    TreeItem* getItem( const QModelIndex& index ) const;

    /// Internal functions to check if an item is in the tree.
    bool findInTree( const TreeItem* item ) const;

    /// Prints elements in the model to stdout (debug only)
    void printModel() const;

    /// Set the check state of all items
    void setAllItemsChecked( bool checked = true );

    /// Set the check state of one item and all of its children
    void setItemChecked( const QModelIndex& index, bool checked = true );

  protected:
    /// Root of the tree.
    std::unique_ptr<TreeItem> m_rootItem;
};
} // namespace Gui
} // namespace Ra
