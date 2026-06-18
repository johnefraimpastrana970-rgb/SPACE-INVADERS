#pragma once
#include <raylib.h>
#include <vector>
#include <string>
#include "utils.hpp"

class Menu {
    public:
        Menu();
        ~Menu();
        void Draw();
        void HandleInput();
        bool GetStartGame() const;
        void Update();
        void Reset();
        void LoadCharacterHighscores();
        void SaveCharacterHighscores();
        void UpdateCharacterHighscore(int characterIndex, int newScore);
        int GetSelectedCharacter() const { return selectedCharacter; }
        std::vector<int> characterHighscores; // Store high scores for each character
        int lastRecordCharacterIdx = -1;      // Index of the character who just set a record
    private:
        struct CharacterOption {
            Texture2D texture;
            std::string name;
        };
        enum MenuState { MAIN, SETTINGS, LEADERBOARD, CREDITS, CHARACTER_SELECT, QUIT_CONFIRM };
        MenuState menuState = MAIN;
        int selectedOption; // 0 = Start, 1 = Settings, 2 = Leaderboard, 3 = Quit
        bool startGame;
        float pickFlashTimer = 0.0f;
        bool loading;
        int selectedCharacter = 0;
        std::vector<CharacterOption> characterSlots;
        double loadingEndTime;
        Font font;
        Sound selectSound;
        Utils::GifStream loadingIconStream;
        bool loadingIconLoaded;
        int currentLoadingIconFrame = 0;
        float loadingIconFrameTimer = 0.0f;
        float loadingIconFrameTime = 0.08f;
        Utils::GifStream backgroundStream;
        int currentBackgroundFrame = 0;
        float backgroundFrameTimer = 0.0f;
        float backgroundFrameTime = 0.08f;
        float masterVolume = 1.0f;
        int settingsSelectedOption = 0; // 0 = Volume, 1 = Credits, 2 = Return
        float menuTransitionTimer = 0.0f;
        const float transitionDuration = 0.5f;

        Utils::GifStream blopCatStream;
        int currentBlopCatFrame = 0;
        float blopCatFrameTimer = 0.0f;
        float blopCatFrameTime = 0.08f;

        int quitConfirmOption = 1; // 0 = Yes, 1 = No (Default to No for safety)
        std::string quitStatusMessage = "";
        float quitStatusTimer = 0.0f;
        bool pendingQuit = false;
};
