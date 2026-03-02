#pragma once
#include "scene.hpp"

namespace angry
{

struct LevelResult
{
    bool win = false;
    int score = 0;
    int stars = 0;
};

class ResultScene : public Scene
{
private:
    sf::Font font_;
    sf::Text title_;
    sf::Text score_text_;
    sf::Text stars_text_;
    sf::Text prompt_;

    LevelResult result_;

public:
    explicit ResultScene ( const sf::Font& font );

    void set_result ( const LevelResult& result );

    SceneId handle_input ( const sf::Event& event ) override;
    void update() override;
    void render ( sf::RenderWindow& window ) override;
};

}  // namespace angry
