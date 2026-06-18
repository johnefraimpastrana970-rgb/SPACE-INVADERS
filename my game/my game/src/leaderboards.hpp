#pragma once
#include <raylib.h>
#include <string>
#include <vector>

namespace Leaderboards {
    struct Entry {
        std::string name;
        int score;
    };

    void Init(int score, bool isHighScore = false);
    void InitView();
    void Reset();
    void Update();
    void Draw(Font font);
    bool IsSaved();
    bool WantReturn();
    bool IsNewRecord();
}
