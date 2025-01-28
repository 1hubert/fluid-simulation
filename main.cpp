#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <array>
#include <sstream>
#include <iomanip>
#include <time.h>
#include <functional>


// onclick square /w collision
// parametrization for grid size

// CTRL+SHIFT+C to comment
// CTRL+SHIFT+X to UNcomment

bool show_menu = true;

class Slider {
private:
    sf::RectangleShape track;
    sf::CircleShape knob;
    sf::Font font;
    sf::Text valueText;
    sf::Text NameText;

    float trackStartX, trackEndX;
    int minValue, maxValue;
    int currentValue;
    bool isDragging = false;
public:
    Slider(float x, float y, float width, int minValue, int maxValue, std::string name)
        : minValue(minValue), maxValue(maxValue), currentValue(minValue) {
        // Set up the track
        track.setSize(sf::Vector2f(width, 5));
        track.setFillColor(sf::Color::White);
        track.setPosition(x, y);

        // Set up the knob
        knob.setRadius(10);
        knob.setFillColor(sf::Color::Red);
        knob.setOrigin(knob.getRadius(), knob.getRadius());
        knob.setPosition(x, y + track.getSize().y / 2);

        // Calculate the bounds
        trackStartX = x;
        trackEndX = x + width;

        // Load the font for text
        if (!font.loadFromFile("./resources/tuffy.ttf")) { // Ensure you have a font file in your project directory
            std::cerr << "Failed to load font\n";
        }

        // Value display
        valueText.setFont(font);
        valueText.setCharacterSize(16);
        valueText.setFillColor(sf::Color::White);
        valueText.setString(std::to_string(currentValue));
        valueText.setPosition(x + width + 20, y - 5);

        // Name
        NameText.setFont(font);
        NameText.setCharacterSize(21);
        NameText.setFillColor(sf::Color::White);
        NameText.setString(name);
        NameText.setPosition(x + width/3, y - 36);
    }

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
            if (knob.getGlobalBounds().contains(mousePos)) {
                isDragging = true;
            }
        } else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
            isDragging = false;
        } else if (event.type == sf::Event::MouseMoved && isDragging) {
            float mouseX = static_cast<float>(event.mouseMove.x);
            if (mouseX < trackStartX) mouseX = trackStartX;
            if (mouseX > trackEndX) mouseX = trackEndX;
            knob.setPosition(mouseX, knob.getPosition().y);

            // Map mouseX to the range of minValue to maxValue
            float percentage = (mouseX - trackStartX) / (trackEndX - trackStartX);
            currentValue = static_cast<int>(minValue + percentage * (maxValue - minValue));
            valueText.setString(std::to_string(currentValue));
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(track);
        window.draw(knob);
        window.draw(valueText);
        window.draw(NameText);
    }

    int getValue() const {
        return currentValue;
    }
};

class Button {
private:
    sf::RectangleShape shape;
    sf::Text text;
    sf::Font font;
    bool enabled = true;
    std::function<void()> callback;
public:
    Button(float x, float y, float width, float height, const std::string& label, const sf::Font& font) {
        shape.setPosition(x, y);
        shape.setSize(sf::Vector2f(width, height));
        shape.setFillColor(sf::Color::Blue);
        shape.setOutlineColor(sf::Color::Black);
        shape.setOutlineThickness(2);

        this->font = font;
        text.setFont(this->font);
        text.setString(label);
        text.setCharacterSize(20);
        text.setFillColor(sf::Color::White);

        // Center text
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setOrigin(textBounds.left + textBounds.width / 2.0f, textBounds.top + textBounds.height / 2.0f);
        text.setPosition(x + width / 2.0f, y + height / 2.0f);
    }

    void setCallback(const std::function<void()>& func) {
        callback = func;
    }

