#include "menu.hpp"
#include "leaderboards.hpp"
#include "background_restore.hpp"
#include "utils.hpp"
#include <cstdlib>
#include <fstream>
#include <string>
#include <cmath>

Menu::Menu()
{
    if (!IsAudioDeviceReady()) InitAudioDevice();
    SetMasterVolume(1.0f);

    selectedOption = 0;
    startGame = false;
    loading = false;
    loadingEndTime = 0.0;
    loadingIconLoaded = false;
    pickFlashTimer = 0.0f;
    menuTransitionTimer = 0.0f;
    font = LoadFontEx(Utils::ResolveAssetPath("Font/monogram.ttf").c_str(), 64, 0, 0);
    std::string selectPath = Utils::ResolveAssetPath("Sounds/select.wav");
    selectSound = LoadSound(selectPath.c_str());
    if(selectSound.frameCount == 0) {
        TraceLog(LOG_WARNING, "Failed to load menu select sound: %s", selectPath.c_str());
    }

    loadingIconStream = Utils::LoadGifStream("Graphics/cat.gif");
    if(loadingIconStream.IsValid()) {
        loadingIconLoaded = true;
    } else {
        TraceLog(LOG_WARNING, "Failed to load loading icon gif: Graphics/cat.gif");
    }

    backgroundStream = Utils::LoadGifStream("Graphics/MAIN MENU.gif", GetScreenWidth(), GetScreenHeight(), true, PIXELFORMAT_UNCOMPRESSED_R4G4B4A4);
    blopCatStream = Utils::LoadGifStream("Graphics/blop cat.gif");

    // Load character sprites for selection menu
    struct { std::string path; std::string name; } charData[] = {
        {"Graphics/pixilart-drawing.png", "CLASSIC CHARACTER"},
        {"Graphics/Palkia.png", "PALKIA"},
        {"Graphics/DIALGA.png", "DALGIA"},
        {"Graphics/mewtwo.png", "MEWTWO"},
        {"Graphics/green ninja.png", "GREENNINJA"},
        {"Graphics/players characer 4.png", "PLAYER 4"}
    };

    for (int i = 0; i < 6; i++) {
        Texture2D tex = { 0 };
        if (!charData[i].path.empty()) {
            std::string fullPath = Utils::ResolveAssetPath(charData[i].path);
            if (FileExists(fullPath.c_str())) tex = LoadTexture(fullPath.c_str());
        }
        characterSlots.push_back({ tex, charData[i].name });
    }

    LoadCharacterHighscores();
}

Menu::~Menu()
{
    loadingIconStream.Unload();
    backgroundStream.Unload();
    blopCatStream.Unload();
    for(auto& charOpt : characterSlots) {
        if (charOpt.texture.id > 0) UnloadTexture(charOpt.texture);
    }
    UnloadSound(selectSound);
    UnloadFont(font);
}

