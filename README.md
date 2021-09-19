![Banner](Bloopair.png?raw=true)
# Bloopair
Bloopair allows connecting controllers from other consoles like native Wii U Pro Controllers on the Wii U.  
It temporarily applies patches to the IOS-PAD module responsible for Bluetooth controller connections.

## Features
- Connect up to 4 controllers wirelessly via Bluetooth
- Rumble support
- Battery levels

## Supported controllers
- Nintendo Switch Pro Controller
- Nintendo Switch Joy-Con
- Microsoft Xbox One S/X Controller
- Sony Dualsense Controller

## Installation
- Download the latest .zip from the [releases page](https://github.com/GaryOderNichts/Bloopair/releases)
- Extract it to the root of your SD Card

## Usage
- Run Bloopair from the Homebrew Launcher  
Once launched, the Wii U menu should open
- Once back in the Wii U menu, press the SYNC button on your console and controller
- Wait until the Controller is connected

If a controller had been paired in the past, simply turn it on again and it should reconnect.  
After rebooting the console or exiting System Settings, relaunch Bloopair.

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
- Determine controller based on vendor and product ID instead of controller name
- Rumble for Joy-Con
- Battery levels for Switch Pro Controller and Joy-Con
- Bluetooth LE support (the Wii U's bluetooth stack seems to support this?)

## How it works
Bloopair will patch the IOSU's IOS-PAD module in memory. It will make sure any bluetooth peripheral can be paired to the console.  
Once paired and connected it will convert received HID reports to the Pro Controller HID report format, which padscore expects.

## Building
Install devkitPPC, devkitARM and wut.  
Run `make`.
