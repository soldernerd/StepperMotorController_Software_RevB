#include <stdint.h>
#include <xc.h>
//#include <pic18f26j50.h>
#include "motor.h"
#include "motor_config.h"
#include "hardware_config.h"
#include "i2c.h"
#include "application_config.h"
#include "os.h"

//Motor command cue
motorCommand_t motor_command_cue[MOTOR_COMMAND_CUE_SIZE];
uint8_t motor_cue_read_index; 
uint8_t motor_cue_write_index; 

//Current status of motor while it is running
motorMoveType_t motor_move_type;
motorDirection_t motor_direction;
uint16_t motor_maximum_speed;
volatile motorMode_t motor_mode;
volatile uint16_t motor_current_speed;
volatile uint32_t motor_current_stepcount;
volatile uint32_t motor_final_stepcount;
volatile uint32_t motor_next_speed_check;

//Static functions
static void _motor_run(motorDirection_t direction, uint32_t distance_in_steps, uint16_t speed, motorMoveType_t type);
static motorReturnValue_t _motor_schedule_command(motorDirection_t direction, uint32_t distance_in_steps, uint16_t speed, motorMoveType_t type);
static motorReturnValue_t _motor_move_steps(motorDirection_t direction, uint32_t distance_in_steps, uint16_t speed, motorOvershoot_t overshoot, motorMoveType_t type);
static motorReturnValue_t _motor_move_back_forth(void);
static motorDirection_t _motor_decide_direction(uint32_t target_position_in_steps, motorOvershoot_t overshoot);
static uint32_t _motor_steps_from_degrees_float(float degrees);
static uint32_t _motor_steps_from_degrees_int(uint16_t degrees);
static int32_t _motor_distance_to_limit(motorDirection_t direction);
static uint32_t _motor_calculate_distance(motorDirection_t direction, uint32_t target_position);

/* ****************************************************************************
 *  Static Functions
 * ****************************************************************************/

static void _motor_run(motorDirection_t direction, uint32_t distance_in_steps, uint16_t speed, motorMoveType_t type)
{
    //Error checking is done when items are added to the cue
    //So here we can safely assume that we are working with valid inputs
    //This function is only called by motor_process_cue()
    
    //Save direction, distance, speed & move type
    motor_direction = direction;
    motor_final_stepcount = distance_in_steps;
    motor_maximum_speed = speed;
    motor_move_type = type;
    
    //Set distance to maximum if this is an endless move
    if(motor_move_type==MOTOR_MOVE_TYPE_ENDLESS)
    {
        motor_final_stepcount = 0xFFFFFFFF;
    }
    
    //Initialize variables
    motor_current_speed = 0;
    motor_current_stepcount = 0;
    motor_next_speed_check = motor_steps_lookup[1];
    
    //Disable PWM module
    CCP1CONbits.CCP1M = 0b0000;
    
    //Set output pins
    MOTOR_ENABLE_PIN = 0; //Enable drive
    if(direction==MOTOR_DIRECTION_CCW)
    {
        if(config.inverse_direction)
            MOTOR_DIRECTION_PIN = 1;
        else
            MOTOR_DIRECTION_PIN = 0;
    }
    else
    {
        if(config.inverse_direction)
            MOTOR_DIRECTION_PIN = 0;
        else
            MOTOR_DIRECTION_PIN = 1;
    }
    
    //Set pin high. This is already the first step
    MOTOR_STEP_PIN = 1;
    
    //Keep track of position
    ++motor_current_stepcount;
    
    //Calculate new position
    os.current_position_in_steps += motor_direction;
    if(os.current_position_in_steps==config.full_circle_in_steps)
    {
        os.current_position_in_steps = 0;
        ++os.absolute_position;
    }
    if(os.current_position_in_steps==0xFFFFFFFF)
    {
        os.current_position_in_steps = (config.full_circle_in_steps-1);
        --os.absolute_position;
    }
    
    //Manually control step
    PPSUnLock();
    MOTOR_STEP_PPS = 0;
    PPSLock();
    
    //Set motor mode
    motor_mode = MOTOR_MODE_MANUAL;
    
    //Set up timer 2 and PWM module
    //Prescaler
    T2CONbits.T2CKPS = motor_prescaler_lookup[motor_current_speed];
    //Period
    PR2 = motor_period_lookup[motor_current_speed];
    //Postscaler
    T2CONbits.T2OUTPS = motor_postscaler_lookup[motor_current_speed];
    //Duty cycle = 50%
    CCPR1L = PR2>>1;
    
    //Configure interrupts
    PIR1bits.TMR2IF = 0;
    PIE1bits.TMR2IE = 1;
    
    //Clear and start timer
    TMR2 = 0;
    T2CONbits.TMR2ON = 1;
    
    //Indicate that the motor is running
    os.busy = 1;
}

