// Include Radium base application and its simple Gui
#include <Gui/BaseApplication.hpp>
#include <Gui/Viewer/Viewer.hpp>

// Create dummy scene if needed
#include <Core/Geometry/MeshPrimitives.hpp>
#include <Engine/Scene/EntityManager.hpp>
#include <Engine/Scene/GeometryComponent.hpp>
#include <Engine/Scene/GeometrySystem.hpp>

// To access render objects
#include <Engine/Rendering/RenderObject.hpp>
#include <Engine/Rendering/RenderObjectManager.hpp>

// To access picking results
#include <Engine/Rendering/Renderer.hpp>
#include <Gui/RadiumWindow/SimpleWindowFactory.hpp>

#include <Core/Utils/Log.hpp>

#include <sstream>

using namespace Ra::Core::Utils; // log

/**
 * Simple custom windows for picking demonstration
 */
class DemoWindow : public Ra::Gui::SimpleWindow
{
    Q_OBJECT
  public:
    explicit DemoWindow( uint w = 800, uint h = 640, QWidget* parent = nullptr ) :
        Ra::Gui::SimpleWindow( w, h, parent ) {
        connect( getViewer(),
                 &Ra::Gui::Viewer::toggleBrushPicking,
                 this,
                 &DemoWindow::toggleCirclePicking );
        connect(
            getViewer(), &Ra::Gui::Viewer::rightClickPicking, this, &DemoWindow::handlePicking );
    }
    DemoWindow( const DemoWindow& ) = delete;
    DemoWindow& operator=( const DemoWindow& ) = delete;
    DemoWindow( SimpleWindow&& )               = delete;
    DemoWindow& operator=( DemoWindow&& ) = delete;
    ~DemoWindow() override                = default;

    // activate circle picking
    void toggleCirclePicking( bool on ) {
        m_isTracking = on;
        centralWidget()->setMouseTracking( on );
    }

    // Handle the picking event generated when clicking on the window with  <RightButton>
    // When no key is pressed, only the nearest renderObject/Face/vertex of the designated pixel is
    // selected Once the "C" key is pressed, a orange circle is drawn that define the radius of
    // selection (<shift+whell modify the radius) This Radius allow to select things for all pixels
    // within the circle (with the "T", "V" or "E" key pressed while clicking) For each pixel i
    // within the circle, whe have, for the render object below the mouse ( pickingResult.m_roIdx ):
    //      pickResults.m_elementIdx : the nearest triangle
    //      pickResults.m_vertexIdx  : the nearest vertex in the triangle(0, 1 or 2)
    //      pickResults.m_edgeIdx    : the nearest edge in the triangle
    void handlePicking( const Ra::Engine::Rendering::Renderer::PickingResult& pickingResult ) {
        if ( m_isTracking )
        {
            LOG( logINFO ) << "Circle selection mode : ";
            // Access to the selection buffer within the circle
            auto& pickResults = getViewer()->getRenderer()->getPickingResults();
            // Not sure why, sometimes, we have several pickingResults ...
            LOG( logINFO ) << "Selected objects (" << pickResults.size() << ") : ";
            std::ostringstream ss;
            for ( const auto& pr : pickResults )
            { ss << pr.getRoIdx() << ' '; }
            LOG( logINFO ) << ss.str();

            LOG( logINFO ) << "Selection buffers  (" << pickResults.size() << ") : ";
            for ( const auto& pr : pickResults )
            { processPickResult( pr ); }
        }
        else
        {
            LOG( logINFO ) << "Single selection mode : ";
            // Access only the pixel from the selection buffer below the mouse
            processPickResult( pickingResult );
        }
    }

    void processPickResult( const Ra::Engine::Rendering::Renderer::PickingResult& pickingResult ) {

        //! [processing the picking info]
        LOG( logINFO ) << "Picking mode          : " << pickingResult.getMode();
        LOG( logINFO ) << "RenderObject selected : " << pickingResult.getRoIdx();
        LOG( logINFO ) << "Pixels in the selection : " << pickingResult.getIndices().size();
        pickingResult.removeDuplicatedIndices();
        LOG( logINFO ) << "Nearest [face, vertex, edge] by pixels : ";
        for ( const auto& [f, v, e] : pickingResult.getIndices() )
            LOG( logINFO ) << f << ' ' << v << ' ' << e;
    }

  private:
    bool m_isTracking { false };
};
#include "main.moc"

/**
 *      Main code to demonstrate how to manage selection
 */

/**
 * Define a factory that set the wanted renderer on the window
 */
class DemoWindowFactory : public Ra::Gui::BaseApplication::WindowFactory
{
  public:
    ~DemoWindowFactory() = default;
    inline Ra::Gui::MainWindowInterface* createMainWindow() const override {
        auto window = new DemoWindow();
        return window;
    }
};

void addDummyScene( Ra::Gui::BaseApplication& app ) {
    auto cube = Ra::Core::Geometry::makeSharpBox( { 0.1f, 0.1f, 0.1f } );
    auto e    = app.m_engine->getEntityManager()->createEntity( "Cube" );
    auto c =
        new Ra::Engine::Scene::TriangleMeshComponent( "Cube Mesh", e, std::move( cube ), nullptr );

    auto geometrySystem = app.m_engine->getSystem( "GeometrySystem" );
    geometrySystem->addComponent( e, c );
    app.m_mainWindow->prepareDisplay();
}

/**
 * main function.
 */
int main( int argc, char* argv[] ) {

    //! [Instatiating the application]
    Ra::Gui::BaseApplication app( argc, argv );
    //! [Instatiating the application]

    //! [Initializing the application]
    // The customization functor is called here
    app.initialize( DemoWindowFactory() );
    //! [Initializing the application]

    if ( argc < 2 )
    {
        // Create a scene when called without input file
        addDummyScene( app );
    }

    return app.exec();
}