    void setEnabled(bool value) {
        enabled = value;
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
        window.draw(text);
    }

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
        if (enabled && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            if (shape.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
                if (callback) callback(); // Run the given function
            }
        }
    }

    void setFillColor(const sf::Color& color) {
        shape.setFillColor(color);
    }

    void setTextColor(const sf::Color& color) {
        text.setFillColor(color);
    }
};


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

    std::string getFPSString() const {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << fps << " FPS";
        return ss.str();
    }

    void draw(sf::RenderWindow& window, const sf::Font& font,
             unsigned int characterSize = 20,
             sf::Vector2f position = sf::Vector2f(27, 25)) {
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

    void removeAllParticles() {

        particles.clear();
    }

    void update(float dt) {
        computeDensityPressure();
        computeForces();
        integrate(dt);
    }

    void shake() {
        for (auto& p : particles) {
                switch(rand() % 4) {
                case 0:
                    p.velocity += sf::Vector2f(0.f, static_cast<float>(rand() % 10000));
                    break;
                case 1:
                    p.velocity += sf::Vector2f(static_cast<float>(rand() % 10000), 0.f);
                    break;
                case 2:
                    p.velocity += sf::Vector2f(0.f, -static_cast<float>(rand() % 10000));
                    break;
                case 3:
                    p.velocity += sf::Vector2f(-static_cast<float>(rand() % 10000), 0.f);
                    break;
            }
        }
    }

    void wind(int direction, float force) {
        // 0123 - up right down left
        switch(direction) {
            case 0:
                for (auto& p : particles) {
                    p.velocity += sf::Vector2f(0.f, -force);
                }
                break;
            case 1:
                for (auto& p : particles) {
                    p.velocity += sf::Vector2f(force, 0.f);
                }
                break;
            case 2:
                for (auto& p : particles) {
                    p.velocity += sf::Vector2f(0.f, force);
                }
                break;
            case 3:
                for (auto& p : particles) {
                    p.velocity += sf::Vector2f(-force, 0.f);
                }
                break;
        }
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

        // Create a color gradient from blue (low pressure) to red (high pressure)
        sf::Color color(
            static_cast<sf::Uint8>(200 * pressure_scale),                    // Red
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

                        // Calculate impulse
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
            if (p.position.y + PARTICLE_RADIUS < bounds.top) {
                p.position.y = bounds.top - PARTICLE_RADIUS;
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
    // Seed rand
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

    // Square setup
//    sf::RectangleShape movableSquare(sf::Vector2f(50.f, 50.f));
//    movableSquare.setFillColor(sf::Color::Green);
//    movableSquare.setPosition(200.f, 200.f); // Initial position

    // Define FluidSimulator
    FluidSimulator simulator(bounds);

    // Button setup
    Button button_start(300, 200, 200, 50, "Start", font);
    Button button_reset(300, 260, 200, 50, "Reset", font);
    Slider slider_gridsize(300, 320, 200, 1, 35, "Grid Size");

    button_start.setCallback([&show_menu, &bounds, &simulator, &button_reset, &button_start, &slider_gridsize]() {
        show_menu = false;
        button_reset.setEnabled(true);
        button_start.setEnabled(false);

        const int GRID_SIZE = slider_gridsize.getValue();
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
    });

    button_reset.setCallback([&show_menu, &simulator, &button_reset, &button_start]() {
        show_menu = true;
        button_reset.setEnabled(false);
        button_start.setEnabled(true);

        simulator.removeAllParticles();
    });

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            slider_gridsize.handleEvent(event, window);
            switch (event.type) {
                case sf::Event::Closed:
                    window.close();
                    break;
                case sf::Event::KeyPressed:
                    switch(event.key.code) {
                    case sf::Keyboard::Q:
                        window.close();
                        break;
                    case sf::Keyboard::Space:
                        simulator.shake();
                        break;
                    case sf::Keyboard::Up:
                        simulator.wind(0, 10.f);
                        break;
                    case sf::Keyboard::Right:
                        simulator.wind(1, 10.f);
                        break;
                    case sf::Keyboard::Down:
                        simulator.wind(2, 10.f);
                        break;
                    case sf::Keyboard::Left:
                        simulator.wind(3, 10.f);
                        break;
                    default:
                        break;
                    }

                    break;
                case sf::Event::MouseButtonPressed:
                    button_start.handleEvent(event, window);
                    button_reset.handleEvent(event, window);
                    break;
                default:
                    break;
            }
        }

//         Move the square based on mouse position
//        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
//            sf::Vector2i mousePosition = sf::Mouse::getPosition(window);
//             Constrain square within the border
//            sf::Vector2f newPos(
//                std::clamp(static_cast<float>(mousePosition.x) - movableSquare.getSize().x / 2.f,
//                bounds.left, bounds.left + bounds.width - movableSquare.getSize().x),
//                std::clamp(static_cast<float>(mousePosition.y) - movableSquare.getSize().y / 2.f,
//                bounds.top, bounds.top + bounds.height - movableSquare.getSize().y)
//            );
//            movableSquare.setPosition(newPos);
//        }

        fps_counter.update();

        //simulator.checkSquareCollision(movableSquare);

        window.clear();

        //window.draw(movableSquare);

        if (show_menu) {
            button_start.draw(window);
            slider_gridsize.draw(window);
        } else {
            simulator.update(DELTA_TIME);
            simulator.draw(window);
            button_reset.draw(window);
        }

        window.draw(border);
        fps_counter.draw(window, font);
        window.display();
    }

    return 0;
}
