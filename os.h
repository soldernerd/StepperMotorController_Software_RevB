/* 
 * File:   system.h
 * Author: Luke
 *
 * Created on 5. September 2016, 21:17
 */

#ifndef OS_H
#define	OS_H

#include <stdint.h>
#include "motor.h"
#include "i2c.h"
#include "spi.h"
#include "encoder.h"

/*
 * Application configuration
 */



#define POLARIZER

#ifdef ROTARY_TABLE_180_16
    #define CONFIG_FULL_CIRCLE_IN_STEPS 576000
    #define CONFIG_INVERSE_DIRECTION 1
    #define CONFIG_OVERSHOOT_IN_STEPS 6400
    #define CONFIG_OVERSHOOT_COST_IN_STEPS 3000
    #define CONFIG_MINIMUM_SPEED 1
    #define CONFIG_MAXIMUM_SPEED 380
    #define CONFIG_INITIAL_SPEED_ARC 47
    #define CONFIG_MAXIMUM_SPEED_ARC 305
    #define CONFIG_INITIAL_SPEED_MANUAL 99
    #define CONFIG_MAXIMUM_SPEED_MANUAL 305
    #define CONFIG_USE_CCW_LIMIT 0
    #define CONFIG_CCW_LIMIT 35500
    #define CONFIG_USE_CW_LIMIT 0
    #define CONFIG_CW_LIMIT 500
    #define CONFIG_BEEP_DURATION 10
    #define CONFIG_POSITION_DISPLAY_180 0
    #define CONFIG_MENU_STRUCTURE 0
#endif /*ROTARY_TABLE_180_16*/

#ifdef POLARIZER
    #define CONFIG_FULL_CIRCLE_IN_STEPS 218880
    #define CONFIG_INVERSE_DIRECTION 0
    #define CONFIG_OVERSHOOT_IN_STEPS 3648
    #define CONFIG_OVERSHOOT_COST_IN_STEPS 3000
    #define CONFIG_MINIMUM_SPEED 1
    #define CONFIG_MAXIMUM_SPEED 380
    #define CONFIG_INITIAL_SPEED_ARC 47
    #define CONFIG_MAXIMUM_SPEED_ARC 305
    #define CONFIG_INITIAL_SPEED_MANUAL 99
    #define CONFIG_MAXIMUM_SPEED_MANUAL 305
    #define CONFIG_USE_CCW_LIMIT 0
    #define CONFIG_CCW_LIMIT 9000
    #define CONFIG_USE_CW_LIMIT 1
    #define CONFIG_CW_LIMIT 9000
    #define CONFIG_BEEP_DURATION 10
    #define CONFIG_POSITION_DISPLAY_180 1
    #define CONFIG_MENU_STRUCTURE 1
#endif /*ROTARY_TABLE_180_16*/

#ifdef ROTARY_TABLE_4_16
    #define CONFIG_FULL_CIRCLE_IN_STEPS 12800
    #define CONFIG_INVERSE_DIRECTION 1
    #define CONFIG_OVERSHOOT_IN_STEPS 320
    #define CONFIG_MINIMUM_SPEED 1
    #define CONFIG_MAXIMUM_SPEED 300
    #define CONFIG_INITIAL_SPEED_ARC 10
    #define CONFIG_MAXIMUM_SPEED_ARC 200
    #define CONFIG_INITIAL_SPEED_MANUAL 10
    #define CONFIG_MAXIMUM_SPEED_MANUAL 200
    #define CONFIG_BEEP_DURATION 3
#endif /*ROTARY_TABLE_4_16*/


/*
 * Type definitions
 */

typedef enum
{
    MENU_STRUCTURE_NOMRAL = 0,
    MENU_STRUCTURE_SIMPLIFIED = 1
} menuStructure_t;

