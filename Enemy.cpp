#include "Enemy.h"
#include "Player.h"
#include <cmath>
#include <iostream>
#include <algorithm>



// Projectile implementation
Enemy::Projectile::Projectile(sf::Vector2f pos, sf::Vector2f dir, float spd, float dmg, const sf::Texture& tex)
    : direction(dir), speed(spd), damage(dmg), lifetime(0.f) {
    sprite.setTexture(tex);
    sprite.setOrigin(4.f, 3.f);
    sprite.setPosition(pos);
    collisionBox = sf::FloatRect(pos.x - 3.f, pos.y - 2.f, 6.f, 4.f);
}

/// Update the collision box based on the projectile's position
void Enemy::Projectile::update(float deltaTime) {
    sprite.move(direction * speed * deltaTime);
    collisionBox.left = sprite.getPosition().x - 3.f;
    collisionBox.top = sprite.getPosition().y - 2.f;
    lifetime += deltaTime; // Update lifetime
}

// Enemy constructor
Enemy::Enemy(sf::Vector2f position, float health,EnemyType type)
    : health(health), maxHealth(health), alive(true), currentState(State::IDLE),
    enemyType(type), sizeModifier(1.0f), attackSpeedModifier(1.0f) {

    setType(type);
    // Load all assets
    loadIdleTextures();
    loadVanishTextures();
    loadShriekTextures();
    loadBulletAssets();
    // Load bullet texture
    if (!bulletTexture.loadFromFile("assets/bullet.png")) {
        std::cerr << "Failed to load bullet texture\n";
    }
    // Load sound buffer
    if (!bulletSoundBuffer.loadFromFile("assets/bullet.wav")) {
        std::cerr << "Failed to load bullet sound\n";
    }
    bulletSound.setBuffer(bulletSoundBuffer);
    // Initialize sprite with first idle frame if available
    if (!idleTextures.empty()) {
        sprite.setTexture(idleTextures[0]);
        float scaleFactor = 2.0f;
        sprite.setScale(scaleFactor, scaleFactor);
        sprite.setOrigin(idleTextures[0].getSize().x / 2.f, idleTextures[0].getSize().y / 2.f);
        sprite.setPosition(position);
    }
    else {
        std::cerr << "Warning: Failed to load enemy textures\n";
        sprite.setPosition(position);
    }

    updateCollisionBox();
    collisionDebug.setSize(sf::Vector2f(collisionBox.width, collisionBox.height));
    collisionDebug.setFillColor(sf::Color(255, 0, 0, 100));
    collisionDebug.setOutlineColor(sf::Color::Red);
    collisionDebug.setOutlineThickness(1.f);

    if (!alive) return;
    // Initialize HP bar
    hpBarBackground.setSize(sf::Vector2f(hpBarWidth, hpBarHeight));
    hpBarBackground.setFillColor(sf::Color(50, 50, 50));
    hpBarBackground.setOutlineThickness(1.f);
    hpBarBackground.setOutlineColor(sf::Color::Black);

    hpBarFill.setSize(sf::Vector2f(hpBarWidth, hpBarHeight));
    hpBarFill.setFillColor(sf::Color(0, 255, 0)); // Start green

    // Position the HP bar initially
    updateHPBar();
}
// Destructor
Enemy::~Enemy() {
    // Clean up if needed
}

