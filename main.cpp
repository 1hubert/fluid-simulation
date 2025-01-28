#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <array>
#include <sstream>
#include <iomanip>
#include <time.h>

// todo
// add wind
// przede wszystkim zrobi� parametryzacj� jak mieszko zrobi�

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

float dot(const sf::Vector2f& a, const sf::Vector2f& b) {
    return a.x * b.x + a.y * b.y;
}

class FPSCounter {
private:
    float fps;
    sf::Clock clock;
    sf::Time previousTime;

    // For smoothing
    static const int SAMPLE_SIZE = 10;
    std::array<float, SAMPLE_SIZE> fpsHistory;
    int currentSample = 0;

public:
    FPSCounter() : fps(0.0f), currentSample(0) {
        previousTime = clock.getElapsedTime();
        fpsHistory.fill(0.0f);
    }

    void update() {
        sf::Time currentTime = clock.getElapsedTime();
        sf::Time deltaTime = currentTime - previousTime;
        previousTime = currentTime;

        // Calculate instantaneous fps
        float currentFps = 1.0f / deltaTime.asSeconds();

        // Store in circular buffer
        fpsHistory[currentSample] = currentFps;
        currentSample = (currentSample + 1) % SAMPLE_SIZE;

        // Calculate average fps
        float sum = 0.0f;
        for (float sample : fpsHistory) {
            sum += sample;
        }
        fps = sum / SAMPLE_SIZE;
    }

    // Get FPS as formatted string
    std::string getFPSString() const {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << fps << " FPS";
        return ss.str();
    }

    // Optional: Draw FPS counter directly to window
    void draw(sf::RenderWindow& window, const sf::Font& font,
             unsigned int characterSize = 20,
             sf::Vector2f position = sf::Vector2f(10, 10)) {
        sf::Text text;
        text.setFont(font);
        text.setString(getFPSString());
        text.setCharacterSize(characterSize);
        text.setFillColor(sf::Color::White);
        text.setPosition(position);
        window.draw(text);
    }
};

class FluidSimulator {
private:
    sf::Vector2f gravity;
    sf::FloatRect bounds;
    std::vector<Particle> particles;

    const float PARTICLE_MASS = 5.0f;
    const float REST_DENSITY = 1000.f;
    const float GAS_CONSTANT = 100.f;
    const float VISCOSITY = 7000.f;
    const float SMOOTHING_LENGTH = 15.f;
    const float PARTICLE_RADIUS = 5.f;
    const float SMOOTHING_LENGTH_SQ = SMOOTHING_LENGTH * SMOOTHING_LENGTH;
    const float POLY6_SCALE = 315.f / (64.f * M_PI * std::pow(SMOOTHING_LENGTH, 4));
    const float SPIKY_GRAD_SCALE = -45.f / (M_PI * std::pow(SMOOTHING_LENGTH, 6));
    const float VISC_LAP_SCALE = 45.f / (M_PI * std::pow(SMOOTHING_LENGTH, 6));
    const float MAX_VELOCITY = 300.f;

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
    // Find max pressure in current frame for dynamic scaling
    float max_pressure = 0.0f;
    for (const auto& p : particles) {
        max_pressure = std::max(max_pressure, p.pressure);

    }

    // Avoid division by zero
    max_pressure = std::max(max_pressure, 0.0001f);

