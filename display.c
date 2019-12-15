
#include <xc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "display.h"
#include "i2c.h"
#include "os.h"
#include "hardware_config.h"
#include "application_config.h"

char display_content[4][20];

#define DISPLAY_CC_VERTICALBAR_ADDRESS 0x00
#define DISPLAY_CC_VERTICALBAR_BIT_PATTERN {0b00000100,0b00000100,0b00000100,0b00000100,0b00000100,0b00000100,0b00000100,0b00000100}
const uint8_t bit_pattern_verticalbar[8] = DISPLAY_CC_VERTICALBAR_BIT_PATTERN;

#define DISPLAY_CC_DEGREE_ADDRESS 0x01
#define DISPLAY_CC_DEGREE_BIT_PATTERN {0b00011000, 0b00011000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000}
const uint8_t bit_pattern_degree[8] = DISPLAY_CC_DEGREE_BIT_PATTERN;

#define DISPLAY_CC_ae_ADDRESS 0x02
#define DISPLAY_CC_ae_BIT_PATTERN {0b00001010, 0b00000000, 0b00001110, 0b00000001, 0b00001111, 0b00010001, 0b00001111, 0b00000000}
const uint8_t bit_pattern_ae[8] = DISPLAY_CC_ae_BIT_PATTERN;

