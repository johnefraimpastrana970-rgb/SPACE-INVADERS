#include "spaceship.hpp"
#include <string>
#include <cmath>

static std::string ResolveAssetPath(const std::string& assetPath)
{
    std::string resolved = std::string(GetDirectoryPath(GetApplicationDirectory())) + "/" + assetPath;
    for (char &c : resolved) {
        if (c == '\\') c = '/';
    }
    return resolved;
}

Spaceship::Spaceship()
{
    image = LoadTexture(ResolveAssetPath("Graphics/pixilart-drawing.png").c_str());
    scale = 0.8f; // Reduced base scale for smaller sprites
    targetWidth = (image.width > 0) ? (float)image.width * scale : 60.0f;
    targetHeight = (image.height > 0) ? (float)image.height * scale : 60.0f;
    position.x = (GetScreenWidth() - image.width * scale) / 2.0f;
    position.y = GetScreenHeight() - image.height * scale - 75; // Raised start position by 20 pixels
    lastFireTime = 0.0;
    baseFireRate = 0.35;
    std::string laserPath = ResolveAssetPath("Sounds/laser.ogg");
    std::string fallbackLaserPath = ResolveAssetPath("Sounds/Laser Sound Effect.mp3");
    laserSound = LoadSound(laserPath.c_str());
    if(laserSound.frameCount == 0 && FileExists(fallbackLaserPath.c_str())) {
        TraceLog(LOG_WARNING, "Primary spaceship laser sound failed, falling back to: %s", fallbackLaserPath.c_str());
        laserSound = LoadSound(fallbackLaserPath.c_str());
    }
    if(laserSound.frameCount == 0) {
        TraceLog(LOG_WARNING, "Failed to load spaceship laser sound: %s", laserPath.c_str());
    }
    rapidUntil = 0.0;
    doubleScoreUntil = 0.0;
    shieldUntil = 0.0;
    deflectGlintTimer = 0.0;
    isShieldOvercharged = false;
    lastHitTime = -1.0;
    invincibleUntil = 0.0;
    lastDashTime = 0.0;
    rotation = 0.0f;
    mewtwoAbilityCooldown = 0.0;
    mewtwoAbilityActiveUntil = 0.0;
    greenNinjaAbilityCooldown = 0.0;
    greenNinjaAbilityActiveUntil = 0.0;
    palkiaAbilityCooldown = 0.0;
    palkiaAbilityActiveUntil = 0.0;
    dialgaAbilityCooldown = 0.0;
    dialgaAbilityActiveUntil = 0.0;
    novaAbilityCooldown = 0.0;
    novaAbilityActiveUntil = 0.0;
}

const double Spaceship::INVINCIBILITY_DURATION = 2.0;
const double Spaceship::DASH_COOLDOWN = 0.8;
const double Spaceship::MEWTWO_ABILITY_DURATION = 3.0;
const double Spaceship::MEWTWO_ABILITY_COOLDOWN_TIME = 10.0;
const double Spaceship::GREENNINJA_ABILITY_DURATION = 5.0;
const double Spaceship::GREENNINJA_ABILITY_COOLDOWN_TIME = 12.0;
const double Spaceship::PALKIA_ABILITY_DURATION = 0.5;
const double Spaceship::PALKIA_ABILITY_COOLDOWN_TIME = 8.0;
const double Spaceship::DIALGA_ABILITY_DURATION = 4.0;
const double Spaceship::DIALGA_ABILITY_COOLDOWN_TIME = 15.0;
const double Spaceship::NOVA_ABILITY_DURATION = 1.5;
const double Spaceship::NOVA_ABILITY_COOLDOWN_TIME = 20.0;

Spaceship::~Spaceship() {
    UnloadTexture(image);
    UnloadSound(laserSound);
}

void Spaceship::SetImage(const std::string& assetPath)
{
    UnloadTexture(image);
    image = LoadTexture(ResolveAssetPath(assetPath).c_str());
    if (image.width > 0) {
        float scaleX = targetWidth / (float)image.width;
        float scaleY = targetHeight / (float)image.height;
        scale = fminf(scaleX, scaleY); // Use the smaller scale to fit within both dimensions
    } else {
        scale = 1.25f;
    }
}

