#pragma once

#include "Screen.hpp"

#include <memory>

class MessageBox;

class NintendontPairingScreen : public Screen
{
public:
    NintendontPairingScreen();
    virtual ~NintendontPairingScreen();

    void Draw();
    bool Update(const CombinedInputController& input);

private:
    void ExportPairing();

    std::unique_ptr<MessageBox> mMessageBox;
};

