#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <SFML/Window.hpp>
class MainMenu {
public:
    enum MenuResult { Nothing, Play, Exit };

    struct MenuItem {
        sf::Rect<float> rect;
        MenuResult action;
        sf::RectangleShape shape;
		sf::Text text;
        std::string label;
    };

    MainMenu(sf::RenderWindow& window);
    void CreateTextButton(MenuItem& item, const std::string& label, sf::Color color);
    ~MainMenu() = default;

    MenuResult Show(sf::RenderWindow& window);

private:
    sf::Texture backgroundTexture;
    sf::Sprite backgroundSprite;
    std::vector<MenuItem> menuItems;
	sf::Font font;
    void SetupMenuItems(sf::RenderWindow& window);
    void Draw(sf::RenderWindow& window);
    MenuResult HandleClick(int x, int y);
    MenuResult GetMenuResponse(sf::RenderWindow& window);

};