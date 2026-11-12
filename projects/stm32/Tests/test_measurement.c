#include "measurement.h"
#include "display.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static uint16_t samples[MEAS_SAMPLES*2];
static unsigned checks;
static float worst_pp,worst_freq;
#define CHECK(e) do { checks++;if(!(e)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#e);exit(1);} }while(0)
static uint16_t code(float mv) {
    long v=lroundf(mv*4095.0f/3300.0f);if(v<0)v=0;if(v>4095)v=4095;return (uint16_t)v;
}
static float minf(float a,float b) {return a<b?a:b;}
static float maxf(float a,float b) {return a>b?a:b;}
static void synth(float f,float pp,float offset,float fs,float phase,int shape,int noise) {
    unsigned rng=17;
    for(unsigned i=0;i<MEAS_SAMPLES;i++) {
        float phi=6.28318530718f*f*(float)i/fs+phase;
        float v=sinf(phi);
        if(shape==1)v=v>=0?1.0f:-1.0f;
        if(shape==2)v=2.0f*asinf(v)/3.14159265359f;
        float vin=offset+0.5f*pp*v;
        float main_mv=MAIN_OFFSET_V*1000.0f+MAIN_GAIN*vin;
        float high=1650.0f+0.5f*pp*small_gain_at(f)*v;
        rng=rng*1664525u+1013904223u;float err=noise?((float)((rng>>16)&255u)/255.0f-0.5f)*0.8f:0;
        samples[2*i]=code(main_mv+err);samples[2*i+1]=code(high+err);
    }
}
static void test_dc(void) {
    Calibration cal;calibration_defaults(&cal);CHECK(calibration_valid(&cal));
    for(int mv=-5000;mv<=5000;mv+=50) {
        for(unsigned i=0;i<MEAS_SAMPLES;i++) {
            samples[2*i]=code(MAIN_OFFSET_V*1000.0f+MAIN_GAIN*(float)mv);samples[2*i+1]=2047;
        }
        Measurement m;measure_frame(samples,MEAS_SAMPLES,20000,0,1490,&cal,0,&m);
        CHECK(fabsf(m.dc_mv-(float)mv)<2.0f);CHECK(m.frequency_hz==0);
    }
    CHECK(!calibration_add_dc(&cal,NAN,0,1490));
    CHECK(!calibration_add_dc(&cal,2300,INFINITY,1490));
    CHECK(!calibration_add_dc(&cal,0,0,1490));
    const float input[]={-5000,-2500,0,2500,5000};
    for(unsigned i=0;i<5;i++) {
        float raw=2301.0f+input[i]/3.210f+1.1f*(input[i]/5000.0f)*(input[i]/5000.0f);
        CHECK(calibration_add_dc(&cal,raw,input[i],1490));
    }
    CHECK(cal.dc_count==5);
    for(int mv=-5000;mv<=5000;mv+=50) {
        float raw=2301.0f+(float)mv/3.210f+1.1f*((float)mv/5000.0f)*((float)mv/5000.0f);
        CHECK(fabsf(calibration_dc(&cal,raw,1490)-(float)mv)<1.0f);
    }
    CHECK(!calibration_add_dc(&cal,2301,5000,1490));
    CHECK(cal.dc_count==5);
    CHECK(calibration_add_dc(&cal,2301,0,1490));
    CHECK(cal.dc_count==5);
    /* VREFINT is used as a relative ratio, not as a claimed precision factory reference. */
    float z=2301.0f,raw=z+(1550.0f/3.210f)/1.02f;
    CHECK(fabsf(calibration_dc(&cal,raw,1490.0f/1.02f)-1550.0f)<4.0f);
    Calibration bad=cal;bad.dc_count=100;bad.crc=calibration_crc(&bad);CHECK(!calibration_valid(&bad));
    bad=cal;bad.dc[1].code=bad.dc[0].code;bad.crc=calibration_crc(&bad);CHECK(!calibration_valid(&bad));
}
static void test_ac(void) {
    Calibration cal;calibration_defaults(&cal);
    const float f[]={100,101,200,999,1000,1001,3000,10000,19999,20000};
    const float pp[]={100,200,500,1000,2000,5000};
    for(unsigned fi=0;fi<sizeof(f)/sizeof(f[0]);fi++)
      for(unsigned pi=0;pi<sizeof(pp)/sizeof(pp[0]);pi++)
        for(unsigned phase=0;phase<8;phase++) {
            float fs=f[fi]<1000?20000.0f:200000.0f;
            synth(f[fi],pp[pi],0,fs,0.71f*(float)phase,0,1);
            Measurement m;measure_frame(samples,MEAS_SAMPLES,(uint32_t)fs,f[fi],1490,&cal,1,&m);
            float ep=fabsf(m.vpp_mv-pp[pi])/pp[pi],ef=fabsf(m.frequency_hz-f[fi])/f[fi];
            worst_pp=maxf(worst_pp,ep);worst_freq=maxf(worst_freq,ef);
            CHECK(ep<0.03f);CHECK(ef<0.01f);CHECK(m.flags&MEAS_FREQ_OK);
            if(pp[pi]>=2000)CHECK(m.channel==0 && (m.flags&MEAS_SMALL_CLIP));
            if(pp[pi]==100)CHECK(m.channel==1);
        }
    /* Basic AC range includes positive DC offsets. */
    synth(1000,2000,2500,200000,1.1f,0,0);Measurement m;
    measure_frame(samples,MEAS_SAMPLES,200000,1000,1490,&cal,1,&m);
    CHECK(fabsf(m.vpp_mv-2000)<10);CHECK(fabsf(m.dc_mv-2500)<80);
    /* Missing capture connection: use Schmitt-armed interpolated ADC crossings. */
    synth(100,100,0,20000,0.2f,0,1);measure_frame(samples,MEAS_SAMPLES,20000,0,1490,&cal,1,&m);
    CHECK(fabsf(m.frequency_hz-100)<0.5f);CHECK(fabsf(m.vpp_mv-100)<3);
    synth(20000,5000,0,200000,0.17f,0,0);measure_frame(samples,MEAS_SAMPLES,200000,0,1490,&cal,1,&m);
    CHECK(fabsf(m.frequency_hz-20000)<50);CHECK(fabsf(m.vpp_mv-5000)<100);
    /* Timer clock calibration must correct both frequency and waveform window scale. */
    cal.frequency_trim=1.015f;
    synth(10000,500,0,203000,0.31f,0,0);
    measure_frame(samples,MEAS_SAMPLES,200000,10000.0f/1.015f,1490,&cal,1,&m);
    CHECK(fabsf(m.frequency_hz-10000)<5);CHECK(fabsf(m.vpp_mv-500)<10);
    cal.frequency_trim=1;
    /* Comparator chatter on a DC input is not accepted as a real signal. */
    for(unsigned i=0;i<MEAS_SAMPLES;i++){samples[2*i]=2298;samples[2*i+1]=(uint16_t)(2047+(i&1));}
    measure_frame(samples,MEAS_SAMPLES,200000,20000,1490,&cal,1,&m);
    CHECK(m.flags&MEAS_NO_SIGNAL);CHECK(m.frequency_hz==0);CHECK(m.vpp_mv==0);
    /* Square wave must be flagged as non-sinusoidal in AC mode. */
    synth(1000,500,0,200000,0.4f,1,0);measure_frame(samples,MEAS_SAMPLES,200000,1000,1490,&cal,1,&m);
    CHECK(m.flags&MEAS_DISTORTED);
}
static void test_wave(void) {
    Calibration cal;calibration_defaults(&cal);
    for(unsigned shape=0;shape<3;shape++)for(unsigned fi=0;fi<2;fi++) {
        float f=fi?10000.0f:1000.0f;
        synth(f,6600,0,400000,0.2f,(int)shape,0);
        Measurement m;Waveform w;
        measure_frame(samples,MEAS_SAMPLES,400000,f,1490,&cal,0,&m);
        measure_waveform(samples,MEAS_SAMPLES,&m,&cal,1490,&w);
        CHECK(w.periods>=1.0f);CHECK(w.periods<=1.26f);CHECK(m.channel==0);
        CHECK(fabsf(m.vpp_mv-6600)<350);
        int lo=32767,hi=-32768;
        for(unsigned x=0;x<128;x++) {
            CHECK(w.low[x]<=w.center[x]);CHECK(w.center[x]<=w.high[x]);
            lo=(int)minf((float)lo,(float)w.low[x]);hi=(int)maxf((float)hi,(float)w.high[x]);
        }
        CHECK(hi-lo>6100);
        gfx_clear();gfx_wave(&w);gfx_text(0,0,"WAVE TEST",1);
        unsigned pixels=0;for(unsigned i=0;i<1024;i++)pixels+=oled_frame[i]!=0;CHECK(pixels>100);
    }
    gfx_clear();gfx_pixel(-1,0,1);gfx_pixel(128,63,1);gfx_pixel(0,64,1);
    CHECK(oled_frame[0]==0);gfx_pixel(127,63,1);CHECK(oled_frame[1023]==128);
    gfx_text(120,60,"~ABC",2); /* Boundary and last glyph are valid, without writing past framebuffer. */
}
static void test_flash_record(void) {
    Calibration a,b;calibration_defaults(&a);calibration_defaults(&b);
    a.generation=9;a.crc=calibration_crc(&a);b.generation=10;b.crc=calibration_crc(&b);
    CHECK(calibration_select(&a,&b)==&b);
    b.dc[0].mv=1;CHECK(calibration_select(&a,&b)==&a);
    a.crc^=1;CHECK(calibration_select(&a,&b)==NULL);
    calibration_defaults(&a);calibration_defaults(&b);
    a.generation=0xffffffffu;a.crc=calibration_crc(&a);b.generation=0;b.crc=calibration_crc(&b);
    CHECK(calibration_select(&a,&b)==&b);
    unsigned char page[sizeof(Calibration)];memset(page,0xff,sizeof(page));
    Calibration partial;memcpy(&partial,page,sizeof(partial));CHECK(!calibration_valid(&partial));
    /* Every interrupted halfword write must leave the old page selected. */
    for(size_t i=4;i<sizeof(b);i+=2) {
        memcpy(page+i,(const uint8_t *)&b+i,2);memcpy(&partial,page,sizeof(partial));
        CHECK(calibration_select(&a,&partial)==&a);
    }
    memcpy(page,&b,2);memcpy(&partial,page,sizeof(partial));CHECK(calibration_select(&a,&partial)==&a);
    memcpy(page+2,(const uint8_t *)&b+2,2);memcpy(&partial,page,sizeof(partial));CHECK(calibration_valid(&partial));
    CHECK(sizeof(Calibration)<=1024);CHECK((sizeof(Calibration)&1u)==0);
}
int main(void) {
    test_dc();test_ac();test_wave();test_flash_record();
    printf("PASS %u checks; synthetic sine worst Vpp error %.4f%%, frequency %.4f%%\n",checks,100*worst_pp,100*worst_freq);
    puts("These are algorithm tests, not a physical board accuracy certificate.");return 0;
}
