#pragma once

#include "physics_engine.hpp"

#include "../shared/command.hpp"
#include "../shared/event.hpp"
#include "../shared/level_data.hpp"
#include "../shared/thread_safe_queue.hpp"
#include "../shared/world_snapshot.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace angry
{

// Stage 1 skeleton: lifecycle + API surface, no worker thread yet.
class PhysicsThread
{
public:
    PhysicsThread() = default;
    ~PhysicsThread();

    void start();
    void stop();
    bool isRunning() const;

    void registerLevel(const LevelData& level);
    void loadLevel(const LevelData& level);
    void pushCommand(const Command& cmd);

    // Temporary adapter for single-thread integration while worker loop is not introduced yet.
    void tickSingleThread(float dt);

    WorldSnapshot readSnapshot() const;
    std::vector<Event> drainEvents();

private:
    void workerLoop();

    static constexpr float kFixedDtSec = 1.0f / 60.0f;

    mutable std::mutex mutex_;
    std::condition_variable stopCv_;
    std::thread worker_;
    std::atomic<bool> stopRequested_{false};
    PhysicsEngine engine_;
    ThreadSafeQueue<Command> commandQueue_;
    ThreadSafeQueue<Event> eventQueue_;
    bool running_ = false;
};

}  // namespace angry
