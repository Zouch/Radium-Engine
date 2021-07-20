#include <Engine/Rendering/DebugRender.hpp>

#include <Engine/Data/DrawPrimitives.hpp>
#include <Engine/Data/Mesh.hpp>
#include <Engine/Data/ShaderProgram.hpp>
#include <Engine/Data/ShaderProgramManager.hpp>
#include <Engine/OpenGL.hpp>
#include <Engine/RadiumEngine.hpp>

#include <Core/Containers/MakeShared.hpp>
#include <Core/Geometry/MeshPrimitives.hpp>
#include <Core/Utils/Log.hpp>

#include <globjects/Program.h>
#include <globjects/Shader.h>
#include <globjects/base/StaticStringSource.h>

#include <fstream>

namespace Ra {
namespace Engine {
namespace Rendering {

using namespace Core::Utils; // log

DebugRender::DebugRender() = default;

DebugRender::~DebugRender() = default;

void DebugRender::initialize() {
    /// FIXME : this was not ported to globject ...
    /// \todo FIXED but not tested
    auto setShader = []( Data::ShaderProgramManager* manager,
                         const std::string& configName,
                         const char* vertexShader,
                         const char* fragmentShader ) -> const Data::ShaderProgram* {
        Data::ShaderConfiguration config { configName };
        config.addShaderSource( Data::ShaderType::ShaderType_VERTEX, vertexShader );
        config.addShaderSource( Data::ShaderType::ShaderType_FRAGMENT, fragmentShader );
        auto added = manager->addShaderProgram( config );
        if ( added ) { return *added; }
        else
        { return nullptr; }
    };

    auto shaderProgramManager = RadiumEngine::getInstance()->getShaderProgramManager();

    const char* lineVertStr = R"(
                layout (location = 0) in vec3 in_pos;
                layout (location = 5) in vec3 in_col;

                uniform mat4 model;
                uniform mat4 view;
                uniform mat4 proj;

                out vec3 v_color;
                void main()
                {
                    gl_Position = proj * view * model * vec4(in_pos, 1.0);
                    v_color = in_col;
                }
                )";

    const char* lineFragStr = R"(
                in vec3 v_color;
                out vec4 f_color;

                void main()
                {
                    f_color = vec4(v_color, 1.0);
                }
                )";

    static const char* pointVertStr = R"(
            layout (location = 0) in vec3 in_pos;
            layout (location = 1) in vec3 in_col;

            uniform mat4 view;
            uniform mat4 proj;

            out vec3 v_color;

            void main()
            {
                gl_Position = proj * view * vec4(in_pos, 1.0);
                v_color = in_col;
                gl_PointSize = 40 / gl_Position.w;
            }
            )";

    static const char* pointFragStr = R"(
                in vec3 v_color;
                out vec4 f_color;

                void main()
                {
                    f_color = vec4(v_color, 1.0);
                }
                )";
    static const char* meshVertStr  = R"(
                layout (location = 0) in vec3 in_pos;
                layout (location = 5) in vec3 in_col;

                uniform mat4 model;
                uniform mat4 view;
                uniform mat4 proj;

                out vec3 v_color;

                void main()
                {
                    gl_Position = proj * view * model * vec4(in_pos, 1.0);
                    v_color = in_col;
                }
                )";

    static const char* meshFragStr = R"(
                in vec3 v_color;
                out vec4 f_color;

                void main()
                {
                    f_color = vec4(v_color, 1.0);
                }
                )";

    m_lineProg  = setShader( shaderProgramManager, "dbgLineShader", lineVertStr, lineFragStr );
    m_pointProg = setShader( shaderProgramManager, "dbgPointShader", pointVertStr, pointFragStr );
    m_meshProg  = setShader( shaderProgramManager, "dbgMeshShader", meshVertStr, meshFragStr );

    GL_CHECK_ERROR;
}

