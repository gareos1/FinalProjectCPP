#include "Game.h"
#include <algorithm>
#include <cmath>
#include <sstream>

// Initialize static constants
const float Game::tileSize = 32.f;

Game::Game() : window(sf::VideoMode(1280, 720), "Maze Adventure") {
    window.setFramerateLimit(60);
    gameOver=false;
    showMenu=true;
    totalEnemiesKilled=0;
    loadHighScores();
    // Initialize views
    gameView.setSize(window.getSize().x, window.getSize().y);
    uiView = window.getDefaultView();
    //load sound 
    loadSounds();
   


    // Initialize game
    loadHighScores();
    generateMaze();
    player.setEnemyList(&enemies);
}

Game::~Game()
{
    menuSound.stop();
    gameOverSound.stop();
}

//main run code
void Game::run() {
    sf::Clock clock;

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();

        if (showMenu) {
            showMainMenu();
            continue;
        }

        if (gameOver) {
            showGameOverScreen();
            continue;
        }

        processEvents();

        // Only update if game has started
        if (gameStarted && !showMenu && !gameOver) {
            update(deltaTime);
        }

        render();
    }
}

// for collision
void Game::toggleCollisionDebug()
{
	showCollisionDebug = !showCollisionDebug;
	
}

// Get the pointer to enemies
std::vector<Enemy*> Game::getEnemyPointers() {
    std::vector<Enemy*> pointers;
    pointers.reserve(enemies.size());
    for (auto& enemy : enemies) {
        pointers.push_back(enemy.get());
    }
    return pointers;
}

// Add an enemy to the game
void Game::addEnemy(sf::Vector2f position, float health, Enemy::EnemyType type)
{
    enemies.push_back(std::make_unique<Enemy>(position, health, type));
    enemies.back()->setPlayer(&player);
}

// Spawn collectables in the maze
void Game::spawnCollectables() {
    if (powerupSpawned) return; // Prevent spawning if already spawned

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> typeDis(0, 4);
    std::uniform_int_distribution<> posDis(1, static_cast<int>(maze.size()) - 2);
    std::uniform_real_distribution<> valueDis(10.f, 30.f);

    collectables.clear();

    // Calculate base powerup count based on level and enemies
    int baseCount = 3 + (currentLevel / 3); // Minimum 3, +1 every 3 levels
    int enemyBasedCount = static_cast<int>(enemies.size()) / 3;
    int toSpawn = std::min(baseCount + enemyBasedCount, 8); // Cap at 8 powerups

    // Get player position in grid coordinates
    sf::Vector2i playerGrid(
        static_cast<int>(player.getPosition().x / tileSize),
        static_cast<int>(player.getPosition().y / tileSize)
    );

    for (int i = 0; i < toSpawn; i++) {
        int attempts = 0;
        bool validPosition = false;

        while (!validPosition && attempts < 50) { // Increased max attempts
            int x = posDis(gen);
            int y = posDis(gen);
            attempts++;

            // Check basic validity
            if (maze[y][x] != 0) continue;

            // Convert to grid position for distance check
            sf::Vector2i powerupGrid(x, y);

            // Calculate distance to player
            float distanceToPlayer = std::sqrt(
                std::pow(x - playerGrid.x, 2) +
                std::pow(y - playerGrid.y, 2)
            );

            // Minimum distance from player (3 tiles)
            if (distanceToPlayer < 3.0f) continue;

            // Check distance from other powerups
            bool tooCloseToOther = false;
            for (const auto& collectable : collectables) {
                sf::Vector2f pos = collectable.getPosition();
                sf::Vector2i otherGrid(
                    static_cast<int>(pos.x / tileSize),
                    static_cast<int>(pos.y / tileSize)
                );

                float distance = std::sqrt(
                    std::pow(x - otherGrid.x, 2) +
                    std::pow(y - otherGrid.y, 2)
                );

                if (distance < 2.0f) { // Minimum 2 tiles apart
                    tooCloseToOther = true;
                    break;
                }
            }

            if (tooCloseToOther) continue;

            // Adjust spawn weights based on level
            Collectable::Type type;
            float value;

            // Higher levels get more powerful powerups
            if (currentLevel > 5) {
                // 50% chance for combat powerups in later levels
                int weightedRoll = gen() % (currentLevel > 10 ? 10 : 15);
                if (weightedRoll < 5) {
                    type = static_cast<Collectable::Type>(2 + gen() % 3); // DAMAGE, SPEED, or FIRE_RATE
                }
                else {
                    type = static_cast<Collectable::Type>(gen() % 2); // HEALTH or MANA
                }

                // Scale values with level
                value = valueDis(gen) * (1.0f + currentLevel * 0.05f);
            }
            else {
                type = static_cast<Collectable::Type>(typeDis(gen));
                value = valueDis(gen);
            }

            // Double health values
            if (type == Collectable::Type::HEALTH) {
                value *= 2.0f;
            }

            collectables.emplace_back(
                sf::Vector2f(x * tileSize + tileSize / 2, y * tileSize + tileSize / 2),
                type,
                value
            );
            validPosition = true;
        }

        if (!validPosition && showCollisionDebug) {
            std::cout << "Failed to find valid position for powerup after " << attempts << " attempts\n";
        }
    }

    powerupSpawned = true;
}

// Update collectables
void Game::updateCollectables(float deltaTime) {
    for (auto& collectable : collectables) {
        collectable.update(deltaTime);
    }

    // Remove collected or expired collectables
    collectables.erase(std::remove_if(collectables.begin(), collectables.end(),
        [](const Collectable& c) { return c.isCollected(); }),
        collectables.end());
}

// Check for collisions between player and collectables
void Game::checkCollectableCollisions() {
    for (auto it = collectables.begin(); it != collectables.end(); ) {
        if (player.getBounds().intersects(it->getBounds())) {
            it->applyEffect(player);
            it = collectables.erase(it);
        }
        else {
            ++it;
        }
    }
}

