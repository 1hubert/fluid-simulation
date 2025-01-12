#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

struct Particle {
    sf::CircleShape shape;
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Vector2f force;
    float density;
    float pressure;

    Particle(float radius) : shape(radius), density(0.f), pressure(0.f) {
        shape.setFillColor(sf::Color::Cyan);
    }
};

class FluidSimulator {
private:
    sf::Vector2f gravity;
    sf::FloatRect bounds;
    std::vector<Particle> particles;

    // Adjusted physics constants for more stable simulation
    const float PARTICLE_MASS = 100.0f;          // Reduced from 100.0f
    const float REST_DENSITY = 50.f;           // Reduced from 100.f
    const float GAS_CONSTANT = 100.f;           // Reduced from 20.f
    const float VISCOSITY = 7000.f;               // Reduced from 2.f
    const float SMOOTHING_LENGTH = 15.f;       // Adjusted from 13.f
    const float PARTICLE_RADIUS = 5.f;         // Made explicit for collision
    const float SMOOTHING_LENGTH_SQ = SMOOTHING_LENGTH * SMOOTHING_LENGTH;
    const float POLY6_SCALE = 315.f / (64.f * static_cast<float>(M_PI) * std::pow(SMOOTHING_LENGTH, 9));
    const float SPIKY_GRAD_SCALE = -45.f / (static_cast<float>(M_PI) * std::pow(SMOOTHING_LENGTH, 6));
    const float VISC_LAP_SCALE = 45.f / (static_cast<float>(M_PI) * std::pow(SMOOTHING_LENGTH, 6));
    const float MAX_VELOCITY = 300.f;          // Added velocity clamping

public:
    FluidSimulator(const sf::FloatRect& boundsRect, const sf::Vector2f& gravityVec = sf::Vector2f(0.f, 981.f))
        : gravity(gravityVec), bounds(boundsRect) {}

    void addParticle(const sf::Vector2f& pos) {
        Particle p(PARTICLE_RADIUS);
        p.position = pos;
        p.velocity = sf::Vector2f(0.f, 0.f);
        p.force = sf::Vector2f(0.f, 0.f);
        p.shape.setPosition(pos);
        particles.push_back(p);
    }

    void update(float dt) {
        computeDensityPressure();
        computeForces();
        integrate(dt);
    }

    void draw(sf::RenderWindow& window) {
        for (auto& p : particles) {
            float pressure_scale = std::min(p.pressure / (GAS_CONSTANT * REST_DENSITY), 1.0f);
            sf::Color color(
                static_cast<sf::Uint8>(51 + 204 * pressure_scale),
                static_cast<sf::Uint8>(153 + 102 * (1.0f - pressure_scale)),
                255
            );
            p.shape.setFillColor(color);
            p.shape.setPosition(p.position);
            window.draw(p.shape);
        }
    }

private:
    void computeDensityPressure() {
        for (auto& pi : particles) {
            pi.density = 0.f;
            for (auto& pj : particles) {
                sf::Vector2f diff = pi.position - pj.position;
                float r2 = diff.x * diff.x + diff.y * diff.y;

                if (r2 < SMOOTHING_LENGTH_SQ) {
                    pi.density += PARTICLE_MASS * POLY6_SCALE * std::pow(SMOOTHING_LENGTH_SQ - r2, 3.f);
                }
            }
            pi.pressure = GAS_CONSTANT * (pi.density - REST_DENSITY);
        }
    }

