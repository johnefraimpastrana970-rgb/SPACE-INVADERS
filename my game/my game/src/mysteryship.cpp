#include "mysteryship.hpp"
#include <string>

static std::string ResolveAssetPath(const std::string& assetPath)
{
    std::string resolved = std::string(GetDirectoryPath(GetApplicationDirectory())) + "/" + assetPath;
    for (char &c : resolved) {
        if (c == '\\') c = '/';
    }
    return resolved;
}

MysteryShip::MysteryShip()
{
    image = LoadTexture(ResolveAssetPath("Graphics/mystery.png").c_str());
    alive = false;
}

MysteryShip::~MysteryShip() {
    UnloadTexture(image);
}

void MysteryShip::Spawn() {
    position.y = 90;
    int side = GetRandomValue(0, 1);

    if(side == 0) {
        position.x = 25;
        speed = 3;
    } else {
        position.x = GetScreenWidth() - image.width - 25;
        speed = -3;
    }
    alive = true;
}

Rectangle MysteryShip::getRect()
{
    if(alive){
        return {position.x, position.y, float(image.width), float(image.height)};
    } else {
        return {position.x, position.y, 0, 0};
    }
}

void MysteryShip::Update() {
    if(alive) {
        position.x += speed;
        if(position.x > GetScreenWidth() - image.width -25 || position.x < 25) {
            alive = false;
        }
    }
}

void MysteryShip::Draw() {
    if(alive) {
        DrawTextureV(image, position, WHITE);
    }
}