// Adds a move to the motor's command cue
// This function is ONLY called by the function _motor_move_steps() and _motor_move_back_forth()
static motorReturnValue_t _motor_schedule_command(motorDirection_t direction, uint32_t distance_in_steps, uint16_t speed, motorMoveType_t type)
{
    if(motor_items_in_cue()==MOTOR_COMMAND_CUE_SIZE-1)
    {
        //Buffer is full, indicate an error
        return MOTOR_RETURN_VALUE_BUFFER_FULL;
    }
    
    if((direction!=MOTOR_DIRECTION_CW) && (direction!=MOTOR_DIRECTION_CCW))
    {
        //Not a valid direction, indicate an error
        return MOTOR_RETURN_VALUE_INVALID_DIRECTION;
    }
    
    if(distance_in_steps==0)
    {
        //Not a valid direction, indicate an error
        return MOTOR_RETURN_VALUE_INVALID_DISTANCE;
    }
    
    if((type!=MOTOR_MOVE_TYPE_NORMAL) && (type!=MOTOR_MOVE_TYPE_ENDLESS))
    {
        //Not a valid move type, indicate an error
        return MOTOR_RETURN_VALUE_INVALID_MOVE_TYPE;
    }
    
    if(speed==0)
    {
        //Not a valid speed, indicate an error
        return MOTOR_RETURN_VALUE_INVALID_SPEED;
    }
    
    //Limit speed to device's global maximum speed
    if(speed>config.maximum_speed)
    {
        speed = config.maximum_speed;
    }
    
    //Add element
    motor_command_cue[motor_cue_write_index].direction = direction;
    motor_command_cue[motor_cue_write_index].distance = distance_in_steps;
    motor_command_cue[motor_cue_write_index].speed = speed;
    motor_command_cue[motor_cue_write_index].type = type;
    
    //Increment write index
    ++motor_cue_write_index;
    motor_cue_write_index &= MOTOR_COMMAND_CUE_MASK;
    
    //Indicate success
    return MOTOR_RETURN_VALUE_OK;
}

static motorReturnValue_t _motor_move_steps(motorDirection_t direction, uint32_t distance_in_steps, uint16_t speed, motorOvershoot_t overshoot, motorMoveType_t type)
{
    int32_t distance_to_limit;
    motorReturnValue_t return_value_1;
    motorReturnValue_t return_value_2;
    
    if(os.busy || motor_items_in_cue()!=0)
    {
        //We are busy doing other things, indicate an error
        return MOTOR_RETURN_VALUE_BUSY;
    }
  
    if((direction!=MOTOR_DIRECTION_CW) && (direction!=MOTOR_DIRECTION_CCW))
    {
        //Not a valid direction, indicate an error
        return MOTOR_RETURN_VALUE_INVALID_DIRECTION;
    }
    
    if(distance_in_steps==0)
    {
        //Not a valid direction, indicate an error
        return MOTOR_RETURN_VALUE_INVALID_DISTANCE;
    }
    
    if(speed==0)
    {
        //Not a valid speed, indicate an error
        return MOTOR_RETURN_VALUE_INVALID_SPEED;
    }
    
    if(!((type==MOTOR_MOVE_TYPE_NORMAL) || (type==MOTOR_MOVE_TYPE_ENDLESS)))
    {
        //Not a valid move type, indicate an error
        return MOTOR_RETURN_VALUE_INVALID_MOVE_TYPE;
    }
    
    if(!((overshoot==MOTOR_OVERSHOOT_NO_OVERSHOOT) || (overshoot==MOTOR_OVERSHOOT_WITH_OVERSHOOT)))
    {
        //Not a valid overshoot type, indicate an error
        return MOTOR_RETURN_VALUE_INVALID_OVERSHOOT;
    }
    
    //Make sure target position is within limits
    //Temporarily violating limits in order to overshoot is okay
    
    //Get distance to limit
    distance_to_limit = _motor_distance_to_limit(direction);
    
    //Immediately return if limit is already reached or even exceeded for whatever reason
    if(distance_to_limit<=0)
    {
        return MOTOR_RETURN_VALUE_LIMIT_VIOLATED;
    }
    
    //If there is a limit, there can not be any unlimited moves
    if(distance_to_limit!=2147483647)
    {
        type = MOTOR_MOVE_TYPE_NORMAL;
    }
    
    //Reduce distance if limit is closer than requested move
    if(distance_in_steps>((uint32_t) distance_to_limit)) //at this point we can be sure that distance_to_limit is strictly positive
    {
       distance_in_steps = (uint32_t) distance_to_limit;
    }
    
    //No overshoot is desired. 
    //Just schedule a single command
    if(overshoot==MOTOR_OVERSHOOT_NO_OVERSHOOT)
    {
        //Debug
        return _motor_schedule_command(direction, distance_in_steps, speed, type);
        //return _motor_schedule_command(direction, distance_in_steps, speed, type);
    }
    
    //Overshoot is desired but direction coincides with approach direction
    //Just schedule a single command
    if(direction==os.approach_direction)
    {
        return _motor_schedule_command(direction, distance_in_steps, speed, type);
    }
    
    //Overshoot is desired and direction is opposite to approach direction
    //But overshoot distance is zero
    //Just schedule a single command
    if(config.overshoot_in_steps==0)
    {
        return _motor_schedule_command(direction, distance_in_steps, speed, type);
    }
    
    //Overshoot is desired and direction is opposite to approach direction
    //Overshoot distance is strictly larger than zero
    //Schedule 2 moves
    distance_in_steps += config.overshoot_in_steps;
    return_value_1 =  _motor_schedule_command(direction, distance_in_steps, speed, type);
    return_value_2 =  _motor_schedule_command(os.approach_direction, (uint32_t) config.overshoot_in_steps, speed, type);
    
    //Return the larger of the two return values
    //0 means no error so if there is an error, we will return it
    if(return_value_1 > return_value_2)
    {
        return return_value_1;
    }
    else
    {
        return return_value_2;
    }
}

