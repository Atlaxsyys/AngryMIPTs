#include <SFML/Graphics.hpp>

int main()
{
    sf::RenderWindow window ( sf::VideoMode( {1280, 720} ), "AngryMipts" );
    window.setFramerateLimit ( 60 );

    while ( window.isOpen() )
    {
        while ( const auto event = window.pollEvent() )
        {
            if ( event->is<sf::Event::Closed>() )
                window.close();

            if ( const auto* key = event->getIf<sf::Event::KeyPressed>() )
            {
                if ( key->code == sf::Keyboard::Key::Escape )
                    window.close();
            }
        }

        window.clear ( sf::Color( 135, 206, 235 ) );
        window.display();
    }

    return 0;
}
