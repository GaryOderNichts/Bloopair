#include <bloopair/bloopair.h>
#include <bloopair/nintendont_pairing.h>
#include <bloopair/nintendont_sync.h>
#include <bloopair/nintendont_sync_state.h>

#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <malloc.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wups.h>

#include <atomic>
#include <cerrno>
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
constexpr uint32_t kGenerationCheckMilliseconds = 500;
constexpr uint32_t kMaximumRetryMilliseconds = 8000;
constexpr uint32_t kExitAttempts = 3;

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

bool ReadBestRecord(NintendontSwitchPairing& record, bool& activeValid) {
    activeValid = ReadRecordAt(kPath, record);
    if (activeValid) return true;
    return ReadRecordAt(kBackupPath, record);
}

bool WriteRecord(const NintendontSwitchPairing& record) {
    if (mkdir("fs:/vol/external01/wiiu", 0777) != 0 && errno != EEXIST) return false;
    if (mkdir(kDirectory, 0777) != 0 && errno != EEXIST) return false;
    FILE* file = std::fopen(kTemporaryPath, "wb");
    if (!file) return false;
    bool ok = std::fwrite(&record, 1, sizeof(record), file) == sizeof(record);
    if (ok) ok = std::fflush(file) == 0;
    if (ok) ok = fsync(fileno(file)) == 0;
    if (std::fclose(file) != 0) ok = false;
    if (!ok) {
        unlink(kTemporaryPath);
        return false;
    }
    if (unlink(kBackupPath) != 0 && errno != ENOENT) {
        unlink(kTemporaryPath);
        return false;
    }
    bool hadActive = rename(kPath, kBackupPath) == 0;
    if (!hadActive && errno != ENOENT) {
        unlink(kTemporaryPath);
        return false;
    }
    if (rename(kTemporaryPath, kPath) != 0) {
        if (hadActive && rename(kBackupPath, kPath) != 0) {
            /* Keep both paths untouched from here; a later retry can recover. */
        }
        unlink(kTemporaryPath);
        return false;
    }
    if (hadActive && unlink(kBackupPath) != 0 && errno != ENOENT) return false;
    return true;
}

bool RemoveExport() {
    bool ok = true;
    if (unlink(kPath) != 0 && errno != ENOENT) ok = false;
    if (unlink(kTemporaryPath) != 0 && errno != ENOENT) ok = false;
    if (unlink(kBackupPath) != 0 && errno != ENOENT) ok = false;
    return ok;
}

bool ReadGeneration(IOSHandle handle, uint32_t& generation) {
    return Bloopair_GetPairingChangeGeneration(handle, &generation) >= 0;
}

bool SyncOnce(IOSHandle handle) {
    BloopairStoredSwitchProList stored{};
    uint8_t consoleBda[6]{};
    if (Bloopair_ReadConsoleBDA(handle, consoleBda) < 0 ||
        Bloopair_GetStoredSwitchProControllers(handle, &stored) < 0) {
        return false;
    }

    NintendontSwitchPairing previous{};
    bool activeValid = false;
    const bool previousValid = ReadBestRecord(previous, activeValid) &&
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
    if (!captured && !previousValid && stored.count != 0) return false;
    if (next.count == 0) {
        return RemoveExport();
    }
    next.checksum = NintendontSwitchPairingChecksum(&next);
    if (activeValid && previousValid && std::memcmp(&previous, &next, sizeof(next)) == 0) {
        bool clean = true;
        if (unlink(kTemporaryPath) != 0 && errno != ENOENT) clean = false;
        if (unlink(kBackupPath) != 0 && errno != ENOENT) clean = false;
        return clean;
    }
    return WriteRecord(next);
}

bool SyncUntilStable(IOSHandle handle, uint32_t maximumAttempts,
                     uint32_t& committedGeneration) {
    for (uint32_t attempt = 0; attempt < maximumAttempts; attempt++) {
        uint32_t before = 0;
        uint32_t after = 0;
        if (!ReadGeneration(handle, before)) return false;
        if (!SyncOnce(handle)) return false;
        if (!ReadGeneration(handle, after)) return false;
        if (before == after) {
            committedGeneration = after;
            return true;
        }
    }
    return false;
}

int32_t SyncThread([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
    IOSHandle handle = Bloopair_Open();
    if (handle < 0 || !Bloopair_IsActive(handle)) {
        if (handle >= 0) Bloopair_Close(handle);
        return 0;
    }
    uint32_t initialGeneration = 1;
    ReadGeneration(handle, initialGeneration);
    NintendontSyncState state{};
    NintendontSyncStateInit(&state, initialGeneration, kGenerationCheckMilliseconds);
    while (!gStop.load()) {
        uint32_t generation = state.pending_generation;
        if (ReadGeneration(handle, generation)) NintendontSyncStateObserve(&state, generation);
        if (NintendontSyncStatePending(&state)) {
            uint32_t committed = 0;
            if (SyncUntilStable(handle, 3, committed)) {
                NintendontSyncStateCommitted(&state, committed,
                    kGenerationCheckMilliseconds);
                if (ReadGeneration(handle, generation))
                    NintendontSyncStateObserve(&state, generation);
            } else {
                NintendontSyncStateFailed(&state, kMaximumRetryMilliseconds);
            }
        }
        uint32_t slept = 0;
        const uint32_t delay = NintendontSyncStatePending(&state) ?
            state.retry_milliseconds : kGenerationCheckMilliseconds;
        while (!gStop.load() && slept < delay) {
            OSSleepTicks(OSMillisecondsToTicks(kGenerationCheckMilliseconds));
            slept += kGenerationCheckMilliseconds;
        }
    }
    Bloopair_Close(handle);
    return 0;
}

void FlushOnExit() {
    IOSHandle handle = Bloopair_Open();
    if (handle < 0 || !Bloopair_IsActive(handle)) {
        if (handle >= 0) Bloopair_Close(handle);
        return;
    }
    uint32_t committed = 0;
    SyncUntilStable(handle, kExitAttempts, committed);
    Bloopair_Close(handle);
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
ON_APPLICATION_REQUESTS_EXIT() { StopThread(); FlushOnExit(); }
