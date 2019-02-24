#include <stdint.h>
#include <stdlib.h>
#include <xc.h>
#include "string.h"
#include "os.h"
#include "fat16.h"

char configFile_name[8] = {'C', 'O', 'N', 'F', 'I', 'G', ' ', ' '};
char configFile_extention[3] = {'T', 'X', 'T'};
uint8_t ConfigFile_buffer[512];

uint8_t _add_item(char *item_string, int32_t value, uint8_t *buffer);

uint8_t _add_item(char *item_string, int32_t value, uint8_t *buffer)
{
    uint8_t length = 0;
    
    //Copy item string
    while(item_string[length])
    {
        buffer[length] = item_string[length];
        ++length;
    }
    
    //Add equal sign
    buffer[length++] = '=';

    //Add value converted to decimal string and find new end of string
    ltoa(&buffer[length], value, 10);
    while(buffer[length])
    {
        ++length;
    }
    
    //Add line break
    buffer[length++] = '\n';
    
    //Return length of string written
    return length;
}

const char full_circle_in_steps_string[] = "FULL_CIRCLE_IN_STEPS";
const char inverse_direction_string[] = "INVERSE_DIRECTION";
const char overshoot_in_steps_string[] = "OVERSHOOT_IN_STEPS";
const char overshoot_cost_in_steps_string[] = "OVERSHOOT_COST_IN_STEPS";
const char minimum_speed_string[] = "MINIMUM_SPEED";
const char maximum_speed_string[] = "MAXIMUM_SPEED";
const char initial_speed_arc_string[] = "INITIAL_SPEED_ARC";
const char maximum_speed_arc_string[] = "MAXIMUM_SPEED_ARC";
const char initial_speed_manual_string[] = "INITIAL_SPEED_MANUAL";
const char maximum_speed_manual_string[] = "MAXIMUM_SPEED_MANUAL";
const char beep_duration_string[] = "BEEP_DURATION";


void configFile_write(void)
{
    uint8_t file_number;
    uint32_t file_size;
    //uint8_t *buffer_ptr;
    
    file_size = 0;
    file_size += _add_item(&full_circle_in_steps_string, (int32_t) config.full_circle_in_steps, &ConfigFile_buffer[file_size]);
    file_size += _add_item(&inverse_direction_string, (int32_t) config.inverse_direction, &ConfigFile_buffer[file_size]);
    file_size += _add_item(&overshoot_in_steps_string, (int32_t) config.overshoot_in_steps, &ConfigFile_buffer[file_size]);
    file_size += _add_item(&overshoot_cost_in_steps_string, (int32_t) config.overshoot_cost_in_steps, &ConfigFile_buffer[file_size]);
    file_size += _add_item(&minimum_speed_string, (int32_t) config.minimum_speed, &ConfigFile_buffer[file_size]);
    file_size += _add_item(&maximum_speed_string, (int32_t) config.maximum_speed, &ConfigFile_buffer[file_size]);
    file_size += _add_item(&initial_speed_arc_string, (int32_t) config.initial_speed_arc, &ConfigFile_buffer[file_size]);
    file_size += _add_item(&maximum_speed_arc_string, (int32_t) config.maximum_speed_arc, &ConfigFile_buffer[file_size]);
    file_size += _add_item(&initial_speed_manual_string, (int32_t) config.initial_speed_manual, &ConfigFile_buffer[file_size]);
    file_size += _add_item(&maximum_speed_manual_string, (int32_t) config.maximum_speed_manual, &ConfigFile_buffer[file_size]);
    file_size += _add_item(&beep_duration_string, (int32_t) config.beep_duration, &ConfigFile_buffer[file_size]);
    
//    //Create file content
//    file_size = 0;
//    
//    memcpy(&ConfigFile_buffer[file_size], &full_circle_in_steps_string, 21);
//    file_size += 21;
//    ltoa(&ConfigFile_buffer[file_size], config.full_circle_in_steps, 10);
//    while(ConfigFile_buffer[file_size])
//    {
//        ++file_size;
//    }
//    ConfigFile_buffer[file_size++] = '\n';
//    
//    memcpy(&ConfigFile_buffer[file_size], &inverse_direction_string, 18);
//    file_size += 18;
//    itoa(&ConfigFile_buffer[file_size], config.inverse_direction, 10);
//    while(ConfigFile_buffer[file_size])
//    {
//        ++file_size;
//    }
//    ConfigFile_buffer[file_size++] = '\n';
    
    
//    uint32_t full_circle_in_steps;
//    uint8_t inverse_direction;
//    uint16_t overshoot_in_steps;

    
    //Find and if necessary create or resize file
    file_number = fat_find_file(&configFile_name, &configFile_extention);
    if(file_number==0xFF)
    {
        file_number = fat_create_file(&configFile_name, &configFile_extention, file_size);
    }
    else
    {
        fat_resize_file(file_number, file_size);
    }
    
    //Write file content
    fat_modify_file(file_number, 0, file_size, &ConfigFile_buffer[0]);
}

void configFile_read(void)
{
    
}
