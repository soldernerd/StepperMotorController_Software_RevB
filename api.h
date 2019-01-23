/* 
 * File:   api.h
 * Author: luke
 *
 * Created on 25. Juli 2018, 11:07
 */

#ifndef API_H
#define	API_H

/******************************************************************************
 * API description
 ******************************************************************************
 * 
 * Data requests. These must be sent as the first byte. Any commands may follow
 *  0x10: General status information
 *  0x11: First 2 lines of display content
 *  0x12: Last 2 lines of display content
 *  0x13: Bootloader details
 *  0x20: Echo (i.e. send back) all data received. Used to test connection.
 * 
 * Single byte commands
 *  0x20: Reboot
 *  0x21: Reboot in bootloader mode
 *  0x22: Reboot in normal mode
 *  0x3C: Turn encoder CCW
 *  0x3D: Turn encoder CW
 *  0x3E: Press push button
 *  0x99: Stop parsing (there are no more commands in this buffer)
 * 
 * Multi byte commands (followed by a 16 bit constant to prevent unintended use)
 *  0x56: Format drive. Parameters: none, 0xDA22
 *  
 ******************************************************************************/

/******************************************************************************
 * Type definitions
 ******************************************************************************/

typedef enum
{
    DATAREQUEST_GET_COMMAND_RESPONSE = 0x00,
    DATAREQUEST_GET_STATUS = 0x10,
    DATAREQUEST_GET_DISPLAY_1 = 0x11,
    DATAREQUEST_GET_DISPLAY_2 = 0x12,
    DATAREQUEST_GET_BOOTLOADER_DETAILS = 0x13,
    DATAREQUEST_GET_ECHO = 0x20,
} apiDataRequest_t;

typedef enum
{
    COMMAND_REBOT = 0x20,
    COMMAND_REBOT_BOOTLOADER_MODE = 0x21,
    COMMAND_REBOT_NORMAL_MODE = 0x22,
    COMMAND_ENCODER_CCW = 0x3C,
    COMMAND_ENCODER_CW = 0x3D,
    COMMAND_ENCODER_PUSH = 0x3E,
    COMMAND_STOP_PARSING = 0x99,
    COMMAND_FORMAT_DRIVE = 0x56,     
} apiCommand_t;


/******************************************************************************
 * Function prototypes
 ******************************************************************************/

void api_prepare(uint8_t *inBuffer, uint8_t *outBuffer);
void api_parse(uint8_t *inBuffer, uint8_t receivedDataLength, uint8_t *outBuffer);


#endif	/* API_H */

