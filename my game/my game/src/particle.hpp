#pragma once
#include <raylib.h>

struct Particle {
    enum Shape { CIRCLE=0, RECT=1, TRIANGLE=2, RING=3 };
    Vector2 pos;
    Vector2 vel;
    Color color;
    float life;
    Shape shape = CIRCLE;

    void Update(float dt){
        vel.y += 200.0f * dt; // gravity
        pos.x += vel.x * dt;
        pos.y += vel.y * dt;
        life -= dt;
    }

    void Draw(){
        if(life <= 0) return;
        float t = life;
        if (t > 1.0f) t = 1.0f; // Stay opaque for life > 1s, then fade out
        Color c = color;
        c.a = (unsigned char)(255 * t);
        switch(shape){
            case CIRCLE:
                DrawCircleV(pos, 3.0f, c);
                break;
            case RECT:
                DrawRectangleV({pos.x-3,pos.y-3}, {6,6}, c);
                break;
            case TRIANGLE: {
                Vector2 p1 = {pos.x, pos.y-4};
                Vector2 p2 = {pos.x-4, pos.y+4};
                Vector2 p3 = {pos.x+4, pos.y+4};
                DrawTriangle(p1,p2,p3,c);
                break; }
            case RING:
                DrawCircleLines(int(pos.x), int(pos.y), 4.0f, c);
                break;
        }
    }
};