static motorReturnValue_t _motor_move_back_forth(void)
{
    motorReturnValue_t return_value_1;
    motorReturnValue_t return_value_2;
    
    if(os.busy || motor_items_in_cue()!=0)
    {
        //We are busy doing other things, indicate an error
        return MOTOR_RETURN_VALUE_BUSY;
    }
    
    if(config.overshoot_in_steps==0)
    {
        //There is nothing for us to do
        return MOTOR_RETURN_VALUE_OK;
    }
    
    if(os.approach_direction==MOTOR_DIRECTION_CW)
    {
        return_value_1 =  _motor_schedule_command(MOTOR_DIRECTION_CCW, config.overshoot_in_steps, 0xFFFF, MOTOR_MOVE_TYPE_NORMAL);
        return_value_2 =  _motor_schedule_command(MOTOR_DIRECTION_CW, config.overshoot_in_steps, 0xFFFF, MOTOR_MOVE_TYPE_NORMAL);
    }
    else
    {
        return_value_1 =  _motor_schedule_command(MOTOR_DIRECTION_CW, config.overshoot_in_steps, 0xFFFF, MOTOR_MOVE_TYPE_NORMAL);
        return_value_2 =  _motor_schedule_command(MOTOR_DIRECTION_CCW, config.overshoot_in_steps, 0xFFFF, MOTOR_MOVE_TYPE_NORMAL);
    }
    
    //Return the larger of the two return values
    //0 means no error so if there is an error, we will return it
    if(return_value_1 > return_value_2)
    {
        return return_value_1;
    }
    else
    {
        return return_value_2;
    }
}

