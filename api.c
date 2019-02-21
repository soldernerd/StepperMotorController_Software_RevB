
#include <string.h>

#include "system.h"
#include "api.h"
#include "display.h"
#include "os.h"
#include "encoder.h"
#include "i2c.h"
#include "fat16.h"
#include "flash.h"
#include "application_config.h"
#include "adc.h"



/******************************************************************************
 * Static function prototypes
 ******************************************************************************/

static void _fill_buffer_get_status(uint8_t *outBuffer);
static void _fill_buffer_get_display(uint8_t *outBuffer, uint8_t secondHalf);
static void _fill_buffer_get_mode_details(uint8_t *outBuffer);

static void _parse_command_short(uint8_t cmd);
static uint8_t _parse_command_long(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);

//static uint8_t _parse_format_drive(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);
static uint8_t _parse_jump_steps(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);

/******************************************************************************
 * Public functions implementation
 ******************************************************************************/


void api_prepare(uint8_t *inBuffer, uint8_t *outBuffer)
{
    apiDataRequest_t command = (apiDataRequest_t) inBuffer[0]; 

    if(command>0x7F)
    {
        switch(command)
        {
            //No extended data requests...
            
            default:
                outBuffer[0] = 0x99;
                outBuffer[1] = 0x99;
        }
    }
    else
    {
        //Normal data request, need no parameters, may be followed by commands
        switch(command)				
        {
            case DATAREQUEST_GET_COMMAND_RESPONSE:
                //nothing to do
                break;
            
            case DATAREQUEST_GET_STATUS:
                //Call function to fill the buffer with general information
                _fill_buffer_get_status(outBuffer);
                break;

            case DATAREQUEST_GET_DISPLAY_1:
                //Call function to fill the buffer with general information
                _fill_buffer_get_display(outBuffer, 0);
                break;

            case DATAREQUEST_GET_DISPLAY_2:
                //Call function to fill the buffer with general information
                _fill_buffer_get_display(outBuffer, 1);
                break;
                
            case DATAREQUEST_GET_MODE_DETAILS:
                //Call function to fill the buffer with mode-specific details
                _fill_buffer_get_mode_details(outBuffer);
                break;
                
            case DATAREQUEST_GET_ECHO:
                //Copy received data to outBuffer
                memcpy(outBuffer, inBuffer, 64);
                break;                
                
            default:
                outBuffer[0] = 0x99;
                outBuffer[1] = 0x99;
        }
    }
}

void api_parse(uint8_t *inBuffer, uint8_t receivedDataLength, uint8_t *outBuffer)
{
    //Check if the host expects us to do anything else

    uint8_t in_idx;
    uint8_t out_idx;
    uint8_t *out_idx_ptr;
    
    out_idx = 0;
    out_idx_ptr = &out_idx;
    
    if(inBuffer[0]>0x7F)
    {
        //Extended data request. May not be followed by commands.
        //Nothing for us to do here
        return;
    }
    
    if(inBuffer[0]==DATAREQUEST_GET_ECHO)
    {
        //Connectivity is being tested. Just echo back whatever we've received
        //Do not try to interpret any of the following bytes as commands
        return;
    }
    
    if(inBuffer[0]==DATAREQUEST_GET_COMMAND_RESPONSE)
    {
        //Echo back to the host PC the command we are fulfilling in the first uint8_t
        outBuffer[0] = DATAREQUEST_GET_COMMAND_RESPONSE;
    
        //Firmware signature
        outBuffer[1] = LOW_BYTE(FIRMWARE_SIGNATURE);
        outBuffer[2] = HIGH_BYTE(FIRMWARE_SIGNATURE);
        
        //Set out_idx, also indicating that we want to provide feedback on the command's execution
        out_idx = 3;
    }
    
    in_idx = 1;
    while(in_idx<receivedDataLength)
    {
        //Check if there is anything more to parse
        if(inBuffer[in_idx]==COMMAND_STOP_PARSING)
        {
            return;
        }
        
        switch(inBuffer[in_idx] & 0xF0)
        {
            case 0x20:
                _parse_command_short(inBuffer[in_idx]);
                ++in_idx;
                break;
                
            case 0x30:
                _parse_command_short(inBuffer[in_idx]);
                ++in_idx;
                break;
                
            case 0x90:
                in_idx += _parse_command_long(&inBuffer[in_idx], outBuffer, out_idx_ptr);
                break;
                
            default:
                //We should never end up here
                //If we still do, stop parsing this buffer
                return;
        }
    }
}


