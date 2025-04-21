#include "Player.h"
#include "Particle.h"
#include <vector>

//constructor
Player::Player() {
    loadTextures();

    sprite.setTexture(idleTextures[0]);
    textureSize = idleTextures[0].getSize();
    sprite.setOrigin(textureSize.x / 2.f, textureSize.y / 2.f);

    shape.setRadius(20.f);
    shape.setFillColor(defaultColor);
    shape.setOrigin(20.f, 20.f);

    health = maxHealth = 100.0f;
    mana = maxMana = 100.0f;
    manaRegenRate = 5.0f;
    speed = 250.0f;
    stamina = maxStamina = 100.0f;
    isSprinting = false;
    sprintCooldown = 0.0f;
    //audio
        // Load sound buffers
    if (!boltSoundBuffer.loadFromFile("assets/bolt.wav")) {
        std::cerr << "Failed to load bolt sound!\n";
    }
    if (!fireballSoundBuffer.loadFromFile("assets/fireball.wav")) {
        std::cerr << "Failed to load fireball sound!\n";
    }

    // Set up sounds
    boltSound.setBuffer(boltSoundBuffer);
    fireballSound.setBuffer(fireballSoundBuffer);

    // Adjust volumes if needed
    boltSound.setVolume(70.f);
    fireballSound.setVolume(80.f);

    // Collision setup
    collisionBox.width = 40.f;
    collisionBox.height = 60.f;
    updateCollisionBox();

    // Charge indicator setup
    chargeIndicator.setRadius(15.f);  // Increased from 10f
    chargeIndicator.setFillColor(sf::Color::Transparent);
    chargeIndicator.setOutlineColor(sf::Color(255, 200, 0));  // Brighter yellow
    chargeIndicator.setOutlineThickness(3.f);  // Thicker outline
    chargeIndicator.setOrigin(15.f, 15.f);

    // Debug visualization
    collisionDebug.setSize(sf::Vector2f(collisionBox.width, collisionBox.height));
    collisionDebug.setFillColor(sf::Color(255, 0, 0, 100));
    collisionDebug.setOutlineColor(sf::Color::Red);
    collisionDebug.setOutlineThickness(1.f);
}

// Destructor
Player::~Player()
{
}

// Update the collision box based on the sprite's position
void Player::setPosition(const sf::Vector2f& position)
{
	shape.setPosition(position);
	sprite.setPosition(position);
	updateCollisionBox(); // Ensure collision box updates
}

//for error checking
void Player::toggleCollisionDebug(bool debug)
{
	showCollisionDebug = debug;

}

//to check projectile colliedrs
void Player::toggleProjectileDebug(bool debug)
{
    
        for (auto& fireball : fireballs) {
            fireball.toggleDebug(debug);
        }
        for (auto& bolt : basicBolts) {
            bolt.toggleDebug(debug);
        }
    
}

//simply adds mana
void Player::addMana(float amount) {
    mana += amount;
    if (mana > maxMana) mana = maxMana;
}

//BOOST FUNCTIONS 
void Player::applyDamageBoost(float multiplier, float duration) {
    damageMultiplier += multiplier;
    damageBoostTimer = duration;
    particles.emplace_back(shape.getPosition(), sf::Color::Red); // Now works with new Particle constructor
}

void Player::applySpeedBoost(float multiplier, float duration) {
    speedMultiplier += multiplier;
    speedBoostTimer = duration;
    particles.emplace_back(shape.getPosition(), sf::Color::Yellow);
}

void Player::applyFireRateBoost(float multiplier, float duration) {
    fireRateMultiplier += multiplier;
    fireRateBoostTimer = duration;
    particles.emplace_back(shape.getPosition(), sf::Color::Magenta);
}

// Update the collision box based on the player's position
void Player::setPosition(float x, float y) {
	setPosition(sf::Vector2f(x, y));
}

//loads textures from files
void Player::loadTextures()
{
    for (int i = 0; i < 4; ++i) {
        sf::Texture texture;
        texture.loadFromFile("assets/idle_" + std::to_string(i) + ".png");
        idleTextures.push_back(texture);
        textures["idle_" + std::to_string(i)] = idleTextures.back();
    }

    for (int i = 0; i < 4; ++i) {
        sf::Texture texture;
        texture.loadFromFile("assets/walk_" + std::to_string(i) + ".png");
        walkTextures.push_back(texture);
        textures["walk_" + std::to_string(i)] = walkTextures.back();
    }

    for (int i = 0; i < 4; ++i) {
        sf::Texture texture;
        texture.loadFromFile("assets/attack_" + std::to_string(i) + ".png");
        attackTextures.push_back(texture);
        textures["attack_" + std::to_string(i)] = attackTextures.back();
    }

    sprite.setTexture(idleTextures[0]);

    // Set the origin of the sprite to the center of the texture
    sprite.setOrigin(sprite.getLocalBounds().width / 2, sprite.getLocalBounds().height / 2);

}

