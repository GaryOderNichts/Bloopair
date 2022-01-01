/*
 *   Copyright (C) 2021 GaryOderNichts
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <string>

#include <coreinit/foreground.h>
#include <coreinit/cache.h>
#include <coreinit/memorymap.h>
#include <coreinit/dynload.h>
#include <coreinit/debug.h>
#include <proc_ui/procui.h>
#include <sysapp/launch.h>
#include <padscore/wpad.h>

#include <chrono>
#include <condition_variable>

#include "ios_exploit.h"
#include "ipc.hpp"
#include "kernel.hpp"

int main(int argc, char **argv)
{
    IOSHandle btrmHandle = openBtrm();
    if (btrmHandle < 0) {
        OSFatal("Can't open btrm");
    }

    if (!isBloopairRunning(btrmHandle)) {
        KernelSetup();

        // nop out security level checks
        KernelWriteU32(((uint32_t) &OSDynLoad_GetRPLInfo) + (22 * 4), 0x60000000);
        KernelWriteU32(((uint32_t) &OSDynLoad_GetNumberOfRPLs) + (6 * 4), 0x60000000);

        int numRpls = OSDynLoad_GetNumberOfRPLs();
        if (numRpls <= 0) {
            OSFatal("OSDynLoad_GetNumberOfRPLs failed");
        }

        OSDynLoad_NotifyData moduleInfos[numRpls];
        if (!OSDynLoad_GetRPLInfo(0, numRpls, moduleInfos)) {
            OSFatal("OSDynLoad_GetRPLInfo failed");
        }

        bool found = false;
        for (int i = 0; i < numRpls; i++) {
            if (strcmp("bin\\ghs\\cafe\\cos\\pads\\padscore\\NDEBUG\\padscore.rpl", moduleInfos[i].name) == 0) {
                // save the address of the data section
                *(uint32_t*) 0xF4158000 = OSEffectiveToPhysical((uint32_t) moduleInfos[i].dataAddr);
                DCStoreRange((void *) 0xF4158000, 4);

                found = true;
                break;
            }
        }

        if (!found) {
            OSFatal("padscore not loaded");
        }

        WPADInit();
        for (uint32_t i = 0; i < 4; i++) {
            WPADDisconnect((WPADChan) i);
        }

        // run the ios exploit
        ExecuteIOSExploit();
    }

    closeBtrm(btrmHandle);  

    return 0;
}
