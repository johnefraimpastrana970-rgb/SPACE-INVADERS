#include "game.hpp"
#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include "particle.hpp"
#include "utils.hpp"
#include "assets.hpp"

// Define color constants not in raylib
constexpr Color TEAL = {0, 128, 128, 255};
constexpr Color CYAN = {0, 255, 255, 255};
constexpr int WINDOW_OFFSET = 50;

Game::Game()
{
    if (!IsAudioDeviceReady()) InitAudioDevice();
    SetMasterVolume(1.0f);

    music = LoadMusicStream(Utils::ResolveAssetPath(Paths::Sounds::Music).c_str());
    const std::string explosionPath = Utils::ResolveAssetPath(Paths::Sounds::Explosion);
    const std::string pickupPath = Utils::ResolveAssetPath(Paths::Sounds::Select);
    std::string laserPath = Utils::ResolveAssetPath(Paths::Sounds::Laser);
    const std::string fallbackLaserPath = Utils::ResolveAssetPath("Sounds/Laser Sound Effect.mp3");
    const std::string gameOverPath = Utils::ResolveAssetPath(Paths::Sounds::GameOver);
    const std::string fadeSoundPath = Utils::ResolveAssetPath(Paths::FadeTransition); // New fade sound path
    const std::string blipPath = Utils::ResolveAssetPath(Paths::Sounds::Blip);
    const std::string mewtwoActivatePath = Utils::ResolveAssetPath(Paths::Sounds::MewtwoActivate);
    const std::string mewtwoReadyPath = Utils::ResolveAssetPath(Paths::Sounds::MewtwoReady);
    const std::string deflectPath = Utils::ResolveAssetPath(Paths::Sounds::Deflect);

    explosionSound = LoadSound(explosionPath.c_str());
    pickupSound = LoadSound(pickupPath.c_str());
    bossAttackSound = LoadSound(laserPath.c_str());
    bossBeamSound = LoadSound(laserPath.c_str());
    blipSound = LoadSound(blipPath.c_str());
    fadeSound = LoadSound(fadeSoundPath.c_str()); // Load fade sound
    bossBannerSound = LoadSound(laserPath.c_str()); // Initialize boss banner sound
    mewtwoActivateSound = LoadSound(mewtwoActivatePath.c_str());
    mewtwoReadySound = LoadSound(mewtwoReadyPath.c_str());
    deflectSound = LoadSound(deflectPath.c_str());

    if (bossAttackSound.frameCount == 0 || bossBeamSound.frameCount == 0) {
        if (FileExists(fallbackLaserPath.c_str())) {
            TraceLog(LOG_WARNING, "Primary laser sound failed, falling back to: %s", fallbackLaserPath.c_str());
            UnloadSound(bossAttackSound);
            UnloadSound(bossBeamSound);
            bossAttackSound = LoadSound(fallbackLaserPath.c_str());
            bossBeamSound = LoadSound(fallbackLaserPath.c_str());
        }
    }

    GameOverMusic = LoadMusicStream(gameOverPath.c_str());

    if (explosionSound.frameCount == 0) TraceLog(LOG_WARNING, "Failed to load sound: %s", explosionPath.c_str());
    if (pickupSound.frameCount == 0) TraceLog(LOG_WARNING, "Failed to load sound: %s", pickupPath.c_str());
    if (bossAttackSound.frameCount == 0) TraceLog(LOG_WARNING, "Failed to load sound: %s", laserPath.c_str());
    if (bossBeamSound.frameCount == 0) TraceLog(LOG_WARNING, "Failed to load sound: %s", laserPath.c_str());
    if (GameOverMusic.frameCount == 0) TraceLog(LOG_WARNING, "Failed to load music: %s", gameOverPath.c_str());
    if (blipSound.frameCount == 0) TraceLog(LOG_WARNING, "Failed to load sound: %s", blipPath.c_str());
    if (music.frameCount == 0) TraceLog(LOG_WARNING, "Failed to load music: %s", Utils::ResolveAssetPath(Paths::Sounds::Music).c_str());
    if (fadeSound.frameCount == 0) TraceLog(LOG_WARNING, "Failed to load fade sound: %s", fadeSoundPath.c_str());
    if (mewtwoActivateSound.frameCount == 0) TraceLog(LOG_WARNING, "Failed to load sound: %s", mewtwoActivatePath.c_str());
    if (mewtwoReadySound.frameCount == 0) TraceLog(LOG_WARNING, "Failed to load sound: %s", mewtwoReadyPath.c_str());
    if (deflectSound.frameCount == 0) TraceLog(LOG_WARNING, "Failed to load sound: %s", deflectPath.c_str());

    SetSoundVolume(blipSound, 0.4f); // Keep the typewriter sound subtle
    PlayMusicStream(music);
    InitGame();
}

Game::~Game() {
    Alien::UnloadImages();
    UnloadMusicStream(music);
    UnloadSound(explosionSound);
    UnloadSound(pickupSound);
    UnloadSound(bossAttackSound);
    UnloadSound(bossBeamSound);
    UnloadSound(blipSound);
    UnloadSound(fadeSound); // Unload fade sound
    UnloadSound(bossBannerSound);
    UnloadSound(mewtwoActivateSound);
    UnloadSound(mewtwoReadySound);
    UnloadSound(deflectSound);
    UnloadMusicStream(GameOverMusic);
}

Vector2 Game::GetScreenShakeOffset() const {
    if (screenShakeTime <= 0.0f || screenShakeDuration <= 0.0f) {
        return {0.0f, 0.0f};
    }
    // Smoother oscillatory shake using sin/cos with decaying amplitude
    float progress = screenShakeTime / screenShakeDuration; // 1.0 -> 0.0 over time
    float decay = progress * progress; // ease-out
    float strength = screenShakeStrength * decay;
    if (strength <= 0.01f) return {0.0f, 0.0f};

    float t = (float)GetTime();
    // Combine two sine waves for a richer feel
    float x = sinf(t * 40.0f) * (strength * 0.6f) + cosf(t * 67.0f) * (strength * 0.4f);
    float y = cosf(t * 34.0f) * (strength * 0.6f) + sinf(t * 53.0f) * (strength * 0.4f);

    // Small random jitter to avoid perfect periodicity
    x += (float)GetRandomValue(-1, 1) * (strength * 0.08f);
    y += (float)GetRandomValue(-1, 1) * (strength * 0.08f);

    float maxOffset = ceilf(strength);
    x = fmaxf(-maxOffset, fminf(maxOffset, x));
    y = fmaxf(-maxOffset, fminf(maxOffset, y));
    return { x, y };
}

    void Game::TriggerScreenShake(float duration, float strength) {
        screenShakeTime = duration;
        screenShakeDuration = duration;
        screenShakeStrength = strength;
}

void Game::Update() {
    // Update music streams based on state
    // Make frame delta available across the whole update function
    float dt = GetFrameTime();

    if (run) {
        dt = GetFrameTime(); // Get frame time once at the beginning of update
        float fadeSpeed = bossNarrativeActive ? 1.5f : 0.5f; // Faster fade during boss arise scene

        if (slowMoTimer > 0.0f) {
            slowMoTimer -= dt;
            dt *= 0.3f; // Slow down game logic during boss death
        }

        bool isBossActive = IsBossActive();

        // Calculate target volumes
        if (isBossActive) {
            musicVolume -= fadeSpeed * dt;
        } else {
            musicVolume += fadeSpeed * dt;
        }

        if (bossBannerActive) bossBannerTimer += dt;

        // Clamp volumes between 0 and 1
        musicVolume = fmaxf(0.0f, fminf(1.0f, musicVolume));

        SetMusicVolume(music, musicVolume);

        // Manage stream playback and updates
        if (musicVolume > 0.0f) {
            if (!IsMusicStreamPlaying(music)) PlayMusicStream(music);
            SetMusicPitch(music, (slowMoTimer > 0.0f) ? 0.8f : 1.0f);
            UpdateMusicStream(music);
        } else if (IsMusicStreamPlaying(music)) StopMusicStream(music);

        // Play 'Ready' sound effects when cooldowns expire
        if (selectedCharacterIndex == 3 && !mewtwoReadySoundPlayed && spaceship.GetMewtwoAbilityCooldownRemaining() <= 0) {
            PlaySound(mewtwoReadySound);
            mewtwoReadySoundPlayed = true;
        }
        if (selectedCharacterIndex == 4 && !greenNinjaReadySoundPlayed && spaceship.GetGreenNinjaAbilityCooldownRemaining() <= 0) {
            PlaySound(mewtwoReadySound); // Reusing the notification blip
            greenNinjaReadySoundPlayed = true;
        }
        if (selectedCharacterIndex == 5 && !novaReadySoundPlayed && spaceship.GetNovaAbilityCooldownRemaining() <= 0) {
            PlaySound(mewtwoReadySound);
            novaReadySoundPlayed = true;
        }
        
        if (fadeToBlackActive) {
            fadeToBlackTimer += dt;
            if (fadeToBlackTimer >= fadeToBlackDuration) {
                fadeToBlackActive = false;
                bossNarrativeActive = true;
                bossNarrativeCharCount = 0.0f;
                skipNarrativeTriggered = false;
                StopSound(fadeSound); // Stop fade sound when fade to black ends
            }
            return; // Skip all other game updates during fade to black
        }

        // Spawn aliens for the new stage if a boss was just defeated and the banner is gone
        if (!bossBannerActive && !aliensSpawnedForCurrentStage && currentStage <= 5 && !bossNarrativeActive) {
            SpawnAliensForCurrentStage();
        }

        if (bossNarrativeActive) {
            int charBefore = (int)bossNarrativeCharCount;
            // Typewriter speed: 30 characters per second
            if (skipNarrativeTriggered) {
                bossNarrativeCharCount = (float)bossNarrativeText.length();
            } else {
            bossNarrativeCharCount += dt * 30.0f;
            }
            int charAfter = (int)bossNarrativeCharCount;

            // Trigger blip when a new character is revealed
            if (charAfter > charBefore) {
                size_t totalChars = bossNarrativeText.length();
                if (charAfter <= (int)totalChars) { 
                    PlaySound(blipSound);
                }
            }

            // Update particles and visual effects so the screen isn't frozen
            float dt_p = GetFrameTime();
            for(auto pit = particles.begin(); pit != particles.end();){
                pit->Update(dt_p);
                if(pit->life <= 0) pit = particles.erase(pit);
                else ++pit;
            }

            return; // Pause all other game logic (aliens, shooting, mystery ship) while narrative is active
        }
    } else if (gameOverActive) {
        UpdateMusicStream(GameOverMusic);
        // Ensure particles continue to move and fall during Game Over
        float dt_p = GetFrameTime();
        for(auto pit = particles.begin(); pit != particles.end();){
            pit->Update(dt_p);
            if(pit->life <= 0) pit = particles.erase(pit);
            else ++pit;
        }
    }

    if(!run) {
        return;
    }

    double currentTime = GetTime();
    if(currentTime - timeLastSpawn > mysteryShipSpawnInterval) {
            mysteryship.Spawn();
            timeLastSpawn = GetTime();
            mysteryShipSpawnInterval = GetRandomValue(10, 20);
        }

        // Update Background Stars (Scrolling effect)
        for (auto& star : stars) {
            star.pos.y += star.speed * dt * 60.0f;
            if (star.pos.y > GetScreenHeight()) {
                star.pos.y = -10;
                star.pos.x = (float)GetRandomValue(0, GetScreenWidth());
            }
        }

        // Persistent Damage Effect: Spawn smoke if lives are low
        if (lives <= 2) { // Only emit particles if 2 or 1 life remaining
            int spawnChance = (lives == 1) ? 4 : 8; // More frequent sparks at 1 life
            if (GetRandomValue(0, spawnChance) == 0) {
                Rectangle rect = spaceship.getRect();
                Particle p;
                // Spawn near the back/center of the ship
                p.pos = { rect.x + rect.width / 2.0f + GetRandomValue(-10, 10), rect.y + rect.height / 2.0f };
                
                if (lives == 1) {
                    // Sparks effect for 1 life
                    p.vel = { (float)GetRandomValue(-40, 40), (float)GetRandomValue(-180, -100) }; // More energetic upward/outward
                    p.color = ORANGE; // Bright orange sparks
                    p.life = (float)GetRandomValue(3, 8) / 10.0f; // Shorter life for sparks
                    p.shape = Particle::TRIANGLE; // Triangular spark shape
                } else {
                    // Smoke effect for 2 lives
                    p.vel = { (float)GetRandomValue(-20, 20), (float)GetRandomValue(-120, -80) }; // Rise up
                    p.color = GRAY; // Gray smoke
                    p.life = (float)GetRandomValue(4, 12) / 10.0f; // Longer life for smoke
                    p.shape = Particle::CIRCLE; // Circular smoke shape
                }
                
                particles.push_back(p);
            }
        }

        for(auto& laser: spaceship.lasers) {
            laser.Update(particles);
        }

        Rectangle shipRect = spaceship.getRect();
        Vector2 shipCenter = { shipRect.x + shipRect.width / 2.0f, shipRect.y + shipRect.height / 2.0f };

        // Nova Blast Aura: Swirling golden vortex centered on the player while active
        if (spaceship.IsNovaAbilityActive()) {
            for (int i = 0; i < 3; i++) {
                Particle p;
                float ang = (float)GetRandomValue(0, 360) * (PI / 180.0f);
                float dist = (float)GetRandomValue(30, 90);
                p.pos = { shipCenter.x + cosf(ang) * dist, shipCenter.y + sinf(ang) * dist };
                
                // Velocity: Pull inward and swirl around the ship
                p.vel = { -cosf(ang) * 180.0f, -sinf(ang) * 180.0f }; 
                p.vel.x += -sinf(ang) * 120.0f; 
                p.vel.y += cosf(ang) * 120.0f;

                p.color = (i % 2 == 0) ? GOLD : WHITE;
                p.life = 0.35f;
                p.shape = (GetRandomValue(0, 1) == 0) ? Particle::RING : Particle::TRIANGLE;
                particles.push_back(p);
            }
        }

        boss.Update(shipCenter, particles);
        if(boss.BeamJustFired()) {
            PlaySound(bossBeamSound);
            boss.ClearBeamJustFired();
        }
        if(boss.IsDivineJudgmentActive()) {
            // Shake gets more violent as the barrage reaches its end
            float progress = boss.GetDivineJudgmentProgress();
            TriggerScreenShake(0.05f, 14.0f + (progress * 22.0f)); // Ramps from 14 to 36 intensity
        }
        if(boss.DivineJudgmentJustFired()) {
            boss.ClearDivineJudgmentJustFired();
        }
        if (boss.GetMeteorImpacted()) {
            screenFlashTime = 0.15f;
            screenFlashColor = ORANGE;
            boss.ClearMeteorImpacted();
        }
        if (boss.GetPhaseShiftJustTriggered()) {
            TriggerScreenShake(1.5f, 25.0f); // Violent shake for Arceus phase transition
            boss.ClearPhaseShiftJustTriggered();
        }
        if (boss.DivineFireJustImpacted()) {
            TriggerScreenShake(0.12f, 8.0f); // Impact shake for Divine Fire bloom
            boss.ClearDivineFireJustImpacted();
        }

        // Update second boss if present
        boss2.Update(shipCenter, particles);
        for (auto& g : guards) {
            g->Update(shipCenter, particles);
            if (g->DivineFireJustImpacted()) {
                TriggerScreenShake(0.12f, 6.0f); // Slightly lighter shake for guard fire
                g->ClearDivineFireJustImpacted();
            }
        }

        if(boss2.BeamJustFired()) {
            PlaySound(bossBeamSound);
            boss2.ClearBeamJustFired();
        }
        if(boss2.IsDivineJudgmentActive()) {
            float progress = boss2.GetDivineJudgmentProgress();
            TriggerScreenShake(0.05f, 14.0f + (progress * 22.0f));
        }
        if(boss2.DivineJudgmentJustFired()) {
            boss2.ClearDivineJudgmentJustFired();
        }
        if (boss2.GetMeteorImpacted()) {
            screenFlashTime = 0.15f;
            screenFlashColor = ORANGE;
            boss2.ClearMeteorImpacted();
        }
        if (boss2.GetPhaseShiftJustTriggered()) {
            TriggerScreenShake(1.5f, 25.0f);
            boss2.ClearPhaseShiftJustTriggered();
        }
        if (boss2.DivineFireJustImpacted()) {
            TriggerScreenShake(0.12f, 8.0f);
            boss2.ClearDivineFireJustImpacted();
        }

        MoveAliens();

        AlienShootLaser();

        for(auto& laser: alienLasers) {
            laser.Update(particles);
        }

        for(auto it = powerups.begin(); it != powerups.end();){
            it->Update(shipCenter, particles);
            if(!it->active || it->position.y > GetScreenHeight()) {
                it = powerups.erase(it);
            } else {
                ++it;
            }
        }

        DeleteInactiveLasers();
        
        mysteryship.Update();

        CheckForCollisions();

        // Persistent particle update: Ensure particles move even if game logic pauses/ends
        float dt_particles = GetFrameTime();
        for(auto pit = particles.begin(); pit != particles.end();){
            pit->Update(dt_particles);
            if(pit->life <= 0) pit = particles.erase(pit);
            else ++pit;
        }

        dt = GetFrameTime();
        if (screenShakeTime > 0.0f) {
            screenShakeTime -= dt;
            if (screenShakeTime < 0.0f) screenShakeTime = 0.0f;
        }

        if (screenFlashTime > 0.0f) {
            screenFlashTime -= dt;
            if (screenFlashTime < 0.0f) screenFlashTime = 0.0f;
        }

        // Intensify distortion based on boss charge progress
        float maxChargeProgress = 0.0f;
        Vector2 targetPx = { (float)GetScreenWidth() / 2.0f, (float)GetScreenHeight() / 2.0f };

        auto updateMaxProgress = [&](const Boss& b) {
            float p = b.GetChargeProgress();
            if (p > maxChargeProgress) {
                maxChargeProgress = p;
                Rectangle r = b.getRect();
                targetPx = { r.x + r.width / 2.0f, r.y + r.height / 2.0f };
            }
        };

        if (boss.alive) updateMaxProgress(boss);
        if (boss2.alive) updateMaxProgress(boss2);
        for (const auto& g : guards) {
            if (g->alive) updateMaxProgress(*g);
        }

        // Calculate normalized center (0.0 to 1.0) for the shader
        distortionCenter = { 
            targetPx.x / (float)GetScreenWidth(), 
            targetPx.y / (float)GetScreenHeight() 
        };

        if (maxChargeProgress > 0.0f) {
            // Scale factor for visual intensity as the charge completes
            distortionLevel = maxChargeProgress * 1.5f; 
            chromaticAmount = fmaxf(chromaticAmount, maxChargeProgress * 0.005f);
        } else {
            // Decay distortion effect if no boss is charging
            if (distortionLevel > 0.0f) {
                distortionLevel -= dt * 4.0f;
                if (distortionLevel < 0.0f) distortionLevel = 0.0f;
            }
        }

        // Decay chromatic aberration
        if (chromaticAmount > 0.0f) {
            chromaticAmount -= dt * 0.03f; // Sharp decay for a punchy effect
            if (chromaticAmount < 0.0f) chromaticAmount = 0.0f;
        }

        for (auto it = nukeExplosions.begin(); it != nukeExplosions.end();) {
            it->elapsed += dt;
            if (it->elapsed >= it->duration) {
                it = nukeExplosions.erase(it);
            } else {
                ++it;
            }
        }
}

