#include "leaderboards.hpp"
#include "utils.hpp"
#include <raylib.h>
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>

namespace {
    const int MaxNameChars = 12;
    bool inputMode = false;
    bool nameSaved = false;
    bool wantReturn = false;
    bool isNewRecord = false;
    int pendingScore = 0;
    std::string playerName = "PLAYER";
    std::vector<Leaderboards::Entry> entries = {
        {"AAA", 99999},
        {"BBB", 87500},
        {"CCC", 73000},
    };

    std::vector<Texture2D> leaderboardCatFrames;
    bool leaderboardCatFramesLoaded = false;
    int currentLeaderboardCatFrame = 0;
    float leaderboardCatFrameTimer = 0.0f;
    const float leaderboardCatFrameTime = 0.08f;

    static void LoadLeaderboardCatFrames()
    {
        if (leaderboardCatFramesLoaded) return;
        leaderboardCatFramesLoaded = true;
        leaderboardCatFrames = Utils::LoadGifFrames("Graphics/cat-cats.gif");
    }

    void SavePlayerName()
    {
        std::string enteredName = playerName;
        if (enteredName.empty()) {
            enteredName = "PLAYER";
        }

        entries.insert(entries.begin(), {enteredName, pendingScore});
        // Sort entries by score in descending order
        std::sort(entries.begin(), entries.end(), [](const Leaderboards::Entry& a, const Leaderboards::Entry& b) {
            return a.score > b.score;
        });

        if (entries.size() > 10) {
            entries.pop_back();
        }

        // Persist leaderboards to disk immediately after saving
        std::ofstream out("leaderboards.txt");
        if (out.is_open()) {
            for (const auto &e : entries) {
                out << e.name << '\t' << e.score << '\n';
            }
            out.close();
        }

        nameSaved = true;
    }
}

void Leaderboards::Init(int score, bool isHighScore)
{
    inputMode = true;
    nameSaved = false;
    pendingScore = score;
    playerName = "PLAYER";
    isNewRecord = isHighScore;
}

void Leaderboards::InitView()
{
    inputMode = false;
    nameSaved = false;
    pendingScore = 0;
}

void Leaderboards::Reset()
{
    inputMode = false;
    nameSaved = false;
    pendingScore = 0;
    playerName = "PLAYER";
}

void Leaderboards::Update()
{
    wantReturn = false;

    Rectangle returnButton = { (800.0f - 280.0f) / 2.0f, 650.0f, 280.0f, 50.0f };
    bool buttonClicked = CheckCollisionPointRec(GetMousePosition(), returnButton) && (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON));
    
    if (inputMode && !nameSaved) {
        // Handle backspace - allow deleting all characters
        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (!playerName.empty()) {
                playerName.pop_back();
            }
        }
        // Handle Enter to submit name
        else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            SavePlayerName();
            wantReturn = true; // Set wantReturn after saving
            return; // Exit update after handling input
        }
        // Handle ESC to skip entry (save with current name)
        else if (IsKeyPressed(KEY_ESCAPE)) {
            SavePlayerName();
            wantReturn = true; // Set wantReturn after saving
            return; // Exit update after handling input
        }

        // If button clicked during name entry, save and return
        if (buttonClicked) {
            SavePlayerName();
            wantReturn = true;
            return;
        }

        // Handle character input for name
        int code = GetCharPressed();
        while (code > 0) {
            if (code >= 32 && code <= 126 && playerName.length() < MaxNameChars) {
                playerName += static_cast<char>(code);
            }
            code = GetCharPressed();
        }
    } else { // Not in name entry mode, or name is saved
        // If button clicked, set wantReturn
        if (buttonClicked) {
            wantReturn = true;
            return;
        }
        // Allow returning from leaderboard view with Space, Enter, or ESC
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
            wantReturn = true;
        }
    }
}