//spawns enemies with different variables
Game::EnemySpawnWeights Game::getLevelSpawnWeights(int level) const {
    EnemySpawnWeights weights;

    // Use explicit float versions of min/max
    weights.basic = std::max<float>(10.0f, 60.0f - level * 3.0f);
    weights.fast = 20.0f + std::min<float>(20.0f, level * 2.0f) - std::max<float>(0.0f, (level - 5.0f) * 1.5f);
    weights.tank = 5.0f + level * 1.5f;
    weights.ranged = std::min<float>(30.0f, std::max<float>(0.0f, level - 3) * 4.0f);

    return weights;
}

float Game::getHealthMultiplier(int level) const {
    return 1.0f + (level * 0.1f); // +10% health per level
}

float Game::getDamageMultiplier(int level) const {
    return 1.0f + (level * 0.07f); // +7% damage per level
}

float Game::getSpeedMultiplier(int level) const {
    return 1.0f + (level * 0.03f); // +3% speed per level
}
//shows starting screen
void Game::showMainMenu() {
    // Play menu sound (only if not already playing)
    try {
        if (menuSound.getStatus() != sf::Sound::Playing) {
            menuSound.play();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error playing menu sound: " << e.what() << std::endl;
    }

    MainMenu menu(window);
    MainMenu::MenuResult result = menu.Show(window);

    // Stop menu sound when leaving the menu
    menuSound.stop();

    switch (result) {
    case MainMenu::Play:
        showMenu = false;
        gameStarted = true;
        resetGame();
        break;
    case MainMenu::Exit:
        window.close();
        break;
    case MainMenu::Nothing:
        break;
    }
}
//creates a rectangle room
void Game::createRectRoom(int x, int y, int width, int height)
{
    for (int ry = y; ry < y + height && ry < maze.size() - 1; ry++) {
        for (int rx = x; rx < x + width && rx < maze[ry].size() - 1; rx++) {
            maze[ry][rx] = 0;
        }
    }
}
// creates a circular room
void Game::createCircularRoom(int x, int y, int width, int height)
{
    float radiusX = width / 2.0f;
    float radiusY = height / 2.0f;
    float centerX = x + radiusX;
    float centerY = y + radiusY;

    for (int ry = y; ry < y + height && ry < maze.size() - 1; ry++) {
        for (int rx = x; rx < x + width && rx < maze[ry].size() - 1; rx++) {
            float dx = (rx - centerX) / radiusX;
            float dy = (ry - centerY) / radiusY;
            if (dx * dx + dy * dy <= 1.0f) {
                maze[ry][rx] = 0;
            }
        }
    }
}
// creates a winding path
void Game::createWindingPath(std::mt19937& gen)
{
    std::uniform_int_distribution<> posDis(1, maze.size() - 2);
    std::uniform_int_distribution<> dirDis(0, 3);
    std::uniform_int_distribution<> lenDis(5, 15);
    std::uniform_int_distribution<> turnDis(0, 4); // 20% chance to turn

    int x = posDis(gen);
    int y = posDis(gen);
    int length = lenDis(gen);
    int direction = dirDis(gen); // 0=right, 1=left, 2=down, 3=up

    for (int i = 0; i < length; i++) {
        // Carve current position
        if (x > 0 && x < maze[0].size() - 1 && y > 0 && y < maze.size() - 1) {
            maze[y][x] = 0;
            // Carve wider path
            for (int w = 0; w < corridorWidth; w++) {
                if (direction == 0 && x + w < maze[0].size() - 1) maze[y][x + w] = 0;
                if (direction == 1 && x - w > 0) maze[y][x - w] = 0;
                if (direction == 2 && y + w < maze.size() - 1) maze[y + w][x] = 0;
                if (direction == 3 && y - w > 0) maze[y - w][x] = 0;
            }
        }

        // Possibly change direction
        if (turnDis(gen) == 0) {
            direction = dirDis(gen);
        }

        // Move in current direction
        switch (direction) {
        case 0: x++; break;
        case 1: x--; break;
        case 2: y++; break;
        case 3: y--; break;
        }
    }
}
// creates deadendss
void Game::createDeadEnd(std::mt19937& gen)
{
    std::uniform_int_distribution<> posDis(1, maze.size() - 2);
    std::uniform_int_distribution<> lenDis(3, 8);
    std::uniform_int_distribution<> dirDis(0, 3);

    // Find a wall adjacent to a path
    int x, y;
    bool found = false;
    for (int tries = 0; tries < 50 && !found; tries++) {
        x = posDis(gen);
        y = posDis(gen);
        if (maze[y][x] == 1) {
            // Check adjacent tiles for path
            for (int dy = -1; dy <= 1 && !found; dy++) {
                for (int dx = -1; dx <= 1 && !found; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx > 0 && nx < maze[0].size() - 1 &&
                        ny > 0 && ny < maze.size() - 1 &&
                        maze[ny][nx] == 0) {
                        found = true;
                    }
                }
            }
        }
    }

    if (found) {
        int length = lenDis(gen);
        int direction = dirDis(gen);
        for (int i = 0; i < length; i++) {
            if (x > 0 && x < maze[0].size() - 1 && y > 0 && y < maze.size() - 1) {
                maze[y][x] = 0;
                switch (direction) {
                case 0: x++; break;
                case 1: x--; break;
                case 2: y++; break;
                case 3: y--; break;
                }
            }
        }
    }
}
//exit and entry sounds
void Game::loadSounds() {
    // Load your sound files
    std::vector<std::string> soundFiles = {
        "assets/1.wav",
        "assets/2.wav",
        "assets/3.wav",
        "assets/4.wav",
        "assets/5.wav",
        "assets/6.wav"
    };

    for (const auto& file : soundFiles) {
        sf::SoundBuffer buffer;
        if (buffer.loadFromFile(file)) {
            soundBuffers.push_back(buffer);
        }
        else {
            // Error handling
            std::cerr << "Failed to load sound: " << file << std::endl;
        }
    }

    
        // First check if sound files exist
        if (!std::ifstream("assets/menu.wav").good()) {
            std::cerr << "ERROR: menu.wav file missing!" << std::endl;
        }
        if (!std::ifstream("assets/over.wav").good()) {
            std::cerr << "ERROR: over.wav file missing!" << std::endl;
        }

        // Then try loading
        if (!menuBuffer.loadFromFile("assets/menu.wav")) {
            std::cerr << "Failed to load menu sound!" << std::endl;
            // Create empty buffer to prevent crashes
            menuBuffer = sf::SoundBuffer();
        }
        menuSound.setBuffer(menuBuffer);

        if (!gameOverBuffer.loadFromFile("assets/over.wav")) {
            std::cerr << "Failed to load game over sound!" << std::endl;
            gameOverBuffer = sf::SoundBuffer();
        }
        gameOverSound.setBuffer(gameOverBuffer);
    
    menuSound.setLoop(true);
}

