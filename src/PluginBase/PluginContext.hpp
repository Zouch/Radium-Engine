#pragma once

#include <PluginBase/RaPluginBase.hpp>
#include <QObject>

namespace Ra {
namespace Engine {
class RadiumEngine;
}
namespace Gui {
class SelectionManager;
class Timeline;
} // namespace Gui
namespace Gui {
class PickingManager;
class Viewer;
} // namespace Gui

namespace Plugins {

/// Data passed to the plugin constructor.
class RA_PLUGINBASE_API Context : public QObject
{
    Q_OBJECT

  public:
    explicit Context( QObject* parent = nullptr ) : QObject( parent ) {}

    Engine::RadiumEngine* m_engine { nullptr };
    Gui::SelectionManager* m_selectionManager { nullptr };
    Gui::Timeline* m_timeline { nullptr };
    Gui::PickingManager* m_pickingManager { nullptr };
    Gui::Viewer* m_viewer { nullptr };
    std::string m_exportDir { "." };

  signals:
    /*!
     * \brief enable continuous rendering update
     * \see BaseApplication::setContinuousUpdate
     *
     * Usage: in your plugin, declare signals, and connect them to the PluginContext signals for
     * forwarding:
     * \code
     * class MyPlugin : public QObject, Ra::Plugins::RadiumPluginInterface
     * {
     *   void MyPlugin::registerPlugin( const Ra::Plugins::Context& context ) {
     *     connect( this, &MyPlugin::askForUpdate, &context, &Ra::Plugins::Context::askForUpdate );
     *     connect( this, &MyPlugin::setContinuousUpdate, &context,
     * &Ra::Plugins::Context::setContinuousUpdate );
     *   }
     *
     * signals:
     *   void setContinuousUpdate( bool b );
     *   void askForUpdate();
     * };
     * \endcode
     */
    void setContinuousUpdate( bool b );

    /// \brief ask for single-shot rendering update
    /// \see BaseApplication::askForUpdate
    /// \see setContinuousUpdate for usage example
    void askForUpdate();
};
} // namespace Plugins
} // namespace Ra