void Spaceship::Draw(int currentLives) {
    double hitElapsed = GetTime() - lastHitTime;
    
    // Permanent damage effect: ship gets darker as lives decrease
    // Range from 100 (dark) at 1 life to 255 (bright) at 5 lives
    float healthPercent = (float)currentLives / 5.0f;
    unsigned char brightness = (unsigned char)(100 + 155 * healthPercent);
    Color tint = { brightness, brightness, brightness, 255 };
    float alpha = 1.0f;

    if (hitElapsed < 0.6) {
        if ((int)(hitElapsed * 15) % 2 == 0) tint = { 220, 60, 60, 255 };
    }

    // Invincibility effect: fade in and out
    if (IsInvincible()) {
        alpha = 0.5f + 0.5f * sinf(float(GetTime() * 10.0f)); // Oscillate between 0.5 and 1.0
    }

    if (IsMewtwoAbilityActive()) {
        tint = {180, 100, 255, 255}; // Purple tint for Mewtwo ability
    }

    if (IsGreenNinjaAbilityActive()) {
        tint = LIME; // Green tint for Ninja ability
    }

    if (IsPalkiaAbilityActive()) {
        tint = PINK; // Pink tint for Palkia teleport
    }

    if (IsDialgaAbilityActive()) {
        tint = SKYBLUE; // Cyan tint for Dialga's Time Stop
    }

    if (IsNovaAbilityActive()) {
        tint = GOLD; // Golden glow for Nova Blast
    }

    // Apply rotation decay and damaged wobble
    rotation *= 0.9f;
    float currentRotation = rotation;
    if (currentLives <= 2) {
        currentRotation += sinf(float(GetTime() * 15.0f)) * (3 - currentLives) * 3.0f;
    }

    Rectangle source = { 0.0f, 0.0f, (float)image.width, (float)image.height };
    Vector2 center = { position.x + (image.width * scale) / 2.0f, position.y + (image.height * scale) / 2.0f };
    Rectangle dest = { center.x, center.y, image.width * scale, image.height * scale };
    Vector2 origin = { (image.width * scale) / 2.0f, (image.height * scale) / 2.0f };
    
    DrawTexturePro(image, source, dest, origin, currentRotation, Fade(tint, alpha));

    if(HasShield()){
        // Use deep BLUE for overcharged shield, SKYBLUE for standard
        Color shieldColor = isShieldOvercharged ? BLUE : SKYBLUE;
        Color halo = Fade(shieldColor, 0.45f);
        
        float radius = (image.width * scale > image.height * scale ? image.width * scale : image.height * scale) * 0.5f;

        if (isShieldOvercharged) {
            // Pulse effect: overcharged shield is larger and expands/contracts rapidly
            radius *= (1.15f + 0.07f * sinf((float)GetTime() * 12.0f));
            // Add a secondary outer ring for visual depth
            DrawCircleLines(int(center.x), int(center.y), radius + 5.0f, Fade(shieldColor, 0.3f));
        }

        DrawCircleV(center, radius, halo);
        DrawCircleLines(int(center.x), int(center.y), radius, shieldColor);
    }

    // Draw visual glint/shine effect upon successful deflection
    double now = GetTime();
    if (now < deflectGlintTimer) {
        float progress = (float)(deflectGlintTimer - now) / 0.2f; // Fade progress (1.0 to 0.0)
        float glintRadius = (dest.width > dest.height ? dest.width : dest.height) * 0.5f;
        DrawCircleV(center, glintRadius * (1.1f + (1.0f - progress) * 0.4f), Fade(WHITE, progress * 0.7f));
    }
}

void Spaceship::MoveLeft(int currentLives) {
    // Move slower as lives decrease (from speed 7 at 5 lives to speed 3 at 1 life)
    float speed = 3.0f + 4.0f * (float(currentLives - 1) / 4.0f);
    if (IsGreenNinjaAbilityActive()) speed *= 1.5f; // Ninja speed boost

    position.x -= speed;
    rotation = -12.0f; // Tilt toward movement

    if(position.x < 25) {
        position.x = 25;
    }
}