void Menu::Draw()
{
    Color grey = {29, 29, 27, 255};
    Color yellow = {243, 216, 63, 255};
    Color dimYellow = {243, 216, 63, 150};
    Color panelColor = {20, 20, 22, 220};

    ClearBackground(grey);

    if(backgroundStream.IsValid()) {
        backgroundStream.UpdateVRAM(currentBackgroundFrame);
        // Make current background available to other modules for region restoration
        g_currentBg = &backgroundStream.texture;
        Rectangle src = {0.0f, 0.0f, float(backgroundStream.texture.width), float(backgroundStream.texture.height)};
        Rectangle dst = {0.0f, 0.0f, float(GetScreenWidth()), float(GetScreenHeight())};
        backgroundStream.Draw(src, dst, {0.0f, 0.0f}, 0.0f, WHITE, nullptr);

        backgroundFrameTimer += GetFrameTime();
        if(backgroundFrameTimer >= backgroundFrameTime) {
            backgroundFrameTimer -= backgroundFrameTime;
            currentBackgroundFrame = (currentBackgroundFrame + 1) % backgroundStream.frames.size();
        }
    }

    if(loading) {
        if(GetTime() >= loadingEndTime) {
            loading = false;
            startGame = true;
        }

        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.9f));

        // Hero Zoom Transition: Draw the picked character behind the cat
        Texture2D& hero = characterSlots[selectedCharacter].texture;
        if (hero.id > 0) {
            float progress = (float)(loadingEndTime - GetTime()) / 2.0f; // 1.0 to 0.0
            float heroScale = 3.0f + (1.0f - progress) * 2.0f; // Zooming in
            float heroAlpha = progress * 0.4f;
            
            Vector2 origin = {(float)hero.width * heroScale / 2.0f, (float)hero.height * heroScale / 2.0f};
            Rectangle dest = { (float)GetScreenWidth() / 2.0f, (float)GetScreenHeight() / 2.0f, (float)hero.width * heroScale, (float)hero.height * heroScale };
            
            // Draw a glowing silhouette of the chosen character
            DrawTexturePro(hero, {0, 0, (float)hero.width, (float)hero.height}, dest, origin, 0.0f, Fade(SKYBLUE, heroAlpha));
        }

        std::string loadingText = "Loading Please Wait";

        float textSizeX = MeasureTextEx(font, loadingText.c_str(), 48, 2).x;
        float iconHeight = 0.0f;
        float iconWidth = 0.0f;

        if(loadingIconLoaded) {
            loadingIconStream.UpdateVRAM(currentLoadingIconFrame);
            iconWidth = loadingIconStream.texture.width * 1.2f;
            iconHeight = loadingIconStream.texture.height * 1.2f;
            float iconX = (GetScreenWidth() - iconWidth) / 2.0f;
            float iconY = (GetScreenHeight() - iconHeight - 60.0f) / 2.0f;
            // Draw a rounded dark panel behind the icon to avoid a harsh square
            float pad = 12.0f;
            Rectangle panel = { iconX - pad, iconY - pad, iconWidth + pad*2.0f, iconHeight + pad*2.0f };
            DrawRectangleRounded(panel, 0.15f, 8, Fade(BLACK, 0.6f));
            loadingIconStream.Draw({0.0f, 0.0f, float(loadingIconStream.texture.width), float(loadingIconStream.texture.height)},
                {iconX, iconY, iconWidth, iconHeight},
                {0.0f, 0.0f}, 0.0f, WHITE, nullptr);

            loadingIconFrameTimer += GetFrameTime();
            if(loadingIconFrameTimer >= loadingIconFrameTime) {
                loadingIconFrameTimer -= loadingIconFrameTime;
                currentLoadingIconFrame = (currentLoadingIconFrame + 1) % loadingIconStream.frames.size();
            }
        }

        float textY = (GetScreenHeight() - 48.0f) / 2.0f;
        if(loadingIconLoaded) textY = (GetScreenHeight() + iconHeight) / 2.0f + 10.0f;
        Vector2 textPos = {(GetScreenWidth() - textSizeX) / 2.0f, textY};
        DrawTextEx(font, loadingText.c_str(), textPos, 48, 2, WHITE);
        return;
    }

    // Dark translucent overlay and main border behind menu content
    // Avoid drawing these when the LEADERBOARD view is active to prevent
    // double-border overlays.
    if (menuState == MAIN) {
        DrawRectangle(60, 120, 660, 520, Fade(BLACK, 0.52f));
        DrawRectangleLinesEx({60.0f, 120.0f, 660.0f, 520.0f}, 6.0f, ColorAlpha(YELLOW, 0.65f));
        int startY = 300;
        int optionSpacing = 90;
        int btnWidth = 560;
        int btnHeight = 70;
        int panelX = 60; // left of menu panel
        int panelWidth = 660; // width of menu panel
        int btnX = panelX + (panelWidth - btnWidth) / 2;

        const char* labels[] = {"START GAME", "SETTINGS", "LEADERBOARD", "QUIT"};
        int fontSizes[] = {48, 48, 36, 48};
        
        for (int i = 0; i < 4; i++) {
            // Calculate staggered slide-in animation
            float buttonDelay = i * 0.1f;
            float t = (menuTransitionTimer - buttonDelay) / transitionDuration;
            if (t < 0) t = 0;
            if (t > 1) t = 1;
            
            // Cubic ease-out: 1 - (1 - t)^3
            float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
            float xOffset = (1.0f - ease) * GetScreenWidth();

            Rectangle btnRect = {(float)btnX + xOffset, (float)(startY + i * optionSpacing - 15), (float)btnWidth, (float)btnHeight};
            bool isSelected = (selectedOption == i);

            // Background and border matching leaderboard style
            DrawRectangleRec(btnRect, isSelected ? Fade(SKYBLUE, 0.4f) : panelColor);
            DrawRectangleLinesEx(btnRect, 2, isSelected ? WHITE : yellow);

            // Centered text
            Vector2 labelSize = MeasureTextEx(font, labels[i], (float)fontSizes[i], 2);
            Vector2 textPos = {btnRect.x + (btnRect.width - labelSize.x) / 2.0f, btnRect.y + (btnRect.height - labelSize.y) / 2.0f};
            DrawTextEx(font, labels[i], textPos, (float)fontSizes[i], 2, WHITE);
        }
        
        // Draw Blop Cat at the bottom right
        if (blopCatStream.IsValid()) {
            blopCatStream.UpdateVRAM(currentBlopCatFrame);
            Texture2D& tex = blopCatStream.texture;
            float posX = (float)GetScreenWidth() - tex.width - 25;
            float posY = (float)GetScreenHeight() - tex.height - 25;
            DrawTextureV(tex, { posX, posY }, WHITE);
        }
        
    } else if(menuState == SETTINGS) {
        // Settings screen
        DrawTextEx(font, "SETTINGS", {280.0f, 130.0f}, 56, 2, yellow);
        
        // Volume control
        int volumeY = 280;
        Color volumeColor = (settingsSelectedOption == 0) ? yellow : dimYellow;
        DrawTextEx(font, "VOLUME", {200.0f, (float)volumeY}, 40, 2, volumeColor);
        
        // Volume bar
        float barWidth = 300.0f;
        float barX = 250.0f;
        float barY = volumeY + 60.0f;
        DrawRectangle((int)barX, (int)barY, (int)barWidth, 40, {100, 100, 100, 200});
        float fillWidth = barWidth * masterVolume;
        DrawRectangle((int)barX, (int)barY, (int)fillWidth, 40, yellow);
        DrawRectangleLinesEx({barX, barY, barWidth, 40.0f}, 2.0f, yellow);
        
        // Volume percentage
        int volumePercent = (int)(masterVolume * 100.0f);
        std::string volumeText = std::to_string(volumePercent) + "%";
        DrawTextEx(font, volumeText.c_str(), {600.0f, barY + 5.0f}, 32, 2, yellow);
        
        // Credits
        int creditsY = 420;
        DrawTextEx(font, "CREDITS", {220.0f, (float)creditsY}, 40, 2, yellow);
        DrawTextEx(font, "Game by: Your Name", {180.0f, (float)(creditsY + 50)}, 28, 2, dimYellow);
        DrawTextEx(font, "Using: Raylib 5.0", {210.0f, (float)(creditsY + 85)}, 28, 2, dimYellow);
        
        // Return button
        float t = menuTransitionTimer / transitionDuration;
        if (t > 1) t = 1;
        float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
        float xOffset = (1.0f - ease) * GetScreenWidth();

        Rectangle returnBtn = { 250.0f + xOffset, 585.0f, 280.0f, 70.0f };
        bool returnSelected = (settingsSelectedOption == 1);

        DrawRectangleRec(returnBtn, returnSelected ? Fade(SKYBLUE, 0.4f) : panelColor);
        DrawRectangleLinesEx(returnBtn, 2, returnSelected ? WHITE : yellow);

        Vector2 retTextSize = MeasureTextEx(font, "RETURN", 40, 2);
        DrawTextEx(font, "RETURN", 
            {returnBtn.x + (returnBtn.width - retTextSize.x)/2, returnBtn.y + (returnBtn.height - retTextSize.y)/2}, 
            40, 2, WHITE);
        
        // Instructions
        DrawTextEx(font, "ARROW KEYS TO ADJUST VOLUME", {100.0f, 680.0f}, 20, 2, yellow);
    } else if (menuState == CHARACTER_SELECT) {
        DrawRectangle(60, 120, 660, 520, Fade(BLACK, 0.52f));
        DrawRectangleLinesEx({60.0f, 120.0f, 660.0f, 520.0f}, 6.0f, ColorAlpha(YELLOW, 0.65f));
        
        DrawTextEx(font, "SELECT YOUR CHARACTER", {180.0f, 140.0f}, 48, 2, yellow);

        int startX = 130;
        int startY = 220;
        int slotSize = 150;
        int spacing = 40;

        for (int i = 0; i < 6; i++) {
            int row = i / 3;
            int col = i % 3;
            
            Rectangle slotRect = {
                (float)startX + col * (slotSize + spacing),
                (float)startY + row * (slotSize + spacing),
                (float)slotSize,
                (float)slotSize
            };

            bool isSelected = (selectedCharacter == i);
            bool isHovered = CheckCollisionPointRec(GetMousePosition(), slotRect);
            float pulse = 0.1f * sinf(GetTime() * 10.0f);
            
            if (isSelected) {
                DrawRectangleRec(slotRect, Fade(SKYBLUE, 0.2f + pulse));
            }
            DrawRectangleRec(slotRect, isSelected ? Fade(SKYBLUE, 0.4f) : panelColor);
            DrawRectangleLinesEx(slotRect, isSelected ? 4.0f : 2.0f, (isSelected || isHovered) ? WHITE : yellow);

            // Draw character sprite
            if (characterSlots[i].texture.id > 0) {
                float scale = (slotSize * 0.6f) / (float)characterSlots[i].texture.height;
                if (characterSlots[i].texture.width * scale > slotSize * 0.8f) {
                    scale = (slotSize * 0.8f) / (float)characterSlots[i].texture.width;
                }
                if (isSelected) scale *= (1.1f + pulse); // Pulsing scale for active character
                
                Vector2 texPos = {
                    slotRect.x + (slotRect.width - characterSlots[i].texture.width * scale) / 2.0f,
                    slotRect.y + (slotRect.height - characterSlots[i].texture.height * scale) / 2.0f - 10.0f
                };
                DrawTextureEx(characterSlots[i].texture, texPos, 0.0f, scale, WHITE);
            }

            // Draw name
            Vector2 labelSize = MeasureTextEx(font, characterSlots[i].name.c_str(), 20, 2);
            DrawTextEx(font, characterSlots[i].name.c_str(), 
                {slotRect.x + (slotRect.width - labelSize.x)/2, slotRect.y + slotRect.height - labelSize.y - 12}, 
                20, 2, isSelected ? WHITE : dimYellow);

            // Draw Best Score for the selected character
            if (isSelected && i < characterHighscores.size()) {
                std::string bestScoreText = TextFormat("BEST: %d", characterHighscores[i]);
                DrawTextEx(font, bestScoreText.c_str(), {slotRect.x + (slotRect.width - MeasureTextEx(font, bestScoreText.c_str(), 18, 2).x)/2, slotRect.y + slotRect.height + 5}, 18, 2, GOLD);
            }

        // Draw "NEW RECORD!" label for the character that just achieved it
        if (i == lastRecordCharacterIdx) {
            float pulse = (sinf((float)GetTime() * 10.0f) + 1.0f) * 0.5f;
            Color recordColor = ColorLerp(GOLD, WHITE, pulse);
            const char* recordLabel = "NEW RECORD!";
            Vector2 labelSize = MeasureTextEx(font, recordLabel, 18, 2);
            DrawTextEx(font, recordLabel, {slotRect.x + (slotRect.width - labelSize.x)/2, slotRect.y - 20}, 18, 2, recordColor);
        }
        }

        DrawTextEx(font, "ARROWS TO NAVIGATE - SPACE TO CONFIRM - ESC TO BACK", {110.0f, 610.0f}, 20, 2, LIGHTGRAY);
    } else if(menuState == LEADERBOARD) {
        Leaderboards::Draw(font);
    } else if (menuState == QUIT_CONFIRM) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.85f));
        
        // Intensity pulse: Red overlay flickering to emphasize the weight of the choice
        float pulse = (sinf((float)GetTime() * 10.0f) + 1.0f) * 0.5f;
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(RED, 0.12f * pulse));

        if (quitStatusTimer > 0.0f) {
            Vector2 msgSize = MeasureTextEx(font, quitStatusMessage.c_str(), 44, 2);
            DrawTextEx(font, quitStatusMessage.c_str(), 
                {(float)(GetScreenWidth() - msgSize.x) / 2.0f, (float)(GetScreenHeight() - msgSize.y) / 2.0f}, 44, 2, yellow);
        } else {
            std::string prompt = "DO YOU REALLY WANT TO QUIT?";
            Vector2 promptSize = MeasureTextEx(font, prompt.c_str(), 40, 2);
            DrawTextEx(font, prompt.c_str(), {(float)(GetScreenWidth() - promptSize.x) / 2.0f, 250}, 40, 2, yellow);
            
            Rectangle yesBtn = { (float)GetScreenWidth() / 2.0f - 200, 380, 160, 60 };
            Rectangle noBtn = { (float)GetScreenWidth() / 2.0f + 40, 380, 160, 60 };
            
            // YES Button
            DrawRectangleRec(yesBtn, quitConfirmOption == 0 ? Fade(SKYBLUE, 0.4f) : panelColor);
            DrawRectangleLinesEx(yesBtn, 2, quitConfirmOption == 0 ? WHITE : yellow);
            Vector2 yesSize = MeasureTextEx(font, "YES", 30, 2);
            DrawTextEx(font, "YES", {yesBtn.x + (yesBtn.width - yesSize.x)/2, yesBtn.y + (yesBtn.height - yesSize.y)/2}, 30, 2, WHITE);

            // NO Button
            DrawRectangleRec(noBtn, quitConfirmOption == 1 ? Fade(SKYBLUE, 0.4f) : panelColor);
            DrawRectangleLinesEx(noBtn, 2, quitConfirmOption == 1 ? WHITE : yellow);
            Vector2 noSize = MeasureTextEx(font, "NO", 30, 2);
            DrawTextEx(font, "NO", {noBtn.x + (noBtn.width - noSize.x)/2, noBtn.y + (noBtn.height - noSize.y)/2}, 30, 2, WHITE);
        }
    }

    // Draw Pick Flash effect overlay
    if (pickFlashTimer > 0) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(WHITE, pickFlashTimer));
    }

    // Main border - Drawn last to ensure it overlays background and menu elements correctly
    DrawRectangleRoundedLinesEx({10, 10, 780, 780}, 0.18f, 20, 3.0f, yellow);
}

