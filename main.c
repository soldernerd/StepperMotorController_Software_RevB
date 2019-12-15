/*
 * File:   main.c
 * Author: Luke
 *
 * Created on 26. December 2016, 21:31
 */


/** INCLUDES *******************************************************/

#include "system.h"

#include "usb.h"
#include "usb_device_hid.h"
#include "usb_device_msd.h"
//
//#include "internal_flash.h"
//
#include "app_device_custom_hid.h"
#include "app_device_msd.h"

//User defined code
#include "application_config.h"
#include "hardware_config.h"
#include "os.h"
#include "i2c.h"
#include "display.h"
#include "encoder.h"
#include "flash.h"
#include "fat16.h"
#include "motor.h"
#include "adc.h"

/********************************************************************
 * Function:        void main(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Main program entry point.
 *
 * Note:            None
 *******************************************************************/
MAIN_RETURN main(void)
{
    uint8_t startup_timer;
    //float tmp;
    
    //This is a user defined function
    system_init();
    
    SYSTEM_Initialize(SYSTEM_STATE_USB_START);

    USBDeviceInit();
    USBDeviceAttach();
    
    //Startup delay
    startup_timer = STARTUP_DELAY;
    while(startup_timer)
    {
        if(!os.done)
        {
            --startup_timer;
            os.done = 1;
        }
    }
    
    while(1)
    {        
        //Do this as often as possible
        APP_DeviceMSDTasks();
        
        if(!os.done)
        {
            //Clear watchdog timer
            ClrWdt();
            
            //Do this every time
            APP_DeviceCustomHIDTasks();
            
            //Take care of state machine
            encoder_statemachine();

            //Run any pending motor commands
            motor_process_cue();
            
            //Read ADC result
            adc_read_temperature();
            
            //Run periodic tasks
            switch(os.timeSlot)
            {     
                case 0:
                    break;
                    
                case 1:
                    //BUZZER_ENABLE_PIN = 0; 
                    break;
                
                case 5:
                    //Calculate position in 0.01 degrees
                    motor_calculate_position_in_degrees();
                    
                case 6: 
                    display_prepare();
                    break;
                    
                case 7:
                    display_update();
                    break;
            }
            
            os.done = 1;
            
        } //if(!os.done)

    }//end while(1)
}//end main

/*******************************************************************************
 End of File
*/
