#pragma once
#include <SFML/Graphics.hpp>
#include "Enemy.h"
#include <vector>

class BasicBolt {
public:
    // Constructor: Initializes bolt with position, direction, and optional speed
    BasicBolt(sf::Vector2f position, sf::Vector2f direction, float speed = 700.f);
    ~BasicBolt();

    // Update bolt state (movement, lifetime, collisions)
    void update(float deltaTime, const std::vector<Enemy*>& enemies);
    
    // Render the bolt to the window
    void draw(sf::RenderWindow& window);
    
    // Check if the bolt is active/alive
    bool isAlive() const;
    
    // Toggle debug visualization of collision box
    void toggleDebug(bool debug);
    
    // Update collision box position to match bolt's position
    void updateCollisionBox();
    
    // Get the current collision box bounds
    sf::FloatRect getCollisionBox() const;

private:
    // Debug flag to show collision box
    bool showDebug = false;
    
    // Collision detection box
    sf::FloatRect collisionBox;
    
    // Debug visualization of collision box
    sf::RectangleShape collisionDebug;

    // Movement properties
    sf::Vector2f velocity;  // Directional velocity
    float speed;            // Movement speed
    bool alive = true;      // Active state flag

    // Lifetime management
    float lifetime = 2.0f;  // Total lifespan (seconds)
    float lifeTimer = 0.f;  // Time elapsed since creation
    bool hasHitEnemy = false; // Flag to prevent multiple hits

    // Visual representation
    sf::CircleShape shape;

    // Check collisions with enemies
    void checkCollisionWithEnemies(const std::vector<Enemy*>& enemies);
};