#define DISPLAY_STARTUP_0 {'*',' ',' ','S','t','e','p','p','e','r',' ','M','o','t','o','r',' ',' ',' ','*'}
#define DISPLAY_STARTUP_1 {'*',' ',' ',' ',' ','C','o','n','t','r','o','l','l','e','r',' ',' ',' ',' ','*'}
#define DISPLAY_STARTUP_2 {'*',' ',' ',' ',' ','v',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','*'}
#define DISPLAY_STARTUP_3 {'*',' ',' ','s','o','l','d','e','r','n','e','r','d','.','c','o','m',' ',' ','*'}
char dc_startup[4][20] = {DISPLAY_STARTUP_0, DISPLAY_STARTUP_1, DISPLAY_STARTUP_2, DISPLAY_STARTUP_3};
#define DISPLAY_MAIN_0 {'M','a','i','n',' ','M','e','n','u',':',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}
#define DISPLAY_MAIN_1 {' ','S','e','t','u','p',' ',' ',' ',' ','D','i','v','i','d','e',' ',' ',' ',' '}
#define DISPLAY_MAIN_2 {' ','A','r','c',' ',' ',' ',' ',' ',' ','M','a','n','u','a','l',' ',' ',' ',' '}
#define DISPLAY_MAIN_3 {' ','G','o','2','Z','e','r','o',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}
const char dc_main[4][20] = {DISPLAY_MAIN_0, DISPLAY_MAIN_1, DISPLAY_MAIN_2, DISPLAY_MAIN_3};
#define DISPLAY_SETUP1_0 {'S','e','t','u','p',':',' ','S','e','t',' ','z','e','r','o',' ','p','o','s','.'}
#define DISPLAY_SETUP1_1 {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}
#define DISPLAY_SETUP1_2 {'S','t','e','p',' ','s','i','z','e',' ',DISPLAY_CC_VERTICALBAR_ADDRESS,' ',' ','C','o','n','f','i','r','m'}
#define DISPLAY_SETUP1_3 {' ','x','.','x','x',DISPLAY_CC_DEGREE_ADDRESS,' ',' ',' ',' ',DISPLAY_CC_VERTICALBAR_ADDRESS,' ',' ','C','a','n','c','e','l',' '}
const char dc_setup1[4][20] = {DISPLAY_SETUP1_0, DISPLAY_SETUP1_1, DISPLAY_SETUP1_2, DISPLAY_SETUP1_3};
#define DISPLAY_SETUP2_0 {'S','e','t','u','p',':',' ','S','e','t',' ','d','i','r','e','c','t','i','o','n'}
#define DISPLAY_SETUP2_1 {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}
#define DISPLAY_SETUP2_2 {' ','C','o','u','n','t','e','r','C','l','o','c','k','w','i','s','e',' ',' ',' '}
#define DISPLAY_SETUP2_3 {' ','C','l','o','c','k','w','i','s','e',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}
const char dc_setup2[4][20] = {DISPLAY_SETUP2_0, DISPLAY_SETUP2_1, DISPLAY_SETUP2_2, DISPLAY_SETUP2_3};
#define DISPLAY_DIVIDE1_0 {'D','i','v','i','d','e',':',' ','S','e','t',' ','d','i','v','i','s','i','o','n'}
#define DISPLAY_DIVIDE1_1 {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}
#define DISPLAY_DIVIDE1_2 {' ',' ',' ','/','s','t','e','p',' ',' ',DISPLAY_CC_VERTICALBAR_ADDRESS,' ',' ','C','o','n','f','i','r','m'}
#define DISPLAY_DIVIDE1_3 {'1','2','3','4',' ',' ',' ',' ',' ',' ',DISPLAY_CC_VERTICALBAR_ADDRESS,' ',' ','C','a','n','c','e','l',' '}
const char dc_divide1[4][20] = {DISPLAY_DIVIDE1_0, DISPLAY_DIVIDE1_1, DISPLAY_DIVIDE1_2, DISPLAY_DIVIDE1_3};
#define DISPLAY_DIVIDE2_0 {'D','i','v','i','d','e',':',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}
#define DISPLAY_DIVIDE2_1 {'P','o','s',':',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}
#define DISPLAY_DIVIDE2_2 {'J','u','m','p',' ','s','i','z','e',':',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}
#define DISPLAY_DIVIDE2_3 {'P','r','e','s','s','T','o','J','u','m','p',' ',DISPLAY_CC_VERTICALBAR_ADDRESS,' ','C','a','n','c','e','l'}
const char dc_divide2[4][20] = {DISPLAY_DIVIDE2_0, DISPLAY_DIVIDE2_1, DISPLAY_DIVIDE2_2, DISPLAY_DIVIDE2_3};
#define DISPLAY_ARC1_0 {'A','r','c',':',' ','S','e','t',' ','a','r','c',' ','s','i','z','e',' ',' ',' '}
#define DISPLAY_ARC1_1 {'A','r','c',' ','s','i','z','e',':',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}
#define DISPLAY_ARC1_2 {'S','t','e','p',' ','s','i','z','e',' ',DISPLAY_CC_VERTICALBAR_ADDRESS,' ',' ','C','o','n','f','i','r','m'}
#define DISPLAY_ARC1_3 {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',DISPLAY_CC_VERTICALBAR_ADDRESS,' ',' ','C','a','n','c','e','l',' '}
const char dc_arc1[4][20] = {DISPLAY_ARC1_0, DISPLAY_ARC1_1, DISPLAY_ARC1_2, DISPLAY_ARC1_3};
#define DISPLAY_ARC2_0 {'A','r','c',':',' ','S','i','z','e','=',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}
#define DISPLAY_ARC2_1 {'C','u','r','r','e','n','t',' ','p','o','s',':',' ',' ',' ',' ',' ',' ',' ',DISPLAY_CC_DEGREE_ADDRESS}
#define DISPLAY_ARC2_2 {'T','u','r','n',' ','C','C','W',' ',DISPLAY_CC_VERTICALBAR_ADDRESS,' ','S','p','e','e','d',' ',' ',' ',' '}
#define DISPLAY_ARC2_3 {'S','t','a','r','t',' ',' ',' ',' ',DISPLAY_CC_VERTICALBAR_ADDRESS,' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}
const char dc_arc2[4][20] = {DISPLAY_ARC2_0, DISPLAY_ARC2_1, DISPLAY_ARC2_2, DISPLAY_ARC2_3};
#define DISPLAY_ZERO_0 {'R','e','t','u','r','n',' ','t','o',' ','Z','e','r','o','?',' ',' ',' ',' ',' '}
#define DISPLAY_ZERO_1 {'C','u','r','r','e','n','t',' ','p','o','s',':',' ',' ',' ',' ',' ',' ',' ',' '}
#define DISPLAY_ZERO_2 {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}
#define DISPLAY_ZERO_3 {' ','Y','e','s',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','N','o',' ',' '}
const char dc_zero[4][20] = {DISPLAY_ZERO_0, DISPLAY_ZERO_1, DISPLAY_ZERO_2, DISPLAY_ZERO_3};
#define DISPLAY_MANUAL_0 {'M','a','n','u','a','l',' ','M','o','d','e',' ',' ',' ',' ',' ',' ',' ',' ',' '}
#define DISPLAY_MANUAL_1 {'C','u','r','r','e','n','t',' ','p','o','s',':',' ',' ',' ',' ',' ',' ',' ',DISPLAY_CC_DEGREE_ADDRESS}
#define DISPLAY_MANUAL_2 {'T','u','r','n',' ','C','C','W',' ',DISPLAY_CC_VERTICALBAR_ADDRESS,' ','S','p','e','e','d',' ',' ',' ',' '}
#define DISPLAY_MANUAL_3 {'S','t','a','r','t',' ',' ',' ',' ',DISPLAY_CC_VERTICALBAR_ADDRESS, ' ',' ',' ',' ',' ',' ',' ',' ',' ', ' '}
const char dc_manual[4][20] = {DISPLAY_MANUAL_0, DISPLAY_MANUAL_1, DISPLAY_MANUAL_2, DISPLAY_MANUAL_3};

