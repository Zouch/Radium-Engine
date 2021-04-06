#pragma once

#include <Engine/Scene/ItemEntry.hpp>
#include <Gui/RaGui.hpp>

#include <QFrame>

namespace Ui {
class SkeletonBasedAnimationUI;
}

namespace Ra {
namespace Engine {
class RadiumEngine;
namespace Scene {
class Entity;
class SkeletonBasedAnimationSystem;
class SkeletonComponent;
class SkinningComponent;
class SkinningSystem;
} // namespace Scene
} // namespace Engine

namespace Gui {
class SelectionManager;
class Timeline;

class RA_GUI_API SkeletonBasedAnimationUI : public QFrame
{
    Q_OBJECT

  public:
    /// \brief Builds the UI for the Skeleton-based Animation process.
    ///
    /// \param system   the Engine::Scene::SkeletonBasedAnimationSystem of the application's engine.
    /// \param timeline [optional] the application timeline for the display of animation keyframes.
    /// \param parent   [optional] the QWidget holding this ui.
    explicit SkeletonBasedAnimationUI( Engine::Scene::SkeletonBasedAnimationSystem* system,
                                       Timeline* timeline = nullptr,
                                       QWidget* parent    = nullptr );

    ~SkeletonBasedAnimationUI();

    /// Returns the number of QActions provided by this UI.
    int getActionNb();

    /// Returns the \p i -th QAction provided by this UI.
    QAction* getAction( int i );

    /// Registers the animation keyframes int the timeline.
    void postLoadFile( Engine::Scene::Entity* entity );

  signals:
    void askForUpdate();

  public slots:
    void selectionChanged( const Engine::Scene::ItemEntry& ent );

  private slots:
    /// \name Player GUI
    /// \{

    /// Slot for the user requesting x-ray animation skeletons (through the toolbar).
    void on_actionXray_triggered( bool checked );

    /// Slot for the user changing the animation speed.
    void on_m_speed_valueChanged( double arg1 );

    /// Slot for the user toggling the animation ping-pong mode.
    void on_m_pingPong_toggled( bool checked );

    /// Slot for the user toggling the animation auto-repeat mode.
    void on_m_autoRepeat_toggled( bool checked );
    /// \}

    /// \name Animation Edition GUI
    /// \{

    /// Slot for the user changing the skeleton animation.
    void on_m_currentAnimation_currentIndexChanged( int index );

    /// Slot for the user adding a new skeleton animation.
    void on_m_newAnim_clicked();

    /// Slot for the user removing a skeleton animation.
    void on_m_removeAnim_clicked();

    /// Slot for the user wanting to load a skeleton animation from a file.
    void on_m_loadAnim_clicked();

    /// Slot for the user wanting to save the current skeleton animation to a file.
    void on_m_saveAnim_clicked();
    /// \}

    /// \name Rig GUI
    /// \{

    /// Slot for the user requesting x-ray animation skeletons (through the toolbox).
    void on_m_xray_clicked( bool checked );

    /// Slot for the user requesting to show the skeleton.
    void on_m_showSkeleton_toggled( bool checked );
    /// \}

    /// \name Skinning GUI
    /// \{

    /// Slot for the user toggling the smart stretch.
    void on_m_smartStretch_toggled( bool checked );

    /// Slot for the user changing the skinning method.
    void on_m_skinningMethod_currentIndexChanged( int index );

    /// Slot for the user requesting to display per-bone skinning weights.
    void on_m_showWeights_toggled( bool checked );

    /// Slot for the user selecting the kind of displayed skinning weights.
    void on_m_weightsType_currentIndexChanged( int index );

    /// Slot for the user changing the way normals are computed.
    void on_m_normalSkinning_currentIndexChanged( int index );

    /// Slot for the user requesting to use LBS skinning.
    void on_actionLBS_triggered();

    /// Slot for the user requesting to use DQS skinning.
    void on_actionDQS_triggered();

    /// Slot for the user requesting to use CoR skinning.
    void on_actionCoR_triggered();

    /// Slot for the user requesting to use STBS-LBS skinning.
    void on_actionSTBSLBS_triggered();

    /// Slot for the user requesting to use STBS-DQS skinning.
    void on_actionSTBSDQS_triggered();
    /// \}

  private:
    /// The actual ui.
    Ui::SkeletonBasedAnimationUI* ui;

    /// The Skeleton-based animation system.
    Engine::Scene::SkeletonBasedAnimationSystem* m_system {nullptr};

    /// The skeleton component of the current entity.
    Engine::Scene::SkeletonComponent* m_currentSkeleton {nullptr};

    /// The skinning components of the current entity.
    std::vector<Engine::Scene::SkinningComponent*> m_currentSkinnings;

    /// The Application's Timeline, used to manage keyframes internally.
    Timeline* m_timeline {nullptr};

    /// The last selection data for updating the Timeline when changing animation.
    Engine::Scene::ItemEntry m_selection;
};

} // namespace Gui
} // namespace Ra
