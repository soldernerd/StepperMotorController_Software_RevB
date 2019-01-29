/* 
 * File:   display.h
 * Author: Luke
 *
 * Created on 11. September 2016, 13:52
 */

#ifndef DISPLAY_H
#define	DISPLAY_H

//extern char lcd_content[2][16];

void display_init(void);
void display_prepare(void);
void display_update(void);
char display_get_character(uint8_t line, uint8_t position);

//uint8_t display_get_character(uint8_t line, uint8_t position);

#endif	/* DISPLAY_H */