    for (auto& p : particles) {
        // Normalize pressure between 0 and 1
        float pressure_scale = p.pressure / max_pressure;

        // Apply smoothstep for more pleasing color transitions
        pressure_scale = pressure_scale * pressure_scale * (3 - 2 * pressure_scale);

        // Create a color gradient from blue (low pressure) to red (high pressure)
        sf::Color color(
            static_cast<sf::Uint8>(255 * pressure_scale),                    // Red
            static_cast<sf::Uint8>(100 * (1.0f - pressure_scale)),          // Green
            static_cast<sf::Uint8>(255 * (1.0f - pressure_scale))           // Blue
        );

        p.shape.setFillColor(color);
        p.shape.setPosition(p.position);
        window.draw(p.shape);
    }
}

    void checkSquareCollision(sf::RectangleShape& square) {
// todo
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
            //std::cout << pi.pressure << std::endl;
            //std::cout << "Particle Density: " << pi.density << ", Pressure: " << pi.pressure << std::endl;
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

                if (r < SMOOTHING_LENGTH && r > 0.0001f) {
                    // Pressure force
                    float pressure_scale = (pi.pressure + pj.pressure) / (2.f * pi.density * pj.density);
                    sf::Vector2f normalized_diff = diff / r;
                    pressure_force += normalized_diff * (PARTICLE_MASS * pressure_scale *
                        SPIKY_GRAD_SCALE * std::pow(SMOOTHING_LENGTH - r, 2.f));

                    // Viscosity force
                    viscosity_force += (pj.velocity - pi.velocity) *
                        (PARTICLE_MASS * VISCOSITY / pj.density * VISC_LAP_SCALE * (SMOOTHING_LENGTH - r));
                }


                // Check for overlap (distance between particles < 2 * radius)
                if (r < 2 * PARTICLE_RADIUS) {
                    sf::Vector2f normalized_diff = diff / r; // Collision normal

                    // Calculate relative velocity
                    sf::Vector2f relative_velocity = pi.velocity - pj.velocity;

                    // Normal velocity component (along collision normal)
                    float normal_velocity = dot(relative_velocity, normalized_diff);

                    // Only resolve if particles are moving toward each other
                    if (normal_velocity < 0) {
                        // Coefficient of restitution (1.0 = perfectly elastic)
                        const float RESTITUTION = 0.8f;

                        // Calculate impulse scalar
                        float impulse = -(1.0f + RESTITUTION) * normal_velocity;
                        impulse /= 2.0f; // Assuming equal mass for both particles

                        // Apply impulse
                        pi.velocity += normalized_diff * impulse;
                        pj.velocity -= normalized_diff * impulse;

                        // Separate particles to prevent overlap
                        float overlap = 2 * PARTICLE_RADIUS - r;
                        sf::Vector2f separation = normalized_diff * (overlap * 0.5f);
                        pi.position += separation;
                        pj.position -= separation;

                        // Clear forces since we're handling collision response through velocity
                        pi.force = sf::Vector2f(0.0f, 0.0f);
                        pj.force = sf::Vector2f(0.0f, 0.0f);
                    }
                }
            }

            // Combine all forces: pressure, viscosity, and gravity
            pi.force = pressure_force + viscosity_force + gravity * pi.density;

            // Limit force magnitude
            float force_magnitude = std::sqrt(pi.force.x * pi.force.x + pi.force.y * pi.force.y);
            if (force_magnitude > MAX_VELOCITY * pi.density) {
                pi.force *= (MAX_VELOCITY * pi.density / force_magnitude);
            }
        }
    }

    void integrate(float dt) {
        const float DAMPING = 0.4f;

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

int main() {
    // Seed
    srand(time(NULL));

    // SFML Window Setup
    sf::RenderWindow window(
        sf::VideoMode(800, 600),
        "Fluid Sim",
        sf::Style::Titlebar | sf::Style::Close
    );
    window.setFramerateLimit(60);
    const float DELTA_TIME = 1.f / 60.f;

    // FPS Counter Setup
    sf::Font font;
    font.loadFromFile("./resources/tuffy.ttf");
    FPSCounter fps_counter;

    // Border Setup
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


    // Fluid Simulator Setup
    FluidSimulator simulator(bounds);

    const int GRID_SIZE = 40;
    const float SPACING = 12.f;
    const float startX = bounds.left + bounds.width * 0.25f;
    const float startY = bounds.top + bounds.height * 0.25f;

    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            simulator.addParticle(sf::Vector2f(
                startX + col * SPACING - 1 + (rand() % 3),
                startY + row * SPACING - 1 + (rand() % 3)
            ));
        }
    }

    // Create the movable square
    sf::RectangleShape movableSquare(sf::Vector2f(50.f, 50.f));
    movableSquare.setFillColor(sf::Color::Green);
    movableSquare.setPosition(200.f, 200.f); // Initial position

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

        // Move the square based on mouse position
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            sf::Vector2i mousePosition = sf::Mouse::getPosition(window);
            // Constrain square within the border
            sf::Vector2f newPos(
                std::clamp(static_cast<float>(mousePosition.x) - movableSquare.getSize().x / 2.f,
                bounds.left, bounds.left + bounds.width - movableSquare.getSize().x),
                std::clamp(static_cast<float>(mousePosition.y) - movableSquare.getSize().y / 2.f,
                bounds.top, bounds.top + bounds.height - movableSquare.getSize().y)
            );
            movableSquare.setPosition(newPos);
        }

        simulator.checkSquareCollision(movableSquare);
        fps_counter.update();
        simulator.update(DELTA_TIME);
        window.clear();
        window.draw(border);
        window.draw(movableSquare);
        fps_counter.draw(window, font);
        simulator.draw(window);
        window.display();
    }

    return 0;
}
