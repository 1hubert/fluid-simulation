#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <iostream>

struct Droplet {
    sf::CircleShape shape;
    sf::Vector2f velocity;

    Droplet(float radius) : shape(radius), velocity(0.f, 0.f) {}
};

int main() {
    // Create window
    sf::RenderWindow window(
        sf::VideoMode(800, 600),
        "Fluid Sim",
        sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);
    const float deltaTime = 1.f / 60.f;

    // Calculate grid parameters
    const int GRID_SIZE = 10;
    const float DROPLET_RADIUS = 10.f;
    const float GAP = 1.f;  // Gap between droplets
    const float SPACING =   DROPLET_RADIUS * 2 + GAP;  // Total space between droplet centers

    // Physics parameters
    const float GRAVITY = 400.f;  // Pixels per second squared
    const float DAMPING = 0.1f;   // Energy loss in collisions

    // Border parameters
    const float BORDER_THICKNESS = 4.f;
    const float BORDER_PADDING = 20.f;
    sf::RectangleShape border;
    border.setPosition(BORDER_PADDING, BORDER_PADDING);
    border.setSize(sf::Vector2f(
        window.getSize().x - 2 * BORDER_PADDING,
        window.getSize().y - 2 * BORDER_PADDING
    ));
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineColor(sf::Color::White);
    border.setOutlineThickness(BORDER_THICKNESS);

    // Calculate grid dimensions and starting position
    const float GRID_WIDTH = GRID_SIZE * SPACING;
    const float GRID_HEIGHT = GRID_SIZE * SPACING;
    const float startX = (window.getSize().x - GRID_WIDTH) / 2;
    const float startY = (window.getSize().y - GRID_HEIGHT) / 2;

    // Create droplets
    std::vector<Droplet> droplets;
    droplets.reserve(GRID_SIZE * GRID_SIZE);

    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            Droplet droplet(DROPLET_RADIUS);
            droplet.shape.setPosition(
                startX + (col * SPACING),
                startY + (row * SPACING)
            );
            droplet.shape.setFillColor(sf::Color::Cyan);
            droplets.push_back(droplet);
        }
    }

    while (window.isOpen()) {
        // Event stuff
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Physics update
        for (auto& droplet : droplets) {
            // Apply gravity
            droplet.velocity.y += GRAVITY * deltaTime;

            // Update position
            droplet.shape.move(droplet.velocity * deltaTime);

            // Border collision
            sf::Vector2f pos = droplet.shape.getPosition();

            // Left border
            if (pos.x < BORDER_PADDING + BORDER_THICKNESS) {
                droplet.shape.setPosition(BORDER_PADDING + BORDER_THICKNESS, pos.y);
                droplet.velocity.x = -droplet.velocity.x * DAMPING;
            }
            // Right border
            if (pos.x + DROPLET_RADIUS * 2 > window.getSize().x - BORDER_PADDING - BORDER_THICKNESS) {
                droplet.shape.setPosition(
                    window.getSize().x - BORDER_PADDING - BORDER_THICKNESS - DROPLET_RADIUS * 2,
                    pos.y
                );
                droplet.velocity.x = -droplet.velocity.x * DAMPING;
            }
            // Top border
            if (pos.y < BORDER_PADDING + BORDER_THICKNESS) {
                droplet.shape.setPosition(pos.x, BORDER_PADDING + BORDER_THICKNESS);
                droplet.velocity.y = -droplet.velocity.y * DAMPING;
            }
            // Bottom border
            if (pos.y + DROPLET_RADIUS * 2 > window.getSize().y - BORDER_PADDING - BORDER_THICKNESS) {
                droplet.shape.setPosition(
                    pos.x,
                    window.getSize().y - BORDER_PADDING - BORDER_THICKNESS - DROPLET_RADIUS * 2
                );
                droplet.velocity.y = -droplet.velocity.y * DAMPING;
            }
        }

        // Droplet collision
        for (size_t i = 0; i < droplets.size(); i++) {
            for (size_t j = i + 1; j < droplets.size(); j++) {
                sf::Vector2f pos1 = droplets[i].shape.getPosition();
                sf::Vector2f pos2 = droplets[j].shape.getPosition();

                // Add radius to get center position
                pos1.x += DROPLET_RADIUS;
                pos1.y += DROPLET_RADIUS;
                pos2.x += DROPLET_RADIUS;
                pos2.y += DROPLET_RADIUS;

                sf::Vector2f diff = pos1 - pos2;
                float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

                if (dist < DROPLET_RADIUS * 2) {
                    // Collision response
                    sf::Vector2f normal = diff / dist;
                    float overlap = DROPLET_RADIUS * 2 - dist;

                    // Separate droplets
                    droplets[i].shape.move(normal * overlap * 0.5f);
                    droplets[j].shape.move(-normal * overlap * 0.5f);

                    // Calculate collision response
                    sf::Vector2f relativeVel = droplets[i].velocity - droplets[j].velocity;
                    float impulse = -(1 + DAMPING) * (relativeVel.x * normal.x + relativeVel.y * normal.y);

                    // Apply impulse
                    droplets[i].velocity += normal * impulse * 0.5f;
                    droplets[j].velocity -= normal * impulse * 0.5f;
                }
            }
        }

        // Render stuff
        window.clear();
        window.draw(border);
        for (const auto& droplet : droplets) {
            window.draw(droplet.shape);
        }
        window.display();
    }

    return 0;
}
