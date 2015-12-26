#ifndef RADIUMENGINE_MAINWINDOW_HPP
#define RADIUMENGINE_MAINWINDOW_HPP

#include <QMainWindow>
#include "ui_MainWindow.h"

#include <qdebug.h>
#include <QEvent>

#include <MainApplication/TimerData/FrameTimerData.hpp>

namespace Ra
{
    namespace Engine
    {
        class Entity;
    }
}

namespace Ra
{
    namespace Gui
    {
        class EntityTreeModel;
        class Viewer;
        class MaterialEditor;
    }
}

namespace Ra
{
    namespace Plugins
    {
        class RadiumPluginInterface;
    }
}

namespace Ra
{
    namespace Gui
    {

        // Just forwards everything to the viewer.
        class MainWindow : public QMainWindow, private Ui::MainWindow
        {
            Q_OBJECT

        public:
            explicit MainWindow( QWidget* parent = nullptr );
            virtual ~MainWindow();

            void createConnections();

            Gui::Viewer* getViewer();

            inline const std::vector<Core::KeyEvent>& getKeyEvents() const;
            inline const std::vector<Core::MouseEvent>& getMouseEvents() const;

            inline void flushEvents();

            void updateUi( Plugins::RadiumPluginInterface* plugin );

        private slots:
            void loadFile();
            void openMaterialEditor();

            void handlePicking( int drawableIndex );

        public slots:
            void onEntitiesUpdated();

            // Frame timers ui slots
            void onUpdateFramestats( const std::vector<FrameTimerData>& stats );

            // Selection tools
            void onSelectionChanged( const QItemSelection& selected, const QItemSelection& deselected );

            // Gizmo buttons slots
            void gizmoShowNone();
            void gizmoShowTranslate();
            void gizmoShowRotate();
            //void gizmoShowScale();

            void displayEntityRenderObjects( Engine::Entity* entity );
            void displayComponentRenderObjects( Engine::Component* component );

            void renderObjectListItemClicked( int item );
            void changeRenderObjectShader( const QString& shaderName );

            void toggleRO();
            void removeRO();
            void editRO();

        signals:
            void closed();
            void fileLoading( const QString path );
            void entitiesUpdated( const std::vector<Engine::Entity*>& );
            void framescountForStatsChanged( int count );

            void selectedEntity( Engine::Entity* selectedEntity );
            void selectedComponent( Engine::Component* selectedComponent);

        private:
            // Basic I/O management
            // Intercept key events from Qt

            virtual void keyPressEvent( QKeyEvent* event ) override;
            virtual void keyReleaseEvent( QKeyEvent* event ) override;

            virtual void closeEvent( QCloseEvent* event ) override;

            void displayRenderObjects( Engine::Component* component );

            std::shared_ptr<Engine::RenderObject> getSelectedRO();

        public:
            // Accept viewer mouse events.
            inline void viewerMousePressEvent( QMouseEvent* event );
            inline void viewerMouseReleaseEvent( QMouseEvent* event );
            inline void viewerMouseMoveEvent( QMouseEvent* event );
            inline void viewerWheelEvent( QWheelEvent* event );

        private:
            static Core::MouseEvent mouseEventQtToRadium( const QMouseEvent* qtEvent );
            static Core::MouseEvent wheelEventQtToRadium( const QWheelEvent* qtEvent );
            static Core::KeyEvent   keyEventQtToRadium( const QKeyEvent* qtEvent );

        private:
            // Stores the events received by the UI during last frame.
            std::vector<Core::MouseEvent> m_mouseEvents;
            std::vector<Core::KeyEvent>   m_keyEvents;

            EntityTreeModel* m_entityTreeModel;

            MaterialEditor* m_materialEditor;
        };

    } // namespace Gui
} // namespace Ra

#include <MainApplication/Gui/MainWindow.inl>

#endif // RADIUMENGINE_MAINWINDOW_HPP