//for sounds
void Game::playRandomLevelSound() {
    if (soundBuffers.empty()) return;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, soundBuffers.size() - 1);

    int randomIndex = dis(gen);
    levelSound.setBuffer(soundBuffers[randomIndex]);
    levelSound.play();
}

//ensures a valid maze path is there
bool Game::validateMazePath() {
    // Simple implementation that checks if exit is reachable from player position
    sf::Vector2i playerTile(
        static_cast<int>(player.getPosition().x / tileSize),
        static_cast<int>(player.getPosition().y / tileSize)
    );
    sf::Vector2i exitTile(
        static_cast<int>(exit.getPosition().x / tileSize),
        static_cast<int>(exit.getPosition().y / tileSize)
    );

    // Simple flood fill algorithm
    std::vector<std::vector<bool>> visited(maze.size(), std::vector<bool>(maze[0].size(), false));
    std::queue<sf::Vector2i> queue;
    queue.push(playerTile);
    visited[playerTile.y][playerTile.x] = true;

    while (!queue.empty()) {
        sf::Vector2i current = queue.front();
        queue.pop();

        // Found exit
        if (current == exitTile) {
            return true;
        }

        // Check adjacent tiles
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx != 0 && dy != 0) continue; // Only cardinal directions

                sf::Vector2i next(current.x + dx, current.y + dy);
                if (next.x >= 0 && next.x < maze[0].size() &&
                    next.y >= 0 && next.y < maze.size() &&
                    maze[next.y][next.x] == 0 && !visited[next.y][next.x]) {
                    visited[next.y][next.x] = true;
                    queue.push(next);
                }
            }
        }
    }

    return false;
}

//connects other rooms to main room
void Game::connectMainRooms() {
    // Find all rooms (contiguous 0s)
    std::vector<std::vector<sf::Vector2i>> rooms;
    std::vector<std::vector<bool>> visited(maze.size(), std::vector<bool>(maze[0].size(), false));

    for (int y = 0; y < maze.size(); y++) {
        for (int x = 0; x < maze[y].size(); x++) {
            if (maze[y][x] == 0 && !visited[y][x]) {
                // New room found, flood fill to identify it
                std::vector<sf::Vector2i> room;
                std::queue<sf::Vector2i> queue;
                queue.push({ x, y });
                visited[y][x] = true;

                while (!queue.empty()) {
                    sf::Vector2i current = queue.front();
                    queue.pop();
                    room.push_back(current);

                    // Check adjacent tiles
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            if (dx != 0 && dy != 0) continue; // Only cardinal directions

                            sf::Vector2i next(current.x + dx, current.y + dy);
                            if (next.x >= 0 && next.x < maze[0].size() &&
                                next.y >= 0 && next.y < maze.size() &&
                                maze[next.y][next.x] == 0 && !visited[next.y][next.x]) {
                                visited[next.y][next.x] = true;
                                queue.push(next);
                            }
                        }
                    }
                }
                rooms.push_back(room);
            }
        }
    }

    // Connect rooms with shortest paths
    for (size_t i = 1; i < rooms.size(); i++) {
        // Find closest points between room i and room i-1
        float minDist = FLT_MAX;
        sf::Vector2i from, to;

        for (const auto& pt1 : rooms[i - 1]) {
            for (const auto& pt2 : rooms[i]) {
                float dist = (pt1.x - pt2.x) * (pt1.x - pt2.x) + (pt1.y - pt2.y) * (pt1.y - pt2.y);
                if (dist < minDist) {
                    minDist = dist;
                    from = pt1;
                    to = pt2;
                }
            }
        }

        // Connect these points
        connectPoints(from, to);
    }
}

//allows for debug and camera scroll
void Game::processEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) window.close();

        // Handle zoom with manual clamping
        if (event.type == sf::Event::MouseWheelScrolled) {
            float newZoom = cameraZoom + (event.mouseWheelScroll.delta > 0 ? zoomSpeed : -zoomSpeed);
            // Manual clamp implementation
            if (newZoom < minZoom) newZoom = minZoom;
            if (newZoom > maxZoom) newZoom = maxZoom;
            cameraZoom = newZoom;
            gameView.setSize(window.getSize().x * cameraZoom, window.getSize().y * cameraZoom);
        }

        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F1) {
            showCollisionDebug = !showCollisionDebug;
            player.toggleCollisionDebug(showCollisionDebug);
            player.toggleProjectileDebug(showCollisionDebug);
            for (auto& enemy : enemies) {
                enemy->toggleDebug(showCollisionDebug);
            }
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F1) {
            showCollisionDebug = !showCollisionDebug;

            // Toggle all debug visuals
            player.toggleCollisionDebug(showCollisionDebug);
            player.toggleProjectileDebug(showCollisionDebug);

            for (auto& enemy : enemies) {
                enemy->toggleDebug(showCollisionDebug);  // Changed to use -> operator
            }
        }
    }
}

