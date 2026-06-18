#pragma once
#include <raylib.h>
#include "laser.hpp"
#include "particle.hpp"
#include <vector>
#include <string>

class Spaceship{
    public:
        Spaceship();
        ~Spaceship();
        void Draw(int currentLives);
        void MoveLeft(int currentLives);
        void MoveRight(int currentLives);
        void WarpDash(int direction, std::vector<Particle>& particles);
        bool FireLaser(std::vector<Particle>& particles);
        void ApplyPowerUp(int type);
        bool HasShield() const;
        bool IsShieldOvercharged() const;
        bool RapidActive();
        bool DoubleScoreActive();
        double RapidRemaining();
        float GetDashProgress() const;
        double DoubleRemaining();
        double ShieldRemaining();
        Rectangle getRect();
        bool IsInvincible() const;
        void TriggerHit();
        void Reset();
        bool TryActivateMewtwoAbility();
        bool IsMewtwoAbilityActive() const;
        double GetMewtwoAbilityCooldownRemaining() const;
        double GetMewtwoAbilityActiveRemaining() const;
        bool TryActivateGreenNinjaAbility();
        bool IsGreenNinjaAbilityActive() const;
        bool TryActivatePalkiaAbility();
        bool IsPalkiaAbilityActive() const;
        double GetPalkiaAbilityCooldownRemaining() const;
        double GetPalkiaAbilityActiveRemaining() const;
        bool TryActivateDialgaAbility();
        bool IsDialgaAbilityActive() const;
        bool TryActivateNovaAbility();
        bool IsNovaAbilityActive() const;
        double GetNovaAbilityCooldownRemaining() const;
        double GetNovaAbilityActiveRemaining() const;
        double GetDialgaAbilityCooldownRemaining() const;
        double GetDialgaAbilityActiveRemaining() const;
        double GetGreenNinjaAbilityCooldownRemaining() const;
        double GetGreenNinjaAbilityActiveRemaining() const;
        float GetDamageMultiplier() const { return damageMultiplier; }
        void SetFireRateMultiplier(float multiplier) { fireRateMultiplier = multiplier; }
        void SetDamageMultiplier(float multiplier) { damageMultiplier = multiplier; }
        void SetSpeedMultiplier(float multiplier) { speedMultiplier = multiplier; }
        void SetInvincibleUntil(double time) { invincibleUntil = time; }
        void SetShieldDuration(float duration) { shieldUntil = GetTime() + duration; } // For temporary shields
        void SetImage(const std::string& assetPath);
        void SetBaseFireRate(double rate) { baseFireRate = rate; }
        std::vector<Laser> lasers;
        void TriggerDeflectGlint();
        void ClearShield();

    private:
        Texture2D image;
        float scale;
        float targetWidth;
        float targetHeight;
        Vector2 position;
        float rotation;
        double baseFireRate;
        double lastFireTime;
        Sound laserSound;
        double rapidUntil;
        double doubleScoreUntil;
        double shieldUntil;
        double lastHitTime;
        double lastDashTime;
        double invincibleUntil;
        bool isShieldOvercharged;
        double deflectGlintTimer;
        double mewtwoAbilityCooldown;
        double mewtwoAbilityActiveUntil;
        double greenNinjaAbilityCooldown;
        double greenNinjaAbilityActiveUntil;
        double palkiaAbilityCooldown;
        double palkiaAbilityActiveUntil;
        double dialgaAbilityCooldown;
        double dialgaAbilityActiveUntil;
        double novaAbilityCooldown;
        double novaAbilityActiveUntil;
        float fireRateMultiplier = 1.0f;
        float damageMultiplier = 1.0f;
        float speedMultiplier = 1.0f;
        static const double INVINCIBILITY_DURATION; // seconds
        static const double DASH_COOLDOWN; // seconds
    public:
        static const double MEWTWO_ABILITY_DURATION; // seconds
        static const double MEWTWO_ABILITY_COOLDOWN_TIME; // seconds
        static const double GREENNINJA_ABILITY_DURATION; // seconds
        static const double GREENNINJA_ABILITY_COOLDOWN_TIME; // seconds
        static const double PALKIA_ABILITY_DURATION; // seconds
        static const double PALKIA_ABILITY_COOLDOWN_TIME; // seconds
        static const double DIALGA_ABILITY_DURATION; // seconds
        static const double DIALGA_ABILITY_COOLDOWN_TIME; // seconds
        static const double NOVA_ABILITY_DURATION; // seconds
        static const double NOVA_ABILITY_COOLDOWN_TIME; // seconds
    private:
};