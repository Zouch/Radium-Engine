#include <QApplication>
#include <QTimer>

#include <QOpenGLContext>

#include <GuiBase/TimerData/FrameTimerData.hpp>
#include <GuiBase/Viewer/Viewer.hpp>

#include <Engine/Managers/EntityManager/EntityManager.hpp>
#include <Engine/Renderer/RenderTechnique/ShaderConfigFactory.hpp>

#include <minimalapp.hpp>
#include <minimalradium.hpp>

#include <QObject>
#include <QWidget>

int main( int argc, char* argv[] ) {

    // Create default format for Qt.
    QSurfaceFormat format;
    format.setVersion( 4, 4 );
    format.setProfile( QSurfaceFormat::CoreProfile );
    format.setDepthBufferSize( 24 );
    format.setStencilBufferSize( 8 );
    // format.setSamples( 16 );
    format.setSwapBehavior( QSurfaceFormat::DoubleBuffer );
    format.setSwapInterval( 0 );
    QSurfaceFormat::setDefaultFormat( format );

    // Create app and show viewer window
    MinimalApp app( argc, argv );

    app.m_viewer->show();
    app.m_viewer->resize( {500, 500} );
    //    CORE_ASSERT( app.m_viewer->getContext()->isValid(), "OpenGL was not initialized" );
    // process all events so that everithing is initialized
    QApplication::processEvents();

    QMetaObject::Connection* const connection = new QMetaObject::Connection;
    *connection                               = QObject::connect(
        app.m_viewer.get(), &Ra::Gui::Viewer::glInitialized, [&app, connection]() {
            // Create one system
            MinimalSystem* sys = new MinimalSystem;
            app.m_engine->registerSystem( "Minimal system", sys );
            // Create and initialize entity and component
            Ra::Engine::Entity* e = app.m_engine->getEntityManager()->createEntity( "Scene" );
            MinimalComponent* c   = new MinimalComponent( e );
            sys->addComponent( e, c );
            c->initialize();
            auto aabb = Ra::Engine::RadiumEngine::getInstance()->computeSceneAabb();
            if ( !aabb.isEmpty() ) { app.m_viewer->fitCameraToScene( aabb ); }

            QObject::disconnect( *connection );
            delete connection;
        } );

    // Start the app.
    app.m_frame_timer->start();
    return app.exec();
}
