#include <Core/Tasks/Task.hpp>
#include <Core/Tasks/TaskQueue.hpp>

#include <algorithm>
#include <iostream>
#include <stack>

namespace Ra {
namespace Core {

TaskQueue::TaskQueue( uint numThreads ) : m_processingTasks( 0 ), m_shuttingDown( false ) {
    CORE_ASSERT( numThreads > 0, " You need at least one thread" );
    m_workerThreads.reserve( numThreads );
    for ( uint i = 0; i < numThreads; ++i ) {
        m_workerThreads.emplace_back( &TaskQueue::runThread, this, i );
    }
}

TaskQueue::~TaskQueue() {
    flushTaskQueue();
    m_shuttingDown = true;
    m_threadNotifier.notify_all();
    for ( auto& t : m_workerThreads ) {
        t.join();
    }
}

TaskQueue::TaskId TaskQueue::registerTask( Task* task ) {
    TimerData tdata;
    // init tdata with task name before moving ownership
    tdata.taskName = task->getName();
    m_timerData.push_back( tdata );

    m_tasks.emplace_back( std::unique_ptr<Task>( task ) );
    m_dependencies.push_back( std::vector<TaskId>() );
    m_remainingDependencies.push_back( 0 );

    CORE_ASSERT( m_tasks.size() == m_dependencies.size(), "Inconsistent task list" );
    CORE_ASSERT( m_tasks.size() == m_remainingDependencies.size(), "Inconsistent task list" );
    CORE_ASSERT( m_tasks.size() == m_timerData.size(), "Inconsistent task list" );
    return TaskId { m_tasks.size() - 1 };
}

void TaskQueue::addDependency( TaskQueue::TaskId predecessor, TaskQueue::TaskId successor ) {
    CORE_ASSERT( predecessor.isValid() && ( predecessor < m_tasks.size() ),
                 "Invalid predecessor task" );
    CORE_ASSERT( successor.isValid() && ( successor < m_tasks.size() ), "Invalid successor task" );
    CORE_ASSERT( predecessor != successor, "Cannot add self-dependency" );

    CORE_ASSERT( std::find( m_dependencies[predecessor].begin(),
                            m_dependencies[predecessor].end(),
                            successor ) == m_dependencies[predecessor].end(),
                 "Cannot add a dependency twice" );

    m_dependencies[predecessor].push_back( successor );
    ++m_remainingDependencies[successor];
}

bool TaskQueue::addDependency( const std::string& predecessors, TaskQueue::TaskId successor ) {
    bool added = false;
    for ( uint i = 0; i < m_tasks.size(); ++i ) {
        if ( m_tasks[i]->getName() == predecessors ) {
            added = true;
            addDependency( TaskId { i }, successor );
        }
    }
    return added;
}

bool TaskQueue::addDependency( TaskQueue::TaskId predecessor, const std::string& successors ) {
    bool added = false;
    for ( uint i = 0; i < m_tasks.size(); ++i ) {
        if ( m_tasks[i]->getName() == successors ) {
            added = true;
            addDependency( predecessor, TaskId { i } );
        }
    }
    return added;
}

void TaskQueue::addPendingDependency( const std::string& predecessors,
                                      TaskQueue::TaskId successor ) {
    m_pendingDepsSucc.emplace_back( predecessors, successor );
}

void TaskQueue::addPendingDependency( TaskId predecessor, const std::string& successors ) {
    m_pendingDepsPre.emplace_back( predecessor, successors );
}

void TaskQueue::resolveDependencies() {
    for ( const auto& pre : m_pendingDepsPre ) {
        ON_ASSERT( bool result = ) addDependency( pre.first, pre.second );
        CORE_WARN_IF( !result,
                      "Pending dependency unresolved : " << m_tasks[pre.first]->getName() << " -> ("
                                                         << pre.second << ")" );
    }
    for ( const auto& pre : m_pendingDepsSucc ) {
        ON_ASSERT( bool result = ) addDependency( pre.first, pre.second );
        CORE_WARN_IF( !result,
                      "Pending dependency unresolved : (" << pre.first << ") -> "
                                                          << m_tasks[pre.second]->getName() );
    }
    m_pendingDepsPre.clear();
    m_pendingDepsSucc.clear();
}

void TaskQueue::queueTask( TaskQueue::TaskId task ) {
    CORE_ASSERT( m_remainingDependencies[task] == 0,
                 " Task" << m_tasks[task]->getName() << "has unmet dependencies" );
    m_taskQueue.push_front( task );
}

void TaskQueue::detectCycles() {
#if defined( CORE_DEBUG )
    // Do a depth-first search of the nodes.
    std::vector<bool> visited( m_tasks.size(), false );
    std::stack<TaskId> pending;

    for ( uint id = 0; id < m_tasks.size(); ++id ) {
        if ( m_dependencies[id].size() == 0 ) { pending.push( TaskId( id ) ); }
    }

    // If you hit this assert, there are tasks in the list but
    // all tasks have dependencies so no task can start.
    CORE_ASSERT( m_tasks.empty() || !pending.empty(), "No free tasks." );

    while ( !pending.empty() ) {
        TaskId id = pending.top();
        pending.pop();

        // The task has already been visited. It means there is a cycle in the task graph.
        CORE_ASSERT( !( visited[id] ), "Cycle detected in tasks !" );

        visited[id] = true;
        for ( const auto& dep : m_dependencies[id] ) {
            pending.push( dep );
        }
    }
#endif
}

void TaskQueue::startTasks() {
    // Add pending dependencies.
    resolveDependencies();

    // Do a debug check
    detectCycles();

    // Enqueue all tasks with no dependencies.
    for ( uint t = 0; t < m_tasks.size(); ++t ) {
        if ( m_remainingDependencies[t] == 0 ) { queueTask( TaskId { t } ); }
    }

    // Wake up all threads.
    m_threadNotifier.notify_all();
}

void TaskQueue::waitForTasks() {
    bool isFinished = false;
    while ( !isFinished ) {
        // TODO : use a notifier for task queue empty.
        m_taskQueueMutex.lock();
        isFinished = ( m_taskQueue.empty() && m_processingTasks == 0 );
        m_taskQueueMutex.unlock();
        if ( !isFinished ) { std::this_thread::yield(); }
    }
}

const std::vector<TaskQueue::TimerData>& TaskQueue::getTimerData() {
    return m_timerData;
}

void TaskQueue::flushTaskQueue() {
    CORE_ASSERT( m_processingTasks == 0, "You have tasks still in process" );
    CORE_ASSERT( m_taskQueue.empty(), " You have unprocessed tasks " );
    m_tasks.clear();
    m_dependencies.clear();
    m_timerData.clear();
    m_remainingDependencies.clear();
}

void TaskQueue::runThread( uint id ) {
    while ( true ) {
        TaskId task;

        // Acquire mutex.
        {
            std::unique_lock<std::mutex> lock( m_taskQueueMutex );

            // Wait for a new task
            // TODO : use the second form of wait()
            while ( !m_shuttingDown && m_taskQueue.empty() ) {
                m_threadNotifier.wait( lock );
            }
            // If the task queue is shutting down we quit, releasing
            // the lock.
            if ( m_shuttingDown ) { return; }

            // If we are here it means we got a task
            task = m_taskQueue.back();
            m_taskQueue.pop_back();
            ++m_processingTasks;
            CORE_ASSERT( task.isValid() && task < m_tasks.size(), "Invalid task" );
        }
        // Release mutex.

        // Run task
        m_timerData[task].start    = Utils::Clock::now();
        m_timerData[task].threadId = id;
        m_tasks[task]->process();
        m_timerData[task].end = Utils::Clock::now();

        // Critical section : mark task as finished and en-queue dependencies.
        uint newTasks = 0;
        {
            std::unique_lock<std::mutex> lock( m_taskQueueMutex );
            for ( auto t : m_dependencies[task] ) {
                uint& nDepends = m_remainingDependencies[t];
                CORE_ASSERT( nDepends > 0, "Inconsistency in dependencies" );
                --nDepends;
                if ( nDepends == 0 ) {
                    queueTask( t );
                    ++newTasks;
                }
                // TODO :Easy optimization : grab one of the new task and process it immediately.
            }
            --m_processingTasks;
        }
        // If we added new tasks, we wake up one thread to execute it.
        if ( newTasks > 0 ) { m_threadNotifier.notify_one(); }
    } // End of while(true)
}

void TaskQueue::printTaskGraph( std::ostream& output ) const {
    output << "digraph tasks {" << std::endl;

    for ( const auto& t : m_tasks ) {
        output << "\"" << t->getName() << "\"" << std::endl;
    }

    for ( uint i = 0; i < m_dependencies.size(); ++i ) {
        const auto& task1 = m_tasks[i];
        for ( const auto& dep : m_dependencies[i] ) {
            const auto& task2 = m_tasks[dep];
            output << "\"" << task1->getName() << "\""
                   << " -> ";
            output << "\"" << task2->getName() << "\"" << std::endl;
        }
    }

    for ( const auto& preDep : m_pendingDepsPre ) {
        const auto& task1  = m_tasks[preDep.first];
        std::string t2name = preDep.second;

        if ( std::find_if( m_tasks.begin(), m_tasks.end(), [=]( const auto& task ) {
                 return task->getName() == t2name;
             } ) == m_tasks.end() ) {
            t2name += "?";
        }
        output << "\"" << task1->getName() << "\""
               << " -> ";
        output << "\"" << t2name << "\"" << std::endl;
    }

    for ( const auto& postDep : m_pendingDepsSucc ) {
        std::string t1name = postDep.first;
        const auto& t2     = m_tasks[postDep.second];

        if ( std::find_if( m_tasks.begin(), m_tasks.end(), [=]( const auto& task ) {
                 return task->getName() == t1name;
             } ) == m_tasks.end() ) {
            t1name += "?";
        }
        output << "\"" << t1name << "\""
               << " -> ";
        output << "\"" << t2->getName() << "\"" << std::endl;
    }

    output << "}" << std::endl;
}
} // namespace Core
} // namespace Ra
