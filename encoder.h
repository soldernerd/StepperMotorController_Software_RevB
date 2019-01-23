/* 
 * File:   encoder.h
 * Author: Luke
 *
 * Created on 09. September 2017, 23:26
 */

#ifndef ENCODER_H
#define	ENCODER_H

void encoder_init(void);
void encoder_run(void);
void encoder_statemachine(void);

uint16_t encoder_next_setup_stepsize(uint16_t old_stepsize);
uint8_t encoder_next_divide_step_size(uint8_t old_stepsize);
uint16_t encoder_next_arc_step_size(uint16_t old_stepsize);

#endif	/* ENCODER_H */
