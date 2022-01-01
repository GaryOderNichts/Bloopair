![Banner](Bloopair.png?raw=true)
# Bloopair
Bloopair allows connecting controllers from other consoles like native Wii U Pro Controllers on the Wii U.  
It temporarily applies patches to the IOS-PAD module responsible for Bluetooth controller connections.

## Features
- Connect up to 7 controllers wirelessly via Bluetooth
- Rumble support
- Battery levels

## Supported controllers
- Nintendo Switch Pro Controller
- Nintendo Switch Joy-Con
- Nintendo Switch Online SNES / N64 Controller
- Microsoft Xbox One S/X Controller  
Note: The Series S/X Controllers are currently not supported due to missing Bluetooth LE support
- Sony DualShock 3 Controller  
To pair a DualShock 3 to the console, see the [Pairing a DualShock 3](#pairing-a-dualshock-3) section
- Sony DualShock 4 Controller
- Sony DualSense Controller

## Installation
- Download the latest .zip from the [releases page](https://github.com/GaryOderNichts/Bloopair/releases)
- Extract it to the root of your SD Card

Make sure you're using Tiramisu. Follow https://wiiu.hacks.guide/#/ to setup Tiramisu.  
More info about Tiramisu here: https://maschell.github.io/homebrew/2021/12/31/tiramisu.html

## Usage
- Once you're booted into Tiramisu and are in the Wii U menu, press the SYNC button on your console and controller
- Wait until the Controller is connected

## Pairing a DualShock 3
The DualShock 3 needs to be paired using a USB cable. After the initial pairing it can be used like any other wireless Bluetooth controller.  
- Open the Bloopair pairing app from the Homebrew Launcher. You should now be in the USB pairing menu.
- Connect the DualShock 3 using a USB cable to the front or back ports of the console.
- The screen will say "Paired!" once the controller has been successfully paired.  
You can now remove the USB cable from the controller. Press the PS button once back on the Wii U menu to connect it to the console.
- Press the HOME button to exit to the HBL

The DualShock 3 is now ready to use with the console.

## FAQ / Troubleshooting

### My controller doesn't pair to the console
Make sure Bloopair is running and both the console and the controller are in SYNC mode.  
Also make sure the controller is on the supported list.  
Wait for about a minute, and if nothing happens restart your console and redo the process.  
You can also try [clearing controller syncs](https://en-americas-support.nintendo.com/app/answers/detail/a_id/1705/~/how-to-clear-all-syncs).

### Will you add support for controller xyz?
Possibly, I've for now added support for all the controllers I currently own. Maybe I can get a few more controllers which I could add support for.  
Pull requests for different controllers are always welcome.

## To-Do
- Support more controllers
- Bluetooth LE support (the Wii U's bluetooth stack seems to support this?)

## How it works
Bloopair will patch the IOSU's IOS-PAD module in memory. It will make sure any bluetooth peripheral can be paired to the console.  
Once paired and connected it will convert received HID reports to the Pro Controller HID report format, which padscore expects.

## Building
Install devkitPPC, devkitARM and wut.  
Run `make`.