static motorDirection_t _motor_decide_direction(uint32_t target_position_in_steps, motorOvershoot_t overshoot)
{
    //Find the direction in which a position on the circle is reached fastest
    //We do not care about the absolute position
    //But we need to take into account the added distance and cost of overshooting and coming back
    //And there may be limits preventing us from taking the shorter way
    
    uint32_t distance_to_target_cw;
    uint32_t distance_to_target_ccw;
    
    int32_t distance_to_limit_cw;
    int32_t distance_to_limit_ccw;
    
    //Get distances (without overshoot)
    distance_to_target_cw = _motor_calculate_distance(MOTOR_DIRECTION_CW, target_position_in_steps);
    distance_to_target_ccw = _motor_calculate_distance(MOTOR_DIRECTION_CCW, target_position_in_steps);
    
    //Get distances to limits
    distance_to_limit_cw = _motor_distance_to_limit(MOTOR_DIRECTION_CW);
    distance_to_limit_ccw = _motor_distance_to_limit(MOTOR_DIRECTION_CCW);
    
    //If one of the moves violates a limit, we return the other
    //If both are beyond limit, we return MOTOR_DIRECTION_SHORTEST, indicating a problem
    if((distance_to_target_cw>distance_to_limit_cw) && (distance_to_target_ccw>distance_to_limit_ccw))
    {
        return MOTOR_DIRECTION_SHORTEST;
    }
    if(distance_to_target_cw>distance_to_limit_cw)
    {
        return MOTOR_DIRECTION_CCW;
    }
    if(distance_to_target_ccw>distance_to_limit_ccw)
    {
        return MOTOR_DIRECTION_CW;
    }
    
    //So we can reach the target position both ways. Now which is shorter?
    //Add twice overshoot distance (back and forth) plus overshoot cost where applicable
    if(overshoot==MOTOR_OVERSHOOT_WITH_OVERSHOOT)
    {
        if(os.approach_direction==MOTOR_DIRECTION_CW)
        {
            //Approach direction is CW so we need to add the cost to moving CCW
            distance_to_target_ccw += config.overshoot_in_steps;
            distance_to_target_ccw += config.overshoot_in_steps;
            distance_to_target_ccw += config.overshoot_cost_in_steps;
        }
        if(os.approach_direction==MOTOR_DIRECTION_CCW)
        {
            //Approach direction is CCW so we need to add the cost to moving CW
            distance_to_target_cw += config.overshoot_in_steps;
            distance_to_target_cw += config.overshoot_in_steps;
            distance_to_target_cw += config.overshoot_cost_in_steps;
        }
    }
    
    //Return the shorter direction
    if(distance_to_target_ccw<distance_to_target_cw)
    {
        return MOTOR_DIRECTION_CCW;
    }
    else
    {
        return MOTOR_DIRECTION_CW;
    }
}

static uint32_t _motor_steps_from_degrees_float(float degrees)
{
    //Make sure we're returning zero when the input is zero
    if(degrees==0.0)
    {
        return 0;
    }
    
    //Make sure we're dealing with a positive number
    if(degrees<0)
    {
        degrees = -degrees;
    }
    
    //Calculate number of steps
    degrees *= config.full_circle_in_steps;
    degrees /= 360.0;
    degrees += 0.5; //for proper rounding
    
    //Return value as integer
    return (uint32_t) degrees;
}

static uint32_t _motor_steps_from_degrees_int(uint16_t degrees)
{
    double temp;
    
    //Make sure we're returning zero when the input is zero
    if(degrees==0)
    {
        return 0;
    }
    
    //Convert input to a floating point number
    temp = (float) degrees;
    
    //Calculate number of steps
    temp *= config.full_circle_in_steps;
    temp /= 36000.0;
    temp += 0.5; //for proper rounding
    
    //Return value as integer
    return (uint32_t) temp;
}

static int32_t _motor_distance_to_limit(motorDirection_t direction)
{
    //A negative number means the limit is already exceeded
    int32_t absolute_position_in_steps;
    int32_t distance_to_limit_in_steps;
    
    //Return maximum value if no limit applies
    if(direction==MOTOR_DIRECTION_CCW && config.use_ccw_limit==0)
    {
        return 2147483647; //largest int32 value
    }
    if(direction==MOTOR_DIRECTION_CW && config.use_cw_limit==0)
    {
        return 2147483647; //largest int32 value
    }
    
    //Calculate absolute position
    absolute_position_in_steps = os.absolute_position;
    absolute_position_in_steps *= config.full_circle_in_steps;
    absolute_position_in_steps += os.current_position_in_steps;
    
    //Calculate distance to limit
    if(direction==MOTOR_DIRECTION_CCW)
    {
        distance_to_limit_in_steps = absolute_position_in_steps - config.ccw_limit_in_steps;
    }
    else
    {
        distance_to_limit_in_steps = config.cw_limit_in_steps - absolute_position_in_steps;
    }

    return distance_to_limit_in_steps;
}

static uint32_t _motor_calculate_distance(motorDirection_t direction, uint32_t target_position)
{
    uint32_t distance;
    
    //Calculate distance in the given direction
    if(direction==MOTOR_DIRECTION_CW)
    {
        distance = target_position - os.current_position_in_steps;
        if(distance>config.full_circle_in_steps)
        {
            //An underflow has occurred, fix this by adding a full circle
            distance += config.full_circle_in_steps;
        }
    }
    else
    {
        distance = os.current_position_in_steps - target_position;
        if(distance>config.full_circle_in_steps)
        {
            //An underflow has occurred, fix this by adding a full circle
            distance += config.full_circle_in_steps;
        }
    }
    
    return distance;
}







motorMode_t motor_get_mode(void)
{
    return motor_mode;
}

