#include <iostream>
#include <SFML/Graphics.hpp>

using namespace std;

// pixels per second^2
float gravity = 400.f;

int main()
{
    sf::RenderWindow window(
        sf::VideoMode(800, 600),
        "Fluid Sim",
        sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);
    float deltaTime = 1.f / 60.f;

    sf::CircleShape droplet(10.f);
    droplet.setPosition(300.f, 400.f);
    droplet.setFillColor(sf::Color::Cyan);

    sf::Vector2f velocity(0.f, 0.f);

    while (window.isOpen())
    {
        // Event stuff
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Update stuff
        velocity.y += gravity * deltaTime;
        cout << velocity.y << endl;
        droplet.move(velocity * deltaTime);

        // Render stuff
        window.clear();
        window.draw(droplet);
        window.display();
    }

    return 0;
}
