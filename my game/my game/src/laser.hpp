#pragma once
#include <raylib.h>
#include "particle.hpp"
#include <vector>

class Laser {
    public:
        Laser(Vector2 position, float speed, Color color, float width = 4.0f); // Changed speed to float
        void Update(std::vector<Particle>& particles);
        void Draw();
        Rectangle getRect();
        bool active;
    private:
        Vector2 position;
        int speed;
        Color color; // Changed speed to float
        float width;
};