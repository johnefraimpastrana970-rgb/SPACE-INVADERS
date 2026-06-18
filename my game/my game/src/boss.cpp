#include "boss.hpp"
#include "background_restore.hpp"
#include <raylib.h>
#include <cmath>
#include "utils.hpp"
#include "assets.hpp"

static float RandomRange(float min, float max) {
    return min + (max - min) * GetRandomValue(0, 1000) / 1000.0f;
}

Boss::Boss()
{
    LoadFromPath(Paths::Graphics::Boss);
}

void Boss::SetVariant(int v)
{
    variant = v;
}

void Boss::SetScale(float scale)
{
    if (scale <= 0.0f) return;
    drawSize.x *= scale;
    drawSize.y *= scale;
    // enforce a sensible minimum size to avoid invisible guards
    if (drawSize.x < 16.0f) drawSize.x = 16.0f;
    if (drawSize.y < 16.0f) drawSize.y = 16.0f;
}

void Boss::LoadFromPath(const std::string& assetPath)
{
    // Automatically unloads textures via unique_ptr deleters
    stream.Unload();
    hasImage = false;
    currentFrame = 0;
    frameTimer = 0.0f;

    // Use the streaming loader
    stream = Utils::LoadGifStream(assetPath, 0, 0, true, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    
    if(stream.IsValid() && !stream.frames.empty()) {
        Image& firstFrame = stream.frames[0];
        TraceLog(LOG_INFO, "Boss sprite loaded (stream): %s (frames=%d)", assetPath.c_str(), (int)stream.frames.size());

        float maxWidth = 260.0f;
        float maxHeight = 160.0f;
        float scaleX = maxWidth / firstFrame.width;
        float scaleY = maxHeight / firstFrame.height;
        float scale = (scaleX < scaleY) ? scaleX : scaleY;
        
        if(scale < 0.5f) scale = 0.5f;
        if(scale > 2.0f) scale = 2.0f;
        
        drawSize.x = firstFrame.width * scale;
        drawSize.y = firstFrame.height * scale;
        hasImage = true;
        return;
    }

    // try single image (png/jpg) if animation load failed
    Image img = LoadImage(assetPath.c_str());
    if(img.data != nullptr) {
        TraceLog(LOG_INFO, "Boss sprite loaded (image): %s (w=%d h=%d)", assetPath.c_str(), img.width, img.height);
        
        stream.texture = LoadTextureFromImage(img);
        stream.frames.push_back(img); // Single frame stream

        currentFrame = 0;
        frameTimer = 0.0f;

        float maxWidth = 260.0f;
        float maxHeight = 160.0f;
        float scaleX = maxWidth / img.width;
        float scaleY = maxHeight / img.height;
        float scale = (scaleX < scaleY) ? scaleX : scaleY;
        if(scale < 0.5f) scale = 0.5f;
        if(scale > 2.0f) scale = 2.0f;
        drawSize.x = img.width * scale;
        drawSize.y = img.height * scale;
        hasImage = true;
        return;
    }

    TraceLog(LOG_WARNING, "Boss sprite failed to load: %s", assetPath.c_str());

    // Attempt fallback to bundled default boss animation/image to avoid missing sprite issues
    if (assetPath != Paths::Graphics::Boss && assetPath != Paths::Graphics::Boss2) {
        TraceLog(LOG_INFO, "Attempting fallback boss asset: %s", Paths::Graphics::Boss.c_str());
        Utils::GifStream fallback = Utils::LoadGifStream(Paths::Graphics::Boss, 0, 0, true, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        if (fallback.IsValid() && !fallback.frames.empty()) {
            stream = std::move(fallback);
            Image &firstFrame = stream.frames[0];
            float maxWidth = 260.0f;
            float maxHeight = 160.0f;
            float scaleX = maxWidth / firstFrame.width;
            float scaleY = maxHeight / firstFrame.height;
            float scale = (scaleX < scaleY) ? scaleX : scaleY;
            if(scale < 0.5f) scale = 0.5f;
            if(scale > 2.0f) scale = 2.0f;
            drawSize.x = firstFrame.width * scale;
            drawSize.y = firstFrame.height * scale;
            hasImage = true;
            TraceLog(LOG_INFO, "Fallback boss sprite loaded: %s", Paths::Graphics::Boss.c_str());
            return;
        }
        // try single image fallback
        Image img2 = LoadImage(Paths::Graphics::Boss.c_str());
        if (img2.data != nullptr) {
            TraceLog(LOG_INFO, "Fallback boss image loaded (image): %s (w=%d h=%d)", Paths::Graphics::Boss.c_str(), img2.width, img2.height);
            stream.texture = LoadTextureFromImage(img2);
            stream.frames.push_back(img2);
            float maxWidth = 260.0f;
            float maxHeight = 160.0f;
            float scaleX = maxWidth / img2.width;
            float scaleY = maxHeight / img2.height;
            float scale = (scaleX < scaleY) ? scaleX : scaleY;
            if(scale < 0.5f) scale = 0.5f;
            if(scale > 2.0f) scale = 2.0f;
            drawSize.x = img2.width * scale;
            drawSize.y = img2.height * scale;
            hasImage = true;
            TraceLog(LOG_INFO, "Fallback boss image loaded (single frame): %s", Paths::Graphics::Boss.c_str());
            return;
        }
    }
}

Boss::~Boss()
{
    stream.Unload();
}

void Boss::Spawn(int stage)
{
    alive = true;
    hasSummoned30 = false;
    spawnGuardsTrigger = false;
    isGuarded = false;
    hasHealed40 = false;
    isInvincible = false;
    nextMeteorTime = 0.0f;
    nextDarkBallTime = 0.0f;
    nextTripleTime = 0.0f;
    nextBarrageTime = 0.0f;
    nextPurpleTime = 0.0f;
    barrageActive = false;
    divineFireImpacted = false;
    charging = false;
    phaseShiftJustTriggered = false;

    // Boss health scales up with each level to keep the challenge "thicker"
    if (variant == 3) { // Arceus
        maxHealth = 90 + (stage - 5) * 20; // Phase 1: Squishier start for the final boss
        nextMeteorTime = GetTime() + 2.0f;
        nextDarkBallTime = GetTime() + 4.0f;
    } else if (variant == 4) { // Elite Guards
        maxHealth = 15 + (stage - 5) * 5;
    } else { // Standard Bosses (Variant 1 & 2)
        maxHealth = 20 + (stage - 1) * 20; // Increases by 20 health per stage (20, 40, 60, 80...)
    }
    health = maxHealth;
    shieldActive = false;
    shieldUsed = false;
    phaseFlashTimer = 0.0f;
    shieldHits = 0;
    shieldUntil = 0.0f;
    nextCircleTime = GetTime() + RandomRange(2.0f, 4.0f);
    nextBeamTime = GetTime() + RandomRange(5.0f, 8.0f);
    beam.active = false;
    beam.telegraph = false;
    projectiles.clear();

    if (variant == 4) {
        maxShieldHealth = 8.0f + (stage - 5) * 3.0f; // Shields get thicker too
        shieldHealth = maxShieldHealth;
        lastShieldHitTime = 0.0f;
    }

    if(hasImage) {
        spawnX = GetScreenWidth()/2.0f - drawSize.x/2.0f;
    } else {
        spawnX = GetScreenWidth()/2.0f - 60;
    }
    position.x = spawnX;
    position.y = 80;
    spawnY = position.y; // Store default spawn Y

    // initialize variant-specific timers
    if(variant == 2) {
        nextTripleTime = GetTime() + RandomRange(1.0f, 3.0f);
        nextBarrageTime = GetTime() + RandomRange(5.0f, 9.0f);
        nextPurpleTime = GetTime() + RandomRange(6.0f, 12.0f);
        barrageActive = false;
        charging = false;
    }
}

void Boss::ActivateShield()
{
    shieldActive = true;
    shieldUsed = true;
    shieldHits = 0;
    phaseFlashTimer = 0.6f;
    shieldUntil = GetTime() + 5.0f;
}

bool Boss::ShieldActive() const
{
    if (variant == 4) return shieldHealth > 0.0f;
    if (variant == 3 && divineJudgmentActive) return true; // Ensure shield is up during barrage
    return shieldActive && GetTime() <= shieldUntil;
}

bool Boss::BeamActive() const
{
    return beam.active;
}

std::vector<BossProjectile>& Boss::GetProjectiles()
{
    return projectiles;
}

BossBeam& Boss::GetBeam()
{
    return beam;
}

void Boss::TryDamage(float amount, std::vector<Particle>& particles, Vector2 impactPoint)
{
    if (isInvincible) return;

    // 40% Health Instant Heal Trigger (Arceus only)
    if (variant == 3 && !hasHealed40 && health <= (maxHealth * 0.4f)) {
        hasHealed40 = true;
        isInvincible = true;
        shieldUntil = GetTime() + 3.0f; // Temporary invincibility while healing
        phaseFlashTimer = 1.0f;
        maxHealth *= 2; // Phase 2: Mid Tanky - doubling his health capacity
        health = maxHealth; 
        phaseShiftJustTriggered = true;
        return;
    }

    // Regenerative shield logic for elite guards
    if (variant == 4 && shieldHealth > 0.0f) {
        shieldHealth -= amount;
        lastShieldHitTime = (float)GetTime();
        shieldHitPoint = impactPoint;
        shieldHitEffectTime = (float)GetTime() + 0.25f;

        // Visual shield impact particles
        for (int i = 0; i < 8; i++) {
            Particle p;
            p.pos = impactPoint;
            float ang = (float)GetRandomValue(0, 360) * (PI / 180.0f);
            float speed = (float)GetRandomValue(50, 150);
            p.vel = { cosf(ang) * speed, sinf(ang) * speed };
            p.color = SKYBLUE;
            p.life = 0.4f;
            p.shape = Particle::RING;
            particles.push_back(p);
        }
        return;
    }

    if(ShieldActive()) {
        shieldHits++;
        shieldHitPoint = impactPoint;
        shieldHitEffectTime = GetTime() + 0.25f;
        if(shieldHits >= 5) {
            shieldActive = false;
            shieldUntil = GetTime();

            // Shield break particle explosion
            Vector2 center = {position.x + drawSize.x / 2.0f, position.y + drawSize.y / 2.0f};
            float radius = drawSize.x * 0.75f;
            for (int i = 0; i < 40; i++) {
                Particle p;
                float ang = (float)GetRandomValue(0, 360) * (PI / 180.0f);
                p.pos = { center.x + cosf(ang) * radius, center.y + sinf(ang) * radius };
                float speed = (float)GetRandomValue(120, 350);
                p.vel = { cosf(ang) * speed, sinf(ang) * speed };
                p.color = SKYBLUE;
                p.life = (float)GetRandomValue(6, 12) / 10.0f;
                p.shape = Particle::RING;
                particles.push_back(p);
            }
        }
        return;
    }

    health -= (int)amount;
    // Brief flash on taking damage
    phaseFlashTimer = 0.12f;
    if(health <= maxHealth / 2 && !ShieldActive() && !shieldUsed) {
        ActivateShield();
    }
}

void Boss::BreakShield(std::vector<Particle>& particles)
{
    if (!ShieldActive()) return;

    // For guards, set rechargeable shield to 0. 
    // For main bosses, deactivate the shield flags and cancel special invincibility phases.
    if (variant == 4) {
        shieldHealth = 0.0f;
    } else {
        shieldActive = false;
        shieldUntil = (float)GetTime();
        if (variant == 3) divineJudgmentActive = false; // Cancel Arceus invincibility/barrage
    }

    // Shield break particle explosion
    Vector2 center = {position.x + drawSize.x / 2.0f, position.y + drawSize.y / 2.0f};
    float radius = drawSize.x * 0.75f;
    for (int i = 0; i < 40; i++) {
        Particle p;
        float ang = (float)GetRandomValue(0, 360) * (PI / 180.0f);
        p.pos = { center.x + cosf(ang) * radius, center.y + sinf(ang) * radius };
        float speed = (float)GetRandomValue(120, 350);
        p.vel = { cosf(ang) * speed, sinf(ang) * speed };
        p.color = SKYBLUE;
        p.life = (float)GetRandomValue(6, 12) / 10.0f;
        p.shape = Particle::RING;
        particles.push_back(p);
    }
}

void Boss::SpawnDarkCircle()
{
    BossProjectile proj;
    proj.pos = {position.x + drawSize.x / 2.0f, position.y + drawSize.y};
    proj.vel = {0.0f, 140.0f};
    proj.radius = 18.0f;
    proj.life = 6.0f;
    proj.active = true;
    proj.type = 0;
    projectiles.push_back(proj);
}

void Boss::SpawnMeteor() {
    BossProjectile proj;
    proj.pos = {position.x + drawSize.x / 2.0f, position.y + 20};
    proj.vel = {(float)GetRandomValue(-40, 40), 180.0f};
    proj.radius = 22.0f;
    proj.life = 6.0f;
    proj.active = true;
    proj.type = 4; // Meteor
    projectiles.push_back(proj);
}

void Boss::SpawnDarkBallTrishot(const Vector2& target) {
    Vector2 start = {position.x + drawSize.x/2.0f, position.y + drawSize.y/2.0f};
    float angToTarget = atan2f(target.y - start.y, target.x - start.x);
    const float spread = 20.0f * (PI/180.0f);
    for(int i = -1; i <= 1; i++) {
        BossProjectile proj;
        proj.pos = start;
        float a = angToTarget + (i * spread);
        proj.vel = { cosf(a) * 250.0f, sinf(a) * 250.0f };
        proj.radius = 12.0f;
        proj.life = 5.0f;
        proj.active = true;
        proj.type = 5; // Dark Ball
        projectiles.push_back(proj);
    }
}

void Boss::SpawnRedBeam(const Vector2& target)
{
    beam.active = true;
    beam.telegraph = true;
    beam.telegraphTime = 1.0f; // 1.0 second warning duration
    beam.start = {position.x + drawSize.x/2.0f, position.y + drawSize.y/2.0f};

    // Calculate direction vector towards the player's current location
    Vector2 direction = {target.x - beam.start.x, target.y - beam.start.y};
    float length = sqrtf(direction.x*direction.x + direction.y*direction.y);

    if(length > 0.001f) {
        direction.x /= length;
        direction.y /= length;
    } else {
        direction = {0.0f, 1.0f}; // Fallback to firing straight down if at the same position
    }

    beam.dir = direction;
    beam.end = {beam.start.x + beam.dir.x * 1200.0f, beam.start.y + beam.dir.y * 1200.0f};
    beam.life = 2.0f;
    beam.width = 8.0f;
    beamJustFired = true;
}

void Boss::UpdateProjectiles(const Vector2& target, std::vector<Particle>& particles)
{
    float dt = GetFrameTime();
    std::vector<BossProjectile> fireDrops; // Collect fire drops to avoid invalidating iterators

    for(auto& proj : projectiles) {
        if(!proj.active) continue;

        // Divine Judgment homing logic
        if (proj.type == 6) {
            Vector2 toPlayer = { target.x - proj.pos.x, target.y - proj.pos.y };
            float dist = sqrtf(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);
            
            if (dist > 1.0f) {
                toPlayer.x /= dist; 
                toPlayer.y /= dist;
                
                float currentSpeed = sqrtf(proj.vel.x * proj.vel.x + proj.vel.y * proj.vel.y);
                float turnStrength = 180.0f * dt; // Adjust this value for stronger/weaker homing
                
                proj.vel.x += toPlayer.x * turnStrength;
                proj.vel.y += toPlayer.y * turnStrength;
                
                // Normalize and restore speed to ensure they don't accelerate infinitely
                float newSpeed = sqrtf(proj.vel.x * proj.vel.x + proj.vel.y * proj.vel.y);
                proj.vel.x = (proj.vel.x / newSpeed) * currentSpeed;
                proj.vel.y = (proj.vel.y / newSpeed) * currentSpeed;
            }

            // Add glowing golden trail particles
            if (GetRandomValue(0, 2) == 0) { // Spawn chance for optimization
                Particle p;
                p.pos = proj.pos;
                p.vel = { proj.vel.x * -0.15f + GetRandomValue(-10, 10), proj.vel.y * -0.15f + GetRandomValue(-10, 10) };
                p.color = Fade(GOLD, 0.6f);
                p.life = 0.4f;
                p.shape = (GetRandomValue(0, 1) == 0) ? Particle::RING : Particle::CIRCLE;
                particles.push_back(p);
            }
        } else if (proj.type == 7) {
            // Divine Fire particle trail: Intense glowing embers
            if (GetRandomValue(0, 1) == 0) {
                Particle p;
                p.pos = { proj.pos.x + GetRandomValue(-15, 15), proj.pos.y + GetRandomValue(-15, 15) };
                p.vel = { (float)GetRandomValue(-20, 20), (float)GetRandomValue(-40, -10) }; // Drift upward like embers
                p.color = (GetRandomValue(0, 1) == 0) ? GOLD : ORANGE;
                p.life = RandomRange(0.4f, 0.8f);
                p.shape = (GetRandomValue(0, 1) == 0) ? Particle::TRIANGLE : Particle::CIRCLE;
                particles.push_back(p);
            }
        }

        proj.pos.x += proj.vel.x * dt;
        proj.pos.y += proj.vel.y * dt;
        proj.life -= dt;
        if(proj.life <= 0.0f) {
            proj.active = false;

            // When Divine Judgment projectiles expire, they "bloom" into a lingering fire area
            if (proj.type == 6) {
                BossProjectile fire;
                fire.pos = proj.pos;
                fire.vel = { 0, 0 };
                fire.radius = 24.0f; // Initial radius for the fire hazard
                fire.life = 1.8f;   // Matches the t = life / 1.8f scaling in Draw
                fire.active = true;
                fire.type = 7;
                fireDrops.push_back(fire);
                
                // Flag for screen shake on impact/bloom
                divineFireImpacted = true;
            }
        }
        if(proj.pos.y > GetScreenHeight() + 50 || proj.pos.y < -50 || proj.pos.x < -50 || proj.pos.x > GetScreenWidth() + 50) {
            proj.active = false;
        }
    }

    if (!fireDrops.empty()) {
        projectiles.insert(projectiles.end(), fireDrops.begin(), fireDrops.end());
    }
}

void Boss::UpdateBeam()
{
    if(!beam.active) return;

    float dt = GetFrameTime();
    if (beam.telegraph) {
        beam.telegraphTime -= dt;
        if (beam.telegraphTime <= 0.0f) {
            beam.telegraph = false;
        }
    } else {
        beam.life -= dt;
        if(beam.life <= 0.0f) {
            beam.active = false;
            return;
        }
    }
    beam.start = {position.x + drawSize.x/2.0f, position.y + drawSize.y/2.0f};
    beam.end = {beam.start.x + beam.dir.x * 1200.0f, beam.start.y + beam.dir.y * 1200.0f};
}

void Boss::SpawnDeathSplash()
{
    const int count = 14;
    for(int i = 0; i < count; i++) {
        float angle = (360.0f / count) * i * (3.14159265f / 180.0f);
        BossProjectile proj;
        proj.pos = {position.x + drawSize.x/2.0f, position.y + drawSize.y/2.0f};
        proj.vel = {cosf(angle) * 220.0f, sinf(angle) * 220.0f};
        proj.radius = 8.0f;
        proj.life = 3.0f;
        proj.active = true;
        proj.type = 1;
        projectiles.push_back(proj);
    }
}

void Boss::Update(const Vector2& target, std::vector<Particle>& particles)
{
    if(!alive) {
        UpdateProjectiles(target, particles);
        UpdateBeam();
        return;
    }

    if (phaseFlashTimer > 0) phaseFlashTimer -= GetFrameTime();

    // Reset invincibility when shield expires (for Arceus heal phase)
    if (isInvincible && !ShieldActive()) {
        isInvincible = false;
    }

    if(!beam.active) {
        // Framerate-independent oscillation around the spawn point
        if (variant == 4) { // Specific movement for guards
            // Slowly move in a graceful elliptical pattern
            float oscillationX = sinf((float)GetTime() * 1.2f + spawnX * 0.05f) * 30.0f; // Horizontal oscillation
            float oscillationY = cosf((float)GetTime() * 0.8f + spawnY * 0.05f) * 15.0f; // Vertical oscillation
            position.x = spawnX + oscillationX;
            position.y = spawnY + oscillationY;

            // Ghostly trail effect for guards
            if (GetRandomValue(0, 2) == 0) {
                Particle p;
                p.pos = { position.x + drawSize.x / 2.0f + (float)GetRandomValue(-20, 20), 
                          position.y + drawSize.y / 2.0f + (float)GetRandomValue(-20, 20) };
                p.vel = { (float)GetRandomValue(-10, 10), (float)GetRandomValue(5, 25) }; // Drift slowly downwards
                p.color = Fade(SKYBLUE, 0.4f);
                p.life = RandomRange(0.6f, 1.2f);
                p.shape = (GetRandomValue(0, 1) == 0) ? Particle::RING : Particle::CIRCLE;
                particles.push_back(p);
            }

            // Shield regeneration: Recharges after 3 seconds of no damage
            if (GetTime() - lastShieldHitTime > 3.0f && shieldHealth < maxShieldHealth) {
                shieldHealth += GetFrameTime() * 1.5f; // Recharges slowly over time
                if (shieldHealth > maxShieldHealth) shieldHealth = maxShieldHealth;
            }
        } else { // Default oscillation for main bosses (variant 1, 2, 3)
            float oscillation = sinf((float)GetTime() * 2.0f) * 40.0f;
            position.x = spawnX + oscillation;
        }
        
        // Set movement direction for the beam based on the derivative of the sine wave (cosine)
        float velX = cosf((float)GetTime() * 2.0f);
        if(velX > 0.1f) moveDir = {1.0f, 0.0f};
        else if(velX < -0.1f) moveDir = {-1.0f, 0.0f};
    }

    if(hasImage && stream.frames.size() > 1) {
        frameTimer += GetFrameTime();
        if(frameTimer >= frameTime) {
            frameTimer -= frameTime;
            currentFrame = (currentFrame + 1) % stream.frames.size();
        }
    }

    if(health <= maxHealth / 2 && !ShieldActive() && !shieldUsed) {
        ActivateShield();
    }

    float now = GetTime();
    if(variant == 3) { // Arceus logic
        if (!hasSummoned30 && health <= (maxHealth * 0.3f)) {
            hasSummoned30 = true; // Trigger guard summoning
            spawnGuardsTrigger = true; // Signal Game class to spawn guards
            phaseFlashTimer = 0.8f; // Visual flash
            isGuarded = true; // Boss is now guarded
        }
        if (now >= nextMeteorTime) {
            SpawnMeteor();
            nextMeteorTime = now + RandomRange(4.0f, 6.0f) * attackRateMultiplier;
        }
        if (now >= nextDarkBallTime) {
            SpawnDarkBallTrishot(target);
            nextDarkBallTime = now + RandomRange(3.0f, 5.0f) * attackRateMultiplier;
        }

        // Divine Judgment: Thick yellow barrage every 6 seconds
        if (now >= nextDivineJudgmentTime * attackRateMultiplier && !divineJudgmentActive) {
            divineJudgmentActive = true;
            divineJudgmentUntil = now + 2.2f; // Duration of intense barrage
            nextDivineJudgmentTime = now + 6.0f;
            divineJudgmentTriggered = true;

            // Hype visuals: Radiant golden burst of energy rings
            for (int i = 0; i < 60; i++) {
                Particle p;
                p.pos = { position.x + drawSize.x / 2.0f, position.y + drawSize.y / 2.0f };
                float ang = (float)GetRandomValue(0, 360) * (PI / 180.0f);
                float speed = (float)GetRandomValue(200, 600);
                p.vel = { cosf(ang) * speed, sinf(ang) * speed };
                p.color = GOLD;
                p.life = 1.0f;
                p.shape = (i % 2 == 0) ? Particle::RING : Particle::TRIANGLE;
                particles.push_back(p);
            }
        }

        if (divineJudgmentActive) {
            if (now >= divineJudgmentUntil) {
                divineJudgmentActive = false;
            } else if (now - lastDivineJudgmentShot >= divineJudgmentInterval) {
                lastDivineJudgmentShot = now;
                Vector2 start = { position.x + drawSize.x / 2.0f, position.y + drawSize.y / 2.0f };
                float ang = (float)GetRandomValue(70, 110) * (PI / 180.0f); // Randomized downward spread
                BossProjectile proj;
                proj.pos = start;
                proj.vel = { cosf(ang) * 520.0f, sinf(ang) * 520.0f };
                proj.radius = 16.0f; // Thick powerful bullet
                proj.life = 5.0f;
                proj.active = true;
                proj.type = 6; // Divine Judgment bullet type
                projectiles.push_back(proj);
            }
        }
    } else if(variant == 2) {
        if(now >= nextTripleTime) {
            SpawnTripleBlueBall(target);
            nextTripleTime = now + RandomRange(3.0f, 5.0f) * attackRateMultiplier;
        }
        if(now >= nextBarrageTime) {
            StartBarrage(5.0f, target);
            nextBarrageTime = now + RandomRange(10.0f, 16.0f) * attackRateMultiplier;
        }
        if(now >= nextPurpleTime) {
            StartPurpleCharge(target);
            nextPurpleTime = now + RandomRange(8.0f, 14.0f) * attackRateMultiplier;
        }
    } else if (variant == 4) {
        // Guards fire occasional small blue energy balls aimed at the player
        if (now >= nextCircleTime) {
            Vector2 start = {position.x + drawSize.x / 2.0f, position.y + drawSize.y / 2.0f};
            float angle = atan2f(target.y - start.y, target.x - start.x); // Aim at player
            BossProjectile proj;
            proj.pos = start;
            proj.vel = { cosf(angle) * 180.0f, sinf(angle) * 180.0f };
            proj.radius = 8.0f;
            proj.life = 6.0f;
            proj.active = true;
            proj.type = 3; // small blue energy ball
            projectiles.push_back(proj); // Add projectile
            nextCircleTime = now + RandomRange(3.5f, 6.0f) * attackRateMultiplier;
        }
    } else { // Default (Variant 1)
        if(now >= nextCircleTime) {
            SpawnDarkCircle();
            nextCircleTime = now + RandomRange(3.0f, 5.0f) * attackRateMultiplier;
        }
        if(now >= nextBeamTime) {
            SpawnRedBeam(target);
            nextBeamTime = now + RandomRange(7.0f, 10.0f) * attackRateMultiplier;
        }
    }

    UpdateProjectiles(target, particles);
    UpdateBeam();
    // update variant-specific states
    if(variant == 2) {
        UpdateBarrage(target);
        UpdateCharge(target);
    }

    // Energy Aura Effect: Spawns swirling particles around the boss during the charging state
    if (charging) {
        Vector2 center = { position.x + drawSize.x / 2.0f, position.y + drawSize.y / 2.0f };
        for (int i = 0; i < 2; i++) {
            Particle p;
            float angle = (float)GetRandomValue(0, 360) * (PI / 180.0f);
            float radius = drawSize.x * 0.7f; // Spawns slightly outside the boss's silhouette
            p.pos = { center.x + cosf(angle) * radius, center.y + sinf(angle) * radius };
            
            // Tangential velocity creates the "circling" vortex look
            float swirlSpeed = 250.0f;
            p.vel = { -sinf(angle) * swirlSpeed, cosf(angle) * swirlSpeed };
            
            p.color = (GetRandomValue(0, 1) == 0) ? PURPLE : WHITE;
            p.life = 0.45f;
            p.shape = (GetRandomValue(0, 1) == 0) ? Particle::RING : Particle::TRIANGLE;
            particles.push_back(p);
        }
    }

    // Beam Telegraph Charging Effect: Particles being sucked into the muzzle
    if (beam.active && beam.telegraph) {
        for (int i = 0; i < 3; i++) {
            Particle p;
            float angle = (float)GetRandomValue(0, 360) * (PI / 180.0f);
            float dist = (float)GetRandomValue(30, 70);
            p.pos = { beam.start.x + cosf(angle) * dist, beam.start.y + sinf(angle) * dist };
            
            float inwardSpeed = dist * 5.0f; // Speed proportional to distance for a natural "pull"
            p.vel = { -cosf(angle) * inwardSpeed, -sinf(angle) * inwardSpeed };
            
            p.color = RED;
            p.life = 0.22f;
            p.shape = (i % 2 == 0) ? Particle::RING : Particle::TRIANGLE;
            particles.push_back(p);
        }
    }
}

void Boss::SpawnTripleBlueBall(const Vector2& target)
{
    // spawn 3 small blue projectiles in a spread toward target
    Vector2 start = {position.x + drawSize.x/2.0f, position.y + drawSize.y/2.0f};
    float angToTarget = atan2f(target.y - start.y, target.x - start.x);
    const int count = 3;
    const float spread = 12.0f * (3.14159265f/180.0f);
    for(int i=0;i<count;i++){
        float a = angToTarget + (i - (count-1)/2.0f) * spread;
        BossProjectile proj;
        proj.pos = start;
        float speed = 200.0f;
        proj.vel = { cosf(a)*speed, sinf(a)*speed };
        proj.radius = 10.0f;
        proj.life = 4.0f;
        proj.active = true;
        proj.type = 3; // blue small
        projectiles.push_back(proj);
    }
}

void Boss::StartPurpleCharge(const Vector2& target)
{
    charging = true;
    chargeStart = GetTime();
    chargeTarget = target;
}

void Boss::UpdateCharge(const Vector2& target)
{
    if(!charging) return;
    float now = GetTime();
    float t = (now - chargeStart) / chargeDuration;
    if(t >= 1.0f) {
        // launch big purple ball toward target
        Vector2 start = {position.x + drawSize.x/2.0f, position.y + drawSize.y/2.0f};
        float ang = atan2f(chargeTarget.y - start.y, chargeTarget.x - start.x);
        BossProjectile proj;
        proj.pos = start;
        float speed = 220.0f;
        proj.vel = { cosf(ang)*speed, sinf(ang)*speed };
        proj.radius = 28.0f;
        proj.life = 5.0f;
        proj.active = true;
        proj.type = 2; // big purple
        projectiles.push_back(proj);
        charging = false;
    }
}

void Boss::StartBarrage(float duration, const Vector2& target)
{
    barrageActive = true;
    barrageUntil = GetTime() + duration;
    lastBarrageShot = GetTime() - barrageInterval; // allow immediate shot
}

void Boss::UpdateBarrage(const Vector2& target)
{
    if(!barrageActive) return;
    float now = GetTime();
    if(now >= barrageUntil) {
        barrageActive = false;
        return;
    }
    if(now - lastBarrageShot >= barrageInterval) {
        Vector2 start = {position.x + drawSize.x/2.0f, position.y + drawSize.y/2.0f};
        float ang = atan2f(target.y - start.y, target.x - start.x);
        BossProjectile proj;
        proj.pos = start;
        float speed = 360.0f;
        proj.vel = { cosf(ang)*speed, sinf(ang)*speed };
        proj.radius = 6.0f;
        proj.life = 3.0f;
        proj.active = true;
        proj.type = 1; // death bullet
        projectiles.push_back(proj);
        lastBarrageShot = now;
    }
}

void Boss::Draw()
{
    if(alive) {
        float scaleMult = 1.0f;
        Color tint = WHITE;
        
        if (phaseFlashTimer > 0) {
            // Pulse scale and tint red based on the remaining flash time
            scaleMult = 1.0f + sinf(phaseFlashTimer * 15.0f) * 0.15f;
            tint = Color{ 255, (unsigned char)(255 * (1.0f - phaseFlashTimer)), (unsigned char)(255 * (1.0f - phaseFlashTimer)), 255 };
        }

        // Divine Judgment Hype pulse effect on the boss sprite
        if (divineJudgmentActive) {
            scaleMult = 1.15f + sinf((float)GetTime() * 25.0f) * 0.08f;
            tint = ColorLerp(tint, GOLD, (sinf((float)GetTime() * 15.0f) + 1.0f) * 0.5f);
        }

        // Spectral transparency and flickering for guards (variant 4)
        float guardAlpha = 0.6f;
        if (variant == 4) {
            float shieldPct = (maxShieldHealth > 0.0f) ? (shieldHealth / maxShieldHealth) : 0.0f;
            // Pulse faster as the shield gets closer to breaking
            float flickerSpeed = 35.0f + (1.0f - shieldPct) * 65.0f;
            guardAlpha = 0.45f + 0.2f * sinf((float)GetTime() * flickerSpeed);
            tint = Fade(tint, guardAlpha);
        }

        if(hasImage && stream.IsValid()) {
            stream.UpdateVRAM(currentFrame);
            Rectangle source = {0.0f, 0.0f, float(stream.texture.width), float(stream.texture.height)};
            
            // Draw centered to allow smooth scaling pulses
            Vector2 center = {position.x + drawSize.x/2.0f, position.y + drawSize.y/2.0f};
            Rectangle dest = {center.x, center.y, drawSize.x * scaleMult, drawSize.y * scaleMult};
            Vector2 origin = { (drawSize.x * scaleMult) / 2.0f, (drawSize.y * scaleMult) / 2.0f };

            Shader* activeShader = nullptr;
            
            // If charging, use the silhouette shader with a purple-white flicker
            if (charging) {
                activeShader = g_silhouetteShader;
                float flicker = (sinf((float)GetTime() * 30.0f) + 1.0f) * 0.5f;
                tint = ColorLerp(PURPLE, WHITE, 0.4f + 0.5f * flicker);
            }

            stream.Draw(source, dest, origin, 0.0f, tint, activeShader);
            
            if (phaseFlashTimer > 0.0f) {
                // Overlays a white flash on top of the boss during phase flash
                float flashAlpha = phaseFlashTimer * 0.7f;
                if (variant == 4) flashAlpha *= guardAlpha;
                stream.Draw(source, dest, origin, 0.0f, Fade(WHITE, flashAlpha), nullptr);
            }

            if(ShieldActive()) {
                Color shieldColor = {90, 150, 255, 120};
                if (variant == 4) {
                    float shieldPct = (maxShieldHealth > 0.0f) ? (shieldHealth / maxShieldHealth) : 0.0f;
                    // Fade from blue to red as shield health drops
                    Color healthyColor = {90, 150, 255, 255};
                    Color criticalColor = {255, 50, 50, 255};
                    shieldColor = ColorLerp(criticalColor, healthyColor, shieldPct);
                    shieldColor = Fade(shieldColor, guardAlpha);
                }

                float cx = position.x + drawSize.x/2.0f;
                float cy = position.y + drawSize.y/2.0f;
                float radius = drawSize.x * 0.75f;
                DrawCircleLines(int(cx), int(cy), radius, shieldColor);
                DrawCircle(int(cx), int(cy), int(radius), ColorAlpha(shieldColor, 0.18f));
                // Only draw shield cracks (red lines) if it's NOT the Divine Judgment barrage
                if (variant != 4 && !divineJudgmentActive) for(int i = 0; i < shieldHits; i++) {
                    float angle = float(i) * 360.0f / 5.0f + GetTime() * 40.0f;
                    float rad = angle * (3.14159265f / 180.0f);
                    Vector2 start = {cx + cosf(rad) * radius, cy + sinf(rad) * radius};
                    Vector2 end = {cx + cosf(rad) * (radius * 0.58f), cy + sinf(rad) * (radius * 0.58f)};
                    DrawLineEx(start, end, 4.0f, RED);
                }
            }

            float healthPct = (float)health / maxHealth;
            Color barColor;
            if (isInvincible) barColor = GOLD;
            else if (hasHealed40) barColor = ColorLerp(RED, PURPLE, healthPct); // Phase 2: Intense Purple/Red
            else if (healthPct >= 0.5f) barColor = ColorLerp(YELLOW, LIME, (healthPct - 0.5f) * 2.0f);
            else barColor = ColorLerp(RED, YELLOW, healthPct * 2.0f);

            DrawRectangle(int(position.x), int(position.y - 10), int(drawSize.x * healthPct), 6, (variant == 4 ? Fade(barColor, guardAlpha) : barColor));
        } else {
            DrawRectangleV(position, {120, 60}, (variant == 4 ? Fade(PURPLE, guardAlpha) : PURPLE));
            if(ShieldActive()) {
                Color sCol = SKYBLUE;
                if (variant == 4) sCol = Fade(sCol, guardAlpha);
                else if (variant == 3 && divineJudgmentActive) {
                    float pulse = (sinf((float)GetTime() * 15.0f) + 1.0f) * 0.5f;
                    sCol = ColorLerp(SKYBLUE, GOLD, 0.8f);
                    sCol = Fade(sCol, 0.4f + 0.3f * pulse);
                }
                DrawCircleLines(int(position.x + 60), int(position.y + 30), 70, sCol);
            }

            float healthPct = (float)health / maxHealth;
            Color barColor;
            if (isInvincible) barColor = GOLD;
            else if (hasHealed40) barColor = ColorLerp(RED, PURPLE, healthPct);
            else if (healthPct >= 0.5f) barColor = ColorLerp(YELLOW, LIME, (healthPct - 0.5f) * 2.0f);
            else barColor = ColorLerp(RED, YELLOW, healthPct * 2.0f);

            DrawRectangle(int(position.x), int(position.y - 10), int(120 * healthPct), 6, (variant == 4 ? Fade(barColor, guardAlpha) : barColor));
        }
    }

    for(auto& proj : projectiles) {
        if(!proj.active) continue;
        if(proj.type == 0) {
            DrawCircleV(proj.pos, proj.radius, {20, 20, 40, 200});
            DrawCircleLines(int(proj.pos.x), int(proj.pos.y), int(proj.radius), DARKPURPLE);
        } else if(proj.type == 1) {
            DrawCircleV(proj.pos, proj.radius, RED);
            DrawCircleLines(int(proj.pos.x), int(proj.pos.y), int(proj.radius), ORANGE);
        } else if(proj.type == 2) {
            // big purple charged ball
            DrawCircleV(proj.pos, proj.radius, {60, 10, 80, 220});
            DrawCircleLines(int(proj.pos.x), int(proj.pos.y), int(proj.radius), PURPLE);
        } else if(proj.type == 3) {
            // dark blue small ball
            Color ballCol = {30, 60, 160, 220};
            Color lineCol = DARKBLUE;
            if (variant == 4) {
                float projAlpha = 0.5f + 0.2f * sinf((float)GetTime() * 40.0f);
                ballCol = Fade(ballCol, projAlpha);
                lineCol = Fade(lineCol, projAlpha);
            }
            DrawCircleV(proj.pos, proj.radius, ballCol);
            DrawCircleLines(int(proj.pos.x), int(proj.pos.y), int(proj.radius), lineCol);
        } else if(proj.type == 4) {
            // Meteor
            DrawCircleV(proj.pos, proj.radius, BROWN);
            DrawCircleLines(int(proj.pos.x), int(proj.pos.y), int(proj.radius), BLACK);
        } else if(proj.type == 5) {
            // Dark Ball + Lightning
            DrawCircleV(proj.pos, proj.radius, BLACK);
            DrawCircleLines(int(proj.pos.x), int(proj.pos.y), int(proj.radius), PURPLE);
            if (GetRandomValue(0, 5) == 0) {
                Vector2 start = proj.pos;
                Vector2 end = {proj.pos.x + GetRandomValue(-30, 30), proj.pos.y + GetRandomValue(-30, 30)};
                DrawLineEx(start, end, 2.0f, SKYBLUE);
            }
        } else if(proj.type == 6) {
            // Divine Judgment Bullet: Thick yellow with white "hot" core
            DrawCircleV(proj.pos, proj.radius, YELLOW);
            DrawCircleLines(int(proj.pos.x), int(proj.pos.y), int(proj.radius), GOLD);
            DrawCircleV(proj.pos, proj.radius * 0.5f, WHITE);
        } else if(proj.type == 7) {
            // Divine Fire: Pulsing orange/red/gold lingering area
            float t = proj.life / 1.8f;
            
            // Use a power curve (t^4) to make growth extremely fast at the start (t near 1.0)
            float growth = 0.5f * (1.0f - (t * t * t * t)); 
            
            float pulse = (sinf((float)GetTime() * 12.0f) + 1.0f) * 0.5f;
            Color inner = ColorLerp(ORANGE, GOLD, pulse);
            float currentRadius = proj.radius * (0.9f + 0.2f * pulse + growth);
            DrawCircleV(proj.pos, currentRadius, Fade(inner, t * 0.7f));
            DrawCircleLines((int)proj.pos.x, (int)proj.pos.y, (int)currentRadius, Fade(RED, t));
        }
    }

    if(beam.active) {
        if (beam.telegraph) {
            // Draw thin warning line with a slight flicker
            DrawLineEx(beam.start, beam.end, 1.5f, Fade(RED, 0.4f + 0.3f * sinf(GetTime() * 20.0f)));
        } else {
            DrawLineEx(beam.start, beam.end, beam.width, RED);
        }
        DrawCircleV(beam.start, 8.0f, RED);
    }

    // draw charge buildup for purple ball
    if(charging) {
        float t = (GetTime() - chargeStart) / chargeDuration;
        if(t < 0.0f) t = 0.0f;
        if(t > 1.0f) t = 1.0f;
        float radius = 8.0f + t * 40.0f;
        Vector2 center = { position.x + drawSize.x/2.0f, position.y + drawSize.y/2.0f };
        Color c = {120, 30, 160, 140};
        DrawCircleV(center, radius, c);
        DrawCircleLines(int(center.x), int(center.y), int(radius), PURPLE);
    }
}

bool Boss::BeamJustFired() const
{
    return beamJustFired;
}

bool Boss::DivineJudgmentJustFired() const
{
    return divineJudgmentTriggered;
}

bool Boss::IsDivineJudgmentActive() const
{
    return divineJudgmentActive;
}

bool Boss::GetPhaseShiftJustTriggered() const
{
    return phaseShiftJustTriggered;
}

void Boss::ClearPhaseShiftJustTriggered()
{
    phaseShiftJustTriggered = false;
}

float Boss::GetDivineJudgmentProgress() const
{
    if (!divineJudgmentActive) return 0.0f;
    float duration = 2.2f; // Matches the duration set in Update()
    float remaining = divineJudgmentUntil - (float)GetTime();
    return fmaxf(0.0f, fminf(1.0f, 1.0f - (remaining / duration)));
}

float Boss::GetChargeProgress() const
{
    if (!charging) return 0.0f;
    float t = ((float)GetTime() - chargeStart) / chargeDuration;
    return fmaxf(0.0f, fminf(1.0f, t));
}

int Boss::GetMaxHealth() const
{
    return maxHealth;
}

void Boss::ClearDivineJudgmentJustFired()
{
    divineJudgmentTriggered = false;
}

void Boss::ClearBeamJustFired()
{
    beamJustFired = false;
}

bool Boss::GetMeteorImpacted() const
{
    return meteorImpacted;
}

bool Boss::DivineFireJustImpacted() const
{
    return divineFireImpacted;
}

void Boss::ClearDivineFireJustImpacted()
{
    divineFireImpacted = false;
}

void Boss::ClearMeteorImpacted()
{
    meteorImpacted = false;
}

Rectangle Boss::getRect() const
{
    if(hasImage) {
        // Use a tighter hitbox factor to remove the 'rectangle' feel of the GIF canvas
        // This makes collision more accurate to the visual sprite of Roxo and other bosses
        float widthFactor = 0.75f;
        float heightFactor = 0.85f;
        
        float hitBoxWidth = drawSize.x * widthFactor;
        float hitBoxHeight = drawSize.y * heightFactor;
        
        return { 
            position.x + (drawSize.x - hitBoxWidth) / 2.0f, 
            position.y + (drawSize.y - hitBoxHeight) / 2.0f, 
            hitBoxWidth, 
            hitBoxHeight 
        };
    }
    return {position.x, position.y, 120, 60};
}
