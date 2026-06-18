#pragma once
#include "spaceship.hpp"
#include "star.hpp"
#include "obstacle.hpp"
#include "alien.hpp"
#include "mysteryship.hpp"
#include "powerup.hpp"
#include "boss.hpp"
#include <string>
#include <vector>
#include "particle.hpp"

class Game {
    public:
        Game();
        ~Game();
        void Draw(Font uiFont);
        void Update();
        void HandleInput();
        Vector2 GetScreenShakeOffset() const;
        float GetDistortionLevel() const { return distortionLevel; }
        float GetChromaticAmount() const { return chromaticAmount; }
        Vector2 GetDistortionCenter() const { return distortionCenter; }
        void TriggerScreenShake(float duration, float strength);
        bool IsStageTransitionActive() const { return !stageMessageShown; }
        bool run;
        bool gameOverActive;
        double gameOverEndTime;
        int lives;
        int score;
        int highscore;
        Music music;
        Music GameOverMusic;
        void Reset(bool fullReset = true);
        void InitGame();
        // Replace boss2 sprite at runtime (path relative to project)
        void SpawnConfetti();
        void DrawBuffIcons(float startX, float y, Font font, bool drawTooltips);
        void DrawCharacterStats(Font font);
        void SetBoss2Sprite(const std::string& assetPath);
        void SetSpaceshipImage(const std::string& assetPath);
        void SetSelectedCharacterIndex(int idx) { selectedCharacterIndex = idx; }
        int GetSelectedCharacterIndex() const { return selectedCharacterIndex; }
        bool IsBossNarrativeActive() const { return bossNarrativeActive; }
        int GetCurrentStage() const { return currentStage; }
        bool IsBossActive() const;
    private:
        void DeleteInactiveLasers();
        std::vector<Obstacle> CreateObstacles();
        std::vector<Alien> CreateAliens(int stage);
        void SpawnAliensForCurrentStage();
        void MoveAliens();
        void MoveDownAliens(int distance); 
        void AlienShootLaser();
        void SpawnKamikazeExplosion(Vector2 position, std::vector<Particle>& particles);
        void CheckForCollisions();
        void GameOver();
        bool AreAllNormalEnemiesCleared() const;
        void ActivateMewtwoAbility();
        void ActivateGreenNinjaAbility();
        void ActivatePalkiaAbility();
        void ActivateDialgaAbility();
        void ActivateNovaAbility();
        void checkForHighscore();
        void InitStars();
        void saveHighscoreToFile(int highscore);
        int loadHighscoreFromFile();
        Spaceship spaceship;
        std::vector<Obstacle> obstacles;
        std::vector<Alien> aliens;
        std::vector<PowerUp> powerups;
        Boss boss;
        Boss boss2;
        std::vector<std::unique_ptr<Boss>> guards;
        bool bossSpawned = false;
        bool boss2Spawned = false;
        int currentStage = 1;
        double stageStartTime = 0.0;
        std::vector<Star> stars;
        bool stageMessageShown = false;
        std::vector<Particle> particles;
        std::string pickupLabel;
        double pickupLabelUntil = 0.0;
        int aliensDirection;
        std::vector<Laser> alienLasers;
        constexpr static float alienLaserShootInterval = 0.35;
        float timeLastAlienFired;
        MysteryShip mysteryship;
        float mysteryShipSpawnInterval;
        float timeLastSpawn;
        struct NukeExplosion { Vector2 pos; int type; float elapsed; float duration; };
        std::vector<NukeExplosion> nukeExplosions;
        float screenShakeTime = 0.0f;
        float screenShakeDuration = 0.0f;
        float screenShakeStrength = 0.0f;
        float distortionLevel = 0.0f;
        float chromaticAmount = 0.0f;
        Vector2 distortionCenter = { 0.5f, 0.5f };
        float screenFlashTime = 0.0f;
        Color screenFlashColor = PURPLE;
        Sound fadeSound; // New sound for fade transition
        Sound explosionSound;
        Sound pickupSound;
        Sound bossAttackSound;
        Sound bossBeamSound;
        Sound blipSound;
        Sound mewtwoActivateSound;
        Sound mewtwoReadySound;
        Sound deflectSound;
        Sound bossBannerSound; // New sound for boss banner appearance
        float musicVolume = 1.0f;
        float slowMoTimer = 0.0f;
        bool bossNarrativeActive = false;
        bool skipNarrativeTriggered = false;
        std::string bossNarrativeText;
        float bossNarrativeCharCount = 0.0f;
        bool aliensSpawnedForCurrentStage = false;
        bool bossBannerActive = false;
        Color bossBannerColor = RED;
        float bossBannerTimer = 0.0f;
        bool fadeToBlackActive = false;
        float fadeToBlackTimer = 0.0f;
        bool bossTransitionPending = false;
        float bossTransitionTimer = 0.0f;
        const float bossTransitionDelay = 1.2f; // Delay before boss fade starts after clearing aliens
        const float fadeToBlackDuration = 1.0f; // Duration of the fade to black effect
        const float bossBannerDuration = 1.5f; // How long the banner stays fully visible
        const float bossBannerFadeDuration = 0.75f; // How long it takes to fade in/out
        int selectedCharacterIndex = 0;
        bool mewtwoReadySoundPlayed = false;
        bool greenNinjaReadySoundPlayed = false;
        bool novaReadySoundPlayed = false;
};