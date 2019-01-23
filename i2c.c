#include <xc.h>
#include "hardware_config.h"
#include "i2c.h"
#include "os.h"

#define _XTAL_FREQ 8000000

#define I2C_WRITE 0x00
#define I2C_READ 0x01

#define I2C_ADC_SLAVE_ADDRESS 0b11010000
#define I2C_EEPROM_SLAVE_ADDRESS 0b10100000

//eeprom_write_task_t task_list[16];
//uint8_t task_list_read_index = 0;
//uint8_t task_list_write_index = 0;

/* ****************************************************************************
 * General I2C functionality
 * ****************************************************************************/

//Replacements for the PLIB functions

void i2c_init(void)
{
    I2C_SDA_TRIS = PIN_INPUT;
    I2C_SCL_TRIS = PIN_INPUT;
    SSP1STATbits.SMP = 0; //Enable slew rate control
    SSP1STATbits.CKE = 0; //Disable SMBus inputs
    //SSP1ADD = 29; //400kHz at 48MHz system clock
    SSP1ADD = 119; //100kHz at 48MHz system clock
    //SSP1ADD = 239; //50kHz at 48MHz system clock
    SSP1CON1bits.WCOL = 0; //Clear write colision bit
    SSP1CON1bits.SSPOV = 0; //Clear receive overflow bit bit
    SSP1CON1bits.SSPM = 0b1000; //I2C master mode
    SSP1CON1bits.SSPEN = 1; //Enable module
}

static void _i2c_wait_idle(void)
{
    while(SSP1CON2bits.ACKEN | SSP1CON2bits.RCEN1 | SSP1CON2bits.PEN | SSP1CON2bits.RSEN | SSP1CON2bits.SEN | SSP1STATbits.R_W ){}
}

static void _i2c_start(void)
{
    SSP1CON2bits.SEN=1;
    while(SSP1CON2bits.SEN){}
}

static void _i2c_send(uint8_t dat)
{
    SSP1BUF = dat;
}

static uint8_t _i2c_get(void)
{
    SSP1CON2bits.RCEN = 1 ; //initiate I2C read
    while(SSP1CON2bits.RCEN){} //wait for read to complete
    return SSP1BUF; //return the value in the buffer
}

static void _i2c_stop(void)
{
    SSP1CON2bits.PEN = 1;
    while(SSP1CON2bits.PEN){}
}

static void _i2c_acknowledge(void)
{
    SSP1CON2bits.ACKDT = 0;
    SSP1CON2bits.ACKEN = 1;
    while(SSP1CON2bits.ACKEN){}
}

static void _i2c_not_acknowledge(void)
{
    SSP1CON2bits.ACKDT = 1;
    SSP1CON2bits.ACKEN = 1;
    while(SSP1CON2bits.ACKEN){}
}


static void _i2c_write(uint8_t slave_address, uint8_t *data, uint8_t length)
{
    uint8_t cntr;

    _i2c_wait_idle();
    _i2c_start();
    _i2c_wait_idle();
    _i2c_send(slave_address);
    _i2c_wait_idle();
    
    for(cntr=0; cntr<length; ++cntr)
    {
        _i2c_send(data[cntr]);
        _i2c_wait_idle();      
    } 
    
    _i2c_stop();    
}

static void _i2c_read(uint8_t slave_address, uint8_t *data, uint8_t length)
{
    uint8_t cntr;

    _i2c_wait_idle();
    _i2c_start();
    _i2c_wait_idle();
    _i2c_send(slave_address | I2C_READ);
    _i2c_wait_idle();
    /*
    for(cntr=0; cntr<length; ++cntr)
    {
        data[cntr] = _i2c_get();
        _i2c_acknowledge();       
    } 
    _i2c_not_acknowledge();
    */
    
    for(cntr=0; cntr<length-1; ++cntr)
    {
        data[cntr] = _i2c_get();
        _i2c_acknowledge();       
    } 
    data[cntr] = _i2c_get();
    _i2c_not_acknowledge();
     
    _i2c_stop();
}


/* ****************************************************************************
 * I2C ADC Functionality
 * ****************************************************************************/


void i2c_adc_start(i2cAdcResolution_t resolution, i2cAdcGain_t gain)
 {
     uint8_t configuration_byte;
     configuration_byte = 0b10000000;
     configuration_byte |= (resolution<<2);
     configuration_byte |= gain;
     
     _i2c_write(I2C_ADC_SLAVE_ADDRESS, &configuration_byte, 1);
 }
 
 int16_t i2c_adc_read(void)
 {
    int16_t result;
    _i2c_wait_idle();
    _i2c_start();
    _i2c_wait_idle();
    _i2c_send(I2C_ADC_SLAVE_ADDRESS | I2C_READ);
    _i2c_wait_idle();
    result = _i2c_get();
    result <<= 8;
    _i2c_acknowledge();
    result |= _i2c_get();
    _i2c_not_acknowledge();
    _i2c_stop(); 
    return result;
 };
 
