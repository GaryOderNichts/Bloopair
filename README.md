![Banner](Bloopair.png?raw=true)
# Bloopair
Bloopair allows connecting controllers from other consoles like native Wii U Pro Controllers on the Wii U.  
It temporarily applies patches to the IOS-PAD module responsible for Bluetooth controller connections.

## Features
- Connect up to 7 controllers wirelessly via Bluetooth
- Rumble support
- Battery levels
- Button and stick remapping (only for Bloopair controllers)

## Supported controllers
- Nintendo Switch Pro Controller
- Nintendo Switch Joy-Con
- Nintendo Switch Online SNES / N64 Controller
- Microsoft Xbox One S/X Controller  
Note: The latest firmware versions and all Series S/X Controllers are currently not supported due to missing Bluetooth LE support.
- Sony DualShock 3 Controller  
To pair a DualShock 3 to the console, see the [Pairing a DualShock 3](#pairing-a-dualshock-3) section.
- Sony DualShock 4 Controller
- Sony DualSense Controller

## Installation
- Download and extract the latest .zip from the [releases page](https://github.com/GaryOderNichts/Bloopair/releases).
- Copy the `30_bloopair.rpx` from the .zip file to the `modules/setup/` folder of your target environment on the SD Card.  
  This would be `wiiu/environments/aroma/modules/setup/` for Aroma.
- Copy the `wiiu` folder from the .zip and copy it to the root of your SD Card.  
  If you're using aroma you can delete the `Koopair.rpx` in the `wiiu/apps` folder and use the .wuhb instead.

Make sure you're using Aroma or Tiramisu. Follow https://wiiu.hacks.guide/#/ to setup Aroma.

## Usage
- Once you're booted into Aroma or Tiramisu and are in the Wii U menu, press the SYNC button on your console and controller.
- Wait until the Controller is connected.

If a controller had been paired in the past, simply turn it on again and it should reconnect.

## Koopair
Koopair is the Bloopair companion app which comes with Bloopair.  

<img src="https://i.imgur.com/w4CaDXL.png" width="23%"></img> <img src="https://i.imgur.com/ugzZorg.png" width="23%"></img> <img src="https://i.imgur.com/Zf3XoBZ.png" width="23%"></img> <img src="https://i.imgur.com/VUnR3S8.png" width="23%"></img>
<img src="https://i.imgur.com/E4CqggT.png" width="23%"></img> <img src="https://i.imgur.com/eaych2U.png" width="23%"></img> <img src="https://i.imgur.com/CFLP3WL.png" width="23%"></img> <img src="https://i.imgur.com/7GbLf2P.png" width="23%"></img>

Koopair supports:
- Testing connected controllers
- Creating mappings for buttons and sticks
- Editing controller options
- Managing configuration files
- Pairing DualShock 3 controllers

## Pairing a DualShock 3
The DualShock 3 needs to be paired using a USB cable. After the initial pairing it can be used like any other wireless Bluetooth controller.  
- Open Koopair from the Wii U menu or Homebrew Launcher. Now open the "Controller Pairing" option on the menu.
- Connect the DualShock 3 using a USB cable to the front or back ports of the console.
- The screen will say "Successfully paired controller!" once the controller has been successfully paired.  
You can now remove the USB cable from the controller. Press the PS button to connect it to the console.
- Press the HOME button to exit.

The DualShock 3 is now ready to use with the console.

## FAQ / Troubleshooting

**My controller doesn't pair to the console**  
Make sure Bloopair is running and both the console and the controller are in SYNC mode.  
Also make sure the controller is on the supported list.  
Wait for about a minute, and if nothing happens restart your console and redo the process.  
You can also try [clearing controller syncs](https://en-americas-support.nintendo.com/app/answers/detail/a_id/1705/~/how-to-clear-all-syncs).

**Will you add support for controller xyz?**  
Possibly, I've for now added support for all the controllers I currently own. Maybe I can get a few more controllers which I could add support for.  
Pull requests for different controllers are always welcome.

**Where are configuration files stored?**
Bloopair loads configuration files from the `wiiu/bloopair` folder on your SD Card.  
This means configurations work across multiple environments.

## To-Do
- Support more controllers
- Bluetooth LE support (Unlikely, only partially supported by the Bluetooth Stack)

## How it works
Bloopair will patch the IOSU's IOS-PAD module in memory. It will make sure any bluetooth peripheral can be paired to the console.  
Once paired and connected it will convert received HID reports to the Pro Controller HID report format, which padscore expects.

## Project structure
```
Bloopair
├── dist            - Used for creating distributable packages.
├── ios             - Bloopair IOSU patches.
│   ├── ios_kernel  - Kernel patches used for setting up Bloopair.
│   ├── ios_pad     - Core Bloopair patches.
│   └── ios_usb     - Patches used to recover from IOS exploit done by loader.
├── koopair         - Bloopair companion app.
├── libbloopair     - Library to communicate with Bloopair IPC.
├── loader          - Setup module which loads Bloopair.
└── third_party     - Third-party content included in Bloopair.
```

## Building
Install devkitPPC, devkitARM and wut.

**Koopair dependencies**  
Koopair additionally requires the following packages:
- wiiu-sdl2
- wiiu-sdl2_gfx
- wiiu-sdl2_ttf
- wiiu-sdl2_image

Run `make`.