void Game::Draw(Font uiFont) {
    // Draw Background Visual Design (Starfield)
    for (const auto& star : stars) {
        DrawCircleV(star.pos, star.size, star.color);
    }

    spaceship.Draw(lives);

    for(auto& laser: spaceship.lasers) {
        laser.Draw();
    }
    for (auto& g : guards) g->Draw();

    for(auto& obstacle: obstacles) {
        obstacle.Draw();
    }

    for(auto& alien: aliens) {
        alien.Draw();
    }

    for (auto& exp : nukeExplosions) {
        float alpha = 1.0f - (exp.elapsed / exp.duration);
        int idx = exp.type - 1;
        if (idx < 0) idx = 0;
        if (idx >= (int)Alien::alienImages.size()) idx = (int)Alien::alienImages.size() - 1;
        Texture2D& tex = Alien::alienImages[idx];
        if (tex.id == 0 || tex.width <= 0 || tex.height <= 0) {
            TraceLog(LOG_WARNING, "Skipping nuke explosion draw due to invalid texture at idx %d (type %d)", idx, exp.type);
            continue;
        }
        Vector2 center = { exp.pos.x + tex.width * 0.5f, exp.pos.y + tex.height * 0.5f };
        DrawTextureV(tex, exp.pos, Fade(WHITE, alpha));
        DrawCircleLines(center.x, center.y, 20.0f + 30.0f * (1.0f - alpha), Fade(GOLD, alpha));
    }

    for(auto& powerup: powerups) {
        powerup.Draw();
    }

    // draw particles
    for(auto& p: particles) p.Draw();

    boss.Draw();
    boss2.Draw();

    // Draw screen flash if active (psychic purple)
    if (screenFlashTime > 0.0f) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(screenFlashColor, screenFlashTime * 1.5f));
    }

    // Divine Judgment screen tint (Golden atmosphere)
    bool judgmentActive = (boss.alive && boss.IsDivineJudgmentActive()) || (boss2.alive && boss2.IsDivineJudgmentActive());
    if (judgmentActive) {
        float pulse = (sinf((float)GetTime() * 4.0f) + 1.0f) * 0.5f; // Slower, regal pulse for hype
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(GOLD, 0.12f + 0.08f * pulse));
    }

    // Arceus specific: Violent red pulsing during screen shake
    if (currentStage == 5 && screenShakeTime > 0.0f && screenShakeDuration > 0.0f) {
        float pulseAlpha = (sinf((float)GetTime() * 20.0f) + 1.0f) * 0.5f; // Fast intense pulse
        float intensity = (screenShakeTime / screenShakeDuration);        // Fading intensity with the shake
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(RED, pulseAlpha * 0.4f * intensity));
    }

    // Draw fade to black overlay
    if (fadeToBlackActive) {
        float alpha = fadeToBlackTimer / fadeToBlackDuration;
        if (alpha > 1.0f) alpha = 1.0f;
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, alpha));
        return; // Don't draw anything else during fade to black
    }

    // Draw boss narrative scene overlay
    if (bossNarrativeActive) {
        float overlayPulse = (sinf(GetTime() * 4.0f) + 1.0f) * 0.5f;
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.75f));
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(RED, 0.15f * overlayPulse));
        
        Vector2 nSize = MeasureTextEx(uiFont, bossNarrativeText.c_str(), 40, 2);

        // Calculate substring for typewriter effect
        std::string visibleText = bossNarrativeText.substr(0, (size_t)bossNarrativeCharCount);
        DrawTextEx(uiFont, visibleText.c_str(), {(float)GetScreenWidth() * 0.5f - nSize.x * 0.5f, (float)GetScreenHeight() * 0.5f - 40}, 40, 2, RED);
        
        // Only show interaction prompt once text is fully typed
        if (bossNarrativeCharCount >= (float)bossNarrativeText.length()) {
            std::string prompt = "PRESS SPACE OR CLICK TO FACE YOUR DESTINY";
            Vector2 pSize = MeasureTextEx(uiFont, prompt.c_str(), 24, 2);
            float promptAlpha = 0.6f + 0.4f * sinf((float)GetTime() * 5.0f);
            DrawTextEx(uiFont, prompt.c_str(), {(float)GetScreenWidth() * 0.5f - pSize.x * 0.5f, (float)GetScreenHeight() * 0.5f + 40}, 24, 2, Fade(WHITE, promptAlpha));
        }

        // Draw SKIP button if narrative is not fully typed yet
        if (bossNarrativeCharCount < (float)bossNarrativeText.length()) {
            std::string skipText = "SKIP [ESC/SPACE]";
            Vector2 skipTextSize = MeasureTextEx(uiFont, skipText.c_str(), 20, 2);
            Rectangle skipButton = {
                (float)GetScreenWidth() - skipTextSize.x - 30,
                (float)GetScreenHeight() - skipTextSize.y - 20,
                skipTextSize.x + 20,
                skipTextSize.y + 10
            };
            bool hovered = CheckCollisionPointRec(GetMousePosition(), skipButton);
            DrawRectangleRec(skipButton, Fade(BLACK, hovered ? 0.7f : 0.5f));
            DrawRectangleLinesEx(skipButton, 2, hovered ? WHITE : GRAY);
            DrawTextEx(uiFont, skipText.c_str(),
                       {skipButton.x + 10, skipButton.y + 5},
                       20, 2, WHITE);
        }
    }

    for(auto& laser: alienLasers) {
        laser.Draw();
    }

    mysteryship.Draw();

    // Draw boss battle banner
    if (bossBannerActive) {
        float alpha = 0.0f;
        float currentBannerTime = bossBannerTimer;
        float totalBannerDuration = bossBannerFadeDuration * 2 + bossBannerDuration;

        if (currentBannerTime < bossBannerFadeDuration) {
            alpha = currentBannerTime / bossBannerFadeDuration; // Fade in
        } else if (currentBannerTime < bossBannerFadeDuration + bossBannerDuration) {
            alpha = 1.0f; // Hold opaque
        } else if (currentBannerTime < totalBannerDuration) {
            alpha = 1.0f - ((currentBannerTime - (bossBannerFadeDuration + bossBannerDuration)) / bossBannerFadeDuration); // Fade out
        } else {
            bossBannerActive = false; // Banner finished its cycle
        }

        // Clamp alpha to ensure it's within [0, 1]
        alpha = fmaxf(0.0f, fminf(1.0f, alpha));

        std::string bannerText = "BOSS BATTLE";
        Vector2 textSize = MeasureTextEx(uiFont, bannerText.c_str(), 72, 2);
        DrawTextEx(uiFont, bannerText.c_str(), {(GetScreenWidth() - textSize.x) * 0.5f, (GetScreenHeight() - textSize.y) * 0.5f - 50}, 72, 2, Fade(bossBannerColor, alpha));
    }

    // Draw stage message
    if (!stageMessageShown && GetTime() - stageStartTime < 3.0) {
        float fadeAlpha = 1.0f;
        if (GetTime() - stageStartTime > 2.0) {
            fadeAlpha = 1.0f - (GetTime() - stageStartTime - 2.0f);
        }
        std::string stageText = TextFormat("ENTERING NEW STAGE: STAGE %d", currentStage);
        int textWidth = MeasureText(stageText.c_str(), 48);
        DrawText(stageText.c_str(), GetScreenWidth()/2 - textWidth/2, GetScreenHeight()/2 - 24, 48, Fade(YELLOW, fadeAlpha));
    } else if (GetTime() - stageStartTime >= 3.0 && !stageMessageShown) {
        stageMessageShown = true;
    }

    // Draw pickup label
    if(GetTime() <= pickupLabelUntil && !pickupLabel.empty()){
        DrawText(pickupLabel.c_str(), 20, 20, 20, YELLOW);
    }

    // Draw active ability timers
    int sx = 20;
    int sy = 90;
    if(spaceship.RapidActive()){
        int t = int(ceil(spaceship.RapidRemaining()));
        DrawText(TextFormat("Rapid: %ds", t), sx, sy, 18, ORANGE);
        sy += 28;
    }
    if(spaceship.DoubleScoreActive()){
        int t = int(ceil(spaceship.DoubleRemaining()));
        DrawText(TextFormat("Double: %ds", t), sx, sy, 18, GOLD);
        sy += 28;
    }
    if(spaceship.HasShield()){
        int t = (int)ceil(spaceship.ShieldRemaining());
        DrawText(TextFormat("Shield: %ds", t), sx, sy, 18, SKYBLUE);
        sy += 32;
    }

    // Draw Warp Dash stamina bar
    Color barColor = {243, 216, 63, 255}; // Using the 'yellow' theme color
    DrawText("DASH", sx, sy, 18, SKYBLUE);
    float dashProgress = spaceship.GetDashProgress();
    DrawRectangle(sx + 65, sy + 2, 100, 14, Fade(BLACK, 0.6f));
    Color barFillColor = BLUE;
    if (dashProgress >= 1.0f) {
        // Rapidly toggle between SKYBLUE and WHITE when the dash is ready
        barFillColor = ((int)(GetTime() * 10) % 2 == 0) ? SKYBLUE : WHITE;
    }
    DrawRectangle(sx + 65, sy + 2, (int)(100 * dashProgress), 14, barFillColor);
    DrawRectangleLines(sx + 65, sy + 2, 100, 14, barColor);
    sy += 22;

    // Draw Mewtwo ability cooldown/status (Fixed nested structure)
    if (selectedCharacterIndex == 3) {
        double cooldownRemaining = spaceship.GetMewtwoAbilityCooldownRemaining();
        double activeRemaining = spaceship.GetMewtwoAbilityActiveRemaining();

        std::string abilityDisplayString;
        Color abilityColor;
        float progress = 0.0f; // 0.0 to 1.0 for the bar fill

        if (activeRemaining > 0) {
            abilityDisplayString = TextFormat("Mewtwo (E): ACTIVE! %ds", (int)ceil(activeRemaining));
            abilityColor = PURPLE;
            progress = activeRemaining / Spaceship::MEWTWO_ABILITY_DURATION;
        } else if (cooldownRemaining > 0) {
            abilityDisplayString = TextFormat("Mewtwo (E): CD %ds", (int)ceil(cooldownRemaining));
            abilityColor = GRAY;
            progress = 1.0f - (cooldownRemaining / Spaceship::MEWTWO_ABILITY_COOLDOWN_TIME);
        } else {
            abilityDisplayString = "Mewtwo (E): READY!";
            abilityColor = SKYBLUE;
            progress = 1.0f;
        }

        DrawTextEx(uiFont, abilityDisplayString.c_str(), { (float)sx, (float)sy }, 18, 2, abilityColor);

        float barWidth = 60.0f; // Smaller bar
        float textWidth = MeasureTextEx(uiFont, abilityDisplayString.c_str(), 18, 2).x;
        float barX = sx + textWidth + 5; // 5 pixels spacing
        DrawRectangle(barX, sy + 2, barWidth, 14, Fade(BLACK, 0.6f));
        DrawRectangle(barX, sy + 2, (int)(barWidth * progress), 14, abilityColor);
        DrawRectangleLines(barX, sy + 2, barWidth, 14, abilityColor);
        sy += 28; // Increment sy for next HUD element
    } else if (selectedCharacterIndex == 4) { // Green Ninja HUD
        double activeRemaining = spaceship.GetGreenNinjaAbilityActiveRemaining();
        double cooldownRemaining = spaceship.GetGreenNinjaAbilityCooldownRemaining();

        std::string abilityDisplayString;
        Color abilityColor;
        float progress = 0.0f;

        if (activeRemaining > 0) {
            abilityDisplayString = TextFormat("Hurricane (X): ACTIVE! %ds", (int)ceil(activeRemaining));
            abilityColor = LIME;
            progress = activeRemaining / Spaceship::GREENNINJA_ABILITY_DURATION;
        } else if (cooldownRemaining > 0) {
            abilityDisplayString = TextFormat("Hurricane (X): CD %ds", (int)ceil(cooldownRemaining));
            abilityColor = GRAY;
            progress = 1.0f - (cooldownRemaining / Spaceship::GREENNINJA_ABILITY_COOLDOWN_TIME);
        } else {
            abilityDisplayString = "Hurricane (X): READY!";
            abilityColor = GREEN;
            progress = 1.0f;
        }

        DrawTextEx(uiFont, abilityDisplayString.c_str(), { (float)sx, (float)sy }, 18, 2, abilityColor);
        float textWidth = MeasureTextEx(uiFont, abilityDisplayString.c_str(), 18, 2).x;
        float barX = sx + textWidth + 5;
        DrawRectangle(barX, sy + 2, 60, 14, Fade(BLACK, 0.6f));
        DrawRectangle(barX, sy + 2, (int)(60 * progress), 14, abilityColor);
        DrawRectangleLines(barX, sy + 2, 60, 14, abilityColor);
        sy += 28;
    } else if (selectedCharacterIndex == 1) { // Palkia HUD
        double activeRemaining = spaceship.GetPalkiaAbilityActiveRemaining();
        double cooldownRemaining = spaceship.GetPalkiaAbilityCooldownRemaining();
        std::string abilityDisplayString;
        Color abilityColor;
        float progress = 0.0f;

        if (activeRemaining > 0) {
            abilityDisplayString = TextFormat("Spacial (X): ACTIVE! %ds", (int)ceil(activeRemaining));
            abilityColor = PINK;
            progress = activeRemaining / Spaceship::PALKIA_ABILITY_DURATION;
        } else if (cooldownRemaining > 0) {
            abilityDisplayString = TextFormat("Spacial (X): CD %ds", (int)ceil(cooldownRemaining));
            abilityColor = GRAY;
            progress = 1.0f - (cooldownRemaining / Spaceship::PALKIA_ABILITY_COOLDOWN_TIME);
        } else {
            abilityDisplayString = "Spacial (X): READY!";
            abilityColor = WHITE;
            progress = 1.0f;
        }

        DrawTextEx(uiFont, abilityDisplayString.c_str(), { (float)sx, (float)sy }, 18, 2, abilityColor);
        float textWidth = MeasureTextEx(uiFont, abilityDisplayString.c_str(), 18, 2).x;
        float barX = sx + textWidth + 5;
        DrawRectangle(barX, sy + 2, 60, 14, Fade(BLACK, 0.6f));
        DrawRectangle(barX, sy + 2, (int)(60 * progress), 14, abilityColor);
        DrawRectangleLines(barX, sy + 2, 60, 14, abilityColor);
        sy += 28;
    } else if (selectedCharacterIndex == 2) { // Dialga HUD
        double activeRemaining = spaceship.GetDialgaAbilityActiveRemaining();
        double cooldownRemaining = spaceship.GetDialgaAbilityCooldownRemaining();
        std::string abilityDisplayString;
        Color abilityColor;
        float progress = 0.0f;

        if (activeRemaining > 0) {
            abilityDisplayString = TextFormat("Time (X): ACTIVE! %ds", (int)ceil(activeRemaining));
            abilityColor = SKYBLUE;
            progress = activeRemaining / Spaceship::DIALGA_ABILITY_DURATION;
        } else if (cooldownRemaining > 0) {
            abilityDisplayString = TextFormat("Time (X): CD %ds", (int)ceil(cooldownRemaining));
            abilityColor = GRAY;
            progress = 1.0f - (cooldownRemaining / Spaceship::DIALGA_ABILITY_COOLDOWN_TIME);
        } else {
            abilityDisplayString = "Time (X): READY!";
            abilityColor = YELLOW;
            progress = 1.0f;
        }

        DrawTextEx(uiFont, abilityDisplayString.c_str(), { (float)sx, (float)sy }, 18, 2, abilityColor);
        float textWidth = MeasureTextEx(uiFont, abilityDisplayString.c_str(), 18, 2).x;
        float barX = sx + textWidth + 5;
        DrawRectangle(barX, sy + 2, 60, 14, Fade(BLACK, 0.6f));
        DrawRectangle(barX, sy + 2, (int)(60 * progress), 14, abilityColor);
        DrawRectangleLines(barX, sy + 2, 60, 14, abilityColor);
        sy += 28;
    } else if (selectedCharacterIndex == 5) { // Nova Blast HUD
        double activeRemaining = spaceship.GetNovaAbilityActiveRemaining();
        double cooldownRemaining = spaceship.GetNovaAbilityCooldownRemaining();
        std::string abilityDisplayString;
        Color abilityColor;
        float progress = 0.0f;

        if (activeRemaining > 0) {
            abilityDisplayString = TextFormat("Nova Blast (X): ACTIVE! %ds", (int)ceil(activeRemaining));
            abilityColor = GOLD;
            progress = activeRemaining / Spaceship::NOVA_ABILITY_DURATION;
        } else if (cooldownRemaining > 0) {
            abilityDisplayString = TextFormat("Nova Blast (X): CD %ds", (int)ceil(cooldownRemaining));
            abilityColor = GRAY;
            progress = 1.0f - (cooldownRemaining / Spaceship::NOVA_ABILITY_COOLDOWN_TIME);
        } else {
            abilityDisplayString = "Nova Blast (X): READY!";
            abilityColor = ORANGE;
            progress = 1.0f;
        }

        DrawTextEx(uiFont, abilityDisplayString.c_str(), { (float)sx, (float)sy }, 18, 2, abilityColor);
        float textWidth = MeasureTextEx(uiFont, abilityDisplayString.c_str(), 18, 2).x;
        float barX = sx + textWidth + 5;
        DrawRectangle(barX, sy + 2, 60, 14, Fade(BLACK, 0.6f));
        DrawRectangle(barX, sy + 2, (int)(60 * progress), 14, abilityColor);
        DrawRectangleLines(barX, sy + 2, 60, 14, abilityColor);
        sy += 28;
    }
}

