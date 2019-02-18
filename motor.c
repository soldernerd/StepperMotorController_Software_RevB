#include <stdint.h>
#include <xc.h>
//#include <pic18f26j50.h>
#include "motor.h"
#include "motor_config.h"
#include "hardware_config.h"
#include "os.h"

motorCommand_t motor_command_cue[MOTOR_COMMAND_CUE_SIZE];
uint8_t motor_cue_read_index; 
uint8_t motor_cue_write_index; 

motorMode_t motor_mode;
motorDirection_t motor_direction;
uint16_t motor_maximum_speed;

volatile uint16_t motor_current_speed;
volatile uint32_t motor_current_stepcount;
volatile uint32_t motor_final_stepcount;
volatile uint32_t motor_next_speed_check;

static void _motor_run(motorDirection_t direction, uint32_t distance_in_steps, uint16_t speed);

motorMode_t motor_get_mode(void)
{
    return motor_mode;
}

//Translate 0.01 degrees into steps
uint32_t motor_steps_from_degrees(uint16_t degrees)
{
    float steps;
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

uint8_t motor_schedule_command(motorDirection_t direction, uint32_t distance_in_steps, uint16_t speed)
{
    if((motor_items_in_cue()==0) && (os.busy==0))
    {
        //Cue is empty and motor is not busy
        //Run command directly
        _motor_run(direction, distance_in_steps, speed);
        //Indicate success
        return 1;
    }
    if(motor_items_in_cue()==MOTOR_COMMAND_CUE_SIZE-1)
    {
        //Buffer is full
        //Indicate an error
        return 0;
    }
    else
    {
        //Add element
        motor_command_cue[motor_cue_write_index].direction = direction;
        motor_command_cue[motor_cue_write_index].distance = distance_in_steps;
        motor_command_cue[motor_cue_write_index].speed = speed;
        //Increment write index
        ++motor_cue_write_index;
        //Indicate success
        return 1;
    }
}

void motor_process_cue(void)
{
    if(motor_items_in_cue()==0)
    {
        //Cue is empty, nothing to do
        return;
    }
    else if(os.busy)
    {
        //Motor is busy, maybe next time
        return;
    }
    else
    {
        //Run oldest command
        _motor_run(
            motor_command_cue[motor_cue_read_index].direction,
            motor_command_cue[motor_cue_read_index].distance,
            motor_command_cue[motor_cue_read_index].speed
        );
        //Inrement read index
        ++motor_cue_read_index;
    }
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

static void _motor_run(motorDirection_t direction, uint32_t distance_in_steps, uint16_t speed)
{
    //Save direction
    motor_direction = direction;
    
    //Calculate distance
    if(distance_in_steps==0)
    {
        //Essentially infinity. This will take a day to reach even at high speeds
        motor_final_stepcount = 0xFFFFFF00;
    }
    else
    {
        motor_final_stepcount = distance_in_steps;
        //motor_final_stepcount <<= FULL_STEP_SHIFT;
    }
    
    //Maximum speed
    if(speed==0)
    {
        motor_maximum_speed = config.maximum_speed;
    }
    else
    {
        motor_maximum_speed = speed;
    }
    
    //Initialize variables, calculate distances
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
        os.current_position_in_steps = 0;
    if(os.current_position_in_steps==0xFFFFFFFF)
        os.current_position_in_steps = (config.full_circle_in_steps-1);
    
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

void motor_isr(void)
{
    uint32_t steps_remaining;
    uint16_t steps_until_standstill;
    uint16_t steps_until_standstill_if_accelerate;
    
    //Clear interrupt flag
    PIR1bits.TMR2IF = 0;
    
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
        os.current_position_in_steps = 0;
    if(os.current_position_in_steps==0xFFFFFFFF)
        os.current_position_in_steps = (config.full_circle_in_steps-1);
    
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
    
    //Just to make sure we don't miss a step in case the interrupt flag has been set again in the mean time
    if(PIR1bits.TMR2IF)
    {
        //Clear interrupt flag
        PIR1bits.TMR2IF = 0;
        //Keep track of position
        ++motor_current_stepcount;
        
        //Calculate new position
        os.current_position_in_steps += motor_direction;
        if(os.current_position_in_steps==config.full_circle_in_steps)
            os.current_position_in_steps = 0;
        if(os.current_position_in_steps==0xFFFFFFFF)
            os.current_position_in_steps = (config.full_circle_in_steps-1);
    }
}

void motor_stop(void)
{
    motor_final_stepcount = motor_current_stepcount + motor_steps_lookup[motor_current_speed];
}

void motor_change_speed(uint16_t new_speed)
{
    motor_maximum_speed = new_speed;
}

void motor_go_to_steps_position(uint32_t target_position)
{
    uint32_t distance_cw;
    uint32_t overhead_cw;
    uint32_t distance_ccw;
    uint32_t overhead_ccw;
 
    //Don't try to do this while the motor is moving
    if(os.busy)
    {
        return;
    }
    
    //Nothing to do
    if(os.current_position_in_steps == target_position)
    {
        return;
    }
    
    //Calculate distance and overhead if traveled clockwise
    overhead_cw = 0;
    distance_cw = target_position - os.current_position_in_steps;
    if(distance_cw>config.full_circle_in_steps)
    {
        distance_cw += config.full_circle_in_steps;
    }
    if(os.approach_direction==MOTOR_DIRECTION_CCW)
    {
        distance_cw += config.overshoot_in_steps;
        overhead_cw += config.overshoot_in_steps;
        overhead_cw += config.overshoot_cost_in_steps;
    }
    
    //Calculate distance and overhead if traveled counter-clockwise
    overhead_ccw = 0;
    distance_ccw = os.current_position_in_steps - target_position;
    if(distance_ccw>config.full_circle_in_steps)
    {
        distance_ccw += config.full_circle_in_steps;
    }
    if(os.approach_direction==MOTOR_DIRECTION_CW)
    {
        distance_ccw += config.overshoot_in_steps;
        overhead_ccw += config.overshoot_in_steps;
        overhead_ccw += config.overshoot_cost_in_steps;
    }
    
    //Move the shorter of the two distances
    if((distance_cw+overhead_cw) < (distance_ccw+overhead_ccw))
    {
        if(os.approach_direction==MOTOR_DIRECTION_CW)
        {
            //It's shorter clockwise and this is our preferred direction
            motor_schedule_command(MOTOR_DIRECTION_CW, distance_cw, 0);
        }
        else
        {
            //It's shorter clockwise but we need to overshoot and return
            motor_schedule_command(MOTOR_DIRECTION_CW, distance_cw, 0);
            motor_schedule_command(MOTOR_DIRECTION_CCW, config.overshoot_in_steps, 0);
        }
    }
    else
    {
        if(os.approach_direction==MOTOR_DIRECTION_CCW)
        {
            //It's shorter counter-clockwise and this is our preferred direction
            motor_schedule_command(MOTOR_DIRECTION_CCW, distance_ccw, 0);
        }
        else
        {
            //It's shorter clockwise but we need to overshoot and return
            motor_schedule_command(MOTOR_DIRECTION_CCW, distance_ccw, 0);
            motor_schedule_command(MOTOR_DIRECTION_CW, config.overshoot_in_steps, 0);
        }
    }
}

void motor_go_to_degrees_position(float target_position)
{
    target_position *= (float) config.full_circle_in_steps;
    target_position /= 360.0;
    motor_go_to_steps_position((uint32_t) target_position);
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
    
    //Move motor and save position
    os.divide_position = target_divide_position;
    motor_go_to_steps_position(target_position_in_steps);
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
    
    //Move motor and save position
    os.divide_position = nearest_divide_position;
    motor_go_to_steps_position(nearest_position_in_steps);
}

void motor_arc_move(motorDirection_t direction)
{
    float arc_in_steps;
    
    arc_in_steps = (float) os.arc_size;
    arc_in_steps *= (float) config.full_circle_in_steps;
    arc_in_steps /= (float) 36000;
    motor_schedule_command(direction, (uint32_t ) arc_in_steps, os.arc_speed);
}