static void _display_start(void);
static void _display_clear(void);
static void _display_padded_itoa(int16_t value, uint8_t length, char *text);
static void _display_signed_itoa(int16_t value, char *text);
static void _display_itoa(int16_t value, uint8_t decimals, char *text);
static void _display_itoa_long(int32_t value, uint8_t decimals, char *text);
static uint8_t _display_itoa_u16(uint16_t value,  char *text);

static void _display_clear(void)
{
    uint8_t row;
    uint8_t col;
    for(row=0;row<4;++row)
    {
        for(col=0;col<20;++col)
        {
            display_content[row][col] = ' ';
        }
    }
}

static void _display_padded_itoa(int16_t value, uint8_t length, char *text)
{
    uint8_t pos;
    uint8_t padding;
    
    uint8_t len;
    char tmp[10];
    itoa(tmp, value, 10);
    len = strlen(tmp);
    padding = 0;
    while((padding+len)<length)
    {
        text[padding] = ' ';
        ++padding;
    }
    for(pos=0; tmp[pos]; ++pos)
    {
        text[pos+padding] = tmp[pos];
    }
    text[pos+padding] = 0x00;
}

static void _display_signed_itoa(int16_t value, char *text)
{
    if(value<0)
    {
        value = -value;
        text[0] = '-';
    }
    else
    {
        text[0] = '+';
    }
    _display_padded_itoa(value, 0, &text[1]);
}

static void _display_itoa(int16_t value, uint8_t decimals, char *text)
{
    uint8_t pos;
    uint8_t len;
    int8_t missing;
    char tmp[10];
    itoa(tmp, value, 10);
    len = strlen(tmp);
    
    if(value<0) //negative values
    {
        missing = decimals + 2 - len;
        if(missing>0) //zero-padding needed
        {
            for(pos=decimals;pos!=0xFF;--pos)
            {
                if(pos>=missing) //there is a character to copy
                {
                    tmp[pos+1] = tmp[pos+1-missing];
                }
                else //there is no character
                {
                    tmp[pos+1] = '0';
                }
            }
            len = decimals + 2;
        }  
    }
    else
    {
        missing = decimals + 1 - len;
        if(missing>0) //zero-padding needed
        {
            for(pos=decimals;pos!=0xFF;--pos)
            {
                if(pos>=missing) //there is a character to copy
                {
                    tmp[pos] = tmp[pos-missing];
                }
                else //there is no character
                {
                    tmp[pos] = '0';
                }
            }
            len = decimals + 1;
        }       
    }
 
    decimals = len - decimals - 1;
    
    for(pos=0;pos<len;++pos)
    {
        text[pos] = tmp[pos];
        if(pos==decimals)
        {
            //Insert decimal point
            ++pos;
            text[pos] = '.';
            break;
        }
    }
    for(;pos<len;++pos)
    {
        text[pos+1] = tmp[pos];
    }
    text[pos+1] = 0;
}

static void _display_itoa_long(int32_t value, uint8_t decimals, char *text)
{
    int16_t short_value;
    int8_t last_digit;
    uint8_t length;
    
    short_value = (int16_t) value;
    if(short_value==value)
    {
        _display_itoa(short_value, decimals, text);
    }
    else
    {
        short_value = value / 10;
        _display_itoa(short_value, decimals-1, text);
        length = strlen(text);
        last_digit = value % 10;
        if(last_digit<0)
            last_digit = -last_digit;
        text[length] = last_digit + 0x30; //single digit to character
        text[length+1] = 0x00;
    }
}

