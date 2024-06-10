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

#include <bloopair/bloopair.h>

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

#include "config.hpp"
#include "ios_exploit.h"
#include "kernel.hpp"

int main(int argc, char **argv)
{
    IOSHandle bloopairHandle = Bloopair_Open();
    if (bloopairHandle < 0) {
        OSFatal("Bloopair Loader: Failed to open btrm");
    }

    if (!Bloopair_IsActive(bloopairHandle)) {
        KernelSetup();

        // nop out security level checks
        KernelWriteU32(((uint32_t) &OSDynLoad_GetRPLInfo) + (22 * 4), 0x60000000);
        KernelWriteU32(((uint32_t) &OSDynLoad_GetNumberOfRPLs) + (6 * 4), 0x60000000);

        int numRpls = OSDynLoad_GetNumberOfRPLs();
        if (numRpls <= 0) {
            OSFatal("Bloopair Loader: OSDynLoad_GetNumberOfRPLs failed");
        }

        OSDynLoad_NotifyData moduleInfos[numRpls];
        if (!OSDynLoad_GetRPLInfo(0, numRpls, moduleInfos)) {
            OSFatal("Bloopair Loader: OSDynLoad_GetRPLInfo failed");
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
            OSFatal("Bloopair Loader: Padscore not loaded");
        }

        WPADInit();
        for (uint32_t i = 0; i < 4; i++) {
            WPADDisconnect((WPADChan) i);
        }

        // run the ios exploit
        ExecuteIOSExploit();

        int32_t version = Bloopair_GetVersion(bloopairHandle);
        if (version >= 0) {
            char commitHash[41];
            if (Bloopair_GetCommitHash(bloopairHandle, commitHash, sizeof(commitHash)) >= 0) {
                commitHash[7] = '\0';
            } else {
                commitHash[0] = '\0';
            }

            OSReport("Bloopair Loader: Loaded Bloopair version %d.%d.%d%s%s\n",
                BLOOPAIR_VERSION_MAJOR(version),
                BLOOPAIR_VERSION_MINOR(version),
                BLOOPAIR_VERSION_PATCH(version),
                commitHash[0] ? "-" : "", commitHash);
        } else {
            OSReport("Bloopair Loader: Failed to load Bloopair\n");
        }

        LoadAndApplyBloopairConfiguration(bloopairHandle);
    }

    Bloopair_Close(bloopairHandle);

    return 0;
}