//updates animation from sprites
void Player::updateAnimation(float deltaTime) {
    animationTimer += deltaTime;

    // Attack animation has priority
    if (isAttacking) {
        if (animationTimer >= attackAnimationSpeed) {
            animationTimer = 0.f;
            currentFrame++;

            if (currentFrame >= 4) {
                currentFrame = 0;
                isAttacking = false;
            }

            std::string textureName = "attack_" + std::to_string(currentFrame);
            if (textures.count(textureName)) {
                sprite.setTexture(textures[textureName]);
                sprite.setOrigin(sprite.getLocalBounds().width / 2.f, sprite.getLocalBounds().height / 2.f);
                // Flip based on facing direction
                sprite.setScale(facingRight ? 1.f : -1.f, 1.f);
            }
        }
        return;
    }

    // Handle walking and idle animations
    if (animationTimer >= animationSpeed) {
        animationTimer = 0.f;
        currentFrame++;

        std::string animationType = isWalking ? "walk" : "idle";
        if (currentFrame >= 4) currentFrame = 0;

        std::string textureName = animationType + "_" + std::to_string(currentFrame);
        if (textures.count(textureName)) {
            sprite.setTexture(textures[textureName]);
            sprite.setOrigin(sprite.getLocalBounds().width / 2.f, sprite.getLocalBounds().height / 2.f);
            // Flip based on facing direction
            sprite.setScale(facingRight ? 1.f : -1.f, 1.f);
        }
    }
}


//main update function
void Player::update(float deltaTime, sf::RenderWindow& window) {
    //  Handle Movement Input
    handleMovementInput();

    //  Apply Movement Modifiers (Sprint/Dash)
    float currentSpeed = applyMovementModifiers(deltaTime);

    // Process Attacks
    handleAttacks(deltaTime, window);

    // Apply Physics
    shape.move(velocity * currentSpeed * deltaTime);
    sprite.setPosition(shape.getPosition());
    updateCollisionBox(); // Critical: Update hitbox position

    //  Update Cooldowns
    updateCooldowns(deltaTime);

    if (!canTakeDamage) {
        damageCooldown -= deltaTime;
        if (damageCooldown <= 0) {
            canTakeDamage = true;
            damageCooldown = 0.0f;
        }
    }

    // Update boost timers
    if (damageBoostTimer > 0.f) {
        damageBoostTimer -= deltaTime;
        if (damageBoostTimer <= 0.f) {
            damageMultiplier = 1.f;
        }
    }

    if (speedBoostTimer > 0.f) {
        speedBoostTimer -= deltaTime;
        if (speedBoostTimer <= 0.f) {
            speedMultiplier = 1.f;
        }
    }

    if (fireRateBoostTimer > 0.f) {
        fireRateBoostTimer -= deltaTime;
        if (fireRateBoostTimer <= 0.f) {
            fireRateMultiplier = 1.f;
        }
    }

    // Apply multipliers to relevant calculations
    float effectiveSpeed = speed * speedMultiplier;
    float effectiveFireRate = fireballRate / fireRateMultiplier;

    //  Update Animations
    updateAnimation(deltaTime);

    //  Update Projectiles
    updateProjectiles(deltaTime);

    //  Handle Collisions
    handleEnemyCollisions();

    //  Regenerate Resources
    regenerateMana(deltaTime);

    //Update particles
	updateParticles(deltaTime);

    // Update charge particles
    if (isChargingFireball) {
        chargeParticleTimer += deltaTime;
        if (chargeParticleTimer > 0.05f) {
            chargeParticleTimer = 0.f;
            sf::CircleShape particle(3.f);
            particle.setFillColor(sf::Color(255, 150 + rand() % 100, 0, 200));
            particle.setOrigin(3.f, 3.f);
            particle.setPosition(shape.getPosition());
            chargeParticles.push_back(particle);
        }
    }

    // Update existing charge particles
    for (auto& p : chargeParticles) {
        float scale = p.getScale().x - (0.5f * deltaTime);
        p.setScale(scale, scale);
        p.move(0, -20.f * deltaTime);  // Float upwards
    }

    // Remove dead particles
    chargeParticles.erase(std::remove_if(chargeParticles.begin(), chargeParticles.end(),
        [](const sf::CircleShape& p) { return p.getScale().x <= 0.1f; }),
        chargeParticles.end());
}