void DebugRender::render( const Core::Matrix4& viewMatrix, const Core::Matrix4& projMatrix ) {
    renderLines( viewMatrix.cast<float>(), projMatrix.cast<float>() );
    renderPoints( viewMatrix.cast<float>(), projMatrix.cast<float>() );
    renderMeshes( viewMatrix.cast<float>(), projMatrix.cast<float>() );
}

void DebugRender::renderLines( const Core::Matrix4f& viewMatrix,
                               const Core::Matrix4f& projMatrix ) {
    Core::Vector3Array vertices;
    Core::Vector4Array colors;
    Core::Geometry::LineMesh::IndexContainerType indices;
    unsigned int indexI = 0;
    for ( const auto& l : m_lines )
    {
        vertices.push_back( l.a );
        vertices.push_back( l.b );

        colors.push_back( l.col );
        colors.push_back( l.col );

        indices.push_back( { indexI, indexI + 1 } );
        indexI += 2;
    }

    if ( !vertices.empty() )
    {
        const Core::Matrix4f id = Core::Matrix4f::Identity();

        m_lineProg->bind();
        m_lineProg->setUniform( "model", id );
        m_lineProg->setUniform( "view", viewMatrix );
        m_lineProg->setUniform( "proj", projMatrix );

        Core::Geometry::LineMesh geom;
        geom.setVertices( vertices );
        geom.setIndices( std::move( indices ) );
        geom.addAttrib( Data::Mesh::getAttribName( Data::Mesh::VERTEX_COLOR ), colors );

        Data::LineMesh mesh( "temp", std::move( geom ) );

        mesh.updateGL();
        ///\todo
        mesh.render( m_lineProg );
    }

    m_lines.clear();
}

void DebugRender::renderPoints( const Core::Matrix4f& viewMatrix,
                                const Core::Matrix4f& projMatrix ) {
    uint size = m_points.size();
    if ( 0 == size ) { return; }

    ///\todo use Engine::Mesh
    GLuint vao, vbo;
    glGenVertexArrays( 1, &vao );

    glBindVertexArray( vao );
    glGenBuffers( 1, &vbo );
    glBindBuffer( GL_ARRAY_BUFFER, vbo );
    glBufferData(
        GL_ARRAY_BUFFER, size * 2 * sizeof( Core::Vector3 ), m_points.data(), GL_DYNAMIC_DRAW );
    GLint64 ptr = 0;
    glVertexAttribPointer( 0, 3, GL_SCALAR, GL_FALSE, 6 * sizeof( Scalar ), (GLvoid*)ptr );
    glEnableVertexAttribArray( 0 );
    ptr += 3 * sizeof( Scalar );
    glVertexAttribPointer( 1, 3, GL_SCALAR, GL_FALSE, 6 * sizeof( Scalar ), (GLvoid*)ptr );
    glEnableVertexAttribArray( 1 );

    glEnable( GL_PROGRAM_POINT_SIZE );
    m_pointProg->bind();

    m_pointProg->setUniform( "view", viewMatrix );
    m_pointProg->setUniform( "proj", projMatrix );

    glDrawArrays( GL_POINTS, 0, size );
    glDisable( GL_PROGRAM_POINT_SIZE );

    glBindVertexArray( 0 );
    glDeleteVertexArrays( 1, &vao );
    glDeleteBuffers( 1, &vbo );

    m_points.clear();
}

