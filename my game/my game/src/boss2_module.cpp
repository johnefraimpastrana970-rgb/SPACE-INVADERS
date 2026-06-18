#include "boss2_module.hpp"
#include "game.hpp"

namespace Boss2Module {
    void ApplyBoss2Sprite(Game &game, const std::string &assetPath) {
        game.SetBoss2Sprite(assetPath);
    }
}
