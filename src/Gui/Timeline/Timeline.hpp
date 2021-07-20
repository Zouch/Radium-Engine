#pragma once
/*!
 * \brief Custom Qt widget for animation (timeline)
 * \author Gauthier Bouyjou (email : gauthierbouyjou@aol.com)
 * \date april 2019
 *
 * For question send me mail or add issue to initial
 * github repo : https://github.com/hiergaut/AnimTimeline.git
 * current commit id : aaa363ee8ec0247e9299a8949e5bd334b10dc413
 */

#include <Gui/RaGui.hpp>

#include <Core/Animation/KeyFramedValue.hpp>
#include <Core/Animation/KeyFramedValueController.hpp>
#include <Engine/Scene/ItemEntry.hpp>

#include <QDialog>
#include <QObject>

namespace Ui {
class Timeline;
}

namespace Ra::Gui {

/*!
 * \brief The Timeline class provides display and management of time, as well as
 *        keyframes.
 *
 * The Timeline is to be linked to the TimeSystem in the RadiumEngine through
 * the application's MainWindow.
 *
 * Doing so, time manipulations from the user can be done through the timeline,
 * such as play, pause and reset animations or simulations, but also going to
 * a given point in time.
 *
 * Regarding KeyFramedValues, the Timeline provides their edition / manipulation.
 * To do so, KeyFramedValues must be registered into the Timeline, which can be
 * done using the registerKeyFramedValue methods (see the related Documentation page ).
 */
class RA_GUI_API Timeline : public QDialog
{
    Q_OBJECT

  public:
    explicit Timeline( QWidget* parent = nullptr );
    ~Timeline();

  public:
    /**
     * Updates the keyframes display according to \p ent.
     */
    void selectionChanged( const Ra::Engine::Scene::ItemEntry& ent );

    /**
     * Registers \p value as a new KeyFramedValue for \p ent with the given name.
     */
    void registerKeyFramedValue(
        Ra::Engine::Scene::Entity* ent,
        const Ra::Core::Animation::KeyFramedValueController& keyFramedValueController );

    /**
     * Unregisters the KeyFramedValue for \p ent with the given name.
     */
    void unregisterKeyFramedValue( Ra::Engine::Scene::Entity* ent, const std::string& name );

    /**
     * Registers \p value as a new KeyFramedValue for \p comp with the given name.
     */
    void registerKeyFramedValue(
        Ra::Engine::Scene::Component* comp,
        const Ra::Core::Animation::KeyFramedValueController& keyFramedValueController );

    /**
     * Unregisters the KeyFramedValue for \p comp with the given name.
     */
    void unregisterKeyFramedValue( Ra::Engine::Scene::Component* comp, const std::string& name );

    /**
     * Registers \p value as a new KeyFramedValue for \p roIdx with the given name.
     */
    void registerKeyFramedValue(
        Ra::Core::Utils::Index roIdx,
        const Ra::Core::Animation::KeyFramedValueController& keyFramedValueController );

    /**
     * Unregisters the KeyFramedValue for \p roIdx with the given name.
     */
    void unregisterKeyFramedValue( Ra::Core::Utils::Index roIdx, const std::string& name );

    /**
     * @returns the current time.
     */
    Scalar getTime() const;

    /** Detach from engine signals
     *
     */
    void detachFromEngine();

  signals:
    /*!
     * \brief startChanged is emitted when user move left slider of playzone or set new value in
     * start spin (green) \param time is the new start time for playzone.
     */
    void startChanged( Scalar time );

    /*!
     * \brief endChanged is emitted when user move right slider of playzone or set new value in end
     * spin (red) \param time is the new end time for playzone.
     */
    void endChanged( Scalar time );

    /*!
     * \brief cursorChanged is emitted when user move cursor.
     * \param time is the new time of cursor to render in engine.
     */
    void cursorChanged( Scalar time );

    /*!
     * \brief durationChanged is emitted when user change duration time in top right spin.
     * \param time is the new time for animation.
     */
    void durationChanged( Scalar time );

    /*!
     * \brief playClicked is emitted when user click on play button.
     */
    void playClicked( bool play );

    /*!
     * \brief setPingPong is emitted when user click on PingPong button.
     */
    void setPingPong( bool );

    /*!
     * \brief keyFrameAdded is emitted when user add new keyFrame.
     * \param time is the time of new keyFrame, if a keyFrame is already here so signal
     * keyFrameChanged is called.
     */
    void keyFrameAdded( Scalar time );