// --- Helper Functions  ---


void Player::handleMovementInput() {
    velocity = { 0, 0 };
    isWalking = false;

    // Horizontal movement
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
        velocity.x = -1.f;
        facingRight = false;
        isWalking = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
        velocity.x = 1.f;
        facingRight = true;
        isWalking = true;
    }

    // Vertical movement
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
        velocity.y = -1.f;
        isWalking = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
        velocity.y = 1.f;
        isWalking = true;
    }

    // Normalize diagonal movement
    if (velocity.x != 0.f && velocity.y != 0.f) {
        velocity /= std::sqrt(2.f);
    }
}

float Player::applyMovementModifiers(float deltaTime) {
    float currentSpeed = speed;

    // Sprinting
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) &&
        stamina > 0.f &&
        sprintCooldown <= 0.f) {
        currentSpeed *= 2.f;
        stamina -= 40.f * deltaTime;
        particles.emplace_back(shape.getPosition());

        if (stamina <= 0.f) {
            stamina = 0.f;
            sprintCooldown = 2.f;
        }
    }
    else {
        // Stamina recovery
        stamina += 20.f * deltaTime;
        if (stamina > maxStamina) stamina = maxStamina;
        if (sprintCooldown > 0.f) sprintCooldown -= deltaTime;
    }

    // Dash
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && dashCooldown <= 0.f && !isDashing && stamina >= 30.f) {
        if (velocity != sf::Vector2f(0, 0)) {
            stamina -= 30.f;
            dashCooldown = 0.5f;

            shape.move(velocity * 200.f); // Dash distance
            particles.emplace_back(shape.getPosition());
        }
    }

    return currentSpeed;
}

void Player::handleAttacks(float deltaTime, sf::RenderWindow& window) {
    // Get the current view from the window
    const sf::View& currentView = window.getView();

    // Convert mouse position to world coordinates using the current view
    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window), currentView);
    sf::Vector2f playerCenter = shape.getPosition();
    sf::Vector2f direction = mousePos - playerCenter;

    // Debug output
    std::cout << "Player: (" << playerCenter.x << "," << playerCenter.y << ") ";
    std::cout << "Mouse: (" << mousePos.x << "," << mousePos.y << ")\n";

    // Normalize direction vector with minimum length check
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length < 5.0f) { // Minimum distance threshold
        direction = facingRight ? sf::Vector2f(1.f, 0.f) : sf::Vector2f(-1.f, 0.f);
    }
    else {
        direction /= length;
    }

    // Update facing direction with deadzone to prevent flickering
    const float facingDeadzone = 10.f;
    if (std::abs(mousePos.x - playerCenter.x) > facingDeadzone) {
        facingRight = (mousePos.x > playerCenter.x);

        // Update sprite scale immediately when direction changes
        sprite.setScale(facingRight ? 1.f : -1.f, 1.f);
    }

    // Fireball charging system - improved version
    if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
        if (!isChargingFireball && mana >= 20.f && fireballCooldown <= 0.f) {
            isChargingFireball = true;
            fireballChargeTime = 0.f;
            chargeParticles.clear();
            mana -= 20.f;
        }

        if (isChargingFireball) {
            fireballChargeTime += deltaTime;
            if (fireballChargeTime > MAX_CHARGE_TIME) {
                fireballChargeTime = MAX_CHARGE_TIME;
                // Visual feedback for max charge
                sprite.setColor(sf::Color(255, 150, 150));
            }

            // Dynamic charge indicator
            float chargeRatio = fireballChargeTime / MAX_CHARGE_TIME;
            chargeIndicator.setPosition(shape.getPosition());
            chargeIndicator.setScale(1.f + chargeRatio * 0.5f, 1.f + chargeRatio * 0.5f);

            // Color progression from yellow to red
            sf::Color chargeColor(
                255,
                255 - static_cast<sf::Uint8>(200 * chargeRatio),
                0
            );
            chargeIndicator.setOutlineColor(chargeColor);

            // Consume mana over time while charging
            mana -= 5.f * deltaTime;
            if (mana <= 0.f) {
                mana = 0.f;
                isChargingFireball = false;
                fireballChargeTime = 0.f;
            }
        }
    }
    else if (isChargingFireball) {
        // Release fireball if charged enough
        if (fireballChargeTime >= MIN_CHARGE_TIME) {

            float additionalCost = 10.f * (fireballChargeTime / MAX_CHARGE_TIME);
            mana -= additionalCost;


            // Calculate damage with exponential curve for better progression
            float chargeRatio = fireballChargeTime / MAX_CHARGE_TIME;
            float damage = MIN_FIREBALL_DAMAGE +
                (MAX_FIREBALL_DAMAGE - MIN_FIREBALL_DAMAGE) *
                std::pow(chargeRatio, 1.5f);  // 1.5 power for faster scaling

            fireballCooldown = fireballRate * (1.f + chargeRatio);  // Longer cooldown for stronger shots
            isAttacking = true;
            fireballSound.play();
            fireballSound.setPitch(0.8f + chargeRatio * 0.4f);  // Lower pitch for stronger shots

            sf::Vector2f spawnPos = playerCenter + direction * 30.f;
            fireballs.emplace_back(spawnPos, direction, 400.f + chargeRatio * 300.f, damage);

          
        }

        // Reset charging state
        isChargingFireball = false;
        fireballChargeTime = 0.f;
        sprite.setColor(sf::Color::White);
        chargeParticles.clear();
    }
    // Basic attack
    if (attackCooldown <= 0.f &&
        sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        attackCooldown = 0.3f;
        isAttacking = true;
        boltSound.play();
        // Spawn bolt slightly in front of player
        sf::Vector2f spawnPos = playerCenter + direction * 25.f;
        basicBolts.emplace_back(spawnPos, direction, 700.f);

        // Debug output
        std::cout << "Bolt created at (" << spawnPos.x << "," << spawnPos.y
            << ") with direction (" << direction.x << "," << direction.y << ")\n";
    }
}

