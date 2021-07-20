#include <Gui/Timeline/TimelineTimeScale.hpp>

#include <QPainter>

#include <Gui/Timeline/TimelineScrollArea.hpp>

namespace Ra::Gui {

TimelineTimeScale::TimelineTimeScale( QWidget* parent ) : QFrame( parent ) {}

void TimelineTimeScale::setScrollArea( TimelineScrollArea* scrollArea ) {
    m_scrollArea = scrollArea;
}

void TimelineTimeScale::paintEvent( QPaintEvent* event ) {
    if ( m_scrollArea == nullptr ) {
        QFrame::paintEvent( event );
        return;
    }

    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing );

    painter.drawText( 0, 11, "sec" );
    Scalar pixPerSec = m_scrollArea->getPixPerSec();
    Scalar step      = m_scrollArea->getStep();
    for ( int i = 1; i < m_scrollArea->getNbInterval(); i++ ) {
        int x        = int( pixPerSec * step * i );
        QString time = QString::number( double( ( i - 1 ) * step ) );
        int dec      = time.size() * 6 / 2;
        painter.drawText( x - dec, 11, time );
    }
}

} // namespace Ra::Gui
