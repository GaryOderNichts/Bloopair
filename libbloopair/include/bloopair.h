/*
 *   Copyright (C) 2021-2023 GaryOderNichts
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

#pragma once

#include "bloopair_ipc.h"
#include <coreinit/ios.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open Bloopair IPC.
 * 
 * \note
 * Note that this will also succeed if Bloopair has not been loaded yet.
 * Use \link Bloopair_IsActive \endlink to check if Bloopair is laoded.
 * 
 * \return
 * Positive \c IOSHandle to the Bloopair IPC or a negative \c IOSError value.
 */
IOSHandle Bloopair_Open(void);

/**
 * Close an active Bloopair IPC handle.
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \return
 * \c IOS_ERROR_OK on success.
 */
IOSError Bloopair_Close(IOSHandle handle);

/**
 * Check if Bloopair has been loaded and is active.
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \return
 * \c TRUE if Bloopair is active.
 */
BOOL Bloopair_IsActive(IOSHandle handle);

/**
 * Get the currently running Bloopair version.
 * See the \c BLOOPAIR_VERSION_* macros for more information.
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \return
 * The currently running Bloopair version or a negative value on error.
 */
int32_t Bloopair_GetVersion(IOSHandle handle);

/**
 * Read the local Bluetooth Device Address of the Bluetooth controller.
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \param outBDA
 * A pointer to write the 6-byte BDA to.
 * 
 * \return
 * \c IOS_ERROR_OK on success.
 */
IOSError Bloopair_ReadControllerBDA(IOSHandle handle, uint8_t* outBDA);

/**
 * Manually adds a controller pairing.
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \param bda
 * A pointer to a 6-byte Bluetooth Device Address.
 * 
 * \param link_key
 * A pointer to a 16-byte link key.
 * 
 * \param name
 * A pointer to a max. 64-byte name.
 * Note that Bloopair reserves the last 8 bytes of the name, which makes the actual usable size 56 bytes.
 * 
 * \param vid
 * The vendor ID.
 * 
 * \param pid
 * The product ID.
 * 
 * \return
 * \c IOS_ERROR_OK on success.
 */
IOSError Bloopair_AddControllerPairing(IOSHandle handle, uint8_t* bda, uint8_t* link_key, const char* name, uint16_t vid, uint16_t pid);

#ifdef __cplusplus
}
#endif