/******************************************************************************
 * Static functions implementation
 ******************************************************************************/

//Fill buffer with general status information
static void _fill_buffer_get_status(uint8_t *outBuffer)
{
    //Echo back to the host PC the command we are fulfilling in the first uint8_t
    outBuffer[0] = DATAREQUEST_GET_STATUS;
    
    //Firmware signature
    outBuffer[1] = LOW_BYTE(FIRMWARE_SIGNATURE);
    outBuffer[2] = HIGH_BYTE(FIRMWARE_SIGNATURE);
    
    //Firmware version
    outBuffer[3] = FIRMWARE_VERSION_MAJOR;
    outBuffer[4] = FIRMWARE_VERSION_MINOR;
    outBuffer[5] = FIRMWARE_VERSION_FIX;
    
    //General status information
    outBuffer[6] = os.subTimeSlot;
    outBuffer[7] = os.timeSlot;
    outBuffer[8] = os.done;
    outBuffer[9] = os.encoder1Count;
    outBuffer[10] = os.button1;
    outBuffer[11] = os.encoder2Count;
    outBuffer[12] = os.button2;
    memcpy(&outBuffer[13], &os.current_position_in_steps, 4);
//    outBuffer[13] = LOW_WORD(LOW_BYTE(os.current_position_in_steps));
//    outBuffer[14] = LOW_WORD(HIGH_BYTE(os.current_position_in_steps));
//    outBuffer[15] = HIGH_WORD(LOW_BYTE(os.current_position_in_steps));
//    outBuffer[16] = HIGH_WORD(HIGH_BYTE(os.current_position_in_steps)); 
    memcpy(&outBuffer[17], &os.current_position_in_degrees, 4);
//    outBuffer[17] = LOW_WORD(LOW_BYTE(os.current_position_in_degrees));
//    outBuffer[18] = LOW_WORD(HIGH_BYTE(os.current_position_in_degrees));
//    outBuffer[19] = HIGH_WORD(LOW_BYTE(os.current_position_in_degrees));
//    outBuffer[20] = HIGH_WORD(HIGH_BYTE(os.current_position_in_degrees));
    outBuffer[21] = os.displayState;
    outBuffer[22] = os.beep_count;
    outBuffer[23] = LOW_BYTE(os.temperature[TEMPERATURE_SOURCE_INTERNAL]);
    outBuffer[24] = HIGH_BYTE(os.temperature[TEMPERATURE_SOURCE_INTERNAL]);
    outBuffer[25] = LOW_BYTE(os.temperature[TEMPERATURE_SOURCE_EXTERNAL]);
    outBuffer[26] = HIGH_BYTE(os.temperature[TEMPERATURE_SOURCE_EXTERNAL]);
    outBuffer[27] = os.fan_on;
    outBuffer[28] = os.brake_on;
    outBuffer[29] = os.busy;
    
    //Full copy of config:
    //42-45:    uint32_t full_circle_in_steps
    //46:       uint8_t inverse_direction
    //47-48:    uint16_t overshoot_in_steps
    //49-50:    uint16_t overshoot_cost_in_steps
    //51-52:    uint16_t minimum_speed
    //53-54:    uint16_t maximum_speed
    //55-56:    uint16_t initial_speed_arc
    //57-58:    uint16_t maximum_speed_arc
    //59-60:    uint16_t initial_speed_manual
    //61-62:    uint16_t maximum_speed_manual
    //63:       uint8_t beep_duration 
    memcpy(&outBuffer[42], &config, 22);
}

