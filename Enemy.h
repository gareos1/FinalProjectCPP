
#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <queue>
#include <functional>
#include <random>
class Player;

class Enemy {
public:
    enum EnemyType {
        BASIC,
        FAST,
        TANK,
        RANGED
    };

    Enemy(sf::Vector2f position, float health, EnemyType type);
    ~Enemy();
    //foreward declaratrion of projectile
	struct Projectile;
    enum class State { IDLE, VANISHING, DEAD, ATTACKING };

    float lifetime;  // Add lifetime tracking
    const float maxLifetime = 2.0f; // 2 seconds

    // Delete copy operations
    Enemy(const Enemy&) = delete;
    Enemy& operator=(const Enemy&) = delete;

    // Movement and combat
    void update(float deltaTime, const std::vector<std::vector<int>>& maze,
        float tileSize, const std::vector<Enemy*>& otherEnemies);
    void takeDamage(float damage);
    bool isAlive() const;
    float getHealth() const;
    void setPlayer(Player* p);


    // Projectile control methods
    const std::vector<Projectile>& getProjectiles() const { return projectiles; }
    

    // Projectile parameter setters
    void setProjectileSpeed(float speed) { projectileSpeed = speed; }
    void setProjectileDamage(float damage) { projectileDamage = damage; }
    void setAttackRange(float range) { attackRange = range; }
    void setAttackCooldown(float cooldown) { attackCooldown = cooldown; }



    // Rendering
    void draw(sf::RenderWindow& window);
    void toggleDebug(bool debug);
    void drawDebug(sf::RenderWindow& window) const;

    // Collision
    sf::FloatRect getCollisionBox() const;
    sf::Vector2f getPosition() const;
    void setPosition(const sf::Vector2f& position);
    void disableCollision();
    const std::vector<sf::FloatRect>& getProjectileColliders() const;
    void clearProjectiles();




    void setType(EnemyType type);
    static EnemyType getRandomType();

    void setMovementSpeed(float speed) { baseSpeed = speed; }
private:
    //enemy variety
    EnemyType enemyType;
    sf::Color enemyColor;
    float sizeModifier;
    float attackSpeedModifier;

    // HP Bar members
    sf::RectangleShape hpBarBackground;
    sf::RectangleShape hpBarFill;
    float maxHealth = 100.f; // Set this to match your enemy's starting health
    bool showHPBar = true;
    float hpBarWidth = 40.f;
    float hpBarHeight = 5.f;
    float hpBarOffset = -25.f; // Vertical offset from enemy position

    // Projectile data
    struct Projectile {
        sf::Sprite sprite;
        sf::Vector2f direction;
        float speed;
        float damage;
        sf::FloatRect collisionBox;
        float lifetime = 0.f;
        float maxLifetime = 2.0f;
        Projectile(sf::Vector2f pos, sf::Vector2f dir, float spd, float dmg, const sf::Texture& tex);
        void update(float deltaTime);
        bool isExpired() const { return lifetime >= maxLifetime; }
    };

    // Enemy states
    State currentState = State::IDLE;
    bool alive = true;
    bool collisionEnabled = true;
    bool showDebug = false;

    // Visual components
    sf::Sprite sprite;
    std::vector<sf::Texture> idleTextures;
    std::vector<sf::Texture> vanishTextures;
    std::vector<sf::Texture> shriekTextures;
    sf::Texture bulletTexture;
    std::vector<Projectile> projectiles;
    sf::RectangleShape collisionDebug;

    // Audio
    sf::Sound bulletSound;
    sf::SoundBuffer bulletSoundBuffer;

    // Stats
    float health;
    float speed = 100.f;
    float baseSpeed;
    float avoidanceRadius = 80.f;
    float projectileSpeed = 400.f;
    float projectileDamage = 15.f;
    float attackRange = 250.f;
    float attackCooldown = 0.5f;
    float attackTimer = 0.f;

    // Animation
    float animationTimer = 0.f;
    float frameDuration = 0.2f;
    unsigned int currentFrame = 0;
    float vanishTimer = 0.f;
    const float VANISH_DURATION = 0.7f;

    // Pathfinding
    sf::Vector2f velocity;
    std::vector<sf::Vector2f> currentPath;
    float repathTimer = 0.f;
    const float repathCooldown = 0.5f;
    Player* player = nullptr;

    // Collision
    sf::FloatRect collisionBox;
    float collisionShrinkFactor = 0.4f;
    float verticalCollisionOffset = 0.f;

    // Pathfinding node
    struct PathNode {
        sf::Vector2i pos;
        float cost;
        float heuristic;
        sf::Vector2i parent;

        bool operator>(const PathNode& other) const {
            return (cost + heuristic) > (other.cost + other.heuristic);
        }
    };

    // Helper methods

    void loadIdleTextures();
    void loadVanishTextures();
    void loadShriekTextures();
    void loadBulletAssets();
    void updateIdleAnimation(float deltaTime);
    void updateVanishAnimation(float deltaTime);
    void updateShriekAnimation(float deltaTime);
    void updateCollisionBox();
    void updatePathfinding(const std::vector<std::vector<int>>& maze, float tileSize);
    sf::Vector2f calculateSteeringForce(const std::vector<std::vector<int>>& maze,
        float tileSize, const std::vector<Enemy*>& otherEnemies);
    void attackPlayer();
    void setHPBarVisible(bool visible);
    void updateHPBar();
};