#include <Gui/Timeline/TimelineSlider.hpp>

#include <QMouseEvent>

namespace Ra::Gui {

TimelineSlider::TimelineSlider( QWidget* parent ) : QLabel( parent ) {}

void TimelineSlider::mousePressEvent( QMouseEvent* event ) {
    if ( event->button() == Qt::LeftButton ) {
        m_clicked = true;
        emit slide( event->x() );
    }
}

void TimelineSlider::mouseReleaseEvent( QMouseEvent* event ) {
    if ( event->button() == Qt::LeftButton ) {
        setStyleSheet( "background-color: gray" );

        m_clicked = false;
    }
}

void TimelineSlider::mouseMoveEvent( QMouseEvent* event ) {
    if ( m_clicked ) { emit slide( event->x() ); }
}

} // namespace Ra::Gui
