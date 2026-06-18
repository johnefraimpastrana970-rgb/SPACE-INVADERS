#pragma once
#include <raylib.h>
#include <array>
#include <vector>
#include "particle.hpp"

class Alien {
    public:
        Alien(int type, Vector2 position, int stage); // Stage is used for initial health scaling
        void Update(int direction, Vector2 target, float dt, std::vector<Particle>& particles, float speedMultiplier = 1.0f);
        void Draw();
        int GetType();
        static void UnloadImages();
        Rectangle getRect();
        static std::array<Texture2D, 5> alienImages;
        float horizontalSpeedMultiplier;
        float divingSpeed;
        int type;
        Vector2 position;
        int health;
        int maxHealth;
        bool isDiving;
        float rotation; // New member for rotation angle
        float flashTimer = 0.0f; // brief hit flash timer
    private:
};