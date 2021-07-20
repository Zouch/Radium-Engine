#pragma once

#include <Engine/RaEngine.hpp>

#include <set>
#include <vector>

#include <Core/Containers/AlignedAllocator.hpp>
#include <Core/Types.hpp>
#include <Core/Utils/Color.hpp>
#include <Core/Utils/Log.hpp>

namespace Ra {
namespace Engine {
namespace Data {

class Texture;
class ShaderProgram;

/**
 * Management of shader parameters with automatic binding of a named parameter to the corresponding
 * glsl uniform.
 */
class RA_ENGINE_API RenderParameters final
{
  public:
    /// Base abstract parameter Class
    ///\todo use globjects Uniform directly (which seems to correspond to the
    /// same data/purpose, even if not well documented
    class Parameter
    {
      public:
        Parameter() = default;
        explicit Parameter( const std::string& name ) : m_name( name ) {}
        virtual ~Parameter() = default;
        /** Bind the parameter uniform on the shader program
         *
         * @param shader The shader to bind to.
         */
        virtual void bind( const Data::ShaderProgram* shader ) const = 0;
        /** The name of the parameter.
         * This must correspond to the name of a Uniform in the shader the Parameter is bound to.
         */
        std::string m_name {};
    };

    /** Typed parameter.
     * Define a Parameter with a typed value
     * @tparam T The type of the parameter's value.
     */
    template <typename T>
    class TParameter final : public Parameter
    {
      public:
        TParameter() = default;
        TParameter( const std::string& name, const T& value ) :
            Parameter( name ), m_value( value ) {}
        ~TParameter() override = default;
        void bind( const Data::ShaderProgram* shader ) const override;
        /// The value of the parameter
        T m_value {};
    };

    /**
     * Special case of a Texture parameter
     */
    class TextureParameter final : public Parameter
    {
      public:
        TextureParameter() = default;
        TextureParameter( const std::string& name, Data::Texture* tex, int texUnit ) :
            Parameter( name ), m_texture( tex ), m_texUnit( texUnit ) {}
        ~TextureParameter() override = default;
        void bind( const Data::ShaderProgram* shader ) const override;

        /// The texture object
        Data::Texture* m_texture { nullptr };
        /// The texture unit where to bind the parameter
        int m_texUnit { -1 };
    };

    /** Set of typed parameters
     * For a given shader Program, all the parameters are stored by type, in several parameter sets.
     *
     * @tparam T The type of parameteres in the set.
     */
    template <typename T>
    class UniformBindableSet final
        : public std::map<
              std::string,
              T,
              std::less<std::string>,
              Core::AlignedAllocator<std::pair<const std::string, T>, EIGEN_MAX_ALIGN_BYTES>>
    {
      public:
        /** Bind the whole parameter set to the corresponding shader uniforms
         *
         * @param shader The shader to bind to.
         */
        void bind( const Data::ShaderProgram* shader ) const;
    };

    ///\todo use array of parameter to templated uniform, as in globjects ?
    /// Parameter of type int
    using IntParameter  = TParameter<int>;
    using BoolParameter = TParameter<bool>;
    /// Parameter of type unsigned int
    using UIntParameter = TParameter<uint>;
    /// Parameter of type Scalar
    using ScalarParameter = TParameter<Scalar>;
    /// Parameter of type vector of int
    using IntsParameter = TParameter<std::vector<int>>;
    /// Parameter of type vector of unsigned int
    using UIntsParameter = TParameter<std::vector<uint>>;

    /** Parameter of type vector of unsigned float
     * @warning : no double parameter as globjects seems to not handle vector of double
     */
    using ScalarsParameter = TParameter<std::vector<float>>;

    /// Parameter of type Vector2
    using Vec2Parameter = TParameter<Core::Vector2>;
    /// Parameter of type Vector3
    using Vec3Parameter = TParameter<Core::Vector3>;
    /// Parameter of type Vector4
    using Vec4Parameter  = TParameter<Core::Vector4>;
    using ColorParameter = TParameter<Core::Utils::Color>;

    /// Parameter of type Matrix2
    using Mat2Parameter = TParameter<Core::Matrix2>;
    /// Parameter of type Matrix3
    using Mat3Parameter = TParameter<Core::Matrix3>;
    /// Parameter of type Matrix3
    using Mat4Parameter = TParameter<Core::Matrix4>;

