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

#include "ipc.h"
#include "controllers/common.h"
#include <coreinit/ios.h>
#include <padscore/wpad.h>

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
 * Get the currently running Bloopair commit hash as a hex string for debug versions.
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \param commitHashStr
 * A pointer to write the commit hash string to.
 * 
 * \param commitHashStrLen
 * The maximum size of the buffer.
 * Should be at least 41 for the full SHA1 hash string.
 * 
 * \return
 * 0 on success or a negative error value. \¢ IOS_ERROR_INVALID if not a debug build.
 */
IOSError Bloopair_GetCommitHash(IOSHandle handle, char* commitHashStr, uint32_t commitHashStrLen);

/**
 * Read the local Bluetooth Device Address of the console's Bluetooth controller.
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
IOSError Bloopair_ReadConsoleBDA(IOSHandle handle, uint8_t* outBDA);

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
IOSError Bloopair_AddControllerPairing(IOSHandle handle, const uint8_t* bda, const uint8_t* link_key, const char* name, uint16_t vid, uint16_t pid);

/**
 * Get Controller Information for the specified channel;
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \param chan
 * A WPAD / KPAD channel to get the information for.
 * 
 * \param outData
 * A pointer to store the controller information to.
 * 
 * \return
 * \c IOS_ERROR_OK on success.
 */
IOSError Bloopair_GetControllerInformation(IOSHandle handle, WPADChan chan, BloopairControllerInformationData* outData);

/**
 * Read a raw report buffer from the specified channel;
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \param chan
 * A WPAD / KPAD channel to read the report from.
 * 
 * \param outReport
 * A pointer to store the report to.
 * 
 * \return
 * \c IOS_ERROR_OK on success.
 */
IOSError Bloopair_ReadRawReport(IOSHandle handle, WPADChan chan, BloopairReportBuffer* outReport);

/**
 * Apply a configuration for the specified BDA.
 * 
 * \warning
 * If the controller is currently connected it needs to be disconnected first.
 * Keeping the controller connected results in undefined behaviour.
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \param bda
 * A pointer to a 6-byte bluetooth device address to apply the configuration for.
 * 
 * \param commonConfiguration
 * A pointer to read the configuration from or \c NULL to remove the configuration.
 * 
 * \return
 * \c IOS_ERROR_OK on success.
 */
IOSError Bloopair_ApplyControllerConfigurationForBDA(IOSHandle handle, const uint8_t* bda, const BloopairCommonConfiguration* commonConfiguration);

/**
 * Apply a configuration for all controllers of the specified controller type.
 * 
 * \warning
 * If any controllers of this type are currently connected they needs to be disconnected first.
 * Keeping the controllers connected results in undefined behaviour.
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \param controllerType
 * The controller type to apply the configuration for.
 * 
 * \param commonConfiguration
 * A pointer to read the configuration from or \c NULL to remove the configuration.
 * 
 * \return
 * \c IOS_ERROR_OK on success.
 */
IOSError Bloopair_ApplyControllerConfigurationForControllerType(IOSHandle handle, BloopairControllerType controllerType, const BloopairCommonConfiguration* commonConfiguration);

/**
 * Apply a controller mapping for the specified BDA.
 * 
 * \warning
 * If the controller is currently connected it needs to be disconnected first.
 * Keeping the controller connected results in undefined behaviour.
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \param bda
 * A pointer to a 6-byte bluetooth device address to apply the mapping for.
 * 
 * \param mappings
 * A pointer to read the mappings from or \c NULL to remove the mappings.
 * 
 * \param numMappings
 * The amount of mappings which should be applied or \c 0 to remove the mappings.
 * 
 * \return
 * \c IOS_ERROR_OK on success.
 */
IOSError Bloopair_ApplyControllerMappingForBDA(IOSHandle handle, const uint8_t* bda, const BloopairMappingEntry* mappings, uint8_t numMappings);

/**
 * Apply a controller mapping for all controllers of the specified controller type.
 * 
 * \warning
 * If any controllers of this type are currently connected they needs to be disconnected first.
 * Keeping the controllers connected results in undefined behaviour.
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \param controllerType
 * The controller type to apply the mappings for.
 * 
 * \param mappings
 * A pointer to read the mappings from or \c NULL to remove the mappings.
 * 
 * \param numMappings
 * The amount of mappings which should be applied or \c 0 to remove the mappings.
 * 
 * \return
 * \c IOS_ERROR_OK on success.
 */
IOSError Bloopair_ApplyControllerMappingForControllerType(IOSHandle handle, BloopairControllerType controllerType, const BloopairMappingEntry* mappings, uint8_t numMappings);

