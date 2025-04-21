
#include "Particle.h"

Particle::Particle(sf::Vector2f position, sf::Color color)
    : lifetime(0.f), maxLifetime(0.5f) {
    shape.setRadius(4.f);
    shape.setFillColor(color);
    shape.setPosition(position);
    shape.setOrigin(2.f, 2.f);
}

Particle::~Particle()
{
}

void Particle::update(float deltaTime) {
    lifetime += deltaTime;

    float alpha = 255 * (1.f - (lifetime / maxLifetime));
    shape.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(alpha)));

    // shrink the particle
    float scale = 1.f - (lifetime / maxLifetime);
    shape.setScale(scale, scale);




}

void Particle::draw(sf::RenderWindow& window) {
    window.draw(shape);
}

bool Particle::isAlive() const {
    return lifetime < maxLifetime;
}