void Spaceship::MoveRight(int currentLives) {
    float speed = (3.0f + 4.0f * (float(currentLives - 1) / 4.0f)) * speedMultiplier;
    if (IsGreenNinjaAbilityActive()) speed *= 1.5f; // Ninja speed boost

    position.x += speed;
    rotation = 12.0f; // Tilt toward movement

    if(position.x > GetScreenWidth() - image.width * scale - 25) {
        position.x = GetScreenWidth() - image.width * scale - 25;
    }
}

void Spaceship::WarpDash(int direction, std::vector<Particle>& particles) {
    double now = GetTime();
    if (now - lastDashTime < DASH_COOLDOWN) return;

    float dashDistance = 120.0f;
    float oldX = position.x;
    position.x += (direction * dashDistance);

    // Stay within bounds
    if (position.x < 25) position.x = 25;
    if (position.x > GetScreenWidth() - image.width * scale - 25) 
        position.x = GetScreenWidth() - image.width * scale - 25;

    lastDashTime = now;
    invincibleUntil = now + 0.25; // Brief I-frames during dash

    // Visual Effect: Trail of particles from old position to new
    for(int i = 0; i < 15; i++) {
        Particle p;
        p.pos = { oldX + (image.width * scale / 2) + (i * (position.x - oldX) / 15), position.y + (image.height * scale / 2) };
        p.vel = { 0, (float)GetRandomValue(-20, 20) };
        p.color = SKYBLUE;
        p.life = 0.4f;
        p.shape = Particle::RING;
        particles.push_back(p);
    }
}

bool Spaceship::FireLaser(std::vector<Particle>& particles)
{
    double now = GetTime();
    bool rapid = RapidActive(); // Rapid fire power-up
    double interval = rapid ? 0.06 : baseFireRate;

    // Increase firing rate significantly while Dialga's Time Stop is active
    if (IsDialgaAbilityActive()) {
        interval *= 0.4; // 2.5x faster firing rate
    }

    if(now - lastFireTime >= interval * fireRateMultiplier) { // Apply galaxy event fire rate multiplier
        // Select laser/particle color based on active power-ups
        Color shootColor = { 243, 216, 63, 255 }; // Default yellow
        float shootWidth = 4.0f;
        if (rapid) {
            shootColor = ORANGE;
        } else if (DoubleScoreActive()) {
            shootColor = GOLD;
            shootWidth = 8.0f;
        } else if (HasShield()) {
            shootColor = SKYBLUE;
        } else if (IsDialgaAbilityActive()) {
            shootColor = SKYBLUE; // Unique blue lasers during time stop
        }

        Vector2 muzzlePos = {position.x + (image.width * scale) / 2.0f, position.y};
        lasers.push_back(Laser({muzzlePos.x - (shootWidth / 2.0f), muzzlePos.y}, -6, shootColor, shootWidth));
        
        if (IsGreenNinjaAbilityActive()) {
            // Ninja firing logic: Triple parallel lasers
            lasers.push_back(Laser({muzzlePos.x - 15.0f - (shootWidth / 2.0f), muzzlePos.y + 5}, -6, shootColor, shootWidth));
            lasers.push_back(Laser({muzzlePos.x + 15.0f - (shootWidth / 2.0f), muzzlePos.y + 5}, -6, shootColor, shootWidth));
        }

        lastFireTime = now;
        PlaySound(laserSound);

        // Visual effect: Muzzle flash sparks
        for(int i = 0; i < 6; i++) {
            Particle p;
            p.pos = muzzlePos;
            // Spawn particles in an upward arc (240 to 300 degrees)
            float ang = (float)GetRandomValue(240, 300) * (3.14159f / 180.0f);
            float speed = (float)GetRandomValue(120, 280);
            p.vel = { cosf(ang) * speed, sinf(ang) * speed };
            p.color = shootColor;
            p.life = (float)GetRandomValue(10, 30) / 100.0f; // Short life: 0.1s - 0.3s
            p.shape = Particle::TRIANGLE;
            particles.push_back(p);
        }
        return true;
    }
    return false;
}

Rectangle Spaceship::getRect()
{
    float visualWidth = (float)image.width * scale;
    float visualHeight = (float)image.height * scale;

    // Adjust these factors to shrink the collision area relative to the sprite.
    // For example, 0.7f means the hitbox is 70% of the sprite's visual width.
    float widthFactor = 0.7f;
    float heightFactor = 0.8f;

    float hitBoxWidth = visualWidth * widthFactor;
    float hitBoxHeight = visualHeight * heightFactor;

    return { 
        position.x + (visualWidth - hitBoxWidth) / 2.0f, 
        position.y + (visualHeight - hitBoxHeight) / 2.0f, 
        hitBoxWidth, 
        hitBoxHeight 
    };
}