void Menu::Update()
{
    if(loading && GetTime() >= loadingEndTime) {
        loading = false;
        startGame = true;
    }
    if (pickFlashTimer > 0) pickFlashTimer -= GetFrameTime() * 2.0f;
    menuTransitionTimer += GetFrameTime();

    if (quitStatusTimer > 0.0f) {
        quitStatusTimer -= GetFrameTime();
        if (quitStatusTimer <= 0.0f) {
            if (pendingQuit) exit(0);
            else menuState = MAIN;
        }
    }

    // Blop cat reaction: Speed up the animation when hovering over 'START GAME'
    if (menuState == MAIN && blopCatStream.IsValid()) {
        int startY = 300;
        int btnWidth = 560;
        int btnHeight = 70;
        int btnX = 60 + (660 - btnWidth) / 2;
        Rectangle startBtn = {(float)btnX, (float)(startY - 15), (float)btnWidth, (float)btnHeight};

        if (CheckCollisionPointRec(GetMousePosition(), startBtn)) {
            blopCatFrameTime = 0.04f; // Hyper-blop speed!
        } else {
            blopCatFrameTime = 0.08f; // Regular relaxed speed
        }
    }

    if (blopCatStream.IsValid() && !blopCatStream.frames.empty()) {
        blopCatFrameTimer += GetFrameTime();
        if (blopCatFrameTimer >= blopCatFrameTime) {
            blopCatFrameTimer -= blopCatFrameTime;
            currentBlopCatFrame = (currentBlopCatFrame + 1) % blopCatStream.frames.size();
        }
    }
}

