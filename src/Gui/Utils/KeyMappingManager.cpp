#include "KeyMappingManager.hpp"

#include <Core/Resources/Resources.hpp>
#include <Core/Utils/Log.hpp>

#include <QMessageBox>
#include <QtGlobal> //QT_VERSION, QT_VERSION_CHECK

namespace Ra::Gui {

using namespace Core::Utils; // log

KeyMappingManager::KeyMappingManager() :
    m_domDocument( "Key Mapping QDomDocument" ),
    m_metaEnumKey( QMetaEnum::fromType<Qt::Key>() ),
    m_file( nullptr ) {

    auto optionalPath { Core::Resources::getRadiumResourcesPath() };
    auto resourcesRootDir { optionalPath.value_or( "[[Default resrouces path not found]]" ) };

    ///\todo how to check here ?
    m_defaultConfigFile = resourcesRootDir + std::string( "Configs/default.xml" );

    QSettings settings;
    QString keyMappingFilename =
        settings.value( "keymapping/config", QString::fromStdString( m_defaultConfigFile ) )
            .toString();
    if ( !keyMappingFilename.contains( QString::fromStdString( m_defaultConfigFile ) ) )
    {
        LOG( logDEBUG ) << "Loading key mapping " << keyMappingFilename.toStdString() << " (from "
                        << settings.fileName().toStdString() << ")";
    }
    else
    { LOG( logDEBUG ) << "Loading default key mapping " << m_defaultConfigFile; }
    loadConfiguration( keyMappingFilename.toStdString() );
}

KeyMappingManager::KeyMappingAction
KeyMappingManager::getAction( const KeyMappingManager::Context& context,
                              const Qt::MouseButtons& buttons,
                              const Qt::KeyboardModifiers& modifiers,
                              int key,
                              bool wheel ) {

    if ( context.isInvalid() )
    {
        LOG( logDEBUG ) << "try to get action from an invalid context";
        return KeyMappingAction();
    }
    // skip key as modifiers,
    if ( ( key == Qt::Key_Shift ) || ( key == Qt::Key_Control ) || ( key == Qt::Key_Alt ) ||
         ( key == Qt::Key_Meta ) )
    { key = -1; }
    KeyMappingManager::MouseBinding binding { buttons, modifiers, key, wheel };

    auto action = m_mappingAction[context].find( binding );
    if ( action != m_mappingAction[context].end() ) { return action->second; }

    return KeyMappingManager::KeyMappingAction();
}

KeyMappingManager::KeyMappingAction
KeyMappingManager::addAction( const std::string& context,
                              const std::string& keyString,
                              const std::string& modifiersString,
                              const std::string& buttonsString,
                              const std::string& wheelString,
                              const std::string& actionString,
                              bool saveToConfigFile ) {
    auto actionIndex = loadConfigurationMappingInternal(
        context, keyString, modifiersString, buttonsString, wheelString, actionString );
    if ( saveToConfigFile )
    {
        QDomElement domElement   = m_domDocument.documentElement();
        QDomElement elementToAdd = m_domDocument.createElement( "keymap" );
        elementToAdd.setAttribute( "context", context.c_str() );
        elementToAdd.setAttribute( "key", keyString.c_str() );
        elementToAdd.setAttribute( "modifiers", modifiersString.c_str() );
        elementToAdd.setAttribute( "buttons", buttonsString.c_str() );
        if ( !wheelString.empty() ) { elementToAdd.setAttribute( "wheel", wheelString.c_str() ); }
        elementToAdd.setAttribute( "action", actionString.c_str() );

        QString xmlAction;
        QTextStream s( &xmlAction );
        s << elementToAdd;
#if QT_VERSION < QT_VERSION_CHECK( 5, 10, 0 )
        QString xmlActionChopped = xmlAction;
        xmlActionChopped.chop( 1 );
        LOG( logDEBUG ) << "KeyMappingManager : adding The action  "
                        << xmlActionChopped.toStdString();

#else
        LOG( logDEBUG ) << "KeyMappingManager : adding The action  "
                        << xmlAction.chopped( 1 ).toStdString();
#endif

        domElement.appendChild( elementToAdd );
        saveConfiguration();
    }
    return actionIndex;
}

KeyMappingManager::Context KeyMappingManager::addContext( const std::string& contextName ) {
    Ra::Core::Utils::Index contextIndex;

    auto contextItr = m_contextNameToIndex.find( contextName );
    if ( contextItr == m_contextNameToIndex.end() )
    {
        contextIndex                      = m_contextNameToIndex.size();
        m_contextNameToIndex[contextName] = contextIndex;
        m_actionNameToIndex.emplace_back();
        m_mappingAction.emplace_back();

        CORE_ASSERT( m_actionNameToIndex.size() == size_t( contextIndex + 1 ),
                     "Corrupted actionName DB" );
        CORE_ASSERT( m_mappingAction.size() == size_t( contextIndex + 1 ),
                     "Corrupted mappingAction DB" );
    }
    else
        contextIndex = contextItr->second;

    return contextIndex;
}

KeyMappingManager::Context KeyMappingManager::getContext( const std::string& contextName ) {
    // use find so that it do not insert invalid context
    auto itr = m_contextNameToIndex.find( contextName );
    if ( itr != m_contextNameToIndex.end() ) return itr->second;
    return Context {};
}

KeyMappingManager::KeyMappingAction
KeyMappingManager::getActionIndex( const Context& context, const std::string& actionName ) {
    if ( size_t( context ) >= m_actionNameToIndex.size() || context.isInvalid() )
    {
        LOG( logWARNING ) << "try to get action index ( " << actionName
                          << " ) from an invalid context ( " << context << " )";

        return KeyMappingAction {};
    }
    auto itr = m_actionNameToIndex[context].find( actionName );
    if ( itr != m_actionNameToIndex[context].end() ) return itr->second;
    LOG( logWARNING ) << "try to get action index from an invalid action name " << actionName
                      << " (context " << getContextName( context ) << " [" << context << "])";
    LOG( logWARNING ) << "consider add to conf: "
                      << "<keymap context=\"" << getContextName( context )
                      << "\" key=\"\" modifiers=\"\" buttons=\"\" action=\"" << actionName
                      << "\"/>";

    return KeyMappingAction {};
}

std::string KeyMappingManager::getActionName( const Context& context,
                                              const KeyMappingAction& action ) {

    if ( size_t( context ) < m_actionNameToIndex.size() && context.isValid() )
    {

        auto actionFindItr = std::find_if(
            std::begin( m_actionNameToIndex[context] ),
            std::end( m_actionNameToIndex[context] ),
            [&]( const ActionNameMap::value_type& pair ) { return pair.second == action; } );

        if ( actionFindItr != std::end( m_actionNameToIndex[context] ) )
        { return actionFindItr->first; }
    }
    return "Invalid";
}

std::string KeyMappingManager::getContextName( const Context& context ) {
    auto contextFindItr = std::find_if(
        std::begin( m_contextNameToIndex ),
        std::end( m_contextNameToIndex ),
        [&]( const ContextNameMap ::value_type& pair ) { return pair.second == context; } );

    if ( contextFindItr != std::end( m_contextNameToIndex ) ) { return contextFindItr->first; }
    return "Invalid";
}

int KeyMappingManager::addListener( Observer callback ) {
    auto gid = attach( callback );
    // call the registered listener directly to have it up to date if the
    // config is already loaded
    callback();
    return gid;
}

void KeyMappingManager::removeListener( int callbackId ) {
    detach( callbackId );
}

void KeyMappingManager::bindKeyToAction( Ra::Core::Utils::Index contextIndex,
                                         const MouseBinding& binding,
                                         Ra::Core::Utils::Index actionIndex ) {

    CORE_ASSERT( contextIndex < m_contextNameToIndex.size(), "contextIndex is out of range" );

    // search if an action already correspond to this binding.
    auto f = m_mappingAction[contextIndex].find( binding );
    if ( f != m_mappingAction[contextIndex].end() )
    {

        // if yes, search for its name
        auto findResult = std::find_if(
            std::begin( m_actionNameToIndex[contextIndex] ),
            std::end( m_actionNameToIndex[contextIndex] ),
            [&]( const ActionNameMap::value_type& pair ) { return pair.second == actionIndex; } );

        // if the name is not present, something bad happens.
        if ( findResult == std::end( m_actionNameToIndex[contextIndex] ) )
        {
            LOG( logERROR ) << "Corrupted call to bindKeyToAction, index " << actionIndex
                            << " must have been inserted before !\n";
            return;
        }

        // if name is present, find the other action's name
        auto findResult2 = std::find_if(
            std::begin( m_actionNameToIndex[contextIndex] ),
            std::end( m_actionNameToIndex[contextIndex] ),
            [&]( const ActionNameMap::value_type& pair ) { return pair.second == f->second; } );

        // if the name is not present, something bad happens.
        if ( findResult2 == std::end( m_actionNameToIndex[contextIndex] ) )
        {
            LOG( logERROR ) << "Corrupted call to bindKeyToAction, index " << actionIndex
                            << " must have been inserted before !\n";
            return;
        }

        LOG( logWARNING ) << "Binding action " << findResult->first << " to "
                          << "buttons [" << enumNamesFromMouseButtons( binding.m_buttons ) << "] "
                          << "modifiers [" << enumNamesFromKeyboardModifiers( binding.m_modifiers )
                          << "]"
                          << " keycode [" << binding.m_key << "]"
                          << " wheel [" << binding.m_wheel << "]"
                          << ", which is already used for action " << findResult2->first << ".";
    }

    LOG( logDEBUG2 ) << "In context " << getContextName( contextIndex ) << " [" << contextIndex
                     << "]"
                     << " binding action " << getActionName( contextIndex, actionIndex ) << " ["
                     << actionIndex << "]"
                     << " buttons [" << enumNamesFromMouseButtons( binding.m_buttons ) << "]"
                     << " modifiers [" << enumNamesFromKeyboardModifiers( binding.m_modifiers )
                     << "]"
                     << " keycode [" << binding.m_key << "]"
                     << " wheel [" << binding.m_wheel << "]";

    m_mappingAction[contextIndex][binding] = actionIndex;
}

void KeyMappingManager::loadConfiguration( const std::string& inFilename ) {
    // if no filename is given, load default configuration
    std::string filename = inFilename;
    if ( filename.empty() ) { filename = m_defaultConfigFile.c_str(); }

    delete m_file;
    m_file = new QFile( QString::fromStdString( filename ) );

    if ( !m_file->open( QIODevice::ReadOnly ) )
    {
        if ( filename != m_defaultConfigFile )
        {
            LOG( logERROR ) << "Failed to open key mapping configuration file ! "
                            << m_file->fileName().toStdString();
            LOG( logERROR ) << "Trying to load default configuration...";
            loadConfiguration();
            return;
        }
        else
        {
            LOG( logERROR ) << "Failed to open default key mapping configuration file !";
            return;
        }
    }

    if ( !m_domDocument.setContent( m_file ) )
    {
        LOG( logERROR ) << "Can't associate XML file to QDomDocument !";
        LOG( logERROR ) << "Trying to load default configuration...";
        m_file->close();
        loadConfiguration();
        return;
    }

    // Store setting only if not default
    if ( filename != m_defaultConfigFile )
    {
        QSettings settings;
        settings.setValue( "keymapping/config", m_file->fileName() );
    }
    m_file->close();

    loadConfigurationInternal();

    // notify observer that keymapping has changed.
    notify();
}

bool KeyMappingManager::saveConfiguration( const std::string& inFilename ) {
    QString filename { m_file->fileName() };
    if ( !inFilename.empty() ) { filename = QString::fromStdString( inFilename ); }

    QFile saveTo( filename );
    saveTo.open( QIODevice::WriteOnly );
    QXmlStreamWriter stream( &saveTo );
    stream.setAutoFormatting( true );
    stream.setAutoFormattingIndent( 4 );
    stream.setCodec( "ISO 8859-1" );
    stream.writeStartDocument();
    stream.writeComment( "\tRadium KeyMappingManager configuration file\t" );
    stream.writeComment(
        "\n<keymap context=\"thecontext\" action=\"theAction\" buttons=\"QButton\" "
        "modifier=\"QModifier\" key=\"QKey\" wheel=\"boolean\"/>\n" );
    QDomNode root = m_domDocument.documentElement();
    while ( !root.isNull() )
    {
        saveNode( stream, root );
        if ( stream.hasError() ) { break; }
        root = root.nextSibling();
    }

    stream.writeEndDocument();

    if ( stream.hasError() )
    {
        LOG( logERROR ) << "Fail to write Canonical XML.";
        return false;
    }
    return true;
}

void KeyMappingManager::saveNode( QXmlStreamWriter& stream, const QDomNode& domNode ) {
    if ( stream.hasError() ) { return; }

    if ( domNode.isElement() )
    {
        const QDomElement domElement = domNode.toElement();
        if ( !domElement.isNull() )
        {
            auto tagName = domElement.tagName().toStdString();
            stream.writeStartElement( domElement.tagName() );

            if ( tagName == "keymap" )
            {

                auto saveAttrib = [&domElement, &stream]( const QString& attribName,
                                                          const QString& attribDefault,
                                                          bool optional = false ) {
                    QString attribValue = domElement.attribute( attribName, attribDefault );
                    if ( optional && attribValue == attribDefault ) { return false; }
                    stream.writeAttribute( attribName, attribValue );
                    return true;
                };

                if ( !saveAttrib( "context", "" ) )
                {
                    LOG( logERROR ) << "Error, missing context when saving keymap element";
                    return;
                }
                if ( !saveAttrib( "action", "" ) )
                {
                    LOG( logERROR ) << "Error, missing action when saving keymap element";
                    return;
                }
                saveAttrib( "buttons", "" );
                saveAttrib( "modifiers", "" );
                saveAttrib( "key", "" );
                saveAttrib( "wheel", "false", true );
            }
            else
            {
                if ( domElement.hasAttributes() )
                {
                    QMap<QString, QString> attributes;
                    const QDomNamedNodeMap attributeMap = domElement.attributes();
                    for ( int i = 0; i < attributeMap.count(); ++i )
                    {
                        const QDomNode attribute = attributeMap.item( i );
                        attributes.insert( attribute.nodeName(), attribute.nodeValue() );
                    }

                    QMap<QString, QString>::const_iterator i = attributes.constBegin();
                    while ( i != attributes.constEnd() )
                    {
                        stream.writeAttribute( i.key(), i.value() );
                        ++i;
                    }
                }
            }

            if ( domElement.hasChildNodes() )
            {
                QDomNode elementChild = domElement.firstChild();
                while ( !elementChild.isNull() )
                {
                    saveNode( stream, elementChild );
                    elementChild = elementChild.nextSibling();
                }
            }

            stream.writeEndElement();
        }
    }
    else if ( domNode.isComment() )
    { stream.writeComment( domNode.nodeValue() ); }
    else if ( domNode.isText() )
    { stream.writeCharacters( domNode.nodeValue() ); }
}

void KeyMappingManager::loadConfigurationInternal() {
    ///\todo maybe find a better way to handle load and reload.
    /// -> do not clear m_contextNameToIndex m_actionNameToIndex so the keep their index values ...
    m_contextNameToIndex.clear();
    m_actionNameToIndex.clear();
    m_mappingAction.clear();

    QDomElement domElement = m_domDocument.documentElement();

    if ( domElement.tagName() != "keymaps" )
    {
        LOG( logWARNING ) << "No <keymaps> global bounding tag ! Maybe you set a different global "
                             "tag ? (Not a big deal)";
    }

    QDomNode node = domElement.firstChild();
    while ( !node.isNull() )
    {
        if ( !node.isComment() )
        {
            QDomElement nodeElement = node.toElement();
            loadConfigurationTagsInternal( nodeElement );
        }
        node = node.nextSibling();
    }
}

void KeyMappingManager::loadConfigurationTagsInternal( QDomElement& node ) {
    if ( node.tagName() == "keymap" )
    {

        QDomElement e               = node.toElement();
        std::string keyString       = e.attribute( "key", "-1" ).toStdString();
        std::string modifiersString = e.attribute( "modifiers", "NoModifier" ).toStdString();
        std::string buttonsString   = e.attribute( "buttons", "NoButton" ).toStdString();
        std::string contextString   = e.attribute( "context", "AppContext" ).toStdString();
        std::string wheelString     = e.attribute( "wheel", "false" ).toStdString();
        std::string actionString    = e.attribute( "action" ).toStdString();

        loadConfigurationMappingInternal(
            contextString, keyString, modifiersString, buttonsString, wheelString, actionString );
    }
    else
    {
        LOG( logERROR ) << "Unrecognized XML key mapping configuration file tag \""
                        << qPrintable( node.tagName() ) << "\" !";
        LOG( logERROR ) << "Trying to load default configuration...";
        loadConfiguration();

        return;
    }
}

KeyMappingManager::KeyMappingAction
KeyMappingManager::loadConfigurationMappingInternal( const std::string& context,
                                                     const std::string& keyString,
                                                     const std::string& modifiersString,
                                                     const std::string& buttonsString,
                                                     const std::string& wheelString,
                                                     const std::string& actionString ) {

    Ra::Core::Utils::Index contextIndex = addContext( context );

    Ra::Core::Utils::Index actionIndex;
    auto actionItr = m_actionNameToIndex[contextIndex].find( actionString );
    if ( actionItr == m_actionNameToIndex[contextIndex].end() )
    {
        actionIndex                                     = m_actionNameToIndex[contextIndex].size();
        m_actionNameToIndex[contextIndex][actionString] = actionIndex;
    }
    else
    { actionIndex = actionItr->second; }

    Qt::KeyboardModifiers modifiersValue = getQtModifiersValue( modifiersString );
    auto keyValue                        = m_metaEnumKey.keyToValue( keyString.c_str() );
    auto buttonsValue                    = getQtMouseButtonsValue( buttonsString );
    auto wheel                           = wheelString.compare( "true" ) == 0;

    if ( keyValue == -1 && buttonsValue == Qt::NoButton && !wheel )
    {
        LOG( logERROR ) << "No key nor mouse buttons specified for action [" << actionString
                        << "] with key [" << keyString << "], and buttons[" << buttonsString << "]";
    }
    else
    {
        bindKeyToAction( contextIndex,
                         MouseBinding { buttonsValue, modifiersValue, keyValue, wheel },
                         actionIndex );
    }
    return actionIndex;
}

Qt::KeyboardModifiers KeyMappingManager::getQtModifiersValue( const std::string& modifierString ) {
    Qt::KeyboardModifiers modifier = Qt::NoModifier;

    std::istringstream f( modifierString );
    std::string s;

    while ( getline( f, s, ',' ) )
    {
        if ( s == "ShiftModifier" ) { modifier |= Qt::ShiftModifier; }
        else if ( s == "ControlModifier" )
        { modifier |= Qt::ControlModifier; }
        else if ( s == "AltModifier" )
        { modifier |= Qt::AltModifier; }
        else if ( s == "MetaModifier" )
        { modifier |= Qt::MetaModifier; }
        else if ( s == "KeypadModifier" )
        { modifier |= Qt::KeypadModifier; }
        else if ( s == "GroupSwitchModifier" )
        { modifier |= Qt::GroupSwitchModifier; }
    }

    return modifier;
}

Qt::MouseButtons KeyMappingManager::getQtMouseButtonsValue( const std::string& keyString ) {
    Qt::MouseButtons key = Qt::NoButton;

    if ( keyString == "LeftButton" ) { key = Qt::LeftButton; }
    else if ( keyString == "RightButton" )
    { key = Qt::RightButton; }
    else if ( keyString == "MidButton" )
    { key = Qt::MidButton; }
    else if ( keyString == "MiddleButton" )
    { key = Qt::MiddleButton; }
    else if ( keyString == "XButton1" )
    { key = Qt::XButton1; }
    else if ( keyString == "XButton2" )
    { key = Qt::XButton2; }
    ///\todo add other buttons support here
    return key;
}

void KeyMappingManager::reloadConfiguration() {
    QString filename = m_file->fileName();
    loadConfiguration( filename.toStdString().c_str() );
}

KeyMappingManager::~KeyMappingManager() {
    if ( m_file->isOpen() ) { m_file->close(); }
}

std::string KeyMappingManager::getHelpText() {
    //    return m_domDocument.toString().toStdString();
    std::ostringstream text;
    for ( const auto& context : m_contextNameToIndex )
    {
        std::string contextString { context.first };
        auto end      = contextString.find( "Context" );
        contextString = contextString.substr( 0, end );
        text << "<h2>" << contextString << "</h2>\n";

        text << "<table>\n";
        for ( const auto& action : m_mappingAction[context.second] )
        {
            const auto& binding     = action.first;
            const auto& actionIndex = action.second;
            const auto& actionNames = m_actionNameToIndex[context.second];
            const auto& actionName =
                std::find_if( actionNames.begin(),
                              actionNames.end(),
                              [&actionIndex]( const auto& a ) { return a.second == actionIndex; } );
            if ( actionName != actionNames.end() )
            {
                text << "<tr><td>";
                text << "<b>" << actionName->first << "</b>";
                text << "</td><td>";
                if ( binding.m_buttons != Qt::NoButton )
                    text << "[" << enumNamesFromMouseButtons( binding.m_buttons ) << "] ";
                if ( binding.m_wheel ) text << " [Wheel] ";
                if ( binding.m_modifiers != Qt::NoModifier )
                {

                    std::string modifierString { "Modifier" };
                    auto modifiers = enumNamesFromKeyboardModifiers( binding.m_modifiers );
                    auto found     = modifiers.find( modifierString );
                    while ( found != std::string::npos )
                    {
                        modifiers.erase( found, modifierString.length() );
                        found = modifiers.find( modifierString, found );
                    }
                    text << "[" << modifiers << "] ";
                }
                if ( binding.m_key != -1 )
                    text << "[key: " << QKeySequence( binding.m_key ).toString().toStdString()
                         << "] ";
                text << "</td></tr>\n";
            }
        }
        text << "</table>\n";
    }
    return text.str();
}

#define TEST_BUTTON_STRING( BUTTON ) \
    if ( buttons & Qt::BUTTON )      \
    {                                \
        returnText += sep + #BUTTON; \
        sep = ",";                   \
    }

std::string KeyMappingManager::enumNamesFromMouseButtons( const Qt::MouseButtons& buttons ) {
    std::string returnText;
    std::string sep;

    if ( buttons == Qt::NoButton ) return "NoButton";

    TEST_BUTTON_STRING( LeftButton );
    TEST_BUTTON_STRING( RightButton );
    TEST_BUTTON_STRING( MiddleButton );
    TEST_BUTTON_STRING( BackButton );
    TEST_BUTTON_STRING( ForwardButton );
    TEST_BUTTON_STRING( TaskButton );
    TEST_BUTTON_STRING( ExtraButton4 );
    TEST_BUTTON_STRING( ExtraButton5 );
    TEST_BUTTON_STRING( ExtraButton6 );
    TEST_BUTTON_STRING( ExtraButton7 );
    TEST_BUTTON_STRING( ExtraButton8 );
    TEST_BUTTON_STRING( ExtraButton9 );
    TEST_BUTTON_STRING( ExtraButton10 );
    TEST_BUTTON_STRING( ExtraButton11 );
    TEST_BUTTON_STRING( ExtraButton12 );
    TEST_BUTTON_STRING( ExtraButton13 );
    TEST_BUTTON_STRING( ExtraButton14 );
    TEST_BUTTON_STRING( ExtraButton15 );
    TEST_BUTTON_STRING( ExtraButton16 );
    TEST_BUTTON_STRING( ExtraButton17 );
    TEST_BUTTON_STRING( ExtraButton18 );
    TEST_BUTTON_STRING( ExtraButton19 );
    TEST_BUTTON_STRING( ExtraButton20 );
    TEST_BUTTON_STRING( ExtraButton21 );
    TEST_BUTTON_STRING( ExtraButton22 );
    TEST_BUTTON_STRING( ExtraButton23 );
    TEST_BUTTON_STRING( ExtraButton24 );

    return returnText;
}

std::string
KeyMappingManager::enumNamesFromKeyboardModifiers( const Qt::KeyboardModifiers& buttons ) {
    std::string returnText;
    std::string sep;

    if ( buttons == Qt::NoModifier ) return "NoModifier";

    TEST_BUTTON_STRING( ShiftModifier );
    TEST_BUTTON_STRING( ControlModifier );
    TEST_BUTTON_STRING( AltModifier );
    TEST_BUTTON_STRING( MetaModifier );
    TEST_BUTTON_STRING( KeypadModifier );
    TEST_BUTTON_STRING( GroupSwitchModifier );

    return returnText;
}

#undef TEST_BUTTON_STRING

RA_SINGLETON_IMPLEMENTATION( KeyMappingManager );
} // namespace Ra::Gui
