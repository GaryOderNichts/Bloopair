#include "NintendontPairingScreen.hpp"

#include "BloopairIPC.hpp"
#include "ControllerManager.hpp"
#include "Gfx.hpp"
#include "MessageBox.hpp"

#include <bloopair/nintendont_pairing.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace
{
constexpr const char* kPairingPath = NINTENDONT_SWITCH_PAIRING_PATH;
constexpr const char* kTemporaryPairingPath = "wiiu/bloopair/nintendont-switch-pro.tmp";
}

NintendontPairingScreen::NintendontPairingScreen() : mMessageBox()
{
}

NintendontPairingScreen::~NintendontPairingScreen()
{
}

void NintendontPairingScreen::Draw()
{
    DrawTopBar("Nintendont Pairing Export");
    Gfx::Print(Gfx::SCREEN_WIDTH / 2, Gfx::SCREEN_HEIGHT / 2, 54, Gfx::COLOR_TEXT,
        "Connect the original Switch Pro Controller through Bloopair.\n"
        "Press A to export its current pairing for Nintendont.\n\n"
        "The pairing remains local on the SD card.",
        Gfx::ALIGN_CENTER);
    DrawBottomBar("\ue001 Back", nullptr, "\ue000 Export");

    if (mMessageBox) {
        mMessageBox->Draw();
    }
}

bool NintendontPairingScreen::Update(const CombinedInputController& input)
{
    if (mMessageBox) {
        if (!mMessageBox->Update(input)) {
            mMessageBox.reset();
        }
        return true;
    }
    if (input.GetButtonsTriggered() & Controller::BUTTON_B) {
        return false;
    }
    if (input.GetButtonsTriggered() & Controller::BUTTON_A) {
        ExportPairing();
    }
    return true;
}

void NintendontPairingScreen::ExportPairing()
{
    const KPADController* selected = nullptr;
    ControllerManager& manager = ControllerManager::Get();
    for (size_t i = 0; i < ControllerManager::kMaxKPADControllers; i++) {
        const KPADController& controller = manager.GetKPADController(i);
        if (controller.IsConnected() && controller.IsBloopairController() &&
            controller.GetControllerType() == BLOOPAIR_CONTROLLER_SWITCH_PRO) {
            selected = &controller;
            break;
        }
    }

    BloopairControllerPairingData source{};
    auto consoleBda = BloopairIPC::ReadConsoleBDA();
    if (!selected || !consoleBda || !BloopairIPC::GetControllerPairing(selected->GetChannel(), source)) {
        mMessageBox = std::make_unique<MessageBox>(
            "Export failed",
            "Connect the Switch Pro Controller through Bloopair, then try again.",
            std::vector{MessageBox::Option{0, "\ue000 Ok", []() {}}});
        return;
    }

    NintendontSwitchPairing pairing{};
    pairing.magic = NINTENDONT_SWITCH_PAIRING_MAGIC;
    pairing.version = NINTENDONT_SWITCH_PAIRING_VERSION;
    pairing.size = sizeof(pairing);
    memcpy(pairing.controller_bda, source.bd_address, sizeof(pairing.controller_bda));
    memcpy(pairing.console_bda, consoleBda->data(), sizeof(pairing.console_bda));
    memcpy(pairing.link_key, source.link_key, sizeof(pairing.link_key));
    pairing.key_type = source.key_type;
    pairing.controller_type = source.controller_type;
    pairing.vendor_id = source.vendor_id;
    pairing.product_id = source.product_id;
    pairing.checksum = NintendontSwitchPairingChecksum(&pairing);

    std::filesystem::create_directories("wiiu/bloopair");
    std::ofstream file(kTemporaryPairingPath, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char*>(&pairing), sizeof(pairing));
    file.flush();
    bool ok = file.good();
    file.close();
    if (ok) {
        std::error_code error;
        std::filesystem::remove(kPairingPath, error);
        error.clear();
        std::filesystem::rename(kTemporaryPairingPath, kPairingPath, error);
        ok = !error;
    }
    if (!ok) {
        std::error_code ignored;
        std::filesystem::remove(kTemporaryPairingPath, ignored);
    }

    mMessageBox = std::make_unique<MessageBox>(
        ok ? "Export complete" : "Export failed",
        ok ? "Nintendont can now use this Bloopair pairing."
           : "The pairing record could not be written to the SD card.",
        std::vector{MessageBox::Option{0, "\ue000 Ok", []() {}}});
}
