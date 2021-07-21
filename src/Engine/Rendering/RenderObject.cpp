#include <Engine/Rendering/RenderObject.hpp>

#include <Engine/RadiumEngine.hpp>

#include <Engine/Data/DisplayableObject.hpp>
#include <Engine/Data/Material.hpp>
#include <Engine/Data/RenderParameters.hpp>
#include <Engine/OpenGL.hpp>
#include <Engine/Rendering/RenderObjectManager.hpp>
#include <Engine/Scene/Component.hpp>

// STRANGE : only needed to access to the entity and its transform for components --> refactor
// component to give this directly ?
#include <Engine/Scene/Entity.hpp>

#include <Core/Containers/MakeShared.hpp>
#include <Engine/Data/ShaderProgram.hpp>
#include <Engine/Data/ViewingParameters.hpp>

namespace Ra {
namespace Engine {
namespace Rendering {

RenderObject::RenderObject( const std::string& name,
                            Scene::Component* comp,
                            const RenderObjectType& type,
                            int lifetime ) :
    IndexedObject(),
    m_component { comp },
    m_name { name },
    m_type { type },
    m_mesh { nullptr },
    m_lifetime { lifetime },
    m_hasLifetime { lifetime > 0 } {}

RenderObject::~RenderObject() {
    if ( m_mesh ) {
        m_mesh->getAbstractGeometry().getAabbObservable().detach( m_aabbObserverIndex );
    }
}

RenderObject* RenderObject::createRenderObject( const std::string& name,
                                                Scene::Component* comp,
                                                const RenderObjectType& type,
                                                std::shared_ptr<Data::Displayable> mesh,
                                                const RenderTechnique& techniqueConfig ) {
    auto obj = new RenderObject( name, comp, type );
    obj->setMesh( mesh );
    obj->setVisible( true );
    auto rt = Core::make_shared<RenderTechnique>( techniqueConfig );
    obj->setRenderTechnique( rt );
    return obj;
}

void RenderObject::updateGL() {
    // Do not update while we are cloning
    std::lock_guard<std::mutex> lock( m_updateMutex );

    if ( m_renderTechnique ) { m_renderTechnique->updateGL(); }

    if ( m_mesh ) { m_mesh->updateGL(); }

    m_dirty = false;
}

const RenderObjectType& RenderObject::getType() const {
    return m_type;
}

void RenderObject::setType( const RenderObjectType& t ) {
    m_type = t;
}

const std::string& RenderObject::getName() const {
    return m_name;
}

void RenderObject::setVisible( bool visible ) {
    m_visible = visible;
}

void RenderObject::toggleVisible() {
    m_visible = !m_visible;
}

bool RenderObject::isVisible() const {
    return m_visible;
}

void RenderObject::setPickable( bool pickable ) {
    m_pickable = pickable;
}

void RenderObject::togglePickable() {
    m_pickable = !m_pickable;
}

bool RenderObject::isPickable() const {
    return m_pickable;
}

void RenderObject::setXRay( bool xray ) {
    m_xray = xray;
}

void RenderObject::toggleXRay() {
    m_xray = !m_xray;
}

bool RenderObject::isXRay() const {
    return m_xray;
}

void RenderObject::setTransparent( bool transparent ) {
    m_transparent = transparent;
}

void RenderObject::toggleTransparent() {
    m_transparent = !m_transparent;
}

bool RenderObject::isTransparent() const {
    return m_transparent;
}

bool RenderObject::isDirty() const {
    return m_dirty;
}

const Scene::Component* RenderObject::getComponent() const {
    return m_component;
}

Scene::Component* RenderObject::getComponent() {
    return m_component;
}

void RenderObject::setRenderTechnique( std::shared_ptr<RenderTechnique> technique ) {
    CORE_ASSERT( technique, "Passing a nullptr as render technique" );
    m_renderTechnique = technique;
}

std::shared_ptr<const RenderTechnique> RenderObject::getRenderTechnique() const {
    return m_renderTechnique;
}

std::shared_ptr<RenderTechnique> RenderObject::getRenderTechnique() {
    return m_renderTechnique;
}

void RenderObject::setMaterial( std::shared_ptr<Data::Material> material ) {
    m_material = material;
}

std::shared_ptr<const Data::Material> RenderObject::getMaterial() const {
    return m_material;
}

std::shared_ptr<Data::Material> RenderObject::getMaterial() {
    return m_material;
}

void RenderObject::setMesh( std::shared_ptr<Data::Displayable> mesh ) {

    if ( m_mesh ) {
        m_mesh->getAbstractGeometry().getAabbObservable().detach( m_aabbObserverIndex );
    }

    m_mesh = mesh;
    if ( m_mesh ) {
        m_aabbObserverIndex = m_mesh->getAbstractGeometry().getAabbObservable().attach(
            [this]() { this->invalidateAabb(); } );
        invalidateAabb();
    }
}

std::shared_ptr<const Data::Displayable> RenderObject::getMesh() const {
    return m_mesh;
}

const std::shared_ptr<Data::Displayable>& RenderObject::getMesh() {
    return m_mesh;
}

Core::Transform RenderObject::getTransform() const {
    return m_component->getEntity()->getTransform() * m_localTransform;
}

Core::Matrix4 RenderObject::getTransformAsMatrix() const {
    return getTransform().matrix();
}

Core::Aabb RenderObject::computeAabb() {
    if ( !m_isAabbValid ) {
        auto aabb = m_mesh->getAbstractGeometry().computeAabb();
        if ( !aabb.isEmpty() ) {

            Core::Aabb result;
            for ( int i = 0; i < 8; ++i ) {
                result.extend( getTransform() * aabb.corner( Core::Aabb::CornerType( i ) ) );
            }

            m_aabb        = result;
            m_isAabbValid = true;
        }
        else {
            m_aabb        = aabb;
            m_isAabbValid = true;
        }
    }

    return m_aabb;
}

void RenderObject::setLocalTransform( const Core::Transform& transform ) {
    m_localTransform = transform;
    invalidateAabb();
}

void RenderObject::setLocalTransform( const Core::Matrix4& transform ) {
    m_localTransform = Core::Transform( transform );
    invalidateAabb();
}

const Core::Transform& RenderObject::getLocalTransform() const {
    return m_localTransform;
}

const Core::Matrix4& RenderObject::getLocalTransformAsMatrix() const {
    return m_localTransform.matrix();
}

void RenderObject::hasBeenRenderedOnce() {
    if ( m_hasLifetime ) {
        if ( --m_lifetime <= 0 ) {
            RadiumEngine::getInstance()->getRenderObjectManager()->renderObjectExpired( m_idx );
        }
    }
}

void RenderObject::hasExpired() {
    m_component->notifyRenderObjectExpired( m_idx );
}

void RenderObject::setLifetime( int t ) {
    m_lifetime    = t;
    m_hasLifetime = true;
}

void RenderObject::render( const Data::RenderParameters& lightParams,
                           const Data::ViewingParameters& viewParams,
                           const Data::ShaderProgram* shader,
                           const Data::RenderParameters& shaderParams ) {
    if ( !m_visible || !shader ) { return; }
    // Radium V2 : avoid this temporary
    Core::Matrix4 modelMatrix  = getTransformAsMatrix();
    Core::Matrix4 normalMatrix = modelMatrix.inverse().transpose();
    // bind data
    shader->bind();
    shader->setUniform( "transform.proj", viewParams.projMatrix );
    shader->setUniform( "transform.view", viewParams.viewMatrix );
    shader->setUniform( "transform.model", modelMatrix );
    shader->setUniform( "transform.worldNormal", normalMatrix );
    lightParams.bind( shader );
    shaderParams.bind( shader );
    // FIXME : find another solution for FrontFacing polygons (depends on the camera matrix)
    // This is a hack to allow correct face culling
    // Note that this hack implies the inclusion of OpenGL.h in this file
    if ( viewParams.viewMatrix.determinant() < 0 ) { glFrontFace( GL_CW ); }
    else {
        glFrontFace( GL_CCW );
    }
    m_mesh->render( shader );
}

void RenderObject::render( const Data::RenderParameters& lightParams,
                           const Data::ViewingParameters& viewParams,
                           Core::Utils::Index passId ) {
    if ( m_visible ) {
        auto shader = getRenderTechnique()->getShader( passId );
        if ( !shader ) { return; }

        auto paramsProvider = getRenderTechnique()->getParametersProvider( passId );
        render( lightParams,
                viewParams,
                shader,
                paramsProvider ? paramsProvider->getParameters() : Data::RenderParameters() );
    }
}

void RenderObject::invalidateAabb() {
    m_isAabbValid = false;
    m_component->invalidateAabb();
}

} // namespace Rendering
} // namespace Engine
} // namespace Ra
