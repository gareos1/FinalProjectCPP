#include "Fireball.h"
#include <cmath>
#include <iostream>

Fireball::Fireball(sf::Vector2f position, sf::Vector2f direction, float speed, float damage)
    : speed(speed), damage(damage), alive(true) {

    // Calculate size and damage scaling factors
    const float damageScale = 0.8f + (damage / 80.0f) * 0.7f; // Scales between 0.8-1.5 based on damage (20-80)

    // Create fireball effect with multiple concentric circles (scaled by damage)
    const int flameLayers = 5;
    const float baseRadius = 25.f * damageScale;

    for (int i = 0; i < flameLayers; ++i) {
        sf::CircleShape flame(baseRadius * (1.0f - (i * 0.15f)));
        flame.setOrigin(flame.getRadius(), flame.getRadius());
        flame.setPosition(position);

        // Layer colors from inner (hottest) to outer (coolest)
        // Enhanced colors with damage-based intensity
        sf::Color color;
        float intensity = 0.7f + 0.3f * damageScale; // 0.7-1.0 range

        switch (i) {
        case 0: // Yellow core
            color = sf::Color(255,
                static_cast<sf::Uint8>(255 * intensity),
                0,
                200);
            break;
        case 1: // Orange
            color = sf::Color(255,
                static_cast<sf::Uint8>(165 * intensity),
                0,
                180);
            break;
        case 2: // Red-orange
            color = sf::Color(255,
                static_cast<sf::Uint8>(69 * intensity),
                0,
                160);
            break;
        case 3: // Red
            color = sf::Color(255,
                0,
                0,
                static_cast<sf::Uint8>(140 * intensity));
            break;
        case 4: // Dark red
            color = sf::Color(static_cast<sf::Uint8>(139 * intensity),
                0,
                0,
                120);
            break;
        }

        flame.setFillColor(color);
        flames.push_back(flame);
    }

    // Adjust speed slightly based on size (bigger fireballs are slightly slower)
    this->speed = speed * (1.1f - 0.2f * damageScale);
    velocity = direction * this->speed;

    // Collision box (scaled with damage)
    collisionBox.width = baseRadius * 1.2f;
    collisionBox.height = baseRadius * 1.2f;
    updateCollisionBox();

    // Debug visualization
    collisionDebug.setSize(sf::Vector2f(collisionBox.width, collisionBox.height));
    collisionDebug.setFillColor(sf::Color(255,
        static_cast<sf::Uint8>(100 * (1.0f / damageScale)),
        0,
        100));
    collisionDebug.setOutlineColor(sf::Color::Red);
    collisionDebug.setOutlineThickness(1.f);
}

Fireball::~Fireball()
{
}

void Fireball::updateCollisionBox() {
    sf::Vector2f center = flames[0].getPosition();
    collisionBox.left = center.x - collisionBox.width / 2;
    collisionBox.top = center.y - collisionBox.height / 2;
    collisionDebug.setPosition(center);
}



void Fireball::update(float deltaTime, std::vector<Enemy*>& enemies) {
    if (!alive) return;

    lifeTimer += deltaTime;
    if (lifeTimer >= lifetime) {
        alive = false;
        return;
    }

    for (auto& flame : flames) {
        flame.move(velocity * deltaTime);
    }
    updateCollisionBox();


    // Animate fire effect
    updateFireEffect(deltaTime);


    checkCollisionWithEnemies(enemies);
}

void Fireball::draw(sf::RenderWindow& window) {
    if (!alive) return;

    // Draw flames from outer to inner for proper blending
    for (auto it = flames.rbegin(); it != flames.rend(); ++it) {
        window.draw(*it);
    }


    if (showDebug) {
        window.draw(collisionDebug);
    }
}

bool Fireball::isAlive() const {
    return alive;
}

void Fireball::toggleDebug(bool debug) {
    showDebug = debug;
}

sf::FloatRect Fireball::getCollisionBox() const {
    return collisionBox;
}

void Fireball::updateFireEffect(float deltaTime)
{
    animationTimer += deltaTime;
    pulseTimer += deltaTime;

    // Pulsing effect
    if (pulseTimer > 0.1f) {
        pulseTimer = 0.f;
        float pulse = 1.0f + 0.1f * sin(animationTimer * 10.f);

        for (size_t i = 0; i < flames.size(); ++i) {
            float baseScale = 1.0f - (i * 0.15f);
            flames[i].setScale(pulse * baseScale, pulse * baseScale);
        }
    }

    // Flame flickering
    if (animationTimer > 0.05f) {
        animationTimer = 0.f;

        for (size_t i = 0; i < flames.size(); ++i) {
            // Random slight position offsets
            float offsetX = (rand() % 5 - 2) * 0.5f;
            float offsetY = (rand() % 5 - 2) * 0.5f;
            sf::Vector2f pos = flames[0].getPosition();
            flames[i].setPosition(pos.x + offsetX, pos.y + offsetY);

            // Color variation
            sf::Color c = flames[i].getFillColor();
            int r = std::min(255, std::max(0, c.r + (rand() % 11 - 5)));
            int g = std::min(255, std::max(0, c.g + (rand() % 11 - 5)));
            flames[i].setFillColor(sf::Color(r, g, c.b, c.a));
        }
    }
}




// In Fireball.cpp - Implementation should match
void Fireball::checkCollisionWithEnemies(std::vector<Enemy*>& enemies) {
    for (auto it = enemies.begin(); it != enemies.end() && enemiesPierced < pierceCount; ) {
        Enemy* enemy = *it;
        if (enemy && enemy->isAlive() && collisionBox.intersects(enemy->getCollisionBox())) {
            float lifetimeFactor = 1.0f - (lifeTimer / lifetime);
            enemy->takeDamage(damage * lifetimeFactor);
            enemiesPierced++;

            if (enemiesPierced >= pierceCount) {
                alive = false;
                break;
            }
            ++it;
        }
        else {
            ++it;
        }
    }
}