// uodates enemy stuff
void Enemy::update(float deltaTime, const std::vector<std::vector<int>>& maze,
    float tileSize, const std::vector<Enemy*>& otherEnemies) {
    if (!alive) return;

    updateHPBar();
    // Update projectiles
    for (auto it = projectiles.begin(); it != projectiles.end(); ) {
        it->update(deltaTime);

        if (it->isExpired()) {  // Check lifetime instead of bounds
            it = projectiles.erase(it);
        }
        else {
            ++it;
        }
    }

    switch (currentState) {
    case State::IDLE:
        updateIdleAnimation(deltaTime);
        repathTimer += deltaTime;
        attackTimer += deltaTime;

        if (player) {
            // Face player direction
            sf::Vector2f toPlayer = player->getPosition() - sprite.getPosition();
            float distanceToPlayer = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);

            // Only face player if within attack range
            if (distanceToPlayer <= attackRange * 1.5f) {
                float currentScale = std::abs(sprite.getScale().x);
                sprite.setScale(
                    (toPlayer.x > 0) ? currentScale : -currentScale,
                    sprite.getScale().y
                );
            }

            // Check if player is in range for attack
            if (distanceToPlayer <= attackRange && attackTimer >= attackCooldown) {
                attackPlayer();
                attackTimer = 0.0f;
            }

            if (repathTimer >= repathCooldown) {
                repathTimer = 0.f;
                updatePathfinding(maze, tileSize);
            }

            sf::Vector2f steeringForce = calculateSteeringForce(maze, tileSize, otherEnemies);
            velocity = steeringForce * speed;
            sprite.move(velocity * deltaTime);
            updateCollisionBox();
        }
        break;

    case State::ATTACKING:
        updateShriekAnimation(deltaTime);
        break;

    case State::VANISHING:
        updateVanishAnimation(deltaTime);
        break;

    case State::DEAD:
        break;
    }
}
//path finder
sf::Vector2f Enemy::calculateSteeringForce(const std::vector<std::vector<int>>& maze, float tileSize, const std::vector<Enemy*>& otherEnemies) {
    sf::Vector2f force(0, 0);
    if (!player) return force;

    // Seek player
    sf::Vector2f toPlayer = player->getPosition() - sprite.getPosition();
    float distance = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);
    if (distance > 0) {
        force += (toPlayer / distance) * 1.5f;
    }

    // Follow path
    if (!currentPath.empty()) {
        sf::Vector2f toWaypoint = currentPath.back() - sprite.getPosition();
        float wpDistance = std::sqrt(toWaypoint.x * toWaypoint.x + toWaypoint.y * toWaypoint.y);
        if (wpDistance > 0) {
            force += (toWaypoint / wpDistance) * 1.2f;
            if (wpDistance < 10.f) {
                currentPath.pop_back();
            }
        }
    }

    // Avoid other enemies
    for (const Enemy* other : otherEnemies) {
        if (other == this || !other->isAlive()) continue;

        sf::Vector2f away = sprite.getPosition() - other->getPosition();
        float dist = std::sqrt(away.x * away.x + away.y * away.y);
        if (dist < avoidanceRadius && dist > 0) {
            force += (away / dist) * (1.0f - dist / avoidanceRadius) * 2.0f;
        }
    }

    // Normalize
    float forceLength = std::sqrt(force.x * force.x + force.y * force.y);
    if (forceLength > 0) {
        force /= forceLength;
    }

    return force;
}

// Take damage and check if alive
void Enemy::attackPlayer() {
    if (!player || currentState != State::IDLE) return;

    currentState = State::ATTACKING;
    animationTimer = 0.f;
    currentFrame = 0;

    // Play sound effect
    bulletSound.play();

    // Calculate direction to player
    sf::Vector2f direction = player->getPosition() - sprite.getPosition();
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length > 0) {
        direction /= length;
    }

    // Create projectile
    projectiles.emplace_back(
        sprite.getPosition(),
        direction,
        projectileSpeed,
        projectileDamage,
        bulletTexture
    );
}
//hp bar for enemies
void Enemy::setHPBarVisible(bool visible)
{
    showHPBar = visible;
}

void Enemy::updateHPBar() {
    if (!alive) return;

    // Calculate HP percentage
    float hpPercent = health / maxHealth;
    hpPercent = std::max(0.f, std::min(1.f, hpPercent)); // Clamp between 0-1

    // Update fill size
    hpBarFill.setSize(sf::Vector2f(hpBarWidth * hpPercent, hpBarHeight));

    // Position above collision box
    sf::Vector2f colliderTopCenter(
        collisionBox.left + collisionBox.width / 2,
        collisionBox.top
    );

    hpBarBackground.setPosition(
        colliderTopCenter.x - hpBarWidth / 2,
        colliderTopCenter.y + hpBarOffset
    );
    hpBarFill.setPosition(
        colliderTopCenter.x - hpBarWidth / 2,
        colliderTopCenter.y + hpBarOffset
    );

    // Change color based on HP
    if (hpPercent > 0.6f) {
        hpBarFill.setFillColor(sf::Color(0, 255, 0)); // Green when healthy
    }
    else if (hpPercent > 0.3f) {
        hpBarFill.setFillColor(sf::Color(255, 255, 0)); // Yellow when mid
    }
    else {
        hpBarFill.setFillColor(sf::Color(255, 0, 0)); // Red when low
    }
}

