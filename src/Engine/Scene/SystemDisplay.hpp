#pragma once

#include <Engine/RaEngine.hpp>

/// This file's main purpose is to declare a set of macros to allow drawing
/// primitives from everywhere in the code, for debug purposes.
/// These macros can be completely disabled by #defining the following constant :

// define RA_DISABLE_DEBUG_DISPLAY

#include <Core/Utils/Color.hpp>
#include <Core/Utils/Singleton.hpp>

#include <Engine/Scene/Component.hpp>
#include <Engine/Scene/Entity.hpp>

#ifndef RA_DISABLE_DEBUG_DISPLAY
#    include <Engine/Data/DrawPrimitives.hpp>
#endif

namespace Ra {
namespace Engine {
namespace Scene {

#ifndef RA_DISABLE_DEBUG_DISPLAY

/// Component for debug drawing. @see SystemEntity.
class RA_ENGINE_API DebugComponent : public Component
{
  public:
    explicit DebugComponent( Entity* entity ) : Component( "Debug", entity ) {}

    void initialize() override {};

    /// Access render object through RO manager
    Rendering::RenderObject* getRenderObject( Core::Utils::Index idx );
};

#endif

/// Component for UI drawing. @see SystemEntity.
class RA_ENGINE_API UiComponent : public Component
{
  public:
    explicit UiComponent( Entity* entity ) : Component( "UI", entity ) {}

    void initialize() override {}
};

/// This entity allows to add UI and debug drawables from everywhere in the code.
/// It should have only one component and its transform should not change.
class RA_ENGINE_API SystemEntity : public Entity
{
    RA_SINGLETON_INTERFACE( SystemEntity );

  public:
    SystemEntity();

    ~SystemEntity() override = default;

#ifndef RA_DISABLE_DEBUG_DISPLAY
    /// Access the debug component
    static DebugComponent* dbgCmp();

#endif
    /// Access the UI Component
    static UiComponent* uiCmp();
};

} // namespace Scene
} // namespace Engine
} // namespace Ra

#ifndef RA_DISABLE_DEBUG_DISPLAY
/// Macros for debug drawing. All coordinates are in world space.
#    define RA_DISPLAY_POINT( p, color, scale )                     \
        Ra::Engine::Scene::SystemEntity::dbgCmp()->addRenderObject( \
            Ra::Engine::Data::DrawPrimitives::Primitive(            \
                Ra::Engine::Scene::SystemEntity::dbgCmp(),          \
                Ra::Engine::Data::DrawPrimitives::Point( p, color, scale ) ) )

#    define RA_DISPLAY_VECTOR( p, v, color )                        \
        Ra::Engine::Scene::SystemEntity::dbgCmp()->addRenderObject( \
            Ra::Engine::Data::DrawPrimitives::Primitive(            \
                Ra::Engine::Scene::SystemEntity::dbgCmp(),          \
                Ra::Engine::Data::DrawPrimitives::Vector( p, v, color ) ) )

#    define RA_DISPLAY_RAY( r, color )                              \
        Ra::Engine::Scene::SystemEntity::dbgCmp()->addRenderObject( \
            Ra::Engine::Data::DrawPrimitives::Primitive(            \
                Ra::Engine::Scene::SystemEntity::dbgCmp(),          \
                Ra::Engine::Data::DrawPrimitives::Ray( r, color ) ) )

#    define RA_DISPLAY_CIRCLE( c, n, r, color )                     \
        Ra::Engine::Scene::SystemEntity::dbgCmp()->addRenderObject( \
            Ra::Engine::Data::DrawPrimitives::Primitive(            \
                Ra::Engine::Scene::SystemEntity::dbgCmp(),          \
                Ra::Engine::Data::DrawPrimitives::Circle( c, n, r, 20, color ) ) )

#    define RA_DISPLAY_TRIANGLE( a, b, c, color )                   \
        Ra::Engine::Scene::SystemEntity::dbgCmp()->addRenderObject( \
            Ra::Engine::Data::DrawPrimitives::Primitive(            \
                Ra::Engine::SystemEntity::dbgCmp(),                 \
                Ra::Engine::Data::DrawPrimitives::Triangle( a, b, c, color ) ) )

