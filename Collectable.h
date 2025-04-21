// Collectable.h
#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <memory>

class Player; // Forward declaration

class Collectable {
public:
    enum class Type {
        HEALTH,
        MANA,
        DAMAGE_BOOST,
        SPEED_BOOST,
        FIRE_RATE
    };

    Collectable(sf::Vector2f position, Type type, float value);
    void update(float deltaTime);
    void draw(sf::RenderWindow& window) const;
    sf::FloatRect getBounds() const;
    Type getType() const { return type; }
    float getValue() const { return value; }
    bool isCollected() const { return collected; }
    void collect() { collected = true; }
    void applyEffect(Player& player);
sf::Vector2f getPosition() const { return shape.getPosition(); }
private:
    sf::Vector2f position;
    Type type;
    float value;
    bool collected = false;
    float floatTimer = 0.f;
    float floatOffset = 0.f;
    float lifeTime = 30.f; // Seconds before disappearing
    sf::CircleShape shape;
    sf::CircleShape pulseShape;

    void setupVisuals();
    
};
