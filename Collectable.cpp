
// Collectable.cpp
#include "Collectable.h"
#include "Player.h"
#include <cmath>

Collectable::Collectable(sf::Vector2f position, Type type, float value)
    : position(position), type(type), value(value) {
    setupVisuals();
}

void Collectable::setupVisuals() {
    // Main shape
    shape.setRadius(16.f);
    shape.setOrigin(16.f, 16.f);
    shape.setPosition(position);

    // Pulse effect shape
    pulseShape.setRadius(20.f);
    pulseShape.setOrigin(20.f, 20.f);
    pulseShape.setPosition(position);
    pulseShape.setFillColor(sf::Color::Transparent);
    pulseShape.setOutlineThickness(2.f);

    switch (type) {
    case Type::HEALTH:
        shape.setFillColor(sf::Color(0, 255, 0, 200));
        pulseShape.setOutlineColor(sf::Color(50, 255, 50, 150));
        break;
    case Type::MANA:
        shape.setFillColor(sf::Color(0, 100, 255, 200));
        pulseShape.setOutlineColor(sf::Color(100, 150, 255, 150));
        break;
    case Type::DAMAGE_BOOST:
        shape.setFillColor(sf::Color(255, 50, 50, 200));
        pulseShape.setOutlineColor(sf::Color(255, 100, 100, 150));
        break;
    case Type::SPEED_BOOST:
        shape.setFillColor(sf::Color(255, 255, 0, 200));
        pulseShape.setOutlineColor(sf::Color(255, 255, 100, 150));
        break;
    case Type::FIRE_RATE:
        shape.setFillColor(sf::Color(255, 0, 255, 200));
        pulseShape.setOutlineColor(sf::Color(255, 100, 255, 150));
        break;
    }

    shape.setOutlineThickness(1.f);
    shape.setOutlineColor(sf::Color::White);
}

void Collectable::update(float deltaTime) {
    if (collected) return;

    // Floating animation
    floatTimer += deltaTime;
    floatOffset = std::sin(floatTimer * 2.f) * 3.f;
    shape.setPosition(position.x, position.y + floatOffset);
    pulseShape.setPosition(position.x, position.y + floatOffset);

    // Pulse animation
    float pulseScale = 1.0f + 0.1f * std::sin(floatTimer * 3.f);
    pulseShape.setScale(pulseScale, pulseScale);

    // Lifetime
    lifeTime -= deltaTime;
    if (lifeTime <= 0.f) collected = true;
}

void Collectable::draw(sf::RenderWindow& window) const {
    if (!collected) {
        window.draw(pulseShape);
        window.draw(shape);

        // Draw icon based on type (would need texture)
    }
}

sf::FloatRect Collectable::getBounds() const {
    return shape.getGlobalBounds();
}

void Collectable::applyEffect(Player& player) {
    switch (type) {
    case Type::HEALTH:
        player.heal(value);
        break;
    case Type::MANA:
        player.addMana(value);
        break;
    case Type::DAMAGE_BOOST:
        player.applyDamageBoost(value, 10.f); // 10 second duration
        break;
    case Type::SPEED_BOOST:
        player.applySpeedBoost(value * 0.5f, 8.f); // +50% speed for 8 sec
        break;
    case Type::FIRE_RATE:
        player.applyFireRateBoost(value * 0.3f, 7.f); // +30% rate for 7 sec
        break;
    }
}