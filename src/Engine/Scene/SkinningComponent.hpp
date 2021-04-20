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

/**
 * \brief The SkinningComponent class is responsible for applying Geometric
 * Skinning Methods on an animated object's mesh.
 *
 * Regarding Component Communication, a SkinningComponent gives access to
 * the following data from the skinned mesh's name:
 *    - the Ra::Core::Animation::SkinningRefData containing all the
 *      reference data needed for the skinning.
 *    - the Ra::Core::Animation::SkinningFrameData containing all the
 *      data needed for the skinning at the current frame.
 *
 * \warning The Ra::Core::Animation::SkinningFrameData better be accessed after
 *          the skinning task.
 */
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

    /// \name Component Communication (CC)
    /// \{

    virtual void initialize() override;
    /// \}

    /// \name Build from fileData
    /// \{

    /// Loads the skinning data from the given Handledata.
    /// \note Call initialize() afterwards to finalize data registration.
    void handleSkinDataLoading( const Core::Asset::HandleData* data, const std::string& meshName );
    /// \}

    /// \name Skinning Process
    /// \{

    /// Apply the Skinning Method and update the SkinningFrameData.
    void skin();

    /// Update internal data and update the skinned mesh.
    void endSkinning();

    /// Sets the Skinning method to use.
    void setSkinningType( SkinningType type );

    /// Returns the current skinning method.
    inline SkinningType getSkinningType() const { return m_skinningType; }

    /// Sets the method to use to skin the normal, tangent and binormal vectors.
    void setNormalSkinning( NormalSkinning normalSkinning );

    /// Returns the current method used to skin the normal, tangent and binormal vectors.
    inline NormalSkinning getNormalSkinning() const { return m_normalSkinning; }

    /// Toggles use of smart stretch.
    /// \todo rename this as setPseudoIK
    void setSmartStretch( bool on );

    /// Returns whether smart stretch is active or not.
    /// \todo rename this as isPseudoIKOn
    bool isSmartStretchOn() const { return m_smartStretch; }
    /// \}

    /// \name Skinning Data
    /// \{

    /// Returns the name of the skinned mesh.
    const std::string& getMeshName() const;

    /// Returns the name of the skeleton skinning the mesh.
    const std::string& getSkeletonName() const;

    /// Returns the reference skinning data.
    const Core::Animation::SkinningRefData* getSkinningRefData() const { return &m_refData; }

    /// Returns the current Pose data.
    const Core::Animation::SkinningFrameData* getSkinningFrameData() const { return &m_frameData; }
    /// \}

    /// \name Skinning Weights Display
    /// \{

    /// Toggles display of skinning weights.
    void showWeights( bool on );

    /// Returns whether the skinning weights are displayed or not.
    bool isShowingWeights();

    /// Set the type of skinning weight to display
    void showWeightsType( WeightType type );

    /// Returns the type of skinning weights displayed.
    WeightType getWeightsType();

    /// Set the bone to show the weights of.
    void setWeightBone( uint bone );
    /// \}

  private:
    /// Setup Component Communication.
    void setupIO( const std::string& id );

    /// Internal function to create the skinning weights.
    void createWeightMatrix();

    /// Internal function to compute the STBS weights.
    void computeSTBSWeights();

    /// Applies smart stretch to the given pose.
    void applySmartStretch( Core::Animation::Pose& pose );

  private:
    template <typename T>
    using Getter = typename ComponentMessenger::CallbackTypes<T>::Getter;

    template <typename T>
    using ReadWrite = typename ComponentMessenger::CallbackTypes<T>::ReadWrite;

    /// The skinned-mesh name for Component communication.
    std::string m_meshName;

    /// The Skeleton name for Component communication.
    std::string m_skelName;

    /// The refrence Skinning data.
    Core::Animation::SkinningRefData m_refData;

    /// The current Pose data.
    Core::Animation::SkinningFrameData m_frameData;

    /// Getter for the animation skeletton.
    Getter<Core::Animation::Skeleton> m_skeletonGetter;

    /// The Skinning Method.
    SkinningType m_skinningType;

    /// The method to skin the normal, tangent and bitangent vectors.
    NormalSkinning m_normalSkinning;

    /// Stretch mode: false = standard, true = smart.
    /// TODO : verify smart streatch. Does not work with several gltf skeletons
    bool m_smartStretch {false};

    /// Are all the required data available.
    bool m_isReady;

    /// Whether skinning is mandatory for the current frame.
    bool m_forceUpdate;

    /// Read FMC's RO idx.
    Getter<Core::Utils::Index> m_renderObjectReader;

    /// Whether the skinned mesh is a TriangleMesh or a PolyMesh.
    bool m_meshIsPoly {false};

    /// Getter/Setter to the skinned mesh, in case it is a TriangleMesh.
    ReadWrite<Core::Geometry::TriangleMesh> m_triMeshWriter;

    /// Getter/Setter to the skinned mesh, in case it is a PolyMesh.
    ReadWrite<Core::Geometry::PolyMesh> m_polyMeshWriter;

    /// The Topological mesh used to geometrically recompute the normals.
    Core::Geometry::TopologicalMesh m_topoMesh;

    /// The per-bone skinning weights.
    /// \note These are stored this way because we cannot build the weight matrix
    ///       without data from other components (skeleton).
    std::map<std::string, std::vector<std::pair<uint, Scalar>>> m_loadedWeights;

    /// The per-bone bind matrices.
    /// \note These are stored this way because we cannot fill m_refData
    ///       without data from other components (skeleton).
    std::map<std::string, Core::Transform> m_loadedBindMatrices;

    /// Initial RO Material when not showing skinning weights.
    std::shared_ptr<Data::Material> m_baseMaterial;

    /// Material to be used for the skinning weights
    std::shared_ptr<Data::Material> m_weightMaterial;

    /// Initial UV coordinates of the skinned mesh vertices.
    Core::Vector3Array m_baseUV;

    /// UV coordinates to display the skinning weights on the skinned mesh.
    Core::Vector3Array m_weightsUV;

    /// The bone to display the skinning weights of.
    uint m_weightBone;

    /// The kind of skinning weights to display.
    WeightType m_weightType;

    /// Whether the skinning weights are displayed or not.
    bool m_showingWeights;
};

} // namespace Scene
} // namespace Engine
} // namespace Ra