// updates the game stuffs
void Game::update(float deltaTime) {
    window.setView(gameView);

    // Check for player death first
    if (player.getHealth() <= 0 && !gameOver) {
        std::cout << "Player died! Triggering game over..." << std::endl;
        gameOver = true;
        return;  // Skip rest of update if game over
    }

    // Check projectile collisions with player
    for (auto& enemy : enemies) {
        for (const auto& projCollider : enemy->getProjectileColliders()) {
            if (player.getBounds().intersects(projCollider)) {
                player.takeDamage(20); // Adjust damage as needed
                enemy->clearProjectiles();
                break;
            }
        }
    }

    // Store player's position before movement
    sf::Vector2f previousPosition = player.getPosition();

    // Update player
    player.update(deltaTime, window);

    if (!player.isAlive() && !gameOver) {
        gameOver = true;
        return;
    }
    // Wall collision detection
    sf::Vector2i playerTile(
        static_cast<int>(player.getPosition().x / tileSize),
        static_cast<int>(player.getPosition().y / tileSize)
    );


    
    bool collided = false;
    for (int y = std::max(0, playerTile.y - 1); y <= std::min(static_cast<int>(maze.size()) - 1, playerTile.y + 1); y++) {
        for (int x = std::max(0, playerTile.x - 1); x <= std::min(static_cast<int>(maze[0].size()) - 1, playerTile.x + 1); x++) {
            if (maze[y][x] == 1) {
                sf::FloatRect wallRect(x * tileSize, y * tileSize, tileSize, tileSize);
                sf::FloatRect playerBounds = player.getBounds();

                if (playerBounds.intersects(wallRect)) {
                    // Calculate center points
                    sf::Vector2f playerCenter(
                        playerBounds.left + playerBounds.width / 2,
                        playerBounds.top + playerBounds.height / 2
                    );
                    sf::Vector2f wallCenter(
                        wallRect.left + wallRect.width / 2,
                        wallRect.top + wallRect.height / 2
                    );

                    // Calculate minimum translation vector
                    float pushX = (playerCenter.x < wallCenter.x) ?
                        wallRect.left - (playerBounds.left + playerBounds.width) :
                        (wallRect.left + wallRect.width) - playerBounds.left;

                    float pushY = (playerCenter.y < wallCenter.y) ?
                        wallRect.top - (playerBounds.top + playerBounds.height) :
                        (wallRect.top + wallRect.height) - playerBounds.top;

                    // Only push along the axis of least penetration
                    if (std::abs(pushX) < std::abs(pushY)) {
                        player.setPosition(player.getPosition().x + pushX, player.getPosition().y);
                    }
                    else {
                        player.setPosition(player.getPosition().x, player.getPosition().y + pushY);
                    }

                    collided = true;
                    break;
                }
            }
        }
        if (collided) break;
    }

    // If collision occurred and we're still intersecting, revert position
    if (collided) {
        sf::FloatRect playerBounds = player.getBounds();
        for (int y = std::max(0, playerTile.y - 1); y <= std::min(static_cast<int>(maze.size()) - 1, playerTile.y + 1); y++) {
            for (int x = std::max(0, playerTile.x - 1); x <= std::min(static_cast<int>(maze[0].size()) - 1, playerTile.x + 1); x++) {
                if (maze[y][x] == 1) {
                    sf::FloatRect wallRect(x * tileSize, y * tileSize, tileSize, tileSize);
                    if (playerBounds.intersects(wallRect)) {
                        player.setPosition(previousPosition);
                        break;
                    }
                }
            }
        }
    }

    // Update enemies
    for (auto it = enemies.begin(); it != enemies.end(); ) {
        if (!(*it)->isAlive()) {
            it = enemies.erase(it);
            enemiesKilledThisLevel++;
            totalEnemiesKilled++;
        }
        else {
            (*it)->update(deltaTime, maze, tileSize, getEnemyPointers());
            if (showCollisionDebug) {
                (*it)->toggleDebug(showCollisionDebug);
            }
            ++it;
        }
    }
    //collectables 
    updateCollectables(deltaTime);
    checkCollectableCollisions();
    // Update camera
    gameView.setCenter(player.getPosition());

    // Check level completion
    checkLevelCompletion();

    // Update UI text
    levelText.setString("Level: " + std::to_string(currentLevel));
    killsText.setString("Kills: " + std::to_string(enemiesKilledThisLevel) +
        " (Total: " + std::to_string(totalEnemiesKilled) + ")");

    // Show high score for current level if it exists
    if (highScores.find(currentLevel) != highScores.end()) {
        highScoreText.setString("High Score: " + std::to_string(highScores[currentLevel]));
    }
    else {
        highScoreText.setString("High Score: 0");
    }
}
// renders the walls floor player etc window too
void Game::render() {
    window.clear(sf::Color::Black);

    // Draw game world
    window.setView(gameView);
    drawMaze();
    window.draw(exit);
    player.draw(window);
    drawEnemies();
    for (const auto& collectable : collectables) {
        collectable.draw(window);
    }
    // Draw UI
    window.setView(uiView);
    drawUI();

    window.display();
}

//generates a suitable maze 
void Game::generateMaze() {
    int size = baseSize + (currentLevel - 1) * sizeIncreasePerLevel;
    maze.assign(size, std::vector<int>(size, 1)); // Start with all walls

    // Generate until we get a valid maze
    bool valid = false;
    int attempts = 0;

    while (!valid && attempts < 5) {
        // Clear previous attempt (keep outer walls)
        for (int y = 1; y < size - 1; y++) {
            for (int x = 1; x < size - 1; x++) {
                maze[y][x] = 1;
            }
        }

        // Enhanced generation with new parameters
        generateRooms();
        connectMainRooms();
        createOpenAreas();
        addMazeFeatures(); 

        valid = validateMazePath();
        attempts++;
    }

    placePlayer();
    placeExit();
    spawnEnemies();
}