/* ****************************************************************************
 * I2C EEPROM Functionality
 * ****************************************************************************/
 
static uint8_t _i2c_eeprom_busy(void)
{
    uint8_t busy;
    _i2c_wait_idle();
    _i2c_start();
    _i2c_wait_idle();
    _i2c_send(I2C_EEPROM_SLAVE_ADDRESS);
    _i2c_wait_idle();
    //ACKSTAT: Acknowledge Status bit (Master Transmit mode only)
    //1 = Acknowledge was not received from slave
    //0 = Acknowledge was received from slave
    busy = SSP1CON2bits.ACKSTAT;
    _i2c_stop(); 
    
    return 0;
}
 
//void _i2c_eeprom_load_default_calibration(calibration_t *buffer, calibrationIndex_t index);
 
void i2c_eeprom_writeByte(uint16_t address, uint8_t data)
{
    uint8_t slave_address;
    uint8_t dat[2];
    
    //Wait for device to be available
    while(_i2c_eeprom_busy());
    
    slave_address = I2C_EEPROM_SLAVE_ADDRESS | ((address&0b0000011100000000)>>7);
    dat[0] = address & 0xFF;
    dat[1] = data;
    
    _i2c_write(slave_address, &dat[0], 2);
}

uint8_t i2c_eeprom_readByte(uint16_t address)
{
    uint8_t slave_address;
    uint8_t addr;
    slave_address = I2C_EEPROM_SLAVE_ADDRESS | ((address&0b0000011100000000)>>7);
    addr = address & 0xFF;
    
    //Wait for device to be available
    while(_i2c_eeprom_busy());
    
    _i2c_write(slave_address, &addr, 1);
    _i2c_read(slave_address, &addr, 1);
    return addr;
}


void i2c_eeprom_write(uint16_t address, uint8_t *data, uint8_t length)
{
    uint8_t cntr;
    uint8_t slave_address;
    uint8_t dat[17];

    slave_address = I2C_EEPROM_SLAVE_ADDRESS | ((address&0b0000011100000000)>>7);
    dat[0] = address & 0xFF;

    length &= 0b00001111;
    for(cntr=0; cntr<length; ++cntr)
    {
        dat[cntr+1] = data[cntr];
    }
    
    //Wait for device to be available
    while(_i2c_eeprom_busy());
    
    _i2c_write(slave_address, &dat[0], length+1);
}

void i2c_eeprom_read(uint16_t address, uint8_t *data, uint8_t length)
{
    uint8_t slave_address;
    uint8_t addr;
    addr = address & 0xFF;
    address &= 0b0000011100000000;
    address >>= 7;
    slave_address = I2C_EEPROM_SLAVE_ADDRESS | address;
    
    //Wait for device to be available
    while(_i2c_eeprom_busy());
    
    _i2c_write(slave_address, &addr, 1);
    _i2c_read(slave_address, &data[0], length);
}



/* ****************************************************************************
 * I2C Display Functionality
 * ****************************************************************************/

#define I2C_DISPLAY_SLAVE_ADDRESS 0b01111000
#define DISPLAY_INSTRUCTION_REGISTER 0b00000000
#define DISPLAY_DATA_REGISTER 0b01000000
#define DISPLAY_CONTINUE_FLAG 0b10000000
#define DISPLAY_CLEAR 0b00000001
#define DISPLAY_SET_DDRAM_ADDRESS 0b10000000
#define DISPLAY_SET_CGRAM_ADDRESS 0b01000000
#define DISPLAY_FUNC_SET_TBL0 0b00111000

static void _i2c_display_send_init_sequence(void);
static void _ic2_display_set_ddram_address(uint8_t address);
static void _ic2_display_set_cgram_address(uint8_t address);

static void _i2c_display_send_init_sequence(void)
{
    uint8_t init_sequence[9] = {
        0x3A, // 8bit, 4line, RE=1 
        //0x1E, // Set BS1 (1/6 bias) 
        0b00011110, //0b00011110,
        0x39, // 8bit, 4line, IS=1, RE=0 
        //0x1C, // Set BS0 (1/6 bias) + osc 
        0b00001100, //0b00011100
        0x74, //0b1110000, // Set contrast lower 4 bits 
        0b1011100, //Set ION, BON, contrast bits 4&5 
        0x6d, // Set DON and amp ratio
        0x0c, // Set display on
        0x01  // Clear display
    };
    
    //Set I2C frequency to 200kHz (display doesn't like 400kHz at startup)
    //i2c_set_frequency(I2C_FREQUENCY_100kHz);

    //Write initialization sequence
    _i2c_write(I2C_DISPLAY_SLAVE_ADDRESS, &init_sequence[0], 9);
}

