#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "Enemy.h"

class Fireball {
private:
    std::vector<Enemy*> alreadyHit;

  //visuals
    sf::RectangleShape collisionDebug;
    sf::FloatRect collisionBox;
    std::vector<sf::CircleShape> flames;


    // Movement properties
    sf::Vector2f velocity;
    float speed;

    // Timing controls
    float lifetime = 3.0f;
    float lifeTimer = 0.0f;
    float frameDuration = 0.1f;
    float animationTimer = 0.0f;
    int pulseTimer = 0.f;

    // State management
    bool alive = true;
    bool showDebug = false;

    int pierceCount = 7;  // Number of enemies it can pierce through
    int enemiesPierced = 0;
   
    float damage;
    float baseDamage;
    float currentScale = 1.f;

public:
    Fireball(sf::Vector2f position, sf::Vector2f direction, float speed, float damage );

    ~Fireball();
    void updateCollisionBox();
 
    void update(float deltaTime, std::vector<Enemy*>& enemies);
    void draw(sf::RenderWindow& window);
    bool isAlive() const;
    void toggleDebug(bool debug);
    sf::FloatRect getCollisionBox() const;
    void updateFireEffect(float deltaTime);

	void setDamage(float dmg) { damage = dmg; }
    void setScale(float scale) {
        for (auto& flame : flames) {
            flame.setScale(scale, scale);
        }
        collisionBox.width *= scale;
        collisionBox.height *= scale;
    }
    void checkCollisionWithEnemies(std::vector<Enemy*>& enemies);  
};