//Fill buffer with display content
static void _fill_buffer_get_display(uint8_t *outBuffer, uint8_t secondHalf)
{
    uint8_t cntr;
    uint8_t line;
    uint8_t start_line;
    uint8_t position;
    
    //Echo back to the host PC the command we are fulfilling in the first uint8_t
    if(secondHalf)
    {
        outBuffer[0] = DATAREQUEST_GET_DISPLAY_2;
    }
    else
    {
        outBuffer[0] = DATAREQUEST_GET_DISPLAY_1;
    }
   
    //Firmware signature
    outBuffer[1] = LOW_BYTE(FIRMWARE_SIGNATURE);
    outBuffer[2] = HIGH_BYTE(FIRMWARE_SIGNATURE);
   
    //Get display data
    cntr = 3;
    if(secondHalf)
    {
        start_line = 2;
    }
    else
    {
        start_line = 0;
    }
    for(line=start_line; line<start_line+2; ++line)
    {
        for(position=0; position<20; ++position)
        {
            outBuffer[cntr] = (uint8_t) display_get_character(line, position);
            ++cntr;
        }
    }
}

static void _fill_buffer_get_mode_details(uint8_t *outBuffer)
{
    //Echo back to the host PC the command we are fulfilling in the first uint8_t
    outBuffer[0] = DATAREQUEST_GET_MODE_DETAILS;
    
    //Firmware signature
    outBuffer[1] = LOW_BYTE(FIRMWARE_SIGNATURE);
    outBuffer[2] = HIGH_BYTE(FIRMWARE_SIGNATURE);
    
    //What mode are we in?
    outBuffer[3] = os.displayState;
    
    switch(os.displayState & 0x0F)
    {
        case DISPLAY_STATE_MAIN:
            break;
            
        case DISPLAY_STATE_SETUP1:
        case DISPLAY_STATE_SETUP2:
            outBuffer[4] = os.setup_step_size;  
            break;
            
        case DISPLAY_STATE_DIVIDE1:
        case DISPLAY_STATE_DIVIDE2:
            
            outBuffer[4] = os.divide_step_size;
            outBuffer[5] = HIGH_BYTE(os.division);
            outBuffer[6] = LOW_BYTE(os.division);
            outBuffer[7] = HIGH_BYTE(os.divide_jump_size);
            outBuffer[8] = LOW_BYTE(os.divide_jump_size);
            outBuffer[9] = HIGH_BYTE(os.divide_position);
            outBuffer[10] = LOW_BYTE(os.divide_position);
            break;

        case DISPLAY_STATE_ARC1:
        case DISPLAY_STATE_ARC2:
            outBuffer[4] = HIGH_BYTE(os.arc_step_size);
            outBuffer[5] = LOW_BYTE(os.arc_step_size);
            outBuffer[6] = os.arc_direction;
            outBuffer[7] = HIGH_BYTE(os.arc_speed);
            outBuffer[8] = LOW_BYTE(os.arc_speed);
            break;
            
        case DISPLAY_STATE_ZERO:
            break;
            
        case DISPLAY_STATE_MANUAL:
            outBuffer[4] = os.manual_direction;
            outBuffer[5] = HIGH_BYTE(os.manual_speed);
            outBuffer[6] = LOW_BYTE(os.manual_speed);
            break;
    }
}

