#pragma once
#include <QScrollArea>

#include <Core/CoreMacros.hpp>

namespace Ra::Gui {

/**
 * The TimelineScrollArea class manages the ScrollArea for the Timeline's frame
 * display.
 *
 * It offers display info for its sub-widgets (size, zoom level, scroll
 * shift...).
 */
class TimelineScrollArea : public QScrollArea
{
    Q_OBJECT
  public:
    explicit TimelineScrollArea( QWidget* parent = nullptr );

    /// \returns the \b end of the Timeline's playzone.
    Scalar getMaxDuration();

    /// Set the \b end of the Timeline's playzone.
    void setMaxDuration( Scalar duration );

    /// \return the step between two scale graduations.
    Scalar getStep();

    /// \returns the number of scale graduations.
    int getNbInterval();

    /// \returns the number of pixels used to display 1 second on the scale.
    Scalar getPixPerSec();

    /// \returns the pixel corresponding to time 0.
    int getZero();

  signals:
    /// \todo use keymapping manager here
    /// Emitted when zoomed in/out or resized.
    void stepChanged( Scalar step );

    /// Emitted when [i] is pressed.
    void addKeyFrame();

    /// Emitted when [del] / [shift]+[i] is pressed.
    void removeKeyFrame();

    /// Emitted when [left_arrow] is pressed or [shift] + [wheel-up].
    void previousKeyFrame();

    /// Emitted when [right_arrow] is pressed or [shift] + [wheel-down].
    void nextKeyFrame();

    /// Emitted when [up_arrow] is pressed.
    void durationIncrement();

    /// Emitted when [down_arrow] is pressed.
    void durationDecrement();

    /// Emitted when [space] is pressed.
    void togglePlayPause();

    /// Emitted when [crtl] + [z] / [u] key is pressed.
    void undo();

    /// Emitted when [crtl] + [shift] + [z] / [r] key is pressed.
    void redo();

  public slots:
    /// Force redraw with the given width.
    void onDrawRuler( int width );

  protected:
    void keyPressEvent( QKeyEvent* event ) override;

    void wheelEvent( QWheelEvent* event ) override;

    void mousePressEvent( QMouseEvent* event ) override;
    void mouseReleaseEvent( QMouseEvent* event ) override;
    void mouseMoveEvent( QMouseEvent* event ) override;

  private:
    int m_mousePosX { 0 }; ///< x coordinate of the mouse on mouse middle click.
    int m_sliderPos { 0 }; ///< x coordinate of the slider on mouse middle click.

    Scalar m_maxDuration { 20_ra }; ///< \b end  of the Timeline's playzone.
    int m_nbInterval { 0 };         ///< Number of scale graduations.
    Scalar m_step { 1 };            ///< Step between two scale graduations.
    Scalar m_pixPerSec { 10 };      ///< Number of pixels used to display 1 second on the scale.
    int m_zero { 20 };              ///< Pixel corresponding to time 0.

    /// Number of possible steps.
    static constexpr int s_nbSteps = 13;

    /// Possible steps.
    static constexpr Scalar s_steps[s_nbSteps] = { 0.01_ra,
                                                   0.02_ra,
                                                   0.05_ra,
                                                   0.1_ra,
                                                   0.2_ra,
                                                   0.5_ra,
                                                   1.0_ra,
                                                   2.0_ra,
                                                   5.0_ra,
                                                   10.0_ra,
                                                   20.0_ra,
                                                   50.0_ra,
                                                   100.0_ra };
};

} // namespace Ra::Gui