//checks if damage is dealt or not
const std::vector<sf::FloatRect>& Enemy::getProjectileColliders() const {
    static std::vector<sf::FloatRect> colliders;
    colliders.clear();
    for (const auto& proj : projectiles) {
        colliders.push_back(proj.collisionBox);
    }
    return colliders;
}

//in 2-3 seconds projectiles are deleted to free memo space
void Enemy::clearProjectiles() {
    projectiles.clear();
}

//sets what kind of enemy it is 
void Enemy::setType(EnemyType type) {
    enemyType = type;

    switch (type) {
    case FAST:
        enemyColor = sf::Color(150, 255, 150); // Light green
        sizeModifier = 0.8f;
        attackSpeedModifier = 1.5f;
        speed *= 1.5f;
        health *= 0.7f;
        projectileSpeed *= 1.3f;
        break;

    case TANK:
        enemyColor = sf::Color(255, 150, 150); // Light red
        sizeModifier = 1.3f;
        attackSpeedModifier = 0.7f;
        speed *= 0.7f;
        health *= 2.0f;
        projectileDamage *= 1.5f;
        break;

    case RANGED:
        enemyColor = sf::Color(150, 150, 255); // Light blue
        sizeModifier = 1.0f;
        attackSpeedModifier = 1.0f;
        attackRange *= 1.5f;
        projectileSpeed *= 1.2f;
        break;

    case BASIC:
    default:
        enemyColor = sf::Color::White;
        sizeModifier = 1.0f;
        attackSpeedModifier = 1.0f;
        break;
    }

    // Apply size modifier to sprite
    sprite.setScale(sizeModifier * 2.0f, sizeModifier * 2.0f);

    // Apply attack speed modifier
    attackCooldown /= attackSpeedModifier;
}
//randomly pics a type
Enemy::EnemyType Enemy::getRandomType() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 3);

    int type = dis(gen);
    return static_cast<EnemyType>(type);
}

//accurately follows player
void Enemy::updatePathfinding(const std::vector<std::vector<int>>& maze, float tileSize) {
    if (!player || maze.empty()) return;

    sf::Vector2i start(
        static_cast<int>(sprite.getPosition().x / tileSize),
        static_cast<int>(sprite.getPosition().y / tileSize)
    );
    sf::Vector2i target(
        static_cast<int>(player->getPosition().x / tileSize),
        static_cast<int>(player->getPosition().y / tileSize)
    );

    auto heuristic = [](sf::Vector2i a, sf::Vector2i b) {
        return static_cast<float>(std::abs(a.x - b.x) + std::abs(a.y - b.y));
        };

    std::priority_queue<PathNode, std::vector<PathNode>, std::greater<PathNode>> openSet;
    std::vector<std::vector<bool>> closedSet(maze.size(), std::vector<bool>(maze[0].size(), false));
    std::vector<std::vector<PathNode>> nodeInfo(maze.size(), std::vector<PathNode>(maze[0].size()));

    PathNode startNode = { start, 0.f, heuristic(start, target), {-1, -1} };
    openSet.push(startNode);
    nodeInfo[start.y][start.x] = startNode;

    while (!openSet.empty()) {
        PathNode current = openSet.top();
        openSet.pop();

        if (current.pos == target) {
            currentPath.clear();
            sf::Vector2i pathNode = target;
            while (pathNode != start) {
                currentPath.emplace_back(
                    (pathNode.x + 0.5f) * tileSize,
                    (pathNode.y + 0.5f) * tileSize
                );
                pathNode = nodeInfo[pathNode.y][pathNode.x].parent;
            }
            std::reverse(currentPath.begin(), currentPath.end());
            return;
        }

        closedSet[current.pos.y][current.pos.x] = true;

        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 || dy == 0) { // Cardinal directions only
                    sf::Vector2i neighbor(current.pos.x + dx, current.pos.y + dy);

                    if (neighbor.x >= 0 && neighbor.y >= 0 &&
                        neighbor.x < static_cast<int>(maze[0].size()) &&
                        neighbor.y < static_cast<int>(maze.size()) &&
                        maze[neighbor.y][neighbor.x] == 0) {

                        float moveCost = current.cost + 1.0f;
                        float newHeuristic = heuristic(neighbor, target);

                        if (!closedSet[neighbor.y][neighbor.x] &&
                            (nodeInfo[neighbor.y][neighbor.x].parent == sf::Vector2i(-1, -1) ||
                                moveCost < nodeInfo[neighbor.y][neighbor.x].cost)) {

                            PathNode neighborNode = { neighbor, moveCost, newHeuristic, current.pos };
                            nodeInfo[neighbor.y][neighbor.x] = neighborNode;
                            openSet.push(neighborNode);
                        }
                    }
                }
            }
        }
    }
}

