// Include Radium base application and its simple Gui
#include <Gui/BaseApplication.hpp>
#include <Gui/RadiumWindow/SimpleWindowFactory.hpp>

// include the Engine/entity/component interface
#include <Core/Geometry/MeshPrimitives.hpp>
#include <Engine/Rendering/RenderObjectManager.hpp>
#include <Engine/Scene/EntityManager.hpp>
#include <Engine/Scene/GeometryComponent.hpp>
#include <Engine/Scene/GeometrySystem.hpp>

// include the custom material definition
#include <Engine/Data/RawShaderMaterial.hpp>

// include the Viewer to demonstrate dynamic edition of materials
#include <Gui/Viewer/Viewer.hpp>

// Qt
#include <QTimer>

/**
 * Demonstrate the usage of RawShaderMaterial functionalities
 */
// Vertex shader source code
const std::string _vertexShaderSource { "#include \"TransformStructs.glsl\"\n"
                                        "layout (location = 0) in vec3 in_position;\n"
                                        "layout (location = 0) out vec3 out_pos;\n"
                                        "uniform Transform transform;\n"
                                        "void main(void)\n"
                                        "{\n"
                                        "    mat4 mvp    = transform.proj * transform.view;\n"
                                        "    out_pos     = in_position;\n"
                                        "    gl_Position = mvp*vec4(in_position.xyz, 1.0);\n"
                                        "}\n" };
// Fragment shader source code
const std::string _fragmentShaderSource {
    "layout (location = 0) in  vec3 in_pos;\n"
    "layout (location = 0) out vec4 out_color;\n"
    "uniform vec4 aColorUniform;\n"
    "uniform float aScalarUniform;\n"
    "void main(void)\n"
    "{\n"
    "    out_color =  ( 1 + cos( 20 * ( in_pos.x + aScalarUniform ) ) ) * 0.5 * aColorUniform;\n"
    "}\n" };

// Fragment shader source code
const std::string _fragmentShaderSource2 {
    "layout (location = 0) in  vec3 in_pos;\n"
    "layout (location = 0) out vec4 out_color;\n"
    "uniform vec4 aColorUniform;\n"
    "uniform float aScalarUniform;\n"
    "void main(void)\n"
    "{\n"
    "    out_color =  ( 1 + sin( 20 * ( in_pos.y + aScalarUniform ) ) ) * 0.5 * aColorUniform;\n"
    "}\n" };

const std::vector<std::pair<Ra::Engine::Data::ShaderType, std::string>> _config1 {
    { Ra::Engine::Data::ShaderType::ShaderType_VERTEX, _vertexShaderSource },
    { Ra::Engine::Data::ShaderType::ShaderType_FRAGMENT, _fragmentShaderSource } };

const std::vector<std::pair<Ra::Engine::Data::ShaderType, std::string>> _config2 {
    { Ra::Engine::Data::ShaderType::ShaderType_VERTEX, _vertexShaderSource },
    { Ra::Engine::Data::ShaderType::ShaderType_FRAGMENT, _fragmentShaderSource2 } };

class MyParameterProvider : public Ra::Engine::Data::ShaderParameterProvider
{
  public:
    MyParameterProvider() {}
    ~MyParameterProvider() {}
    void updateGL() override {
        // Method called before drawing each frame in Renderer::updateRenderObjectsInternal.
        // The name of the parameter corresponds to the shader's uniform name.
        m_renderParameters.addParameter( "aColorUniform", m_colorParameter );
        m_renderParameters.addParameter( "aScalarUniform", m_scalarParameter );
    }
    void setOrComputeTheParameterValues() {
        // client side computation of the parameters, e.g.
        m_colorParameter  = Ra::Core::Utils::Color::Red();
        m_scalarParameter = .5_ra;
    }

  private:
    Ra::Core::Utils::Color m_colorParameter { Ra::Core::Utils::Color::Green() };
    Scalar m_scalarParameter { 1 };
};

/**
 * Generate a quad with a ShaderMaterial attached
 * @param app
 * @return The renderObject associated to the created component.
 */
std::shared_ptr<Ra::Engine::Rendering::RenderObject> initQuad( Ra::Gui::BaseApplication& app ) {
    //! [Creating the quad]
    auto quad = Ra::Core::Geometry::makeZNormalQuad( { 1_ra, 1_ra } );

    //! [Create the engine entity for the quad]
    auto e = app.m_engine->getEntityManager()->createEntity( "Quad Entity" );

    //! [Create Parameter provider for the shader]
    auto paramProvider = std::make_shared<MyParameterProvider>();
    paramProvider->setOrComputeTheParameterValues();

    //! [Create the shader material]
    Ra::Core::Asset::RawShaderMaterialData mat { "Quad Material", _config1, paramProvider };

    //! [Create a geometry component using the custom material]
    auto c =
        new Ra::Engine::Scene::TriangleMeshComponent( "Quad Mesh", e, std::move( quad ), &mat );

    //! [Register the entity/component association to the geometry system ]
    auto system = app.m_engine->getSystem( "GeometrySystem" );
    system->addComponent( e, c );

    //![get the renderobject for further edition]
    auto ro = Ra::Engine::RadiumEngine::getInstance()->getRenderObjectManager()->getRenderObject(
        c->m_renderObjects[0] );

    // Initialize all OpenGL state for the scene content
    app.m_mainWindow->prepareDisplay();
    return ro;
}

int main( int argc, char* argv[] ) {
    //! [Creating the application]
    Ra::Gui::BaseApplication app( argc, argv );
    app.initialize( Ra::Gui::SimpleWindowFactory {} );
    //! [Creating the application]

    //! [add the custom material to the material system]
    Ra::Engine::Data::RawShaderMaterial::registerMaterial();

    //! [Populate the Radium ecosystem]
    auto ro = initQuad( app );

    //! [Change the shader after 3 seconds]
    auto renderer    = app.m_mainWindow->getViewer()->getRenderer();
    auto changeTimer = new QTimer( &app );
    changeTimer->setInterval( 3000 );
    QObject::connect( changeTimer, &QTimer::timeout, [ro, renderer]() {
        auto paramProvider = std::make_shared<MyParameterProvider>();
        auto mat = static_cast<Ra::Engine::Data::RawShaderMaterial*>( ro->getMaterial().get() );
        mat->updateShaders( _config2, paramProvider );
        renderer->buildRenderTechnique( ro.get() );
    } );
    changeTimer->start();

    //! [terminate the app after 6 second (approximatively).]
    auto close_timer = new QTimer( &app );
    close_timer->setInterval( 6000 );
    QObject::connect( close_timer, &QTimer::timeout, [&app]() { app.appNeedsToQuit(); } );
    close_timer->start();

    return app.exec();
}
