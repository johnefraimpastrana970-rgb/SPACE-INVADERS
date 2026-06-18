#pragma once
#include <string>

class Game;

// Simple helper module to apply a new sprite to boss2
namespace Boss2Module {
    // assetPath is project-relative (e.g. "Graphics/haunter.png")
    void ApplyBoss2Sprite(Game &game, const std::string &assetPath);
}