void Menu::HandleInput()
{
    if(loading) return;

    if(menuState == MAIN) {
        // Mouse click handling for menu buttons (mirror keyboard behavior)
        Vector2 mousePos = GetMousePosition();
        int startY = 300;
        int optionSpacing = 90;
        int btnWidth = 560;
        int btnHeight = 70;
        int panelX = 60; // left of menu panel
        int panelWidth = 660; // width of menu panel
        int btnX = panelX + (panelWidth - btnWidth) / 2;
        for (int i = 0; i < 4; i++) {
            float buttonDelay = i * 0.1f;
            float t = (menuTransitionTimer - buttonDelay) / transitionDuration;
            if (t < 0) t = 0;
            if (t > 1) t = 1;
            float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
            float xOffset = (1.0f - ease) * GetScreenWidth();
            Rectangle btnRect = {(float)btnX + xOffset, (float)(startY + i * optionSpacing - 15), (float)btnWidth, (float)btnHeight};
            if (CheckCollisionPointRec(mousePos, btnRect)) {
                selectedOption = i;
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if(selectSound.frameCount > 0) PlaySound(selectSound);
                    if(selectedOption == 0) {
                        menuState = CHARACTER_SELECT;
                        selectedCharacter = 0;
                        menuTransitionTimer = 0.0f;
                    } else if(selectedOption == 1) {
                        menuState = SETTINGS;
                        settingsSelectedOption = 0;
                        menuTransitionTimer = 0.0f;
                    } else if(selectedOption == 2) {
                        menuState = LEADERBOARD;
                        settingsSelectedOption = 0;
                        Leaderboards::InitView();
                    } else if(selectedOption == 3) {
                        menuState = QUIT_CONFIRM;
                        quitConfirmOption = 1; // Default highlight to NO
                        quitStatusMessage = "";
                        quitStatusTimer = 0.0f;
                    }
                    return; // handled mouse click
                }
            }
        }

        if(IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            selectedOption--;
            if(selectedOption < 0) selectedOption = 3;
            if(selectSound.frameCount > 0) PlaySound(selectSound);
        } else if(IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            selectedOption++;
            if(selectedOption > 3) selectedOption = 0;
            if(selectSound.frameCount > 0) PlaySound(selectSound);
        }
        
        if(IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
            if(selectSound.frameCount > 0) PlaySound(selectSound);
            if(selectedOption == 0) {
                menuState = CHARACTER_SELECT;
                selectedCharacter = 0;
                menuTransitionTimer = 0.0f;
            } else if(selectedOption == 1) {
                menuState = SETTINGS;
                settingsSelectedOption = 0;
                menuTransitionTimer = 0.0f;
            } else if(selectedOption == 2) {
                menuState = LEADERBOARD;
                settingsSelectedOption = 0;
                Leaderboards::InitView();
            } else if(selectedOption == 3) {
                menuState = QUIT_CONFIRM;
                quitConfirmOption = 1;
                quitStatusMessage = "";
                quitStatusTimer = 0.0f;
            }
        }
    } else if(menuState == SETTINGS) {
        if(IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            settingsSelectedOption--;
            if(settingsSelectedOption < 0) settingsSelectedOption = 1;
            if(selectSound.frameCount > 0) PlaySound(selectSound);
        } else if(IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            settingsSelectedOption++;
            if(settingsSelectedOption > 1) settingsSelectedOption = 0;
            if(selectSound.frameCount > 0) PlaySound(selectSound);
        }
        
        // Volume control with left/right arrows
        if(settingsSelectedOption == 0) {
            if(IsKeyPressed(KEY_LEFT)) {
                masterVolume -= 0.1f;
                if(masterVolume < 0.0f) masterVolume = 0.0f;
                if(selectSound.frameCount > 0) PlaySound(selectSound);
            } else if(IsKeyPressed(KEY_RIGHT)) {
                masterVolume += 0.1f;
                if(masterVolume > 1.0f) masterVolume = 1.0f;
                if(selectSound.frameCount > 0) PlaySound(selectSound);
            }
            SetMasterVolume(masterVolume);
        }
        
        if(IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
            if(selectSound.frameCount > 0) PlaySound(selectSound);
            if(settingsSelectedOption == 1) {
                menuState = MAIN;
                selectedOption = 1;
                menuTransitionTimer = 0.0f;
            }
        }
    } else if (menuState == CHARACTER_SELECT) {
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
            if (selectedCharacter % 3 > 0) selectedCharacter--;
            if (selectSound.frameCount > 0) PlaySound(selectSound);
        } else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
            if (selectedCharacter % 3 < 2) selectedCharacter++;
            if (selectSound.frameCount > 0) PlaySound(selectSound);
        } else if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            if (selectedCharacter >= 3) selectedCharacter -= 3;
            if (selectSound.frameCount > 0) PlaySound(selectSound);
        } else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            if (selectedCharacter < 3) selectedCharacter += 3;
            if (selectSound.frameCount > 0) PlaySound(selectSound);
        }

        // Mouse handling for slots
        int startX = 130;
        int startY = 220;
        int slotSize = 150;
        int spacing = 40;
        for (int i = 0; i < 6; i++) {
            int row = i / 3;
            int col = i % 3;
            Rectangle slotRect = {
                (float)startX + col * (slotSize + spacing),
                (float)startY + row * (slotSize + spacing),
                (float)slotSize,
                (float)slotSize
            };
            if (CheckCollisionPointRec(GetMousePosition(), slotRect)) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    selectedCharacter = i;
                    if (selectSound.frameCount > 0) PlaySound(selectSound);
                    loading = true;
                    pickFlashTimer = 1.0f;
                    loadingEndTime = GetTime() + 2.0;
                }
            }
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            if (selectSound.frameCount > 0) PlaySound(selectSound);
            menuState = MAIN;
            selectedOption = 0;
        }

        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
            if (selectSound.frameCount > 0) PlaySound(selectSound);
            loading = true;
            pickFlashTimer = 1.0f;
            loadingEndTime = GetTime() + 2.0;
        }
    } else if(menuState == LEADERBOARD) {
        Leaderboards::Update();
        if (Leaderboards::WantReturn()) {
            if(selectSound.frameCount > 0) PlaySound(selectSound);
            menuState = MAIN;
            selectedOption = 0;
        } else if(Leaderboards::IsSaved() && (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))) {
            if(selectSound.frameCount > 0) PlaySound(selectSound);
            menuState = MAIN;
            selectedOption = 2;
        }
    } else if (menuState == QUIT_CONFIRM) {
        if (quitStatusTimer > 0.0f) return;

        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
            quitConfirmOption = (quitConfirmOption == 0) ? 1 : 0;
            if (selectSound.frameCount > 0) PlaySound(selectSound);
        }

        Vector2 mousePos = GetMousePosition();
        Rectangle yesBtn = { (float)GetScreenWidth() / 2.0f - 200, 380, 160, 60 };
        Rectangle noBtn = { (float)GetScreenWidth() / 2.0f + 40, 380, 160, 60 };

        if (CheckCollisionPointRec(mousePos, yesBtn)) {
            if (quitConfirmOption != 0 && selectSound.frameCount > 0) PlaySound(selectSound);
            quitConfirmOption = 0;
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                quitStatusMessage = "GOOD BYE BRAVE WARRIOR";
                quitStatusTimer = 2.0f;
                pendingQuit = true;
            }
        } else if (CheckCollisionPointRec(mousePos, noBtn)) {
            if (quitConfirmOption != 1 && selectSound.frameCount > 0) PlaySound(selectSound);
            quitConfirmOption = 1;
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                quitStatusMessage = "GOOD CHOICE WARRIOR";
                quitStatusTimer = 1.5f;
                pendingQuit = false;
            }
        }

        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
            if (selectSound.frameCount > 0) PlaySound(selectSound);
            if (quitConfirmOption == 0) {
                quitStatusMessage = "GOOD BYE BRAVE WARRIOR";
                quitStatusTimer = 2.0f;
                pendingQuit = true;
            } else {
                quitStatusMessage = "GOOD CHOICE WARRIOR";
                quitStatusTimer = 1.5f;
                pendingQuit = false;
            }
        }
        
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (selectSound.frameCount > 0) PlaySound(selectSound);
            quitStatusMessage = "GOOD CHOICE WARRIOR";
            quitStatusTimer = 1.2f;
            pendingQuit = false;
        }
    }
}

