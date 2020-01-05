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
 *  0x13: Mode-specific details
 *  0x20: Echo (i.e. send back) all data received. Used to test connection.
 * 
 * Single byte commands
 *  0x20: Go to Main menu
 *  0x21: Go to Setup menu
 *  0x22: Go to Divide menu
 *  0x23: Go to Arc menu
 *  0x24: Go to Manual menu
 *  0x25: Go to Go2Zero menu
 *  0x26: Set Zero position counter-clockwise
 *  0x27: Set Zero position clockwise
 *  0x28: Go to Zero position
 *  0x30: Turn right encoder CCW
 *  0x31: Turn right encoder CW
 *  0x32: Press right push button
 *  0x33: Turn left encoder CCW
 *  0x34: Turn left encoder CW
 *  0x35: Press left push button
 *  0x36: Turn manual CCW
 *  0x37: Turn manual CW
 *  0x99: Stop parsing (there are no more commands in this buffer)
 *
 * Multi byte commands (unprotected)
 *  0x90: Jump steps. Parameters: int32_t NumberOfSteps
 *  0x91: Jump steps with overshoot. Parameters: int32_t NumberOfSteps
 *  0x92: Set manual speed. Parameters: uint16_t NewSpeed
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
    DATAREQUEST_GET_MODE_DETAILS = 0x13,
    DATAREQUEST_GET_ECHO = 0x20
} apiDataRequest_t;

typedef enum
{
    COMMAND_MAIN_MENU = 0x20,
    COMMAND_SETUP_MENU = 0x21,
    COMMAND_DIVIDE_MENU = 0x22,
    COMMAND_ARC_MENU = 0x23,
    COMMAND_MANUAL_MENU = 0x24,
    COMMAND_GO2ZERO_MENU = 0x25,
    COMMAND_SET_ZERO_CCW = 0x26,
    COMMAND_SET_ZERO_CW = 0x27,
    COMMAND_GO_TO_ZERO = 0x28,
    COMMAND_REBOT_BOOTLOADER_MODE = 0x29,
    COMMAND_REBOT_NORMAL_MODE = 0x2A,
    COMMAND_LEFT_ENCODER_CCW = 0x30,
    COMMAND_LEFT_ENCODER_CW = 0x31,
    COMMAND_LEFT_ENCODER_PUSH = 0x32,
    COMMAND_RIGHT_ENCODER_CCW = 0x33,
    COMMAND_RIGHT_ENCODER_CW = 0x34,
    COMMAND_RIGHT_ENCODER_PUSH = 0x35,
    COMMAND_TURN_MANUAL_CCW = 0x36,
    COMMAND_TURN_MANUAL_CW = 0x37,
    COMMAND_STOP_MOTOR_MANUAL = 0x38,
    COMMAND_STOP_PARSING = 0x99,
    //COMMAND_FORMAT_DRIVE = 0x56,     
    COMMAND_JUMP_STEPS = 0x90,
    COMMAND_JUMP_STEPS_WITH_OVERSHOOT = 0x91,
    COMMAND_SET_MANUAL_SPEED = 0x92
} apiCommand_t;


/******************************************************************************
 * Function prototypes
 ******************************************************************************/

void api_prepare(uint8_t *inBuffer, uint8_t *outBuffer);
void api_parse(uint8_t *inBuffer, uint8_t receivedDataLength, uint8_t *outBuffer);


#endif	/* API_H */

