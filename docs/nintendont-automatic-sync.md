# Automatic Nintendont pairing handoff

## Source-proven constraints

- Bloopair is installed as an EnvironmentLoader setup module and patches the
  Wii U IOSU IOS-PAD process (`loader/main.cpp`, `ios/ios_pad/source/main.c`).
- Its API is an extension of Wii U `/dev/usb/btrm`
  (`libbloopair/source/bloopair.c`). Nintendont's loader runs after the vWii
  transition under IOS58 and has no Bloopair IPC client; it reads SD through
  FatFS (`Nintendont/loader/source/main.c`).
- The Wii U native device table at `0x12157778` contains addresses and Bloopair
  type/VID/PID metadata, but normal entries contain no link key
  (`ios/ios_pad/source/info_store.h`).
- The required link key is in Broadcom's private `tBTM_SEC_DEV_REC`. Bloopair
  captures an already-resolved record during the normal security procedure and
  converts its key to HCI order (`pairing_export.c`, `stack/btm_sec.c`). It does
  not scan or log the private security database.
- Aroma integrates Wii U Plugin System and runs plugins with Wii U filesystem
  and Bloopair IPC access. Tiramisu is a setup-module environment and provides
  no WUPS runtime (the respective upstream Aroma and Tiramisu READMEs).

## Considered routes

| Route | User experience | Reliability / maintenance | Key exposure | Changes |
| --- | --- | --- | --- | --- |
| Nintendont reads IOSU directly | Ideal in theory | Not available after vWii transition; would couple Nintendont to unsupported IOSU internals | High if private memory were scanned | Nintendont only, rejected |
| Reuse vWii pairing database | No extra step | It does not contain Bloopair's Wii U pairing or controller protocol metadata | None gained | Insufficient |
| Setup-loader snapshot | Automatic at boot | Runs before pairings made later; cached key may not yet be resolved | Local | Bloopair only, incomplete |
| IOS-PAD writes SD | Automatic | IOS-PAD has no established filesystem lifecycle here; adds privileged filesystem code to the patch | Broad IOSU responsibility | Bloopair only, rejected |
| Aroma background sync | Automatic after pairing | Uses supported WUPS filesystem lifecycle and bounded Bloopair IPC; survives vWii via local handoff | Link keys only in local record; never logged | Bloopair + existing Nintendont reader, selected |
| Forwarder per game | Extra launch constraint | Does not cover all Nintendont launchers | Local | Additional project, rejected |

One upstream PR cannot cleanly contain the complete feature because Bloopair
owns Wii U pairing and Nintendont owns vWii authentication/input. The selected
design therefore needs one change in each project. The Nintendont side remains
generic: it consumes a versioned, validated handoff record and has no Aroma or
Bloopair runtime dependency.

## Selected stateflow

1. Bloopair pairs or reconnects an original Switch Pro Controller.
2. Its existing security hook captures that controller's resolved link key.
3. `pairing_export_capture_security_record()` retains the security-record
   pointer reached from `btm_sec_execute_procedure_hook()`. The link key may be
   filled later by the Broadcom stack, so Bloopair derives a non-secret,
   monotonic generation from the current stored-device metadata and the
   availability/content of each cached key. `store_read_DI_record()` refreshes
   it after Switch VID/PID discovery; `writeDevInfo_hook()` refreshes it after
   the native device table accepts a write, covering normal pairing and
   removal. A changed key changes the generation and therefore covers re-pair.
4. `/dev/usb/btrm` offers synchronous request/response IPC, not a push channel.
   Holding an ioctl open as a wait primitive would block the same BTRM service
   that must finish security and HID work. The plugin therefore reads only the
   32-bit generation every 500 ms through one worker-owned handle. This is the
   remaining technical polling;
   it performs no enumeration and no filesystem I/O while unchanged.
5. On plugin/application start, and only after a generation change, a
   metadata-only IPC call enumerates original Switch Pro addresses still in
   the Wii U device table. A second call returns a cached key only for one of
   those validated addresses.
6. The plugin merges up to four entries, replaces re-paired keys, removes
   deleted devices, and writes a checksummed v3 record only when its bytes
   changed. It writes a temporary file, calls `fflush()` and `fsync()`, closes
   it, then activates it by backup/rename.
   Write, flush, fsync, close, backup removal and both renames are checked.
   A failed operation leaves the generation pending and retries with bounded
   exponential backoff (500 ms through 8 seconds). The last valid active or
   backup record is retained whenever activation fails.
7. `ON_APPLICATION_REQUESTS_EXIT()` first joins the worker, then synchronously
   repeats `generation before -> complete sync/write -> generation after`
   until both generations match, with at most three attempts. The hook never
   waits indefinitely for a missing, full or read-only SD card; on failure the
   generation remains uncommitted for a later application start. This bounded
   barrier is the boundary that covers pairing and immediately launching
   vWii/Nintendont when storage is healthy.
8. Nintendont validates the fixed
   `sd:/wiiu/bloopair/nintendont-switch-pro.bin` record when it starts and uses
   it after entering vWii.

The code proves generation changes for a new key, key replacement and removal;
bounded merge, stale-entry filtering, changed-content-only writes, durable
temporary-file completion, atomic replacement and absence of secret logging.
Hardware must still prove WUPS lifecycle timing, fresh pairing followed by an
immediate vWii launch, cold restart, re-pairing, removal and two-controller
reconnect.
