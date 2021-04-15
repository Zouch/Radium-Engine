#include "ui_SkeletonBasedAnimationUI.h"
#include <Gui/SkeletonBasedAnimation/SkeletonBasedAnimationUI.hpp>

#include <QFileDialog>
#include <QSettings>

#include <fstream>
#include <iostream>

#include <Core/Animation/KeyFramedValueController.hpp>
#include <Core/Animation/KeyFramedValueInterpolators.hpp>
#include <Core/CoreMacros.hpp>
#include <Core/Utils/StringUtils.hpp>

#include <Engine/RadiumEngine.hpp>
#include <Engine/Scene/Entity.hpp>
#include <Engine/Scene/SkeletonBasedAnimationSystem.hpp>
#include <Engine/Scene/SkeletonComponent.hpp>
#include <Engine/Scene/SkinningComponent.hpp>

#include <Gui/Timeline/Timeline.hpp>

using namespace Ra::Core::Animation;

namespace Ra::Gui {

SkeletonBasedAnimationUI::SkeletonBasedAnimationUI(
    Engine::Scene::SkeletonBasedAnimationSystem* system,
    Timeline* timeline,
    QWidget* parent ) :
    QFrame( parent ),
    ui( new Ui::SkeletonBasedAnimationUI ),
    m_system( system ),
    m_timeline( timeline ) {
    ui->setupUi( this );
    connect(
        ui->actionXray, &QAction::toggled, this, &SkeletonBasedAnimationUI::on_m_xray_clicked );
    connect( ui->actionXray,
             &QAction::toggled,
             this,
             &SkeletonBasedAnimationUI::on_m_showSkeleton_toggled );
}

SkeletonBasedAnimationUI::~SkeletonBasedAnimationUI() {
    delete ui;
}

void SkeletonBasedAnimationUI::selectionChanged( const Engine::Scene::ItemEntry& entry ) {
    m_selection       = entry;
    m_currentSkeleton = nullptr;
    m_currentSkinnings.clear();
    ui->actionLBS->setEnabled( false );
    ui->actionDQS->setEnabled( false );
    ui->actionCoR->setEnabled( false );
    ui->actionSTBSLBS->setEnabled( false );
    ui->actionSTBSDQS->setEnabled( false );
    ui->tabWidget->setEnabled( false );
    ui->m_skinning->setEnabled( false );
    if ( m_selection.m_entity == nullptr ) { return; }
    for ( auto& comp : m_selection.m_entity->getComponents() )
    {
        if ( auto curSkel = dynamic_cast<Engine::Scene::SkeletonComponent*>( comp.get() ) )
        {
            // register current Skeleton component
            m_currentSkeleton = curSkel;
            // update the ui accordingly
            ui->tabWidget->setEnabled( true );
            ui->actionXray->setChecked( m_currentSkeleton->isXray() );
            ui->m_speed->setValue( double( m_currentSkeleton->getSpeed() ) );
            ui->m_autoRepeat->setChecked( m_currentSkeleton->isAutoRepeat() );
            ui->m_pingPong->setChecked( m_currentSkeleton->isPingPong() );
            ui->m_currentAnimation->blockSignals( true );
            ui->m_currentAnimation->clear();
            for ( size_t i = 0; i < m_currentSkeleton->getAnimationCount(); ++i )
            {
                ui->m_currentAnimation->addItem( "#" + QString::number( i ) );
            }
            ui->m_currentAnimation->blockSignals( false );
            ui->m_currentAnimation->setCurrentIndex( int( m_currentSkeleton->getAnimationId() ) );
            ui->m_xray->setChecked( m_currentSkeleton->isXray() );
            ui->m_showSkeleton->setChecked( m_currentSkeleton->isShowingSkeleton() );
        }
        if ( auto skinComp = dynamic_cast<Engine::Scene::SkinningComponent*>( comp.get() ) )
        {
            // register the current skinning component
            m_currentSkinnings.emplace_back( skinComp );
            // update the ui accordingly
            ui->m_skinning->setEnabled( true );
            ui->m_smartStretch->setEnabled( true );
            ui->m_smartStretch->setChecked( skinComp->isSmartStretchOn() );
            ui->actionLBS->setEnabled( true );
            ui->actionDQS->setEnabled( true );
            ui->actionCoR->setEnabled( true );
            ui->actionSTBSLBS->setEnabled( true );
            ui->actionSTBSDQS->setEnabled( true );
            ui->m_skinningMethod->setEnabled( true );
            ui->m_skinningMethod->setCurrentIndex( int( skinComp->getSkinningType() ) );
            on_m_skinningMethod_currentIndexChanged( int( skinComp->getSkinningType() ) );
            ui->m_showWeights->setEnabled( true );
            ui->m_showWeights->setChecked( skinComp->isShowingWeights() );
            ui->m_weightsType->setEnabled( true );
            ui->m_weightsType->setCurrentIndex( int( skinComp->getWeightsType() ) );
            ui->m_normalSkinning->setEnabled( true );
            ui->m_normalSkinning->setCurrentIndex( int( skinComp->getNormalSkinning() ) );
        }
    }

    // deal with bone selection for weights display
    if ( m_currentSkeleton != nullptr )
    {
        auto BM   = *m_currentSkeleton->getBoneRO2idx();
        auto b_it = BM.find( m_selection.m_roIndex );
        if ( b_it != BM.end() )
        {
            for ( auto skin : m_currentSkinnings )
            {
                skin->setWeightBone( b_it->second );
            }
        }
    }

    askForUpdate();
}

int SkeletonBasedAnimationUI::getActionNb() {
    return 6;
}

QAction* SkeletonBasedAnimationUI::getAction( int i ) {
    switch ( i )
    {
    case 0:
        return ui->actionXray;
    case 1:
        return ui->actionLBS;
    case 2:
        return ui->actionDQS;
    case 3:
        return ui->actionCoR;
    case 4:
        return ui->actionSTBSLBS;
    case 5:
        return ui->actionSTBSDQS;
    }
    return nullptr;
}

void SkeletonBasedAnimationUI::postLoadFile( Engine::Scene::Entity* entity ) {
    // reset current data and ui
    m_selection       = Engine::Scene::ItemEntry();
    m_currentSkeleton = nullptr;
    m_currentSkinnings.clear();
    ui->tabWidget->setEnabled( false );
    ui->m_skinning->setEnabled( false );
    if ( !m_timeline ) { return; }
    // register the animation keyframes of the first animation into the timeline
    auto c = std::find_if(
        entity->getComponents().begin(), entity->getComponents().end(), []( const auto& cmpt ) {
            return ( dynamic_cast<Ra::Engine::Scene::SkeletonComponent*>( cmpt.get() ) != nullptr );
        } );
    if ( c != entity->getComponents().end() )
    {
        auto skel           = static_cast<Ra::Engine::Scene::SkeletonComponent*>( ( *c ).get() );
        auto& anim          = skel->getAnimation( skel->getAnimationId() );
        const auto& boneMap = skel->getBoneRO2idx();
        for ( size_t j = 0; j < anim.size(); ++j )
        {
            auto it = std::find_if(
                boneMap->begin(), boneMap->end(), [j]( const auto& b ) { return b.second == j; } );
            if ( it == boneMap->end() ) { continue; } // end bone

            m_timeline->registerKeyFramedValue(
                it->first,
                KeyFramedValueController(
                    &anim[j],
                    "Animation_" + skel->getSkeleton()->getLabel( uint( j ) ),
                    [&anim, j, skel]( const Scalar& t ) {
                        anim[j].insertKeyFrame(
                            t, skel->getSkeleton()->getPose( HandleArray::SpaceType::LOCAL )[j] );
                    } ) ); // no update callback here
        }
    }
    // update the timeline display interval to the bounding interval of all anims
    Scalar startTime = std::numeric_limits<Scalar>::max();
    Scalar endTime   = 0;
    for ( const auto& animComp : entity->getComponents() )
    {
        if ( auto skel = dynamic_cast<Ra::Engine::Scene::SkeletonComponent*>( animComp.get() ) )
        {
            auto [s, e] = skel->getAnimationTimeInterval();
            startTime   = std::min( startTime, s );
            endTime     = std::max( endTime, e );
        }
    }
    m_timeline->onChangeStart( startTime );
    m_timeline->onChangeEnd( endTime );
}

void SkeletonBasedAnimationUI::on_actionXray_triggered( bool checked ) {
    ui->m_xray->setChecked( checked );
    on_m_xray_clicked( checked );
}

void SkeletonBasedAnimationUI::on_m_speed_valueChanged( double arg1 ) {
    CORE_ASSERT( m_currentSkeleton, "Null SkeletonComponent." );
    m_currentSkeleton->setSpeed( Scalar( arg1 ) );
    askForUpdate();
}

void SkeletonBasedAnimationUI::on_m_pingPong_toggled( bool checked ) {
    CORE_ASSERT( m_currentSkeleton, "Null SkeletonComponent." );
    m_currentSkeleton->pingPong( checked );
    askForUpdate();
}

void SkeletonBasedAnimationUI::on_m_autoRepeat_toggled( bool checked ) {
    CORE_ASSERT( m_currentSkeleton, "Null SkeletonComponent." );
    m_currentSkeleton->autoRepeat( checked );
    askForUpdate();
}

void SkeletonBasedAnimationUI::on_m_currentAnimation_currentIndexChanged( int index ) {
    if ( !m_timeline ) { return; }

    m_currentSkeleton->useAnimation( uint( index ) );
    auto& anim          = m_currentSkeleton->getAnimation( uint( index ) );
    const auto& boneMap = m_currentSkeleton->getBoneRO2idx();
    auto skel           = m_currentSkeleton->getSkeleton();
    for ( size_t j = 0; j < anim.size(); ++j )
    {
        auto it = std::find_if(
            boneMap->begin(), boneMap->end(), [j]( const auto& b ) { return b.second == j; } );
        if ( it == boneMap->end() ) { continue; } // end bone
        m_timeline->unregisterKeyFramedValue( it->first,
                                              "Animation_" + skel->getLabel( uint( j ) ) );
        m_timeline->registerKeyFramedValue(
            it->first,
            KeyFramedValueController(
                &anim[j],
                "Animation_" + skel->getLabel( uint( j ) ),
                [&anim, j, skel]( const Scalar& t ) {
                    anim[j].insertKeyFrame( t, skel->getPose( HandleArray::SpaceType::LOCAL )[j] );
                } ) ); // no update callback here
    }
    // ask the animation system to update w.r.t. the animation
    m_system->enforceUpdate();
    // update the animation keyframes display in the timeline
    m_timeline->selectionChanged( m_selection );

    askForUpdate();
}

void SkeletonBasedAnimationUI::on_m_newAnim_clicked() {
    CORE_ASSERT( m_currentSkeleton, "Null SkeletonComponent." );
    const int num = ui->m_currentAnimation->count();
    m_currentSkeleton->addNewAnimation();
    ui->m_currentAnimation->addItem( "#" + QString::number( num ) );
    ui->m_currentAnimation->setCurrentIndex( num );
    askForUpdate();
}

void SkeletonBasedAnimationUI::on_m_removeAnim_clicked() {
    if ( ui->m_currentAnimation->count() == 1 ) { return; }

    CORE_ASSERT( m_currentSkeleton, "Null SkeletonComponent." );
    const int removeIndex = ui->m_currentAnimation->currentIndex();
    m_currentSkeleton->removeAnimation( size_t( removeIndex ) );
    ui->m_currentAnimation->removeItem( removeIndex );
    ui->m_currentAnimation->setCurrentIndex( int( m_currentSkeleton->getAnimationId() ) );
    askForUpdate();
}

void SkeletonBasedAnimationUI::on_m_loadAnim_clicked() {
    QSettings settings;
    QString path = settings.value( "Animation::path", QDir::homePath() ).toString();
    path         = QFileDialog::getOpenFileName( nullptr, "Load Animation", path, "*.rdma" );
    if ( path.size() == 0 ) { return; }
    settings.setValue( "Animation::path", path );
    ui->m_animFile->setText( path.split( '/' ).last() );

    // load the animation into the skeleton component
    auto& anim = m_currentSkeleton->addNewAnimation();
    std::ifstream file( path.toStdString(), std::ios::binary );
    size_t n;
    Scalar t;
    Ra::Core::Transform T;
    for ( auto& bAnim : anim )
    {
        file.read( reinterpret_cast<char*>( &n ), sizeof( n ) );
        for ( size_t i = 0; i < n; ++i )
        {
            file.read( reinterpret_cast<char*>( &t ), sizeof( t ) );
            file.read( reinterpret_cast<char*>( &T ), sizeof( T ) );
            bAnim.insertKeyFrame( t, T );
        }
    }

    // update the ui and set the loaded animation as the one used
    const int num = ui->m_currentAnimation->count();
    ui->m_currentAnimation->addItem( "#" + QString::number( num ) );
    ui->m_currentAnimation->setCurrentIndex( num );

    // request update
    askForUpdate();
}

void SkeletonBasedAnimationUI::on_m_saveAnim_clicked() {
    QSettings settings;
    QString path = settings.value( "Animation::path", QDir::homePath() ).toString();
    path         = QFileDialog::getSaveFileName( nullptr, "Load Animation", path, "*.rdma" );
    if ( path.size() == 0 ) { return; }
    settings.setValue( "Animation::path", path );
    ui->m_animFile->setText( path.split( '/' ).last() );

    const auto& anim = m_currentSkeleton->getAnimation( m_currentSkeleton->getAnimationId() );
    std::ofstream file( path.toStdString(), std::ios::trunc | std::ios::binary );
    // Todo: deal with anim timestep when used
    size_t n;
    for ( const auto& bAnim : anim )
    {
        n = bAnim.size();
        file.write( reinterpret_cast<const char*>( &n ), sizeof( n ) );
        for ( const auto t : bAnim.getTimes() )
        {
            file.write( reinterpret_cast<const char*>( &t ), sizeof( t ) );
            auto kf = bAnim.at( t, linearInterpolate<Ra::Core::Transform> );
            file.write( reinterpret_cast<const char*>( &kf ), sizeof( kf ) );
        }
    }
}

void SkeletonBasedAnimationUI::on_m_xray_clicked( bool checked ) {
    m_system->setXray( checked );
    askForUpdate();
}

void SkeletonBasedAnimationUI::on_m_showSkeleton_toggled( bool checked ) {
    m_system->toggleSkeleton( checked );
    askForUpdate();
}

void SkeletonBasedAnimationUI::on_m_smartStretch_toggled( bool checked ) {
    for ( auto skin : m_currentSkinnings )
    {
        if ( skin ) { skin->setSmartStretch( checked ); }
    }
    askForUpdate();
}

void SkeletonBasedAnimationUI::on_m_skinningMethod_currentIndexChanged( int newType ) {
    CORE_ASSERT( newType >= 0 && newType < 5, "Invalid Skinning Type" );
    using SkinningType = Ra::Engine::Scene::SkinningComponent::SkinningType;
    auto type          = SkinningType( newType );
    for ( auto skin : m_currentSkinnings )
    {
        skin->setSkinningType( type );
    }
    switch ( type )
    {
    case SkinningType::LBS: {
        on_actionLBS_triggered();
        break;
    }
    case SkinningType::DQS: {
        on_actionDQS_triggered();
        break;
    }
    case SkinningType::COR: {
        on_actionCoR_triggered();
        break;
    }
    case SkinningType::STBS_LBS: {
        on_actionSTBSLBS_triggered();
        break;
    }
    case SkinningType::STBS_DQS: {
        on_actionSTBSDQS_triggered();
        break;
    }
    default: {
        break;
    }
    }
}

void SkeletonBasedAnimationUI::on_m_showWeights_toggled( bool checked ) {
    for ( auto skin : m_currentSkinnings )
    {
        skin->showWeights( checked );
    }
    askForUpdate();
}

void SkeletonBasedAnimationUI::on_m_weightsType_currentIndexChanged( int newType ) {
    for ( auto skin : m_currentSkinnings )
    {
        skin->showWeightsType( Engine::Scene::SkinningComponent::WeightType( newType ) );
    }
    askForUpdate();
}

void SkeletonBasedAnimationUI::on_m_normalSkinning_currentIndexChanged( int newType ) {
    for ( auto skin : m_currentSkinnings )
    {
        skin->setNormalSkinning( Engine::Scene::SkinningComponent::NormalSkinning( newType ) );
    }
    askForUpdate();
}

void SkeletonBasedAnimationUI::on_actionLBS_triggered() {
    using SkinningType = Ra::Engine::Scene::SkinningComponent::SkinningType;
    ui->m_skinningMethod->setCurrentIndex( int( SkinningType::LBS ) );
    ui->actionLBS->setChecked( true );
    ui->actionDQS->setChecked( false );
    ui->actionCoR->setChecked( false );
    ui->actionSTBSLBS->setChecked( false );
    ui->actionSTBSDQS->setChecked( false );
    askForUpdate();
}

void SkeletonBasedAnimationUI::on_actionDQS_triggered() {
    using SkinningType = Ra::Engine::Scene::SkinningComponent::SkinningType;
    ui->m_skinningMethod->setCurrentIndex( int( SkinningType::DQS ) );
    ui->actionLBS->setChecked( false );
    ui->actionDQS->setChecked( true );
    ui->actionCoR->setChecked( false );
    ui->actionSTBSLBS->setChecked( false );
    ui->actionSTBSDQS->setChecked( false );
    askForUpdate();
}

void SkeletonBasedAnimationUI::on_actionCoR_triggered() {
    using SkinningType = Ra::Engine::Scene::SkinningComponent::SkinningType;
    ui->m_skinningMethod->setCurrentIndex( int( SkinningType::COR ) );
    ui->actionLBS->setChecked( false );
    ui->actionDQS->setChecked( false );
    ui->actionCoR->setChecked( true );
    ui->actionSTBSLBS->setChecked( false );
    ui->actionSTBSDQS->setChecked( false );
    askForUpdate();
}

void SkeletonBasedAnimationUI::on_actionSTBSLBS_triggered() {
    using SkinningType = Ra::Engine::Scene::SkinningComponent::SkinningType;
    ui->m_skinningMethod->setCurrentIndex( int( SkinningType::STBS_LBS ) );
    ui->actionLBS->setChecked( false );
    ui->actionDQS->setChecked( false );
    ui->actionCoR->setChecked( false );
    ui->actionSTBSLBS->setChecked( true );
    ui->actionSTBSDQS->setChecked( false );
    askForUpdate();
}

void SkeletonBasedAnimationUI::on_actionSTBSDQS_triggered() {
    using SkinningType = Ra::Engine::Scene::SkinningComponent::SkinningType;
    ui->m_skinningMethod->setCurrentIndex( int( SkinningType::STBS_DQS ) );
    ui->actionLBS->setChecked( false );
    ui->actionDQS->setChecked( false );
    ui->actionCoR->setChecked( false );
    ui->actionSTBSLBS->setChecked( false );
    ui->actionSTBSDQS->setChecked( true );
    askForUpdate();
}

} // namespace Ra::Gui