static uint8_t _display_itoa_u16(uint16_t value,  char *text)
{
    itoa(text, value, 10);
    if(value>9999)
    {
        *(text+5) = ' ';
        return 5;
    }
    else if (value>999)
    {
        *(text+4) = ' ';
        return 4;
    }
    else if (value>99)
    {
        *(text+3) = ' ';
        return 3;
    }
    else if (value>9)
    {
        *(text+2) = ' ';
        return 2;
    }
    else
    {
        *(text+1) = ' ';
        return 1;
    }
}

void display_init(void)
{
    i2c_display_init();
    i2c_display_program_custom_character(DISPLAY_CC_VERTICALBAR_ADDRESS, bit_pattern_verticalbar); 
    i2c_display_program_custom_character(DISPLAY_CC_DEGREE_ADDRESS, bit_pattern_degree);
    i2c_display_program_custom_character(DISPLAY_CC_ae_ADDRESS, bit_pattern_ae); 
    //Show startup screen including firmware version
    _display_start();
}

static void _display_start(void)
{
    uint8_t cntr;
    memcpy(display_content, dc_startup, sizeof display_content);
    cntr = 6;
    cntr += _display_itoa_u16(FIRMWARE_VERSION_MAJOR, &display_content[2][cntr]);
    display_content[2][cntr++] = '.';
    cntr += _display_itoa_u16(FIRMWARE_VERSION_MINOR, &display_content[2][cntr]);
    display_content[2][cntr++] = '.';
    cntr += _display_itoa_u16(FIRMWARE_VERSION_FIX, &display_content[2][cntr]);
}

