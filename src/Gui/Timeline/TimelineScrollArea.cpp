#include <Gui/Timeline/TimelineScrollArea.hpp>

#include <cmath>

#include <QScrollBar>
#include <QWheelEvent>

#include <Gui/Timeline/Timeline.hpp>
#include <Gui/Timeline/TimelineFrameSelector.hpp>

namespace Ra::Gui {

TimelineScrollArea::TimelineScrollArea( QWidget* parent ) : QScrollArea( parent ) {
    horizontalScrollBar()->setStyleSheet( "\
                    QScrollBar:horizontal {\
                        background: transparent;\
                    }\
                    \
                    QScrollBar::handle:horizontal {\
                        background: white;\
                    }\
                    \
                    QScrollBar::add-line:horizontal {\
                        border: none;\
                        background: transparent;\
                    }\
                    \
                    QScrollBar::sub-line:horizontal {\
                        border: none;\
                        background: transparent;\
                    }" );
}

int TimelineScrollArea::getNbInterval() {
    return m_nbInterval;
}

Scalar TimelineScrollArea::getMaxDuration() {
    return m_maxDuration;
}

void TimelineScrollArea::setMaxDuration( Scalar duration ) {
    m_maxDuration = duration;
}

int TimelineScrollArea::getZero() {
    return m_zero;
}

Scalar TimelineScrollArea::getPixPerSec() {
    return m_pixPerSec;
}

Scalar TimelineScrollArea::getStep() {
    return m_step;
}

void TimelineScrollArea::onDrawRuler( int width ) {
    int iStep = 0;
    while ( iStep < s_nbSteps && width * s_steps[iStep] < 50 * m_maxDuration )
        iStep++;

    if ( iStep == s_nbSteps ) { return; }

    m_step = s_steps[iStep];
    emit stepChanged( m_step );

    m_nbInterval = int( std::ceil( m_maxDuration / m_step ) ) + 2;
    m_pixPerSec  = ( Scalar( width ) / m_nbInterval ) / m_step;
    m_zero       = int( m_pixPerSec * m_step );
    widget()->setMinimumWidth( width );

    update();
}

void TimelineScrollArea::keyPressEvent( QKeyEvent* event ) {
    switch ( event->key() ) {
    case Qt::Key_Space:
        emit togglePlayPause();
        break;

    case Qt::Key_Delete:
        emit removeKeyFrame();
        break;

    case Qt::Key_I:
        if ( event->modifiers() & Qt::Modifier::SHIFT ) { emit removeKeyFrame(); }
        else {
            emit addKeyFrame();
        }
        break;

    case Qt::Key_Left:
        emit previousKeyFrame();
        break;

    case Qt::Key_Right:
        emit nextKeyFrame();
        break;

    case Qt::Key_Up:
        emit durationIncrement();
        break;

    case Qt::Key_Down:
        emit durationDecrement();
        break;

    case Qt::Key_Z:
        if ( event->modifiers() & Qt::Modifier::CTRL ) {
            if ( event->modifiers() & Qt::Modifier::SHIFT ) { emit redo(); }
            else {
                emit undo();
            }
        }
        break;

    case Qt::Key_U:
        emit undo();
        break;

    case Qt::Key_R:
        emit redo();
    }
}

void TimelineScrollArea::wheelEvent( QWheelEvent* event ) {
    int ry         = event->angleDelta().ry();
    bool ctrlDown  = event->modifiers() & Qt::Modifier::CTRL;
    bool shiftDown = event->modifiers() & Qt::Modifier::SHIFT;
    // next/previous KeyFrame
    if ( shiftDown ) {
        if ( ry > 0 ) { emit nextKeyFrame(); }
        else {
            emit previousKeyFrame();
        }
    }
    // scroll left/right bar
    else if ( ctrlDown ) {
        horizontalScrollBar()->setValue(
            int( horizontalScrollBar()->value() + ry * TIMELINE_SLIDE_SPEED ) );
    }
    // zoom in/out
    else {
        int newRulerWidth = int( widget()->minimumWidth() +
                                 ry * TIMELINE_ZOOM_SPEED * widget()->minimumWidth() / width() );
        if ( newRulerWidth <= width() - 2 ) {
            if ( widget()->minimumWidth() == width() - 2 ) { return; }
            else {
                newRulerWidth = width() - 2;
            }
        }

        double hScroll = horizontalScrollBar()->value();
#if QT_VERSION < QT_VERSION_CHECK( 5, 15, 0 )
        double x = event->x();
#else
        double x = event->position().x();
#endif
        double time = ( hScroll + x - double( m_zero ) ) / double( m_pixPerSec );

        onDrawRuler( newRulerWidth );

        // ruler values may have changed
        double a                      = time * double( m_pixPerSec ) + double( m_zero );
        double hScrollAfterProjection = a - x;
        horizontalScrollBar()->setValue( static_cast<int>( hScrollAfterProjection ) );
    }
    event->accept(); // parent is animTimeline (root) with non event catching
}

void TimelineScrollArea::mousePressEvent( QMouseEvent* event ) {
    if ( event->button() == Qt::MiddleButton ) {
        setCursor( Qt::SplitHCursor );
        m_mousePosX = event->x();
        m_sliderPos = horizontalScrollBar()->value();
    }
}

void TimelineScrollArea::mouseReleaseEvent( QMouseEvent* event ) {
    if ( event->button() == Qt::MiddleButton ) { setCursor( Qt::ArrowCursor ); }
}

void TimelineScrollArea::mouseMoveEvent( QMouseEvent* event ) {
    if ( event->buttons() & Qt::MiddleButton ) {
        horizontalScrollBar()->setValue( ( m_sliderPos + m_mousePosX - event->x() ) );
    }
}

} // namespace Ra::Gui
