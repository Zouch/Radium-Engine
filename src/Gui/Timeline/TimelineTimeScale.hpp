#pragma once
#include <QFrame>
#include <QObject>

namespace Ra::Gui {
class TimelineScrollArea;
}

namespace Ra::Gui {

/**
 * The TimelineTimeScale class displays the scale, along the x-axis,
 * of the TimelineScrollArea it belongs to.
 */
class TimelineTimeScale : public QFrame
{
    Q_OBJECT
  public:
    explicit TimelineTimeScale( QWidget* parent = nullptr );

    /// Set the TimelineScrollArea for which to display the scale.
    void setScrollArea( TimelineScrollArea* scrollArea );

  protected:
    virtual void paintEvent( QPaintEvent* event ) override;

  private:
    /// The TimelineScrollArea.
    TimelineScrollArea* m_scrollArea { nullptr };
};

} // namespace Ra::Gui
