#include "alien.hpp"
#include <cmath>
#include <string>

static std::string ResolveAssetPath(const std::string& assetPath)
{
    std::string resolved = std::string(GetDirectoryPath(GetApplicationDirectory())) + "/" + assetPath;
    for (char &c : resolved) {
        if (c == '\\') c = '/';
    }
    return resolved;
}

std::array<Texture2D, 5> Alien::alienImages = {};

Alien::Alien(int type, Vector2 position, int stage)
{
    this -> type = type;
    this -> position = position;
    this -> isDiving = false;
    this -> rotation = 0.0f;

    // Initialize speeds
    horizontalSpeedMultiplier = 1.0f; // Default speed
    divingSpeed = 260.0f; // Default diving speed

    if (type == 4) { // Tank
        horizontalSpeedMultiplier = 0.7f; // Tanks are slower
        health = 4;
        if (stage == 2) {
            health = 6; // More health for Tanks in Stage 2
        }
    } else if (type == 5) { // Kamikaze
        health = 2;
        if (stage == 2) {
            horizontalSpeedMultiplier = 2.5f; // Faster in Stage 2
            divingSpeed = 300.0f; // Also dive faster in Stage 2
        } else {
            horizontalSpeedMultiplier = 1.5f; // Slightly faster than normal in other stages
        }
    } else {
        health = 1;
    }
    maxHealth = health;
    flashTimer = 0.0f;

    if(alienImages[type -1].id == 0){

    switch (type) {
        case 1:
            alienImages[0] = LoadTexture(ResolveAssetPath("Graphics/alien_1.png").c_str());
            break;
        case 2:
            alienImages[1] = LoadTexture(ResolveAssetPath("Graphics/alien_2.png").c_str());
            break;
        case 3: 
            alienImages[2] = LoadTexture(ResolveAssetPath("Graphics/alien_3.png").c_str());
            break;
        case 4:
            alienImages[3] = LoadTexture(ResolveAssetPath("Graphics/alien_2.png").c_str()); // Tank uses Type 2 look
            break;
        case 5:
            alienImages[4] = LoadTexture(ResolveAssetPath("Graphics/alien_1.png").c_str()); // Kamikaze uses Type 1 look
            break;
        default:
            alienImages[0] = LoadTexture(ResolveAssetPath("Graphics/alien_1.png").c_str());
            break;
    }
}

    if (alienImages[type - 1].id == 0) {
        TraceLog(LOG_WARNING, "Alien sprite failed to load for type %d; falling back to Graphics/alien_1.png", type);
        alienImages[type - 1] = LoadTexture(ResolveAssetPath("Graphics/alien_1.png").c_str());
    }
}

void Alien::Draw() {
    Color tint = WHITE;
    float healthPct = (float)health / maxHealth;

    // Apply hit flash tint if recently hit
    if (flashTimer > 0.0f) {
        float t = flashTimer / 0.12f; // assume flash lasts ~0.12s
        if (t > 1.0f) t = 1.0f;
        tint = ColorLerp(tint, WHITE, t);
    }

    if (type == 4) { // Tank gets darker as it takes damage
        unsigned char val = (unsigned char)(100 + 155 * healthPct);
        tint = { val, val, val, 255 };
    } else if (isDiving) {
        tint = RED; // Divers turn red when aggressive
    }

    // Use DrawTexturePro to allow rotation around the center
    Texture2D& tex = alienImages[type - 1];
    if (tex.id == 0 || tex.width <= 0 || tex.height <= 0) {
        return;
    }
    Rectangle source = {0.0f, 0.0f, (float)tex.width, (float)tex.height};
    Vector2 center = {position.x + tex.width / 2.0f, position.y + tex.height / 2.0f};
    Rectangle dest = {center.x, center.y, (float)tex.width, (float)tex.height};
    Vector2 origin = {(float)tex.width / 2.0f, (float)tex.height / 2.0f};
    
    DrawTexturePro(tex, source, dest, origin, rotation, tint);

    // Draw health bar for Tank or Diving archetypes
    if (type == 4 || isDiving) {
        float barWidth = (float)alienImages[type - 1].width;
        float barHeight = 4.0f;
        DrawRectangleV({position.x, position.y - 8.0f}, {barWidth, barHeight}, Fade(BLACK, 0.6f));
        DrawRectangleV({position.x, position.y - 8.0f}, {barWidth * healthPct, barHeight}, RED);
    }
}

int Alien::GetType() {
    return type;
}

void Alien::UnloadImages()
{
    for(int i = 0; i < 5; i++) {
        UnloadTexture(alienImages[i]);
    }
}

Rectangle Alien::getRect()
{
    Texture2D& tex = alienImages[type - 1];
    if (tex.id == 0 || tex.width <= 0 || tex.height <= 0) {
        return {position.x, position.y, 0.0f, 0.0f};
    }
    return {position.x, position.y,
    float(tex.width),
    float(tex.height)
    };
}

void Alien::Update(int direction, Vector2 target, float dt, std::vector<Particle>& particles, float speedMultiplier) {
    // Update flash timer
    if (flashTimer > 0.0f) {
        flashTimer -= dt;
        if (flashTimer < 0.0f) flashTimer = 0.0f;
    }
    if (isDiving) {
        Vector2 dir = { target.x - position.x, target.y - position.y };
        float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
        if (len > 0.1f) {
            // Use divingSpeed for diving movement
            position.x += (dir.x / len) * divingSpeed * dt;
            position.y += (dir.y / len) * divingSpeed * dt;
            
            // Calculate rotation to face the target (atan2f returns radians, convert to degrees)
            rotation = atan2f(dir.y, dir.x) * (180.0f / PI) + 90.0f; // +90 because sprites are usually drawn "up" by default

            // Spawn trail particles behind the diving alien
            if (GetRandomValue(0, 1) == 0) {
                Particle p;
                // Spawn at the center of the alien
                p.pos = { position.x + alienImages[type - 1].width / 2.0f, position.y + alienImages[type - 1].height / 2.0f };
                // Move particles slightly in the opposite direction of the dive
                p.vel = { -(dir.x / len) * 40.0f, -(dir.y / len) * 40.0f };
                p.color = RED;
                p.life = 0.3f;
                p.shape = Particle::CIRCLE;
                particles.push_back(p);
            }
        }
    } else {
        position.x += direction * horizontalSpeedMultiplier * speedMultiplier; // Apply horizontal speed multiplier and galaxy event multiplier
    }
}