typedef enum 
{
    DISPLAY_STATE_MAIN = 0x00,
    DISPLAY_STATE_MAIN_SETUP = 0x01,
    DISPLAY_STATE_MAIN_DIVIDE = 0x02,
    DISPLAY_STATE_MAIN_ARC = 0x03,
    DISPLAY_STATE_MAIN_MANUAL = 0x04,
    DISPLAY_STATE_MAIN_ZERO = 0x05,
    DISPLAY_STATE_SETUP1 = 0x10,
    DISPLAY_STATE_SETUP1_CONFIRM = 0x11,
    DISPLAY_STATE_SETUP1_CANCEL = 0x12,        
    DISPLAY_STATE_SETUP2 = 0x20,
    DISPLAY_STATE_SETUP2_CCW = 0x21,
    DISPLAY_STATE_SETUP2_CW = 0x22,        
    DISPLAY_STATE_DIVIDE1 = 0x30,
    DISPLAY_STATE_DIVIDE1_CONFIRM = 0x31,
    DISPLAY_STATE_DIVIDE1_CANCEL = 0x32,
    DISPLAY_STATE_DIVIDE2 = 0x40,
    DISPLAY_STATE_DIVIDE2_NORMAL = 0x41,
    DISPLAY_STATE_ARC1 = 0x50,
    DISPLAY_STATE_ARC1_CONFIRM = 0x51,
    DISPLAY_STATE_ARC1_CANCEL = 0x52,
    DISPLAY_STATE_ARC2 = 0x60,
    DISPLAY_STATE_ARC2_CCW = 0x61,
    DISPLAY_STATE_ARC2_CANCEL = 0x62,
    DISPLAY_STATE_ARC2_CW = 0x63,
    DISPLAY_STATE_ZERO = 0x70,
    DISPLAY_STATE_ZERO_NORMAL = 0x71,
    DISPLAY_STATE_MANUAL = 0x80,
    DISPLAY_STATE_MANUAL_CCW = 0x81,
    DISPLAY_STATE_MANUAL_CANCEL = 0x82,
    DISPLAY_STATE_MANUAL_CW = 0x83,
    DISPLAY_STATE_MANUAL_BUSY = 0x84,
    DISPLAY_STATE_ENCODER_TEST = 0xF0
} displayState_t;

typedef struct
{
    volatile uint8_t subTimeSlot;
    volatile uint8_t timeSlot;
    volatile uint8_t done;
    volatile int8_t encoder1Count;
    volatile int8_t button1;
    volatile int8_t encoder2Count;
    volatile int8_t button2;
    volatile uint32_t current_position_in_steps;
    volatile int16_t absolute_position;
    uint16_t current_position_in_degrees;
    displayState_t displayState;
    uint8_t busy;
    uint16_t setup_step_size;
    motorDirection_t approach_direction;
    int16_t division;
    uint8_t divide_step_size;
    int16_t divide_position;
    int16_t divide_jump_size;
    uint16_t arc_step_size;
    int32_t arc_size;
    uint16_t arc_speed;
    motorDirection_t arc_direction;
    uint16_t manual_speed;
    motorDirection_t manual_direction;
    uint8_t beep_count;
    int16_t temperature[2];
    uint16_t external_temperature_adc_sum;
    uint8_t external_temperature_count;
    uint8_t fan_on;
    uint8_t brake_on;
} os_t;

typedef struct
{
    uint32_t full_circle_in_steps;
    uint8_t inverse_direction;
    uint16_t overshoot_in_steps;
    uint16_t overshoot_cost_in_steps;
    uint16_t minimum_speed;
    uint16_t maximum_speed;
    uint16_t initial_speed_arc;
    uint16_t maximum_speed_arc;
    uint16_t initial_speed_manual;
    uint16_t maximum_speed_manual;
    uint8_t beep_duration;
    uint8_t use_cw_limit;
    int32_t cw_limit;
    int32_t cw_limit_in_steps;
    uint8_t use_ccw_limit;
    int32_t ccw_limit;
    int32_t ccw_limit_in_steps;
    uint8_t position_display_180;
    menuStructure_t menu_structure;
} config_t;


/*
 * Global variables
 */

os_t os;
config_t config;


/*
 * Function prototypes
 */


void tmr0_isr(void);
void system_init(void);
void reboot(void);


#endif	/* OS_H */

