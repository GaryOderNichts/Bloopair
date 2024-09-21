#include "LanguageSelectionScreen.hpp"
#include "Gfx.hpp"

LanguageSelectionScreen::LanguageSelectionScreen()
 : mSelectedLanguage(0)  // Standardmäßig die erste Sprache auswählen
{
    // Liste der verfügbaren Sprachen
    mLanguages = {
        { "English", "en" },
        { "Deutsch", "de" },
    };
}

LanguageSelectionScreen::~LanguageSelectionScreen()
{
}

void LanguageSelectionScreen::Draw()
{
    DrawTopBar("Select Language");

    // Zeige die Sprachoptionen
    for (size_t i = 0; i < mLanguages.size(); i++) {
        int yOff = 75 + i * 150;
        Gfx::DrawRectFilled(0, yOff, Gfx::SCREEN_WIDTH, 150, Gfx::COLOR_ALT_BACKGROUND);
        Gfx::Print(Gfx::SCREEN_WIDTH / 2, yOff + 75, 60, Gfx::COLOR_TEXT, mLanguages[i].name.c_str(), Gfx::ALIGN_CENTER);

        if (i == mSelectedLanguage) {
            Gfx::DrawRect(0, yOff, Gfx::SCREEN_WIDTH, 150, 8, Gfx::COLOR_HIGHLIGHTED);
        }
    }

    DrawBottomBar("\ue001 Back", nullptr, "\ue000 Select");
}

bool LanguageSelectionScreen::Update(const CombinedInputController& input)
{
    if (input.GetButtonsTriggered() & Controller::BUTTON_B) {
        return false;  // Zurück zum vorherigen Bildschirm
    }

    // Navigiere durch die Sprachliste
    if (input.GetButtonsTriggered() & Controller::BUTTON_DOWN) {
        if (mSelectedLanguage < static_cast<int>(mLanguages.size()) - 1) {
            mSelectedLanguage++;
        }
    } else if (input.GetButtonsTriggered() & Controller::BUTTON_UP) {
        if (mSelectedLanguage > 0) {
            mSelectedLanguage--;
        }
    }

    // Wenn die A-Taste gedrückt wird, wähle die Sprache aus
    if (input.GetButtonsTriggered() & Controller::BUTTON_A) {
        const LanguageOption& selectedLanguage = mLanguages[mSelectedLanguage];
        // Hier könntest du den Sprachcode speichern und die Sprache wechseln
        // z.B. Configuration::SetLanguage(selectedLanguage.code);
        // Rückkehr zum vorherigen Bildschirm
        return false;
    }

    return true;
}