void Game::SpawnConfetti() {
    int confettiCount = 150;
    Color colors[] = { RED, GREEN, BLUE, YELLOW, MAGENTA, CYAN, GOLD, PINK, LIME, ORANGE };
    for (int i = 0; i < confettiCount; i++) {
        Particle p;
        // Spawn randomly across the top of the screen
        p.pos = { (float)GetRandomValue(-50, GetScreenWidth() + 50), (float)GetRandomValue(-100, 0) };
        // Horizontal drift and varied falling speeds
        p.vel = { (float)GetRandomValue(-150, 150), (float)GetRandomValue(100, 300) };
        p.color = colors[GetRandomValue(0, 9)];
        p.life = (float)GetRandomValue(35, 65) / 10.0f; // 3.5 to 6.5 seconds
        p.shape = (GetRandomValue(0, 1) == 0) ? Particle::RECT : Particle::TRIANGLE;
        particles.push_back(p);
    }
}

void Game::DrawBuffIcons(float startX, float y, Font font, bool drawTooltips) {
    float currentX = startX + 10.0f;
    float iconRadius = 12.0f;
    float spacing = 35.0f;
    float tooltipOffsetY = iconRadius * 2 + 5.0f; // Position tooltip below icon

    auto drawHUDIcon = [&](Color color, const char* label, const char* tooltipText, bool isDebuff = false) {
        Vector2 pos = {currentX + iconRadius, y + 14.0f};
        DrawCircleV(pos, iconRadius, color);
        DrawCircleLines(pos.x, pos.y, iconRadius, isDebuff ? RED : WHITE);
        
        Vector2 textSize = MeasureTextEx(font, label, 18, 1);
        DrawTextEx(font, label, {pos.x - textSize.x/2.0f, pos.y - textSize.y/2.0f}, 18, 1, WHITE);
        
        // Only draw tooltips if paused AND the mouse is hovering over the specific icon
        bool isHovered = drawTooltips && CheckCollisionPointCircle(GetMousePosition(), pos, iconRadius);
        if (isHovered && tooltipText != nullptr) {
            Vector2 tooltipSize = MeasureTextEx(font, tooltipText, 16, 1);
            
            // Keep tooltip within screen bounds horizontally
            float tx = pos.x - tooltipSize.x / 2.0f;
            if (tx < 10) tx = 10;
            if (tx + tooltipSize.x > GetScreenWidth() - 10) tx = GetScreenWidth() - tooltipSize.x - 10;

            Rectangle tooltipBg = {tx - 5, pos.y + tooltipOffsetY - 5, tooltipSize.x + 10, tooltipSize.y + 10};
            DrawRectangleRec(tooltipBg, Fade(BLACK, 0.7f));
            DrawRectangleLinesEx(tooltipBg, 1, WHITE);
            DrawTextEx(font, tooltipText, {tx, pos.y + tooltipOffsetY}, 16, 1, WHITE);
        }
        currentX += spacing;
    };

    // Power-up Icons
    if (spaceship.RapidActive()) drawHUDIcon(ORANGE, "R", "Rapid Fire");
    if (spaceship.DoubleScoreActive()) drawHUDIcon(GOLD, "D", "Double Score");
    if (spaceship.HasShield()) drawHUDIcon(spaceship.IsShieldOvercharged() ? BLUE : SKYBLUE, "S", spaceship.IsShieldOvercharged() ? "Shield Overcharged" : "Shield Active");

    // Character Special Ability Icons (only if active)
    if (spaceship.IsMewtwoAbilityActive()) drawHUDIcon(PURPLE, "M", "Mewtwo Ability Active");
    if (spaceship.IsGreenNinjaAbilityActive()) drawHUDIcon(LIME, "G", "Green Ninja Ability Active");
    if (spaceship.IsPalkiaAbilityActive()) drawHUDIcon(PINK, "P", "Palkia Ability Active");
    if (spaceship.IsDialgaAbilityActive()) drawHUDIcon(SKYBLUE, "T", "Dialga Ability Active");
    if (spaceship.IsNovaAbilityActive()) drawHUDIcon(GOLD, "V", "Nova Ability Active");

}