//Translate 0.01 degrees into steps
uint32_t motor_steps_from_degrees(uint16_t degrees)
{
    float steps;
    
    //Safeguard against rounding issues
    if(degrees==0)
    {
        return 0;
    }
    
    steps = (float) degrees;
    steps *= config.full_circle_in_steps;
    steps /= 36000;
    //steps += 0.5;
    return (uint32_t) steps;
}

//Translate 0.01 degrees into steps but never return zero
uint32_t motor_nonzero_steps_from_degrees(uint16_t degrees)
{
    uint32_t steps;
    steps = motor_steps_from_degrees(degrees);
    if(steps==0)
        steps = 1;
    return steps;
}

void motor_calculate_position_in_degrees(void)
{
    float tmp;
    //Calculate position in 0.01 degrees
    tmp = (float) os.current_position_in_steps;
    tmp *= 36000;
    tmp /= config.full_circle_in_steps;
    //tmp += 0.5; //Round correctly
    os.current_position_in_degrees = (uint16_t) tmp;
    if(os.current_position_in_degrees==36000)
    {
        //Due to rounding, this might happen under certain conditions...
        os.current_position_in_degrees = 0;
    }
}

uint16_t _motor_get_speed_in_degrees(uint16_t speed_index)
{
    uint32_t lookup;
    float speed;
    
    lookup = motor_speed_lookup[speed_index];
    lookup *= 16;
    lookup *= 360;
    lookup *= 100;
    
    speed = (float) lookup;
    speed /= config.full_circle_in_steps;
    //speed += 0.5;
    
    return (uint16_t) speed;
}

uint32_t _step_position_from_divide_position(int16_t divide_position)
{
    float target_position;
    
    //Calculate target position in terms of steps
    target_position = (float) config.full_circle_in_steps;
    target_position *= (float) divide_position;
    target_position /= (float) os.division;
    
    return (uint32_t) target_position;
}

uint16_t motor_get_maximum_speed(void)
{
    return _motor_get_speed_in_degrees(motor_maximum_speed);
}

uint16_t motor_get_current_speed(void)
{
    return _motor_get_speed_in_degrees(motor_current_speed);
}

uint16_t motor_speed_from_index(uint16_t speed_index)
{
    return _motor_get_speed_in_degrees(speed_index);
}

uint8_t motor_items_in_cue(void)
{
    return ((motor_cue_write_index-motor_cue_read_index) & MOTOR_COMMAND_CUE_MASK);
}

void motor_clear_command_cue(void)
{
    //This means that there are no more elements in the cue
    motor_cue_read_index = 0; 
    motor_cue_write_index = 0;
}

void motor_process_cue(void)
{
    if(motor_items_in_cue()==0)
    {
        //Cue is empty, nothing to do
        return;
    }
    
    if(os.busy)
    {
        //Motor is busy, maybe next time
        return;
    }
    
    //Run oldest command
    _motor_run(
        motor_command_cue[motor_cue_read_index].direction,
        motor_command_cue[motor_cue_read_index].distance,
        motor_command_cue[motor_cue_read_index].speed,
        motor_command_cue[motor_cue_read_index].type
    );
    //Inrement read index
    ++motor_cue_read_index;
    motor_cue_read_index  &= MOTOR_COMMAND_CUE_MASK;
}

void motor_init(void)
{
    //Initialize variables
    motor_cue_read_index = 0; 
    motor_cue_write_index = 0;
    
    //Initialize timer 2
    //Use timer2 for CCP1 module
    
    //fix this
    CCPTMRS0bits.C2TSEL = 0b000;
//    TCLKCONbits.T3CCP2 = 0b0;
//    TCLKCONbits.T3CCP1 = 0b1;
    
    //Single output mode
    CCP1CONbits.P1M = 0b00;
    
    //Duty cycle LSBs
    CCP1CONbits.DC1B = 0b00;
    
    //Enable drive
    MOTOR_ENABLE_PIN = 0;
}



