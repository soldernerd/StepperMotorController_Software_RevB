/* 
 * File:   application_config.h
 * Author: luke
 *
 * Created on 3. Oktober 2018, 17:16
 */

#ifndef APPLICATION_CONFIG_H
#define	APPLICATION_CONFIG_H

/*
 * A random generated signature to distinguish between bootloader and normal firmware
 */

#define FIRMWARE_SIGNATURE 0x26BC

/*
 * Firmware version
 */

#define FIRMWARE_VERSION_MAJOR 0x00
#define FIRMWARE_VERSION_MINOR 0x05
#define FIRMWARE_VERSION_FIX 0x06

/*
 * Application specific settings
 */

#define NUMBER_OF_TIMESLOTS 16
#define STARTUP_DELAY 150
//#define FORMAT_DRIVE_AT_STARTUP_IF_NECESSARY

//Page 1 (0x100 to 0x1FF) reserved for bootloader
#define EEPROM_BOOTLOADER_BYTE_ADDRESS 0x100
#define BOOTLOADER_BYTE_FORCE_BOOTLOADER_MODE 0x94
#define BOOTLOADER_BYTE_FORCE_NORMAL_MODE 0x78

//Page 2 (0x200 to 0x3FF)
#define EEPROM_CURRENT_POSITION 0x200

#endif	/* APPLICATION_CONFIG_H */