void Game::DrawCharacterStats(Font font) {
    float windowWidth = (float)GetScreenWidth();
    float windowHeight = (float)GetScreenHeight();
    
    // Expanded panel for a dual-column layout (Character stats left, Power-up legend right)
    Rectangle panel = { windowWidth * 0.12f, windowHeight * 0.15f, windowWidth * 0.76f, windowHeight * 0.62f };
    DrawRectangleRec(panel, Fade(BLACK, 0.9f));
    DrawRectangleLinesEx(panel, 2, SKYBLUE);

    const char* name = "";
    const char* ability = "";
    const char* desc = "";
    int baseLives = 5;
    float fireRate = 0.35f;

    switch (selectedCharacterIndex) {
        case 1: name = "PALKIA"; ability = "SPACIAL (X)"; desc = "Teleports to a random location.\nGrants brief invincibility."; baseLives = 4; fireRate = 0.22f; break;
        case 2: name = "DIALGA"; ability = "TIME (X)"; desc = "Slows down time and drastically\nincreases ship fire rate."; baseLives = 7; fireRate = 0.45f; break;
        case 3: name = "MEWTWO"; ability = "PSYCHIC (X)"; desc = "Clears all lasers on screen.\nGrants temporary invincibility."; baseLives = 6; fireRate = 0.27f; break;
        case 4: name = "GREEN NINJA"; ability = "HURRICANE (X)"; desc = "Increases speed and fires\ntriple parallel lasers."; baseLives = 5; fireRate = 0.30f; break;
        case 5: name = "NOVA BLAST"; ability = "NOVA (X)"; desc = "Wipes nearby projectiles and\ndeals massive area damage."; baseLives = 5; fireRate = 0.25f; break;
        default: name = "CLASSIC SHIP"; ability = "None"; desc = "A reliable standard-issue\nFederation scout vessel."; baseLives = 5; fireRate = 0.35f; break;
    }

    float x = panel.x + 30;
    float y = panel.y + 30;

    DrawTextEx(font, name, {x, y}, 36, 2, GOLD);
    y += 50;

    DrawTextEx(font, TextFormat("Base Lives: %d", baseLives), {x, y}, 22, 2, WHITE);
    y += 35;

    DrawTextEx(font, "Fire Speed:", {x, y}, 22, 2, WHITE);
    float barWidth = 150;
    // Normalize: Lower fireRate value means higher visual speed
    float fill = (1.0f - (fireRate - 0.2f) / 0.3f) * barWidth; 
    if (fill < 0) fill = 0;
    if (fill > barWidth) fill = barWidth;
    
    DrawRectangle(x + 120, y + 4, barWidth, 14, DARKGRAY);
    DrawRectangle(x + 120, y + 4, (int)fill, 14, ORANGE);
    DrawRectangleLines(x + 120, y + 4, barWidth, 14, WHITE);
    y += 50;

    DrawTextEx(font, "Special Ability:", {x, y}, 22, 2, SKYBLUE);
    y += 25;
    DrawTextEx(font, ability, {x, y}, 26, 2, WHITE);
    y += 35;
    DrawTextEx(font, desc, {x, y}, 20, 1, GRAY);

    // --- RIGHT COLUMN: Power-up Legend ---
    float rx = panel.x + panel.width / 2.0f + 30.0f;
    float ry = panel.y + 30.0f;

    DrawTextEx(font, "POWER-UP LEGEND", {rx, ry}, 28, 2, GOLD);
    ry += 40;

    auto drawStatPowerUp = [&](Color color, const char* label, const char* pName, const char* pEffect) {
        float iconRadius = 14.0f;
        Vector2 pos = {rx + iconRadius, ry + 12.0f};
        DrawCircleV(pos, iconRadius, color);
        DrawCircleLines(pos.x, pos.y, iconRadius, WHITE);
        
        Vector2 labelSize = MeasureTextEx(font, label, 18, 1);
        DrawTextEx(font, label, {pos.x - labelSize.x/2.0f, pos.y - labelSize.y/2.0f}, 18, 1, WHITE);
        
        DrawTextEx(font, pName, {rx + 40.0f, ry}, 20, 2, color);
        DrawTextEx(font, pEffect, {rx + 40.0f, ry + 22.0f}, 16, 1, GRAY);
        ry += 52; // Vertical spacing between entries
    };

    drawStatPowerUp(ORANGE, "R", "RAPID FIRE", "Double your firing speed.");
    drawStatPowerUp(GOLD, "D", "DOUBLE SCORE", "Earn double points for kills.");
    drawStatPowerUp(SKYBLUE, "S", "SHIELD", "Absorbs one hit from enemies.");
    drawStatPowerUp(RED, "N", "NUKE", "Wipes out all common aliens.");
    drawStatPowerUp(GREEN, "L", "EXTRA LIFE", "Repairs ship and adds 1 life.");
    drawStatPowerUp(BLUE, "C", "OVERCHARGE", "Deflects projectiles back at foes.");
}

void Game::HandleInput() {
    if (fadeToBlackActive) {
        return; // Block all input during fade to black
    }

    if (bossNarrativeActive) {
        bool textComplete = bossNarrativeCharCount >= (float)bossNarrativeText.length();

        // Handle explicit skip actions (Finish typing immediately)
        if (!textComplete) {
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_SPACE)) {
                // Prevent Raylib from treating ESC as an immediate exit key.
                skipNarrativeTriggered = true;
            }

            // Measurement using default font to match the click area of the Draw function
            Vector2 skipTextSize = MeasureTextEx(GetFontDefault(), "SKIP [ESC/SPACE]", 20, 2);
            Rectangle skipButton = {(float)GetScreenWidth() - skipTextSize.x - 30, (float)GetScreenHeight() - skipTextSize.y - 20, skipTextSize.x + 20, skipTextSize.y + 10};
            
            if (CheckCollisionPointRec(GetMousePosition(), skipButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                skipNarrativeTriggered = true;
            }
        }

        // Progress input (Typewriter completion vs Starting the fight)
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
            if (!textComplete) {
                skipNarrativeTriggered = true;
            } else {
                // Narrative is finished, start the actual encounter
                bossNarrativeActive = false;
                skipNarrativeTriggered = false; 
                bossBannerActive = true; 
                PlaySound(bossBannerSound); // Play sound when boss banner appears

                // Visual Shockwave for Boss Banner appearance with themed colors
                std::vector<Color> shockwaveColors;
                switch (currentStage) {
                    case 1: // Giratina (Shadow/Ghost)
                        shockwaveColors = {RED, PURPLE, DARKGRAY};
                        screenFlashColor = PURPLE;
                    bossBannerColor = RED;
                        break;
                    case 2: // Roxo (Electric/Cold)
                        shockwaveColors = {SKYBLUE, BLUE, WHITE};
                        screenFlashColor = SKYBLUE;
                    bossBannerColor = SKYBLUE;
                        break;
                    case 3: // Lugia (Storm/Ocean)
                        shockwaveColors = {WHITE, SKYBLUE, LIGHTGRAY};
                        screenFlashColor = WHITE;
                    bossBannerColor = WHITE;
                        break;
                    case 4: // Scuba Cat (Tropical/Water)
                        shockwaveColors = {CYAN, GREEN, YELLOW};
                        screenFlashColor = CYAN;
                    bossBannerColor = CYAN;
                        break;
                    case 5: // Arceus (Divine/Primal)
                        shockwaveColors = {GOLD, WHITE, MAGENTA};
                        screenFlashColor = GOLD;
                    bossBannerColor = GOLD;
                        break;
                    default:
                        shockwaveColors = {RED, WHITE, GRAY}; // Fallback
                        screenFlashColor = RED;
                    bossBannerColor = RED;
                }
                screenFlashTime = 0.3f; // Sell the impact with a themed flash

                Vector2 screenCenter = {(float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.5f};
                for (int i = 0; i < 80; i++) {
                    Particle p;
                    p.pos = screenCenter;
                    float ang = (float)GetRandomValue(0, 360) * (PI / 180.0f);
                    float speed = (float)GetRandomValue(400, 800); // High speed radial expansion
                    p.vel = { cosf(ang) * speed, sinf(ang) * speed };
                    p.color = shockwaveColors[GetRandomValue(0, shockwaveColors.size() - 1)];
                    p.life = 0.6f;
                    p.shape = (i % 2 == 0) ? Particle::RING : Particle::TRIANGLE;
                    particles.push_back(p);
                }

                bossBannerTimer = 0.0f;

                if (currentStage == 1 && !bossSpawned) {
                    boss.Spawn(currentStage);
                    bossSpawned = true;
                } else if (currentStage >= 2 && !boss2Spawned) {
                    boss2.Spawn(currentStage);
                    boss2Spawned = true;
                }
                PlaySound(bossAttackSound);
            }
        }
        return; // Block other inputs while in narrative
    }

    // Mewtwo special ability input
    if (selectedCharacterIndex == 3) { // Mewtwo is character index 3
        if (IsKeyPressed(KEY_X)) {
            ActivateMewtwoAbility();
        }
    } else if (selectedCharacterIndex == 4) { // Green Ninja
        if (IsKeyPressed(KEY_X)) {
            ActivateGreenNinjaAbility();
        }
    } else if (selectedCharacterIndex == 1) { // Palkia
        if (IsKeyPressed(KEY_X)) {
            ActivatePalkiaAbility();
        }
    } else if (selectedCharacterIndex == 2) { // Dialga
        if (IsKeyPressed(KEY_X)) {
            ActivateDialgaAbility();
        }
    } else if (selectedCharacterIndex == 5) { // Nova Blast
        if (IsKeyPressed(KEY_X)) {
            ActivateNovaAbility();
        }
    }

    if (!run && gameOverActive) {
        if (IsKeyPressed(KEY_ENTER)) {
            StopMusicStream(GameOverMusic);
            Reset();
            InitGame();
        }
    }

    if(run){
        // Allow arrow keys or A/D for movement; allow firing while moving
        if(IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
            spaceship.MoveLeft(lives);
        }
        if(IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)){
            spaceship.MoveRight(lives);
        }
        // Handle Warp Dash
        if(IsKeyPressed(KEY_LEFT_SHIFT)) {
            int dir = 0;
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) dir = -1;
            else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) dir = 1;
            if (dir != 0) spaceship.WarpDash(dir, particles);
        }
        if(IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            if(spaceship.FireLaser(particles)) {
                if (spaceship.DoubleScoreActive()) {
                    TriggerScreenShake(0.10f, 2.0f); // Minimized recoil for large laser
                    distortionLevel = 0.2f;          // Reduced to fix "stocked sprites" look
                } else {
                    TriggerScreenShake(0.06f, 1.5f); // Subtle recoil effect
                }
            }
        }
    }
}

void Game::DeleteInactiveLasers()
{
    for(auto it = spaceship.lasers.begin(); it != spaceship.lasers.end();){
        if(!it -> active) {
            it = spaceship.lasers.erase(it);
        } else {
            ++ it;
        }
    }

    for(auto it = alienLasers.begin(); it != alienLasers.end();){
        if(!it -> active) {
            it = alienLasers.erase(it);
        } else {
            ++ it;
        }
    }
}

std::vector<Obstacle> Game::CreateObstacles()
{
    int obstacleWidth = Obstacle::grid[0].size() * 3;
    float gap = (GetScreenWidth() - (4 * obstacleWidth))/5;

    for(int i = 0; i < 4; i++) {
        float offsetX = (i + 1) * gap + i * obstacleWidth;
        obstacles.push_back(Obstacle({offsetX, float(GetScreenHeight() - 200)}));
    }
    return obstacles;
}

std::vector<Alien> Game::CreateAliens(int stage)
{
    std::vector<Alien> aliens;
    for(int row = 0; row < 5; row++) {
        for(int column = 0; column < 11; column++) {

            int alienType;
            if (stage == 2) {
                if (row == 0) {
                    alienType = 3;
                } else if (row == 1) {
                    alienType = 2;
                } else if (row == 2) {
                    alienType = 4;
                } else if (row == 3) {
                    alienType = 2;
                } else {
                    alienType = 5;
                }

                if (row == 1 && (column == 4 || column == 6)) alienType = 4; // extra tanks
                if (row == 3 && (column == 2 || column == 8)) alienType = 5; // kamikaze flankers
                if (row == 4 && (column == 1 || column == 9)) alienType = 5; // additional kamikazes
            } else {
                if(row == 0) {
                    alienType = 3;
                } else if (row == 1 || row == 2) {
                    alienType = 2;
                } else {
                    alienType = 1;
                }
            }

            float x = 75 + column * 55;
            float y = 110 + row * 55;
            aliens.push_back(Alien(alienType, {x, y}, stage)); // Pass the stage to the Alien constructor
        }
    }
    return aliens;
}

