#include <Gui/TimerData/FrameTimerData.hpp>

namespace Ra {
void FrameTimerData::print( std::ostream& ostream ) const {

    long totalTime = Ra::Core::Utils::getIntervalMicro( frameStart, frameEnd );

    ostream << "frame " << numFrame << ": " << totalTime << "\n";
    ostream << "{"
            << "\n";
    {
        long taStart = Ra::Core::Utils::getIntervalMicro( frameStart, tasksStart );
        long taEnd   = Ra::Core::Utils::getIntervalMicro( frameStart, tasksEnd );

        long reStart = Ra::Core::Utils::getIntervalMicro( frameStart, renderData.renderStart );
        long reEnd   = Ra::Core::Utils::getIntervalMicro( frameStart, renderData.renderEnd );
        ostream << "\ttasks: " << taStart << " " << taEnd << " " << taEnd - taStart << "\n";
        ostream << "\t{"
                << "\n";
        for ( const auto& tData : taskData ) {
            long tadaStart = Ra::Core::Utils::getIntervalMicro( frameStart, tData.start );
            long tadaEnd   = Ra::Core::Utils::getIntervalMicro( frameStart, tData.end );
            ostream << "\t\t" << tData.taskName << "(" << tData.threadId << "): " << tadaStart
                    << " " << tadaEnd << " " << tadaEnd - tadaStart << "\n";
        }
        ostream << "\t}"
                << "\n";
        ostream << "\trender: " << reStart << " " << reEnd << " " << reEnd - reStart << "\n";
    }
    ostream << "}"
            << "\n";
    ostream << std::endl;
}
} // namespace Ra
