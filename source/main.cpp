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
#include <proc_ui/procui.h>

#include <coreinit/cache.h>
#include <coreinit/memorymap.h>
#include <coreinit/dynload.h>
#include <coreinit/debug.h>
#include <sysapp/launch.h>

#include <chrono>
#include <condition_variable>

#include "ios_exploit.h"
#include "ipc.hpp"
#include "pair_menu.hpp"

std::mutex padscore_load_mtx;
std::condition_variable padscore_load_cv;
bool padscore_loaded = false;

void load_callback(OSDynLoad_Module module, void *userContext, OSDynLoad_NotifyReason notifyReason, OSDynLoad_NotifyData *infos)
{
    // make sure this is actually padscore
    if (notifyReason == OS_DYNLOAD_NOTIFY_LOADED && strcmp("bin\\ghs\\cafe\\cos\\pads\\padscore\\NDEBUG\\padscore.rpl", infos->name) == 0) {
        // save the address of the data section
        *(uint32_t*) 0xF4158000 = OSEffectiveToPhysical((uint32_t) infos->dataAddr);
        DCStoreRange((void *) 0xF4158000, 4);

        // notify that we're done
        padscore_loaded = true;
        padscore_load_cv.notify_one();
    }
}

int main(int argc, char **argv)
{
    // init procui
    ProcUIInit(&OSSavesDone_ReadyToRelease);

    IOSHandle btrmHandle = openBtrm();
    if (btrmHandle < 0) {
        OSFatal("Can't open btrm");
    }

    if (!isBloopairRunning(btrmHandle)) {
        // set a callback
        OSDynLoad_AddNotifyCallback(load_callback, nullptr);
        
        // load padscore
        OSDynLoad_Module module;
        OSDynLoad_Acquire("padscore.rpl", &module);

        if (!padscore_loaded) {
            // wait until padscore is loaded
            std::unique_lock<std::mutex> lck(padscore_load_mtx);
            if (padscore_load_cv.wait_until(lck, std::chrono::system_clock::now() + std::chrono::milliseconds(1000))
                == std::cv_status::timeout) {
                OSFatal("padscore load timed out\n");
            }
        }

        void (*WPADInit)(void) = nullptr;
        void (*WPADDisconnect)(uint32_t chan) = nullptr;
        OSDynLoad_FindExport(module, FALSE, "WPADInit", (void**) &WPADInit);
        OSDynLoad_FindExport(module, FALSE, "WPADDisconnect", (void**) &WPADDisconnect);

        WPADInit();
        for (uint32_t i = 0; i < 4; i++) {
            WPADDisconnect(i);
        }

        // run the ios exploit
        ExecuteIOSExploit();

        // release padscore
        OSDynLoad_Release(module);

        // delete callback
        OSDynLoad_DelNotifyCallback(load_callback, nullptr);
    }

    closeBtrm(btrmHandle);

    handle_pairing_menu();

    // exit to the menu as soon as possible
    ProcUIStatus status;
    while ((status = ProcUIProcessMessages(TRUE)) != PROCUI_STATUS_EXITING) {
        if(status == PROCUI_STATUS_RELEASE_FOREGROUND) {
            ProcUIDrawDoneRelease();
        }

        if(status != PROCUI_STATUS_IN_FOREGROUND) {
            continue;
        }

        SYSLaunchMenu();
    }

    // shutdown procui
    ProcUIShutdown();

    return 0;
}
