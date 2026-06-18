#include <raylib.h>
#include "background_restore.hpp"
#include "game.hpp"
#include "menu.hpp"
#include "leaderboards.hpp"
#include "boss2_module.hpp"
#include "utils.hpp"
#include "assets.hpp"
#include <cmath>
#include <string>
#include <vector>

std::string FormatWithLeadingZeros(int number, int width) {
    std::string numberText = std::to_string(number);
    int leadingZeros = width - (int)numberText.length();
    if (leadingZeros < 0) leadingZeros = 0;
    return std::string(leadingZeros, '0') + numberText;
}

int main()
{
    Color yellow = {243, 216, 63, 255};
    int offset = 50;
    int windowWidth = 850;
    int windowHeight = 760;

    const char* distortionShaderSrc = 
        "#version 330\n"
        "in vec2 fragTexCoord;"
        "out vec4 finalColor;"
        "uniform sampler2D texture0;"
        "uniform float distortion;"
        "uniform float chromaticAmount;"
        "uniform vec2 distortionCenter;"
        "void main() {"
        "    vec2 dir = fragTexCoord - distortionCenter;"
        "    vec4 sumR = vec4(0.0);"
        "    vec4 sumG = vec4(0.0);"
        "    vec4 sumB = vec4(0.0);"
        "    const int samples = 16;" // Increased samples for a smoother blur
        "    for (int i = 0; i < samples; i++) {"
        "        float scale = 1.0 - (distortion * 0.04) * (float(i) / float(samples - 1));"
        "        vec2 uv = distortionCenter + dir * scale;"
        "        sumR += texture(texture0, uv + vec2(chromaticAmount, 0.0));"
        "        sumG += texture(texture0, uv);"
        "        sumB += texture(texture0, uv - vec2(chromaticAmount, 0.0));"
        "    }"
        "    finalColor = vec4(sumR.r / float(samples), sumG.g / float(samples), sumB.b / float(samples), sumG.a / float(samples));"
        "}";

    const char* silhouetteShaderSrc =
        "#version 330\n"
        "in vec2 fragTexCoord;"
        "in vec4 fragColor;"
        "out vec4 finalColor;"
        "uniform sampler2D texture0;"
        "void main() {"
        "    vec4 texel = texture(texture0, fragTexCoord);"
        "    if (texel.a < 0.1) discard;"
        "    finalColor = vec4(fragColor.rgb, texel.a * fragColor.a);"
        "}";

    InitWindow(windowWidth + offset, windowHeight + 2 * offset, "C++ Space Invaders");
    InitAudioDevice();
    SetExitKey(KEY_NULL); // Prevent ESC from closing the game, allowing it to be used for UI/Skip logic

    Font font = LoadFontEx(Utils::ResolveAssetPath(Paths::Fonts::Monogram).c_str(), 64, 0, 0);
    // Load the heart icon for a permanent lives indicator
    Texture2D lifeIcon = LoadTexture(Utils::ResolveAssetPath("Graphics/heart.png").c_str());
    // Define a fixed reference size for the heart HUD icons
    float hudReferenceWidth = 28.0f;
    float hudReferenceHeight = 28.0f;

    SetTargetFPS(60);

    Menu menu;
    Game game;
    // Boss2 sprite will be loaded during game initialization.
    bool inMenu = true;

    // Setup Post-processing
    Shader distShader = LoadShaderFromMemory(0, distortionShaderSrc);
    int distLoc = GetShaderLocation(distShader, "distortion");
    int chromaticLoc = GetShaderLocation(distShader, "chromaticAmount");
    int distCenterLoc = GetShaderLocation(distShader, "distortionCenter");
    RenderTexture2D target = LoadRenderTexture(windowWidth + offset, windowHeight + 2 * offset);

    // Setup Silhouette Shader for telegraphing attacks
    Shader silhouetteShader = LoadShaderFromMemory(0, silhouetteShaderSrc);
    g_silhouetteShader = &silhouetteShader;

    bool paused = false;
    bool returningToMenu = false;
    bool showLeaderboardEntry = false;
    bool viewingStats = false;
    bool characterNewBestTriggered = false;
    double returnStartTime = 0.0;
    const double returnFadeDuration = 0.5;

    while(WindowShouldClose() == false) {
        if(inMenu) {
            UpdateMusicStream(game.music);
            menu.HandleInput();
            menu.Update();
            BeginDrawing();
            menu.Draw();
            EndDrawing();
            
            if(menu.GetStartGame()) {
                inMenu = false;
                game.SetSelectedCharacterIndex(menu.GetSelectedCharacter());

                // Handle character selection logic
                int charIdx = menu.GetSelectedCharacter();
                std::string selectedPath = Paths::Graphics::Spaceship; // Default ship

                switch (charIdx) {
                    case 0: selectedPath = "Graphics/pixilart-drawing.png"; break;
                    case 1: selectedPath = "Graphics/Palkia.png"; break;
                    case 2: selectedPath = "Graphics/DIALGA.png"; break;
                    case 3: selectedPath = "Graphics/mewtwo.png"; break;
                    case 4: selectedPath = "Graphics/green ninja.png"; break;
                    case 5: selectedPath = "Graphics/players characer 4.png"; break;
                    default: selectedPath = "Graphics/pixilart-drawing.png"; break;
                }

                // Only apply if the file exists, otherwise keep current
                if (FileExists(Utils::ResolveAssetPath(selectedPath).c_str())) {
                    game.SetSpaceshipImage(selectedPath);
                }

                menu.Reset();
                game.InitGame();
            }
        } else {
            if (IsKeyPressed(KEY_P)) {
                paused = !paused;
                if (!paused) viewingStats = false;
            }

            if (!paused) {  
                game.HandleInput();
                game.Update();
            } else {
                // Keep music playing even while paused
                UpdateMusicStream(game.music);
            }

            // Render game state to texture
            BeginTextureMode(target);
            ClearBackground(BLACK);

            Vector2 shakeOffset = game.GetScreenShakeOffset();
            Camera2D shakeCamera = {{0.0f, 0.0f}, {0.0f, 0.0f}, 0.0f, 1.0f};
            shakeCamera.offset = shakeOffset;

            // Draw game world inside camera (affected by shake) and pass font for UI elements
            BeginMode2D(shakeCamera);
            game.Draw(font);
            EndMode2D();

            EndTextureMode();

            // Draw texture to screen with shader applied
            BeginDrawing();
            ClearBackground(BLACK);

            float distortion = game.GetDistortionLevel();
            SetShaderValue(distShader, distLoc, &distortion, SHADER_UNIFORM_FLOAT);
            float chromatic = game.GetChromaticAmount();
            SetShaderValue(distShader, chromaticLoc, &chromatic, SHADER_UNIFORM_FLOAT);
            Vector2 distCenter = game.GetDistortionCenter();
            SetShaderValue(distShader, distCenterLoc, &distCenter, SHADER_UNIFORM_VEC2);

            BeginShaderMode(distShader);
            DrawTextureRec(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height }, (Vector2){ 0, 0 }, WHITE);
            EndShaderMode();

            // Draw static HUD elements outside camera space (skip when showing leaderboard overlay)
            if (!showLeaderboardEntry) {
                DrawRectangleRoundedLinesEx({10, 10, (float)(windowWidth + offset - 20), (float)(windowHeight + 2 * offset - 20)}, 0.18f, 20, 3.0f, yellow);
                DrawLineEx({25, (float)(windowHeight + 2 * offset - 70)}, {(float)(windowWidth + offset - 25), (float)(windowHeight + 2 * offset - 70)}, 3, yellow);
            }

            if(game.run){
                DrawTextEx(font, TextFormat("LEVEL %02d", game.GetCurrentStage()), {(float)(windowWidth + offset) - 240, (float)windowHeight + 40}, 34, 2, yellow);
            }
            float x = 50.0f;
            for(int i = 0; i < game.lives; i ++) {
                float hudScaleX = hudReferenceWidth / (float)lifeIcon.width;
                float hudScaleY = hudReferenceHeight / (float)lifeIcon.height;
                float hudScale = fminf(hudScaleX, hudScaleY);

                // Add pulsing animation if only 1 life remains
                if (game.lives == 1) {
                    float pulse = 0.1f * sinf(GetTime() * 10.0f); // Oscillate between -0.1 and 0.1
                    hudScale += pulse;
                }
                DrawTextureEx(lifeIcon, {x, (float)windowHeight}, 0.0f, hudScale, WHITE);
                x += 50;
            }
            // Call DrawBuffIcons without tooltips when not paused
            if (!paused) {
                game.DrawBuffIcons(x, (float)windowHeight, font, false);
            }

            DrawTextEx(font, "SCORE", {50, 15}, 34, 2, yellow);
            // Show 0 when player is not actively running (not playing)
            int displayScore = (game.run ? game.score : 0);
            std::string scoreText = FormatWithLeadingZeros(displayScore, 5);
            float scoreLabelWidth = MeasureTextEx(font, "SCORE", 34, 2).x;
            DrawTextEx(font, scoreText.c_str(), {50 + scoreLabelWidth + 20, 15}, 34, 2, yellow);

            DrawTextEx(font, "HIGH-SCORE", {(float)(windowWidth + offset) - 320, 15}, 34, 2, yellow);
            std::string highscoreText = FormatWithLeadingZeros(game.highscore, 5);
            DrawTextEx(font, highscoreText.c_str(), {(float)(windowWidth + offset) - 205, 40}, 34, 2, yellow);


            if(paused) {
                DrawRectangle(0, 0, windowWidth + offset, windowHeight + 2 * offset, Fade(BLACK, 0.58f));
                
                if (!viewingStats) {
                    Rectangle buttons[4];
                    const char* buttonText[4] = {"CONTINUE", "CHARACTER STATS", "BACK TO MAIN MENU", "RETRY"};
                    float buttonWidth = 280.0f;
                    float buttonHeight = 50.0f;
                    float buttonX = (windowWidth + offset - buttonWidth) / 2.0f;
                    float buttonY = 220.0f;
                    
                    DrawTextEx(font, "PAUSED", {(windowWidth + offset) / 2.0f - MeasureTextEx(font, "PAUSED", 64, 2).x / 2.0f, 120}, 64, 2, WHITE);
                    Vector2 mousePos = GetMousePosition();
                    
                    for(int i = 0; i < 4; i++) {
                        buttons[i] = {buttonX, buttonY + i * (buttonHeight + 15.0f), buttonWidth, buttonHeight};
                        bool hovered = CheckCollisionPointRec(mousePos, buttons[i]);
                        
                        DrawRectangleRec(buttons[i], hovered ? Fade(SKYBLUE, 0.4f) : Fade(BLACK, 0.8f));
                        DrawRectangleLinesEx(buttons[i], 2, hovered ? WHITE : yellow);
                        
                        Vector2 textSize = MeasureTextEx(font, buttonText[i], 30, 2);
                        DrawTextEx(font, buttonText[i], {buttons[i].x + (buttons[i].width - textSize.x) / 2.0f, buttons[i].y + (buttons[i].height - textSize.y) / 2.0f}, 30, 2, WHITE);
                        
                        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            if (i == 0) paused = false;
                            else if (i == 1) viewingStats = true;
                            else if (i == 2) {
                                returningToMenu = true;
                                returnStartTime = GetTime();
                                paused = false;
                            }
                            else if (i == 3) {
                                game.Reset();
                                game.InitGame();
                                paused = false;
                            }
                        }
                    }
                }
                else {
                    game.DrawCharacterStats(font);
                    
                    // Centered Back button at the bottom of the HUD area
                    float backBtnY = (float)windowHeight + offset + 10.0f;
                    Rectangle backBtn = { (windowWidth + offset - 200.0f) / 2.0f, backBtnY, 200.0f, 40.0f };
                    bool hovered = CheckCollisionPointRec(GetMousePosition(), backBtn);
                    
                    DrawRectangleRec(backBtn, hovered ? Fade(SKYBLUE, 0.4f) : Fade(BLACK, 0.8f));
                    DrawRectangleLinesEx(backBtn, 2, hovered ? WHITE : yellow);
                    
                    Vector2 bTextSize = MeasureTextEx(font, "BACK", 24, 2);
                    DrawTextEx(font, "BACK", {backBtn.x + (backBtn.width - bTextSize.x)/2, backBtn.y + (backBtn.height - bTextSize.y)/2}, 24, 2, WHITE);
                    
                    if ((hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_X)) {
                        viewingStats = false;
                    }
                }

                float iconStartX = 50.0f + (game.lives * 50); // Calculate starting X based on lives counter
                // Draw a small helper label above the icons
                DrawTextEx(font, "ACTIVE BUFFS (HOVER FOR DETAILS)", {iconStartX, (float)windowHeight - 20}, 16, 1, SKYBLUE);
                
                // Draw buff icons with tooltips enabled
                game.DrawBuffIcons(iconStartX, (float)windowHeight, font, true);
            }

            if(!game.run && game.gameOverActive){
                float alpha = 0.5f + 0.5f * sinf(float(GetTime() * 6.0));
                DrawRectangle(0, 0, windowWidth + offset, windowHeight + 2 * offset, Fade(BLACK, alpha * 0.4f));
                std::string text = "GAME OVER";
                Vector2 size = MeasureTextEx(font, text.c_str(), 72, 2);
                DrawTextEx(font, text.c_str(), {(windowWidth + offset - size.x)/2, (windowHeight + 2 * offset - size.y)/2 - 20}, 72, 2, RED);

                Rectangle homeButton = {(windowWidth + offset) * 0.5f - 90.0f, (windowHeight + 2 * offset) * 0.5f + 80.0f, 180.0f, 48.0f};
                Vector2 mousePos = GetMousePosition();
                bool homeHover = CheckCollisionPointRec(mousePos, homeButton);
                DrawRectangleRec(homeButton, homeHover ? Fade(SKYBLUE, 0.85f) : Fade(BLUE, 0.75f));
                DrawRectangleLinesEx(homeButton, 3.0f, homeHover ? WHITE : YELLOW);
                std::string homeLabel = "RETURN HOME";
                Vector2 homeSize = MeasureTextEx(font, homeLabel.c_str(), 24, 2);
                DrawTextEx(font, homeLabel.c_str(), {homeButton.x + (homeButton.width - homeSize.x) / 2.0f, homeButton.y + (homeButton.height - homeSize.y) / 2.0f}, 24, 2, WHITE);

                if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && homeHover){
                    StopMusicStream(game.GameOverMusic);
                    game.Reset();
                    game.InitGame();
                    inMenu = true;
                    showLeaderboardEntry = false;
                    characterNewBestTriggered = false;
                    menu.Reset();
                }

                if(GetTime() > game.gameOverEndTime){
                    if(!showLeaderboardEntry){
                        showLeaderboardEntry = true;
                        // Update character-specific high score if applicable
                        if (game.score > menu.characterHighscores[game.GetSelectedCharacterIndex()]) {
                            characterNewBestTriggered = true;
                            menu.UpdateCharacterHighscore(game.GetSelectedCharacterIndex(), game.score);
                            game.SpawnConfetti(); // Start the celebration!
                        }
                        Leaderboards::Init(game.score, game.score >= game.highscore);
                    }

                    Leaderboards::Update();
                    Leaderboards::Draw(font);
                    
                    if (characterNewBestTriggered) {
                        float pulse = (sinf((float)GetTime() * 8.0f) + 1.0f) * 0.5f;
                        std::string bestMsg = "NEW CHARACTER BEST!";
                        Vector2 bestSize = MeasureTextEx(font, bestMsg.c_str(), 28, 2);
                        DrawTextEx(font, bestMsg.c_str(), {(float)(windowWidth + offset - bestSize.x)/2, 110.0f}, 28, 2, ColorLerp(GOLD, YELLOW, pulse));
                    }
                    
                    if(Leaderboards::WantReturn()){
                        showLeaderboardEntry = false;
                        inMenu = true;
                        characterNewBestTriggered = false;
                        menu.Reset();
                        game.Reset();
                        game.InitGame();
                    }
                }
            }

            if(returningToMenu && !showLeaderboardEntry){
                double fadeElapsed = GetTime() - returnStartTime;
                float fadeAlpha = float(fadeElapsed / returnFadeDuration);
                if(fadeAlpha > 1.0f) fadeAlpha = 1.0f;
                DrawRectangle(0, 0, windowWidth + offset, windowHeight + 2 * offset, Fade(BLACK, fadeAlpha));
                if(fadeElapsed >= returnFadeDuration){
                    returningToMenu = false;
                    inMenu = true;
                    menu.Reset();
                    game.Reset();
                    game.InitGame();
                }
            }
            EndDrawing();

        }
    }

    UnloadShader(distShader);
    UnloadShader(silhouetteShader);
    UnloadTexture(lifeIcon);
    UnloadRenderTexture(target);

    CloseWindow();
    CloseAudioDevice();
}