void motor_isr(void)
{
    uint32_t steps_remaining;
    uint16_t steps_until_standstill;
    uint16_t steps_until_standstill_if_accelerate;
    
    //Clear interrupt flag
    PIR1bits.TMR2IF = 0;
    
    //If this is an endless move, the distance to target remains at maximum
    if(motor_move_type==MOTOR_MOVE_TYPE_ENDLESS)
    {
        motor_final_stepcount = 0xFFFFFFFF;
    }
    
    if(motor_mode==MOTOR_MODE_MANUAL)
    {
        //Need to toggle step pin manually
        if(MOTOR_STEP_PIN)
        {
            //Set pin low
            MOTOR_STEP_PIN = 0;
            
            //Check if we are done altogether
            if(motor_current_stepcount==motor_final_stepcount)
            {
                //We are done. Stop everything
                //Disable timer, disable motor drive, clear and disable interrupts
                T2CONbits.TMR2ON = 0;
                //MOTOR_ENABLE_PIN = 1; //disable
                PIR1bits.TMR2IF = 0;
                PIE1bits.TMR2IE = 0;
                //Write new position to EEPROM
                i2c_eeprom_save_position();
                //Indicate that the motor is now idle
                os.busy = 0;
            }
            
            //We're done for this time
            return;
        }
        else
        {
            //Set pin high
            MOTOR_STEP_PIN = 1;
        }
    }
    
    ++motor_current_stepcount;
    
    //Calculate new position
    os.current_position_in_steps += motor_direction;
    if(os.current_position_in_steps==config.full_circle_in_steps)
    {
        os.current_position_in_steps = 0;
        ++os.absolute_position;
    }
    if(os.current_position_in_steps==0xFFFFFFFF)
    {
        os.current_position_in_steps = (config.full_circle_in_steps-1);
        --os.absolute_position;
    }
    
    //Check if we need to (maybe) change speed.
    if(motor_current_stepcount==motor_next_speed_check)
    {  
        //Calculate some basic values
        steps_remaining = motor_final_stepcount - motor_current_stepcount;
        steps_until_standstill = motor_steps_lookup[motor_current_speed];
        steps_until_standstill_if_accelerate = motor_steps_lookup[motor_current_speed+2];
                
        if((motor_current_speed>motor_maximum_speed) || (steps_until_standstill>=steps_remaining))
        {
            //Need to de-accelerate
            if(motor_current_speed>0)
            {
                --motor_current_speed;
            }
            
            //Check if we need to change drive mode
            if((motor_mode==MOTOR_MODE_PWM) && (motor_postscaler_lookup[motor_current_speed]>0))
            {
                //Need to change from PWM mode to manual mode
                motor_mode = MOTOR_MODE_MANUAL;
            
                //Control steps manually
                MOTOR_STEP_PIN = 1;
                PPSUnLock();
                MOTOR_STEP_PPS = 0;
                PPSLock();
                
                //clear timer
                TMR2 = 0;
                
                //Turn off PWM module
                CCP1CONbits.CCP1M = 0b0000;
            }
        
            //Update parameters
            //Prescaler
            T2CONbits.T2CKPS = motor_prescaler_lookup[motor_current_speed];
            //Period
            PR2 = motor_period_lookup[motor_current_speed];
            //Postscaler
            T2CONbits.T2OUTPS = motor_postscaler_lookup[motor_current_speed];
            //Duty cycle = 50%
            CCPR1L = PR2>>1;
            
            //Set when to de-accelerate next time
            if(motor_current_speed>0)
            {
                motor_next_speed_check = motor_current_stepcount + motor_steps_lookup[motor_current_speed] - motor_steps_lookup[motor_current_speed-1];
            }
            else
            {
                motor_next_speed_check = motor_current_stepcount + motor_steps_lookup[motor_current_speed];
            }
        }
        else if((motor_current_speed==motor_maximum_speed) || (steps_until_standstill_if_accelerate>=steps_remaining))
        {
            //Maintain current speed
            //Calculate when to revise speed next time
            motor_next_speed_check = motor_current_stepcount + motor_steps_lookup[motor_current_speed+1] - motor_steps_lookup[motor_current_speed];
        }
        else
        {
            //can accelerate further
            ++motor_current_speed;
            
            //Update parameters
            //Prescaler
            T2CONbits.T2CKPS = motor_prescaler_lookup[motor_current_speed];
            //Period
            PR2 = motor_period_lookup[motor_current_speed];
            //Postscaler
            T2CONbits.T2OUTPS = motor_postscaler_lookup[motor_current_speed];
            //Duty cycle = 50%
            CCPR1L = PR2>>1;
            
            if((motor_mode==MOTOR_MODE_MANUAL) && (motor_postscaler_lookup[motor_current_speed]==0))
            {
                //Need to change from manual mode to PWM mode
                motor_mode = MOTOR_MODE_PWM;
                
                //Enable PWM module
                CCP1CONbits.CCP1M = 0b1100;
                
                //Control step pin via PWM module
                PPSUnLock();
                MOTOR_STEP_PPS = PPS_FUNCTION_CCP1_OUTPUT;
                PPSLock();
            }

            //Calculate when to revise speed next time
            motor_next_speed_check = motor_current_stepcount + motor_steps_lookup[motor_current_speed+1] - motor_steps_lookup[motor_current_speed];
        }
    }
    
//    //Just to make sure we don't miss a step in case the interrupt flag has been set again in the mean time
//    if(PIR1bits.TMR2IF)
//    {
//        //Clear interrupt flag
//        PIR1bits.TMR2IF = 0;
//        //Keep track of position
//        ++motor_current_stepcount;
//        
//        //Calculate new position
//        os.current_position_in_steps += motor_direction;
//        if(os.current_position_in_steps==config.full_circle_in_steps)
//        {
//            os.current_position_in_steps = 0;
//            ++os.absolute_position;
//        }
//        if(os.current_position_in_steps==0xFFFFFFFF)
//        {
//            os.current_position_in_steps = (config.full_circle_in_steps-1);
//            --os.absolute_position;
//        }
//    }
}