//generates rooms 
void Game::generateRooms() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> roomSizeDis(minRoomSize, maxRoomSize);
    std::uniform_int_distribution<> posDis(2, static_cast<int>(maze.size()) - 3);
    std::uniform_int_distribution<> roomVarDis(0, 10);

    int roomCount = 12 + (currentLevel * 3); // More rooms

    for (int i = 0; i < roomCount; i++) {
        int roomWidth = roomSizeDis(gen);
        int roomHeight = roomSizeDis(gen);
        int x = posDis(gen);
        int y = posDis(gen);

        // Ensure room stays within bounds
        x = std::max(2, std::min(x, static_cast<int>(maze[0].size()) - roomWidth - 2));
        y = std::max(2, std::min(y, static_cast<int>(maze.size()) - roomHeight - 2));

        // More varied room shapes
        if (roomVarDis(gen) > 7) { // 30% chance for non-rectangular rooms
            createCircularRoom(x, y, roomWidth, roomHeight);
        }
        else {
            createRectRoom(x, y, roomWidth, roomHeight);
        }
    }
}

//connects therooms for the player to move through
void Game::connectRooms() {
    std::vector<sf::Vector2i> roomCenters;

    // Find room centers (simplified version)
    for (int y = 1; y < maze.size() - 1; y++) {
        for (int x = 1; x < maze[y].size() - 1; x++) {
            if (maze[y][x] == 0) {
                roomCenters.emplace_back(x, y);
            }
        }
    }

    // Connect rooms
    if (!roomCenters.empty()) {
        for (size_t i = 1; i < roomCenters.size(); i++) {
            connectPoints(roomCenters[i - 1], roomCenters[i]);
        }
    }
}

//adds features to the maze
void Game::connectPoints(sf::Vector2i p1, sf::Vector2i p2) {
    // Horizontal connection
    int stepX = p1.x < p2.x ? 1 : -1;
    for (int x = p1.x; x != p2.x; x += stepX) {
        for (int dy = -corridorWidth / 2; dy <= corridorWidth / 2; dy++) {
            int y = p1.y + dy;
            if (y > 0 && y < maze.size() - 1 && x > 0 && x < maze[y].size() - 1) {
                maze[y][x] = 0;
            }
        }
    }

    // Vertical connection
    int stepY = p1.y < p2.y ? 1 : -1;
    for (int y = p1.y; y != p2.y; y += stepY) {
        for (int dx = -corridorWidth / 2; dx <= corridorWidth / 2; dx++) {
            int x = p2.x + dx;
            if (x > 0 && x < maze[0].size() - 1 && y > 0 && y < maze.size() - 1) {
                maze[y][x] = 0;
            }
        }
    }
}

//ensures open areas are created
void Game::createOpenAreas() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> areaSizeDis(5, 15);
    std::uniform_int_distribution<> posDis(1, maze.size() - 2);

    int areaCount = 3 + (currentLevel / 2);
    for (int i = 0; i < areaCount; i++) {
        int size = areaSizeDis(gen);
        int x = posDis(gen);
        int y = posDis(gen);

        for (int dy = -size; dy <= size; dy++) {
            for (int dx = -size; dx <= size; dx++) {
                if (dx * dx + dy * dy <= size * size) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx > 0 && nx < maze[0].size() - 1 && ny > 0 && ny < maze.size() - 1) {
                        maze[ny][nx] = 0;
                    }
                }
            }
        }
    }
}

//special areas are created but not used in game , fur further update i am including this
void Game::placeSpecialAreas() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> countDis(3, 5);
    std::uniform_int_distribution<> posDis(1, maze.size() - 2);

    int specialCount = countDis(gen);
    for (int i = 0; i < specialCount; i++) {
        int x = posDis(gen);
        int y = posDis(gen);

        // Find open space
        while (y < maze.size() && x < maze[y].size() && maze[y][x] == 1) {
            x = posDis(gen);
            y = posDis(gen);
        }

        // Create 5x5 area
        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                int nx = x + dx;
                int ny = y + dy;
                if (nx > 0 && nx < maze[0].size() - 1 && ny > 0 && ny < maze.size() - 1) {
                    maze[ny][nx] = 0;
                    if (dx == 0 && dy == 0) {
                        maze[ny][nx] = 2; // Mark center as special
                    }
                }
            }
        }
    }
}

//places the player in the maze
void Game::placePlayer() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, maze.size() - 2);

    while (true) {
        int x = dis(gen);
        int y = dis(gen);

        if (maze[y][x] == 0) {
            player.setPosition(x * tileSize + tileSize / 2, y * tileSize + tileSize / 2);
            break;
        }
    }
}

//places the exit in the maze
void Game::placeExit() {
    std::random_device rd;
    std::mt19937 gen(rd());
    // Ensure exit is placed at least 1 tile away from outer walls
    std::uniform_int_distribution<> dis(2, static_cast<int>(maze.size()) - 3);

    while (true) {
        int x = dis(gen);
        int y = dis(gen);

        if (maze[y][x] == 0) {
            sf::Vector2f playerPos = player.getPosition();
            sf::Vector2f exitPos(x * tileSize + tileSize / 2, y * tileSize + tileSize / 2);

            // Ensure exit is far enough from player
            if (isFarEnough(
                sf::Vector2i(playerPos.x / tileSize, playerPos.y / tileSize),
                sf::Vector2i(x, y),
                maze.size() * 0.4f)) {

                exit.setSize(sf::Vector2f(tileSize, tileSize));
                exit.setPosition(x * tileSize, y * tileSize);
                exit.setFillColor(sf::Color::Green);
                break;
            }
        }
    }
}