/**
 * Apply a custom configuration for the specified BDA.
 * 
 * \warning
 * If the controller is currently connected it needs to be disconnected first.
 * Keeping the controller connected results in undefined behaviour.
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \param bda
 * A pointer to a 6-byte bluetooth device address to apply the configuration for.
 * 
 * \param customConfiguration
 * A pointer to read the configuration from or \c NULL to remove the configuration.
 * 
 * \param size
 * The size of the configuration or \c 0 to remove the configuration;
 * 
 * \return
 * \c IOS_ERROR_OK on success.
 */
IOSError Bloopair_ApplyCustomConfigurationForBDA(IOSHandle handle, const uint8_t* bda, const void* customConfiguration, uint32_t size);

/**
 * Apply a custom configuration for all controllers of the specified controller type.
 * 
 * \warning
 * If any controllers of this type are currently connected they needs to be disconnected first.
 * Keeping the controllers connected results in undefined behaviour.
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \param controllerType
 * The controller type to apply the configuration for.
 * 
 * \param customConfiguration
 * A pointer to read the configuration from or \c NULL to remove the configuration.
 * 
 * \param size
 * The size of the configuration or \c 0 to remove the configuration;
 * 
 * \return
 * \c IOS_ERROR_OK on success.
 */
IOSError Bloopair_ApplyCustomConfigurationForControllerType(IOSHandle handle, BloopairControllerType controllerType, const void* customConfiguration, uint32_t size);

/**
 * Get the controller configuration for the specified channel.
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \param chan
 * The channel to get the configuration for.
 * 
 * \param outConfiguration
 * A pointer to store the configuration to.
 * 
 * \return
 * \c IOS_ERROR_OK on success.
 */
IOSError Bloopair_GetControllerConfiguration(IOSHandle handle, WPADChan chan, BloopairCommonConfiguration* outConfiguration);

/**
 * Get the default controller mapping for the specified controller type.
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \param controllerType
 * The controller type to get the configuration for.
 * 
 * \param outConfiguration
 * A pointer to store the configuration to.
 * 
 * \return
 * \c IOS_ERROR_OK on success.
 */
IOSError Bloopair_GetDefaultControllerConfiguration(IOSHandle handle, BloopairControllerType controllerType, BloopairCommonConfiguration* outConfiguration);

/**
 * Get the controller mapping for the specified channel.
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \param chan
 * The channel to get the mapping for.
 * 
 * \param outMappings
 * A pointer to store the mappings to or \c NULL.
 * 
 * \param outNumMappings
 * A pointer to read the amount of mappings which can be stored from and to write the amount of mappings successfully stored to.
 * 
 * \return
 * \c IOS_ERROR_OK on success.
 */
IOSError Bloopair_GetControllerMapping(IOSHandle handle, WPADChan chan, BloopairMappingEntry* outMappings, uint8_t* outNumMappings);

/**
 * Get the default controller mapping for the specified controller type.
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \param controllerType
 * The controller type to get the configuration for.
 * 
 * \param outMappings
 * A pointer to store the mappings to or \c NULL.
 * 
 * \param outNumMappings
 * A pointer to read the amount of mappings which can be stored from and to write the amount of mappings successfully stored to.
 * 
 * \return
 * \c IOS_ERROR_OK on success.
 */
IOSError Bloopair_GetDefaultControllerMapping(IOSHandle handle, BloopairControllerType controllerType, BloopairMappingEntry* outMappings, uint8_t* outNumMappings);

/**
 * Get the controller custom configuration for the specified channel.
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \param chan
 * The channel to get the configuration for.
 * 
 * \param outCustom
 * A pointer to store the configuration to or \c NULL to only store the size.
 * 
 * \param outSize
 * A pointer to read the maximum size of the configuration from or store the size of the configuration to.
 * 
 * \return
 * \c IOS_ERROR_OK on success.
 */
IOSError Bloopair_GetCustomConfiguration(IOSHandle handle, WPADChan chan, void* outCustom, uint32_t* outSize);

/**
 * Get the default controller custom mapping for the specified controller type.
 * 
 * \param handle
 * A handle obtained by \link Bloopair_Open \endlink.
 * 
 * \param controllerType
 * The controller type to get the configuration for.
 * 
 * \param outCustom
 * A pointer to store the configuration to or \c NULL to only store the size.
 *
 * \param outSize
 * A pointer to read the maximum size of the configuration from or store the size of the configuration to.
 * 
 * \return
 * \c IOS_ERROR_OK on success.
 */
IOSError Bloopair_GetDefaultCustomConfiguration(IOSHandle handle, BloopairControllerType controllerType, void* outCustom, uint32_t* outSize);

#ifdef __cplusplus
}
#endif
