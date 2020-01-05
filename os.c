
#include <xc.h>
#include <stdint.h>
#include "hardware_config.h"
#include "os.h"
#include "i2c.h"
#include "encoder.h"
#include "flash.h"
#include "fat16.h"
#include "display.h"
#include "adc.h"
#include "motor.h"
#include "config_file.h"
#include "application_config.h"

#define MILLISECONDS_PER_TIMESLOTS 10
#define NUMBER_OF_TIMESLOTS 16

// 1ms
#define TIMER0_LOAD_HIGH_48MHZ 0xFA
#define TIMER0_LOAD_LOW_48MHZ 0x24

//void interrupt _isr(void)
void tmr0_isr(void)
{ 
    //Timer 0
    if(INTCONbits.T0IF)
    {
        //Re-load timer: 1ms until overflow
        TMR0H = TIMER0_LOAD_HIGH_48MHZ;
        TMR0L = TIMER0_LOAD_LOW_48MHZ;

        //Clear interrupt flag
        INTCONbits.T0IF = 0;
        
        //Take care of encoders
        encoder_run();
        
        //Take care of time slots
        ++os.subTimeSlot;
        if(os.subTimeSlot>=MILLISECONDS_PER_TIMESLOTS)
        {
            if(os.done) 
            {
                ++os.timeSlot;
                if(os.timeSlot==NUMBER_OF_TIMESLOTS)
                {
                    os.timeSlot = 0;
                }
                os.subTimeSlot = 0;
                os.done = 0;
            }
        }
    }
}



static void _system_pin_setup(void)
{
    //SPI Pins
    SPI_MISO_TRIS = PIN_INPUT;
    SPI_MOSI_TRIS = PIN_OUTPUT;
    SPI_SCLK_TRIS = PIN_OUTPUT;
    SPI_SS1_TRIS = PIN_OUTPUT;
    SPI_SS1_PIN = 1;

    TEMPERATURE_INTERNAL_TRIS = PIN_INPUT;
    TEMPERATURE_INTERNAL_ANCON = PIN_ANALOG;

    BUZZER_ENABLE_TRIS = PIN_OUTPUT;
    BUZZER_ENABLE_PIN = 0;

    FAN_ENABLE_TRIS = PIN_OUTPUT;
    FAN_ENABLE_PIN = 0;

    DISP_RESET_TRIS = PIN_OUTPUT;
    DISP_RESET_PIN = 0;

    DISP_BACKLIGHT_TRIS = PIN_OUTPUT;
    DISP_BACKLIGHT_PIN = 0;
//    PPSUnLock();
//    DISP_BACKLIGHT_PPS = PPS_FUNCTION_CCP2_OUTPUT;
//    PPSLock();

    I2C_SDA_TRIS = PIN_INPUT;
    I2C_SCL_TRIS = PIN_INPUT;

    MOTOR_ENABLE_TRIS = PIN_OUTPUT;
    MOTOR_ENABLE_PIN = 1;

    MOTOR_DIRECTION_TRIS = PIN_OUTPUT;
    MOTOR_DIRECTION_PIN = 0;

    MOTOR_STEP_TRIS = PIN_OUTPUT;
    MOTOR_STEP_ANCON = PIN_DIGITAL;
    MOTOR_STEP_PIN = 0;
    PPSUnLock();
    MOTOR_STEP_PPS = PPS_FUNCTION_CCP1_OUTPUT;
    PPSLock();

    MOTOR_ERROR_TRIS = PIN_INPUT;

    ENCODER1_A_TRIS = PIN_INPUT;
    ENCODER1_A_ANCON = PIN_DIGITAL;

    ENCODER1_B_TRIS = PIN_INPUT;
    ENCODER1_B_ANCON = PIN_DIGITAL;

    ENCODER1_PB_TRIS = PIN_INPUT;
    ENCODER1_PB_ANCON = PIN_DIGITAL;

    ENCODER2_A_TRIS = PIN_INPUT;
    ENCODER2_A_ANCON = PIN_DIGITAL;

    ENCODER2_B_TRIS = PIN_INPUT;
    ENCODER2_B_ANCON = PIN_DIGITAL;

    ENCODER2_PB_TRIS = PIN_INPUT;
    ENCODER2_PB_ANCON = PIN_DIGITAL;
}



