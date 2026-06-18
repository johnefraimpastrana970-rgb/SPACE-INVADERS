#pragma once
#include <string>

namespace Paths {
    namespace Graphics {
        const std::string Spaceship = "Graphics/spaceship.png";
        const std::string Mystery = "Graphics/mystery.png";
        const std::string Boss = "Graphics/giratina BOSS 4.gif";
        const std::string Boss2 = "Graphics/roxo mini boss.gif";
        const std::string GameBackground = "Graphics/MAIN GAME BACKGROUND.gif";
        const std::string LoadingIcon = "Graphics/cat.gif";
    }
    namespace Sounds {
        const std::string Music = "Sounds/music.ogg";
        const std::string Explosion = "Sounds/explosion.ogg";
        const std::string Select = "Sounds/select.wav";
        const std::string Laser = "Sounds/laser.ogg";
        const std::string GameOver = "Sounds/Game Over sound effect.mp3";
        // Use a safe fallback for boss music to avoid problematic filename characters
        const std::string BossMusic = "Sounds/music.ogg";
        const std::string Blip = "Sounds/select.wav";
        const std::string MewtwoActivate = "Sounds/mewtwo_activate.wav";
        const std::string MewtwoReady = "Sounds/mewtwo_ready.wav";
        const std::string Deflect = "Sounds/clink.wav";
    }
    const std::string FadeTransition = "Sounds/swoosh.ogg"; // Placeholder for fade transition sound
    namespace Fonts {
        const std::string Monogram = "Font/monogram.ttf";
    }
}