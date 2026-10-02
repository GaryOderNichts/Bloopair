# Bloopair BLE Extensions (bleext)

**Bloopair BLE extensions:** Extensions on top of the existing minimal BLE support in the Bluetooth stack to get BLE controllers working.  

These extensions include:
- Helpers for the ATT protocol
- A limited and minimal GATT client
- Basic HID over GATT implementation
- A basic SMP implementation for pairing and bonding support
- Special "HID" implementation for Switch 2 controllers

## TODO
- Battery

## References
- https://www.bluetooth.com/specifications/specs/core-specification-4-0/
- https://www.bluetooth.com/specifications/specs/hid-over-gatt-profile-1-0/
- https://www.bluetooth.com/specifications/specs/human-interface-device-service-1-0/
- https://www.bluetooth.com/specifications/specs/bas-1-1/
- https://github.com/darthcloud/BlueRetro/blob/e1a9831a875f5313a923160a1379a7ebbfaa2b11/main/bluetooth/att_hid.c
- https://github.com/darthcloud/BlueRetro/blob/e1a9831a875f5313a923160a1379a7ebbfaa2b11/main/bluetooth/smp.c
