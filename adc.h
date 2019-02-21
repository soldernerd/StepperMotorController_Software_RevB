/* 
 * File:   adc.h
 * Author: luke
 *
 * Created on 21. Februar 2019, 23:18
 */

#ifndef ADC_H
#define	ADC_H

typedef enum
{
    TEMPERATURE_SOURCE_INTERNAL = 0,
    TEMPERATURE_SOURCE_EXTERNAL = 1
}temperatureSource_t;

void adc_init(void);
void adc_read_temperature(void);

#endif	/* ADC_H */

