#pragma once

#include <Engine/Data/DisplayableObject.hpp>
#include <Engine/Data/Mesh.hpp>
#include <Engine/Data/Texture.hpp>
#include <Engine/RaEngine.hpp>

namespace Ra {
namespace Core {
namespace Geometry {
class AbstractVolume;
} // namespace Geometry
} // namespace Core
namespace Engine {
namespace Data {
/**
 * A class representing an openGL volume
 *
 * @todo Right now, a volumeObject define both the geometry (a parallelepiped) and the material (a
 * density grid) This should be modified to handle density grids deleimited by a general geometry.
 */
class RA_ENGINE_API VolumeObject : public Displayable
{

  public:
    explicit VolumeObject( const std::string& name );
    VolumeObject( const VolumeObject& rhs ) = delete;
    void operator=( const VolumeObject& rhs ) = delete;

    ~VolumeObject() override;

    using Displayable::getName;

    /// Returns the underlying AbstractGeometry, which is in fact a AbstractVolume
    /// \see getTriangleMesh
    inline const Core::Geometry::AbstractGeometry& getAbstractGeometry() const override;
    inline Core::Geometry::AbstractGeometry& getAbstractGeometry() override;

    /// Returns the underlying AbstractVolume
    inline const Core::Geometry::AbstractVolume& getVolume() const;
    inline Core::Geometry::AbstractVolume& getVolume();

    /// Use the given volume for display. \warning Takes the pointer ownership
    void loadGeometry( Core::Geometry::AbstractVolume* volume );

    /// Use the given volume for display and build the proxy from the given aabb. \warning Takes the
    /// pointer ownership
    void loadGeometry( Core::Geometry::AbstractVolume* volume, const Core::Aabb& aabb );

    /// Mark the data types as dirty, forcing an update of the openGL buffer.
    inline void setDirty() { m_isDirty = true; }

    /// get read access to the 3d texture storing the data
    const Texture& getDataTexture() { return m_tex; }

    /**
     * This function is called at the start of the rendering. It will update the
     * necessary openGL buffers.
     */
    void updateGL() override;

    /// Draw the mesh.
    void render( const ShaderProgram* prog ) override;

    /// 6 quad faces of the cube, thus 12 triangles.
    size_t getNumFaces() const override { return 12; }
    /// 8 vertices of the cube
    inline size_t getNumVertices() const override { return 8; }

  private:
    std::unique_ptr<Core::Geometry::AbstractVolume> m_volume;
    Texture m_tex;
    /// Mesh used to display the bounding box of the grid for the ray marching
    Mesh m_mesh;

    /// General dirty bit of the mesh. An empty volume is not dirty
    bool m_isDirty { false };
};

} // namespace Data
} // namespace Engine
} // namespace Ra

#include <Engine/Data/VolumeObject.inl>
