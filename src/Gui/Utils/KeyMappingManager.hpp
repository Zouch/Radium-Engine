#pragma once

#include <QXmlStreamWriter>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtXml/QtXml>

#include <Gui/RaGui.hpp>

#include <Core/Utils/Index.hpp>
#include <Core/Utils/Log.hpp>
#include <Core/Utils/Observable.hpp>
#include <Core/Utils/Singleton.hpp>

namespace Ra {
namespace Gui {
/// An utility class used to map a (combination) of key / modifier to a specific action.
/// It can load configuration from a file or if no config is found it will load an
/// internal version of the default configuration.
class RA_GUI_API KeyMappingManager : public Ra::Core::Utils::ObservableVoid
{
    RA_SINGLETON_INTERFACE( KeyMappingManager );

    // Needed by Q_ENUM
    Q_GADGET

  public:
    using KeyMappingAction = Ra::Core::Utils::Index;
    using Context          = Ra::Core::Utils::Index;

    /// load configuration from filename, or default configration filename. It
    /// calls the listener callback then.
    void loadConfiguration( const std::string& filename = {} );

    /// Save the configuration
    /// @param filename the file to write to. It will be replaced
    /// @return true if file was correctly saved
    bool saveConfiguration( const std::string& filename = {} );

    /// reload last open file.
    void reloadConfiguration();

    std::string getLoadedFilename() { return m_file->fileName().toStdString(); }

    /// Return the action associated to the binding buttons + modifiers + key
    /// \param context is the context to get action from (e.g. camera or gizmo)
    /// \param buttons are the mouse buttons pressed, could be NoButton
    /// \param modifiers are the keyboard modifiers, could be NoModifiers
    /// \param key is the key pressed, could be -1
    /// \param wheel specifies if we consider a wheel event
    KeyMappingManager::KeyMappingAction getAction( const KeyMappingManager::Context& context,
                                                   const Qt::MouseButtons& buttons,
                                                   const Qt::KeyboardModifiers& modifiers,
                                                   int key,
                                                   bool wheel = false );

    /// \brief Add a given action within a possibly non existing context (also created in this case)
    /// to the mapping system.
    /// This allow to define default behavior when some KeyMappingManageable object is not
    /// parameterized in the application config file. The action can be added to the current config
    /// file so that it will remain for subsequent usage. \param context the context of the action
    /// \param keyString represents the key that needs to be pressed to trigger the event
    /// (ie Key_Z, for example), "" or "-1" corresponds to no key needed.
    /// \param modifiersString represents the modifier used along with key or mouse button `
    /// (needs to be a Qt::Modifier enum value) to trigger the action. Multiples modifiers can be
    /// specified, separated by commas as in "ControlModifier,ShiftModifier".
    /// \param buttonsString represents the button to trigger the event (e.g. LeftButton).
    /// \param wheelString if true, it's a wheel event !
    /// \param actionString represents the KeyMappingAction enum's value you want to
    /// trigger.
    /// \param saveToConfigFile request to save the action on the config file (true by default).
    KeyMappingManager::KeyMappingAction addAction( const std::string& context,
                                                   const std::string& keyString,
                                                   const std::string& modifiersString,
                                                   const std::string& buttonsString,
                                                   const std::string& wheelString,
                                                   const std::string& actionString,
                                                   bool saveToConfigFile = true );

    /// \brief Creates the context index for the given context name.
    /// If the context already exist, return the existing index. If not, the context is created
    /// and its index is returned.
    /// \param contextName the name of the context
    /// \return a valid context index for the given context name.
    Context addContext( const std::string& contextName );

    /// Return the context index corresponding to contextName
    /// \param contextName the name of the context
    /// \return an invalid context if contextName has not been created
    Context getContext( const std::string& contextName );

    /// Return the action index corresponding to a context index and actionName
    /// \param context the index of the context
    /// \param actionName the name of the action
    /// \return an invalid action if context is not valid, or if actionName has  not been created.
    /// (i,e action.isInvalid())
    KeyMappingAction getActionIndex( const Context& context, const std::string& actionName );

    /// \return Action name if context index and action index are valid, "Invalid" otherwise
    std::string getActionName( const Context& context, const KeyMappingAction& action );

    /// \return Context name if context index is valid, "Invalid" otherwise
    std::string getContextName( const Context& context );

    /// Add a callback, triggered when configuration is load or reloaded.
    int addListener( Observable::Observer callback );

    /// Remove a callback. To be called when the related Context/Actions are no more needed.
    /// @param callbackId the Id, returned by addListener, of the Observer to be removed.
    void removeListener( int callbackId );

    /// Return a string corresponding to the current dom document.
    std::string getHelpText();

    /// return a string of enum names from mouse buttons, comma separated,
    /// without space
    static std::string enumNamesFromMouseButtons( const Qt::MouseButtons& buttons );

