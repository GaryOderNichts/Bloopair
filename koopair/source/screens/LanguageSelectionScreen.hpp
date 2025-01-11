/*
 *   LanguageSelectionScreen.hpp
 *   Defines the page for language selection.
 */

#ifndef LANGUAGESELECTIONSCREEN_HPP
#define LANGUAGESELECTIONSCREEN_HPP

#include "Screen.hpp"
#include <vector>
#include <string>

class LanguageSelectionScreen : public Screen
{
public:
    LanguageSelectionScreen();
    virtual ~LanguageSelectionScreen();

    void Draw() override;
    bool Update(const CombinedInputController& input) override;

private:
    struct LanguageOption {
        std::string name;
        std::string code;
    };

    std::vector<LanguageOption> mLanguages;
    int mSelectedLanguage;
};

#endif // LANGUAGESELECTIONSCREEN_HPP