void Spaceship::Reset()
{
    position.x = (GetScreenWidth() - image.width * scale) / 2.0f;
    position.y = GetScreenHeight() - image.height * scale - 75; // Keep ship higher after reset as well
    lasers.clear();
    rapidUntil = 0.0;
    doubleScoreUntil = 0.0;
    shieldUntil = 0.0;
    invincibleUntil = 0.0;
    deflectGlintTimer = 0.0;
    isShieldOvercharged = false;
    lastHitTime = -1.0;
    rotation = 0.0f;
    palkiaAbilityCooldown = 0.0;
    palkiaAbilityActiveUntil = 0.0;
    dialgaAbilityCooldown = 0.0;
    dialgaAbilityActiveUntil = 0.0;
    novaAbilityCooldown = 0.0;
    novaAbilityActiveUntil = 0.0;
}

void Spaceship::ClearShield() {
    shieldUntil = 0.0;
    isShieldOvercharged = false;
}

void Spaceship::ApplyPowerUp(int type)
{
    double now = GetTime();
    const double duration = 8.0; // seconds
    switch(type) {
        case 1: // rapid
            rapidUntil = now + duration;
            break;
        case 2: // double score
            doubleScoreUntil = now + duration;
            break;
        case 3: // shield
            shieldUntil = now + duration;
            isShieldOvercharged = false;
            break;
        case 6: // shield recharge (heavy overcharge)
            shieldUntil = now + 15.0;
            isShieldOvercharged = true;
            break;
        case 4: // nuke handled by Game
        default:
            break;
    }
}

bool Spaceship::HasShield() const
{
    return GetTime() <= shieldUntil;
}

bool Spaceship::RapidActive()
{
    return GetTime() <= rapidUntil;
}

bool Spaceship::DoubleScoreActive()
{
    return GetTime() <= doubleScoreUntil;
}

double Spaceship::RapidRemaining()
{
    double r = rapidUntil - GetTime();
    return r > 0.0 ? r : 0.0;
}

double Spaceship::DoubleRemaining()
{
    double r = doubleScoreUntil - GetTime();
    return r > 0.0 ? r : 0.0;
}

double Spaceship::ShieldRemaining()
{
    double r = shieldUntil - GetTime();
    return r > 0.0 ? r : 0.0;
}

void Spaceship::TriggerHit() {
    lastHitTime = GetTime();
    invincibleUntil = GetTime() + INVINCIBILITY_DURATION;
}

bool Spaceship::IsInvincible() const {
    return GetTime() < invincibleUntil;
}

void Spaceship::TriggerDeflectGlint() {
    deflectGlintTimer = GetTime() + 0.2; // 200ms glint duration
}

bool Spaceship::IsShieldOvercharged() const {
    return isShieldOvercharged && HasShield();
}

float Spaceship::GetDashProgress() const {
    float elapsed = (float)(GetTime() - lastDashTime);
    float progress = elapsed / (float)DASH_COOLDOWN;
    if (progress > 1.0f) progress = 1.0f;
    return progress;
}

double Spaceship::GetMewtwoAbilityCooldownRemaining() const {
    double now = GetTime();
    if (now < mewtwoAbilityCooldown) {
        return mewtwoAbilityCooldown - now;
    }
    return 0.0;
}

bool Spaceship::TryActivateMewtwoAbility() {
    double now = GetTime();
    if (now >= mewtwoAbilityCooldown) {
        mewtwoAbilityActiveUntil = now + MEWTWO_ABILITY_DURATION;
        invincibleUntil = mewtwoAbilityActiveUntil;
        mewtwoAbilityCooldown = now + MEWTWO_ABILITY_COOLDOWN_TIME;
        return true;
    }
    return false;
}

bool Spaceship::IsMewtwoAbilityActive() const {
    return GetTime() < mewtwoAbilityActiveUntil;
}

