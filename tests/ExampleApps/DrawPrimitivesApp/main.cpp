#include <Engine/RaEngine.hpp>

#include <QApplication>

#include <QOpenGLContext>

#include <Engine/Scene/EntityManager.hpp>
#include <Gui/Viewer/RotateAroundCameraManipulator.hpp>

#include <minimalapp.hpp>
#include <minimalradium.hpp>

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
    app.m_viewer->resize( { 500, 500 } );
    CORE_ASSERT( app.m_viewer->getContext()->isValid(), "OpenGL was not initialized" );
    // process all events so that everithing is initialized
    QApplication::processEvents();

    // Create one system
    MinimalSystem* sys = new MinimalSystem;
    app.m_engine->registerSystem( "Minimal system", sys );

    // Create and initialize entity and component
    Ra::Engine::Scene::Entity* e = app.m_engine->getEntityManager()->createEntity( "Cube" );
    MinimalComponent* c          = new MinimalComponent( e );
    sys->addComponent( e, c );
    c->initialize();

    // prepare the viewer to render the scene (i.e. build RenderTechniques for the active renderer)
    app.m_viewer->prepareDisplay();
    auto keyMappingManager = Ra::Gui::KeyMappingManager::getInstance();
    // Add default manipulator listener
    keyMappingManager->addListener(
        Ra::Gui::RotateAroundCameraManipulator::KeyMapping::configureKeyMapping );

    // Start the app.
    app.m_frame_timer->start();
    app.m_viewer->setCameraManipulator( new Ra::Gui::RotateAroundCameraManipulator(
        *( app.m_viewer->getCameraManipulator() ), app.m_viewer.get() ) );

    return app.exec();
}
