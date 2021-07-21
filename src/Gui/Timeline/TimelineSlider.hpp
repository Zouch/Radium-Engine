#pragma once
#include <QLabel>

namespace Ra::Gui {

/**
 * A TimelineSlider is a widget which transmits mouse move events along the
 * x-axis.
 * It is used to display blocks movable along the x-axis, e.g. timezone's
 * \b start and \b end time points.
 */
class TimelineSlider : public QLabel
{
    Q_OBJECT
  public:
    explicit TimelineSlider( QWidget* parent = nullptr );

  signals:
    /**
     * Emitted when the user clicks on the widget and drags it.
     * \param deltaX The x position of the mouse cursor relative to the widget.
     */
    void slide( int deltaX );

  protected:
    void mousePressEvent( QMouseEvent* event ) override;
    void mouseReleaseEvent( QMouseEvent* event ) override;
    void mouseMoveEvent( QMouseEvent* event ) override;

  private:
    /// Whether the widget being dragged or not.
    bool m_clicked { false };
};

} // namespace Ra::Gui
