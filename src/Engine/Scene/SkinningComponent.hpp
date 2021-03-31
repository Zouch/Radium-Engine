#pragma once

#include <Core/Animation/HandleWeight.hpp>
#include <Core/Animation/Pose.hpp>
#include <Core/Animation/SkinningData.hpp>
#include <Core/Asset/HandleData.hpp>
#include <Core/Geometry/TopologicalMesh.hpp>
#include <Core/Geometry/TriangleMesh.hpp>
#include <Core/Math/DualQuaternion.hpp>
#include <Core/Utils/Index.hpp>

#include <Engine/Data/Material.hpp>
#include <Engine/Scene/Component.hpp>
#include <Engine/Scene/ComponentMessenger.hpp>

namespace Ra {
namespace Engine {
namespace Scene {

/// The SkinningComponent class is responsible for applying Geometric Skinning Methods
/// on an animated object's mesh.
class RA_ENGINE_API SkinningComponent : public Component
{
  public:
    /// The Geometric Skinning Method.
    enum SkinningType {
        LBS = 0,  ///< Linear Blend Skinning
        DQS,      ///< Dual Quaternion Skinning
        COR,      ///< Center of Rotation skinning
        STBS_LBS, ///< Stretchable Twistable Bone Skinning with LBS
        STBS_DQS  ///< Stretchable Twistable Bone Skinning with DQS
    };

    /// How to skin the normal, tangent and binormal vectors.
    enum NormalSkinning {
        APPROX = 0, ///< Use the standard approximation method.
        GEOMETRIC   ///< Recompute from scratch from the skinned positions.
    };

    /// The skinning weight type.
    enum WeightType {
        STANDARD = 0, ///< Standard geometric skinning weights
        STBS          ///< Stretchable Twistable Bone Skinning weights
    };

    SkinningComponent( const std::string& name, SkinningType type, Entity* entity ) :
        Component( name, entity ),
        m_skinningType( type ),
        m_normalSkinning( APPROX ),
        m_isReady( false ),
        m_forceUpdate( false ),
        m_weightBone( 0 ),
        m_weightType( STANDARD ),
        m_showingWeights( false ) {}

    ~SkinningComponent() override {}

    virtual void initialize() override;

    /// Apply the Skinning Method.
    void skin();

    /// Update internal data and apply postprocesses to the mesh, e.g. normal computation.
    void endSkinning();

    /// Sets the Skinning method to use.
    void setSkinningType( SkinningType type );

    /// \returns the current skinning method.
    inline SkinningType getSkinningType() const { return m_skinningType; }

    /// Sets the method to use to skin the normal, tangent and binormal vectors.
    void setNormalSkinning( NormalSkinning normalSkinning );

    /// \returns the current method used to skin the normal, tangent and binormal vectors.
    inline NormalSkinning getNormalSkinning() const { return m_normalSkinning; }

    /// Loads the skinning data from the given Handledata.
    /// \note Call initialize() afterwards to finalize data registration.
    void handleSkinDataLoading( const Core::Asset::HandleData* data,
                                const std::string& meshName );

    /// @returns the reference skinning data.
    const Core::Animation::SkinningRefData* getSkinningRefData() const { return &m_refData; }

    /// @returns the current Pose data.
    const Core::Animation::SkinningFrameData* getSkinningFrameData() const { return &m_frameData; }

    /// @returns the name of the skinned mesh.
    const std::string getMeshName() const;

    /// @returns the name of the skeleton skinning the mesh.
    const std::string getSkeletonName() const;

    /// Toggles use of smart stretch.
    void setSmartStretch( bool on );

    /// @returns whether smart stretch is active or not.
    bool isSmartStretchOn() const { return m_smartStretch; }

    /// Toggles display of skinning weights.
    void showWeights( bool on );

    /// Returns whether the skinning weights are displayed or not.
    bool isShowingWeights();

    /// Set the type of skinning weight to display:
    ///  - 0 for standard skinning weights
    ///  - 1 for stbs weights
    void showWeightsType( WeightType type );

    /// Returns the type of skinning weights displayed.
    WeightType getWeightsType();

    /// Set the bone to show the weights of.
    void setWeightBone( uint bone );

  public:
    /// The Entity name for Component communication.
    std::string m_skelName;

  private:
    /// Registers the Entity name for Component communication (out).
    void setupIO( const std::string& id );

    /// Computes internal data related to the Skinning method.
    void setupSkinningType( SkinningType type );

    /// Internal function to create the skinning weights.
    void createWeightMatrix();

    /// Skinning Weight Matrix getter for CC.
    const Core::Animation::WeightMatrix* getWeightsOutput() const;

    /// Applies smart stretch to the given pose.
    void applySmartStretch( Core::Animation::Pose& pose );

  private:
    template <typename T>
    using Getter = typename ComponentMessenger::CallbackTypes<T>::Getter;

    template <typename T>
    using ReadWrite = typename ComponentMessenger::CallbackTypes<T>::ReadWrite;

    /// The mesh name for Component communication.
    std::string m_meshName;

    /// The refrence Skinning data.
    Core::Animation::SkinningRefData m_refData;

    /// The current Pose data.
    Core::Animation::SkinningFrameData m_frameData;

    /// Getter for the animation skeletton.
    Getter<Core::Animation::Skeleton> m_skeletonGetter;

    /// Read FMC's RO idx.
    Getter<Core::Utils::Index> m_renderObjectReader;

    /// Getter/Setter to the mesh
    bool m_meshIsPoly {false};
    ReadWrite<Core::Geometry::TriangleMesh> m_triMeshWriter;
    ReadWrite<Core::Geometry::PolyMesh> m_polyMeshWriter;

    /// The Skinning Method.
    SkinningType m_skinningType;
    NormalSkinning m_normalSkinning;

    /// Are all the required data available.
    bool m_isReady;

    /// Whether skinning is mandatory for the current frame.
    bool m_forceUpdate;

    /// The Topological mesh used to geometrically recompute the normals.
    Core::Geometry::TopologicalMesh m_topoMesh;

    /// The skinning weights, stored per bone.
    /// \note These are stored this way because we cannot build the weight matrix
    ///       without data from other components (skeleton).
    std::map<std::string, std::vector<std::pair<uint, Scalar>>> m_loadedWeights;
    /// The bind matrices, stored per bone.
    /// \note These are stored this way because we cannot fill m_refData
    ///       without data from other components (skeleton).
    std::map<std::string, Core::Transform> m_loadedBindMatrices;

    /// stretch mode: false = standard, true = smart.
    bool m_smartStretch {true};

    /// Initial RO Material when not showing skinning weights.
    std::shared_ptr<Data::Material> m_baseMaterial;
    /// Material to be used for the skinning weights
    std::shared_ptr<Data::Material> m_weightMaterial;
    Core::Vector3Array m_baseUV;
    Core::Vector3Array m_weightsUV;
    uint m_weightBone;
    WeightType m_weightType;
    bool m_showingWeights;
};

} // namespace Scene
} // namespace Engine
} // namespace Ra