void Game::SpawnAliensForCurrentStage() {
    aliens.clear(); // Clear existing aliens
    aliens = CreateAliens(currentStage); // Create new aliens for the current stage
    aliensDirection = 1; // Reset direction
    timeLastAlienFired = GetTime(); // Reset alien firing timer
    mysteryShipSpawnInterval = GetRandomValue(10, 20); // Reset mystery ship interval
    aliensSpawnedForCurrentStage = true; // Mark aliens as spawned
}
void Game::MoveAliens() {
    float dt = GetFrameTime();
    Rectangle sr = spaceship.getRect();
    Vector2 target = { sr.x + sr.width/2.0f, sr.y + sr.height/2.0f };
    bool hitWall = false;

    for(auto& alien: aliens) {
        if (!alien.isDiving) {
            if(alien.position.x + alien.alienImages[alien.type - 1].width > GetScreenWidth() - 25) {
                aliensDirection = -1;
                hitWall = true;
            }
            if(alien.position.x < 25) {
                aliensDirection = 1;
                hitWall = true;
            }
        }
    }

    if (hitWall) MoveDownAliens(4);

    for(auto& alien: aliens) {
        alien.Update(aliensDirection, target, dt, particles, 1.0f);
    }
}

void Game::MoveDownAliens(int distance)
{
    for(auto& alien: aliens) {
        if (!alien.isDiving) alien.position.y += distance;
    }
}

void Game::AlienShootLaser()
{
    double currentTime = GetTime();
    if(currentTime - timeLastAlienFired >= alienLaserShootInterval && !aliens.empty()) {
        int randomIndex = GetRandomValue(0, aliens.size() - 1);
        const Alien& alien = aliens[randomIndex]; // Use const reference as we only read
        alienLasers.push_back(Laser({alien.position.x + alien.alienImages[alien.type -1].width/2, 
                                    alien.position.y + alien.alienImages[alien.type - 1].height}, 
                                    6, {243, 216, 63, 255}));
        timeLastAlienFired = GetTime();
    }
}

void Game::SpawnKamikazeExplosion(Vector2 position, std::vector<Particle>& particles) {
    for(int i = 0; i < 25; i++) { // 25 particles for the explosion
        Particle p;
        p.pos = position;
        float ang = GetRandomValue(0, 360) * (PI / 180.0f);
        float speed = GetRandomValue(80, 250);
        p.vel = { cosf(ang) * speed, sinf(ang) * speed };
        p.color = RED; // Kamikaze explosion color
        p.life = GetRandomValue(5, 12) / 10.0f; // 0.5 to 1.2 seconds life
        p.shape = Particle::TRIANGLE; // Explosive shape
        particles.push_back(p);
    }
    PlaySound(explosionSound); // Reuse existing explosion sound
}