void Player::updateCooldowns(float deltaTime) {
    if (attackCooldown > 0.f) attackCooldown -= deltaTime;
    if (dashCooldown > 0.f) dashCooldown -= deltaTime;
    if (fireballCooldown > 0.f) fireballCooldown -= deltaTime;
}

void Player::updateProjectiles(float deltaTime) {
    if (!enemyList) return;

    // Convert unique_ptr vector to raw pointer vector
    std::vector<Enemy*> rawEnemies;
    for (auto& enemy : *enemyList) {
        rawEnemies.push_back(enemy.get());
    }

    // Update projectiles with raw pointers
    for (auto& fireball : fireballs) {
        fireball.update(deltaTime, rawEnemies);
    }
    for (auto& bolt : basicBolts) {
        bolt.update(deltaTime, rawEnemies);
    }

    // Clean up dead projectiles
    fireballs.erase(std::remove_if(fireballs.begin(), fireballs.end(),
        [](const auto& proj) { return !proj.isAlive(); }),
        fireballs.end());
    basicBolts.erase(std::remove_if(basicBolts.begin(), basicBolts.end(),
        [](const auto& proj) { return !proj.isAlive(); }),
        basicBolts.end());
}

void Player::handleEnemyCollisions() {
    if (!canTakeDamage || !enemyList) return;

    for (const auto& enemy : *enemyList) {
        if (enemy && enemy->isAlive() && getBounds().intersects(enemy->getCollisionBox())) {
            takeDamage(15);
            canTakeDamage = false;
            damageCooldown = DAMAGE_COOLDOWN_TIME;

            

        }
    }
}

void Player::updateParticles(float deltaTime) {
    // Update all particles
    for (auto& p : particles) {
        p.update(deltaTime);
    }

    // Remove dead particles (lifetime expired)
    particles.erase(
        std::remove_if(
            particles.begin(),
            particles.end(),
            [](const Particle& p) {
                return !p.isAlive();
            }
        ),
        particles.end()
    );
}

// Utility function
void Player::normalizeVector(sf::Vector2f& vec) {
    float length = std::sqrt(vec.x * vec.x + vec.y * vec.y);
    if (length != 0.f) vec /= length;
}


//draw functions
void Player::draw(sf::RenderWindow& window) {
    // Draw all particles first (background effects)
    for (auto& p : particles) {
        p.draw(window);
    }

    // Draw all projectiles
    for (auto& fireball : fireballs) {
        fireball.draw(window);
    }
    for (auto& bolt : basicBolts) {
        bolt.draw(window);
    }

    // Visual feedback for dash
    if (isDashing) {
        sprite.setColor(sf::Color(255, 255, 255, 200)); // Semi-transparent
    }
    else {
        sprite.setColor(sf::Color::White);
    }


    // Draw the player sprite
    window.draw(sprite);

  


    // Draw collision debug visualization (can be toggled)
    if (showCollisionDebug) {
        window.draw(collisionDebug);

        // Optional: Draw direction indicator
        sf::Vertex line[] = {
            sf::Vertex(shape.getPosition(), sf::Color::Green),
            sf::Vertex(shape.getPosition() + sf::Vector2f(velocity.x * 20.f, velocity.y * 20.f), sf::Color::Red)
        };
        window.draw(line, 2, sf::Lines);
    }

    // Draw HUD elements
    drawHUD(window);

    // Draw attack effects
    for (auto& attack : attacks) {
        attack.draw(window);

    }


     // Draw charge particles
    for (const auto& particle : chargeParticles) {
        window.draw(particle);
    }

    // Draw charge indicator if charging
    if (isChargingFireball) {
        window.draw(chargeIndicator);
        
        // Draw charge percentage text
        if (showCollisionDebug) {  // Or always show it
            int chargePercent = static_cast<int>((fireballChargeTime / MAX_CHARGE_TIME) * 100);
            sf::Text chargeText;
            chargeText.setString(std::to_string(chargePercent) + "%");
            chargeText.setPosition(shape.getPosition().x - 20.f, shape.getPosition().y - 50.f);
            window.draw(chargeText);
        }
    }

}