void motor_stop(void)
{
    //Temporarily disable interrupts
    INTCONbits.GIE = 0;
    
    //Tell motor to stop
    motor_move_type=MOTOR_MOVE_TYPE_NORMAL; //This is no longer an endless move if it ever was
    motor_final_stepcount = motor_current_stepcount + motor_steps_lookup[motor_current_speed];
    
    //Re-enable interrupts
    INTCONbits.GIE = 1;
}

void motor_increase_manual_speed(void)
{
    if(os.manual_speed<=0xFFFF)
    {
        motor_set_manual_speed(os.manual_speed+1);
    }
}

void motor_decrease_manual_speed(void)
{
    if(os.manual_speed>0)
    {
        motor_set_manual_speed(os.manual_speed-1);
    }
}

void motor_set_manual_speed(uint16_t new_speed)
{
    if(new_speed>config.maximum_speed_manual)
    {
        os.manual_speed = config.maximum_speed_manual;
    }
    else if(new_speed<config.minimum_speed)
    {
        os.manual_speed = config.minimum_speed;
    }
    else
    {
        os.manual_speed = new_speed;
    }
    motor_change_speed(new_speed);
}

void motor_change_speed(uint16_t new_speed)
{
    motor_maximum_speed = new_speed;
}

void motor_divide_jump(void)
{
    int16_t target_divide_position;
    uint32_t target_position_in_steps;
    
    //Calculate target position
    if(os.divide_jump_size>0)
    {
        target_divide_position = os.divide_position + os.divide_jump_size;
        if(target_divide_position>=os.division)
        {
            target_divide_position -= os.division;
        }
    }
    else
    {
        target_divide_position = os.divide_position + os.divide_jump_size;
        if(target_divide_position<0)
        {
            target_divide_position += os.division;
        }
    }

    //Calculate target position in terms of steps
    target_position_in_steps = _step_position_from_divide_position(target_divide_position);
    
    //Save position and move motor
    os.divide_position = target_divide_position;
    motor_goto_steps(MOTOR_DIRECTION_SHORTEST, target_position_in_steps, 0xFFFF, MOTOR_OVERSHOOT_WITH_OVERSHOOT);
}

void motor_divide_jump_to_nearest(void)
{
    float divide_position_float;
    int16_t nearest_divide_position;
    uint32_t nearest_position_in_steps;
    
    divide_position_float = (float) os.current_position_in_steps;
    divide_position_float *= (float) os.division;
    divide_position_float /= (float) config.full_circle_in_steps;
    divide_position_float += 0.5;
    nearest_divide_position = (int16_t) divide_position_float;
    
    //Calculate target position in terms of steps
    nearest_position_in_steps = _step_position_from_divide_position(nearest_divide_position);
    
    //Save position and move motor
    os.divide_position = nearest_divide_position;
    motor_goto_steps(MOTOR_DIRECTION_SHORTEST, nearest_position_in_steps, 0xFFFF, MOTOR_OVERSHOOT_WITH_OVERSHOOT);
}

void motor_arc_move(motorDirection_t direction)
{
    float arc_in_steps;
    
    //Calculate arc size in steps
    arc_in_steps = (float) os.arc_size;
    arc_in_steps *= (float) config.full_circle_in_steps;
    arc_in_steps /= (float) 36000.0;
    
    //Move motor
    motor_move_steps(direction, (uint32_t) arc_in_steps, os.arc_speed, MOTOR_OVERSHOOT_NO_OVERSHOOT);
}

void motor_set_zero(motorDirection_t direction)
{
    //Reset all position variables
    os.approach_direction = direction;
    os.displayState = DISPLAY_STATE_MAIN_SETUP;
    os.current_position_in_steps = 0;
    os.absolute_position = 0;
    os.current_position_in_degrees = 0;
    os.divide_position = 0;
    
    //Run the motor
    _motor_move_back_forth();
}

