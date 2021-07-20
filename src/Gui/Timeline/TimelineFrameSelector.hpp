#pragma once
#include <Gui/Timeline/Configurations.hpp>

#include <QDoubleSpinBox>
#include <QFrame>
#include <QLabel>
#include <QSpacerItem>
#include <QToolButton>
#include <set>

#include <Core/CoreMacros.hpp>

namespace Ui {
class Timeline;
}

namespace Ra::Gui {

/**
 * The TimelineFrameSelector manages the display of keyframes in the Timeline.
 * It also manages the update of the Timeline UI.
 */
class TimelineFrameSelector : public QFrame
{
    Q_OBJECT
  public:
    explicit TimelineFrameSelector( QWidget* parent = nullptr );
    ~TimelineFrameSelector() override;

    /// Registers the Timeline's UI for update.
    void setTimelineUi( Ui::Timeline* ui );

    /**
     * Sets the start point of the Timeline's playzone to the given time.
     * @param time the new start point.
     * @param internal whether the call has been made internally by the Timeline,
     *        e.g. through the UI; or externally through Timeline::onChangeStart().
     * \note If \p internal is set to true, the signal startChanged() will be
     *       emitted.
     */
    void onChangeStart( Scalar time, bool internal = true );

    /**
     * Sets the end point of the Timeline's playzone to the given time.
     * @param time the new end point.
     * @param internal whether the call has been made internally by the Timeline,
     *        e.g. through the UI; or externally through Timeline::onChangeEnd().
     * \note If \p internal is set to true, the signal endChanged() will be
     *       emitted.
     */
    void onChangeEnd( Scalar time, bool internal = true );

    /**
     * Set the duration of the Timeline's playzone.
     * \param time the new duration.
     * \param internal whether the call has been made internally by the Timeline,
     *        e.g. through the UI; or externally through Timeline::onChangeDuration().
     * \note If \p internal is set to true, the signal durationChanged() will be
     *       emitted.
     */
    void onChangeDuration( Scalar time, bool internal = true );

    /**
     * Set the current time of the Timeline's playzone to the given time.
     * \param time the new current time.
     * \param internal whether the call has been made internally by the Timeline,
     *        e.g. through the UI; or externally through Timeline::onChangeEnd().
     * \note If \p internal is set to true, the signal cursorChanged() will be
     *       emitted.
     */
    void onChangeCursor( Scalar time, bool internal = true );

  public slots:
    /**
     * Adds a KeyFrame at the given time.
     * \param time the time to add a KeyFrame at.
     * \param internal whether the call has been made internally by the Timeline,
     *        e.g. through the UI; or externally, e.g. by setting Keyrames in the
     *        Timeline.
     * \note If \p time is set to its default value, then it is considered as
     *       the current time.
     * \note If there already is a KeyFrame at time \p time, then it will be
     *       replaced, and the KeyFrame display will Flash.
     * \note If \p internal is set to true:
     *       - the signal keyFrameAdded() will be emitted if a KeyFrame has been
     *         added;
     *       - the signal keyFrameChanged() will be emitted if a KeyFrame has been
     *         changed.
     */
    void onAddingKeyFrame( Scalar time = -1.0, bool internal = true );

    /**
     * Removes the KeyFrame at the current time, if any.
     * \param internal whether the call has been made internally by the Timeline,
     *        e.g. through the UI; or externally.
     * \note If \p internal is set to true, the signal keyFrameDeleted() will be
     *       emitted if a KeyFrame has been effectively removed.
     */
    void onDeletingKeyFrame( bool internal = true );

    /**
     * Removes the KeyFrame at time \p time0 and adds a new one at time \p time1.
     * \param time0 the time to remove the KeyFrame at.
     * \param time1 the time to add the KeyFrame at.
     * \param internal whether the call has been made internally by the Timeline,
     *        e.g. through the UI; or externally.
     * \note If \p time0 == \p time1, does nothing.
     * \note If \p internal is set to true, the signal keyFrameMoved() will be
     *       emitted if the KeyFrame has been effectively moved.
     */
    void onMoveKeyFrame( Scalar time0, Scalar time1, bool internal = true );

    /**
     * Offsets the KeyFrames after time \p time of an offsets \p offset.
     * \param time the time from which to offset KeyFrames.
     * \param offset the offset to offset KeyFrames of.
     * \param internal whether the call has been made internally by the Timeline,
     *        e.g. through the UI; or externally    .
     * \note If \p offset == 0, or \p time has no KeyFrame, does nothing.
     * \note If \p internal is set to true, the signal keyFramesMoved() will be
     *       emitted if the KeyFrames have been effectively moved.
     * \note If \p offset is negative and offsetting the KeyFrame at \p time covers
     *       the start point of the Timeline's playzone, this one is offset too.
     *       In this case, if \p internal is set to true, the signal
     *       startChanged() will be emitted.
     * \note If offsetting the last KeyFrame overtakes the duration, this one
     *       comes along.
     *       In this case, if \p internal is set to true, the signal
     *       durationChanged() will be emitted.
     * \note If \p time comes before the end point of the Timeline's playzone,
     *       this one will be offset too.
     *       In this case, if \p internal is set to true, the signal
     *       endChanged() will be emitted.
     * \note If \p offset is negative and offsetting the KeyFrame at \p time covers
     *       the current time, this one will be offset too.
     *       In this case, if \p internal is set to true, the signal
     *       cursorChanged() will be emitted.
     */
    void onMoveKeyFrames( Scalar time, Scalar offset, bool internal = true );