//spawns enemies randomly
void Game::spawnEnemies() {
    enemies.clear();
    enemiesKilledThisLevel = 0;
    int enemyCount = baseEnemies + (currentLevel - 1) * enemiesIncreasePerLevel;

    auto weights = getLevelSpawnWeights(currentLevel);
    float totalWeight = weights.basic + weights.fast + weights.tank + weights.ranged;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> typeDis(0.0f, totalWeight);
    std::uniform_real_distribution<float> healthDis(80.0f, 120.0f);
    std::uniform_int_distribution<> posDis(1, static_cast<int>(maze.size()) - 2);

    for (int i = 0; i < enemyCount; i++) {
        while (true) {
            int x = posDis(gen);
            int y = posDis(gen);

            if (maze[y][x] == 0) {
                sf::Vector2f pos(x * tileSize + tileSize / 2, y * tileSize + tileSize / 2);

                // Determine enemy type
                float typeRoll = typeDis(gen);
                Enemy::EnemyType type;

                if (typeRoll < weights.basic) {
                    type = Enemy::BASIC;
                }
                else if (typeRoll < weights.basic + weights.fast) {
                    type = Enemy::FAST;
                }
                else if (typeRoll < weights.basic + weights.fast + weights.tank) {
                    type = Enemy::TANK;
                }
                else {
                    type = Enemy::RANGED;
                }

                // Apply level scaling
                float health = healthDis(gen) * getHealthMultiplier(currentLevel);
                auto enemy = std::make_unique<Enemy>(pos, health, type);
                enemy->setPlayer(&player);

                // Scale stats
                enemy->setProjectileDamage(15.0f * getDamageMultiplier(currentLevel));
                enemy->setMovementSpeed(100.0f * getSpeedMultiplier(currentLevel)); // Changed to setMovementSpeed

                // Type-specific base adjustments
                switch (type) {
                case Enemy::FAST:
                    enemy->setProjectileSpeed(500.0f);
                    enemy->setAttackCooldown(0.3f);
                    enemy->setMovementSpeed(150.0f * getSpeedMultiplier(currentLevel)); // Faster base speed
                    break;
                case Enemy::TANK:
                    enemy->setProjectileDamage(25.0f * getDamageMultiplier(currentLevel));
                    enemy->setAttackCooldown(1.0f);
                    enemy->setMovementSpeed(70.0f * getSpeedMultiplier(currentLevel)); // Slower base speed
                    break;
                case Enemy::RANGED:
                    enemy->setProjectileSpeed(450.0f);
                    enemy->setAttackRange(350.0f);
                    break;
                }

                enemies.push_back(std::move(enemy));
                break;
            }
        }
    }
}

//updates enemies position render state etc
void Game::updateEnemies(float deltaTime) {
    auto livingEnemies = getEnemyPointers();

    for (auto it = enemies.begin(); it != enemies.end(); ) {
        auto& enemy = *it;
        enemy->update(deltaTime, maze, tileSize, livingEnemies);

        // Wall collision
        sf::FloatRect enemyBounds = enemy->getCollisionBox();
        for (int y = 0; y < maze.size(); y++) {
            for (int x = 0; x < maze[y].size(); x++) {
                if (maze[y][x] == 1) {
                    sf::FloatRect wallRect(x * tileSize, y * tileSize, tileSize, tileSize);
                    if (enemyBounds.intersects(wallRect)) {
                        sf::Vector2f enemyPos = enemy->getPosition();
                        sf::Vector2f wallCenter(x * tileSize + tileSize / 2, y * tileSize + tileSize / 2);
                        sf::Vector2f dir = enemyPos - wallCenter;
                        float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                        if (length > 0) {
                            enemy->setPosition(wallCenter + (dir / length) * (tileSize * 0.6f));
                        }
                    }
                }
            }
        }
        // Projectile-wall collision
        for (auto& proj : enemy->getProjectiles()) {
            sf::Vector2i projTile(
                static_cast<int>(proj.sprite.getPosition().x / tileSize),
                static_cast<int>(proj.sprite.getPosition().y / tileSize)
            );

            for (int y = std::max(0, projTile.y - 1); y <= std::min(static_cast<int>(maze.size()) - 1, projTile.y + 1); y++) {
                for (int x = std::max(0, projTile.x - 1); x <= std::min(static_cast<int>(maze[0].size()) - 1, projTile.x + 1); x++) {
                    if (maze[y][x] == 1) {
                        sf::FloatRect wallRect(x * tileSize, y * tileSize, tileSize, tileSize);
                        if (proj.collisionBox.intersects(wallRect)) {
                            // Mark projectile for removal
                            enemy->clearProjectiles();
                            break;
                        }
                    }
                }
            }
        }

        if (!enemy->isAlive()) {
            it = enemies.erase(it);
            enemiesKilledThisLevel++;
            totalEnemiesKilled++;
        }
        else {
            ++it;
        }
    }
}

//draws the enemies
void Game::drawEnemies() {
    for (auto& enemy : enemies) {
        enemy->draw(window);
        if (showCollisionDebug) {
            enemy->drawDebug(window);
        }
    }
}

//draws the maze
void Game::drawMaze() {
    // Get player position in tile coordinates
    sf::Vector2f playerPos = player.getPosition();
    int px = static_cast<int>(playerPos.x / tileSize);
    int py = static_cast<int>(playerPos.y / tileSize);

    // Calculate visible area with clamping
    int startX = std::max(0, px - renderDistance);
    int endX = std::min(static_cast<int>(maze[0].size()) - 1, px + renderDistance);
    int startY = std::max(0, py - renderDistance);
    int endY = std::min(static_cast<int>(maze.size()) - 1, py + renderDistance);

    // Pre-create tiles for better performance
    sf::RectangleShape wallTile(sf::Vector2f(tileSize - 1, tileSize - 1));
    wallTile.setFillColor(sf::Color(70, 70, 70));
    wallTile.setOutlineThickness(0.f);

    sf::RectangleShape floorTile(sf::Vector2f(tileSize - 1, tileSize - 1));
    floorTile.setFillColor(sf::Color(30, 30, 30));
    floorTile.setOutlineThickness(0.f);

    // Draw tiles in the visible area
    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            sf::Vector2f position(x * tileSize, y * tileSize);

            if (maze[y][x] == 1) { // Wall
                wallTile.setPosition(position);
                window.draw(wallTile);
            }
            else { // Floor (0)
                floorTile.setPosition(position);
                window.draw(floorTile);
            }
        }
    }

    // Draw exit with appropriate color based on enemy status
    if (enemies.empty()) {
        exit.setFillColor(sf::Color::Green);
    }
    else {
        exit.setFillColor(sf::Color(100, 100, 100));
    }
    window.draw(exit);
}

//draws the UI
void Game::drawUI() {
    window.draw(levelText);
    window.draw(killsText);
    window.draw(highScoreText);
}