    /// return a string of enum names from keyboard modifiers, comma separated,
    /// without space
    static std::string enumNamesFromKeyboardModifiers( const Qt::KeyboardModifiers& modifiers );

  private:
    KeyMappingManager();
    ~KeyMappingManager();

    /// Save an XML node that describes an event/action.
    void saveNode( QXmlStreamWriter& stream, const QDomNode& domNode );

    // Private for now, but may need to be public if we want to customize keymapping configuration
    // otherwise than by editing the XML configuration file.
    class MouseBinding
    {
      public:
        explicit MouseBinding( Qt::MouseButtons buttons,
                               Qt::KeyboardModifiers modifiers = Qt::NoModifier,
                               int key                         = -1,
                               bool wheel                      = false ) :

            m_buttons { buttons }, m_modifiers { modifiers }, m_key { key }, m_wheel { wheel } {}
        bool operator<( const MouseBinding& b ) const {
            return ( m_buttons < b.m_buttons ) ||
                   ( ( m_buttons == b.m_buttons ) && ( m_modifiers < b.m_modifiers ) ) ||
                   ( ( m_buttons == b.m_buttons ) && ( m_modifiers == b.m_modifiers ) &&
                     ( m_key < b.m_key ) ) ||
                   ( ( m_buttons == b.m_buttons ) && ( m_modifiers == b.m_modifiers ) &&
                     ( m_key == b.m_key ) && ( m_wheel < b.m_wheel ) );
        }

        //  private:
        Qt::MouseButtons m_buttons;
        Qt::KeyboardModifiers m_modifiers;
        // only one key
        int m_key;
        bool m_wheel;
    };

    /// bind binding to actionIndex, in contextIndex. If replace previously
    /// binded action, with a warning if binding was alreasly present.,
    void bindKeyToAction( Ra::Core::Utils::Index contextIndex,
                          const MouseBinding& binding,
                          Ra::Core::Utils::Index actionIndex );

    void loadConfigurationInternal();
    void loadConfigurationTagsInternal( QDomElement& node );
    KeyMappingAction loadConfigurationMappingInternal( const std::string& context,
                                                       const std::string& keyString,
                                                       const std::string& modifiersString,
                                                       const std::string& buttonsString,
                                                       const std::string& wheelString,
                                                       const std::string& actionString );

    /// Return KeyboardModifiers described in modifierString, multiple modifiers
    /// are comma separated in the modifiers string, as in
    /// "ShiftModifier,AltModifier". This function do note trim any white space.
    static Qt::KeyboardModifiers getQtModifiersValue( const std::string& modifierString );

    /// Return MouseButtons desribed in buttonString, multiple modifiers
    /// are comma separated in the modifiers string, \note only one button is
    /// supported for the moment.
    static Qt::MouseButtons getQtMouseButtonsValue( const std::string& buttonsString );

  private:
    std::string m_defaultConfigFile;
    // For XML parsing using Qt
    QDomDocument m_domDocument;
    QMetaEnum m_metaEnumAction;
    QMetaEnum m_metaEnumKey;
    QFile* m_file;

    using MouseBindingMapping = std::map<MouseBinding, KeyMappingAction>;
    using ContextNameMap      = std::map<std::string, Context>;
    using ActionNameMap       = std::map<std::string, Ra::Core::Utils::Index>;

    ContextNameMap m_contextNameToIndex;              ///< context string give index
    std::vector<ActionNameMap> m_actionNameToIndex;   ///< one element per context
    std::vector<MouseBindingMapping> m_mappingAction; ///< one element per context
};

/// KeyMappingManageable decorate, typical use as a CRTP :
/// class MyClass : public KeyMappingManageable<MyClass> { [...]
/// it defines a static class member m_keyMappingContext, readable with
/// getContext()
/// This context index must be registered by the class to the KeyMappingManager
/// with KeyMappingManager::getContext("MyClassContextName"); after a
/// configuration file is loaded.
template <typename T>
class KeyMappingManageable
{
  public:
    static inline KeyMappingManager::Context getContext() { return m_keyMappingContext; }

    /// KeyManageable class should implement static configureKeyMapping_impl to get their
    /// keyMappingAction constants.
    static inline void configureKeyMapping() { T::configureKeyMapping_impl(); }

  protected:
    T& self() { return static_cast<T&>( *this ); }
    static inline void setContext( const KeyMappingManager::Context& c ) {
        m_keyMappingContext = c;
    }

  private:
    static KeyMappingManager::Context m_keyMappingContext;
};

/// create one m_keyMappingContext by template type.
template <typename T>
KeyMappingManager::Context KeyMappingManageable<T>::m_keyMappingContext;

} // namespace Gui
} // namespace Ra