void display_prepare()
{
    uint8_t cntr;
    uint8_t space;
    char temp[10];
    
    switch(os.displayState & 0xF0)
    {
        
        case DISPLAY_STATE_MAIN:
            memcpy(display_content, dc_main, sizeof display_content);
            switch(os.displayState)
            {
                case DISPLAY_STATE_MAIN_SETUP:
                    display_content[1][0] = '>';
                    break;
                case DISPLAY_STATE_MAIN_DIVIDE:
                    display_content[1][9] = '>';
                    break;
                case DISPLAY_STATE_MAIN_ARC:
                    display_content[2][0] = '>';
                    break;
                case DISPLAY_STATE_MAIN_MANUAL:
                    display_content[2][9] = '>';
                    break;
                case DISPLAY_STATE_MAIN_ZERO:
                    display_content[3][0] = '>';
                    break;
            }
            break;
            
        case DISPLAY_STATE_SETUP1:    
            memcpy(display_content, dc_setup1, sizeof display_content);
            _display_itoa(os.setup_step_size, 2, temp);
            if(os.setup_step_size>999)
                space = 0;
            else
                space = 1;
            for(cntr=0; temp[cntr]; ++cntr)
            {
                display_content[3][space+cntr] = temp[cntr];
            }
            switch(os.displayState)
            {
                case DISPLAY_STATE_SETUP1_CONFIRM:
                    display_content[2][12] = '>';
                    break;
                case DISPLAY_STATE_SETUP1_CANCEL:
                    display_content[3][12] = '>';
                    break;
            }
            break;
            
        case DISPLAY_STATE_SETUP2:    
            memcpy(display_content, dc_setup2, sizeof display_content);
            switch(os.displayState)
            {
                case DISPLAY_STATE_SETUP2_CCW:
                    display_content[2][0] = '>';
                    break;
                case DISPLAY_STATE_SETUP2_CW:
                    display_content[3][0] = '>';
                    break;
            }
            break;
            
        case DISPLAY_STATE_DIVIDE1:    
            memcpy(display_content, dc_divide1, sizeof display_content);
            //Display divide step size
            _display_padded_itoa(os.divide_step_size, 3, temp);
            for(cntr=0; cntr<3; ++cntr)
            {
                display_content[2][cntr] = temp[cntr];
            }
            //display division
            _display_padded_itoa(os.division, 4, temp);
            for(cntr=0; cntr<4; ++cntr)
            {
                display_content[3][cntr] = temp[cntr];
            }
            switch(os.displayState)
            {
                case DISPLAY_STATE_DIVIDE1_CONFIRM:
                    display_content[2][12] = '>';
                    break;
                case DISPLAY_STATE_DIVIDE1_CANCEL:
                    display_content[3][12] = '>';
                    break;
            }
            break;
            
        case DISPLAY_STATE_DIVIDE2:    
            memcpy(display_content, dc_divide2, sizeof display_content);
            //Display division setting
            _display_padded_itoa(os.division, 0, temp);
            for(cntr=0; temp[cntr]; ++cntr)
            {
                display_content[0][cntr+8] = temp[cntr];
            }
            //Display direction
            display_content[0][cntr+8] = ',';
            if(os.approach_direction==MOTOR_DIRECTION_CCW)
            {
                display_content[0][cntr+10] = 'C';
                display_content[0][cntr+11] = 'C';
                display_content[0][cntr+12] = 'W';
            }
            else
            {
                display_content[0][cntr+10] = 'C';
                display_content[0][cntr+11] = 'W'; 
            }
            //Display current position (integer division position)
            _display_padded_itoa(os.divide_position, 0, temp);
            for(cntr=0; temp[cntr]; ++cntr)
            {
                display_content[1][cntr+5] = temp[cntr];
            }
            //Write current position (in degrees)
            display_content[1][cntr+6] = '(';
            space = cntr + 7;
            _display_itoa_long(os.current_position_in_degrees, 2, temp);
            for(cntr=0; temp[cntr]; ++cntr)
            {
                display_content[1][cntr+space] = temp[cntr];
            }
            display_content[1][cntr+space] = DISPLAY_CC_DEGREE_ADDRESS;
            display_content[1][cntr+space+1] = ')';
            //Display jump size
            _display_signed_itoa(os.divide_jump_size, temp);
            for(cntr=0; temp[cntr]; ++cntr)
            {
                display_content[2][cntr+11] = temp[cntr];
            }
            break;
            
        case DISPLAY_STATE_ARC1:    
            memcpy(display_content, dc_arc1, sizeof display_content);
            switch(os.displayState)
            {
                case DISPLAY_STATE_ARC1_CONFIRM:
                    display_content[2][12] = '>';
                    break;
                case DISPLAY_STATE_ARC1_CANCEL:
                    display_content[3][12] = '>';
                    break;
            }
            
            //Write arc size
            _display_itoa_long(os.arc_size, 2, temp);
            for(cntr=0; temp[cntr]; ++cntr)
            {
                display_content[1][10+cntr] = temp[cntr];
            }
            display_content[1][10+cntr] = DISPLAY_CC_DEGREE_ADDRESS;
            
            //Write arc step size
            _display_itoa(os.arc_step_size, 2, temp);
            if(os.arc_step_size>999)
                space = 0;
            else
                space = 1;
            for(cntr=0; temp[cntr]; ++cntr)
            {
                display_content[3][space+cntr] = temp[cntr];
            }
            display_content[3][space+cntr] = DISPLAY_CC_DEGREE_ADDRESS ;
            
            break;
            
        case DISPLAY_STATE_ARC2:    
            memcpy(display_content, dc_arc2, sizeof display_content);
            switch(os.displayState)
            {
                case DISPLAY_STATE_ARC2_CW:
                    display_content[2][6] = 'W';
                    display_content[2][7] = ' ';
                    break;
                case DISPLAY_STATE_ARC2_CANCEL:
                    memcpy(display_content[2], "        ", 8);
                    memcpy(display_content[3], "Cancel", 6);
                    break;
            }
            
            //Write arc size
            _display_itoa_long(os.arc_size, 2, temp);
            for(cntr=0; temp[cntr]; ++cntr)
            {
                display_content[0][10+cntr] = temp[cntr];
            }
            display_content[0][10+cntr] = DISPLAY_CC_DEGREE_ADDRESS;
            
            //Write current position
            _display_itoa_long(os.current_position_in_degrees, 2, temp);
            space = 6-strlen(temp);
            for(cntr=0; temp[cntr]; ++cntr)
            {
                display_content[1][13+space+cntr] = temp[cntr];
            }
            
            //Write speed
            _display_itoa(motor_speed_from_index(os.arc_speed), 2, temp);
            for(cntr=0; temp[cntr]; ++cntr)
            {
                display_content[3][11+cntr] = temp[cntr];
            }
            display_content[3][11+cntr] = DISPLAY_CC_DEGREE_ADDRESS;
            display_content[3][12+cntr] = '/';
            display_content[3][13+cntr] = 's';
//            space = 5-strlen(temp);
//            for(cntr=0; temp[cntr]; ++cntr)
//            {
//                display_content[3][10+space+cntr] = temp[cntr];
//            }
            break;
            
        case DISPLAY_STATE_ZERO:    
            memcpy(display_content, dc_zero, sizeof display_content);
            
            //Write current position
            _display_itoa_long(os.current_position_in_degrees, 2, temp);
            for(cntr=0; temp[cntr]; ++cntr)
            {
                display_content[1][13+cntr] = temp[cntr];
            }
            display_content[1][13+cntr] = DISPLAY_CC_DEGREE_ADDRESS;
            
            break;
            
        case DISPLAY_STATE_MANUAL:    
            memcpy(display_content, dc_manual, sizeof display_content);
            switch(os.displayState)
            {
                case DISPLAY_STATE_MANUAL_CW:
                    display_content[2][6] = 'W';
                    display_content[2][7] = ' ';
                    break;
                case DISPLAY_STATE_MANUAL_CANCEL:
                    memcpy(display_content[2], "        ", 8);
                    memcpy(display_content[3], "Cancel", 6);
                    break;
                case DISPLAY_STATE_MANUAL_BUSY:
                    memcpy(&display_content[2][0], "        ", 8);
                    memcpy(&display_content[3][0], "Stop ", 5); 
                    break;
            }
            
            //Write current position
            _display_itoa_long(os.current_position_in_degrees, 2, temp);
            space = 7-strlen(temp);
            for(cntr=0; temp[cntr]; ++cntr)
            {
                display_content[1][12+space+cntr] = temp[cntr];
            }
                     
            //Write speed
            _display_itoa(motor_speed_from_index(os.manual_speed), 2, temp);
            for(cntr=0; temp[cntr]; ++cntr)
            {
                display_content[3][11+cntr] = temp[cntr];
            }
            display_content[3][11+cntr] = DISPLAY_CC_DEGREE_ADDRESS;
            display_content[3][12+cntr] = '/';
            display_content[3][13+cntr] = 's';
            break;
            
        case DISPLAY_STATE_ENCODER_TEST:
            _display_clear();
            _display_itoa((int16_t) (os.encoder1Count), 0, display_content[0]);
            if(ENCODER1_A_PIN)
                display_content[0][8] = 'H';
            else
                display_content[0][8] = 'L';
            if(ENCODER1_B_PIN)
                display_content[0][9] = 'H';
            else
                display_content[0][9] = 'L';
            _display_itoa((int16_t) (os.button1), 0, display_content[1]);
            if(ENCODER1_PB_PIN)
                display_content[1][8] = 'H';
            else
                display_content[1][8] = 'L';
            _display_itoa((int16_t) (os.encoder2Count), 0, display_content[2]);
            if(ENCODER2_A_PIN)
                display_content[2][8] = 'H';
            else
                display_content[2][8] = 'L';
            if(ENCODER2_B_PIN)
                display_content[2][9] = 'H';
            else
                display_content[2][9] = 'L';
            _display_itoa((int16_t) (os.button2), 0, display_content[3]);
            if(ENCODER2_PB_PIN)
                display_content[3][8] = 'H';
            else
                display_content[3][8] = 'L';
            break;
    } //switch
    
    if(!MOTOR_ERROR_PIN)
    {
        display_content[0][0] = 'E';
        display_content[0][1] = 'R';
        display_content[0][2] = 'R';
        display_content[0][3] = 'O';
        display_content[0][4] = 'R';
    }
    
//    //DEBUG
//    if(ENCODER1_PB_PIN)
//        display_content[0][17] = '+';
//    else
//        display_content[0][17] = '-';
//    
//    if(ENCODER1_A_PIN)
//        display_content[0][18] = '+';
//    else
//        display_content[0][18] = '-';
//    
//    if(ENCODER1_B_PIN)
//        display_content[0][19] = '+';
//    else
//        display_content[0][19] = '-';
//    
//    if(ENCODER2_PB_PIN)
//        display_content[1][17] = '+';
//    else
//        display_content[1][17] = '-';
//    
//    if(ENCODER2_A_PIN)
//        display_content[1][18] = '+';
//    else
//        display_content[1][18] = '-';
//    
//    if(ENCODER2_B_PIN)
//        display_content[1][19] = '+';
//    else
//        display_content[1][19] = '-';
}

void display_update(void)
{
    uint8_t line;
    for(line=0; line<4; ++line)
    {
        i2c_display_cursor(line, 0);
        i2c_display_write_fixed(display_content[line], 20);
    }
}

char display_get_character(uint8_t line, uint8_t position)
{
    return display_content[line][position];
}