void Game::CheckForCollisions()
{
    // Spaceship Lasers
    Rectangle shipRect = spaceship.getRect();
    for(auto& laser: spaceship.lasers) {
        // Aliens collision check (reverse order for front-row blocking)
        for (int i = (int)aliens.size() - 1; i >= 0; i--) {
            if(CheckCollisionRecs(aliens[i].getRect(), laser.getRect()))
            {
                // explosion particles at alien
                Vector2 explPos = aliens[i].position;
                for(int pi=0; pi<12; ++pi){
                    Particle p;
                    p.pos = explPos;
                    float ang = GetRandomValue(0, 360) * (3.14159f/180.0f);
                    float speed = GetRandomValue(40, 200);
                    p.vel = { cosf(ang)*speed, sinf(ang)*speed };
                    Color pCol = YELLOW;
                    if(aliens[i].type == 3) pCol = PURPLE;
                    else if(aliens[i].type == 2 || aliens[i].type == 4) pCol = ORANGE;
                    else if(aliens[i].type == 5) pCol = RED;
                    p.color = pCol;
                    p.life = GetRandomValue(4,10) / 8.0f;
                    p.shape = Particle::CIRCLE;
                    particles.push_back(p);
                }

                aliens[i].health--;
                // Brief hit flash and subtle shake for feedback
                aliens[i].flashTimer = 0.12f;
                TriggerScreenShake(0.08f, 3.0f);
                if (aliens[i].health > 0) {
                    if (aliens[i].type == 5 && aliens[i].health <= aliens[i].maxHealth / 2) {
                        aliens[i].isDiving = true;
                    } // No damage multiplier here, as it's applied to player's damage output
                    laser.active = false;
                    goto next_laser; // Stop checking this laser once it hits something
                }

                PlaySound(explosionSound);
                int base = 0;
                if(aliens[i].type == 1) base = 100;
                else if(aliens[i].type == 2) base = 200;
                else if(aliens[i].type == 3) base = 300;
                else if(aliens[i].type == 4) base = 500;
                else if(aliens[i].type == 5) base = 400;
                
                int add = base * (spaceship.DoubleScoreActive() ? 2 : 1);
                score += add;
                checkForHighscore();

                if(GetRandomValue(1,100) <= 20) { // 20% chance
                    int ptype = GetRandomValue(1,5);
                    powerups.push_back(PowerUp(ptype, aliens[i].position));
                }

                aliens.erase(aliens.begin() + i);
                laser.active = false;
                goto next_laser;
            }
        }
        next_laser:;

        // obstacles
        for(auto& obstacle: obstacles){
            auto it = obstacle.blocks.begin();
            while(it != obstacle.blocks.end()){
                if(CheckCollisionRecs(it -> getRect(), laser.getRect())){
                    it = obstacle.blocks.erase(it);
                    laser.active = false;
                } else {
                    ++it;
                }
            }
        }

        // mystery ship
        if(CheckCollisionRecs(mysteryship.getRect(), laser.getRect())){
            // explosion at mystery ship
            Rectangle mr = mysteryship.getRect();
            Vector2 explPos = { mr.x + mr.width/2.0f, mr.y + mr.height/2.0f };
            for(int pi=0; pi<24; ++pi){
                Particle p;
                p.pos = explPos;
                float ang = GetRandomValue(0, 360) * (3.14159f/180.0f);
                float speed = GetRandomValue(60, 260);
                p.vel = { cosf(ang)*speed, sinf(ang)*speed };
                p.color = PINK;
                p.life = GetRandomValue(6,12) / 8.0f;
                p.shape = Particle::RING;
                particles.push_back(p);
            }
            mysteryship.alive = false;
            laser.active = false;
            score += 500 * (spaceship.DoubleScoreActive() ? 2 : 1);
            checkForHighscore();
            PlaySound(explosionSound);
        }

        // boss
        if(boss.alive && CheckCollisionRecs(boss.getRect(), laser.getRect())) {
            laser.active = false;
            bool wasShielded = boss.ShieldActive();
            Rectangle lr = laser.getRect();
            Vector2 impactPoint = {lr.x + lr.width * 0.5f, lr.y + lr.height * 0.5f};
            boss.TryDamage(1 * spaceship.GetDamageMultiplier(), particles, impactPoint);
            if(!wasShielded) {
                PlaySound(explosionSound);
            } else {
                PlaySound(bossAttackSound);
            }
            if(boss.health <= 0) {
                // big explosion at boss
                Vector2 explPos = { boss.position.x + boss.getRect().width / 2.0f, boss.position.y + boss.getRect().height / 2.0f };
                for(int pi=0; pi<60; ++pi){
                    Particle p;
                    p.pos = explPos;
                    float ang = GetRandomValue(0, 360) * (3.14159f/180.0f);
                    float speed = GetRandomValue(80, 360);
                    p.vel = { cosf(ang)*speed, sinf(ang)*speed };
                    p.color = GOLD;
                    p.life = GetRandomValue(8,16) / 8.0f;
                    p.shape = Particle::TRIANGLE;
                    particles.push_back(p);
                }
                boss.SpawnDeathSplash();
                boss.alive = false;
                score += 2000;
                checkForHighscore(); // Check highscore after boss defeat
                
                // Transition to new stage
                currentStage++;
                stageStartTime = GetTime();
                stageMessageShown = false;
                aliensSpawnedForCurrentStage = false; // Mark for new alien spawning
            }
        }

        // boss2
        if(boss2.alive && CheckCollisionRecs(boss2.getRect(), laser.getRect())) {
            laser.active = false;
            bool wasShielded2 = boss2.ShieldActive();
            Rectangle lr2 = laser.getRect();
            Vector2 impactPoint2 = {lr2.x + lr2.width * 0.5f, lr2.y + lr2.height * 0.5f};
            boss2.TryDamage(1 * spaceship.GetDamageMultiplier(), particles, impactPoint2);
            if(!wasShielded2) {
                PlaySound(explosionSound);
            } else {
                PlaySound(bossAttackSound);
            }
            if(boss2.health <= 0) {
                // big explosion at boss2
                Vector2 explPos2 = { boss2.position.x + boss2.getRect().width / 2.0f, boss2.position.y + boss2.getRect().height / 2.0f };
                for(int pi=0; pi<60; ++pi){
                    Particle p;
                    p.pos = explPos2;
                    float ang = GetRandomValue(0, 360) * (3.14159f/180.0f);
                    float speed = GetRandomValue(80, 360);
                    p.vel = { cosf(ang)*speed, sinf(ang)*speed };
                    p.color = GOLD;
                    p.life = GetRandomValue(8,16) / 8.0f;
                    p.shape = Particle::TRIANGLE;
                    particles.push_back(p);
                }
                boss2.SpawnDeathSplash();
                boss2.alive = false;
                score += 2000;
                checkForHighscore(); // Check highscore after boss defeat

                // Transition to new stage
                if (currentStage < 5) currentStage++;
                stageStartTime = GetTime();
                stageMessageShown = false;
                aliensSpawnedForCurrentStage = false; // Mark for new alien spawning
                boss2Spawned = false; // Reset so the next boss variant can spawn later
            }
        }
    }

    // Move boss hazards collision OUTSIDE the laser loop so they work even when not shooting
    for(auto& proj : boss.GetProjectiles()) {
        if(!proj.active) continue;
        if(CheckCollisionCircleRec(proj.pos, proj.radius, shipRect)) {
            if(!spaceship.HasShield() && !spaceship.IsInvincible()) {
                proj.active = false;
                lives--;
                spaceship.TriggerHit();
                if (proj.type == 4) { // Meteor Impact Explosion on Player
                    for(int i=0; i<45; i++) {
                        Particle p;
                        p.pos = proj.pos;
                        float ang = GetRandomValue(0, 360) * (PI/180.0f);
                        float spd = GetRandomValue(150, 450);
                        p.vel = { cosf(ang)*spd, sinf(ang)*spd };
                        p.color = (i % 3 == 0) ? BROWN : (i % 3 == 1 ? ORANGE : RED);
                        p.life = GetRandomValue(8, 20) / 10.0f;
                        p.shape = (i % 2 == 0) ? Particle::CIRCLE : Particle::TRIANGLE;
                        particles.push_back(p);
                    }
                } else {
                    for(int i=0; i<12; i++) {
                        Particle p;
                        p.pos = proj.pos;
                        float ang = GetRandomValue(0, 360) * (PI/180.0f);
                        float spd = GetRandomValue(40, 140);
                        p.vel = { cosf(ang)*spd, sinf(ang)*spd };
                        p.color = i % 2 == 0 ? GRAY : DARKGRAY;
                        p.life = GetRandomValue(4, 12) / 10.0f;
                        p.shape = Particle::CIRCLE;
                        particles.push_back(p);
                    }
                }
                if(lives == 0) GameOver();
            } else if (spaceship.IsShieldOvercharged()) {
                // Deflect: Convert boss projectile into a powerful player laser
                proj.active = false;
                spaceship.TriggerDeflectGlint();
                chromaticAmount = 0.006f; // Trigger subtle screen color splitting
                PlaySound(deflectSound);
                spaceship.lasers.push_back(Laser(proj.pos, -9, BLUE, 8.0f));
                for(int i=0; i<12; i++) {
                    Particle p;
                    p.pos = proj.pos;
                    float ang = (float)GetRandomValue(0, 360) * (PI/180.0f);
                    float spd = (float)GetRandomValue(120, 300);
                    p.vel = { cosf(ang)*spd, sinf(ang)*spd };
                    p.color = BLUE;
                    p.life = 0.4f;
                    p.shape = Particle::RING;
                    particles.push_back(p);
                }
            }
        }
    }

    for(auto& proj : boss2.GetProjectiles()) {
        if(!proj.active) continue;
        if(CheckCollisionCircleRec(proj.pos, proj.radius, shipRect)) {
            if(!spaceship.HasShield() && !spaceship.IsInvincible()) {
                proj.active = false;
                lives--;
                spaceship.TriggerHit();
                if (proj.type == 4) { // Meteor Impact Explosion on Player
                    for(int i=0; i<45; i++) {
                        Particle p;
                        p.pos = proj.pos;
                        float ang = GetRandomValue(0, 360) * (PI/180.0f);
                        float spd = GetRandomValue(150, 450);
                        p.vel = { cosf(ang)*spd, sinf(ang)*spd };
                        p.color = (i % 3 == 0) ? BROWN : (i % 3 == 1 ? ORANGE : RED);
                        p.life = GetRandomValue(8, 20) / 10.0f;
                        p.shape = (i % 2 == 0) ? Particle::CIRCLE : Particle::TRIANGLE;
                        particles.push_back(p);
                    }
                } else {
                    for(int i=0; i<12; i++) {
                        Particle p;
                        p.pos = proj.pos;
                        float ang = GetRandomValue(0, 360) * (PI/180.0f);
                        float spd = GetRandomValue(40, 140);
                        p.vel = { cosf(ang)*spd, sinf(ang)*spd };
                        p.color = i % 2 == 0 ? GRAY : DARKGRAY;
                        p.life = GetRandomValue(4, 12) / 10.0f;
                        p.shape = Particle::CIRCLE;
                        particles.push_back(p);
                    }
                }
                if(lives == 0) GameOver();
            } else if (spaceship.IsShieldOvercharged()) {
                // Deflect: Convert boss2 projectile into a powerful player laser
                proj.active = false;
                spaceship.TriggerDeflectGlint();
                chromaticAmount = 0.006f;
                PlaySound(deflectSound);
                spaceship.lasers.push_back(Laser(proj.pos, -9, BLUE, 8.0f));
                for(int i=0; i<12; i++) {
                    Particle p;
                    p.pos = proj.pos;
                    float ang = (float)GetRandomValue(0, 360) * (PI/180.0f);
                    float spd = (float)GetRandomValue(120, 300);
                    p.vel = { cosf(ang)*spd, sinf(ang)*spd };
                    p.color = BLUE;
                    p.life = 0.4f;
                    p.shape = Particle::RING;
                    particles.push_back(p);
                }
            }
        }
    }

    BossBeam& beam = boss.GetBeam();
    if(beam.active && !beam.telegraph) {
        Vector2 shipCenter = { shipRect.x + shipRect.width / 2.0f, shipRect.y + shipRect.height / 2.0f };
        Vector2 a = beam.start;
        Vector2 b = beam.end;
        Vector2 ab = { b.x - a.x, b.y - a.y };
        Vector2 ap = { shipCenter.x - a.x, shipCenter.y - a.y };
        float abLen2 = ab.x*ab.x + ab.y*ab.y;
        float t = 0.0f;
        if(abLen2 > 0.0001f) t = (ap.x*ab.x + ap.y*ab.y) / abLen2;
        if(t < 0.0f) t = 0.0f;
        if(t > 1.0f) t = 1.0f;
        Vector2 closest = { a.x + ab.x * t, a.y + ab.y * t };
        float dx = shipCenter.x - closest.x;
        float dy = shipCenter.y - closest.y;
        float dist2 = dx*dx + dy*dy;
        float threshold = beam.width * 0.75f;
        if(dist2 <= threshold*threshold) {
            if(!spaceship.HasShield() && !spaceship.IsInvincible()) {
                lives--;
                spaceship.TriggerHit();
                if (GetRandomValue(0, 4) == 0) {
                    Particle p;
                    p.pos = closest;
                    p.vel = { (float)GetRandomValue(-100, 100), (float)GetRandomValue(-100, 100) };
                    p.color = RED;
                    p.life = 0.4f;
                    p.shape = Particle::TRIANGLE;
                    particles.push_back(p);
                }
                if(lives == 0) GameOver();
            }
        }
    }

    BossBeam& beam2 = boss2.GetBeam();
    if(beam2.active && !beam2.telegraph) {
        Rectangle _shipRect = spaceship.getRect();
        Vector2 shipCenter = { _shipRect.x + _shipRect.width / 2.0f, _shipRect.y + _shipRect.height / 2.0f };
        Vector2 a = beam2.start;
        Vector2 b = beam2.end;
        Vector2 ab = { b.x - a.x, b.y - a.y };
        Vector2 ap = { shipCenter.x - a.x, shipCenter.y - a.y };
        float abLen2 = ab.x*ab.x + ab.y*ab.y;
        float t = 0.0f;
        if(abLen2 > 0.0001f) t = (ap.x*ab.x + ap.y*ab.y) / abLen2;
        if(t < 0.0f) t = 0.0f;
        if(t > 1.0f) t = 1.0f;
        Vector2 closest = { a.x + ab.x * t, a.y + ab.y * t };
        float dx = shipCenter.x - closest.x;
        float dy = shipCenter.y - closest.y;
        float dist2 = dx*dx + dy*dy;
        float threshold = beam2.width * 0.75f;
        if(dist2 <= threshold*threshold) {
            if(!spaceship.HasShield() && !spaceship.IsInvincible()) {
                lives--;
                // No proj.active = false here as it's a continuous beam
                spaceship.TriggerHit();
                if (GetRandomValue(0, 4) == 0) {
                    Particle p;
                    p.pos = closest;
                    p.vel = { (float)GetRandomValue(-100, 100), (float)GetRandomValue(-100, 100) };
                    p.color = RED;
                    p.life = 0.4f;
                    p.shape = Particle::TRIANGLE;
                    particles.push_back(p);
                }
                if(lives == 0) GameOver();
            }
        }
    }

    //Alien Lasers

    for(auto& laser: alienLasers) {
        Rectangle shipRect = spaceship.getRect();
        if(CheckCollisionRecs(laser.getRect(), spaceship.getRect())){
            if(!spaceship.HasShield() && !spaceship.IsInvincible()) {
                laser.active = false;
                lives --;
                spaceship.TriggerHit();
                for(int i=0; i<10; i++) {
                    Particle p;
                    p.pos = Vector2{ shipRect.x + shipRect.width/2.0f, shipRect.y };
                    float ang = GetRandomValue(220, 320) * (PI/180.0f);
                    float spd = GetRandomValue(50, 150);
                    p.vel = Vector2{ cosf(ang)*spd, sinf(ang)*spd };
                    p.color = i % 2 == 0 ? GRAY : ORANGE;
                    p.life = GetRandomValue(5, 10) / 10.0f;
                    p.shape = Particle::CIRCLE;
                    particles.push_back(p);
                }
                if(lives == 0) {
                    GameOver();
                }
            } else {
                if (spaceship.IsShieldOvercharged()) {
                    // Deflect: Convert alien laser into a player laser
                    laser.active = false;
                    spaceship.TriggerDeflectGlint();
                    chromaticAmount = 0.006f;
                    PlaySound(deflectSound);
                    spaceship.lasers.push_back(Laser({laser.getRect().x, laser.getRect().y}, -8, BLUE, 6.0f));
                    for(int i=0; i<8; i++){
                        Particle p;
                        p.pos = {laser.getRect().x, laser.getRect().y};
                        float ang = GetRandomValue(0, 360) * (PI/180.0f);
                        p.vel = {cosf(ang)*150.0f, sinf(ang)*150.0f};
                        p.color = BLUE;
                        p.life = 0.3f;
                        p.shape = Particle::RING;
                        particles.push_back(p);
                    }
                }
            }
        }

          for(auto& obstacle: obstacles){
            auto it = obstacle.blocks.begin();
            while(it != obstacle.blocks.end()){
                if(CheckCollisionRecs(it -> getRect(), laser.getRect())){
                    it = obstacle.blocks.erase(it);
                    laser.active = false;
                } else {
                    ++it;
                }
            }
        }
    }

    //Alien Collision with Obstacle
    
    // Iterate through aliens and obstacles to check for collisions
    // Iterate backwards to safely erase aliens
    for (int i = (int)aliens.size() - 1; i >= 0; --i) {
        Alien& alien = aliens[i];
        bool alienRemoved = false;

        for(auto& obstacle: obstacles) {
            auto it = obstacle.blocks.begin();
            while(it != obstacle.blocks.end()) {
                if(CheckCollisionRecs(it -> getRect(), alien.getRect())) {
                    if (alien.type == 5) { // Kamikaze alien
                        SpawnKamikazeExplosion(alien.position, particles);
                        aliens.erase(aliens.begin() + i);
                        alienRemoved = true;
                        it = obstacle.blocks.erase(it); // Remove obstacle block
                        break; // Break from inner obstacle.blocks loop, this alien is gone
                    } else { // Regular alien
                        it = obstacle.blocks.erase(it); // Just remove obstacle block
                    }
                } else {
                    ++it;
                }
            }
            if (alienRemoved) break; // If alien was removed, no need to check other obstacles
        }
        if (alienRemoved) continue; // If alien was removed, move to the next alien

        // Collision with Spaceship (after obstacle checks, as obstacle collision might remove alien)
        if (CheckCollisionRecs(alien.getRect(), spaceship.getRect())) {
            // If it's a Kamikaze, it explodes and deals damage.
            // If it's a regular alien, it also deals damage and is removed.
            // In both cases, the alien is removed.
            if (alien.type == 5) { // Kamikaze alien
                SpawnKamikazeExplosion(alien.position, particles);
            }
            aliens.erase(aliens.begin() + i); // Remove the alien after collision

            if(!spaceship.HasShield() && !spaceship.IsInvincible()) {
                lives--;
                spaceship.TriggerHit();
                if (lives == 0) GameOver();
            }
        } // No 'else' here, as the alien is always removed if it hits the spaceship
    }

    // PowerUp pickups
    for(auto it = powerups.begin(); it != powerups.end();){
        if(CheckCollisionRecs(it->getRect(), spaceship.getRect())){
            int type = it->type;
            Vector2 spawnPos = it->position;
            if(type == POWERUP_NUKE){
                // convert each alien into a nuke explosion visual and award score
                for(auto &a : aliens){
                    if(a.type == 1) score += 100;
                    else if(a.type == 2) score += 200;
                    else if(a.type == 3) score += 300;
                    nukeExplosions.push_back({a.position, a.type, 0.0f, 0.75f});

                    // spread explosion particles around the alien
                    int explIdx = a.type - 1;
                    if (explIdx < 0) explIdx = 0;
                    if (explIdx >= (int)Alien::alienImages.size()) explIdx = (int)Alien::alienImages.size() - 1;
                    Texture2D& explTex = Alien::alienImages[explIdx];
                    Vector2 explCenter;
                    if (explTex.id == 0 || explTex.width <= 0 || explTex.height <= 0) {
                        explCenter = { a.position.x + 16.0f, a.position.y + 16.0f };
                    } else {
                        explCenter = { a.position.x + explTex.width * 0.5f,
                                       a.position.y + explTex.height * 0.5f };
                    }
                    int particleCount = 12;
                    if (aliens.size() > 30) particleCount = 10;
                    if (aliens.size() > 45) particleCount = 8;
                    for(int i=0;i<particleCount;i++){
                        Particle p;
                        p.pos = explCenter;
                        float ang = GetRandomValue(0, 360) * (3.14159f/180.0f);
                        float speed = GetRandomValue(80, 260);
                        p.vel = { cosf(ang)*speed, sinf(ang)*speed };
                        p.color = (a.type == 3 ? PURPLE : (a.type == 2 ? ORANGE : YELLOW));
                        p.life = GetRandomValue(5,10) / 8.0f;
                        p.shape = Particle::TRIANGLE;
                        particles.push_back(p);
                    }
                }
                if(spaceship.DoubleScoreActive()) score *= 2;
                aliens.clear();
                TriggerScreenShake(0.9f, 8.0f);
                checkForHighscore();
                pickupLabel = "NUKE acquired!";
                pickupLabelUntil = GetTime() + 2.0;
                PlaySound(pickupSound);
                // central nuke burst at pickup
                for(int i=0;i<24;i++){
                    Particle p;
                    p.pos = spawnPos;
                    float ang = GetRandomValue(0, 360) * (3.14159f/180.0f);
                    float speed = GetRandomValue(60, 220);
                    p.vel = { cosf(ang)*speed, sinf(ang)*speed };
                    p.color = RED;
                    p.life = GetRandomValue(4,8) / 8.0f; // 0.5-1.0s
                    p.shape = Particle::TRIANGLE; // nuke feels explosive
                    particles.push_back(p);
                }
            } else if (type == POWERUP_LIFE) {
                if (lives < 5) lives++;
                pickupLabel = "Extra Life! Ship Repaired";
                pickupLabelUntil = GetTime() + 2.0;
                PlaySound(pickupSound);
                // Green healing particles
                for(int i=0; i<30; i++) {
                    Particle p;
                    p.pos = spawnPos;
                    float ang = GetRandomValue(0, 360) * (3.14159f/180.0f);
                    float speed = GetRandomValue(40, 180);
                    p.vel = { cosf(ang)*speed, sinf(ang)*speed };
                    p.color = GREEN;
                    p.life = GetRandomValue(4,8) / 8.0f;
                    p.shape = Particle::CIRCLE;
                    particles.push_back(p);
                }
            } else {
                spaceship.ApplyPowerUp(type);
                if(type == POWERUP_RAPID) pickupLabel = "Rapid Fire";
                else if(type == POWERUP_DOUBLE) pickupLabel = "Double Score";
                else if(type == POWERUP_SHIELD) pickupLabel = "Shield";
                else if(type == POWERUP_SHIELD_RECHARGE) pickupLabel = "Shield Overcharged!";
                pickupLabelUntil = GetTime() + 2.0;
                PlaySound(pickupSound);
                // particle burst colored per type
                Color col = WHITE;
                if(type == POWERUP_RAPID) col = ORANGE;
                else if(type == POWERUP_DOUBLE) col = GOLD;
                else if(type == POWERUP_SHIELD) col = SKYBLUE;
                else if(type == POWERUP_SHIELD_RECHARGE) col = BLUE;
                for(int i=0;i<30;i++){
                    Particle p;
                    p.pos = spawnPos;
                    float ang = GetRandomValue(0, 360) * (3.14159f/180.0f);
                    float speed = GetRandomValue(40, 180);
                    p.vel = { cosf(ang)*speed, sinf(ang)*speed };
                    p.color = col;
                    p.life = GetRandomValue(4,8) / 8.0f;
                    // shape per power-up type
                    if(type == POWERUP_RAPID) p.shape = Particle::TRIANGLE;
                    else if(type == POWERUP_DOUBLE) p.shape = Particle::RECT;
                    else if(type == POWERUP_SHIELD) p.shape = Particle::RING;
                    else if(type == POWERUP_SHIELD_RECHARGE) p.shape = Particle::RING;
                    else p.shape = Particle::CIRCLE;
                    particles.push_back(p);
                }
            }
            it = powerups.erase(it);
        } else {
            ++it;
        }
    }

    // Arceus Guard Summoning Logic (improved spacing & scaling based on current screen ratio)
    if (currentStage == 5 && boss2.spawnGuardsTrigger && guards.empty()) {
        boss2.spawnGuardsTrigger = false;
        TriggerScreenShake(1.5f, 25.0f); // Even more violent shake for the creator's arrival
        screenFlashTime = 1.2f;         // Longer flash to emphasize the power
        distortionLevel = 2.5f;          // Massive space-time distortion burst via shader

        std::vector<std::string> guardPaths = {
            Utils::ResolveAssetPath("Graphics/giratina BOSS 4.gif"),
            Utils::ResolveAssetPath("Graphics/roxo mini boss.gif"),
            Utils::ResolveAssetPath("Graphics/lugia boss.gif")
        };

        // Create guards first, allow each to set its draw size, then compute spacing
        std::vector<std::unique_ptr<Boss>> newGuards;
        for (size_t i = 0; i < guardPaths.size(); ++i) {
            auto g = std::make_unique<Boss>();
            g->LoadFromPath(guardPaths[i]);
            g->SetVariant(4);
            g->Spawn(currentStage);

            // target each guard to occupy ~15% of screen width for better boss presence
            float screenW = (float)GetScreenWidth();
            float desiredPer = 0.15f;
            float desiredWidth = screenW * desiredPer;
            float currentWidth = g->getRect().width;
            float scale = 1.0f;
            if (currentWidth > 0.01f) scale = desiredWidth / currentWidth;
            // clamp scale to avoid too small/large
            if (scale > 1.0f) scale = 1.0f;
            if (scale < 0.45f) scale = 0.45f;
            g->SetScale(scale);

            newGuards.push_back(std::move(g));
        }

        // Compute layout for a tight-knit centered formation
        float totalWidth = 0.0f;
        for (auto &ng : newGuards) totalWidth += ng->getRect().width;
        float screenW = (float)GetScreenWidth();
        float gap = 40.0f; // Fixed spacing between guards for a squad look
        float x = (screenW - (totalWidth + (gap * ((float)newGuards.size() - 1.0f)))) / 2.0f;
        float y = 160.0f; // vertical position for guards
        for (auto &ng : newGuards) {
            float w = ng->getRect().width;
            float h = ng->getRect().height;
            ng->position = { x, y };
            ng->spawnX = x;       // Set the origin for the AI oscillation
            ng->spawnY = y;
            ng->SetVariant(4);    // Enable the guard movement pattern

            // Visual effect: Energy burst at each guard's spawn location
            Vector2 spawnCenter = { ng->position.x + w/2.0f, ng->position.y + h/2.0f };
            
            // 1. Enhanced Divine Radial Burst
            for(int i = 0; i < 100; i++) {
                Particle p;
                p.pos = spawnCenter;
                float ang = (float)GetRandomValue(0, 360) * (PI / 180.0f);
                float speed = (float)GetRandomValue(150, 650);
                p.vel = { cosf(ang) * speed, sinf(ang) * speed };
                p.color = (i % 3 == 0) ? GOLD : (i % 3 == 1 ? WHITE : SKYBLUE);
                p.life = (float)GetRandomValue(8, 25) / 10.0f;
                // Mix shapes for a cooler "divine" summoning look
                p.shape = (i % 4 == 0) ? Particle::RING : (i % 4 == 1 ? Particle::TRIANGLE : Particle::CIRCLE);
                particles.push_back(p);
            }

            // 2. Divine Pillar Streaks (Vertical rays of light falling from above)
            for(int i = 0; i < 30; i++) {
                Particle p;
                p.pos = { spawnCenter.x + GetRandomValue(-40, 40), (float)GetRandomValue(-200, GetScreenHeight()) };
                p.vel = { 0, (float)GetRandomValue(600, 1200) }; // Rapidly falling rays
                p.color = (i % 2 == 0) ? GOLD : WHITE;
                p.life = 0.5f;
                p.shape = Particle::RECT;
                particles.push_back(p);
            }

            x += w + gap;
            guards.push_back(std::move(ng));
        }
    }

    // Collision for guards
    for (auto it = guards.begin(); it != guards.end();) {
        if (!(*it)->alive) { 
            it = guards.erase(it); 
            if (guards.empty()) boss2.isGuarded = false;
            continue; 
        }
        
        for (auto& laser : spaceship.lasers) {
            if (!laser.active) continue;
            if (CheckCollisionRecs((*it)->getRect(), laser.getRect())) {
                laser.active = false;
                (*it)->TryDamage(1, particles, {laser.getRect().x, laser.getRect().y});
                PlaySound(explosionSound);
                if ((*it)->health <= 0) {
                    (*it)->alive = false;
                    score += 500;
                    checkForHighscore();

                    // Elite guards drop a cluster: 2 high-tier items + 1 unique Shield Recharge
                    Vector2 chargePos = { (*it)->position.x + (float)GetRandomValue(-30, 30), (*it)->position.y + (float)GetRandomValue(-30, 30) };
                    powerups.push_back(PowerUp(POWERUP_SHIELD_RECHARGE, chargePos, true));

                    for (int i = 0; i < 2; i++) {
                        int ptype = GetRandomValue(POWERUP_NUKE, POWERUP_LIFE);
                        // Add a slight random offset so they burst out in a cluster
                        Vector2 dropPos = { (*it)->position.x + (float)GetRandomValue(-30, 30), (*it)->position.y + (float)GetRandomValue(-30, 30) };
                        powerups.push_back(PowerUp(ptype, dropPos, true));
                    }
                    
                    (*it)->SpawnDeathSplash();
                }
            }
        }
        ++it;
    }

    // Optimized boss transition trigger
    if (AreAllNormalEnemiesCleared() && currentStage <= 5) {
        bool noBossActive = !boss.alive && !boss2.alive && guards.empty();
        bool bossNotYetSpawned = (currentStage == 1 && !bossSpawned) || (currentStage > 1 && !boss2Spawned);

        if (bossNotYetSpawned && noBossActive && !bossTransitionPending && !fadeToBlackActive && !bossNarrativeActive && !bossBannerActive) {
            bossTransitionPending = true;
            bossTransitionTimer = 0.0f;

            // Pre-load resources and set narrative immediately to prevent stutters later
            switch(currentStage) {
                case 1: 
                    bossNarrativeText = "A DISTORTION IN SPACE DETECTED... GIRATINA APPEARS!"; 
                    break;
                case 2: 
                    bossNarrativeText = "THE MINI BOSS ROXO CHALLENGES YOU!"; 
                    boss2.LoadFromPath("Graphics/roxo mini boss.gif");
                    boss2.SetVariant(2);
                    break;
                case 3: 
                    bossNarrativeText = "THE GUARDIAN OF THE SEAS DESCENDS... LUGIA!"; 
                    boss2.LoadFromPath("Graphics/lugia boss.gif");
                    boss2.SetVariant(2);
                    break;
                case 4: 
                    bossNarrativeText = "A STRANGE SCUBA CAT EMERGES FROM THE DEPTHS!"; 
                    boss2.LoadFromPath("Graphics/kucing-scuba-scuba-cat.gif");
                    boss2.SetVariant(2);
                    break;
                case 5: 
                    bossNarrativeText = "THE CREATOR HAS ARRIVED... FACE ARCEUS!"; 
                    boss2.LoadFromPath("Graphics/areceus 2.gif");
                    boss2.SetVariant(3);
                    break;
                default: bossNarrativeText = "AN UNKNOWN ENTITY IS APPROACHING..."; break;
            }
        }
    }

    if (bossTransitionPending && !bossNarrativeActive && !bossBannerActive && !fadeToBlackActive) {
        float dt = GetFrameTime();
        bossTransitionTimer += dt;
        if (bossTransitionTimer >= bossTransitionDelay) {
            bossTransitionPending = false;
            fadeToBlackActive = true; // Start fade to black transition
            PlaySound(fadeSound); // Play fade sound
            fadeToBlackTimer = 0.0f;
            skipNarrativeTriggered = false;
            bossNarrativeCharCount = 0.0f;
            bossBannerActive = false;
            bossBannerTimer = 0.0f;
        }
    }
}