//checks if the level is completed
void Game::checkLevelCompletion() {
    // Only check for level completion if player is at exit AND all enemies are dead
    if (player.getBounds().intersects(exit.getGlobalBounds()) && enemies.empty()) {
        levelComplete = true;
        saveHighScores();
        nextLevel();
    }
}

//makes level progression
void Game::nextLevel() {
    currentLevel++;
    generateMaze();
    levelComplete = false;
    playRandomLevelSound();
	powerupSpawned = false;
	spawnCollectables();
}

//loads highscore from file
void Game::loadHighScores() {
    std::ifstream file("scores.txt");
    if (file.is_open()) {
        file >> highScoreLevel;
        file.close();
    }
    else {
        // Create file if doesn't exist
        highScoreLevel = 0;
        std::ofstream newFile("scores.txt");
        newFile << highScoreLevel;
        newFile.close();
    }
}
//saves highscore to file
void Game::saveHighScores() {
    // Update high score if current level is higher
    if (currentLevel > highScoreLevel) {
        highScoreLevel = currentLevel;
    }

    // Save to file
    std::ofstream file("scores.txt");
    if (file.is_open()) {
        file << highScoreLevel;
        file.close();
    }
}
//resets game when dead
void Game::resetGame()
{
    currentLevel = 1;
    totalEnemiesKilled = 0;
    enemiesKilledThisLevel = 0;
    player.reset();
    generateMaze();
    spawnCollectables();
    gameOver = false;
    showMenu = false;
    gameStarted = true; 
    powerupSpawned = false;
    playRandomLevelSound();
}

// could have used another class but :) forgot to 
void Game::showGameOverScreen() {
    // Play game over sound
    gameOverSound.play();

    // Create dark overlay background
    sf::RectangleShape overlay(sf::Vector2f(window.getSize().x, window.getSize().y));
    overlay.setFillColor(sf::Color(0, 0, 0, 200));

    // Load Arial font 
    if (!font.loadFromFile("assets/arial.ttf")) {
        std::cerr << "Failed to load Arial font!" << std::endl;
    }

    // Title text with medieval style wording
    sf::Text titleText;
    titleText.setFont(font);
    titleText.setString("Thou Hast Fallen");
    titleText.setCharacterSize(72);
    titleText.setFillColor(sf::Color(200, 50, 50)); // Dark red color
    titleText.setStyle(sf::Text::Bold);

    // Center title
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin(titleBounds.left + titleBounds.width / 2.0f,
        titleBounds.top + titleBounds.height / 2.0f);
    titleText.setPosition(window.getSize().x / 2.0f, 120.f);

    // Score text with medieval style wording
    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setString("Vanquished Foes: " + std::to_string(totalEnemiesKilled));
    scoreText.setCharacterSize(48);
    scoreText.setFillColor(sf::Color(200, 180, 100)); // Gold color

    // Center score
    sf::FloatRect scoreBounds = scoreText.getLocalBounds();
    scoreText.setOrigin(scoreBounds.left + scoreBounds.width / 2.0f,
        scoreBounds.top + scoreBounds.height / 2.0f);
    scoreText.setPosition(window.getSize().x / 2.0f, 220.f);

    // Define buttons
    struct Button {
        sf::Rect<float> rect;
        sf::RectangleShape shape;
        sf::Text text;
        enum Action { Restart, Menu, Exit, Nothing } action;
    };

    std::vector<Button> buttons;

    auto createButton = [&](const std::string& label, float yPos, sf::Color color, Button::Action action) {
        Button btn;
        btn.rect = sf::Rect<float>(window.getSize().x / 2.f - 150.f, yPos, 300.f, 60.f);
        btn.shape.setSize(sf::Vector2f(btn.rect.width, btn.rect.height));
        btn.shape.setPosition(btn.rect.left, btn.rect.top);
        btn.shape.setFillColor(color);
        btn.shape.setOutlineColor(sf::Color(150, 150, 150));
        btn.shape.setOutlineThickness(2.f);

        btn.text.setFont(font);
        btn.text.setString(label);
        btn.text.setCharacterSize(30);
        btn.text.setFillColor(sf::Color::Black);

        // Center text in button
        sf::FloatRect textBounds = btn.text.getLocalBounds();
        btn.text.setOrigin(textBounds.left + textBounds.width / 2.0f,
            textBounds.top + textBounds.height / 2.0f);
        btn.text.setPosition(btn.rect.left + btn.rect.width / 2.0f,
            btn.rect.top + btn.rect.height / 2.0f);

        btn.action = action;
        buttons.push_back(btn);
        };

    // Create buttons with medieval-style labels
    float buttonY = window.getSize().y / 2.f;
    createButton("Rise Again", buttonY, sf::Color(100, 200, 100), Button::Restart);
    createButton("Retreat to Menu", buttonY + 80.f, sf::Color(200, 200, 100), Button::Menu);
    createButton("Abandon Quest", buttonY + 160.f, sf::Color(200, 100, 100), Button::Exit);

    // Game Over Loop
    while (window.isOpen() && gameOver) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                int mx = event.mouseButton.x;
                int my = event.mouseButton.y;
                for (auto& btn : buttons) {
                    if (btn.rect.contains(mx, my)) {
                        switch (btn.action) {
                        case Button::Restart:
                            resetGame();
                            return;
                        case Button::Menu:
                            showMenu = true;
                            gameOver = false;
                            gameStarted = false;
                            return;
                        case Button::Exit:
                            window.close();
                            return;
                        default:
                            break;
                        }
                    }
                }
            }

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::R) {
                    resetGame();
                    return;
                }
                else if (event.key.code == sf::Keyboard::M) {
                    showMenu = true;
                    gameOver = false;
                    gameStarted = false;
                    return;
                }
                else if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                    return;
                }
            }
        }

        // Hover effect
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        for (auto& btn : buttons) {
            if (btn.rect.contains(mousePos.x, mousePos.y)) {
                btn.shape.setOutlineColor(sf::Color::Yellow);
                btn.shape.setOutlineThickness(4.f);
                btn.text.setFillColor(sf::Color::White);
            }
            else {
                btn.shape.setOutlineColor(sf::Color(150, 150, 150));
                btn.shape.setOutlineThickness(2.f);
                btn.text.setFillColor(sf::Color::Black);
            }
        }

        // Draw
        window.clear();

        // Draw the game view first (frozen in its current state)
        window.setView(gameView);
        drawMaze();
        player.draw(window);
        drawEnemies();

        // Draw UI overlay
        window.setView(uiView);
        window.draw(overlay);
        window.draw(titleText);
        window.draw(scoreText);

        // Draw buttons
        for (const auto& btn : buttons) {
            window.draw(btn.shape);
            window.draw(btn.text);
        }

        window.display();
    }
}

