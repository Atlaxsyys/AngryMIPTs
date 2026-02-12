#include "physics_engine.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <type_traits>
#include <utility>

namespace angry
{
namespace
{

inline float clampLength(float value, float minVal, float maxVal)
{
    return std::max(minVal, std::min(value, maxVal));
}

}  // namespace

void PhysicsEngine::loadLevel(const LevelData& level)
{
    currentLevel_ = level;
    levelLoaded_ = true;
    paused_ = false;

    nextId_ = 1;
    nextProjectileIndex_ = 0;
    currentProjectileObjectIndex_ = -1;
    events_.clear();
    objects_.clear();

    snapshot_.objects.clear();
    snapshot_.slingshot.basePx = level.slingshot.positionPx;
    snapshot_.slingshot.pullOffsetPx = {0.0f, 0.0f};
    snapshot_.slingshot.maxPullPx = level.slingshot.maxPullPx;
    snapshot_.slingshot.canShoot = !level.projectiles.empty();
    snapshot_.slingshot.nextProjectile = level.projectiles.empty()
        ? ProjectileType::Standard
        : level.projectiles.front().type;

    snapshot_.score = 0;
    snapshot_.shotsRemaining = static_cast<int>(level.projectiles.size());
    snapshot_.totalShots = level.meta.totalShots > 0
        ? level.meta.totalShots
        : static_cast<int>(level.projectiles.size());
    snapshot_.status = LevelStatus::Running;
    snapshot_.stars = 0;
    snapshot_.physicsStepMs = 0.0f;

    for (const BlockData& block : level.blocks)
    {
        SimObject object;
        object.snapshot.id = nextId_++;
        object.snapshot.kind = ObjectSnapshot::Kind::Block;
        object.snapshot.positionPx = block.positionPx;
        object.snapshot.angleDeg = block.angleDeg;
        object.snapshot.sizePx = block.sizePx;
        object.snapshot.radiusPx = block.radiusPx;
        object.snapshot.material = block.material;
        object.snapshot.hpNormalized = 1.0f;
        object.snapshot.isActive = true;
        object.hp = std::max(1.0f, block.hp);
        objects_.push_back(object);
    }

    for (const TargetData& target : level.targets)
    {
        SimObject object;
        object.snapshot.id = nextId_++;
        object.snapshot.kind = ObjectSnapshot::Kind::Target;
        object.snapshot.positionPx = target.positionPx;
        object.snapshot.angleDeg = 0.0f;
        object.snapshot.sizePx = {target.radiusPx * 2.0f, target.radiusPx * 2.0f};
        object.snapshot.radiusPx = target.radiusPx;
        object.snapshot.material = Material::Wood;
        object.snapshot.hpNormalized = 1.0f;
        object.snapshot.isActive = true;
        object.hp = std::max(1.0f, target.hp);
        objects_.push_back(object);
    }

    refreshSnapshot();
}

void PhysicsEngine::processCommands(ThreadSafeQueue<Command>& cmdQueue)
{
    while (const std::optional<Command> cmd = cmdQueue.try_pop())
    {
        pendingCommands_.push_back(*cmd);
    }
}

void PhysicsEngine::step(float dt)
{
    const auto start = std::chrono::steady_clock::now();

    if (!levelLoaded_ || paused_ || snapshot_.status != LevelStatus::Running)
    {
        snapshot_.physicsStepMs = 0.0f;
        return;
    }

    for (const Command& cmd : pendingCommands_)
    {
        applyCommand(cmd);
    }
    pendingCommands_.clear();

    const float clampedDt = clampLength(dt, 0.0f, 0.05f);
    const float gravityPx = PIXELS_PER_METER * 9.81f;

    for (size_t i = 0; i < objects_.size(); ++i)
    {
        SimObject& object = objects_[i];
        if (!object.snapshot.isActive)
        {
            continue;
        }

        if (object.snapshot.kind == ObjectSnapshot::Kind::Projectile)
        {
            object.velocityPx.y += gravityPx * clampedDt;
            object.snapshot.positionPx.x += object.velocityPx.x * clampedDt;
            object.snapshot.positionPx.y += object.velocityPx.y * clampedDt;

            if (object.snapshot.positionPx.y > 1400.0f
                || object.snapshot.positionPx.x > 2600.0f
                || object.snapshot.positionPx.x < -200.0f)
            {
                object.snapshot.isActive = false;
                if (static_cast<int>(i) == currentProjectileObjectIndex_)
                {
                    currentProjectileObjectIndex_ = -1;
                    tryPrepareNextProjectile();
                }
            }
        }
    }

    refreshSnapshot();

    if (snapshot_.shotsRemaining == 0 && currentProjectileObjectIndex_ < 0)
    {
        snapshot_.status = LevelStatus::Lose;
    }

    const auto end = std::chrono::steady_clock::now();
    snapshot_.physicsStepMs = std::chrono::duration<float, std::milli>(end - start).count();
}

WorldSnapshot PhysicsEngine::getSnapshot() const
{
    return snapshot_;
}

std::vector<Event> PhysicsEngine::drainEvents()
{
    std::vector<Event> out;
    out.swap(events_);
    return out;
}

void PhysicsEngine::applyCommand(const Command& cmd)
{
    std::visit(
        [this](const auto& concrete)
        {
            using T = std::decay_t<decltype(concrete)>;

            if constexpr (std::is_same_v<T, LoadLevelCmd>)
            {
                if (concrete.levelId == currentLevel_.meta.id)
                {
                    loadLevel(currentLevel_);
                }
            }
            else if constexpr (std::is_same_v<T, RestartCmd>)
            {
                if (concrete.levelId == currentLevel_.meta.id)
                {
                    loadLevel(currentLevel_);
                }
            }
            else if constexpr (std::is_same_v<T, PauseCmd>)
            {
                paused_ = concrete.paused;
            }
            else if constexpr (std::is_same_v<T, LaunchCmd>)
            {
                if (!levelLoaded_ || !snapshot_.slingshot.canShoot || snapshot_.shotsRemaining <= 0)
                {
                    return;
                }

                SimObject projectile;
                projectile.snapshot.id = nextId_++;
                projectile.snapshot.kind = ObjectSnapshot::Kind::Projectile;
                projectile.snapshot.positionPx = snapshot_.slingshot.basePx;
                projectile.snapshot.sizePx = {24.0f, 24.0f};
                projectile.snapshot.radiusPx = 12.0f;
                projectile.snapshot.hpNormalized = 1.0f;
                projectile.snapshot.isActive = true;
                projectile.hp = 1.0f;

                const float power = 4.5f;
                projectile.velocityPx.x = -concrete.pullVectorPx.x * power;
                projectile.velocityPx.y = -concrete.pullVectorPx.y * power;

                objects_.push_back(projectile);
                currentProjectileObjectIndex_ = static_cast<int>(objects_.size()) - 1;
                nextProjectileIndex_++;
                snapshot_.shotsRemaining = std::max(0, snapshot_.shotsRemaining - 1);
                snapshot_.slingshot.canShoot = false;
                snapshot_.slingshot.pullOffsetPx = {0.0f, 0.0f};

                if (nextProjectileIndex_ < static_cast<int>(currentLevel_.projectiles.size()))
                {
                    snapshot_.slingshot.nextProjectile = currentLevel_.projectiles[nextProjectileIndex_].type;
                }
            }
            else if constexpr (std::is_same_v<T, ActivateAbilityCmd>)
            {
                // v1 MVP: no abilities yet.
            }
        },
        cmd);
}

void PhysicsEngine::refreshSnapshot()
{
    snapshot_.objects.clear();
    snapshot_.objects.reserve(objects_.size());

    for (const SimObject& object : objects_)
    {
        snapshot_.objects.push_back(object.snapshot);
    }
}

void PhysicsEngine::tryPrepareNextProjectile()
{
    snapshot_.slingshot.canShoot = nextProjectileIndex_ < static_cast<int>(currentLevel_.projectiles.size());
    if (snapshot_.slingshot.canShoot)
    {
        snapshot_.slingshot.nextProjectile = currentLevel_.projectiles[nextProjectileIndex_].type;
        events_.push_back(ProjectileReadyEvent{snapshot_.slingshot.nextProjectile, snapshot_.shotsRemaining});
    }
}

}  // namespace angry
