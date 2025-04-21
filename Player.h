#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>
#include "Particle.h"
#include <vector>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <SFML/Audio.hpp>
#include "Fireball.h"
#include "BasicBolt.h"
#include <memory>

// Forward declaration of Enemy
class Enemy;

struct AttackEffect {
    sf::ConvexShape shape;
    float timer;

    AttackEffect(sf::Vector2f pos, sf::Vector2f dir) {
        shape.setPointCount(3);
        shape.setFillColor(sf::Color(255, 0, 0, 100));
        shape.setPoint(0, pos);

        sf::Vector2f right(-dir.y, dir.x);
        shape.setPoint(1, pos + dir * 50.f + right * 20.f);
        shape.setPoint(2, pos + dir * 50.f - right * 20.f);

        timer = 0.2f;
    }

    bool update(float dt) {
        timer -= dt;
        return timer <= 0.f;
    }

    void draw(sf::RenderWindow& win) {
        win.draw(shape);
    }
};

class Player {
public:
    Player();
	~Player();
    void loadTextures();
    void updateAnimation(float deltaTime);
    void update(float deltaTime, sf::RenderWindow& window);
    void draw(sf::RenderWindow& window);
    void setPosition(float x, float y);
    float getX() const;
    float getY() const;
    float getStamina() const { return stamina; }
    float getMaxStamina() const { return maxStamina; }
	float getHealth() const { return health; }
	float getMaxHealth() const { return maxHealth; }
    void takeDamage(float damage);
    void heal(float amount);
    void useMana(float amount);
    void regenerateMana(float deltaTime);

    // Enemy-related methods
    void setEnemyList(std::vector<std::unique_ptr<Enemy>>* enemies);
    sf::Vector2f getPosition() const { return shape.getPosition(); }
    sf::FloatRect getBounds() const;
    sf::FloatRect getCollisionBox() const;
    void setPosition(const sf::Vector2f& position);
    void toggleCollisionDebug(bool debug);
    void toggleProjectileDebug(bool debug);

    void healToFull() { health = maxHealth; }
	void reset() { health = maxHealth; mana = maxMana; stamina = maxStamina; }

    bool isAlive() const { return health > 0.0f; }

    //poweups
    void addMana(float amount);
    void applyDamageBoost(float multiplier, float duration);
    void applySpeedBoost(float multiplier, float duration);
    void applyFireRateBoost(float multiplier, float duration);

private:

 //firebal
    bool isChargingFireball = false;
    float fireballChargeTime = 0.f;
    const float MAX_CHARGE_TIME = 2.5f;  // Increased from 2.0f for better gameplay
    const float MIN_FIREBALL_DAMAGE = 20.f;
    const float MAX_FIREBALL_DAMAGE = 80.f;  // Increased from 60f
    const float MIN_CHARGE_TIME = 0.3f;  // Minimum time to register a charge
    sf::CircleShape chargeIndicator;
    std::vector<sf::CircleShape> chargeParticles;
    float chargeParticleTimer = 0.f;
    //audio
    sf::SoundBuffer boltSoundBuffer;
    sf::SoundBuffer fireballSoundBuffer;
    sf::Sound boltSound;
    sf::Sound fireballSound;

    // Enemy list - now using unique_ptr
    std::vector<std::unique_ptr<Enemy>>* enemyList = nullptr;

    // Constants
    const float minAttackDistance = 5.0f;
    const float facingDeadzone = 10.0f;

    // Helper methods
    void updateParticles(float deltaTime);
    void handleMovementInput();
    float applyMovementModifiers(float deltaTime);
    void handleAttacks(float deltaTime, sf::RenderWindow& window);
    void updateCooldowns(float deltaTime);
    void updateProjectiles(float deltaTime);
    void handleEnemyCollisions();
    void normalizeVector(sf::Vector2f& vec);
    void drawHUD(sf::RenderWindow& window);
    void drawBar(sf::RenderWindow& window, float x, float y, float width,
        float height, float ratio, const sf::Color& fillColor,
        const std::string& label = "");

    // Debug
    bool showCollisionDebug = false;

    // Attack state
    bool fireballCharging = false;
    float chargeStartTime = 0.f;
    bool isBasicBoltAttack = false;

    // Collision
    sf::FloatRect collisionBox;
    sf::RectangleShape collisionDebug;
    void updateCollisionBox();

    // Graphics
    sf::Vector2u textureSize;
    sf::Texture texture;
    sf::Sprite sprite;
    std::vector<sf::Texture> idleTextures;
    std::vector<sf::Texture> walkTextures;
    std::vector<sf::Texture> attackTextures;
    std::map<std::string, sf::Texture> textures;

    // Animation
    float attackAnimationSpeed = 0.1f;
    float animationTimer = 0.0f;
    float animationSpeed = 0.15f;
    int animationIndex = 0;
    int currentFrame = 0;
    bool isWalking = false;
    bool facingRight = true;
    bool isAttacking = false;
    float attackTimer = 0.0f;

    // Particles
    std::vector<Particle> particles;
    sf::Color defaultColor = sf::Color::Cyan;

    // Stats
    float stamina;
    float maxStamina;
    bool isSprinting;
    float sprintCooldown;
    float health;
    float maxHealth;
    float mana;
    float maxMana;
    float manaRegenRate;

    // Movement
    sf::CircleShape shape;
    sf::Vector2f velocity;
    float speed = 250.0f;

    // Dash
    bool isDashing = false;
    float dashCooldown = 0.0f;
    float dashDuration = 0.2f;
    float dashTimer = 0.0f;
    const float dashSpeed = 600.0f;
    const float dashDistance = 200.f;
    sf::Vector2f dashDirection;

    // Attacks
    float attackCooldown = 0.0f;
    const float attackRate = 0.4f;
    std::vector<AttackEffect> attacks;
    std::vector<Fireball> fireballs;
    float fireballCooldown = 0.f;
    float fireballRate = 0.5;
    std::vector<BasicBolt> basicBolts;
    float boltCooldown = 0.f;
    const float boltRate = 0.2f;


   //for fixing enemy damage
    float damageCooldown = 0.0f;
    const float DAMAGE_COOLDOWN_TIME = 0.5f; // Half-second cooldown
    bool canTakeDamage = true;



    // Powerup effects
    float damageMultiplier = 1.f;
    float speedMultiplier = 1.f;
    float fireRateMultiplier = 1.f;
    
    // Timers for temporary boosts
    float damageBoostTimer = 0.f;
    float speedBoostTimer = 0.f;
    float fireRateBoostTimer = 0.f;
    



    //dash improve







};