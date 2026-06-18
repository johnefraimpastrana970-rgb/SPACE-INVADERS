#pragma once
#include <raylib.h>
#include <vector>
#include <string>
#include <memory>
#include "particle.hpp"
#include "utils.hpp"

struct BossProjectile {
    Vector2 pos;
    Vector2 vel;
    float radius;
    float life;
    bool active;
    int type; // 0 = dark circle, 1 = death bullet
};

struct BossBeam {
    Vector2 start;
    Vector2 end;
    Vector2 dir;
    float life;
    bool active;
    float width;
    bool telegraph;
    float telegraphTime;
};

class Boss {
public:
    Boss();
    ~Boss();
    void LoadFromPath(const std::string& assetPath);
    void SetVariant(int v);
    // Scale draw size by multiplier (1.0 = unchanged)
    void SetScale(float scale);
    void Spawn(int stage = 1);
    void Update(const Vector2& target, std::vector<Particle>& particles);
    void Draw();
    Rectangle getRect() const;
    bool ShieldActive() const;
    bool BeamActive() const;
    bool BeamJustFired() const;
    bool GetMeteorImpacted() const;
    void ClearMeteorImpacted();
    bool DivineFireJustImpacted() const;
    void ClearDivineFireJustImpacted();
    bool DivineJudgmentJustFired() const;
    void ClearDivineJudgmentJustFired();
    bool IsDivineJudgmentActive() const;
    bool GetPhaseShiftJustTriggered() const;
    void ClearPhaseShiftJustTriggered();
    float GetDivineJudgmentProgress() const;
    void ClearBeamJustFired();
    int GetMaxHealth() const;
    void SetAttackRateMultiplier(float multiplier) { attackRateMultiplier = multiplier; }
    float GetChargeProgress() const;
    std::vector<BossProjectile>& GetProjectiles();
    BossBeam& GetBeam();
    void TryDamage(float amount, std::vector<Particle>& particles, Vector2 impactPoint = {0.0f, 0.0f});
    void SpawnDeathSplash();
    void BreakShield(std::vector<Particle>& particles);

    bool isInvincible = false;
    bool spawnGuardsTrigger = false;
    bool isGuarded = false;

    float shieldHealth = 0.0f;
    float maxShieldHealth = 0.0f;
    float lastShieldHitTime = 0.0f;

    bool alive = false;
    int health = 0;
    Vector2 position = {0,0};
    float spawnX = 0.0f;
    float spawnY = 0.0f;
    bool phaseShiftJustTriggered = false;
private:
    void SpawnDarkCircle();
    void SpawnRedBeam(const Vector2& target);
    void ActivateShield();
    void UpdateProjectiles(const Vector2& target, std::vector<Particle>& particles);
    void UpdateBeam();

    Utils::GifStream stream;
    bool hasImage = false;
    int currentFrame = 0;
    float frameTime = 0.12f;
    float frameTimer = 0.0f;
    Vector2 drawSize = {120, 60};
    Vector2 moveDir = {1.0f, 0.0f};
    bool shieldActive = false;
    bool shieldUsed = false;
    int shieldHits = 0;
    float shieldUntil = 0.0f;
    float shieldReadyTime = 0.0f;
    float nextCircleTime = 0.0f;
    float nextBeamTime = 0.0f;
    int maxHealth = 20;
    float phaseFlashTimer = 0.0f;
    float healTimer = 0.0f;
    BossBeam beam;
    bool meteorImpacted = false;
    bool beamJustFired = false;
    std::vector<BossProjectile> projectiles;
    float shieldHitEffectTime = 0.0f;
    Vector2 shieldHitPoint = {0.0f, 0.0f};
    // variant and special ability state
    int variant = 1;
    // barrage state
    bool barrageActive = false;
    float barrageUntil = 0.0f;
    float lastBarrageShot = 0.0f;
    float barrageInterval = 0.12f;
    // purple charge state
    bool charging = false;
    float chargeStart = 0.0f;
    float chargeDuration = 1.5f;
    bool hasHealed40 = false;
    bool hasSummoned30 = false;
    Vector2 chargeTarget = {0.0f, 0.0f};
    // ability timings
    float nextTripleTime = 0.0f;
    float nextBarrageTime = 0.0f;
    float nextPurpleTime = 0.0f;
    float nextMeteorTime = 0.0f;
    float nextDarkBallTime = 0.0f;
    bool divineJudgmentActive = false;
    float divineJudgmentUntil = 0.0f;
    float attackRateMultiplier = 1.0f;
    float lastDivineJudgmentShot = 0.0f;
    float divineJudgmentInterval = 0.08f;
    float nextDivineJudgmentTime = 0.0f;
    bool divineJudgmentTriggered = false;
    bool divineFireImpacted = false;

    void SpawnMeteor();
    void SpawnDarkBallTrishot(const Vector2& target);
    void SpawnTripleBlueBall(const Vector2& target);
    void StartBarrage(float duration, const Vector2& target);
    void UpdateBarrage(const Vector2& target);
    void StartPurpleCharge(const Vector2& target);
    void UpdateCharge(const Vector2& target);
};
