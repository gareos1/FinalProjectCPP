#include "MainMenu.h"
#include <iostream>

MainMenu::MainMenu(sf::RenderWindow& window) {
    // Load font
    if (!font.loadFromFile("assets/arial.ttf")) {
        std::cerr << "Error loading font file" << std::endl;
    }

    // Load background image
    if (!backgroundTexture.loadFromFile("assets/bg.jpg")) {
        std::cerr << "Error loading background image" << std::endl;
        // Fallback: create a simple color background
        backgroundTexture.create(window.getSize().x, window.getSize().y);
        backgroundTexture.setSmooth(true);
    }

    // Set up background sprite
    backgroundSprite.setTexture(backgroundTexture);

    // Scale background to fit window while maintaining aspect ratio
    float scaleX = (float)window.getSize().x / backgroundTexture.getSize().x;
    float scaleY = (float)window.getSize().y / backgroundTexture.getSize().y;
    backgroundSprite.setScale(scaleX, scaleY);

    SetupMenuItems(window);
}

void MainMenu::CreateTextButton(MenuItem& item, const std::string& label, sf::Color color) {
    item.shape.setSize(sf::Vector2f(item.rect.width, item.rect.height));
    item.shape.setPosition(item.rect.left, item.rect.top);
    item.shape.setFillColor(color);
    item.shape.setOutlineThickness(2.f);
    item.shape.setOutlineColor(sf::Color(150, 150, 150));

    item.text.setFont(font);
    item.text.setString(label);
    item.text.setCharacterSize(24);
    item.text.setFillColor(sf::Color::White);

    // Center text in button
    sf::FloatRect textRect = item.text.getLocalBounds();
    item.text.setOrigin(textRect.left + textRect.width / 2.0f,
        textRect.top + textRect.height / 2.0f);
    item.text.setPosition(sf::Vector2f(item.rect.left + item.rect.width / 2.0f,
        item.rect.top + item.rect.height / 2.0f));
}

void MainMenu::SetupMenuItems(sf::RenderWindow& window) {
    // Play button
    MenuItem playButton;
    playButton.rect = sf::Rect<float>(
        window.getSize().x / 2 - 100.f,
        window.getSize().y / 2 - 50.f,
        200.f, 50.f
    );
    playButton.action = Play;
    CreateTextButton(playButton, "PLAY", sf::Color(100, 200, 100, 200)); // Added alpha for transparency
    menuItems.push_back(playButton);

    // Exit button
    MenuItem exitButton;
    exitButton.rect = sf::Rect<float>(
        window.getSize().x / 2 - 100.f,
        window.getSize().y / 2 + 50.f,
        200.f, 50.f
    );
    exitButton.action = Exit;
    CreateTextButton(exitButton, "EXIT", sf::Color(200, 100, 100, 200)); // Added alpha for transparency
    menuItems.push_back(exitButton);
}

MainMenu::MenuResult MainMenu::Show(sf::RenderWindow& window) {
    return GetMenuResponse(window);
}

MainMenu::MenuResult MainMenu::GetMenuResponse(sf::RenderWindow& window) {
    sf::Event menuEvent;

    while (window.isOpen()) {
        while (window.pollEvent(menuEvent)) {
            if (menuEvent.type == sf::Event::Closed) {
                return Exit;
            }

            if (menuEvent.type == sf::Event::MouseButtonPressed) {
                if (menuEvent.mouseButton.button == sf::Mouse::Left) {
                    return HandleClick(menuEvent.mouseButton.x, menuEvent.mouseButton.y);
                }
            }

            if (menuEvent.type == sf::Event::KeyPressed) {
                if (menuEvent.key.code == sf::Keyboard::Escape) {
                    return Exit;
                }
                if (menuEvent.key.code == sf::Keyboard::Return) {
                    return Play;
                }
            }
        }

        // Update hover effects
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        for (auto& item : menuItems) {
            if (item.rect.contains(mousePos.x, mousePos.y)) {
                item.shape.setOutlineColor(sf::Color::Yellow);
                item.shape.setOutlineThickness(4.f);
                item.text.setFillColor(sf::Color::Yellow);
                item.shape.setFillColor(sf::Color(
                    item.shape.getFillColor().r,
                    item.shape.getFillColor().g,
                    item.shape.getFillColor().b,
                    255)); // Full opacity when hovered
            }
            else {
                item.shape.setOutlineColor(sf::Color(150, 150, 150));
                item.shape.setOutlineThickness(2.f);
                item.text.setFillColor(sf::Color::White);
                item.shape.setFillColor(sf::Color(
                    item.shape.getFillColor().r,
                    item.shape.getFillColor().g,
                    item.shape.getFillColor().b,
                    200)); // Slightly transparent when not hovered
            }
        }

        Draw(window);
        window.display();
    }
    return Exit;
}

MainMenu::MenuResult MainMenu::HandleClick(int x, int y) {
    for (const auto& item : menuItems) {
        if (item.rect.contains(x, y)) {
            return item.action;
        }
    }
    return Nothing;
}

void MainMenu::Draw(sf::RenderWindow& window) {
    window.clear();
    window.draw(backgroundSprite);

    // Draw title
    sf::Text titleText;
    titleText.setFont(font);
    titleText.setString("Emberbound");
    titleText.setCharacterSize(48);
    titleText.setFillColor(sf::Color::White);
    titleText.setStyle(sf::Text::Bold);

    sf::FloatRect titleRect = titleText.getLocalBounds();
    titleText.setOrigin(titleRect.left + titleRect.width / 2.0f,
        titleRect.top + titleRect.height / 2.0f);
    titleText.setPosition(sf::Vector2f(window.getSize().x / 2.0f, 120.f));

    // Title background
    sf::RectangleShape titleBackground(sf::Vector2f(titleRect.width + 40.f, titleRect.height + 40.f));
    titleBackground.setPosition(window.getSize().x / 2 - (titleRect.width + 40.f) / 2, 80.f);
    titleBackground.setFillColor(sf::Color(0, 0, 0, 150)); // Semi-transparent black
    titleBackground.setOutlineThickness(3.f);
    titleBackground.setOutlineColor(sf::Color::White);

    window.draw(titleBackground);
    window.draw(titleText);

    // Draw buttons and their text
    for (const auto& item : menuItems) {
        window.draw(item.shape);
        window.draw(item.text);
    }
}