bool Game::AreAllNormalEnemiesCleared() const
{
    // Strictly require that normal aliens have been spawned for this stage
    // and that no normal enemies remain in the alien list.
    return aliensSpawnedForCurrentStage && aliens.empty();
}

void Game::GameOver()
{
    // Create a large, visible explosion at the player's position
    Rectangle shipRect = spaceship.getRect();
    Vector2 center = { shipRect.x + shipRect.width/2.0f, shipRect.y + shipRect.height/2.0f };
    
    for(int i = 0; i < 100; i++) {
        Particle p;
        p.pos = center;
        float ang = (float)GetRandomValue(0, 360) * (3.14159f / 180.0f);
        float speed = (float)GetRandomValue(100, 500);
        p.vel = { cosf(ang) * speed, sinf(ang) * speed };
        int rng = GetRandomValue(0, 2);
        p.color = (rng == 0) ? RED : (rng == 1 ? ORANGE : YELLOW);
        p.life = (float)GetRandomValue(8, 20) / 10.0f; // 0.8s to 2.0s
        p.shape = (Particle::Shape)GetRandomValue(0, 3);
        particles.push_back(p);
    }

    TriggerScreenShake(0.3f, 5.0f); // Further reduced shake duration for a smoother transition
    run = false;
    gameOverActive = true;
    float soundDuration = GetMusicTimeLength(GameOverMusic);
    gameOverEndTime = GetTime() + soundDuration;
    StopMusicStream(music);
    SeekMusicStream(GameOverMusic, 0.0f);
    PlayMusicStream(GameOverMusic);
}

void Game::InitGame()
{
    spaceship.Reset(); // Recalculate position based on current character scale
    obstacles = CreateObstacles(); // Re-create obstacles for new game
    SpawnAliensForCurrentStage(); // Create aliens for current stage
    aliensDirection = 1;
    timeLastAlienFired = 0.0;
    timeLastSpawn = 0.0;

    // Set character-specific stats
    double fireRate = 0.35;
    switch (selectedCharacterIndex) {
        case 1: // PALKIA
            lives = 4;
            fireRate = 0.22;
            break;
        case 2: // DIALGA
            lives = 7;
            fireRate = 0.45;
            break;
        case 3: // MEWTWO
            lives = 6;
            fireRate = 0.27;
            break;
        case 4: // GREEN NINJA
            lives = 5;
            fireRate = 0.30; // Ninja is also a bit faster at firing
            break;
        case 5: // PLAYER 4
            lives = 5;
            fireRate = 0.25;
            break;
        default: // Default / Classic spaceship
            lives = 5;
            fireRate = 0.35;
            break;
    }
    spaceship.SetBaseFireRate(fireRate);

    score = 0;
    highscore = loadHighscoreFromFile();
    run = true;
    gameOverActive = false;
    gameOverEndTime = 0.0;
    mysteryShipSpawnInterval = GetRandomValue(10, 20);
    bossSpawned = false;
    boss.alive = false;
    boss2Spawned = false;
    boss2.alive = false;
    screenShakeTime = 0.0f;
    screenShakeDuration = 0.0f;
    bossNarrativeActive = false;
    screenShakeStrength = 0.0f;
    distortionLevel = 0.0f;
    chromaticAmount = 0.0f;
    distortionCenter = { 0.5f, 0.5f };
    screenFlashTime = 0.0f;
    screenFlashColor = PURPLE;
    currentStage = 1;
    stageStartTime = GetTime();
    stageMessageShown = false;
    bossBannerActive = false;
    bossBannerColor = RED;
    aliensSpawnedForCurrentStage = true; // Initial aliens are spawned
    fadeToBlackActive = false;
    fadeToBlackTimer = 0.0f;
    bossBannerTimer = 0.0f;
    
    musicVolume = 1.0f;
    bossTransitionPending = false;
    bossTransitionTimer = 0.0f;
    SetMusicVolume(music, musicVolume);
    mewtwoReadySoundPlayed = true; // Mewtwo starts with ability ready
    greenNinjaReadySoundPlayed = true;
    novaReadySoundPlayed = true;
    InitStars();

    // Load boss for stage 1 (giratina)
    boss.LoadFromPath("Graphics/giratina BOSS 4.gif");
    boss.SetVariant(1);
    
    // load second boss asset (roxo mini boss.gif)
    std::string defaultBoss2Path = Utils::ResolveAssetPath(Paths::Graphics::Boss2);
    if (FileExists(defaultBoss2Path.c_str())) {
        boss2.LoadFromPath(defaultBoss2Path);
        TraceLog(LOG_INFO, "Using boss2 sprite: %s", defaultBoss2Path.c_str());
    } else {
        TraceLog(LOG_WARNING, "Boss2 sprite missing (%s), falling back to default boss sprite.", defaultBoss2Path.c_str());
        boss2.LoadFromPath(Utils::ResolveAssetPath(Paths::Graphics::Boss));
    }
    boss2.SetVariant(2);
    if(!IsMusicStreamPlaying(music)) PlayMusicStream(music);
}
void Game::checkForHighscore()
{
    if(score > highscore) {
        highscore = score;
        saveHighscoreToFile(highscore);
    }
}

