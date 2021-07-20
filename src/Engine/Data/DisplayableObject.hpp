#pragma once
#include <Engine/RaEngine.hpp>

#include <array>
#include <map>
#include <vector>

#include <Core/Containers/VectorArray.hpp>
#include <Core/Geometry/TriangleMesh.hpp>
#include <Core/Utils/Color.hpp>

namespace Ra {
namespace Engine {
namespace Data {

class ShaderProgram;

/**
 * Base class of any displayable object.
 */
class RA_ENGINE_API Displayable
{
  public:
    enum PickingRenderMode {
        PKM_POINTS   = 0,
        PKM_LINES    = 1,
        PKM_LINE_ADJ = 2,
        PKM_TRI      = 3,
        NO_PICKING
    };

    inline Displayable( const std::string& name ) : m_name( name ) {}
    Displayable( const Displayable& rhs ) = delete;
    void operator=( const Displayable& rhs ) = delete;

    virtual ~Displayable() = default;

    /// Returns the picking rendermode advised by the object (default PickingRenderMode::NO_PICKING)
    inline PickingRenderMode pickingRenderMode() const { return m_pickingRenderMode; }

    /// Returns the name of the mesh.
    inline const std::string& getName() const { return m_name; }

    /// Returns the underlying AbstractGeometry, which is in fact a TriangleMesh
    /// \see getTriangleMesh
    /// \todo not sure the displayable must have an abstract geometry ...
    virtual const Core::Geometry::AbstractGeometry& getAbstractGeometry() const = 0;
    virtual Core::Geometry::AbstractGeometry& getAbstractGeometry()             = 0;

    /**
     * This function is called at the start of the rendering. It will update the
     * necessary openGL buffers.
     */
    virtual void updateGL() = 0;

    /// Draw the mesh. Prog is used to set VertexAttribBinding, but it has to be
    /// already binded
    virtual void render( const ShaderProgram* prog ) = 0;

    //// Utility methods, used to display statistics
    virtual size_t getNumFaces() const { return 0; }
    virtual size_t getNumVertices() const { return 0; }

  protected:
    PickingRenderMode m_pickingRenderMode { NO_PICKING };

  private:
    std::string m_name {}; /// Name of the mesh.
};

} // namespace Data
} // namespace Engine
} // namespace Ra
