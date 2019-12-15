#include <stdint.h>
#include <stdlib.h>
#include <xc.h>
#include "string.h"
#include "os.h"
#include "fat16.h"
#include "config_file.h"

//This config file is read
char configFile_name[8] = {'C', 'O', 'N', 'F', 'I', 'G', ' ', ' '};
char configFile_extention[3] = {'T', 'X', 'T'};

//This config file is written with the values actually used
char configFile_used_name[8] = {'U', 'S', 'E', 'D', 'C', 'O', 'N', 'F'};
char configFile_used_extention[3] = {'T', 'X', 'T'};

uint8_t ConfigFile_buffer[512];

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
const char use_cw_limit_string[] = "USE_CW_LIMIT";
const char cw_limit_string[] = "CW_LIMIT";
const char use_ccw_limit_string[] = "USE_CCW_LIMIT";
const char ccw_limit_string[] = "CCW_LIMIT";

uint8_t _add_item(char *item_string, int32_t value, uint8_t *buffer);
uint8_t _get_item(char *item_string, char *value_string, uint8_t *buffer);
uint8_t _parse_item(char *item_string, char *value_string);

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

uint8_t _get_item(char *item_string, char *value_string, uint8_t *buffer)
{
    uint8_t item_ctr;
    uint8_t value_ctr;
    
    //Get item string (maximum length=30 plus zero character)
    for(item_ctr=0;item_ctr<30;++item_ctr)
    {
        //Item string is terminated by a equal sign
        if(buffer[item_ctr] == '=')
        {
            break;
        }
        item_string[item_ctr] = buffer[item_ctr];
    }
    item_string[item_ctr++] = 0;
    
    //Get item string (maximum length=11 plus zero character)
    for(value_ctr=0;value_ctr<11;++value_ctr)
    {
        //Value string is terminated by a line break (be tolerant of different system's line endings)
        if((buffer[item_ctr+value_ctr]=='\r') || (buffer[item_ctr+value_ctr]=='\n'))
        {
            break;
        }
        value_string[value_ctr] = buffer[item_ctr+value_ctr];
    }
    value_string[value_ctr++] = 0;
    
    //Continue while white space or line break continues 
    if((buffer[item_ctr+value_ctr]=='\r') || (buffer[item_ctr+value_ctr]=='\n'))
    {
        return (item_ctr+value_ctr+1);
    }
    else
    {
        return (item_ctr+value_ctr);
    }
}


uint8_t _parse_item(char *item_string, char *value_string)
{
    if(stricmp(item_string, full_circle_in_steps_string)==0)
    {
        config.full_circle_in_steps = atol(value_string);
        return 1;
    }
    
    if(stricmp(item_string, overshoot_in_steps_string)==0)
    {
        config.overshoot_in_steps = atoi(value_string);
        return 2;
    }
    
    if(stricmp(item_string, inverse_direction_string)==0)
    {
        config.inverse_direction = atol(value_string);
        return 3;
    }
    
    if(stricmp(item_string, overshoot_in_steps_string)==0)
    {
        config.overshoot_in_steps = atoi(value_string);
        return 4;
    }
    
    if(stricmp(item_string, overshoot_cost_in_steps_string)==0)
    {
        config.overshoot_cost_in_steps = atoi(value_string);
        return 5;
    }
    
    if(stricmp(item_string, minimum_speed_string)==0)
    {
        config.minimum_speed = atoi(value_string);
        return 6;
    }
    
    if(stricmp(item_string, maximum_speed_string)==0)
    {
        config.maximum_speed = atoi(value_string);
        return 7;
    }
    
    if(stricmp(item_string, initial_speed_arc_string)==0)
    {
        config.initial_speed_arc = atoi(value_string);
        return 8;
    }
    
    if(stricmp(item_string, maximum_speed_arc_string)==0)
    {
        config.maximum_speed_arc = atoi(value_string);
        return 9;
    }
    
    if(stricmp(item_string, initial_speed_manual_string)==0)
    {
        config.initial_speed_manual = atoi(value_string);
        return 10;
    }
    
    if(stricmp(item_string, maximum_speed_manual_string)==0)
    {
        config.maximum_speed_manual = atoi(value_string);
        return 11;
    }
    
    if(stricmp(item_string, beep_duration_string)==0)
    {
        config.beep_duration = atoi(value_string);
        return 12;
    }
    
    if(stricmp(item_string, use_cw_limit_string)==0)
    {
        config.use_cw_limit = atol(value_string);
        return 13;
    }
    
    if(stricmp(item_string, cw_limit_string)==0)
    {
        config.cw_limit = atol(value_string);
        return 14;
    }
    
    if(stricmp(item_string, use_ccw_limit_string)==0)
    {
        config.use_ccw_limit = atol(value_string);
        return 15;
    }
    
    if(stricmp(item_string, ccw_limit_string)==0)
    {
        config.ccw_limit = atol(value_string);
        return 16;
    }
    
    return 0;
}