bool Menu::GetStartGame() const
{
    return startGame;
}

void Menu::Reset()
{
    menuState = MAIN;
    selectedOption = 0;
    startGame = false;
    loading = false;
    loadingEndTime = 0.0;
    pickFlashTimer = 0.0f;
    settingsSelectedOption = 0;
    menuTransitionTimer = 0.0f;
    lastRecordCharacterIdx = -1; // Reset record highlight when returning to menu
}

void Menu::LoadCharacterHighscores() {
    characterHighscores.clear();
    std::ifstream file("character_highscores.txt");
    if (file.is_open()) {
        int score;
        while (file >> score) {
            characterHighscores.push_back(score);
        }
        file.close();
    }

    // Ensure all characters have a high score entry, initialize to 0 if missing
    while (characterHighscores.size() < characterSlots.size()) {
        characterHighscores.push_back(0);
    }

    // If there are more entries than characters (e.g., after removing a character),
    // truncate the vector to match the number of characters.
    if (characterHighscores.size() > characterSlots.size()) {
        characterHighscores.resize(characterSlots.size());
    }
}

void Menu::SaveCharacterHighscores() {
    std::ofstream file("character_highscores.txt");
    if (file.is_open()) {
        for (int score : characterHighscores) {
            file << score << std::endl;
        }
        file.close();
    } else {
        TraceLog(LOG_WARNING, "Failed to save character high scores.");
    }
}

void Menu::UpdateCharacterHighscore(int characterIndex, int newScore) {
    if (characterIndex >= 0 && characterIndex < characterHighscores.size()) {
        if (newScore > characterHighscores[characterIndex]) {
            lastRecordCharacterIdx = characterIndex;
            characterHighscores[characterIndex] = newScore;
            SaveCharacterHighscores(); // Save immediately after update
            TraceLog(LOG_INFO, "New high score for character %d: %d", characterIndex, newScore);
        }
    }
}
