#pragma once
#include "data/level_loader.hpp"
#include "physics/physics_engine.hpp"
#include "render/renderer.hpp"
#include "scene.hpp"
#include "shared/thread_safe_queue.hpp"
#include "shared/world_snapshot.hpp"
#include "ui/result_scene.hpp"
#include "ui/slingshot.hpp"

namespace angry
{

class GameScene : public Scene
{
private:
    Renderer renderer_;
    Slingshot slingshot_;
    PhysicsEngine physics_;
    ThreadSafeQueue<Command> command_queue_;
    LevelLoader level_loader_;
    WorldSnapshot snapshot_;
    sf::Font font_;
    sf::Text hud_text_;
    sf::Clock frame_clock_;
    LevelResult last_result_;

    static WorldSnapshot make_mock_snapshot();

public:
    explicit GameScene ( const sf::Font& font );

    const LevelResult& get_last_result() const { return last_result_; }

    void load_level ( int level_id );

    SceneId handle_input ( const sf::Event& event ) override;
    void update() override;
    void render ( sf::RenderWindow& window ) override;
};

}  // namespace angry
