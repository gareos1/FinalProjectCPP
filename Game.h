#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#include <queue>
#include <random>
#include <limits>
#include <fstream>
#include "Player.h"
#include "Enemy.h"
#include "MainMenu.h"
#include "Collectable.h"

class Game {
public:
    // Constructor/Destructor
    Game();
    ~Game();

    // Core game loop
    void run();

    // Debug and enemy management
    void toggleCollisionDebug();
    std::vector<Enemy*> getEnemyPointers();
    void addEnemy(sf::Vector2f position, float health, Enemy::EnemyType type);

private:
    // ===== Collectables System =====
    std::vector<Collectable> collectables;
    bool powerupSpawned = false;
    void spawnCollectables();
    void updateCollectables(float deltaTime);
    void checkCollectableCollisions();

    // ===== Difficulty Scaling =====
    struct EnemySpawnWeights {
        float basic;
        float fast;
        float tank;
        float ranged;
    };
    EnemySpawnWeights getLevelSpawnWeights(int level) const;
    float getHealthMultiplier(int level) const;
    float getDamageMultiplier(int level) const;
    float getSpeedMultiplier(int level) const;

    // ===== Game State =====
    bool gameStarted = false;
    bool showMenu = true;
    bool showCollisionDebug = false;
    int currentLevel = 1;
    int enemiesKilledThisLevel = 0;
    int totalEnemiesKilled = 0;
    bool levelComplete = false;
    bool gameOver = false;
    int highScoreLevel = 0;

    // ===== Maze Generation Constants =====
    static const int baseSize = 60;
    static const int sizeIncreasePerLevel = 12;
    static const float tileSize;
    static const int minRoomSize = 8;
    static const int maxRoomSize = 18;
    static const int corridorWidth = 5;
    static const int renderDistance = 20;
    static const int baseEnemies = 3;
    static const int enemiesIncreasePerLevel = 2;

    // ===== Game Objects =====
    std::vector<std::unique_ptr<Enemy>> enemies;
    std::vector<std::vector<int>> maze;
    Player player;
    sf::RectangleShape exit;

    // ===== Audio System =====
    std::vector<sf::SoundBuffer> soundBuffers;
    sf::Sound levelSound;
    sf::SoundBuffer gameOverBuffer;
    sf::Sound gameOverSound;
    sf::SoundBuffer menuBuffer;
    sf::Sound menuSound;
    void loadSounds();
    void playRandomLevelSound();

    // ===== Views and UI =====
    sf::RenderWindow window;
    sf::View gameView;
    sf::View uiView;
    sf::Font font;
    sf::Text levelText;
    sf::Text killsText;
    sf::Text highScoreText;

    // ===== Camera System =====
    float cameraZoom = 0.5f;
    const float minZoom = 0.3f;
    const float maxZoom = 1.0f;
    const float zoomSpeed = 0.1f;

    // ===== Score System =====
    std::unordered_map<int, int> highScores;
    const std::string scoreFile = "scores.txt";
    void loadHighScores();
    void saveHighScores();

    // ===== Menu System =====
    void showMainMenu();
    void showGameOverScreen();

    // ===== Maze Generation =====
    void createRectRoom(int x, int y, int width, int height);
    void createCircularRoom(int x, int y, int width, int height);
    void createWindingPath(std::mt19937& gen);
    void createDeadEnd(std::mt19937& gen);
    void connectPoints(sf::Vector2i p1, sf::Vector2i p2);
    void addDeadEnds();
    void addDiagonalPaths();
    void addMazeFeatures();

    // ===== Core Game Methods =====
    void processEvents();
    void update(float deltaTime);
    void render();
    void generateMaze();
    bool validateMazePath();
    void connectMainRooms();
    void generateRooms();
    void connectRooms();
    void createOpenAreas();
    void placeSpecialAreas();
    void placePlayer();
    void placeExit();
    void spawnEnemies();
    void updateEnemies(float deltaTime);
    void drawEnemies();
    void checkLevelCompletion();
    void nextLevel();
    void resetGame();

    // ===== Rendering Methods =====
    void drawMaze();
    void drawUI();
    void drawDigit(float x, float y, int digit);
    void drawNumber(float x, float y, int number);

    // ===== Utility Methods =====
    bool isValidPosition(sf::Vector2i pos);
    bool isFarEnough(sf::Vector2i pos1, sf::Vector2i pos2, float minDistance);
};