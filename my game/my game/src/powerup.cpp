#include "powerup.hpp"
#include <raylib.h>

void PowerUp::Draw()
{
    Color c = WHITE;
    char label = '?';
    switch(type) {
        case POWERUP_RAPID: c = ORANGE; label = 'R'; break;
        case POWERUP_DOUBLE: c = GOLD; label = 'D'; break;
        case POWERUP_SHIELD: c = SKYBLUE; label = 'S'; break;
        case POWERUP_NUKE: c = RED; label = 'N'; break;
        case POWERUP_LIFE: c = GREEN; label = 'L'; break;
        case POWERUP_SHIELD_RECHARGE: c = BLUE; label = 'C'; break;
    }

    if (isMagnetic) {
        // Create a pulsating glowing aura effect
        float pulse = (sinf((float)GetTime() * 8.0f) + 1.0f) * 0.5f; // Range 0.0 to 1.0
        float auraRadius = 14.0f + (8.0f * pulse);
        DrawCircleV(position, auraRadius, Fade(c, 0.2f + 0.1f * pulse));
        DrawCircleLines((int)position.x, (int)position.y, auraRadius, Fade(c, 0.4f * pulse));
    }

    DrawCircleV(position, 12, c);
    char buf[2] = { label, '\0' };
    DrawText(buf, int(position.x-6), int(position.y-8), 14, WHITE);
}