/* ****************************************************************************
 *  Main tools
 * ****************************************************************************/

motorReturnValue_t motor_move_steps(motorDirection_t direction, uint32_t distance, uint16_t speed, motorOvershoot_t overshoot)
{
    //Just pass on the values
    return _motor_move_steps(direction, distance, speed, overshoot, MOTOR_MOVE_TYPE_NORMAL);
}

motorReturnValue_t motor_move_degrees_float(float distance, uint16_t speed, motorOvershoot_t overshoot)
{
    motorDirection_t direction;
    uint32_t distance_in_steps;
    
    //Get direction
    if(distance<0.0)
    {
        direction = MOTOR_DIRECTION_CCW;
    }
    else
    {
        direction = MOTOR_DIRECTION_CW;
    }
    
    //Get distance in steps
    distance_in_steps = _motor_steps_from_degrees_float(distance);
    
    //Pass on the calculated values
    return _motor_move_steps(direction, distance_in_steps, speed, overshoot, MOTOR_MOVE_TYPE_NORMAL);
}

motorReturnValue_t motor_move_degrees_int(motorDirection_t direction, uint16_t distance, uint16_t speed, motorOvershoot_t overshoot)
{
    //Get distance in steps
    uint32_t distance_in_steps = _motor_steps_from_degrees_int(distance); 
    
    //Pass on the calculated values
    return _motor_move_steps(direction, distance_in_steps, speed, overshoot, MOTOR_MOVE_TYPE_NORMAL);
}

motorReturnValue_t motor_goto_steps(motorDirection_t direction, uint32_t position, uint16_t speed, motorOvershoot_t overshoot)
{
    uint32_t distance_in_steps;
    
    if(position>=config.full_circle_in_steps)
    {
        //Not a valid position, indicate an error
        return MOTOR_RETURN_VALUE_INVALID_POSITION;
    }
        
    //Get direction if necessary
    if(direction==MOTOR_DIRECTION_SHORTEST)
    {
        direction = _motor_decide_direction(position, overshoot);
    }
    
    //Get distance in the given direction
    distance_in_steps = _motor_calculate_distance(direction, position);
    
    //Pass on the calculated values
    return _motor_move_steps(direction, distance_in_steps, speed, overshoot, MOTOR_MOVE_TYPE_NORMAL);
}

motorReturnValue_t motor_goto_degrees_float(motorDirection_t direction, float position, uint16_t speed, motorOvershoot_t overshoot)
{
    uint32_t target_position_in_steps;
    uint32_t distance_in_steps;
    
    if(position<0.0 || position>=360.0)
    {
        //Not a valid position, indicate an error
        return MOTOR_RETURN_VALUE_INVALID_POSITION;
    }
    
    //Get target position in steps
    target_position_in_steps = _motor_steps_from_degrees_float(position);
        
    //Get direction if necessary
    if(direction==MOTOR_DIRECTION_SHORTEST)
    {
        direction = _motor_decide_direction(target_position_in_steps, overshoot);
    }
    
    //Get distance in the given direction
    distance_in_steps = _motor_calculate_distance(direction, target_position_in_steps);
    
    //Pass on the calculated values
    return _motor_move_steps(direction, distance_in_steps, speed, overshoot, MOTOR_MOVE_TYPE_NORMAL);
}

motorReturnValue_t motor_goto_degrees_int(motorDirection_t direction, uint16_t position, uint16_t speed, motorOvershoot_t overshoot)
{
    uint32_t target_position_in_steps;
    uint32_t distance_in_steps;
    
    if(position>35999)
    {
        //Not a valid position, indicate an error
        return MOTOR_RETURN_VALUE_INVALID_POSITION;
    }
    
    //Get target position in steps
    target_position_in_steps = _motor_steps_from_degrees_int(position);
        
    //Get direction if necessary
    if(direction==MOTOR_DIRECTION_SHORTEST)
    {
        direction = _motor_decide_direction(target_position_in_steps, overshoot);
    }
    
    //Get distance in the given direction
    distance_in_steps = _motor_calculate_distance(direction, target_position_in_steps);
    
    //Pass on the calculated values
    return _motor_move_steps(direction, distance_in_steps, speed, overshoot, MOTOR_MOVE_TYPE_NORMAL);
}

motorReturnValue_t motor_move_endless(motorDirection_t direction, uint16_t speed)
{
    return _motor_move_steps(direction, 0xFFFFFFFF, speed, MOTOR_OVERSHOOT_NO_OVERSHOOT, MOTOR_MOVE_TYPE_ENDLESS);
}
