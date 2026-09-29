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
        "Connect up to four original Switch Pro Controllers through Bloopair.\n"
        "Press A to export their current pairings for Nintendont.\n\n"
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
    ControllerManager& manager = ControllerManager::Get();
    auto consoleBda = BloopairIPC::ReadConsoleBDA();
    if (!consoleBda) {
        mMessageBox = std::make_unique<MessageBox>(
            "Export failed",
            "The Wii U Bluetooth address could not be read.",
            std::vector{MessageBox::Option{0, "\ue000 Ok", []() {}}});
        return;
    }

    NintendontSwitchPairing pairing{};
    pairing.magic = NINTENDONT_SWITCH_PAIRING_MAGIC;
    pairing.version = NINTENDONT_SWITCH_PAIRING_VERSION;
    pairing.size = sizeof(pairing);
    memcpy(pairing.console_bda, consoleBda->data(), sizeof(pairing.console_bda));
    for (size_t i = 0; i < ControllerManager::kMaxKPADControllers &&
            pairing.count < NINTENDONT_SWITCH_PAIRING_MAX_CONTROLLERS; i++) {
        const KPADController& controller = manager.GetKPADController(i);
        if (!controller.IsConnected() || !controller.IsBloopairController() ||
            controller.GetControllerType() != BLOOPAIR_CONTROLLER_SWITCH_PRO) {
            continue;
        }
        BloopairControllerPairingData source{};
        if (!BloopairIPC::GetControllerPairing(controller.GetChannel(), source)) {
            continue;
        }
        NintendontSwitchPairingEntry& entry = pairing.controllers[pairing.count++];
        memcpy(entry.controller_bda, source.bd_address, sizeof(entry.controller_bda));
        memcpy(entry.hci_link_key, source.hci_link_key, sizeof(entry.hci_link_key));
        entry.key_type = source.key_type;
        entry.controller_type = source.controller_type;
        entry.vendor_id = source.vendor_id;
        entry.product_id = source.product_id;
    }
    if (pairing.count == 0) {
        mMessageBox = std::make_unique<MessageBox>(
            "Export failed",
            "Connect at least one original Switch Pro Controller through Bloopair.",
            std::vector{MessageBox::Option{0, "\ue000 Ok", []() {}}});
        return;
    }
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
        ok ? "Nintendont can now use the exported Bloopair pairings."
           : "The pairing record could not be written to the SD card.",
        std::vector{MessageBox::Option{0, "\ue000 Ok", []() {}}});
}