void DebugRender::renderMeshes( const Core::Matrix4f& view, const Core::Matrix4f& proj ) {
    if ( m_meshes.empty() ) { return; }

    // Avoid too much states change
    uint idx = 0;
    std::sort( m_meshes.begin(), m_meshes.end(), []( const DbgMesh& a, const DbgMesh& b ) -> bool {
        return a.mesh->getRenderMode() < b.mesh->getRenderMode();
    } );

    for ( ; idx < m_meshes.size() && m_meshes[idx].mesh->getRenderMode() != GL_TRIANGLES; ++idx )
        ;

    m_lineProg->bind();
    m_lineProg->setUniform( "view", view );
    m_lineProg->setUniform( "proj", proj );

    for ( uint i = 0; i < idx; ++i )
    {
        m_lineProg->setUniform( "model", m_meshes[i].transform.matrix() );
        m_meshes[i].mesh->updateGL();
        m_meshes[i].mesh->render( m_lineProg );
    }

    m_meshProg->bind();
    m_meshProg->setUniform( "view", view );
    m_meshProg->setUniform( "proj", proj );

    for ( uint i = idx; i < m_meshes.size(); ++i )
    {
        m_meshProg->setUniform( "model", m_meshes[i].transform.matrix() );
        m_meshes[i].mesh->updateGL();
        m_meshes[i].mesh->render( m_meshProg );
    }

    m_meshes.clear();
}

void DebugRender::addLine( const Core::Vector3& from,
                           const Core::Vector3& to,
                           const Core::Utils::Color& color ) {
    Line l( from, to, color );
    m_lines.push_back( l );
}

void DebugRender::addPoint( const Core::Vector3& p, const Core::Utils::Color& c ) {
    m_points.push_back( { p, c.head<3>() } );
}

void DebugRender::addPoints( const Core::Vector3Array& p, const Core::Utils::Color& c ) {
    for ( uint i = 0; i < p.size(); ++i )
    { m_points.push_back( { p[i], c.head<3>() } ); }
}

void DebugRender::addPoints( const Core::Vector3Array& p, const Core::Vector4Array& c ) {
    CORE_ASSERT( p.size() == c.size(), "Data sizes mismatch." );
    for ( uint i = 0; i < p.size(); ++i )
    { m_points.push_back( { p[i], c[i].head<3>() } ); }
}

void DebugRender::addMesh( const std::shared_ptr<Data::AttribArrayDisplayable>& mesh,
                           const Core::Transform& transform ) {
    m_meshes.push_back( { mesh, transform } );
}

void DebugRender::addCross( const Core::Vector3& position,
                            Scalar size,
                            const Core::Utils::Color& color ) {
    const Scalar hz = size / 2.0;
    for ( int i = 0; i < 3; ++i )
    {
        Core::Vector3 offset = Core::Vector3::Zero();
        offset[i]            = hz;

        const Core::Vector3 from = position - offset;
        const Core::Vector3 to   = position + offset;

        addLine( from, to, color );
    }
}

void DebugRender::addSphere( const Core::Vector3& center,
                             Scalar radius,
                             const Core::Utils::Color& color ) {
    addMesh( Data::DrawPrimitives::Sphere( center, radius, color ) );
}

void DebugRender::addCircle( const Core::Vector3& center,
                             const Core::Vector3& normal,
                             Scalar radius,
                             const Core::Utils::Color& color ) {
    addMesh( Data::DrawPrimitives::Circle( center, normal, radius, 64, color ) );
}

void DebugRender::addFrame( const Core::Transform& transform, Scalar size ) {
    addMesh( Data::DrawPrimitives::Frame( transform, size ) );
}

void DebugRender::addTriangle( const Core::Vector3& p0,
                               const Core::Vector3& p1,
                               const Core::Vector3& p2,
                               const Core::Utils::Color& color ) {
    addMesh( Data::DrawPrimitives::Triangle( p0, p1, p2, color ) );
}

void DebugRender::addAABB( const Core::Aabb& box, const Core::Utils::Color& color ) {
    addMesh( Data::DrawPrimitives::AABB( box, color ) );
}

void DebugRender::addOBB( const Core::Aabb& box,
                          const Core::Transform& transform,
                          const Core::Utils::Color& color ) {
    addMesh( Data::DrawPrimitives::AABB( box, color ), transform );
}

RA_SINGLETON_IMPLEMENTATION( DebugRender );

} // namespace Rendering
} // namespace Engine
} // namespace Ra
