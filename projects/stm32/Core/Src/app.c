#include "app.h"
#include "board.h"
#include "display.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

enum { MODE_DC,MODE_AC,MODE_WAVE };
static Calibration cal,cal_backup;
static Measurement latest;
static Waveform wave;
static uint8_t mode,hold,have_measurement,busy,oled_ok,wizard;
static uint32_t sample_fs,started,settle_until,last_frame,last_display,last_vref,last_oled_retry;
static float vref;
static char message[22];
static uint32_t message_until;
static char rx_line[96];static unsigned rx_used;static uint8_t rx_drop;
static unsigned wizard_index;
static const int32_t wizard_mv[]={-5000,-2500,0,2500,5000};

static int expired(uint32_t now,uint32_t deadline) {return (int32_t)(now-deadline)>=0;}
static void tell(const char *s) {
    snprintf(message,sizeof(message),"%s",s);message_until=HAL_GetTick()+2000u;
    board_uart_puts(s);board_uart_puts("\r\n");
}
static void format_voltage(char *out,size_t n,float mv) {
    int32_t v=(int32_t)lroundf(mv);uint32_t a=(uint32_t)(v<0?-v:v);
    snprintf(out,n,"%s%lu.%03luV",v<0?"-":"",(unsigned long)(a/1000u),(unsigned long)(a%1000u));
}
static void format_frequency(char *out,size_t n,float hz) {
    uint32_t f=(uint32_t)lroundf(fmaxf(hz,0.0f)*10.0f);
    snprintf(out,n,"%lu.%luHZ",(unsigned long)(f/10u),(unsigned long)(f%10u));
}
static void render(void) {
    char text[40],value[24];uint32_t now=HAL_GetTick();gfx_clear();
    if(wizard) {
        gfx_text(0,0,"DC CALIBRATION",1);
        format_voltage(value,sizeof(value),(float)wizard_mv[wizard_index]);
        snprintf(text,sizeof(text),"APPLY %s",value);gfx_text(0,12,text,1);
        gfx_text(0,24,"SW3:CAPTURE",1);gfx_text(0,34,"SW2:SKIP",1);
        gfx_text(0,44,"HOLD SW2:CANCEL",1);
        if(!expired(now,message_until))gfx_text(0,56,message,1);
        else {snprintf(text,sizeof(text),"POINTS:%lu",(unsigned long)cal.dc_count);gfx_text(0,56,text,1);}
    } else {
        const char *name=mode==MODE_DC?"DC":(mode==MODE_AC?"AC":"WAVE");
        int calibrated=cal.dc_count>=2;
        if(mode==MODE_AC)calibrated=calibrated && cal.ac_valid[0]==15u && cal.ac_valid[1]==15u;
        snprintf(text,sizeof(text),"%s %s%s",name,calibrated?"CAL":"UNCAL",hold?" HOLD":"");gfx_text(0,0,text,1);
        if(!have_measurement)gfx_text(0,24,"WAIT FOR NEW DATA",1);
        else if(mode==MODE_DC) {
            format_voltage(value,sizeof(value),latest.dc_mv);gfx_text(0,20,value,2);
            gfx_text(0,44,"RANGE -5V TO +5V",1);
            gfx_text(0,56,latest.flags&(MEAS_MAIN_CLIP|MEAS_OVER_RANGE)?"OVER RANGE":"SW2 MODE / SW3 HOLD",1);
        } else if(mode==MODE_AC) {
            format_frequency(value,sizeof(value),latest.frequency_hz);snprintf(text,sizeof(text),"F: %s",value);gfx_text(0,16,text,1);
            format_voltage(value,sizeof(value),latest.vpp_mv);snprintf(text,sizeof(text),"VPP: %s",value);gfx_text(0,32,text,1);
            gfx_text(0,46,latest.channel?"SMALL SIGNAL RANGE":"WIDE SIGNAL RANGE",1);
            const char *status=latest.flags&MEAS_MAIN_CLIP?"INPUT CLIPPED":
                (latest.flags&MEAS_NO_SIGNAL?"NO SIGNAL":(!(latest.flags&MEAS_FREQ_OK)?"WAIT / CHECK FREQ":
                (latest.flags&MEAS_DISTORTED?"NOT A CLEAN SINE":"SW2 MODE / SW3 HOLD")));
            gfx_text(0,56,status,1);
        } else {
            format_frequency(value,sizeof(value),latest.frequency_hz);snprintf(text,sizeof(text),"F:%s",value);gfx_text(0,8,text,1);
            format_voltage(value,sizeof(value),latest.vpp_mv);snprintf(text,sizeof(text),"PP:%s",value);gfx_text(0,16,text,1);
            gfx_wave(&wave);
        }
        if(mode!=MODE_WAVE && !expired(now,message_until)) {
            /* Clear the status row before drawing a temporary message. */
            memset(&oled_frame[7*128],0,128);gfx_text(0,56,message,1);
        }
    }
    if(oled_ok && !oled_flush()) {oled_ok=0;last_oled_retry=now;board_uart_puts("WARN OLED I2C failed; measurement continues\r\n");}
    last_display=now;
}
static void restart(void) {
    board_acquire_stop();busy=0;have_measurement=0;hold=0;
    memset(&latest,0,sizeof(latest));
    board_frequency_reset();settle_until=HAL_GetTick()+35u;
    render();
}
static void status(void) {
    char t[240];
    snprintf(t,sizeof(t),"MODE=%u VALID=%u DC_MV=%ld PP_MV=%ld FREQ_MILLIHZ=%lu FS=%lu CH=%u FLAGS=0x%lX\r\n"
        "RAW_MAIN_X100=%ld RAW_SMALL_X100=%ld VREF_X100=%ld DC_POINTS=%lu AC_MASK=%lu,%lu OLED=0x%02X TYPE=%lu\r\n",
        mode,have_measurement,(long)lroundf(latest.dc_mv),(long)lroundf(latest.vpp_mv),
        (unsigned long)lroundf(latest.frequency_hz*1000.0f),(unsigned long)sample_fs,latest.channel,
        (unsigned long)latest.flags,(long)lroundf(latest.mean[0]*100.0f),(long)lroundf(latest.mean[1]*100.0f),
        (long)lroundf(vref*100.0f),(unsigned long)cal.dc_count,(unsigned long)cal.ac_valid[0],
        (unsigned long)cal.ac_valid[1],oled_address(),(unsigned long)cal.oled_type);
    board_uart_puts(t);
}
static int stable_dc(void) {
    return have_measurement && mode==MODE_DC && HAL_GetTick()-last_frame<250u &&
        latest.stddev[0]<4.0f && !(latest.flags&MEAS_MAIN_CLIP) && vref>800;
}
static void save(void) {
    busy=0;
    if(board_cal_save(&cal))tell("SAVED TO FLASH");else tell("ERROR FLASH SAVE");
    have_measurement=0;settle_until=HAL_GetTick()+35u;render();
}
static void wizard_next(void) {
    wizard_index++;
    if(wizard_index==5) {
        wizard=0;
        if(cal.dc_count>=2)save();
        else {cal=cal_backup;tell("NEED 2+ POINTS");}
        restart();
    } else render();
}
static void handle_keys(uint32_t e) {
    if(wizard) {
        if(e&KEY_MODE_LONG) {cal=cal_backup;wizard=0;tell("CAL CANCELED");restart();return;}
        if(e&KEY_HOLD_SHORT) {
            if(!stable_dc()) {tell("NEED STABLE DC");render();return;}
            float nominal=(latest.mean[0]*3300.0f/4095.0f-MAIN_OFFSET_V*1000)/MAIN_GAIN;
            if(fabsf(nominal-(float)wizard_mv[wizard_index])>500.0f) {tell("CHECK APPLIED VOLTS");render();return;}
            if(calibration_add_dc(&cal,latest.mean[0],(float)wizard_mv[wizard_index],vref)) {
                tell("POINT CAPTURED");wizard_next();
            } else {tell("INVALID CAL POINT");render();}
        } else if(e&KEY_MODE_SHORT)wizard_next();
        return;
    }
    if(e&KEY_MODE_LONG) {
        cal_backup=cal;cal.dc_count=0;cal.vref_at_cal=0;
        memset(cal.dc,0,sizeof(cal.dc));
        for(unsigned ch=0;ch<2;ch++) {cal.ac_valid[ch]=0;for(unsigned i=0;i<AC_POINTS;i++)cal.ac_trim[ch][i]=1;}
        wizard=1;wizard_index=0;mode=MODE_DC;restart();return;
    }
    if(e&KEY_MODE_SHORT) {mode=(uint8_t)((mode+1u)%3u);restart();return;}
    if(e&KEY_HOLD_LONG) {
        cal.oled_type^=1u;oled_ok=(uint8_t)oled_init((uint8_t)cal.oled_type);
        tell(cal.oled_type?"SH1106 SELECTED":"SSD1306 SELECTED");render();return;
    }
    if(e&KEY_HOLD_SHORT) {hold^=1u;render();}
}
static int read_number(const char *s,float *v) {
    while(*s==' ')s++;
    char *end;*v=strtof(s,&end);
    if(s==end)return 0;
    while(*end==' ')end++;
    return !*end && isfinite(*v);
}
static void command(char *s) {
    while(*s==' ')s++;
    for(char *p=s;*p;p++)*p=(char)toupper((unsigned char)*p);
    if(!*s)return;
    if(wizard) {tell("EXIT KEY CAL FIRST");return;}
    if(!strcmp(s,"HELP")) {
        board_uart_puts("STATUS | RAW | MODE DC/AC/WAVE | CAL DC <mV> | CAL AC <Vpp_mV> | CAL FREQ <Hz>\r\n"
            "SAVE | CAL CLEAR | DEFAULTS | OLED SSD1306/SH1106 | HOLD ON/OFF\r\n"
            "CAL changes RAM only; SAVE commits. DC first, then AC. Use known reference inputs.\r\n");
    } else if(!strcmp(s,"STATUS")||!strcmp(s,"RAW"))status();
    else if(!strncmp(s,"MODE ",5)) {
        if(!strcmp(s+5,"DC"))mode=MODE_DC;else if(!strcmp(s+5,"AC"))mode=MODE_AC;
        else if(!strcmp(s+5,"WAVE"))mode=MODE_WAVE;else {tell("ERROR MODE");return;}
        restart();
    } else if(!strcmp(s,"SAVE"))save();
    else if(!strcmp(s,"CAL CLEAR")||!strcmp(s,"DEFAULTS")) {
        uint32_t controller=cal.oled_type;float freq_trim=cal.frequency_trim;
        calibration_defaults(&cal);
        if(!strcmp(s,"CAL CLEAR")) {cal.oled_type=controller;cal.frequency_trim=freq_trim;}
        tell("RAM CAL RESET");restart();
    } else if(!strncmp(s,"OLED ",5)) {
        if(!strcmp(s+5,"SSD1306"))cal.oled_type=0;else if(!strcmp(s+5,"SH1106"))cal.oled_type=1;
        else {tell("ERROR OLED TYPE");return;}
        oled_ok=(uint8_t)oled_init((uint8_t)cal.oled_type);render();tell("OLED RAM CONFIG SET");
    } else if(!strncmp(s,"HOLD ",5)) {
        if(!strcmp(s+5,"ON"))hold=1;else if(!strcmp(s+5,"OFF"))hold=0;else {tell("ERROR HOLD");return;}
        render();
    } else if(!strncmp(s,"CAL DC ",7)) {
        float mv;
        if(!read_number(s+7,&mv)||!stable_dc()) {tell("ERROR STABLE DC REF");return;}
        if(calibration_add_dc(&cal,latest.mean[0],mv,vref))tell("DC POINT ADDED RAM");else tell("INVALID CAL POINT");
    } else if(!strncmp(s,"CAL AC ",7)) {
        float mv;
        if(!read_number(s+7,&mv)||mv<80 || mv>5500 || cal.dc_count<2 || mode!=MODE_AC || !have_measurement ||
           HAL_GetTick()-last_frame>250 || !(latest.flags&MEAS_FREQ_OK) ||
           (latest.flags&(MEAS_DISTORTED|MEAS_MAIN_CLIP|MEAS_NO_SIGNAL))) {tell("ERROR AC REFERENCE");return;}
        unsigned k=0;float distance=1e9f;
        for(unsigned i=0;i<AC_POINTS;i++) {
            float d=fabsf(latest.frequency_hz-ac_frequencies[i])/ac_frequencies[i];if(d<distance){distance=d;k=i;}
        }
        if(distance>0.03f) {tell("USE 100/1K/10K/20K");return;}
        Calibration next=cal;unsigned changed=0;
        if(!next.vref_at_cal)next.vref_at_cal=vref;
        for(uint8_t ch=0;ch<2;ch++) {
            if(ch && ((latest.flags&MEAS_SMALL_CLIP)||latest.raw_pp[ch]>3000 || latest.raw_pp[ch]<24))continue;
            if(latest.fitted_pp[ch]<12)continue;
            float prior=calibration_ac_factor(&next,ch,latest.frequency_hz,vref);
            float trim=next.ac_trim[ch][k]*mv/(latest.fitted_pp[ch]*prior);
            if(trim>=0.5f && trim<=2.0f) {next.ac_trim[ch][k]=trim;next.ac_valid[ch]|=1u<<k;changed++;}
        }
        next.crc=calibration_crc(&next);
        if(changed && calibration_valid(&next)){cal=next;tell("AC TRIM ADDED RAM");}else tell("ERROR AC TRIM");
    } else if(!strncmp(s,"CAL FREQ ",9)) {
        float known,raw=board_frequency();
        if(!read_number(s+9,&known)||known<100 || known>20000 || raw<80 || known/raw<0.8f || known/raw>1.2f)
            tell("ERROR FREQ REFERENCE");
        else {cal.frequency_trim=known/raw;tell("FREQ TRIM SET RAM");}
    } else tell("UNKNOWN CMD; HELP");
}
static void uart_poll(void) {
    int ch;
    while((ch=board_uart_getchar())!=-1) {
        if(ch==-2) {rx_used=0;rx_drop=1;tell("ERROR UART OVERFLOW");continue;}
        if(ch=='\r'||ch=='\n') {
            if(rx_drop) {rx_drop=0;rx_used=0;}
            else {rx_line[rx_used]=0;command(rx_line);rx_used=0;}
        } else if(!rx_drop) {
            if(ch==8||ch==127) {if(rx_used)rx_used--;}
            else if(ch>=32 && ch<=126) {
                if(rx_used+1<sizeof(rx_line))rx_line[rx_used++]=(char)ch;
                else {rx_drop=1;rx_used=0;tell("ERROR LINE TOO LONG");}
            }
        }
    }
}
void app_init(void) {
    (void)board_cal_load(&cal);vref=board_vref_read();
    oled_ok=(uint8_t)oled_init((uint8_t)cal.oled_type);
    if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_1)==GPIO_PIN_RESET) {
        cal.oled_type^=1u;oled_ok=(uint8_t)oled_init((uint8_t)cal.oled_type);
    }
    board_uart_puts("B CARRIER HAL V1.0 STM32F103C8 72MHz SSD1306\r\n");
    board_uart_puts(cal.dc_count>=2?"DC calibration loaded\r\n":"UNCALIBRATED: nominal conversion only\r\n");
    if(!oled_ok)board_uart_puts("WARN OLED not responding at 0x3C/0x3D\r\n");
    last_vref=HAL_GetTick();restart();
}
void app_poll(void) {
    uint32_t now=HAL_GetTick();handle_keys(board_keys());uart_poll();now=HAL_GetTick();
    if(!oled_ok && now-last_oled_retry>=2000u) {
        last_oled_retry=now;oled_ok=(uint8_t)oled_init((uint8_t)cal.oled_type);if(oled_ok)render();
    }
    if(busy && (board_acquisition_error() || (!board_frame_ready() && now-started>120u))) {
        board_acquire_stop();busy=0;have_measurement=0;tell("ADC TIMEOUT/ERROR");
        settle_until=now+35u;render();return;
    }
    if(busy && board_frame_ready()) {
        /* DMA is normal mode. The complete buffer remains immutable during analysis/drawing. */
        board_acquire_stop();busy=0;
        board_normalize_frame();
        measure_frame(adc_pairs,MEAS_SAMPLES,sample_fs,board_frequency(),vref,&cal,mode==MODE_AC,&latest);
        if(mode==MODE_WAVE)measure_waveform(adc_pairs,MEAS_SAMPLES,&latest,&cal,vref,&wave);
        have_measurement=1;last_frame=HAL_GetTick();
        HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,(last_frame/500u)&1u?GPIO_PIN_SET:GPIO_PIN_RESET);
        if(!hold && (last_frame-last_display>=80u || wizard))render();
    }
    if(!busy && expired(HAL_GetTick(),settle_until)) {
        if(HAL_GetTick()-last_vref>=2000u) {
            float next=board_vref_read();if(next>800 && next<2000)vref=next;last_vref=HAL_GetTick();
        }
        float f=board_frequency()*cal.frequency_trim;
        if(f==0 && have_measurement)f=latest.frequency_hz;
        sample_fs=(mode==MODE_DC || (mode==MODE_AC && f>=80 && f<1000))?20000u:200000u;
        if(mode==MODE_WAVE)sample_fs=400000u;
        /* If a low-frequency capture wire is absent, obtain a longer window after the first fast frame. */
        if(mode==MODE_AC && f==0 && have_measurement && latest.raw_pp[latest.channel]>=12.0f &&
           latest.sample_rate==200000u)sample_fs=20000u;
        if(board_acquire(sample_fs)==HAL_OK){busy=1;started=HAL_GetTick();}
        else {tell("ADC START FAILED");settle_until=HAL_GetTick()+100u;render();}
    }
}
