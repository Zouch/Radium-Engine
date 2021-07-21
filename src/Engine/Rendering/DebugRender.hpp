#pragma once

#include <Engine/RaEngine.hpp>

#include <memory>
#include <vector>

#include <Core/Containers/VectorArray.hpp>
#include <Core/Types.hpp>
#include <Core/Utils/Color.hpp>
#include <Core/Utils/Singleton.hpp>
#include <Engine/Data/ShaderProgram.hpp>

namespace Ra {
namespace Engine {
namespace Data {
class AttribArrayDisplayable;
class ShaderProgram;
} // namespace Data

namespace Rendering {

/** This allow to draw debug objects.
 * @todo : port this to a more Radium-style code
 */
class RA_ENGINE_API DebugRender final
{
    RA_SINGLETON_INTERFACE( DebugRender );

  public:
    DebugRender();
    ~DebugRender();

    void initialize();
    void render( const Core::Matrix4& view, const Core::Matrix4& proj );

    void
    addLine( const Core::Vector3& from, const Core::Vector3& to, const Core::Utils::Color& color );
    void addPoint( const Core::Vector3& p, const Core::Utils::Color& color );
    void addPoints( const Core::Vector3Array& p, const Core::Utils::Color& color );
    void addPoints( const Core::Vector3Array& p, const Core::Vector4Array& colors );
    void addMesh( const std::shared_ptr<Data::AttribArrayDisplayable>& mesh,
                  const Core::Transform& transform = Core::Transform::Identity() );

    // Shortcuts
    void addCross( const Core::Vector3& position, Scalar size, const Core::Utils::Color& color );

    void addSphere( const Core::Vector3& center, Scalar radius, const Core::Utils::Color& color );

    void addCircle( const Core::Vector3& center,
                    const Core::Vector3& normal,
                    Scalar radius,
                    const Core::Utils::Color& color );

    void addFrame( const Core::Transform& transform, Scalar size );

    void addTriangle( const Core::Vector3& p0,
                      const Core::Vector3& p1,
                      const Core::Vector3& p2,
                      const Core::Utils::Color& color );

    void addAABB( const Core::Aabb& box, const Core::Utils::Color& color );

    void addOBB( const Core::Aabb& box,
                 const Core::Transform& transform,
                 const Core::Utils::Color& color );

  private:
    struct Line {
        Line( const Core::Vector3& la, const Core::Vector3& lb, const Core::Utils::Color& lcol ) :
            a { la }, b { lb }, col { lcol } {}

        Core::Vector3 a, b;
        Core::Utils::Color col;
    };

    struct Point {
        Core::Vector3 p;
        Core::Vector3 c;
    };

    struct DbgMesh {
        std::shared_ptr<Data::AttribArrayDisplayable> mesh;
        Core::Transform transform;
    };

    void renderLines( const Core::Matrix4f& view, const Core::Matrix4f& proj );
    void renderPoints( const Core::Matrix4f& view, const Core::Matrix4f& proj );
    void renderMeshes( const Core::Matrix4f& view, const Core::Matrix4f& proj );

  private:
    // these shaders are owned by the manager, just keep a raw copy, since the
    // manager is available during whole execution.
    const Data::ShaderProgram* m_lineProg { nullptr };
    const Data::ShaderProgram* m_pointProg { nullptr };
    const Data::ShaderProgram* m_meshProg { nullptr };

    std::vector<Line> m_lines;
    std::vector<DbgMesh> m_meshes;

    std::vector<Point> m_points;
};

} // namespace Rendering
} // namespace Engine
} // namespace Ra
