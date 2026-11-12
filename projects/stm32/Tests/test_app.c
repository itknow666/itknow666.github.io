/* Execute the production app/measurement code with deterministic peripheral mocks.
   GPIO/DMA/I2C/flash remain hardware acceptance items, not covered by these mocks. */
#include "app.h"
#include "board.h"
#include "display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static unsigned checks;
#define CHECK(c) do {checks++;if(!(c)){fprintf(stderr,"FAIL line %u: %s\n",(unsigned)__LINE__,#c);exit(1);}} while(0)
uint16_t adc_pairs[MEAS_SAMPLES*2];uint8_t oled_frame[1024];
static uint32_t ticks,events,active_fs;
static int ready,acquiring,error,save_ok=1,saves,canceled;
static float capture;
static unsigned normalized,controller;
static char serial[16384],input[256];static unsigned input_pos;
static char draw[2048],screen[2048];static Calibration flash;
uint32_t HAL_GetTick(void){return ticks;}
GPIO_PinState HAL_GPIO_ReadPin(int p,int n){(void)p;(void)n;return GPIO_PIN_SET;}
void HAL_GPIO_WritePin(int p,int n,GPIO_PinState v){(void)p;(void)n;(void)v;}
uint32_t board_keys(void){uint32_t v=events;events=0;return v;}
HAL_StatusTypeDef board_acquire(uint32_t fs){active_fs=fs;acquiring=1;ready=0;return HAL_OK;}
void board_acquire_stop(void){if(acquiring&&!ready)canceled++;acquiring=0;ready=0;error=0;}
int board_frame_ready(void){return ready;}
int board_acquisition_error(void){return error;}
void board_normalize_frame(void){normalized++;}
float board_frequency(void){return capture;}
void board_frequency_reset(void){capture=0;}
float board_vref_read(void){return 1490;}
int board_cal_load(Calibration *c){calibration_defaults(c);return 0;}
int board_cal_save(Calibration *c){saves++;board_acquire_stop();if(!save_ok)return 0;c->generation++;c->crc=calibration_crc(c);flash=*c;return 1;}
int board_uart_getchar(void){return input[input_pos]?(unsigned char)input[input_pos++]:-1;}
void board_uart_puts(const char *s){if(strlen(serial)+strlen(s)<sizeof(serial))strcat(serial,s);}
void gfx_clear(void){draw[0]=0;memset(oled_frame,0,sizeof(oled_frame));}
void gfx_text(int x,int y,const char *s,int scale){(void)x;(void)y;(void)scale;if(strlen(draw)+strlen(s)+2<sizeof(draw)){strcat(draw,s);strcat(draw,"\n");}}
void gfx_wave(const Waveform *w){CHECK(w->periods>=1.0f);strcat(draw,"WAVE_PIXELS\n");}
int oled_init(uint8_t c){controller=c;return 1;}
int oled_flush(void){strcpy(screen,draw);return 1;}
uint8_t oled_address(void){return 0x3c;}
static void advance(unsigned ms){ticks+=ms;app_poll();}
static void key(unsigned k){events=k;app_poll();}
static void cmd(const char *s){CHECK(strlen(s)+2<sizeof(input));snprintf(input,sizeof(input),"%s\n",s);input_pos=0;serial[0]=0;app_poll();input[0]=0;input_pos=0;}
static uint16_t code(float mv){float v=mv*MAIN_GAIN+MAIN_OFFSET_V*1000;int c=(int)lroundf(v*4095/3300);return (uint16_t)(c<0?0:(c>4095?4095:c));}
static void frame(float dc,float hz,float pp){
    CHECK(acquiring);capture=hz;
    for(unsigned i=0;i<MEAS_SAMPLES;i++){
        float s=sinf(6.2831853f*hz*(float)i/(float)active_fs);
        adc_pairs[2*i]=code(dc+0.5f*pp*s);
        float v=1650+0.5f*pp*small_gain_at(hz)*s;int c=(int)lroundf(v*4095/3300);
        adc_pairs[2*i+1]=(uint16_t)(c<0?0:(c>4095?4095:c));
    }
    ready=1;advance(90);
}
int main(void){
    app_init();CHECK(strstr(screen,"DC UNCAL"));CHECK(strstr(screen,"WAIT FOR NEW DATA"));
    advance(40);CHECK(active_fs==20000);frame(2500,0,0);CHECK(strstr(screen,"2.500V"));
    key(KEY_HOLD_SHORT);CHECK(strstr(screen,"HOLD"));char held[2048];strcpy(held,screen);
    frame(1000,0,0);CHECK(!strcmp(held,screen));
    unsigned old=(unsigned)canceled;key(KEY_MODE_SHORT);
    CHECK((unsigned)canceled>old);CHECK(strstr(screen,"AC UNCAL"));CHECK(strstr(screen,"WAIT FOR NEW DATA"));CHECK(!strstr(screen,"HOLD"));
    cmd("STATUS");CHECK(strstr(serial,"MODE=1 VALID=0 DC_MV=0 PP_MV=0"));
    advance(40);frame(0,1000,500);CHECK(strstr(screen,"1000.0HZ"));CHECK(strstr(screen,"0.500V"));
    key(KEY_MODE_SHORT);CHECK(strstr(screen,"WAIT FOR NEW DATA"));advance(40);CHECK(active_fs==400000);
    frame(0,10000,2000);CHECK(strstr(screen,"WAVE_PIXELS"));CHECK(normalized>=4);
    key(KEY_MODE_SHORT);advance(40);CHECK(active_fs==20000);frame(0,0,0);
    CHECK(strstr(screen,"DC UNCAL"));CHECK(!strstr(screen,"WAVE_PIXELS"));
    unsigned ns=(unsigned)saves;
    cmd("CAL DC nan");CHECK(strstr(serial,"ERROR"));cmd("CAL DC inf");CHECK(strstr(serial,"ERROR"));
    cmd("CAL DC    ");CHECK(strstr(serial,"ERROR"));CHECK((unsigned)saves==ns);
    cmd("CAL DC 0");CHECK(strstr(serial,"POINT ADDED RAM"));CHECK((unsigned)saves==ns);
    frame(5000,0,0);cmd("CAL DC 5000");CHECK(strstr(serial,"POINT ADDED RAM"));
    cmd("STATUS");CHECK(strstr(serial,"DC_POINTS=2"));cmd("SAVE");CHECK(strstr(serial,"SAVED TO FLASH"));CHECK(calibration_valid(&flash));
    CHECK(flash.dc_count==2);CHECK(saves==(int)ns+1);advance(40);frame(2500,0,0);CHECK(strstr(screen,"DC CAL"));
    /* Wizard abort restores previous calibration without a flash write. */
    ns=(unsigned)saves;key(KEY_MODE_LONG);CHECK(strstr(screen,"DC CALIBRATION"));
    key(KEY_MODE_LONG);cmd("STATUS");CHECK(strstr(serial,"DC_POINTS=2"));CHECK((unsigned)saves==ns);
    advance(40);frame(2500,0,0);cmd("SAVE");CHECK(saves==(int)ns+1);
    save_ok=0;cmd("SAVE");CHECK(strstr(serial,"ERROR FLASH SAVE"));save_ok=1;
    cmd("OLED SH1106");CHECK(controller==1);cmd("OLED SSD1306");CHECK(controller==0);
    /* ADC timeout discards the frame and starts a fresh acquisition after settling. */
    advance(40);advance(130);CHECK(strstr(serial,"ADC TIMEOUT/ERROR"));
    CHECK(strstr(screen,"WAIT FOR NEW DATA"));advance(40);CHECK(acquiring);
    frame(1000,0,0);cmd("STATUS");const char *dcvalue=strstr(serial,"DC_MV=");CHECK(dcvalue);
    CHECK(abs(atoi(dcvalue+6)-1000)<10);
    cmd("MODE AC");advance(40);frame(0,1000,500);cmd("CAL AC 500");CHECK(strstr(serial,"TRIM ADDED RAM"));
    cmd("STATUS");CHECK(strstr(serial,"AC_MASK=2,2"));
    cmd("MODE DC");advance(40);frame(0,0,0);cmd("CAL DC 0");cmd("STATUS");CHECK(strstr(serial,"AC_MASK=0,0"));
    char longline[160];memset(longline,'A',sizeof(longline)-1);longline[sizeof(longline)-1]=0;
    cmd(longline);CHECK(strstr(serial,"LINE TOO LONG"));cmd("STATUS");CHECK(strstr(serial,"MODE=0"));
    /* Complete the on-device five-point wizard, including negative extension inputs. */
    ns=(unsigned)saves;key(KEY_MODE_LONG);advance(40);
    const int refs[]={-5000,-2500,0,2500,5000};
    for(unsigned i=0;i<5;i++){frame((float)refs[i],0,0);key(KEY_HOLD_SHORT);}
    CHECK(saves==(int)ns+1);CHECK(flash.dc_count==5);CHECK(calibration_valid(&flash));
    CHECK(strstr(screen,"WAIT FOR NEW DATA"));
    printf("PASS %u app checks: fresh mode transitions, hold, calibration, save failure, timeout and UART recovery\n",checks);return 0;
}
