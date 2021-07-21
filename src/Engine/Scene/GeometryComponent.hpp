#pragma once

#include <Core/Asset/GeometryData.hpp>
#include <Core/Asset/VolumeData.hpp>
#include <Core/Geometry/TriangleMesh.hpp>
#include <Core/Geometry/Volume.hpp>
#include <Engine/Data/Mesh.hpp>
#include <Engine/Scene/Component.hpp>

namespace Ra {
namespace Engine {
namespace Data {
class VolumeObject;
} // namespace Data

namespace Scene {

/// Abstract interface of a geometric compoennet in the Engine.
class RA_ENGINE_API GeometryComponent : public Component
{
  public:
    GeometryComponent( const std::string& name, Entity* entity ) :
        Component( name, entity ), m_contentName( name ) {}
    ~GeometryComponent() override {}

    void initialize() override {}

    //  public:
    // Component communication management
    virtual void setupIO( const std::string& id );
    void setContentName( const std::string& name ) { m_contentName = name; }
    //    void setDeformable( bool b );

    /// Returns the index of the associated RO (the display mesh)
    //    Ra::Core::Utils::Index getRenderObjectIndex() const;
  protected:
  private:
    const Ra::Core::Utils::Index* roIndexRead() const;

  protected:
    // the index of the renderObject
    Ra::Core::Utils::Index m_roIndex {};
    std::string m_contentName {};
};

/*!
 * \brief Main class to convert Ra::Core::Asset::GeometryData to Ra::Engine::Mesh
 *
 * Exports access to the mesh geometry:
 *  - SurfaceMeshComponent: get, rw (set vertices, normals and triangles dirty)
 *  - Vertices: rw (if deformable)
 *  - normals: rw (if deformable)
 *  - triangles: rw (if deformable)
 */
template <typename CoreMeshType>
class SurfaceMeshComponent : public GeometryComponent
{
    using base = GeometryComponent;

  public:
    using RenderMeshType = typename Data::RenderMeshType::getType<CoreMeshType>::Type;

    inline SurfaceMeshComponent( const std::string& name,
                                 Entity* entity,
                                 const Ra::Core::Asset::GeometryData* data );

    /*!
     * Constructor from an existing mesh
     * \warning Moves the mesh and takes its ownership
     */
    inline SurfaceMeshComponent( const std::string& name,
                                 Entity* entity,
                                 CoreMeshType&& mesh,
                                 Core::Asset::MaterialData* mat = nullptr );

    ~SurfaceMeshComponent() override = default;

    /// Returns the current display geometry.
    inline const CoreMeshType& getCoreGeometry() const;
    inline RenderMeshType* getDisplayable();

    // Component communication management
    inline void setupIO( const std::string& id ) override;
    inline void setDeformable( bool b );

  private:
    inline void generateMesh( const Ra::Core::Asset::GeometryData* data );

    inline void finalizeROFromGeometry( const Core::Asset::MaterialData* data,
                                        Core::Transform transform );

    // Give access to the mesh and (if deformable) to update it
    inline const CoreMeshType* getMeshOutput() const;
    inline CoreMeshType* getMeshRw();

  private:
    // directly hold a reference to the displayMesh to simplify accesses in handlers
    std::shared_ptr<RenderMeshType> m_displayMesh { nullptr };
};

using TriangleMeshComponent = SurfaceMeshComponent<Ra::Core::Geometry::TriangleMesh>;
using PolyMeshComponent     = SurfaceMeshComponent<Ra::Core::Geometry::PolyMesh>;

/// \warning, WIP
/// \todo doc.
class RA_ENGINE_API PointCloudComponent : public GeometryComponent
{
    using base = GeometryComponent;

  public:
    PointCloudComponent( const std::string& name,
                         Entity* entity,
                         const Ra::Core::Asset::GeometryData* data );

    /*!
     * Constructor from an existing mesh
     * \warning Moves the mesh and takes its ownership
     */
    PointCloudComponent( const std::string& name,
                         Entity* entity,
                         Core::Geometry::PointCloud&& mesh,
                         Core::Asset::MaterialData* mat = nullptr );

    ~PointCloudComponent() override;

    void initialize() override;

    /// Returns the current display geometry.
    const Ra::Core::Geometry::PointCloud& getCoreGeometry() const;
    Data::PointCloud* getGeometry();

    /// set the splat size for rendering
    inline void setSplatSize( float s ) { m_splatSize = s; }

    /// get the splat size for rendering
    inline float getSplatSize() const { return m_splatSize; }

  public:
    // Component communication management
    void setupIO( const std::string& id ) override;
    void setDeformable( bool b );

  private:
    void generatePointCloud( const Ra::Core::Asset::GeometryData* data );

    void finalizeROFromGeometry( const Core::Asset::MaterialData* data, Core::Transform transform );

    // Give access to the mesh and (if deformable) to update it
    const Ra::Core::Geometry::PointCloud* getMeshOutput() const;
    Ra::Core::Geometry::PointCloud* getPointCloudRw();

  private:
    // directly hold a reference to the displayMesh to simplify accesses in handlers
    std::shared_ptr<Data::PointCloud> m_displayMesh { nullptr };
    // The diameter of the splat when rendered
    float m_splatSize { 0.0025f };
};

/*-----------------------------------------------------------------------------------------------*/

/*!
 * \brief Main class to convert Ra::Core::Asset::VolumeData to Ra::Engine::VolumeObject
 *
 * Exports access to the volume:
 *  - VolumeObject: get, rw
 */
class RA_ENGINE_API VolumeComponent : public Component
{
  public:
    VolumeComponent( const std::string& name,
                     Entity* entity,
                     const Ra::Core::Asset::VolumeData* data );
    ~VolumeComponent() override;

    void initialize() override;

  public:
    // Component communication management
    void setupIO( const std::string& id );
    void setContentName( const std::string& name );

    /// Returns the index of the associated RO (the display volume)
    Ra::Core::Utils::Index getRenderObjectIndex() const;
    Data::VolumeObject* getDisplayVolume();

  private:
    void generateVolumeRender( const Ra::Core::Asset::VolumeData* data );

    const Ra::Core::Geometry::AbstractVolume* getVolumeOutput() const;
    Ra::Core::Geometry::AbstractVolume* getVolumeRw();

    const Ra::Core::Utils::Index* roIndexRead() const;

  private:
    Ra::Core::Utils::Index m_volumeIndex {};
    std::string m_contentName {};
    std::shared_ptr<Data::VolumeObject> m_displayVolume { nullptr };
};

} // namespace Scene
} // namespace Engine
} // namespace Ra

#include <Engine/Scene/GeometryComponent.inl>
