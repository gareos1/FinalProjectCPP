#include "BasicBolt.h"
#include <cmath>
#include <iostream>

// Constructor: Sets up shape, collision, and movement properties
BasicBolt::BasicBolt(sf::Vector2f position, sf::Vector2f direction, float speed)
    : speed(speed)
{
    // --- Initialize Circle Shape ---
    const float radius = 8.0f;  // Bolt visual radius
    shape.setRadius(radius);
    shape.setOrigin(radius, radius);  // Center origin for rotation/positioning
    shape.setPosition(position);

    // Visual styling
    shape.setFillColor(sf::Color(255, 255, 0));    // Yellow fill
    shape.setOutlineColor(sf::Color(255, 165, 0)); // Orange outline
    shape.setOutlineThickness(2.f);

    // --- Movement Setup ---
    // Normalized direction (from Player) scaled by speed
    velocity = direction * this->speed;

    // Calculate rotation angle from velocity (convert radians to degrees)
    float angle = std::atan2(velocity.y, velocity.x) * 180.f / 3.14159f;
    shape.setRotation(angle);

    // --- Collision Setup ---
    // Collision box is slightly larger than the sprite (150% of radius)
    collisionBox.width = radius * 1.5f;
    collisionBox.height = radius * 1.5f;
    updateCollisionBox();  // Sync position

    // Debug collision visualization
    collisionDebug.setSize(sf::Vector2f(collisionBox.width, collisionBox.height));
    collisionDebug.setFillColor(sf::Color(0, 255, 255, 100));  // Semi-transparent cyan
    collisionDebug.setOutlineColor(sf::Color::Cyan);
    collisionDebug.setOutlineThickness(1.f);
}

// Destructor (empty, no dynamic resources to free)
BasicBolt::~BasicBolt() {}

// Update bolt state each frame
void BasicBolt::update(float deltaTime, const std::vector<Enemy*>& enemies) {
    if (!alive) return;  // Skip if inactive

    // --- Lifetime Management ---
    lifeTimer += deltaTime;
    if (lifeTimer >= lifetime) {
        alive = false;  // Deactivate after lifetime expires
        return;
    }

    // --- Movement ---
    if (!hasHitEnemy) {
        shape.move(velocity * deltaTime);  // Move based on velocity
        updateCollisionBox();              // Sync collision box
    }

    // --- Collision Detection ---
    if (!hasHitEnemy) {
        checkCollisionWithEnemies(enemies);
    }
}

// Render the bolt (and debug visuals if enabled)
void BasicBolt::draw(sf::RenderWindow& window) {
    if (!alive) return;

    window.draw(shape);  // Draw main shape

    if (showDebug) {
        window.draw(collisionDebug);  // Draw collision box (debug)
    }
}

// Check if bolt is active
bool BasicBolt::isAlive() const {
    return alive;
}

// Toggle debug collision visualization
void BasicBolt::toggleDebug(bool debug) {
    showDebug = debug;
}

// Update collision box position to match bolt's position
void BasicBolt::updateCollisionBox() {
    collisionBox.left = shape.getPosition().x - collisionBox.width / 2;
    collisionBox.top = shape.getPosition().y - collisionBox.height / 2;
    collisionDebug.setPosition(collisionBox.left, collisionBox.top);
}

// Get current collision bounds
sf::FloatRect BasicBolt::getCollisionBox() const {
    return collisionBox;
}

// Check for collisions with enemies
void BasicBolt::checkCollisionWithEnemies(const std::vector<Enemy*>& enemies) {
    for (const auto enemy : enemies) {
        if (enemy && collisionBox.intersects(enemy->getCollisionBox())) {
            enemy->takeDamage(12.5f);  // Apply damage
            alive = false;             // Deactivate on hit
            break;                     // Stop after first collision
        }
    }
}