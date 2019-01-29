
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



/******************************************************************************
 * Static function prototypes
 ******************************************************************************/

static void _fill_buffer_get_status(uint8_t *outBuffer);
static void _fill_buffer_get_display(uint8_t *outBuffer, uint8_t secondHalf);
static void _fill_buffer_get_bootloader_details(uint8_t *outBuffer);

static void _parse_command_short(uint8_t cmd);
static uint8_t _parse_command_long(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);

static uint8_t _parse_format_drive(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);

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
                
            case DATAREQUEST_GET_BOOTLOADER_DETAILS:
                //Call function to fill the buffer with bootloader details
                _fill_buffer_get_bootloader_details(outBuffer);
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
        outBuffer[1] = FIRMWARE_SIGNATURE >> 8; //MSB
        outBuffer[2] = (uint8_t) FIRMWARE_SIGNATURE; //LSB
        
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
                
            case 0x50:
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
    
    //Bootloader signature
    outBuffer[1] = FIRMWARE_SIGNATURE >> 8; //MSB
    outBuffer[2] = (uint8_t) FIRMWARE_SIGNATURE; //LSB
    
    //Flash busy or not
    outBuffer[3] = (uint8_t) flash_is_busy();
    
    //Firmware version
    outBuffer[4] = FIRMWARE_VERSION_MAJOR;
    outBuffer[5] = FIRMWARE_VERSION_MINOR;
    outBuffer[6] = FIRMWARE_VERSION_FIX;
    
    //Display status, display off, startup etc
//    outBuffer[7] = ui_get_status();
    
    //Entire os struct
//    outBuffer[8] = os.encoderCount;
//    outBuffer[9] = os.buttonCount;
//    outBuffer[10] = os.timeSlot;
//    outBuffer[11] = os.done;
//    outBuffer[12] = os.bootloader_mode;
//    outBuffer[13] = os.display_mode;
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
   
    //Bootloader signature
    outBuffer[1] = FIRMWARE_SIGNATURE >> 8; //MSB
    outBuffer[2] = (uint8_t) FIRMWARE_SIGNATURE; //LSB
   
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

static void _fill_buffer_get_bootloader_details(uint8_t *outBuffer)
{
    uint8_t cntr;
    uint8_t data_length;
    uint16_t buffer_small;
    uint32_t buffer_large;
    
    //Echo back to the host PC the command we are fulfilling in the first uint8_t
    outBuffer[0] = DATAREQUEST_GET_BOOTLOADER_DETAILS;
    
    //Bootloader signature
    outBuffer[1] = HIGH_BYTE(FIRMWARE_SIGNATURE); //MSB
    outBuffer[2] = LOW_BYTE(FIRMWARE_SIGNATURE); //LSB
   
//    //Bootloader information (high level)
//    buffer_large = bootloader_get_file_size();
//    outBuffer[3] = HIGH_BYTE(HIGH_WORD(buffer_large));
//    outBuffer[4] = LOW_BYTE(HIGH_WORD(buffer_large));
//    outBuffer[5] = HIGH_BYTE(LOW_WORD(buffer_large));
//    outBuffer[6] = LOW_BYTE(LOW_WORD(buffer_large));
//    
//    buffer_small = bootloader_get_entries();
//    outBuffer[7] = HIGH_BYTE(buffer_small);
//    outBuffer[8] = LOW_BYTE(buffer_small);
//    
//    buffer_small = bootloader_get_total_entries();
//    outBuffer[9] = HIGH_BYTE(buffer_small);
//    outBuffer[10] = LOW_BYTE(buffer_small);
//    
//    outBuffer[11] = (uint8_t) bootloader_get_error();
//    
//    buffer_small = bootloader_get_flashPagesWritten();
//    outBuffer[12] = HIGH_BYTE(buffer_small);
//    outBuffer[13] = LOW_BYTE(buffer_small);
//    
//    //Bootloader information (last record)
//    buffer_small = bootloader_get_rec_dataLength();
//    outBuffer[14] = HIGH_BYTE(buffer_small);
//    outBuffer[15] = LOW_BYTE(buffer_small);
//    
//    buffer_small = bootloader_get_rec_address();
//    outBuffer[16] = HIGH_BYTE(buffer_small);
//    outBuffer[17] = LOW_BYTE(buffer_small);
//    
//    outBuffer[18] = (uint8_t) bootloader_get_rec_recordType();
//    outBuffer[19] = bootloader_get_rec_checksum();
//    outBuffer[20] = bootloader_get_rec_checksumCheck();
//
//    data_length = (uint8_t) bootloader_get_rec_dataLength();
//    if(data_length>43)
//    {
//        //More will not fit into our 64byte buffer
//        data_length = 43;
//    }
//    for(cntr=0; cntr<data_length; ++cntr)
//    {
//        outBuffer[21+cntr] = bootloader_get_rec_data(cntr);
//    }
}

static void _parse_command_short(uint8_t cmd)
{
    switch(cmd)
    {
        case COMMAND_REBOT:
            i2c_eeprom_writeByte(EEPROM_BOOTLOADER_BYTE_ADDRESS, 0x00); //0x00 is a neutral value
//            system_delay_ms(10); //ensure data has been written before rebooting
            reboot();
            break;
            
        case COMMAND_REBOT_BOOTLOADER_MODE:
            i2c_eeprom_writeByte(EEPROM_BOOTLOADER_BYTE_ADDRESS, BOOTLOADER_BYTE_FORCE_BOOTLOADER_MODE);
//            system_delay_ms(10); //ensure data has been written before rebooting
            reboot();
            break;
                
        case COMMAND_REBOT_NORMAL_MODE:
            i2c_eeprom_writeByte(EEPROM_BOOTLOADER_BYTE_ADDRESS, BOOTLOADER_BYTE_FORCE_NORMAL_MODE);
//            system_delay_ms(10); //ensure data has been written before rebooting
            reboot();
            break;
                
//        case COMMAND_ENCODER_CCW:
//            --os.encoderCount;
//            break;
//            
//        case COMMAND_ENCODER_CW:
//            ++os.encoderCount;
//            break;
//            
//        case COMMAND_ENCODER_PUSH:
//            ++os.buttonCount;
//            break;
    }
}

static uint8_t _parse_command_long(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    uint8_t length = 65;
    
    switch(data[0])
    {          
        case COMMAND_FORMAT_DRIVE:
            length = _parse_format_drive(data, out_buffer, out_idx_ptr);
            break;
    }    
    
    return length;
}

static uint8_t _parse_format_drive(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    //0x56: Format drive. Parameters: none, 0xDA22
    
    uint8_t return_value;
    
    if((data[0]!=COMMAND_FORMAT_DRIVE) || (data[1]!=0xDA) || (data[2]!=0x22))
    {
        return 3;
    }
    
    return_value = fat_format();
    
    //Return confirmation if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<63))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_FORMAT_DRIVE;
        out_buffer[(*out_idx_ptr)++] = return_value;
    }
    
    return 3;
} 
