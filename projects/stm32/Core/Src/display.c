#include "display.h"
#include "oledfont.h"
#include <string.h>
#include <stdlib.h>
uint8_t oled_frame[1024];
void gfx_clear(void) { memset(oled_frame,0,sizeof(oled_frame)); }
void gfx_pixel(int x,int y,int on) {
    if(x<0||x>=128||y<0||y>=64)return;
    unsigned index=(unsigned)(y/8)*128u+(unsigned)x;
    uint8_t mask=(uint8_t)(1u<<(y&7));
    if(on)oled_frame[index]|=mask;else oled_frame[index]&=(uint8_t)~mask;
}
void gfx_line(int x0,int y0,int x1,int y1) {
    int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy;
    for(;;) {gfx_pixel(x0,y0,1);if(x0==x1&&y0==y1)break;int e=2*err;if(e>=dy){err+=dy;x0+=sx;}if(e<=dx){err+=dx;y0+=sy;}}
}
void gfx_text(int x,int y,const char *t,int scale) {
    if(scale!=1 && scale!=2)return;
    while(*t && x<128) {
        unsigned char ch=(unsigned char)*t++;if(ch<32||ch>126)ch='?';
        for(int col=0;col<6;col++) for(int bit=0;bit<8;bit++) if(F6x8[ch-32][col]&(1u<<bit))
            for(int a=0;a<scale;a++)for(int b=0;b<scale;b++)gfx_pixel(x+col*scale+a,y+bit*scale+b,1);
        x+=6*scale;
    }
}
void gfx_wave(const Waveform *w) {
    int lo=32767,hi=-32768;
    for(unsigned x=0;x<128;x++){if(w->low[x]<lo)lo=w->low[x];if(w->high[x]>hi)hi=w->high[x];}
    int range=hi-lo;if(range<100)range=100;int center=(hi+lo)/2;
    /* Plot occupies y=25..63; the zero line is shown only when it lies inside the view. */
    int zero=44+center*34/range;
    if(zero>=26 && zero<=62)for(int x=0;x<128;x+=4)gfx_pixel(x,zero,1);
    int previous=44;
    for(int x=0;x<128;x++) {
        int y=44-((int)w->center[x]-center)*34/range;
        int yl=44-((int)w->low[x]-center)*34/range,yh=44-((int)w->high[x]-center)*34/range;
        if(y<26)y=26;
        if(y>62)y=62;
        if(yl<26)yl=26;
        if(yl>62)yl=62;
        if(yh<26)yh=26;
        if(yh>62)yh=62;
        gfx_line(x,yl,x,yh);if(x)gfx_line(x-1,previous,x,y);previous=y;
    }
}