#    define RA_DISPLAY_NORMAL( p, n, color, scale )                 \
        Ra::Engine::Scene::SystemEntity::dbgCmp()->addRenderObject( \
            Ra::Engine::Scene::DrawPrimitives::Primitive(           \
                Ra::Engine::Scene::SystemEntity::dbgCmp(),          \
                Ra::Engine::Scene::DrawPrimitives::Normal( p, n, color, scale ) ) )

#    define RA_DISPLAY_FRAME( t, scale )                            \
        Ra::Engine::Scene::SystemEntity::dbgCmp()->addRenderObject( \
            Ra::Engine::Data::DrawPrimitives::Primitive(            \
                Ra::Engine::Scene::SystemEntity::dbgCmp(),          \
                Ra::Engine::Data::DrawPrimitives::Frame( t, scale ) ) )

#    define RA_DISPLAY_AABB( a, color )                             \
        Ra::Engine::Scene::SystemEntity::dbgCmp()->addRenderObject( \
            Ra::Engine::Data::DrawPrimitives::Primitive(            \
                Ra::Engine::Scene::SystemEntity::dbgCmp(),          \
                Ra::Engine::Data::DrawPrimitives::AABB( a, color ) ) )

#    define RA_DISPLAY_OBB( a, color )                              \
        Ra::Engine::Scene::SystemEntity::dbgCmp()->addRenderObject( \
            Ra::Engine::Data::DrawPrimitives::Primitive(            \
                Ra::Engine::Scene::SystemEntity::dbgCmp(),          \
                Ra::Engine::Data::DrawPrimitives::OBB( a, color ) ) )

#    define RA_DISPLAY_SPHERE( c, r, color )                        \
        Ra::Engine::Scene::SystemEntity::dbgCmp()->addRenderObject( \
            Ra::Engine::Data::DrawPrimitives::Primitive(            \
                Ra::Engine::Scene::SystemEntity::dbgCmp(),          \
                Ra::Engine::Data::DrawPrimitives::Sphere( c, r, color ) ) )

#    define RA_DISPLAY_CAPSULE( p1, p2, r, color )                  \
        Ra::Engine::Scene::SystemEntity::dbgCmp()->addRenderObject( \
            Ra::Engine::Data::DrawPrimitives::Primitive(            \
                Ra::Engine::Scene::SystemEntity::dbgCmp(),          \
                Ra::Engine::Data::DrawPrimitives::Capsule( p1, p2, r, color ) ) )

#    define RA_DISPLAY_LINE( a, b, color )                          \
        Ra::Engine::Scene::SystemEntity::dbgCmp()->addRenderObject( \
            Ra::Engine::Data::DrawPrimitives::Primitive(            \
                Ra::Engine::Scene::SystemEntity::dbgCmp(),          \
                Ra::Engine::Data::DrawPrimitives::Line( a, b, color ) ) )

#    define RA_CLEAR_DEBUG_DISPLAY()                                \
        {                                                           \
            auto cmp = Ra::Engine::Scene::SystemEntity::dbgCmp();   \
            while ( cmp->m_renderObjects.size() )                   \
            { cmp->removeRenderObject( cmp->m_renderObjects[0] ); } \
        }

#else // if debug display is disabled

#    define RA_DISPLAY_POINT( p, color, scale )     // Nothing
#    define RA_DISPLAY_VECTOR( p, v, color )        // ...
#    define RA_DISPLAY_RAY( r, color )              // ...
#    define RA_DISPLAY_CIRCLE( c, n, r, color )     // ...
#    define RA_DISPLAY_TRIANGLE( a, b, c, color )   // ...
#    define RA_DISPLAY_NORMAL( p, n, color, scale ) // ...
#    define RA_DISPLAY_FRAME( t, scale )            // ...
#    define RA_DISPLAY_AABB( a, color )             // ...
#    define RA_DISPLAY_OBB( a, color )              // ...
#    define RA_DISPLAY_SPHERE( c, r, color )        // ...
#    define RA_DISPLAY_CAPSULE( p1, p2, r, color )  // ...
#    define RA_DISPLAY_LINE( a, b, color )          // ...
#    define RA_CLEAR_DEBUG_DISPLAY()                // ...

#endif //! defined DISABLED_DEBUG_DISPLAY
