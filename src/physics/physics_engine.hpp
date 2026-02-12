#pragma once

#include "../shared/command.hpp"
#include "../shared/event.hpp"
#include "../shared/level_data.hpp"
#include "../shared/thread_safe_queue.hpp"
#include "../shared/world_snapshot.hpp"

#include <vector>

namespace angry
{

class PhysicsEngine
{
public:
    void loadLevel(const LevelData& level);
    void step(float dt);
    void processCommands(ThreadSafeQueue<Command>& cmdQueue);

    WorldSnapshot getSnapshot() const;
    std::vector<Event> drainEvents();

private:
    struct SimObject
    {
        ObjectSnapshot snapshot;
        Vec2 velocityPx{};
        float hp = 1.0f;
    };

    void applyCommand(const Command& cmd);
    void refreshSnapshot();
    void tryPrepareNextProjectile();

    EntityId nextId_ = 1;
    WorldSnapshot snapshot_{};
    std::vector<SimObject> objects_;
    std::vector<Event> events_;
    std::vector<Command> pendingCommands_;

    LevelData currentLevel_{};
    bool levelLoaded_ = false;
    bool paused_ = false;

    int nextProjectileIndex_ = 0;
    int currentProjectileObjectIndex_ = -1;
};

}  // namespace angry