    void computeForces() {
        for (auto& pi : particles) {
            sf::Vector2f pressure_force(0.f, 0.f);
            sf::Vector2f viscosity_force(0.f, 0.f);

            for (auto& pj : particles) {
                if (&pi == &pj) continue;

                sf::Vector2f diff = pi.position - pj.position;
                float r = std::sqrt(diff.x * diff.x + diff.y * diff.y);

                if (r < SMOOTHING_LENGTH && r > 0.0001f) { // Added minimum distance check
                    // Pressure force
                    float pressure_scale = (pi.pressure + pj.pressure) / (2.f * pi.density * pj.density);
                    sf::Vector2f normalized_diff = diff / r;
                    pressure_force += normalized_diff * (PARTICLE_MASS * pressure_scale *
                        SPIKY_GRAD_SCALE * std::pow(SMOOTHING_LENGTH - r, 2.f));

                    // Viscosity force
                    viscosity_force += (pj.velocity - pi.velocity) *
                        (PARTICLE_MASS * VISCOSITY / pj.density * VISC_LAP_SCALE * (SMOOTHING_LENGTH - r));
                }
            }

            // Limit force magnitude
            pi.force = pressure_force + viscosity_force + gravity * pi.density;
            float force_magnitude = std::sqrt(pi.force.x * pi.force.x + pi.force.y * pi.force.y);
            if (force_magnitude > MAX_VELOCITY * pi.density) {
                pi.force *= (MAX_VELOCITY * pi.density / force_magnitude);
            }
        }
    }

    void integrate(float dt) {
        const float DAMPING = 0.4f;  // Increased from 0.2f

        for (auto& p : particles) {
            // Update velocity with force
            p.velocity += dt * p.force / p.density;

            // Clamp velocity magnitude
            float speed = std::sqrt(p.velocity.x * p.velocity.x + p.velocity.y * p.velocity.y);
            if (speed > MAX_VELOCITY) {
                p.velocity *= MAX_VELOCITY / speed;
            }

            // Update position
            p.position += dt * p.velocity;

            // Border collision with particle radius
            if (p.position.x + PARTICLE_RADIUS < bounds.left) {
                p.position.x = bounds.left - PARTICLE_RADIUS;
                p.velocity.x *= -DAMPING;
            }
            if (p.position.x + PARTICLE_RADIUS > bounds.left + bounds.width) {
                p.position.x = bounds.left + bounds.width - PARTICLE_RADIUS;
                p.velocity.x *= -DAMPING;
            }
            if (p.position.y - PARTICLE_RADIUS < bounds.top) {
                p.position.y = bounds.top + PARTICLE_RADIUS;
                p.velocity.y *= -DAMPING;
            }
            if (p.position.y + PARTICLE_RADIUS > bounds.top + bounds.height) {
                p.position.y = bounds.top + bounds.height - PARTICLE_RADIUS;
                p.velocity.y *= -DAMPING;
            }
        }
    }
};

// Main function remains the same
int main() {
    sf::RenderWindow window(
        sf::VideoMode(800, 600),
        "SPH Fluid Simulation",
        sf::Style::Titlebar | sf::Style::Close
    );
    window.setFramerateLimit(60);

    const float BORDER_PADDING = 20.f;
    const float BORDER_THICKNESS = 4.f;
    sf::RectangleShape border;
    border.setPosition(BORDER_PADDING, BORDER_PADDING);
    border.setSize(sf::Vector2f(
        window.getSize().x - 2 * BORDER_PADDING,
        window.getSize().y - 2 * BORDER_PADDING
    ));
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineColor(sf::Color::White);
    border.setOutlineThickness(BORDER_THICKNESS);

    sf::FloatRect bounds(
        BORDER_PADDING + BORDER_THICKNESS,
        BORDER_PADDING + BORDER_THICKNESS,
        window.getSize().x - 2 * (BORDER_PADDING + BORDER_THICKNESS),
        window.getSize().y - 2 * (BORDER_PADDING + BORDER_THICKNESS)
    );

    FluidSimulator simulator(bounds);

    const int GRID_SIZE = 30;
    const float SPACING = 12.f;
    const float startX = bounds.left + bounds.width * 0.25f;
    const float startY = bounds.top + bounds.height * 0.25f;

    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            simulator.addParticle(sf::Vector2f(
                startX + col * SPACING,
                startY + row * SPACING
            ));
        }
    }

    sf::Clock clock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    window.close();
                    break;
                case sf::Event::KeyPressed:
                    if (event.key.code == sf::Keyboard::Q)
                        window.close();
                    break;


            }
        }

        float dt = std::min(clock.restart().asSeconds(), 1.f / 60.f);
        simulator.update(dt);

        window.clear();
        window.draw(border);
        simulator.draw(window);
        window.display();
    }

    return 0;
}
