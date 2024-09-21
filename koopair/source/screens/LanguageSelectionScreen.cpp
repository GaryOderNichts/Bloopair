#include "LanguageSelectionScreen.hpp"
#include "Gfx.hpp"

LanguageSelectionScreen::LanguageSelectionScreen()
 : mSelectedLanguage(0)  // Select the first language by default
{
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
        return false;
    }

    if (input.GetButtonsTriggered() & Controller::BUTTON_DOWN) {
        if (mSelectedLanguage < static_cast<int>(mLanguages.size()) - 1) {
            mSelectedLanguage++;
        }
    } else if (input.GetButtonsTriggered() & Controller::BUTTON_UP) {
        if (mSelectedLanguage > 0) {
            mSelectedLanguage--;
        }
    }

if (input.GetButtonsTriggered() & Controller::BUTTON_B) {
        return false;
    }

    if (input.GetButtonsTriggered() & Controller::BUTTON_DOWN) {
        mSelectedLanguage = (mSelectedLanguage + 1) % mLanguages.size();
    } else if (input.GetButtonsTriggered() & Controller::BUTTON_UP) {
        mSelectedLanguage = (mSelectedLanguage - 1 + mLanguages.size()) % mLanguages.size();
    }

    if (input.GetButtonsTriggered() & Controller::BUTTON_A) {
        const LanguageOption& selectedLanguage = mLanguages[mSelectedLanguage];
        Configuration::SetLanguage(selectedLanguage.code);
        return false;
    }

    return true;
}