  public:
    /**
     * Overloaded operators to set shader parameters
     * @{
     */
    void addParameter( const std::string& name, bool value );
    void addParameter( const std::string& name, int value );
    void addParameter( const std::string& name, uint value );
    void addParameter( const std::string& name, Scalar value );

    void addParameter( const std::string& name, std::vector<int> values );
    void addParameter( const std::string& name, std::vector<uint> values );
    void addParameter( const std::string& name, std::vector<float> values );

    void addParameter( const std::string& name, const Core::Vector2& value );
    void addParameter( const std::string& name, const Core::Vector3& value );
    void addParameter( const std::string& name, const Core::Vector4& value );
    void addParameter( const std::string& name, const Core::Utils::Color& value );

    void addParameter( const std::string& name, const Core::Matrix2& value );
    void addParameter( const std::string& name, const Core::Matrix3& value );
    void addParameter( const std::string& name, const Core::Matrix4& value );

    /**
     * Adding a texture parameter.
     * The default (-1) for the texUnit parameter implies automatic uniform binding for the
     * texture unit associated with the named sampler.
     * If texUnit is given, then uniform binding will be made at this explicit location.
     */
    void addParameter( const std::string& name, Data::Texture* tex, int texUnit = -1 );
    /**@}*/
    /***
     * Concatenates two RenderParameters
     * @param params the render parameter to merge with the current.
     */
    void concatParameters( const RenderParameters& params );

    /** Bind the parameter uniform on the shader program
     *
     * @param shader The shader to bind to.
     */
    void bind( const Data::ShaderProgram* shader ) const;

    /**
     * Get a typed parameter set
     * @tparam T the type of the parameter set to get
     * @return The corresponding set parameter
     */
    template <typename T>
    const UniformBindableSet<T>& getParameterSet() const;

  private:
    /**
     * Storage of the parameters
     * @todo : find a way to simplify this (à la Ra::Core::Geometry::AttribArrayGeometry
     */
    ///@{
    UniformBindableSet<BoolParameter> m_boolParamsVector;
    UniformBindableSet<IntParameter> m_intParamsVector;
    UniformBindableSet<UIntParameter> m_uintParamsVector;
    UniformBindableSet<ScalarParameter> m_scalarParamsVector;

    UniformBindableSet<IntsParameter> m_intsParamsVector;
    UniformBindableSet<UIntsParameter> m_uintsParamsVector;
    UniformBindableSet<ScalarsParameter> m_scalarsParamsVector;

    UniformBindableSet<Vec2Parameter> m_vec2ParamsVector;
    UniformBindableSet<Vec3Parameter> m_vec3ParamsVector;
    UniformBindableSet<Vec4Parameter> m_vec4ParamsVector;
    UniformBindableSet<ColorParameter> m_colorParamsVector;

    UniformBindableSet<Mat2Parameter> m_mat2ParamsVector;
    UniformBindableSet<Mat3Parameter> m_mat3ParamsVector;
    UniformBindableSet<Mat4Parameter> m_mat4ParamsVector;

    UniformBindableSet<TextureParameter> m_texParamsVector;
    /**@}*/
};

/**
 * Shader program parameter provider.
 * a ShaderParameterProvider is an object that is associated to a render technique to provide the
 * uniform parameter set for the program. When an RenderObject is drawn using a given
 * rendertechnique, the ShaderParameterProvider associated to the renderTechnique is responsible to
 * set all the uniforms needed by the rendertechnique.
 */
class RA_ENGINE_API ShaderParameterProvider
{
  public:
    virtual ~ShaderParameterProvider() = default;
    virtual RenderParameters& getParameters() { return m_renderParameters; }
    virtual const RenderParameters& getParameters() const { return m_renderParameters; }
    /**
     * Update the OpenGL states used by the ShaderParameterProvider.
     * These state could be the ones from an associated material (textures, precomputed tables or
     * whatever data associated to the material)  or some parameters that are
     * specific to the provider sementic.
     */
    virtual void updateGL() = 0;

    /**
     * Get the list of properties the provider migh use  in a shader.
     * Each property will be added to the shader used for rendering under the form
     * "#define theProperty" when the provider is associated with the render technique.
     *
     * The defaul implementation returns an empty list.
     *
     * @todo : Validate this proposal
     */
    virtual std::list<std::string> getPropertyList() const { return {}; };

  protected:
    /// The parameters to set for a shader
    RenderParameters m_renderParameters;
};

} // namespace Data
} // namespace Engine
} // namespace Ra

#include <Engine/Data/RenderParameters.inl>