void i2c_display_init(void)
{
    uint8_t init_sequence[9] = {
        0x3A, // 8bit, 4line, RE=1
        //0x1E, // Set BS1 (1/6 bias)
        0b00011110, //0b00011110,
        0x39, // 8bit, 4line, IS=1, RE=0
        //0x1C, // Set BS0 (1/6 bias) + osc
        0b00001100, //0b00011100
        0x74, //0b1110000, // Set contrast lower 4 bits
        0b1011100, // Set ION, BON, contrast bits 4&5
        0x6d, // Set DON and amp ratio
        0x0c, // Set display on
        0x01  // Clear display
    };
    
    //Reset display
    DISP_RESET_PIN = 0;
    __delay_ms(10);
    DISP_RESET_PIN = 1;
    __delay_ms(50);
    
    //Set I2C frequency to 200kHz (display doesn't like 400kHz at startup)
    //i2c_set_frequency(I2C_FREQUENCY_100kHz);

    //Write initialization sequence
    _i2c_write(I2C_DISPLAY_SLAVE_ADDRESS, &init_sequence[0], 9);
}

static void _ic2_display_set_ddram_address(uint8_t address)
{
    uint8_t data_array[2];
    data_array[0] = DISPLAY_INSTRUCTION_REGISTER;
    data_array[1] = DISPLAY_SET_DDRAM_ADDRESS | address;
    
    //Set I2C frequency to 400kHz
    //i2c_set_frequency(I2C_FREQUENCY_100kHz);
    
    //Set address
    _i2c_write(I2C_DISPLAY_SLAVE_ADDRESS, &data_array[0], 2);
}

static void _ic2_display_set_cgram_address(uint8_t address)
{
    uint8_t data_array[3];
    
    //Check address and set GCRAM address
    //Byte 2 is not mentioned anywhere in the data sheet but is absolutely required
    //Thank you, MIDAS
    address &= 0b001111111;
    data_array[0] = DISPLAY_INSTRUCTION_REGISTER;
    data_array[1] = DISPLAY_FUNC_SET_TBL0;
    data_array[2] = DISPLAY_SET_CGRAM_ADDRESS | address;
    
    //Send data over I2C
    _i2c_write(I2C_DISPLAY_SLAVE_ADDRESS, data_array, 3);
}

void i2c_display_cursor(uint8_t line, uint8_t position)
{
    uint8_t address;
    
    //Figure out display address
    line &= 0b11;
    address = line<<5;
    position &= 0b11111;
    address |= position;
    
    //Set I2C frequency to 400kHz
    //i2c_set_frequency(I2C_FREQUENCY_100kHz);
    
    //Set address
    _ic2_display_set_ddram_address(address);
}

void i2c_display_write(char *data)
{
    //Set I2C frequency to 400kHz
    //i2c_set_frequency(I2C_FREQUENCY_100kHz);

    _i2c_wait_idle();
    _i2c_start();
    _i2c_wait_idle();
    _i2c_send(I2C_DISPLAY_SLAVE_ADDRESS);
    _i2c_wait_idle();
    _i2c_send(DISPLAY_DATA_REGISTER);
    _i2c_wait_idle();
    
    //Print entire (zero terminated) string
    while(*data)
    {
        _i2c_send(*data++);
        _i2c_wait_idle();        
    } 
    
    _i2c_stop();
}

void i2c_display_write_fixed(char *data, uint8_t length)
{
    uint8_t pos;
    
    //Set I2C frequency to 400kHz
    //i2c_set_frequency(I2C_FREQUENCY_100kHz);

    _i2c_wait_idle();
    _i2c_start();
    _i2c_wait_idle();
    _i2c_send(I2C_DISPLAY_SLAVE_ADDRESS);
    _i2c_wait_idle();
    _i2c_send(DISPLAY_DATA_REGISTER);
    _i2c_wait_idle();
    
    //Print entire (zero terminated) string
    for(pos=0; pos<length; ++pos)
    {
        _i2c_send(data[pos]);
        _i2c_wait_idle();        
    } 
    
    _i2c_stop();    
}

void i2c_display_program_custom_character(uint8_t address, uint8_t *bit_pattern)
{
    uint8_t cntr;
    uint8_t data_array[11];
    
    //Set GC Ram address
    _ic2_display_set_cgram_address(address<<3);
        
    //Prepare actual data to be sent
    data_array[0] = DISPLAY_DATA_REGISTER;
    for(cntr=0; cntr<8; ++cntr)
    {
        data_array[cntr+1] = bit_pattern[cntr] & 0b00011111;
    }
    _i2c_write(I2C_DISPLAY_SLAVE_ADDRESS, data_array, 9);  
}

