#pragma once

#include <QKeyEvent>
#include <QLayout>
#include <QWidget>

// ============================================================================
namespace Qt_utils {
// ============================================================================

/// Recursively hide/show every widget of a QLayout
static inline void rec_set_visible( const QLayout& layout, bool visible ) {
    for ( int i = 0; i < layout.count(); ++i ) {
        if ( QWidget* w = layout.itemAt( i )->widget() )
            w->setVisible( visible );
        else if ( QLayout* l = layout.itemAt( i )->layout() )
            rec_set_visible( *l, visible );
    }
}

} // namespace Qt_utils
