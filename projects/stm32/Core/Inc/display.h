#ifndef DISPLAY_H
#define DISPLAY_H
#include <stdint.h>
#include "measurement.h"
extern uint8_t oled_frame[1024];
void gfx_clear(void);
void gfx_pixel(int x,int y,int on);
void gfx_line(int x0,int y0,int x1,int y1);
void gfx_text(int x,int y,const char *text,int scale);
void gfx_wave(const Waveform *w);
int oled_init(uint8_t controller);
int oled_flush(void);
uint8_t oled_address(void);
#endif