//takes dmg
void Enemy::takeDamage(float damage) {
    if (!alive || currentState != State::IDLE) return;

    health -= damage;
	updateHPBar();
    if (health <= 0) {
        currentState = State::VANISHING;
        vanishTimer = 0.f;
       
        collisionEnabled = false;
        showHPBar = false;
    }
}

//alive or no
bool Enemy::isAlive() const {
    return alive;
}
//getter
float Enemy::getHealth() const {
    return health;
}
//setter
void Enemy::setPlayer(Player* p) {
    player = p;
}
//draws enemy stuffs
void Enemy::draw(sf::RenderWindow& window) {
    if (alive) {
		//apply color to sprite
        sprite.setColor(enemyColor);
        window.draw(sprite);
    }
    // Draw HP bar if visible and enemy is alive
    if (showHPBar) {
        window.draw(hpBarBackground);
        window.draw(hpBarFill);
    }


    // Draw projectiles
    for (auto& projectile : projectiles) {
        window.draw(projectile.sprite);

        if (showDebug) {
            // Draw projectile collision box
            sf::RectangleShape debugRect(sf::Vector2f(
                projectile.collisionBox.width,
                projectile.collisionBox.height
            ));
            debugRect.setPosition(projectile.collisionBox.left, projectile.collisionBox.top);
            debugRect.setFillColor(sf::Color(255, 255, 0, 100));
            window.draw(debugRect);
        }
    }
}
//collision checks 
sf::FloatRect Enemy::getCollisionBox() const {
    return collisionBox;
}

sf::Vector2f Enemy::getPosition() const
{
	return sprite.getPosition();
}

void Enemy::setPosition(const sf::Vector2f& position)
{
    sprite.setPosition(position);
	// Update collision box position
    updateCollisionBox();
}

void Enemy::toggleDebug(bool debug)
{
	showDebug = debug;
}

void Enemy::drawDebug(sf::RenderWindow& window) const {
    if (!showDebug) return;

    // Draw collision box
    sf::RectangleShape debugRect(sf::Vector2f(
        collisionBox.width,
        collisionBox.height
    ));
    debugRect.setPosition(collisionBox.left, collisionBox.top);
    debugRect.setFillColor(sf::Color(255, 0, 0, 100)); // Semi-transparent red
    window.draw(debugRect);

    // Draw path if available
    if (!currentPath.empty()) {
        sf::VertexArray pathLines(sf::LineStrip, currentPath.size() + 1);
        pathLines[0].position = sprite.getPosition();
        pathLines[0].color = sf::Color::Green;

        for (size_t i = 0; i < currentPath.size(); i++) {
            pathLines[i + 1].position = currentPath[i];
            pathLines[i + 1].color = sf::Color::Green;
        }

        window.draw(pathLines);
    }
}
//disables collision when dead
void Enemy::disableCollision()
{
    collisionEnabled = false;
}
//texture and animation stuff
void Enemy::loadShriekTextures()
{
    sf::Texture sheet;
    if (!sheet.loadFromFile("assets/ghost-shriek.png")) {
        std::cerr << "Failed to load ghost-shriek spritesheet\n";
        return;
    }

    // Split 256x80 sheet into 4 frames (each 64x80)
    for (int i = 0; i < 4; ++i) {
        sf::Texture frame;
        frame.loadFromImage(sheet.copyToImage(), sf::IntRect(i * 64, 0, 64, 80));
        shriekTextures.push_back(frame);
    }
}