static void _parse_command_short(uint8_t cmd)
{
    switch(cmd)
    {
//        case COMMAND_REBOT:
//            i2c_eeprom_writeByte(EEPROM_BOOTLOADER_BYTE_ADDRESS, 0x00); //0x00 is a neutral value
//            reboot();
//            break;
//            
//        case COMMAND_REBOT_BOOTLOADER_MODE:
//            i2c_eeprom_writeByte(EEPROM_BOOTLOADER_BYTE_ADDRESS, BOOTLOADER_BYTE_FORCE_BOOTLOADER_MODE);
//            reboot();
//            break;
//                
//        case COMMAND_REBOT_NORMAL_MODE:
//            i2c_eeprom_writeByte(EEPROM_BOOTLOADER_BYTE_ADDRESS, BOOTLOADER_BYTE_FORCE_NORMAL_MODE);
//            reboot();
//            break;
        
        case COMMAND_MAIN_MENU:
            os.displayState = DISPLAY_STATE_MAIN_SETUP;
            break;
            
        case COMMAND_SETUP_MENU:
            os.displayState = DISPLAY_STATE_SETUP1_CONFIRM;
            break;
            
        case COMMAND_DIVIDE_MENU:
            os.displayState = DISPLAY_STATE_DIVIDE1_CONFIRM;
            break;
            
        case COMMAND_ARC_MENU:
            os.displayState = DISPLAY_STATE_ARC1_CONFIRM;
            break;
            
        case COMMAND_MANUAL_MENU:
            os.displayState = DISPLAY_STATE_MANUAL_CANCEL;
            break;
            
        case COMMAND_GO2ZERO_MENU:
            os.displayState = DISPLAY_STATE_ZERO_NORMAL;
            break;
            
        case COMMAND_SET_ZERO_CCW:
            os.current_position_in_steps = 0;
            os.divide_position = 0;
            motor_schedule_command(MOTOR_DIRECTION_CW, config.overshoot_in_steps, 0);
            motor_schedule_command(MOTOR_DIRECTION_CCW, config.overshoot_in_steps, 0);
            os.approach_direction = MOTOR_DIRECTION_CCW;
            break;
            
        case COMMAND_SET_ZERO_CW:
            os.current_position_in_steps = 0;
            os.divide_position = 0;
            motor_schedule_command(MOTOR_DIRECTION_CCW, config.overshoot_in_steps, 0);
            motor_schedule_command(MOTOR_DIRECTION_CW, config.overshoot_in_steps, 0);
            os.approach_direction = MOTOR_DIRECTION_CW;
            break;
            
        case COMMAND_GO_TO_ZERO:
            motor_go_to_steps_position(0);
                
        case COMMAND_LEFT_ENCODER_CCW:
            --os.encoder2Count;
            break;
            
        case COMMAND_LEFT_ENCODER_CW:
            ++os.encoder2Count;
            break;
            
        case COMMAND_LEFT_ENCODER_PUSH:
            ++os.button2;
            break;
            
        case COMMAND_RIGHT_ENCODER_CCW:
            --os.encoder1Count;
            break;
            
        case COMMAND_RIGHT_ENCODER_CW:
            ++os.encoder1Count;
            break;
            
        case COMMAND_RIGHT_ENCODER_PUSH:
            ++os.button1;
            break;
    }
}

static uint8_t _parse_command_long(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    uint8_t length = 65;
    
    switch(data[0])
    {          
//        case COMMAND_FORMAT_DRIVE:
//            length = _parse_format_drive(data, out_buffer, out_idx_ptr);
//            break;
        
        case COMMAND_JUMP_STEPS:
            length = _parse_jump_steps(data, out_buffer, out_idx_ptr);
            break;
    }    
    
    
    
    return length;
}

//static uint8_t _parse_format_drive(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
//{
//    //0x56: Format drive. Parameters: none, 0xDA22
//    
//    uint8_t return_value;
//    
//    if((data[0]!=COMMAND_FORMAT_DRIVE) || (data[1]!=0xDA) || (data[2]!=0x22))
//    {
//        return 3;
//    }
//    
//    return_value = fat_format();
//    
//    //Return confirmation if desired
//    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<63))
//    {
//        out_buffer[(*out_idx_ptr)++] = COMMAND_FORMAT_DRIVE;
//        out_buffer[(*out_idx_ptr)++] = return_value;
//    }
//    
//    return 3;
//} 

static uint8_t _parse_jump_steps(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    //0x90: Jump steps. Parameters: int32_t NumberOfSteps
    
    uint8_t return_value;
    int32_t number_of_steps;
    
    number_of_steps = data[1];
    number_of_steps <<= 8;
    number_of_steps |= data[2];
    number_of_steps <<= 8;
    number_of_steps |= data[3];
    number_of_steps <<= 8;
    number_of_steps |= data[4];
    
    if(number_of_steps>0)
    {
        return_value = motor_schedule_command(MOTOR_DIRECTION_CW, (uint32_t) number_of_steps, 0);
    }
    else
    {
        number_of_steps = -number_of_steps;
        return_value = motor_schedule_command(MOTOR_DIRECTION_CCW, (uint32_t) number_of_steps, 0);
    }
    
    //Return confirmation if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<63))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_JUMP_STEPS;
        out_buffer[(*out_idx_ptr)++] = return_value;
    }
    
    return 5;
} 
