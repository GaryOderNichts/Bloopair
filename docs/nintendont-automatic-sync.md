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
3. `bloopair_nintendont_sync.wps` polls bounded IPC every five seconds.
4. A metadata-only IPC call enumerates original Switch Pro addresses still in
   the Wii U device table. A second call returns a cached key only for one of
   those validated addresses.
5. The plugin merges up to four entries, replaces re-paired keys, removes
   deleted devices, and writes a checksummed v3 record by temp/backup/rename.
6. Nintendont validates the record and uses it after entering vWii.

The code proves the bounded merge, stale-entry filtering, atomic replacement
and absence of secret logging. Hardware must still prove WUPS lifecycle timing,
fresh pairing, cold restart, re-pairing, removal and two-controller reconnect.
