#include "laser.hpp"
#include <iostream>

Laser::Laser(Vector2 position, float speed, Color color, float width) // Changed speed to float
{
    this -> position = position;
    this -> speed = speed;
    this -> color = color;
    this -> width = width;
    active = true;
}

void Laser::Draw() {
    if(active) {
        // Add a subtle glow around the core laser
        DrawRectangle(position.x - 1, position.y - 1, width + 2, 17, Fade(color, 0.4f));
        DrawRectangle(position.x, position.y, width, 15, color);
    }
}

Rectangle Laser::getRect()
{
    Rectangle rect;
    rect.x = position.x;
    rect.y = position.y;
    rect.width = width;
    rect.height = 15;
    return rect;
}

void Laser::Update(std::vector<Particle>& particles) {
    position.y += speed;
    if(active) {
        // Trail effect: spawn a particle at the laser's tail
        Particle p;
        // Tail position depends on direction (speed < 0 is player shooting up)
        p.pos = { position.x + width/2.0f, (speed < 0) ? position.y + 15.0f : position.y };
        p.vel = { (float)GetRandomValue(-10, 10) * 0.1f, (float)speed * 0.1f }; 
        p.color = color;
        p.life = 0.15f;
        p.shape = Particle::CIRCLE;
        particles.push_back(p);

        if(position.y > GetScreenHeight() - 100 || position.y < 25) {
            active = false;
        }
    }
}