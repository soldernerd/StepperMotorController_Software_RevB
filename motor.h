/* 
 * File:   motor.h
 * Author: Luke
 *
 * Created on 12. September 2017, 21:26
 */

#ifndef MOTOR_H
#define	MOTOR_H
 
typedef enum 
{
    MOTOR_DIRECTION_CCW = -1,
    MOTOR_DIRECTION_CW = 1,
    MOTOR_DIRECTION_SHORTEST = 0
} motorDirection_t;

typedef enum
{
    MOTOR_MODE_MANUAL,
    MOTOR_MODE_PWM
}motorMode_t;

typedef enum
{
    MOTOR_MOVE_TYPE_NORMAL,
    MOTOR_MOVE_TYPE_ENDLESS
} motorMoveType_t;

typedef enum
{
    MOTOR_OVERSHOOT_WITH_OVERSHOOT,
    MOTOR_OVERSHOOT_NO_OVERSHOOT
} motorOvershoot_t;

typedef enum
{
    MOTOR_RETURN_VALUE_OK = 0,
    MOTOR_RETURN_VALUE_BUSY = 1,
    MOTOR_RETURN_VALUE_INVALID_SPEED = 2,
    MOTOR_RETURN_VALUE_INVALID_DISTANCE = 3,
    MOTOR_RETURN_VALUE_INVALID_DIRECTION = 4,
    MOTOR_RETURN_VALUE_INVALID_MOVE_TYPE = 5,
    MOTOR_RETURN_VALUE_INVALID_OVERSHOOT = 6,
    MOTOR_RETURN_VALUE_LIMIT_VIOLATED = 7,
    MOTOR_RETURN_VALUE_BUFFER_FULL = 8,
    MOTOR_RETURN_VALUE_INVALID_POSITION = 9
} motorReturnValue_t;

typedef struct
{
    motorDirection_t direction;
    uint32_t distance;
    uint16_t speed;
    motorMoveType_t type;
} motorCommand_t;

#define MOTOR_COMMAND_CUE_SIZE 8
#define MOTOR_COMMAND_CUE_MASK 0b111

void motor_init(void);
void motor_isr(void);

//Main tools
motorReturnValue_t motor_move_steps(motorDirection_t direction, uint32_t distance, uint16_t speed, motorOvershoot_t overshoot);
motorReturnValue_t motor_move_degrees_float(float distance, uint16_t speed, motorOvershoot_t overshoot);
motorReturnValue_t motor_move_degrees_int(motorDirection_t direction, uint16_t distance, uint16_t speed, motorOvershoot_t overshoot);
motorReturnValue_t motor_goto_steps(motorDirection_t direction, uint32_t position, uint16_t speed, motorOvershoot_t overshoot);
motorReturnValue_t motor_goto_degrees_float(motorDirection_t direction, float position, uint16_t speed, motorOvershoot_t overshoot);
motorReturnValue_t motor_goto_degrees_int(motorDirection_t direction, uint16_t position, uint16_t speed, motorOvershoot_t overshoot);
motorReturnValue_t motor_move_endless(motorDirection_t direction, uint16_t speed);

//Debugging functions
motorMode_t motor_get_mode(void);
uint16_t motor_get_current_speed(void);
uint16_t motor_get_maximum_speed(void);

//For the display
uint16_t motor_speed_from_index(uint16_t speed_index);

void motor_increase_manual_speed(void);
void motor_decrease_manual_speed(void);
void motor_set_manual_speed(uint16_t new_speed);

void startup(void);
void motor_start(motorDirection_t direction);
void motor_stop(void);
void motor_change_speed(uint16_t new_speed);

uint32_t motor_steps_from_degrees(uint16_t degrees);
uint32_t motor_nonzero_steps_from_degrees(uint16_t degrees);
void motor_calculate_position_in_degrees(void);


//uint8_t motor_schedule_command(motorDirection_t direction, uint32_t distance_in_steps, uint16_t speed, motorMoveType_t type);
void motor_clear_command_cue(void);
//void motor_go_to_steps_position(uint32_t target_position);
//void motor_go_to_degrees_position(float target_position);
void motor_divide_jump(void);
void motor_divide_jump_to_nearest(void);
void motor_arc_move(motorDirection_t direction);
void motor_process_cue(void);
uint8_t motor_items_in_cue(void);
void motor_set_zero(motorDirection_t direction);
//void motor_move_steps_with_overshoot(motorDirection_t direction, uint32_t distance);

#endif	/* MOTOR_H */

