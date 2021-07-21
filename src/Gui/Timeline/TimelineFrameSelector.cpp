#include <Gui/Timeline/TimelineFrameSelector.hpp>

#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QtGlobal>

#include <Core/Math/Math.hpp>
#include <Gui/Timeline/TimelineScrollArea.hpp>

#include "ui_Timeline.h"

namespace Ra::Gui {

TimelineFrameSelector::TimelineFrameSelector( QWidget* parent ) : QFrame( parent ) {
    m_timer = new QTimer( this );
    connect( m_timer, SIGNAL( timeout() ), this, SLOT( update() ) );
}

TimelineFrameSelector::~TimelineFrameSelector() {
    delete m_timer;
}

void TimelineFrameSelector::setTimelineUi( Ui::Timeline* ui ) {
    // register UI
    m_timelineUI = ui;

    // init values from UI
    m_start  = Scalar( m_timelineUI->m_startSpin->value() );
    m_end    = Scalar( m_timelineUI->m_endSpin->value() );
    m_cursor = Scalar( m_timelineUI->m_cursorSpin->value() );
    m_timelineUI->m_scrollArea->setMaxDuration( Scalar( m_timelineUI->m_durationSpin->value() ) );

    // create connections
#define CAST_DOUBLE static_cast<void ( QDoubleSpinBox::* )( double )>
    connect( m_timelineUI->m_startSpin,
             CAST_DOUBLE( &QDoubleSpinBox::valueChanged ),
             [this]( double start ) { onChangeStart( Scalar( start ) ); } );
    connect( m_timelineUI->m_cursorSpin,
             CAST_DOUBLE( &QDoubleSpinBox::valueChanged ),
             [this]( double cursor ) { onChangeCursor( Scalar( cursor ) ); } );
    connect( m_timelineUI->m_endSpin,
             CAST_DOUBLE( &QDoubleSpinBox::valueChanged ),
             [this]( double end ) { onChangeEnd( Scalar( end ) ); } );
    connect( m_timelineUI->m_durationSpin,
             CAST_DOUBLE( &QDoubleSpinBox::valueChanged ),
             [this]( double duration ) { onChangeDuration( Scalar( duration ) ); } );
#undef CAST_DOUBLE

    connect( m_timelineUI->m_leftSlider,
             &TimelineSlider::slide,
             this,
             &TimelineFrameSelector::onSlideLeftSlider );
    connect( m_timelineUI->m_rightSlider,
             &TimelineSlider::slide,
             this,
             &TimelineFrameSelector::onSlideRightSlider );

    connect( m_timelineUI->toolButton_start,
             &QToolButton::clicked,
             this,
             &TimelineFrameSelector::onSetCursorToStart );
    connect( m_timelineUI->toolButton_rearward,
             &QToolButton::clicked,
             this,
             &TimelineFrameSelector::onSetCursorToPreviousKeyFrame );
    connect( m_timelineUI->toolButton_forward,
             &QToolButton::clicked,
             this,
             &TimelineFrameSelector::onSetCursorToNextKeyFrame );
    connect( m_timelineUI->toolButton_end,
             &QToolButton::clicked,
             this,
             &TimelineFrameSelector::onSetCursorToEnd );

    connect(
        m_timelineUI->toolButton_keyFrame, SIGNAL( clicked() ), this, SLOT( onAddingKeyFrame() ) );
    connect( m_timelineUI->m_removeKeyFrameButton,
             SIGNAL( clicked() ),
             this,
             SLOT( onDeletingKeyFrame() ) );

    connect(
        m_timelineUI->m_scrollArea, SIGNAL( addKeyFrame() ), this, SLOT( onAddingKeyFrame() ) );
    connect( m_timelineUI->m_scrollArea,
             SIGNAL( removeKeyFrame() ),
             this,
             SLOT( onDeletingKeyFrame() ) );
    connect( m_timelineUI->m_scrollArea,
             &TimelineScrollArea::nextKeyFrame,
             this,
             &TimelineFrameSelector::onSetCursorToNextKeyFrame );
    connect( m_timelineUI->m_scrollArea,
             &TimelineScrollArea::previousKeyFrame,
             this,
             &TimelineFrameSelector::onSetCursorToPreviousKeyFrame );
    connect( m_timelineUI->m_scrollArea,
             &TimelineScrollArea::togglePlayPause,
             m_timelineUI->toolButton_playPause,
             &QToolButton::toggle );
    connect( m_timelineUI->m_scrollArea, &TimelineScrollArea::durationIncrement, [this]() {
        onChangeDuration( Scalar( m_timelineUI->m_durationSpin->value() +
                                  m_timelineUI->m_durationSpin->singleStep() ) );
    } );
    connect( m_timelineUI->m_scrollArea, &TimelineScrollArea::durationDecrement, [this]() {
        onChangeDuration( Scalar( m_timelineUI->m_durationSpin->value() -
                                  m_timelineUI->m_durationSpin->singleStep() ) );
    } );
    connect( m_timelineUI->m_scrollArea, &TimelineScrollArea::stepChanged, [this]( double step ) {
        m_timelineUI->m_startSpin->setSingleStep( 0.5 * step );
        m_timelineUI->m_endSpin->setSingleStep( 0.5 * step );
        m_timelineUI->m_cursorSpin->setSingleStep( 0.5 * step );
        m_timelineUI->m_durationSpin->setSingleStep( 0.5 * step );
    } );
}

void TimelineFrameSelector::onChangeStart( Scalar time, bool internal ) {
    Scalar newStart = std::max( std::min( time, m_end ), 0_ra );

    bool out    = std::abs( newStart - time ) > 1e-5_ra;
    bool change = std::abs( newStart - m_start ) > 1e-5_ra;

    if ( change ) {
        m_start = newStart;
        updateStartSpin();
        redrawPlayZone();

        // emit signal if time of emitter is internal changed due of limits
        if ( internal || out ) { emit startChanged( m_start ); }
    }
    else {
        if ( out ) { updateStartSpin(); }
    }
}

void TimelineFrameSelector::onChangeEnd( Scalar time, bool internal ) {
    Scalar newEnd =
        std::min( std::max( time, m_start ), m_timelineUI->m_scrollArea->getMaxDuration() );

    bool out    = std::abs( newEnd - time ) > 1e-5_ra;
    bool change = std::abs( newEnd - m_end ) > 1e-5_ra;

    // emit signal only if new value of end
    if ( change ) {
        m_end = newEnd;
        updateEndSpin();
        if ( internal || out ) { emit endChanged( m_end ); }
        update();
    }
    else {
        if ( out ) { updateEndSpin(); }
    }
}

void TimelineFrameSelector::onChangeDuration( Scalar time, bool internal ) {
    Scalar newDuration = std::max( time, 0_ra );
    Scalar oldDuration = m_timelineUI->m_scrollArea->getMaxDuration();

    bool out    = std::abs( newDuration - time ) > 1e-5_ra;
    bool change = std::abs( newDuration - oldDuration ) > 1e-5_ra;

    if ( change ) {
        oldDuration = newDuration;
        m_timelineUI->m_scrollArea->setMaxDuration( newDuration );
        m_timelineUI->m_scrollArea->onDrawRuler(
            m_timelineUI->m_scrollArea->widget()->minimumWidth() );
        updateDurationSpin();

        // emit signal if time of emitter is internal changed due of limits
        if ( internal || out ) { emit durationChanged( newDuration ); }
    }
    else {
        if ( out ) { updateDurationSpin(); }
    }

    if ( oldDuration < m_start ) onChangeStart( oldDuration );

    if ( oldDuration < m_end ) onChangeEnd( oldDuration );

    // auto update
}

void TimelineFrameSelector::onChangeCursor( Scalar time, bool internal ) {
    Scalar newCursor = std::max( 0_ra, time );

    if ( internal ) { newCursor = nearestStep( newCursor ); }

    bool out    = std::abs( newCursor - time ) > 1e-5_ra;
    bool change = std::abs( newCursor - m_cursor ) > 1e-5_ra;

    if ( change ) {
        m_cursor = newCursor;
        updateCursorSpin();
        if ( internal || out ) { emit cursorChanged( m_cursor ); }
        update();
    }
    else {
        if ( out ) { updateCursorSpin(); }
    }
}

void TimelineFrameSelector::onAddingKeyFrame( Scalar time, bool internal ) {
    // by default (time = -1.0), add keyFrame on cursor
    if ( static_cast<int>( time ) == -1 ) time = m_cursor;

    auto it = std::find_if( m_keyFrames.begin(), m_keyFrames.end(), [time]( const auto& t ) {
        return Ra::Core::Math::areApproxEqual( t, time );
    } );

    // if keyFrame not already here
    if ( it == m_keyFrames.end() ) {
        updateCursorSpin();

        m_keyFrames.push_back( time );
        std::sort( m_keyFrames.begin(), m_keyFrames.end() );

        if ( internal ) { emit keyFrameAdded( time ); }

        updateNbKeyFrameSpin();

        update();
    }
    // KeyFrame already here, change actual KeyFrame
    else {
        if ( internal ) { emit keyFrameChanged( std::distance( m_keyFrames.begin(), it ) ); }

        m_updateKeyFrameFlash = 6;
        m_keyFrameFlash       = time;

        m_timer->start( 50 );

        update();
    }
}

void TimelineFrameSelector::onDeletingKeyFrame( bool internal ) {
    auto it = std::find_if( m_keyFrames.begin(), m_keyFrames.end(), [this]( const auto& t ) {
        return Ra::Core::Math::areApproxEqual( t, m_cursor );
    } );
    if ( it == m_keyFrames.end() ) { return; }

    if ( internal ) { emit keyFrameDeleted( std::distance( m_keyFrames.begin(), it ) ); }

    m_keyFrames.erase( it );

    updateCursorSpin();

    updateNbKeyFrameSpin();

    update();
}

void TimelineFrameSelector::onMoveKeyFrame( Scalar time0, Scalar time1, bool internal ) {
    if ( Ra::Core::Math::areApproxEqual( time0, time1 ) ) { return; }

    auto it = std::find_if( m_keyFrames.begin(), m_keyFrames.end(), [this]( const auto& t ) {
        return Ra::Core::Math::areApproxEqual( t, m_cursor );
    } );
    if ( it == m_keyFrames.end() ) { return; }

    if ( internal ) { emit keyFrameMoved( std::distance( m_keyFrames.begin(), it ), time1 ); }

    m_cursor = time1;

    m_keyFrames.erase( it );
    m_keyFrames.push_back( m_cursor );
    std::sort( m_keyFrames.begin(), m_keyFrames.end() );

    updateCursorSpin();

    update();
}

void TimelineFrameSelector::onMoveKeyFrames( Scalar time, Scalar offset, bool internal ) {
    if ( Ra::Core::Math::areApproxEqual( offset, 0_ra ) ) { return; }

    auto it = std::find_if( m_keyFrames.begin(), m_keyFrames.end(), [time]( const auto& t ) {
        return Ra::Core::Math::areApproxEqual( t, time );
    } );
    if ( it == m_keyFrames.end() ) { return; }

    // emit keyFramesMoved before emitting cursorChanged to render the truly Frame on cursor
    if ( internal ) { emit keyFramesMoved( std::distance( m_keyFrames.begin(), it ), offset ); }

    std::vector<Scalar> clone;
    std::transform( m_keyFrames.begin(),
                    m_keyFrames.end(),
                    std::back_inserter( clone ),
                    [time, offset]( Scalar p ) { return ( p < time ? p : p + offset ); } );
    std::exchange( m_keyFrames, clone );

    Scalar left = ( offset > 0 ) ? ( time ) : ( time + offset );

    if ( m_start >= left ) {
        m_start = std::max(
            std::min( m_start + offset, m_timelineUI->m_scrollArea->getMaxDuration() ), 0_ra );
        updateStartSpin();
        if ( internal ) { emit startChanged( m_start ); }
    }

    Scalar right = *m_keyFrames.rbegin();
    if ( right > m_timelineUI->m_scrollArea->getMaxDuration() ) {
        m_timelineUI->m_scrollArea->setMaxDuration( right );
        updateDurationSpin();
        if ( internal ) { emit durationChanged( right ); }
    }

    if ( m_end >= left ) {
        m_end = m_end + offset;
        updateEndSpin();
        if ( internal ) { emit endChanged( m_end ); }
    }

    if ( m_cursor >= left ) {
        m_cursor = std::max( m_cursor + offset, 0_ra );
        updateCursorSpin();

        if ( internal ) { emit cursorChanged( m_cursor ); }
    }

    update();
}

void TimelineFrameSelector::onClearKeyFrames() {
    m_keyFrames.clear();
    updateNbKeyFrameSpin();

    updateCursorSpin();
    update();
}

// -------------------------- INTERNAL SLOTS ----------------------------------
void TimelineFrameSelector::onSlideLeftSlider( int deltaX ) {
    { m_timelineUI->m_leftSlider->setStyleSheet( "background-color: green" ); }

    Scalar pixPerSec = m_timelineUI->m_scrollArea->getPixPerSec();
    Scalar newStart  = m_start + deltaX / pixPerSec;

    onChangeStart( newStart );
}

void TimelineFrameSelector::onSlideRightSlider( int deltaX ) {
    { m_timelineUI->m_rightSlider->setStyleSheet( "background-color: red" ); }
    Scalar pixPerSec = m_timelineUI->m_scrollArea->getPixPerSec();
    Scalar newEnd    = m_end + deltaX / pixPerSec;

    onChangeEnd( newEnd );
}

void TimelineFrameSelector::onSetCursorToStart() {
    onChangeCursor( m_start );
}

void TimelineFrameSelector::onSetCursorToEnd() {
    onChangeCursor( m_end );
}

void TimelineFrameSelector::onSetCursorToPreviousKeyFrame() {
    auto it = m_keyFrames.rbegin();
    while ( it != m_keyFrames.rend() && *it >= m_cursor )
        it++;

    if ( it != m_keyFrames.rend() ) { onChangeCursor( *it ); }
}

void TimelineFrameSelector::onSetCursorToNextKeyFrame() {
    auto it = m_keyFrames.begin();
    while ( it != m_keyFrames.end() && *it <= m_cursor )
        it++;

    if ( it != m_keyFrames.end() ) { onChangeCursor( *it ); }
}

void TimelineFrameSelector::paintEvent( QPaintEvent* ) {

    QPainter painter( this );
    int h = height();
    int w = width();

    redrawPlayZone();

    // DRAW FRAME SCALE
    painter.setPen( QPen( Qt::lightGray ) );
    Scalar frameDuration = 1_ra / TIMELINE_FPS;
    int hUp              = h / 3;
    Scalar nbFrame       = m_timelineUI->m_scrollArea->getMaxDuration() / frameDuration;
    Scalar pixPerSec     = m_timelineUI->m_scrollArea->getPixPerSec();
    Scalar step          = m_timelineUI->m_scrollArea->getStep();
    int zero             = m_timelineUI->m_scrollArea->getZero();
    for ( int i = 0; i < nbFrame; i++ ) {
        int x = int( i * frameDuration * pixPerSec + zero );
        painter.drawLine( x, 0, x, hUp );
    }

    // DRAW CURSOR
    painter.setPen( QPen( QColor( 0, 0, 255, 255 ), 3 ) );
    int xCursor = int( zero + m_cursor * pixPerSec );
    painter.drawLine( xCursor, 0, xCursor, h );

    // DRAW KEYFRAMES
    painter.setPen( QPen( QColor( 255, 255, 0, 255 ), 3 ) );
    int hTemp = h / 3 + 2;
    for ( Scalar keyFrame : m_keyFrames ) {
        int xKeyFrame = int( zero + keyFrame * pixPerSec );
        painter.drawLine( xKeyFrame, hTemp, xKeyFrame, h );
    }

    // DRAW TIME SCALE
    int hDown = 2 * h / 3;
    painter.setPen( Qt::black );
    for ( int i = 1; i < m_timelineUI->m_scrollArea->getNbInterval(); i++ ) {
        int x = int( i * step * pixPerSec );
        painter.drawLine( x, hDown, x, h );
    }
    int hDown2 = 3 * h / 4;
    painter.setPen( Qt::darkGray );
    for ( int i = 1; i < m_timelineUI->m_scrollArea->getNbInterval() - 1; i++ ) {
        int middle = int( ( i + 0.5_ra ) * step * pixPerSec );
        painter.drawLine( middle, hDown2, middle, h );
    }

    if ( m_updateKeyFrameFlash > 0 ) {

        if ( m_updateKeyFrameFlash % 2 == 0 ) {
            painter.setPen( QPen( QColor( 0, 0, 255, 255 ), 3 ) );
            int xKeyFrame = static_cast<int>( zero + m_keyFrameFlash * pixPerSec );
            painter.drawLine( xKeyFrame, hTemp, xKeyFrame, h );
        }

        if ( --m_updateKeyFrameFlash == 0 ) m_timer->stop();
    }

    // DRAW MIDDLE RULER SEPARATOR
    int gap = 5;
    painter.setPen( Qt::white );
    painter.drawLine( 0, h / 2 + gap + 2, w, h / 2 + gap + 2 );
    painter.drawLine( 0, h / 2 + gap + 1, w, h / 2 + gap + 1 );

    painter.setPen( Qt::darkGray );
    painter.drawLine( 0, h / 3, w, h / 3 );
    painter.drawLine( 0, h / 2 + gap, w, h / 2 + gap );
}

void TimelineFrameSelector::mousePressEvent( QMouseEvent* event ) {
    bool shiftDown = event->modifiers() & Qt::Modifier::SHIFT;
    bool ctrlDown  = event->modifiers() & Qt::Modifier::CTRL;
    // ---------------------- LEFT CLICK --------------------------------------
    if ( event->button() == Qt::LeftButton ) {
        Scalar newCursor = std::max( Scalar( event->x() - m_timelineUI->m_scrollArea->getZero() ) /
                                         m_timelineUI->m_scrollArea->getPixPerSec(),
                                     0_ra );
        // move cursor without render
        if ( ctrlDown ) { onChangeCursor( newCursor, false ); }
        // delete keyFrames between cursor and newCursor
        else if ( shiftDown ) {
            deleteZone( m_cursor, newCursor );
        }
        // move cursor and update renderer
        else {
            onChangeCursor( newCursor );
            m_mouseLeftClicked = true;
        }
    }
    // ------------------ RIGHT CLICK -------------------------------------
    else if ( event->button() == Qt::RightButton ) {
        Scalar newFrame = std::max( Scalar( event->x() - m_timelineUI->m_scrollArea->getZero() ) /
                                        m_timelineUI->m_scrollArea->getPixPerSec(),
                                    0_ra );
        auto it = std::find_if( m_keyFrames.begin(), m_keyFrames.end(), [this]( const auto& t ) {
            return Ra::Core::Math::areApproxEqual( t, m_cursor );
        } );
        // if already on keyFrame, move current KeyFrame
        // ------------------- CURSOR ON KEYFRAME -----------------------
        if ( it != m_keyFrames.end() ) {
            Scalar nearest = nearestStep( newFrame );
            // -------------- SINGLE MOVE -------------------------------------
            if ( shiftDown ) {
                // if no keyFrame under mouse, move KeyFrame to newFrame
                auto it2 = std::find_if(
                    m_keyFrames.begin(), m_keyFrames.end(), [nearest]( const auto& t ) {
                        return Ra::Core::Math::areApproxEqual( t, nearest );
                    } );
                if ( it2 == m_keyFrames.end() && ( std::abs( m_cursor - nearest ) > 1e-5_ra ) ) {
                    onMoveKeyFrame( m_cursor, nearest );
                }
            }
            // ---------- MULTIPLE MOVE -----------------------------------
            else {
                // if not before preceding KeyFrame, remove or insert time
                auto itLeft = it;
                --itLeft;
                Scalar left = ( it == m_keyFrames.begin() ) ? ( 0.0 ) : ( *itLeft );
                if ( nearest > left ) { onMoveKeyFrames( m_cursor, nearest - m_cursor ); }
            }
        }
        // ---------------- CURSOR NOT ON KEYFRAME --------------------------
        else {
            // if shiftdown, slide first right KeyFrame to the left
            // --------------- MOVE RIGHT KEYFRAME TO THE LEFT -----------------
            if ( shiftDown ) {
                auto itRight = std::lower_bound( m_keyFrames.begin(), m_keyFrames.end(), newFrame );
                // if KeyFrames on the right, remove or insert time
                if ( itRight != m_keyFrames.end() ) {
                    onMoveKeyFrames( *itRight, newFrame - *itRight );
                }
            }
            // if not shiftdown, slide first left KeyFrame to the right
            // ---------------- MOVE LEFT KEYFRAME TO THE RIGHT -----------
            else {
                auto itLeft =
                    --std::lower_bound( m_keyFrames.begin(), m_keyFrames.end(), newFrame );
                // if KeyFrames on the left, remove or insert time
                if ( itLeft != m_keyFrames.end() ) {
                    onMoveKeyFrames( *itLeft, newFrame - *itLeft );
                }
            }
        }
    }
    // no catch mouse event
    else {
        event->ignore();
        return;
    }
    event->accept();
}

void TimelineFrameSelector::mouseMoveEvent( QMouseEvent* event ) {
    if ( m_mouseLeftClicked ) {
        Scalar newCursor = std::max( Scalar( event->x() - m_timelineUI->m_scrollArea->getZero() ) /
                                         m_timelineUI->m_scrollArea->getPixPerSec(),
                                     0_ra );

        onChangeCursor( newCursor );
    }
    else {
        event->ignore();
    }
}

void TimelineFrameSelector::mouseReleaseEvent( QMouseEvent* event ) {
    if ( event->button() == Qt::LeftButton ) {
        m_mouseLeftClicked = false;
        event->accept();
    }
    else {
        event->ignore();
    }
}

void TimelineFrameSelector::updateCursorSpin() {
    auto it = std::find_if( m_keyFrames.begin(), m_keyFrames.end(), [this]( const auto& t ) {
        return Ra::Core::Math::areApproxEqual( t, m_cursor );
    } );
    if ( it != m_keyFrames.end() ) {
        m_timelineUI->m_cursorSpin->setStyleSheet( "background-color: yellow" );
        m_timelineUI->m_removeKeyFrameButton->setEnabled( true );
    }
    else {
        m_timelineUI->m_cursorSpin->setStyleSheet( "background-color: #5555ff" );
        m_timelineUI->m_removeKeyFrameButton->setEnabled( false );
    }
    m_timelineUI->m_cursorSpin->setValue( double( m_cursor ) );
}

void TimelineFrameSelector::updateStartSpin() {
    m_timelineUI->m_startSpin->setValue( double( m_start ) );
    updateDurationSpin();
}

void TimelineFrameSelector::updateEndSpin() {
    m_timelineUI->m_endSpin->setValue( double( m_end ) );
    updateDurationSpin();
}

void TimelineFrameSelector::updateDurationSpin() {
    m_timelineUI->m_durationSpin->setValue(
        double( m_timelineUI->m_scrollArea->getMaxDuration() ) );
}

void TimelineFrameSelector::updateNbKeyFrameSpin() {
    m_timelineUI->m_nbKeyFramesSpin->setText( " " + QString::number( m_keyFrames.size() ) + " " );
}

Scalar TimelineFrameSelector::nearestStep( Scalar time ) const {
    Scalar deltaT =
        TIMELINE_AUTO_SUGGEST_CURSOR_RADIUS / m_timelineUI->m_scrollArea->getPixPerSec();

    Scalar minDist = Scalar( m_timelineUI->m_durationSpin->maximum() );
    Scalar dist;

    Scalar newCursor = time;

    for ( Scalar keyFrame : m_keyFrames ) {
        dist = qAbs( keyFrame - time );
        if ( dist < deltaT && dist < minDist ) {
            minDist   = dist;
            newCursor = keyFrame;
        }
        if ( time < keyFrame ) break;
    }

    Scalar step = m_timelineUI->m_scrollArea->getStep();
    for ( int i = 0; i < m_timelineUI->m_scrollArea->getNbInterval() - 1; ++i ) {
        Scalar pos = i * step;
        dist       = std::abs( pos - time );
        if ( dist < deltaT && dist < minDist ) {
            minDist   = dist;
            newCursor = pos;
        }

        pos  = i * step + 0.5_ra * step;
        dist = std::abs( pos - time );
        if ( dist < deltaT && dist < minDist ) {
            minDist   = dist;
            newCursor = pos;
        }

        if ( time < pos ) break;
    }

    return newCursor;
}

void TimelineFrameSelector::deleteZone( Scalar time, Scalar time2 ) {
    Scalar left  = std::min( time, time2 );
    Scalar right = std::max( time, time2 );

    Scalar dist = right - left;

    // remove KeyFrames
    auto it    = m_keyFrames.begin();
    bool first = true;
    while ( it != m_keyFrames.end() ) {
        Scalar keyFrame = *it;

        if ( keyFrame >= left ) {
            it = m_keyFrames.erase( it );

            if ( keyFrame > right ) {
                if ( first ) {
                    emit keyFramesMoved( std::distance( m_keyFrames.begin(), it ), -dist );
                    first = false;
                }
                it = m_keyFrames.insert( it, keyFrame - dist );
                ++it;
            }
            else {
                emit keyFrameDeleted( std::distance( m_keyFrames.begin(), it ) );
            }
        }
        else {
            ++it;
        }
    }
    updateNbKeyFrameSpin();

    // update playzone
    Scalar newStart = std::max( std::max( std::min( m_start, left ), m_start - dist ), 0_ra );
    if ( std::abs( newStart - m_start ) > 1e-5_ra ) {
        m_start = newStart;
        updateStartSpin();

        emit startChanged( m_start );
    }

    Scalar newEnd = std::max( std::max( std::min( m_end, left ), m_end - dist ), 0_ra );
    if ( std::abs( newEnd - m_end ) > 1e-5_ra ) {
        m_end = newEnd;
        updateEndSpin();

        emit endChanged( m_end );
    }

    emit cursorChanged( m_cursor );

    update();
}

void TimelineFrameSelector::redrawPlayZone() {
    m_timelineUI->m_leftSpacer->setMinimumWidth(
        int( m_timelineUI->m_scrollArea->getZero() +
             m_start * m_timelineUI->m_scrollArea->getPixPerSec() -
             m_timelineUI->m_leftSlider->width() ) );
    m_timelineUI->m_playZone->setMinimumWidth(
        int( ( m_end - m_start ) * m_timelineUI->m_scrollArea->getPixPerSec() ) );
}

} // namespace Ra::Gui
