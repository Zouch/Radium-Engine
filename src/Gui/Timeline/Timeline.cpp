#include "ui_Timeline.h"
#include <Gui/Timeline/Timeline.hpp>

#include <QDesktopWidget>
#include <QEvent>
#include <QMessageBox>
#include <QPainter>
#include <QTimer>
#include <QWheelEvent>

#include <Core/Animation/KeyFramedValueInterpolators.hpp>
#include <Core/Utils/Log.hpp>

#include <Engine/RadiumEngine.hpp>
#include <Engine/Rendering/RenderObject.hpp>
#include <Engine/Rendering/RenderObjectManager.hpp>
#include <Engine/Scene/Component.hpp>
#include <Engine/Scene/Entity.hpp>
#include <Engine/Scene/SignalManager.hpp>

#include <Gui/Timeline/HelpDialog.hpp>

using namespace Ra::Core::Utils;

namespace Ra::Gui {

Timeline::Timeline( QWidget* parent ) : QDialog( parent ), ui( new Ui::Timeline ) {
    ui->setupUi( this );

    // first draw of ruler with current width (default in Timeline.ui) of dialog
    ui->m_scrollArea->onDrawRuler( width() - 2 ); // left/right border width = 2 *1 pixel

    // --- SET INTERNAL REFERENCES ---
    ui->frame_timescale->setScrollArea( ui->m_scrollArea );
    ui->frame_selector->setTimelineUi( ui );

    // --- CREATE INTERNAL CONNECTIONS ---
    connect( ui->frame_selector,
             &TimelineFrameSelector::cursorChanged,
             this,
             &Timeline::updateKeyFrames );
    connect( ui->frame_selector,
             &TimelineFrameSelector::keyFrameAdded,
             this,
             &Timeline::onAddingKeyFrame );
    connect( ui->frame_selector,
             &TimelineFrameSelector::keyFrameDeleted,
             this,
             &Timeline::onRemovingKeyFrame );
    connect( ui->frame_selector,
             &TimelineFrameSelector::keyFrameChanged,
             this,
             &Timeline::onChangingKeyFrame );
    connect( ui->frame_selector,
             &TimelineFrameSelector::keyFrameMoved,
             this,
             &Timeline::onMovingKeyFrame );
    connect( ui->frame_selector,
             &TimelineFrameSelector::keyFramesMoved,
             this,
             &Timeline::onMovingKeyFrames );

    // --- CONNECT INTERNAL SIGNALS TO EXTERNAL SIGNALS ---
    connect( ui->toolButton_playPause, &QToolButton::toggled, this, &Timeline::playClicked );
    connect(
        ui->frame_selector, &TimelineFrameSelector::cursorChanged, this, &Timeline::cursorChanged );
    connect(
        ui->frame_selector, &TimelineFrameSelector::startChanged, this, &Timeline::startChanged );
    connect( ui->frame_selector, &TimelineFrameSelector::endChanged, this, &Timeline::endChanged );
    connect( ui->frame_selector,
             &TimelineFrameSelector::durationChanged,
             this,
             &Timeline::durationChanged );

    // --- DEAL WITH OBJET REMOVAL ---
    auto signalManager = Ra::Engine::RadiumEngine::getInstance()->getSignalManager();

    auto& entityAddedObs = signalManager->getEntityCreatedNotifier();
    m_entityAddObserverId =
        entityAddedObs.attach( [this]( const Ra::Engine::Scene::ItemEntry& entry ) {
            auto it = std::find_if(
                m_entityKeyFrames.begin(), m_entityKeyFrames.end(), [entry]( const auto& frames ) {
                    return entry.m_entity == frames.first;
                } );
            if ( it != m_entityKeyFrames.end() ) { m_entityKeyFrames.erase( it ); }
        } );

    auto& entityRemovedObs = signalManager->getEntityDestroyedNotifier();
    m_entityRemoveObserverId =
        entityRemovedObs.attach( [this]( const Ra::Engine::Scene::ItemEntry& entry ) {
            auto it = std::find_if(
                m_componentKeyFrames.begin(),
                m_componentKeyFrames.end(),
                [entry]( const auto& frames ) { return entry.m_component == frames.first; } );
            if ( it != m_componentKeyFrames.end() ) { m_componentKeyFrames.erase( it ); }
        } );

    auto& roAddedObs  = signalManager->getRenderObjectCreatedNotifier();
    m_roAddObserverId = roAddedObs.attach( [this]( const Ra::Engine::Scene::ItemEntry& entry ) {
        auto it = std::find_if(
            m_renderObjectKeyFrames.begin(),
            m_renderObjectKeyFrames.end(),
            [entry]( const auto& frames ) { return entry.m_roIndex == frames.first; } );
        if ( it != m_renderObjectKeyFrames.end() ) { m_renderObjectKeyFrames.erase( it ); }
    } );
}

void Timeline::detachFromEngine() {
    // Lifetime of Timeline (as a gui object) is hard to predict. Check all pointer and properties.
    if ( auto engine = Ra::Engine::RadiumEngine::getInstance() ) {
        if ( auto signalManager = engine->getSignalManager() ) {
            if ( m_entityAddObserverId != -1 ) {
                auto& obs = signalManager->getEntityCreatedNotifier();
                obs.detach( m_entityAddObserverId );
                m_entityAddObserverId = -1;
            }
            if ( m_entityRemoveObserverId != -1 ) {
                auto& obs = signalManager->getEntityDestroyedNotifier();
                obs.detach( m_entityRemoveObserverId );
                m_entityRemoveObserverId = -1;
            }
            if ( m_roAddObserverId ) {
                auto& obs = signalManager->getRenderObjectCreatedNotifier();
                obs.detach( m_roAddObserverId );
                m_roAddObserverId = -1;
            }
        }
    }
}

Timeline::~Timeline() {
    detachFromEngine();
    delete ui;
}

void Timeline::selectionChanged( const Ra::Engine::Scene::ItemEntry& ent ) {
    // enables ui if any keyframe
    auto enableUI = [this]( bool enable ) {
        ui->comboBox_attribute->setEnabled( enable );
        ui->toolButton_keyFrame->setEnabled( enable );
        ui->m_nbKeyFramesSpin->setEnabled( enable );
        ui->m_removeKeyFrameButton->setEnabled( enable );
        ui->pushButton_editAttribute->setEnabled( enable );
    };
    // reset ui
    ui->comboBox_attribute->blockSignals( true );
    ui->comboBox_attribute->clear();
    ui->comboBox_attribute->blockSignals( false );
    enableUI( false );
    onClearKeyFrames();
    m_current = Ra::Core::Animation::KeyFramedValueController();
    // collects keyframedvalue name; if first one then display times and register as current
    auto registerFrames = [this]( const Ra::Core::Animation::KeyFramedValueController& frame,
                                  const std::string& prefix ) {
        ui->comboBox_attribute->addItem( QString( ( prefix + frame.m_name ).c_str() ) );
        if ( ui->comboBox_attribute->count() == 1 ) {
            const auto times = frame.m_value->getTimes();
            for ( const auto& t : times ) {
                ui->frame_selector->onAddingKeyFrame( t, false );
            }
            m_current = frame;
        }
    };
    // checks if the given keyframedvalue map contains the given key.
    // if so, registers all keyframedvalues with the given prefix.
#define REGISTER_KEYFRAMED_VALUES( map, key, prefix )         \
    {                                                         \
        auto it = map.find( key );                            \
        if ( it != map.end() ) {                              \
            for ( const auto& keyFramedValue : it->second ) { \
                registerFrames( keyFramedValue, prefix );     \
            }                                                 \
        }                                                     \
    }
    // register keyframes for the Entity
    if ( ent.m_entity == nullptr ) { return; }
    const std::string& entityName = ent.m_entity->getName();
    REGISTER_KEYFRAMED_VALUES( m_entityKeyFrames, ent.m_entity, entityName + "::" );
    // register keyframes for the Component
    if ( ent.m_component == nullptr ) {
        enableUI( ui->comboBox_attribute->count() > 0 );
        return;
    }
    const std::string& compName = ent.m_component->getName();
    REGISTER_KEYFRAMED_VALUES(
        m_componentKeyFrames, ent.m_component, entityName + "::" + compName + "::" );
    // register keyframes for the RenderObject
    if ( ent.m_roIndex == Ra::Core::Utils::Index::Invalid() ) {
        enableUI( ui->comboBox_attribute->count() > 0 );
        return;
    }
    const auto& roMngr = Ra::Engine::RadiumEngine::getInstance()->getRenderObjectManager();
    const std::string roName =
        roMngr->getRenderObject( ent.m_roIndex )->getName() + "_" + std::to_string( ent.m_roIndex );
    REGISTER_KEYFRAMED_VALUES( m_renderObjectKeyFrames,
                               ent.m_roIndex,
                               entityName + "::" + compName + "::" + roName + "::" );
#undef REGISTER_KEYFRAMED_VALUES
    if ( ui->comboBox_attribute->count() > 0 ) {
        enableUI( true );
        ui->comboBox_attribute->setCurrentIndex( 0 );
    }
}

void Timeline::registerKeyFramedValue(
    Ra::Engine::Scene::Entity* ent,
    const Ra::Core::Animation::KeyFramedValueController& keyFramedValueController ) {
    auto& values = m_entityKeyFrames[ent];
    auto& name   = keyFramedValueController.m_name;
    auto it      = std::find_if( values.begin(), values.end(), [&name]( const auto& frame ) {
        return frame.m_name == name;
    } );
    if ( it != values.end() ) {
        LOG( logWARNING ) << "[Timeline] Already existing KeyFramedValue: " << name
                          << "\" on Entity \"" << ent->getName() << "\". Will not register."
                          << std::endl;
        return;
    }
    values.push_back( keyFramedValueController );
    selectionChanged( Engine::Scene::ItemEntry( ent ) );
}

void Timeline::unregisterKeyFramedValue( Ra::Engine::Scene::Entity* ent, const std::string& name ) {
    auto& values = m_entityKeyFrames[ent];
    auto it      = std::find_if( values.begin(), values.end(), [&name]( const auto& frame ) {
        return frame.m_name == name;
    } );
    if ( it != values.end() ) { values.erase( it ); }
}

void Timeline::registerKeyFramedValue(
    Ra::Engine::Scene::Component* comp,
    const Ra::Core::Animation::KeyFramedValueController& keyFramedValueController ) {
    auto& values = m_componentKeyFrames[comp];
    auto name    = keyFramedValueController.m_name;
    auto it      = std::find_if( values.begin(), values.end(), [name]( const auto& frame ) {
        return frame.m_name == name;
    } );
    if ( it != values.end() ) {
        LOG( logWARNING ) << "[Timeline] Already existing KeyFramedValue: \"" << name
                          << "\" on Component \"" << comp->getName() << "\". Will not register."
                          << std::endl;
        return;
    }
    values.push_back( keyFramedValueController );
    selectionChanged( Engine::Scene::ItemEntry( comp->getEntity(), comp ) );
}

void Timeline::unregisterKeyFramedValue( Ra::Engine::Scene::Component* comp,
                                         const std::string& name ) {
    auto& values = m_componentKeyFrames[comp];
    auto it      = std::find_if( values.begin(), values.end(), [&name]( const auto& frame ) {
        return frame.m_name == name;
    } );
    if ( it != values.end() ) { values.erase( it ); }
}

void Timeline::registerKeyFramedValue(
    Ra::Core::Utils::Index roIdx,
    const Ra::Core::Animation::KeyFramedValueController& keyFramedValueController ) {
    auto& values = m_renderObjectKeyFrames[roIdx];
    auto name    = keyFramedValueController.m_name;
    auto it      = std::find_if( values.begin(), values.end(), [&name]( const auto& frame ) {
        return frame.m_name == name;
    } );
    if ( it != values.end() ) {
        LOG( logWARNING ) << "[Timeline] Already existing KeyFramedValue: " << name
                          << "\" on RenderObject \"" << roIdx << "\". Will not register."
                          << std::endl;
        return;
    }
    values.push_back( keyFramedValueController );
    auto roMgr = Engine::RadiumEngine::getInstance()->getRenderObjectManager();
    auto RO    = roMgr->getRenderObject( roIdx );
    auto comp  = RO->getComponent();
    selectionChanged( Engine::Scene::ItemEntry( comp->getEntity(), comp, roIdx ) );
}

void Timeline::unregisterKeyFramedValue( Ra::Core::Utils::Index roIdx, const std::string& name ) {
    auto& values = m_renderObjectKeyFrames[roIdx];
    auto it      = std::find_if( values.begin(), values.end(), [&name]( const auto& frame ) {
        return frame.m_name == name;
    } );
    if ( it != values.end() ) { values.erase( it ); }
}

Scalar Timeline::getTime() const {
    return Scalar( ui->m_cursorSpin->value() );
}

void Timeline::onChangeStart( Scalar time ) {
    ui->frame_selector->onChangeStart( time, false );
}

void Timeline::onChangeEnd( Scalar time ) {
    ui->frame_selector->onChangeEnd( time, false );
}

void Timeline::onChangeDuration( Scalar time ) {
    ui->frame_selector->onChangeDuration( time, false );
}

void Timeline::onChangeCursor( Scalar time ) {
    if ( Ra::Core::Math::areApproxEqual( time, Scalar( ui->m_cursorSpin->value() ) ) ) { return; }
    ui->frame_selector->onChangeCursor( time, false );
    updateKeyFrames( time );
}

void Timeline::onSetPlay( bool play ) {
    ui->toolButton_playPause->setChecked( play );
}

// todo : ctrlDown = shiftDown = false, on focus event (initialize state)
void Timeline::resizeEvent( QResizeEvent* event ) {
    ui->m_scrollArea->onDrawRuler( event->size().width() - 2 );
}

void Timeline::on_pingPong_toggled( bool checked ) {
    emit setPingPong( checked );
}

void Timeline::updateKeyFrames( Scalar time ) {
    auto update = [time]( auto& keyFrames ) {
        for ( auto& KF : keyFrames ) {
            for ( auto& kf : KF.second ) {
                kf.updateKeyFrame( time );
            }
        }
    };
    update( m_entityKeyFrames );
    update( m_componentKeyFrames );
    update( m_renderObjectKeyFrames );
}

void Timeline::onClearKeyFrames() {
    ui->frame_selector->onClearKeyFrames();
}

void Timeline::onAddingKeyFrame( Scalar time ) {
    if ( m_current.m_value ) {
        m_current.insertKeyFrame( time );
        emit keyFrameAdded( time );
    }
}

void Timeline::onRemovingKeyFrame( size_t i ) {
    if ( m_current.m_value ) {
        const Scalar time = m_current.m_value->getTimes()[i];
        if ( m_current.m_value->removeKeyFrame( i ) ) {
            m_current.updateKeyFrame( time );
            emit keyFrameDeleted( i );
        }
        else {
            LOG( logWARNING ) << "[Timeline] Error: Cannot remove keyFrame at "
                              << Scalar( ui->m_cursorSpin->value() );
        }
    }
}

void Timeline::onChangingKeyFrame( size_t i ) {
    if ( m_current.m_value ) {
        Scalar time = m_current.m_value->getTimes()[i];
        m_current.insertKeyFrame( time );
        emit keyFrameChanged( i );
    }
}

void Timeline::onMovingKeyFrame( size_t i, Scalar time1 ) {
    if ( m_current.m_value ) {
        if ( !Ra::Core::Math::areApproxEqual( m_current.m_value->getTimes()[i], time1 ) ) {
            m_current.m_value->moveKeyFrame( i, time1 );
            m_current.updateKeyFrame( time1 );
            emit keyFrameMoved( i, time1 );
        }
    }
}

void Timeline::onMovingKeyFrames( size_t first, Scalar offset ) {
    if ( m_current.m_value ) {
        auto times        = m_current.m_value->getTimes();
        const Scalar time = times[first];
        if ( offset < 0 ) {
            for ( int i = 0; i < int( times.size() ); ++i ) {
                const Scalar t = times[i];
                if ( t >= time ) { m_current.m_value->moveKeyFrame( i, t + offset ); }
            }
        }
        else // go from the end to ensure we do not mess up keyframes
        {
            for ( int i = times.size() - 1; i >= 0; --i ) {
                const Scalar t = times[i];
                if ( t >= time ) { m_current.m_value->moveKeyFrame( i, t + offset ); }
            }
        }
        m_current.updateKeyFrame( time );
        emit keyFramesMoved( first, offset );
    }
}

void Timeline::on_comboBox_attribute_currentIndexChanged( const QString& arg1 ) {
    onClearKeyFrames();
    std::vector<Ra::Core::Animation::KeyFramedValueController> list;
    const QStringList names = arg1.split( "::" );

    // prints a warning message for the attribute and returns
#define LOG_AND_RETURN                                                          \
    LOG( logWARNING ) << "[Timeline] Error: attribute \"" << arg1.toStdString() \
                      << "\"'s name is not conform.";                           \
    return

    // Checks if the given vector of KeyFramedValues contains a KeyFramedValue
    // with the given name.
    // If so, set the current frame to this KeyFramedValue;
    // If not, prints a warning message for the attribute and returns.
#define GET_KEYFRAMEDVALUE( list, name )                                                 \
    {                                                                                    \
        auto it = std::find_if( list.begin(), list.end(), [&name]( const auto& frame ) { \
            return frame.m_name == name;                                                 \
        } );                                                                             \
        if ( it == list.end() ) { LOG_AND_RETURN; }                                      \
        m_current = *it;                                                                 \
    }

    // Checks if the given map has an element fulfilling lambda.
    // If so, set list to the associated vector of KeyFramedValues.
    // If not, prints a warning message for the attribute and returns.
#define GET_KEYFRAMEDVALUE_LIST( map, lambda )                                 \
    {                                                                          \
        auto keyFramedValues = std::find_if( map.begin(), map.end(), lambda ); \
        if ( keyFramedValues == map.end() ) { LOG_AND_RETURN; }                \
        list = keyFramedValues->second;                                        \
    }

    switch ( names.size() ) {
    case 2: {
        const std::string entityName = names.at( 0 ).toStdString();
        const std::string frameName  = names.at( 1 ).toStdString();
        auto lambda                  = [entityName]( const auto& frame ) {
            return frame.first->getName() == entityName;
        };
        GET_KEYFRAMEDVALUE_LIST( m_entityKeyFrames, lambda );
        GET_KEYFRAMEDVALUE( list, frameName );
    } break;
    case 3: {
        const std::string compName  = names.at( 1 ).toStdString();
        const std::string frameName = names.at( 2 ).toStdString();
        auto lambda                 = [compName]( const auto& frame ) {
            return frame.first->getName() == compName;
        };
        GET_KEYFRAMEDVALUE_LIST( m_componentKeyFrames, lambda );
        GET_KEYFRAMEDVALUE( list, frameName );
    } break;
    case 4: {
        const QStringList fullRoName = names.at( 2 ).split( '_' );
        bool ok;
        const auto roIdx = fullRoName.last().toInt( &ok );
        if ( !ok ) { LOG_AND_RETURN; }
        auto lambda = [&roIdx]( const auto& frame ) { return frame.first == roIdx; };
        const std::string frameName = names.at( 3 ).toStdString();
        GET_KEYFRAMEDVALUE_LIST( m_renderObjectKeyFrames, lambda );
        GET_KEYFRAMEDVALUE( list, frameName );
    } break;
    default:
        LOG_AND_RETURN;
    }
#undef GET_KEYFRAMEDVALUE_LIST
#undef GET_KEYFRAMEDVALUE
#undef LOG_AND_RETURN

    // update ui
    const auto times = m_current.m_value->getTimes();
    for ( const auto& t : times ) {
        ui->frame_selector->onAddingKeyFrame( t, false );
    }
}

void Timeline::on_pushButton_editAttribute_clicked() {
    // TODO
}

void Timeline::on_toolButton_help_clicked() {
    static bool is_showing = false;
    if ( !is_showing ) {
        HelpDialog* dialog = new HelpDialog( this );
        dialog->show();
        is_showing = true;
        connect( dialog, &HelpDialog::closed, [=]() { is_showing = false; } );
    }
}

} // namespace Ra::Gui