void Enemy::updateShriekAnimation(float deltaTime)
{
    animationTimer += deltaTime;
    float frameTime = 0.1f; // Fast animation for shriek

    if (animationTimer >= frameTime && !shriekTextures.empty()) {
        animationTimer = 0.f;
        currentFrame++;

        if (currentFrame >= shriekTextures.size()) {
            currentState = State::IDLE;
            currentFrame = 0;
        }
        else {
            sprite.setTexture(shriekTextures[currentFrame]);
        }
    }
}

void Enemy::loadBulletAssets()
{
    sf::Texture sheet;
    if (!sheet.loadFromFile("assets/ghost-shriek.png")) {
        std::cerr << "Failed to load ghost-shriek spritesheet\n";
        return;
    }

    // Split 256x80 sheet into 4 frames (each 64x80)
    for (int i = 0; i < 4; ++i) {
        sf::Texture frame;
        frame.loadFromImage(sheet.copyToImage(), sf::IntRect(i * 64, 0, 64, 80));
        shriekTextures.push_back(frame);
    }
}

void Enemy::loadIdleTextures()
{
    sf::Texture sheet;
    if (!sheet.loadFromFile("assets/ghost-idle.png")) {
        std::cerr << "Failed to load ghost-idle spritesheet\n";
        return;
    }

    // Split 448x80 sheet into 7 frames (each 64x80)
    for (int i = 0; i < 7; ++i) {
        sf::Texture frame;
        frame.loadFromImage(sheet.copyToImage(), sf::IntRect(i * 64, 0, 64, 80));
        idleTextures.push_back(frame);
    }
}

void Enemy::loadVanishTextures()
{
    sf::Texture sheet;
    if (!sheet.loadFromFile("assets/ghost-vanish.png")) {
        std::cerr << "Failed to load ghost-vanish spritesheet\n";
        return;
    }

    // Assuming similar dimensions - adjust as needed
    for (int i = 0; i < 7; ++i) { 
        sf::Texture frame;
        frame.loadFromImage(sheet.copyToImage(), sf::IntRect(i * 64, 0, 64, 80));
        vanishTextures.push_back(frame);
    }
}

void Enemy::updateIdleAnimation(float deltaTime)
{
    animationTimer += deltaTime;
    if (animationTimer >= frameDuration && !idleTextures.empty()) {
        animationTimer = 0.f;
        currentFrame = (currentFrame + 1) % idleTextures.size();
        sprite.setTexture(idleTextures[currentFrame]);
    }
}

void Enemy::updateVanishAnimation(float deltaTime)
{
    vanishTimer += deltaTime;
    float frameTime = VANISH_DURATION / vanishTextures.size();
    int frame = static_cast<int>(vanishTimer / frameTime);

    if (frame < vanishTextures.size()) {
        sprite.setTexture(vanishTextures[frame]);
    }
    else {
        currentState = State::DEAD;
        alive = false;
    }
}

void Enemy::updateCollisionBox() {
    sf::FloatRect spriteBounds = sprite.getGlobalBounds();

    // Calculate collision box dimensions (smaller than sprite)
    float width = spriteBounds.width * collisionShrinkFactor;
    float height = spriteBounds.height * (collisionShrinkFactor); //-0.25f makes it more of arectangle

    // Center horizontally, adjust vertically
    float left = spriteBounds.left + (spriteBounds.width - width) / 2.f;
    float top = spriteBounds.top + (spriteBounds.height - height) / 2.f + verticalCollisionOffset;

    collisionBox = sf::FloatRect(left, top, width, height);
}