    /*!
     * \brief keyFrameDeleted is emitted when user remove keyFrame.
     * \param i is the ith keyFrame to remove (chronological order).
     */
    void keyFrameDeleted( size_t i );

    /*!
     * \brief keyFrameChanged is emitted when user insert keyFrame in a known keyFrame position.
     * \param i is the ith keyFrame to change.
     */
    void keyFrameChanged( size_t i );

    /*!
     * \brief keyFrameMoved is emitted when user move the keyFrame on cursor to a new position (on
     * mouse) \param i is the index of the keyFrame. \param time1 is the new time of the
     * keyFrame.
     */
    void keyFrameMoved( size_t i, Scalar time1 );

    /*!
     * \brief keyFramesMoved is emitted when user move keyFrames.
     * \param first is the index of the first keyFrame to move.
     * \param offset is sliding distance for moving.
     */
    void keyFramesMoved( size_t first, Scalar offset );

  public slots:
    /*!
     * \brief onChangeStart change start in timeline.
     * \param time is the new start.
     */
    void onChangeStart( Scalar time );

    /*!
     * \brief onChangeEnd change end in timeline.
     * \param time is the new end.
     */
    void onChangeEnd( Scalar time );

    /*!
     * \brief onChangeDuration change duration in timeline.
     * \param time is the new duration.
     */
    void onChangeDuration( Scalar time );

    /*!
     * \brief onChangeCursor change cursor in timeline.
     * \param time is the new cursor.
     */
    void onChangeCursor( Scalar time );

    /*!
     * \brief onSetPlay set play mode in timeline.
     */
    void onSetPlay( bool play ); // use it if external play button

  protected:
    void resizeEvent( QResizeEvent* ev ) override;

  private slots:
    /**
     * Toggles time flow ping-pong, i.e. going back and forth.
     */
    void on_pingPong_toggled( bool checked );

    /**
     * Calls update on all registered KeyFrames.
     */
    void updateKeyFrames( Scalar time );

    /*!
     * \brief onClearKeyFrames remove all KeyFrames in timeline.
     */
    void onClearKeyFrames();

    /*!
     * \brief onAddingKeyFrame add the current KeyFrame to the timeline.
     */
    void onAddingKeyFrame( Scalar time );

    /*!
     * \brief onRemovingKeyFrame removes the current KeyFrame from the timeline.
     */
    void onRemovingKeyFrame( size_t i );

    /*!
     * \brief onChangingKeyFrame updates the current KeyFrame of the timeline.
     */
    void onChangingKeyFrame( size_t i );

    /*!
     * \brief onMovingKeyFrame moves the KeyFrame at time0 to time1.
     */
    void onMovingKeyFrame( size_t i, Scalar time1 );

    /*!
     * \brief onMovingKeyFrames offsets all Keyframes from the one at time by offset.
     */
    void onMovingKeyFrames( size_t first, Scalar offset );

    /*!
     * \brief on_comboBox_attribute_currentIndexChanged set the current keyframe
     */
    void on_comboBox_attribute_currentIndexChanged( const QString& arg1 );

    /*!
     * \brief on_pushButton_editAttribute_clicked make the keyframe editor pop up
     */
    void on_pushButton_editAttribute_clicked();

    /*!
     * \brief on_toolButton_help_clicked make the help dialog pop up
     */
    void on_toolButton_help_clicked();

  private:
    /// The Timeline UI.
    Ui::Timeline* ui;

    /// The per-Entity keyframes.
    std::map<Ra::Engine::Scene::Entity*, std::vector<Ra::Core::Animation::KeyFramedValueController>>
        m_entityKeyFrames;

    /// The per-Component keyframes.
    std::map<Ra::Engine::Scene::Component*,
             std::vector<Ra::Core::Animation::KeyFramedValueController>>
        m_componentKeyFrames;

    /// The per-RenderObject keyframes.
    std::map<Ra::Core::Utils::Index, std::vector<Ra::Core::Animation::KeyFramedValueController>>
        m_renderObjectKeyFrames;

    /// The current keyframe.
    Ra::Core::Animation::KeyFramedValueController m_current;

    /// Observers id for engine event
    ///@{
    int m_entityAddObserverId { -1 };
    int m_entityRemoveObserverId { -1 };
    int m_roAddObserverId { -1 };
    ///@}
};

} // namespace Ra::Gui
