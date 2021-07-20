#pragma once

#include <vector>

#include <Core/Types.hpp>
#include <Core/Utils/Color.hpp>
#include <Core/Utils/Index.hpp>

#include <Core/Asset/Camera.hpp>
#include <Engine/Data/RenderParameters.hpp>
#include <Engine/RadiumEngine.hpp>
#include <Engine/Rendering/RenderObject.hpp>
#include <Engine/Scene/CameraComponent.hpp>

namespace Ra {
namespace Engine {

namespace Data {
class PlainMaterial;
class RenderObject;
} // namespace Data

namespace Scene {
class Component;
} // namespace Scene

namespace Rendering {
class RenderTechnique;
}
} // namespace Engine

namespace Gui {
/** Base class for gizmos, i.e. graphic tools to manipulate a transform.
 *
 * \todo There is a lot of duplicated code shared between the different Gizmos (rotate, translate,
 * scale). This is due to the fact that they all have Render Objects in xyz directions, and which
 * can be selected with the same logic. An alternative would be to have an intermediate class ,
 * e.g. `XYZGizmo` which performs all the generic operations (e.g. render object coloring in
 * #selectConstraint and #mouseMove).
 */
class Gizmo
{
  public:
    enum Mode {
        LOCAL,
        GLOBAL,
    };

  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    Gizmo( Engine::Scene::Component* c,
           const Core::Transform& worldTo,
           const Core::Transform& t,
           Mode mode );

    virtual ~Gizmo();

    // shows or hides the gizmos drawables.
    void show( bool on );

    /// Called every time the underlying transform may have changed.
    virtual void
    updateTransform( Mode mode, const Core::Transform& worldTo, const Core::Transform& t ) = 0;

    /// Called when one of the drawables of the gizmo has been selected.
    virtual void selectConstraint( int drawableIndex ) = 0;

    /// return the selection state of the gizmo: true if the gizmo is selected, false if not.
    virtual bool isSelected() = 0;

    /// Called when the gizmo is first clicked, with the camera parameters and the initial pixel
    /// coordinates.
    virtual void setInitialState( const Core::Asset::Camera& cam,
                                  const Core::Vector2& initialXY ) = 0;

    /// Called when the mouse movement is recorder with the camera parameters and the current pixel
    /// coordinates.
    virtual Core::Transform mouseMove( const Core::Asset::Camera& cam,
                                       const Core::Vector2& nextXY,
                                       bool stepped = false,
                                       bool whole   = false ) = 0;

  protected:
    /// Find a mouse-designed point on a 3D axis
    static bool findPointOnAxis( const Core::Asset::Camera& cam,
                                 const Core::Vector3& origin,
                                 const Core::Vector3& axis,
                                 const Core::Vector2& pix,
                                 Core::Vector3& pointOut,
                                 std::vector<Scalar>& hits );

    /// Find a mouse-designed point on a 3D plane
    static bool findPointOnPlane( const Core::Asset::Camera& cam,
                                  const Core::Vector3& origin,
                                  const Core::Vector3& axis,
                                  const Core::Vector2& pix,
                                  Core::Vector3& pointOut,
                                  std::vector<Scalar>& hits );

    /// read access to the gizmo render objects id
    inline const std::vector<Engine::Rendering::RenderObject*>& ros() const { return m_ros; }

    /// add a render object to display the Gizmo
    void addRenderObject( Engine::Rendering::RenderObject* ro );

    /** Generate a the rendertechnique to draw the gizmo using the required color :
     * 0-Red, 1-Green, 2-Blue.
     * The build render technique has a selection-dependent parameter provider for the shader
     * configuration used to draw the gizmo. It is this provider that manage the appeartance
     * changes when the selection state changes on the gizmo.
     */
    static std::shared_ptr<Engine::Rendering::RenderTechnique> makeRenderTechnique( int color );

    /** The Materials used to diplay the gizmo: 0-Red, 1-Green, 2-Blue.
     *  The material are shared accros gizmos. This might allow coherent dynamic style for
     *  Ui objects.
     */
    static std::array<std::shared_ptr<Ra::Engine::Data::PlainMaterial>, 3> s_material;

  protected:
    Core::Transform m_worldTo;        ///< World to local space where the transform lives.
    Core::Transform m_transform;      ///< Transform to be edited.
    Engine::Scene::Component* m_comp; ///< Engine Ui component.
    Mode m_mode;                      ///< local or global.

    /** The parameterProvider for Selectable UI Object
     * This class will manage the appearance change when a gizmo element is selected.
     * when building a gizmo component (renderObject),
     *      1 - Create a renderTechnique with the required color parameter
     *      2 - Associate the renderTechnique to the renderObject
     *
     * When the selection state of a gizmo component changes, notify its rendertechnique through
     * a call to toggleState.
     */
    class UiSelectionControler final : public Engine::Data::ShaderParameterProvider
    {
      public:
        /// Construct a controler given a material and the color to used when selected
        explicit UiSelectionControler(
            std::shared_ptr<Ra::Engine::Data::PlainMaterial>& material,
            const Core::Utils::Color& selectedColor = Core::Utils::Color::Yellow() );
        UiSelectionControler()                              = delete;
        UiSelectionControler( const UiSelectionControler& ) = delete;
        ~UiSelectionControler() override                    = default;
        /// Inherited
        void updateGL() override;
        /// Swap the state of the controler
        void toggleState();
        /// Set the state of the controler to true
        void setState();
        /// Set the state of the controler to false
        void clearState();

      private:
        /// The material associated to the controler (PlainMaterial)
        std::shared_ptr<Ra::Engine::Data::PlainMaterial> m_associatedMaterial;
        /// The color to use when selected
        Core::Utils::Color m_selectedColor;
        /// State indicator of the controler
        bool m_selected { false };
    };

    /// Returns the controler (ShaderParametersProvider) associated to the given gizmo component
    Gizmo::UiSelectionControler* getControler( int ro );

  private:
    std::vector<Engine::Rendering::RenderObject*> m_ros; ///< ros for the gizmo.
};
} // namespace Gui
} // namespace Ra
