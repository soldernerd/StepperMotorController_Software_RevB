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
#include "hardware_config.h"
#include "os.h"
#include "i2c.h"
#include "display.h"
#include "encoder.h"
#include "flash.h"
#include "fat16.h"
#include "motor.h"


static void _calculate_adc_sum(void);
static void _calculate_db_value(void);
static void _calculate_s_value(void);

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
    float tmp;
    
    //This is a user defined function
    system_init();
    
    SYSTEM_Initialize(SYSTEM_STATE_USB_START);

    USBDeviceInit();
    USBDeviceAttach();
    
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
            
            //Run periodic tasks
            switch(os.timeSlot)
            {     
                case 0:
                    os_read_temperature();
                    break;
                    
                case 1:
                    //BUZZER_ENABLE_PIN = 0; 
                    break;
                
                case 5:
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