void Player::drawHUD(sf::RenderWindow& window) {
    // Get view position for HUD anchoring
    sf::Vector2f viewCenter = window.getView().getCenter();
    sf::Vector2f viewSize = window.getView().getSize();
    float hudX = viewCenter.x - viewSize.x / 2 + 10.f;
    float hudY = viewCenter.y - viewSize.y / 2 + 30.f;
    float hpBarHeight = 150.0f;

    // Health Bar
    drawBar(window, hudX, hudY, 12.0f, hpBarHeight,
        health / maxHealth,
        sf::Color::Green, "Health");

    // Stamina Bar (20px right of health)
    float staminaRatio = stamina / maxStamina;
    sf::Color staminaColor = (staminaRatio > 0.5f) ? sf::Color::Yellow :
        (staminaRatio > 0.2f) ? sf::Color(255, 165, 0) : sf::Color::Red;
    drawBar(window, hudX + 20.f, hudY, 12.0f, hpBarHeight,
        staminaRatio, staminaColor, "Stamina");

    // Mana Bar (40px right of health)
    float manaRatio = mana / maxMana;
    sf::Color manaColor = (manaRatio > 0.5f) ? sf::Color::Blue :
        (manaRatio > 0.2f) ? sf::Color(100, 100, 255) : sf::Color(50, 50, 150);
    drawBar(window, hudX + 40.f, hudY, 12.0f, hpBarHeight,
        manaRatio, manaColor, "Mana");
}

void Player::drawBar(sf::RenderWindow& window, float x, float y, float width, float height,
    float ratio, const sf::Color& fillColor, const std::string& label) {
    // Background
    sf::RectangleShape background(sf::Vector2f(width, height));
    background.setFillColor(sf::Color(40, 40, 40));
    background.setOutlineThickness(1.0f);
    background.setOutlineColor(sf::Color::Black);
    background.setPosition(x, y);
    window.draw(background);

    // Fill
    sf::RectangleShape bar(sf::Vector2f(width, height * ratio));
    bar.setFillColor(fillColor);
    bar.setPosition(x, y + (height * (1 - ratio)));
    window.draw(bar);

  
    }

//getters
float Player::getX() const
{ return shape.getPosition().x; 
}

float Player::getY() const {
    return shape.getPosition().y; 
}

//damage is taken
void Player::takeDamage(float damage)
{
    if (!canTakeDamage) return;

    health -= damage;
    std::cout << "Player took " << damage << " damage. Health now: " << health << std::endl;
    if (health < 0.0f) health = 0.0f;

    canTakeDamage = false;
    damageCooldown = DAMAGE_COOLDOWN_TIME;
}

//hp++
void Player::heal(float amount)
{
	health += amount;
	if (health > maxHealth) health = maxHealth;
}

//mana--
void Player::useMana(float amount)
{
    
        mana -= amount;
        if (mana < 0.0f) mana = 0.0f;
    
}

//mana++
void Player::regenerateMana(float deltaTime)
{
    mana += manaRegenRate * deltaTime;
    if (mana > maxMana) mana = maxMana;
}

// Set the enemy list for collision detection
void Player::setEnemyList(std::vector<std::unique_ptr<Enemy>>* enemies) {
    enemyList = enemies;
}

sf::FloatRect Player::getBounds() const
{
	return collisionBox;
    
}

sf::FloatRect Player::getCollisionBox() const
{
	return collisionBox;
}

void Player::updateCollisionBox()
{


    // Center the collision box on player position
    collisionBox.left = shape.getPosition().x - collisionBox.width / 2;
    collisionBox.top = shape.getPosition().y - collisionBox.height / 2+40.f;

    // Update debug visualization position
    collisionDebug.setPosition(collisionBox.left, collisionBox.top);
}