double Spaceship::GetMewtwoAbilityActiveRemaining() const {
    double now = GetTime();
    if (now < mewtwoAbilityActiveUntil) {
        return mewtwoAbilityActiveUntil - now;
    }
    return 0.0;
}

bool Spaceship::TryActivateGreenNinjaAbility() {
    double now = GetTime();
    if (now >= greenNinjaAbilityCooldown) {
        greenNinjaAbilityActiveUntil = now + GREENNINJA_ABILITY_DURATION;
        greenNinjaAbilityCooldown = now + GREENNINJA_ABILITY_COOLDOWN_TIME;
        return true;
    }
    return false;
}

bool Spaceship::IsGreenNinjaAbilityActive() const {
    return GetTime() < greenNinjaAbilityActiveUntil;
}

double Spaceship::GetGreenNinjaAbilityCooldownRemaining() const {
    double now = GetTime();
    return (now < greenNinjaAbilityCooldown) ? (greenNinjaAbilityCooldown - now) : 0.0;
}

double Spaceship::GetGreenNinjaAbilityActiveRemaining() const {
    double now = GetTime();
    return (now < greenNinjaAbilityActiveUntil) ? (greenNinjaAbilityActiveUntil - now) : 0.0;
}

bool Spaceship::TryActivatePalkiaAbility() {
    double now = GetTime();
    if (now >= palkiaAbilityCooldown) {
        // Teleport to random horizontal location within boundaries
        float minX = 25.0f;
        float maxX = (float)GetScreenWidth() - (image.width * scale) - 25.0f;
        position.x = (float)GetRandomValue((int)minX, (int)maxX);
        
        palkiaAbilityActiveUntil = now + PALKIA_ABILITY_DURATION;
        invincibleUntil = now + 0.5; // Brief protection during/after teleport
        palkiaAbilityCooldown = now + PALKIA_ABILITY_COOLDOWN_TIME;
        return true;
    }
    return false;
}

bool Spaceship::IsPalkiaAbilityActive() const {
    return GetTime() < palkiaAbilityActiveUntil;
}

double Spaceship::GetPalkiaAbilityCooldownRemaining() const {
    double now = GetTime();
    return (now < palkiaAbilityCooldown) ? (palkiaAbilityCooldown - now) : 0.0;
}

double Spaceship::GetPalkiaAbilityActiveRemaining() const {
    double now = GetTime();
    return (now < palkiaAbilityActiveUntil) ? (palkiaAbilityActiveUntil - now) : 0.0;
}

bool Spaceship::TryActivateDialgaAbility() {
    double now = GetTime();
    if (now >= dialgaAbilityCooldown) {
        dialgaAbilityActiveUntil = now + DIALGA_ABILITY_DURATION;
        dialgaAbilityCooldown = now + DIALGA_ABILITY_COOLDOWN_TIME;
        return true;
    }
    return false;
}

bool Spaceship::IsDialgaAbilityActive() const {
    return GetTime() < dialgaAbilityActiveUntil;
}

double Spaceship::GetDialgaAbilityCooldownRemaining() const {
    double now = GetTime();
    return (now < dialgaAbilityCooldown) ? (dialgaAbilityCooldown - now) : 0.0;
}

double Spaceship::GetDialgaAbilityActiveRemaining() const {
    double now = GetTime();
    return (now < dialgaAbilityActiveUntil) ? (dialgaAbilityActiveUntil - now) : 0.0;
}

bool Spaceship::TryActivateNovaAbility() {
    double now = GetTime();
    if (now >= novaAbilityCooldown) {
        novaAbilityActiveUntil = now + NOVA_ABILITY_DURATION;
        invincibleUntil = now + 1.2; // Extra protection
        novaAbilityCooldown = now + NOVA_ABILITY_COOLDOWN_TIME;
        return true;
    }
    return false;
}

bool Spaceship::IsNovaAbilityActive() const {
    return GetTime() < novaAbilityActiveUntil;
}

double Spaceship::GetNovaAbilityCooldownRemaining() const {
    double now = GetTime();
    return (now < novaAbilityCooldown) ? (novaAbilityCooldown - now) : 0.0;
}

double Spaceship::GetNovaAbilityActiveRemaining() const {
    double now = GetTime();
    return (now < novaAbilityActiveUntil) ? (novaAbilityActiveUntil - now) : 0.0;
}