    /**
     * Removes all KeyFrames.
     */
    void onClearKeyFrames();

  signals:
    /**
     * Emitted when the playzone start has been changed though
     *  - the user changing the start point of the Timeline's playzone,
     *  - the user offsetting a set of KeyFrames,
     *  - the user deleting an entire timezone containing the playzone start.
     */
    void startChanged( Scalar time );

    /**
     * Emitted when the playzone end has been changed though
     *  - the user changing the end point of the Timeline's playzone,
     *  - the user offsetting a set of KeyFrames,
     *  - the user deleting an entire timezone containing the playzone end.
     */
    void endChanged( Scalar time );

    /**
     * Emitted when the playzone duration has been changed though
     * an internal call to onChangeDuration().
     */
    void durationChanged( Scalar time );

    /**
     * Emitted when the playzone current time has been changed though
     *  - the user changing the current time of the Timeline's playzone,
     *  - the user offsetting a set of KeyFrames,
     *  - the user deleting an entire timezone.
     */
    void cursorChanged( Scalar time );

    /**
     * Emitted when a KeyFrame has been added through
     * an internal call to onAddingKeyFrame().
     */
    void keyFrameAdded( Scalar time );

    /**
     * Emitted when a KeyFrame has been deleted through
     *  - the user deleting a single KeyFrame.
     *  - the user deleting an entire timezone containing a KeyFrame.
     * \note Emitted for each suppressed KeyFrame.
     */
    void keyFrameDeleted( size_t i );

    /**
     * Emitted when a KeyFrame has been added through
     * an internal call to onAddingKeyFrame().
     */
    void keyFrameChanged( size_t i );

    /**
     * Emitted when a KeyFrame has been added through
     * an internal call to onMoveKeyFrame().
     */
    void keyFrameMoved( size_t i, Scalar time1 );

    /**
     * Emitted when a KeyFrame has been added through
     *  - the user offsetting a set of keyframes.
     *  - the user deleting an entire timezone preceding KeyFrames.
     */
    void keyFramesMoved( size_t first, Scalar offset );

  private slots:
    /**
     * Slides the slider for the start point of the Timeline's playzone, and
     * resizes the playzone UI.
     * Also change its color for while it is sliding.
     * \note Internally calls onChangeStart().
     */
    void onSlideLeftSlider( int deltaX );

    /**
     * Slides the slider for the end point of the Timeline's playzone, and
     * resizes the playzone UI.
     * Also change its color for while it is sliding.
     * \note Internally calls onChangeEnd().
     */
    void onSlideRightSlider( int deltaX );

    /**
     * Set the cursor to the start point of the Timeline's playzone.
     * \note Internally calls onChangeCursor().
     */
    void onSetCursorToStart();

    /**
     * Set the cursor to the end point of the Timeline's playzone.
     * \note Internally calls onChangeCursor().
     */
    void onSetCursorToEnd();

    /**
     * Set the cursor to the previous KeyFrame, if any.
     * \note Internally calls onChangeCursor().
     */
    void onSetCursorToPreviousKeyFrame();

    /**
     * Set the cursor to the next KeyFrame, if any.
     * \note Internally calls onChangeCursor().
     */
    void onSetCursorToNextKeyFrame();

  protected:
    virtual void paintEvent( QPaintEvent* event ) override;

    void mousePressEvent( QMouseEvent* event ) override;
    void mouseMoveEvent( QMouseEvent* event ) override;
    void mouseReleaseEvent( QMouseEvent* event ) override;

  private:
    /// Updates the "start" SpinBox.
    void updateStartSpin();

    /// Updates the "end" SpinBox.
    void updateEndSpin();

    /// Updates the "cursor" SpinBox.
    void updateCursorSpin();

    /// Updates the "duration" SpinBox.
    void updateDurationSpin();

    /// Updates the "number of KeyFrames" SpinBox.
    void updateNbKeyFrameSpin();

    /**
     * Tries to stick the cursor from \p time to the nearest KeyFrame,
     * depending on the zoom level.
     * \returns the nearest KeyFrame if could stick, time otherwise.
     */
    Scalar nearestStep( Scalar time ) const;

    /**
     * Squashes the time zone between \p time and \p time2.
     * KeyFrames in the time zone are removed and the playzone is updated
     * accordingly.
     */
    void deleteZone( Scalar time, Scalar time2 );

    /**
     * Updates the playzone UI dimensions w.r.t. the current zoom level.
     */
    void redrawPlayZone();

  private:
    /// The start time of the Timeline's playzone.
    Scalar m_start;

    /// The end time of the Timeline's playzone.
    Scalar m_end;

    /// The current time of the Timeline's playzone.
    Scalar m_cursor;

    /// The list of KeyFrames to display.
    std::vector<Scalar> m_keyFrames;

    /// Whether the user is sliding the current time.
    bool m_mouseLeftClicked { false };

    /// The KeyFrame to Flash.
    Scalar m_keyFrameFlash;

    /// The number of KeyFrame Flash we have to do.
    int m_updateKeyFrameFlash { 0 };

    /// The QTimer for KeyFrame Flash.
    QTimer* m_timer { nullptr };

    /// The Timeline UI.
    Ui::Timeline* m_timelineUI { nullptr };
};

} // namespace Ra::Gui