void Leaderboards::Draw(Font font)
{
    LoadLeaderboardCatFrames();

    Color gold = {255, 215, 0, 255};
    Color silver = {192, 192, 192, 255};
    Color bronze = {205, 127, 50, 255};
    Color yellow = {243, 216, 63, 255};
    Color white = WHITE;
    Color panelColor = {20, 20, 22, 220};

    // Draw Back Button
    Rectangle returnButton = { (800.0f - 280.0f) / 2.0f, 650.0f, 280.0f, 50.0f };
    bool hovered = CheckCollisionPointRec(GetMousePosition(), returnButton);
    DrawRectangleRec(returnButton, hovered ? Fade(SKYBLUE, 0.4f) : panelColor);
    DrawRectangleLinesEx(returnButton, 2, hovered ? WHITE : yellow);
    
    std::string buttonText = "BACK TO MENU";
    Vector2 textSize = MeasureTextEx(font, buttonText.c_str(), 24, 2);
    DrawTextEx(font, buttonText.c_str(), 
        {returnButton.x + (returnButton.width - textSize.x)/2, returnButton.y + (returnButton.height - textSize.y)/2}, 24, 2, WHITE);

    // Title
    int titleWidth = MeasureTextEx(font, "LEADERBOARD", 48, 2).x;
    DrawTextEx(font, "LEADERBOARD", {(800.0f - titleWidth) / 2.0f, 60.0f}, 48, 2, yellow);

    // Leaderboard panel
    DrawRectangle(100.0f, 140.0f, 600.0f, 520.0f, panelColor);
    DrawRectangleLinesEx({100.0f, 140.0f, 600.0f, 520.0f}, 3.0f, yellow);

    // Header row
    DrawTextEx(font, "RANK", {130.0f, 160.0f}, 24, 2, yellow);
    DrawTextEx(font, "NAME", {250.0f, 160.0f}, 24, 2, yellow);
    DrawTextEx(font, "SCORE", {500.0f, 160.0f}, 24, 2, yellow);

    const Color rankColors[3] = {gold, silver, bronze};
    const char* rankText[3] = {"1st", "2nd", "3rd"};

    // Leaderboard entries
    const int displayCount = std::min(10, (int)entries.size());
    for (int i = 0; i < displayCount; i++) {
        float y = 210.0f + i * 40.0f;
        
        // Draw rank
        if (i < 3) {
            DrawTextEx(font, rankText[i], {140.0f, y}, 32, 2, rankColors[i]);
        } else {
            DrawTextEx(font, TextFormat("%d.", i + 1), {140.0f, y}, 28, 2, white);
        }
        
        // Draw name and score
        DrawTextEx(font, entries[i].name.c_str(), {220.0f, y}, 28, 2, white);
        DrawTextEx(font, TextFormat("%d", entries[i].score), {500.0f, y}, 28, 2, white);
    }

    // Input/submission area
    if (inputMode) {
        DrawTextEx(font, "ENTER YOUR NAME TO RECORD A TOP SCORE", {115.0f, 450.0f}, 18, 2, white);
        DrawTextEx(font, TextFormat("SCORE: %d", pendingScore), {115.0f, 480.0f}, 20, 2, gold);

        // Input box
        Rectangle inputBox = {115.0f, 520.0f, 570.0f, 50.0f};
        DrawRectangleRec(inputBox, Fade(BLACK, 0.8f));
        DrawRectangleLinesEx(inputBox, 3.0f, yellow);
        DrawTextEx(font, playerName.c_str(), {130.0f, 530.0f}, 32, 2, gold);

        // Instructions
        if (!nameSaved) {
            DrawTextEx(font, "BACKSPACE TO DELETE  -  ENTER TO SUBMIT  -  ESC TO SKIP", {60.0f, 585.0f}, 16, 2, silver);
        } else {
            DrawTextEx(font, "PRESS SPACE, ENTER, OR ESC TO RETURN", {155.0f, 585.0f}, 16, 2, silver);
        }
    } else {
        DrawTextEx(font, "PRESS SPACE, ENTER, OR ESC TO RETURN", {155.0f, 520.0f}, 16, 2, silver);
    }

    // Draw leaderboard cat in bottom-left corner
    if (leaderboardCatFramesLoaded && !leaderboardCatFrames.empty()) {
        Texture2D& catTex = leaderboardCatFrames[currentLeaderboardCatFrame];
        float catScale = 0.9f;
        float catX = 20.0f;
        float catY = 700.0f;
        DrawTextureEx(catTex, {catX, catY}, 0.0f, catScale, WHITE);

        leaderboardCatFrameTimer += GetFrameTime();
        if (leaderboardCatFrameTimer >= leaderboardCatFrameTime) {
            leaderboardCatFrameTimer -= leaderboardCatFrameTime;
            currentLeaderboardCatFrame = (currentLeaderboardCatFrame + 1) % leaderboardCatFrames.size();
        }
    }
}

bool Leaderboards::IsSaved()
{
    return nameSaved;
}

bool Leaderboards::WantReturn()
{
    return wantReturn;
}

bool Leaderboards::IsNewRecord()
{
    return isNewRecord;
}
