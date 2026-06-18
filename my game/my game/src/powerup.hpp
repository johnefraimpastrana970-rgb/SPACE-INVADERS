#pragma once
#include <raylib.h>
#include <cmath>
#include "particle.hpp"
#include <vector>

enum PowerUpType {
    POWERUP_RAPID = 1,
    POWERUP_DOUBLE = 2,
    POWERUP_SHIELD = 3,
    POWERUP_NUKE = 4,
    POWERUP_LIFE = 5,
    POWERUP_SHIELD_RECHARGE = 6
};

class PowerUp {
public:
    PowerUp() {}
    PowerUp(int type, Vector2 pos, bool magnetic = false): type(type), position(pos), active(true), isMagnetic(magnetic) {}
    void Update(Vector2 target, std::vector<Particle>& particles) { 
        float dt = GetFrameTime();
        if (isMagnetic) {
            Vector2 dir = { target.x - position.x, target.y - position.y };
            float dist = sqrtf(dir.x * dir.x + dir.y * dir.y);
            
            // Pull gets progressively stronger as the power-up gets closer to the ship
            float speed = 320.0f;
            if (dist < 600.0f) speed += (600.0f - dist) * 1.5f;

            if (dist > 5.0f) {
                // Particle trail effect
                if (GetRandomValue(0, 1) == 0) {
                    Particle p;
                    p.pos = position;
                    // Drift opposite to movement, scaling with speed for a more intense trail
                    float driftBase = speed * 0.25f;
                    p.vel = { -(dir.x / dist) * driftBase + GetRandomValue(-10, 10), 
                              -(dir.y / dist) * driftBase + GetRandomValue(-10, 10) };
                    Color c = (type == 4) ? RED : GREEN;
                    p.color = Fade(c, 0.6f);
                    p.life = 0.3f;
                    p.shape = Particle::RING;
                    particles.push_back(p);
                }

                position.x += (dir.x / dist) * speed * dt;
                position.y += (dir.y / dist) * speed * dt;
            }
        } else {
            position.y += 160.0f * dt; // Faster baseline falling speed (was 1.0f per frame)
        }
    }
    void Draw();
    Rectangle getRect() { return {position.x - 12, position.y - 12, 24, 24}; }
    int type = POWERUP_RAPID;
    Vector2 position = {0,0};
    bool active = true;
    bool isMagnetic = false;
};