static void _system_timer0_init(void)
{
    //Clock frequency after prescaler: 1.5 MHz
    
    //Clock source = Fosc/4
    T0CONbits.T0CS = 0;
    //Operate in 16bit mode
    T0CONbits.T08BIT = 0;
    //Prescaler=8
    T0CONbits.T0PS2 = 0;
    T0CONbits.T0PS1 = 1;
    T0CONbits.T0PS0 = 0;
    //Use prescaler
    T0CONbits.PSA = 0;
    //1ms until overflow
    TMR0H = TIMER0_LOAD_HIGH_48MHZ;
    TMR0L = TIMER0_LOAD_LOW_48MHZ;
    //Turn timer0 on
    T0CONbits.TMR0ON = 1;
            
    //Enable timer0 interrupts
    INTCONbits.TMR0IF = 0;
    INTCONbits.TMR0IE = 1;
    INTCONbits.GIE = 1;
    
    //Initialize timeSlot
    os.subTimeSlot = 0;
    os.timeSlot = 0;
    os.done = 0;
    
    
}

static void _backlight_init(void)
{
    DISP_BACKLIGHT_PIN = 1;
}


static void _init_buzzer(void)
{
    TRISAbits.TRISA1 = 0;
    PORTAbits.RA1 = 1;
    __delay_ms(200);
    PORTAbits.RA1 = 0;
    __delay_ms(300);
    PORTAbits.RA1 = 1;
    __delay_ms(200);
    PORTAbits.RA1 = 0;
}


void system_init(void)
{
    //Configure all pins as inputs/outputs analog/digital as needed
    _system_pin_setup();
    
    //Configure analog digital converter
    adc_init();
    
    //Initialize SPI / flash
    flash_init();
    
    //Initialize FAT16 file system on flash
    fat_init();
    
    //Initialize encoders
    encoder_init();
    
    //Set up I2C
    i2c_init();
    
    //Initialize display and show startup screen
    display_init();
    display_update();
    _backlight_init();
    
    //Configure timer2 and CCCP1 module to control stepper motor
    motor_init();
    
    //Populate config with default values
    configFile_readDefault();
    
    //Read config file
    configFile_read();
    
    //Write file with used config parameters
    configFile_write();
    
    //Initialize variables
    os.subTimeSlot = 0;
    os.timeSlot = 0;
    os.done = 0;
    os.encoder1Count = 0;
    os.button1 = 0;
    os.encoder2Count = 0;
    os.button2 = 0;
    os.current_position_in_steps = 0;
    os.absolute_position = 0;
    os.current_position_in_degrees = 0;
    os.displayState = DISPLAY_STATE_MAIN_SETUP;
    os.busy = 0;
    os.setup_step_size = 0;
    os.approach_direction = MOTOR_DIRECTION_CW;
    os.division = 36;
    os.divide_step_size = 10;
    os.divide_position = 0;
    os.divide_jump_size = 1;
    os.arc_step_size = 1000;
    os.arc_size = 1000;
    os.arc_speed = config.initial_speed_arc;
    os.arc_direction = MOTOR_DIRECTION_CW;
    os.manual_speed = config.initial_speed_manual;
    os.manual_direction = MOTOR_DIRECTION_CW;
    os.beep_count = 0;
    os.temperature[0] = 0;
    os.temperature[1] = 0;
    os.external_temperature_adc_sum = 0;
    os.external_temperature_count = 0;
    os.fan_on = 0;
    os.brake_on = 0;
    
    //Read back last position and some other settings from EEPROM
    i2c_eeprom_recover_position();
    if(os.current_position_in_steps>config.full_circle_in_steps)
    {
        os.current_position_in_steps = 0;
        os.absolute_position = 0;
    }
    motor_calculate_position_in_degrees();
    
    //Set up timer0 for timeSlots
    _system_timer0_init();
}

void reboot(void)
{
    //Just wait 2 seconds until the WDT resets the device
    while(1);
}