//draws the digits for the score
void Game::drawDigit(float x, float y, int digit)
{
    // Each digit is made of up to 7 segments (like a digital display)
    sf::RectangleShape segment(sf::Vector2f(15, 5));
    segment.setFillColor(sf::Color::White);

    switch (digit) {
    case 0:
        // Top
        segment.setSize(sf::Vector2f(20, 5));
        segment.setPosition(x, y);
        window.draw(segment);
        // Left top
        segment.setSize(sf::Vector2f(5, 20));
        segment.setPosition(x, y);
        window.draw(segment);
        // Right top
        segment.setPosition(x + 15, y);
        window.draw(segment);
        // Bottom
        segment.setSize(sf::Vector2f(20, 5));
        segment.setPosition(x, y + 25);
        window.draw(segment);
        // Left bottom
        segment.setSize(sf::Vector2f(5, 20));
        segment.setPosition(x, y + 10);
        window.draw(segment);
        // Right bottom
        segment.setPosition(x + 15, y + 10);
        window.draw(segment);
        break;
    case 1:
        // Right top
        segment.setSize(sf::Vector2f(5, 25));
        segment.setPosition(x + 15, y);
        window.draw(segment);
        break;
        // Implement cases 2-9 similarly...
    case 2:
        // Implement digit 2
        break;
    case 3:
        // Implement digit 3
        break;
        // ... and so on for digits 4-9
    default:
        break;
    }
}

//draws the number on the screen
void Game::drawNumber(float x, float y, int number) {
    std::string numStr = std::to_string(number);
    float currentX = x;

    for (char c : numStr) {
        int digit = c - '0';
        drawDigit(currentX, y, digit);
        currentX += 25; // Space between digits
    }
}

//checks if the position is valid
bool Game::isValidPosition(sf::Vector2i pos) {
    return pos.x > 0 && pos.x < maze.size() - 1 && pos.y > 0 && pos.y < maze.size() - 1;
}

//checks if the distance between two points is valid
bool Game::isFarEnough(sf::Vector2i pos1, sf::Vector2i pos2, float minDistance) {
    float dx = pos1.x - pos2.x;
    float dy = pos1.y - pos2.y;
    return std::sqrt(dx * dx + dy * dy) >= minDistance;
}

//creates deadends in the maze
void Game::addDeadEnds() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> countDis(5, 10 + currentLevel);
    std::uniform_int_distribution<> lengthDis(3, 8);
    std::uniform_int_distribution<> posDis(1, maze.size() - 2);

    for (int i = 0; i < countDis(gen); i++) {
        int x = posDis(gen);
        int y = posDis(gen);
        int direction = gen() % 4;
        int length = lengthDis(gen);

        // Find a wall adjacent to a path
        bool foundStart = false;
        for (int tries = 0; tries < 20 && !foundStart; tries++) {
            x = posDis(gen);
            y = posDis(gen);

            if (maze[y][x] == 1) {
                // Check if adjacent to a path
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx > 0 && nx < maze[0].size() - 1 && ny > 0 && ny < maze.size() - 1) {
                            if (maze[ny][nx] == 0) {
                                foundStart = true;
                                break;
                            }
                        }
                    }
                    if (foundStart) break;
                }
            }
        }

        if (foundStart) {
            for (int j = 0; j < length; j++) {
                if (x > 0 && x < maze[0].size() - 1 && y > 0 && y < maze.size() - 1) {
                    maze[y][x] = 0;
                    switch (direction) {
                    case 0: x++; break;
                    case 1: x--; break;
                    case 2: y++; break;
                    case 3: y--; break;
                    }
                }
            }
        }
    }
}
//creates winding paths in the maze
void Game::addDiagonalPaths() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> countDis(3, 5 + currentLevel / 2);
    std::uniform_int_distribution<> posDis(1, maze.size() - 2);

    for (int i = 0; i < countDis(gen); i++) {
        int x1 = posDis(gen);
        int y1 = posDis(gen);
        int x2 = posDis(gen);
        int y2 = posDis(gen);

        // Ensure both ends are in walkable areas
        if (maze[y1][x1] == 0 && maze[y2][x2] == 0) {
            int dx = (x2 > x1) ? 1 : -1;
            int dy = (y2 > y1) ? 1 : -1;

            while (x1 != x2 && y1 != y2) {
                x1 += dx;
                y1 += dy;
                if (x1 > 0 && x1 < maze[0].size() - 1 && y1 > 0 && y1 < maze.size() - 1) {
                    maze[y1][x1] = 0;
                    // Add some width to the diagonal
                    if (gen() % 2 == 0 && x1 + dx > 0 && x1 + dx < maze[0].size() - 1) maze[y1][x1 + dx] = 0;
                    if (gen() % 2 == 0 && y1 + dy > 0 && y1 + dy < maze.size() - 1) maze[y1 + dy][x1] = 0;
                }
            }
        }
    }
}
//adds features
void Game::addMazeFeatures() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> featDis(0, 3);

    // Add winding paths
    for (int i = 0; i < 5 + currentLevel; i++) {
        createWindingPath(gen);
    }

    // Add some dead ends
    for (int i = 0; i < 3 + currentLevel; i++) {
        createDeadEnd(gen);
    }
}



