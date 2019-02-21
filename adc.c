#include <xc.h>
#include <stdint.h>
#include "hardware_config.h"
#include "os.h"
#include "adc.h"

#define FAN_ON_THRESHOLD 2500
#define FAN_OFF_THRESHOLD 2400

typedef struct
{
    temperatureSource_t temperature_source;
    uint16_t temperature_adc_sum[2];
    uint8_t temperature_count[2];
} adcParameters_t;

adcParameters_t params;

void adc_init(void)
{
    //Initialize variables
    os.temperature[TEMPERATURE_SOURCE_INTERNAL] = 0;
    os.temperature[TEMPERATURE_SOURCE_EXTERNAL] = 0;
    os.fan_on = 0;
    params.temperature_source = TEMPERATURE_SOURCE_INTERNAL;
    params.temperature_adc_sum[TEMPERATURE_SOURCE_INTERNAL] = 0;
    params.temperature_count[TEMPERATURE_SOURCE_INTERNAL] = 0;
    params.temperature_adc_sum[TEMPERATURE_SOURCE_EXTERNAL] = 0;
    params.temperature_count[TEMPERATURE_SOURCE_EXTERNAL] = 0;
    
    ADCON0bits.VCFG1 = 0; //Ground as negative reference
    ADCON0bits.VCFG0 = 0; //Supply voltage as positive reference
    ADCON0bits.CHS = TEMPERATURE_INTERNAL_CHANNEL; //Select channel
    
    ADCON1bits.ADFM = 1; //Right justified
    ADCON1bits.ADCAL = 0; //Normal mode, no calibration
    ADCON1bits.ACQT = 0b111; //20Tad acquisition time
    ADCON1bits.ADCS = 0b110; //FOSC / 64
    
    ADCON0bits.ADON = 1; //Enable ADC module
    ADCON0bits.GO = 1; //Start a conversion
}

void adc_read_temperature(void)
{
    uint16_t adc_result;
    float temperature;
    
    //Get ADC reading
    adc_result = ADRESH;
    adc_result <<= 8;
    adc_result |= ADRESL;
    
    //Add result to ADC sum and increment count
    params.temperature_adc_sum[params.temperature_source] += adc_result;
    ++params.temperature_count[params.temperature_source];
    
    //Start a new conversion
    ADCON0bits.GO = 1; //Start a conversion
    
    //Calculate temperature
    if(params.temperature_count[params.temperature_source]==8)
    {
        //adc_result = 2633 - adc_result;
        //temperature = 5.924 * (float) adc_result;
        
        temperature = 21064.0 - (float) params.temperature_adc_sum[params.temperature_source];
        temperature *= 0.7405;
        
        //Save result
        os.temperature[params.temperature_source] = (int16_t) temperature;
        
        //Reset count and sum
        params.temperature_adc_sum[params.temperature_source] = 0;
        params.temperature_count[params.temperature_source] = 0;
        
        //Take care of fan
        if(os.fan_on)
        {
            if((os.temperature[TEMPERATURE_SOURCE_INTERNAL]<FAN_OFF_THRESHOLD) && (os.temperature[TEMPERATURE_SOURCE_EXTERNAL]<FAN_OFF_THRESHOLD))
            {
                FAN_ENABLE_PIN = 0;
                os.fan_on = 0;
            }
        }
        else
        {
            if((os.temperature[TEMPERATURE_SOURCE_INTERNAL]>FAN_ON_THRESHOLD) || (os.temperature[TEMPERATURE_SOURCE_EXTERNAL]>FAN_ON_THRESHOLD))
            {
                FAN_ENABLE_PIN = 1;
                os.fan_on = 1;
            }
        }
    }
    
    //Switch to other source
    if(params.temperature_source==TEMPERATURE_SOURCE_INTERNAL)
    {
        ADCON0bits.CHS = TEMPERATURE_EXTERNAL_CHANNEL; //Select channel
        params.temperature_source = TEMPERATURE_SOURCE_EXTERNAL;
    }
    else
    {
        ADCON0bits.CHS = TEMPERATURE_INTERNAL_CHANNEL; //Select channel
        params.temperature_source = TEMPERATURE_SOURCE_INTERNAL;
    }
}