void Game::saveHighscoreToFile(int highscore)
{
    std::ofstream highscoreFile("highscore.txt");
    if(highscoreFile.is_open()) {
        highscoreFile << highscore;
        highscoreFile.close();
    } else {
        std::cerr << "Failed to save highscore to file" << std::endl;
    }
}

int Game::loadHighscoreFromFile() {
    int loadedHighscore = 0;
    std::ifstream highscoreFile("highscore.txt");
    if(highscoreFile.is_open()) {
        highscoreFile >> loadedHighscore;
        highscoreFile.close();
    } else {
        std::cerr << "Failed to load highscore from file." << std::endl;
    }
    return loadedHighscore;
}

void Game::InitStars() {
    stars.clear();
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    if (screenWidth == 0) screenWidth = 800; // Fallback
    if (screenHeight == 0) screenHeight = 800;

    for (int i = 0; i < 100; i++) {
        stars.push_back({
            {(float)GetRandomValue(0, screenWidth), (float)GetRandomValue(0, screenHeight)},
            (float)GetRandomValue(10, 80) / 100.0f,
            (float)GetRandomValue(10, 25) / 10.0f,
            Fade(WHITE, (float)GetRandomValue(10, 60) / 100.0f)
        });
    }
}

void Game::Reset(bool fullReset) {
    spaceship.Reset();
    aliens.clear();
    alienLasers.clear();
    obstacles.clear();
    powerups.clear();
    nukeExplosions.clear();
    boss.alive = false;
    bossSpawned = false;
    boss2.alive = false;
    boss2Spawned = false;
    screenShakeTime = 0.0f;
    screenShakeDuration = 0.0f;
    bossNarrativeActive = false;
    screenShakeStrength = 0.0f;
    distortionLevel = 0.0f;
    chromaticAmount = 0.0f;
    screenFlashTime = 0.0f;
    gameOverActive = false;
    gameOverEndTime = 0.0;
    currentStage = 1;
    InitStars();
    stageStartTime = GetTime();
    stageMessageShown = false;
    skipNarrativeTriggered = false;
    bossNarrativeCharCount = 0.0f; // Reset narrative char count
    aliensSpawnedForCurrentStage = false; // No aliens spawned yet for the new stage
    bossBannerActive = false;
    bossBannerTimer = 0.0f;
    fadeToBlackActive = false;
    fadeToBlackTimer = 0.0f;
    bossTransitionPending = false;
    bossTransitionTimer = 0.0f;
    musicVolume = 1.0f;
}

void Game::SetBoss2Sprite(const std::string& assetPath)
{
    // Load a custom sprite for boss2 (assetPath is relative, e.g. "Graphics/haunter.png")
    boss2.LoadFromPath(Utils::ResolveAssetPath(assetPath));
    boss2.SetVariant(2);
}

bool Game::IsBossActive() const
{
    return boss.alive || boss2.alive || !guards.empty() || bossNarrativeActive || 
           bossTransitionPending || fadeToBlackActive || bossBannerActive;
}

void Game::ActivateMewtwoAbility()
{
    if (spaceship.TryActivateMewtwoAbility()) {
        // If ability was successfully activated, clear alien lasers
        alienLasers.clear();
        PlaySound(mewtwoActivateSound);
        mewtwoReadySoundPlayed = false; // Reset flag so it can play again when CD finishes

        // Visual effects: screen flash and distortion burst
        screenFlashTime = 0.3f;
        screenFlashColor = PURPLE;
        distortionLevel = 1.8f; // Strong psychic warp effect

        // Particle burst: psychic energy rings and triangles
        Rectangle shipRect = spaceship.getRect();
        Vector2 center = { shipRect.x + shipRect.width / 2.0f, shipRect.y + shipRect.height / 2.0f };
        for (int i = 0; i < 60; i++) {
            Particle p;
            p.pos = center;
            float ang = (float)GetRandomValue(0, 360) * (3.14159f / 180.0f);
            float speed = (float)GetRandomValue(150, 600);
            p.vel = { cosf(ang) * speed, sinf(ang) * speed };
            p.color = PURPLE;
            p.life = (float)GetRandomValue(6, 15) / 10.0f;
            p.shape = (i % 2 == 0) ? Particle::RING : Particle::TRIANGLE;
            particles.push_back(p);
        }
    }
}

void Game::ActivateGreenNinjaAbility()
{
    if (spaceship.TryActivateGreenNinjaAbility()) {
        PlaySound(mewtwoActivateSound);
        greenNinjaReadySoundPlayed = false;

        // Visual effects: green flash and distortion
        screenFlashTime = 0.25f;
        screenFlashColor = LIME;
        distortionLevel = 1.2f;

        // Leaf particles (Triangles)
        Rectangle shipRect = spaceship.getRect();
        Vector2 center = { shipRect.x + shipRect.width / 2.0f, shipRect.y + shipRect.height / 2.0f };
        for (int i = 0; i < 40; i++) {
            Particle p;
            p.pos = center;
            float ang = (float)GetRandomValue(0, 360) * (PI / 180.0f);
            float speed = (float)GetRandomValue(200, 500);
            p.vel = { cosf(ang) * speed, sinf(ang) * speed };
            p.color = LIME;
            p.life = (float)GetRandomValue(5, 12) / 10.0f;
            p.shape = Particle::TRIANGLE;
            particles.push_back(p);
        }
    }
}

void Game::ActivatePalkiaAbility()
{
    if (spaceship.TryActivatePalkiaAbility()) {
        PlaySound(mewtwoActivateSound);
        
        screenFlashTime = 0.15f;
        screenFlashColor = PINK;
        distortionLevel = 1.2f; // Psychic space distortion effect

        Rectangle shipRect = spaceship.getRect();
        Vector2 center = { shipRect.x + shipRect.width / 2.0f, shipRect.y + shipRect.height / 2.0f };
        for (int i = 0; i < 40; i++) {
            Particle p;
            p.pos = center;
            float ang = (float)GetRandomValue(0, 360) * (PI / 180.0f);
            float speed = (float)GetRandomValue(100, 300);
            p.vel = { cosf(ang) * speed, sinf(ang) * speed };
            p.color = PINK;
            p.life = (float)GetRandomValue(4, 10) / 10.0f;
            p.shape = Particle::RING;
            particles.push_back(p);
        }
    }
}

void Game::ActivateDialgaAbility()
{
    if (spaceship.TryActivateDialgaAbility()) {
        PlaySound(mewtwoActivateSound);
        slowMoTimer = (float)Spaceship::DIALGA_ABILITY_DURATION; // Use existing slow-mo logic
        
        screenFlashTime = 0.2f;
        screenFlashColor = SKYBLUE;
        distortionLevel = 1.5f;

        Rectangle shipRect = spaceship.getRect();
        Vector2 center = { shipRect.x + shipRect.width / 2.0f, shipRect.y + shipRect.height / 2.0f };
        for (int i = 0; i < 50; i++) {
            Particle p;
            p.pos = center;
            float ang = (float)GetRandomValue(0, 360) * (PI / 180.0f);
            float speed = (float)GetRandomValue(100, 400);
            p.vel = { cosf(ang) * speed, sinf(ang) * speed };
            p.color = SKYBLUE;
            p.life = (float)GetRandomValue(5, 15) / 10.0f;
            p.shape = Particle::RING;
            particles.push_back(p);
        }

        // Destroy nearby alien lasers
        Rectangle shipRectAfterTeleport = spaceship.getRect();
        Vector2 teleportCenter = { shipRectAfterTeleport.x + shipRectAfterTeleport.width / 2.0f, shipRectAfterTeleport.y + shipRectAfterTeleport.height / 2.0f };
        float destructionRadius = 100.0f; // Define a radius for laser destruction

        for (auto it = alienLasers.begin(); it != alienLasers.end(); ) {
            if (it->active) {
                Rectangle laserRect = it->getRect();
                Vector2 laserCenter = { laserRect.x + laserRect.width / 2.0f, laserRect.y + laserRect.height / 2.0f };
                if (CheckCollisionPointCircle(laserCenter, teleportCenter, destructionRadius)) {
                    it->active = false; // Deactivate the laser
                    // Add some particles for visual feedback of destroyed laser
                    for (int i = 0; i < 5; ++i) {
                        Particle p;
                        p.pos = laserCenter;
                        float ang = (float)GetRandomValue(0, 360) * (PI / 180.0f);
                        float speed = (float)GetRandomValue(50, 150);
                        p.vel = { cosf(ang) * speed, sinf(ang) * speed };
                        p.color = WHITE; // White particles for destroyed lasers
                        p.life = (float)GetRandomValue(3, 8) / 10.0f;
                        p.shape = Particle::CIRCLE;
                        particles.push_back(p);
                    }
                    it = alienLasers.erase(it); // Erase the laser from the vector
                } else {
                    ++it;
                }
            } else {
                ++it;
            }
        }
    }
}

void Game::ActivateNovaAbility()
{
    if (spaceship.TryActivateNovaAbility()) {
        PlaySound(mewtwoActivateSound);
        novaReadySoundPlayed = false;

        // Global Clear: Wipe all projectile hazards
        alienLasers.clear();
        for (auto& p : boss.GetProjectiles()) p.active = false;
        for (auto& p : boss2.GetProjectiles()) p.active = false;
        for (auto& g : guards) {
            for (auto& p : g->GetProjectiles()) p.active = false;
        }

        // Visuals: Heavy distortion and flash
        screenFlashTime = 0.5f;
        screenFlashColor = GOLD;
        distortionLevel = 2.0f;
        chromaticAmount = 0.015f;
        TriggerScreenShake(0.8f, 15.0f);

        // Massive particle explosion
        Rectangle shipRect = spaceship.getRect();
        Vector2 center = { shipRect.x + shipRect.width / 2.0f, shipRect.y + shipRect.height / 2.0f };
        for (int i = 0; i < 150; i++) {
            Particle p;
            p.pos = center;
            float ang = (float)GetRandomValue(0, 360) * (PI / 180.0f);
            float speed = (float)GetRandomValue(100, 800);
            p.vel = { cosf(ang) * speed, sinf(ang) * speed };
            p.color = (i % 2 == 0) ? GOLD : WHITE;
            p.life = (float)GetRandomValue(10, 25) / 10.0f;
            p.shape = (i % 3 == 0) ? Particle::RING : Particle::CIRCLE;
            particles.push_back(p);
        }

        // Nova Blast Radius: Destroy nearby normal aliens
        float blastRadius = 350.0f;
        for (auto it = aliens.begin(); it != aliens.end(); ) {
            float alienMidX = it->position.x + Alien::alienImages[it->type - 1].width / 2.0f;
            float alienMidY = it->position.y + Alien::alienImages[it->type - 1].height / 2.0f;
            float dx = alienMidX - center.x;
            float dy = alienMidY - center.y;
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist <= blastRadius) {
                int base = 0;
                if(it->type == 1) base = 100;
                else if(it->type == 2) base = 200;
                else if(it->type == 3) base = 300;
                else if(it->type == 4) base = 500;
                else if(it->type == 5) base = 400;
                score += base * (spaceship.DoubleScoreActive() ? 2 : 1);
                
                nukeExplosions.push_back({it->position, it->type, 0.0f, 0.75f});
                it = aliens.erase(it);
            } else {
                ++it;
            }
        }

        // Destroy Mystery Ship if it is currently on screen
        if (mysteryship.alive) {
            Rectangle mr = mysteryship.getRect();
            Vector2 explPos = { mr.x + mr.width / 2.0f, mr.y + mr.height / 2.0f };
            for (int i = 0; i < 24; i++) {
                Particle p;
                p.pos = explPos;
                float ang = (float)GetRandomValue(0, 360) * (PI / 180.0f);
                float speed = (float)GetRandomValue(60, 260);
                p.vel = { cosf(ang) * speed, sinf(ang) * speed };
                p.color = PINK;
                p.life = 0.8f;
                p.shape = Particle::RING;
                particles.push_back(p);
            }
            score += 500 * (spaceship.DoubleScoreActive() ? 2 : 1);
            mysteryship.alive = false;
            PlaySound(explosionSound);

            // Guaranteed high-tier power-up drop (Nuke, Life, or Shield Overcharge)
            int highTierTypes[] = { POWERUP_NUKE, POWERUP_LIFE, POWERUP_SHIELD_RECHARGE };
            int chosenType = highTierTypes[GetRandomValue(0, 2)];
            powerups.push_back(PowerUp(chosenType, explPos, true));
        }
        checkForHighscore();

        // Shockwave Damage to Bosses
        if (boss.alive) {
            boss.BreakShield(particles);
            boss.TryDamage(5 * spaceship.GetDamageMultiplier(), particles, center);
        }
        if (boss2.alive) {
            boss2.BreakShield(particles);
            boss2.TryDamage(5 * spaceship.GetDamageMultiplier(), particles, center);
        }
        for (auto& g : guards) {
            if (g->alive) {
                g->BreakShield(particles);
                g->TryDamage(3 * spaceship.GetDamageMultiplier(), particles, center);
            }
        }
    }
}

void Game::SetSpaceshipImage(const std::string& assetPath)
{
    spaceship.SetImage(assetPath);
}