void configFile_readDefault(void)
{
    config.full_circle_in_steps = CONFIG_FULL_CIRCLE_IN_STEPS;
    config.inverse_direction = CONFIG_INVERSE_DIRECTION;
    config.overshoot_in_steps = CONFIG_OVERSHOOT_IN_STEPS;
    config.overshoot_cost_in_steps = CONFIG_OVERSHOOT_COST_IN_STEPS;
    config.minimum_speed = CONFIG_MINIMUM_SPEED;
    config.maximum_speed = CONFIG_MAXIMUM_SPEED;
    config.maximum_speed_arc = CONFIG_MAXIMUM_SPEED_ARC;
    config.maximum_speed_manual = CONFIG_MAXIMUM_SPEED_MANUAL;
    config.use_ccw_limit = CONFIG_USE_CCW_LIMIT;
    config.ccw_limit = CONFIG_CCW_LIMIT;
    config.use_cw_limit = CONFIG_USE_CW_LIMIT;
    config.cw_limit = CONFIG_CW_LIMIT;
    config.beep_duration = CONFIG_BEEP_DURATION;
}


void configFile_read(void)
{
    uint8_t file_number;
    uint32_t file_size;
    uint32_t position;
    char item_string[31];
    char value_string[12];
    
    //Locate config file
    file_number = fat_find_file(&configFile_name, &configFile_extention);
    if(file_number==0xFF)
    {
        //File not found
        return;
    }
    
    //Determine file size
    file_size = fat_get_file_size(file_number);
    
    //Read file chunk by chunk
    position = 0;
    while((file_size-position)>3)
    { 
        if((position+45) > file_size)
        {
            //Read until end of file
            fat_read_from_file(file_number, position, file_size-position, &ConfigFile_buffer[0]);
        }
        else
        {
            //Read 45 characters
            fat_read_from_file(file_number, position, 45, &ConfigFile_buffer[0]);
        }
        
        //Split item and value pair
        position += _get_item(&item_string[0], &value_string[0], &ConfigFile_buffer[0]);
        
        //Parse item and value pair and save value in config
        _parse_item(&item_string[0], &value_string[0]);
    }
}


void configFile_write(void)
{
    uint8_t file_number;
    uint32_t file_size;
    
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
    file_size += _add_item(&use_ccw_limit_string, (int32_t) config.use_ccw_limit, &ConfigFile_buffer[file_size]);
    file_size += _add_item(&ccw_limit_string, (int32_t) config.ccw_limit, &ConfigFile_buffer[file_size]);
    file_size += _add_item(&use_cw_limit_string, (int32_t) config.use_cw_limit, &ConfigFile_buffer[file_size]);
    file_size += _add_item(&cw_limit_string, (int32_t) config.cw_limit, &ConfigFile_buffer[file_size]);
    file_size += _add_item(&beep_duration_string, (int32_t) config.beep_duration, &ConfigFile_buffer[file_size]);

    //Find and if necessary create or resize file
    file_number = fat_find_file(&configFile_used_name, &configFile_used_extention);
    if(file_number==0xFF)
    {
        file_number = fat_create_file(&configFile_used_name, &configFile_used_extention, file_size);
    }
    else
    {
        fat_resize_file(file_number, file_size);
    }
    
    //Write file content
    fat_modify_file(file_number, 0, file_size, &ConfigFile_buffer[0]);
}


