
#pragma once
#include <SFML/Graphics.hpp>

class Particle {
public:
    Particle(sf::Vector2f position, sf::Color color = sf::Color::White);
    ~Particle();
    void update(float deltaTime);
    void draw(sf::RenderWindow& window);
    bool isAlive() const;

private:
    sf::CircleShape shape;
    float lifetime;
    float maxLifetime;
};
