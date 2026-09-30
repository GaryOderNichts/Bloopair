#include <bloopair/bloopair.h>
#include <bloopair/nintendont_pairing.h>
#include <bloopair/nintendont_sync.h>

#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <malloc.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wups.h>

#include <atomic>
#include <cstdio>
#include <cstring>

WUPS_PLUGIN_NAME("Bloopair Nintendont sync");
WUPS_PLUGIN_DESCRIPTION("Keeps original Switch Pro pairings available to Nintendont");
WUPS_PLUGIN_VERSION("0.2.0");
WUPS_PLUGIN_AUTHOR("Bloopair contributors");
WUPS_PLUGIN_LICENSE("GPLv2");
WUPS_USE_WUT_DEVOPTAB();

namespace {
constexpr const char* kDirectory = "fs:/vol/external01/wiiu/bloopair";
constexpr const char* kPath = "fs:/vol/external01/wiiu/bloopair/nintendont-switch-pro.bin";
constexpr const char* kTemporaryPath = "fs:/vol/external01/wiiu/bloopair/nintendont-switch-pro.tmp";
constexpr const char* kBackupPath = "fs:/vol/external01/wiiu/bloopair/nintendont-switch-pro.bak";
constexpr uint32_t kStackSize = 16 * 1024;
constexpr uint32_t kGenerationCheckMilliseconds = 100;

OSThread* gThread = nullptr;
void* gStack = nullptr;
std::atomic_bool gStop{false};

bool ReadRecordAt(const char* path, NintendontSwitchPairing& record) {
    FILE* file = std::fopen(path, "rb");
    if (!file) return false;
    const bool ok = std::fread(&record, 1, sizeof(record), file) == sizeof(record) &&
                    std::fgetc(file) == EOF;
    std::fclose(file);
    return ok && NintendontSwitchPairingIsValid(&record);
}

bool ReadRecord(NintendontSwitchPairing& record) {
    if (ReadRecordAt(kPath, record)) return true;
    if (!ReadRecordAt(kBackupPath, record)) return false;
    unlink(kPath);
    rename(kBackupPath, kPath);
    return true;
}

bool WriteRecord(const NintendontSwitchPairing& record) {
    mkdir("fs:/vol/external01/wiiu", 0777);
    mkdir(kDirectory, 0777);
    FILE* file = std::fopen(kTemporaryPath, "wb");
    if (!file) return false;
    bool ok = std::fwrite(&record, 1, sizeof(record), file) == sizeof(record) &&
              std::fflush(file) == 0 && fsync(fileno(file)) == 0;
    std::fclose(file);
    if (!ok) {
        unlink(kTemporaryPath);
        return false;
    }
    unlink(kBackupPath);
    const bool hadActive = rename(kPath, kBackupPath) == 0;
    if (rename(kTemporaryPath, kPath) != 0) {
        if (hadActive) rename(kBackupPath, kPath);
        unlink(kTemporaryPath);
        return false;
    }
    if (hadActive) unlink(kBackupPath);
    return true;
}

bool ReadGeneration(uint32_t& generation) {
    IOSHandle handle = Bloopair_Open();
    if (handle < 0 || !Bloopair_IsActive(handle)) {
        if (handle >= 0) Bloopair_Close(handle);
        return false;
    }
    const bool ok = Bloopair_GetPairingChangeGeneration(handle, &generation) >= 0;
    Bloopair_Close(handle);
    return ok;
}

void SyncOnce() {
    IOSHandle handle = Bloopair_Open();
    if (handle < 0 || !Bloopair_IsActive(handle)) {
        if (handle >= 0) Bloopair_Close(handle);
        return;
    }
    BloopairStoredSwitchProList stored{};
    uint8_t consoleBda[6]{};
    if (Bloopair_ReadConsoleBDA(handle, consoleBda) < 0 ||
        Bloopair_GetStoredSwitchProControllers(handle, &stored) < 0) {
        Bloopair_Close(handle);
        return;
    }

    NintendontSwitchPairing previous{};
    const bool previousValid = ReadRecord(previous) &&
                               std::memcmp(previous.console_bda, consoleBda, 6) == 0;
    NintendontSwitchPairing next{};
    next.magic = NINTENDONT_SWITCH_PAIRING_MAGIC;
    next.version = NINTENDONT_SWITCH_PAIRING_VERSION;
    next.size = sizeof(next);
    std::memcpy(next.console_bda, consoleBda, sizeof(next.console_bda));

    bool captured = false;
    for (uint8_t i = 0; i < stored.count && i < BLOOPAIR_MAX_STORED_SWITCH_PROS; i++) {
        BloopairControllerPairingData source{};
        if (Bloopair_GetControllerPairingByAddress(handle,
                stored.controllers[i].bd_address, &source) >= 0) {
            NintendontSyncAddOrReplace(&next, &source);
            captured = true;
        }
    }
    if (previousValid) {
        for (uint8_t i = 0; i < previous.count &&
                next.count < NINTENDONT_SWITCH_PAIRING_MAX_CONTROLLERS; i++) {
            const auto& old = previous.controllers[i];
            if (!NintendontSyncIsStored(&stored, old.controller_bda)) continue;
            if (NintendontSyncHasAddress(&next, old.controller_bda)) continue;
            BloopairControllerPairingData source{};
            std::memcpy(source.bd_address, old.controller_bda, 6);
            std::memcpy(source.hci_link_key, old.hci_link_key, 16);
            source.key_type = old.key_type;
            source.controller_type = old.controller_type;
            source.vendor_id = old.vendor_id;
            source.product_id = old.product_id;
            NintendontSyncAddOrReplace(&next, &source);
        }
    }
    Bloopair_Close(handle);

    if (!captured && !previousValid) return;
    if (next.count == 0) {
        unlink(kPath);
        return;
    }
    next.checksum = NintendontSwitchPairingChecksum(&next);
    if (previousValid && std::memcmp(&previous, &next, sizeof(next)) == 0) return;
    WriteRecord(next);
}

void SyncUntilStable() {
    for (;;) {
        uint32_t before = 0;
        uint32_t after = 0;
        const bool haveBefore = ReadGeneration(before);
        SyncOnce();
        const bool haveAfter = ReadGeneration(after);
        if (!haveBefore || !haveAfter || before == after) return;
    }
}

int32_t SyncThread([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
    uint32_t observedGeneration = 0;
    SyncUntilStable();
    ReadGeneration(observedGeneration);
    while (!gStop.load()) {
        uint32_t generation = observedGeneration;
        if (ReadGeneration(generation) && generation != observedGeneration) {
            SyncUntilStable();
            ReadGeneration(observedGeneration);
        }
        OSSleepTicks(OSMillisecondsToTicks(kGenerationCheckMilliseconds));
    }
    return 0;
}

void StartThread() {
    if (gThread) return;
    gStop.store(false);
    gThread = static_cast<OSThread*>(memalign(8, sizeof(OSThread)));
    gStack = memalign(0x20, kStackSize);
    if (!gThread || !gStack ||
        !OSCreateThread(gThread, SyncThread, 0, nullptr,
            static_cast<uint8_t*>(gStack) + kStackSize, kStackSize, 31,
            OS_THREAD_ATTRIB_AFFINITY_ANY)) {
        free(gThread); free(gStack); gThread = nullptr; gStack = nullptr;
        return;
    }
    OSSetThreadName(gThread, "Bloopair Nintendont sync");
    OSResumeThread(gThread);
}

void StopThread() {
    if (!gThread) return;
    gStop.store(true);
    OSJoinThread(gThread, nullptr);
    free(gThread); free(gStack); gThread = nullptr; gStack = nullptr;
}
}

INITIALIZE_PLUGIN() {}
DEINITIALIZE_PLUGIN() { StopThread(); }
ON_APPLICATION_START() { StartThread(); }
ON_APPLICATION_REQUESTS_EXIT() { StopThread(); SyncUntilStable(); }
