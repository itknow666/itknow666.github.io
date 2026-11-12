#include "measurement.h"
#include <math.h>
#include <string.h>

const float ac_frequencies[AC_POINTS] = {100.0f,1000.0f,10000.0f,20000.0f};
static float clampf(float x,float a,float b) { return x<a?a:(x>b?b:x); }
static float reference_ratio(const Calibration *c,float vref) {
    if(c->vref_at_cal>800.0f && vref>800.0f)
        return clampf(c->vref_at_cal/vref,0.85f,1.15f);
    return 1.0f;
}
void calibration_defaults(Calibration *c) {
    memset(c,0,sizeof(*c)); c->magic=CAL_MAGIC; c->version=CAL_VERSION;
    c->frequency_trim=1.0f;
    for(unsigned ch=0;ch<2;ch++) for(unsigned i=0;i<AC_POINTS;i++) c->ac_trim[ch][i]=1.0f;
    c->crc=calibration_crc(c);
}
uint32_t calibration_crc(const Calibration *c) {
    const uint8_t *b=(const uint8_t *)c; uint32_t crc=0xffffffffu;
    for(size_t i=0;i<offsetof(Calibration,crc);i++) {
        crc^=b[i]; for(unsigned j=0;j<8;j++) crc=(crc>>1)^((crc&1u)?0xedb88320u:0u);
    }
    return ~crc;
}
int calibration_valid(const Calibration *c) {
    if(c->magic!=CAL_MAGIC || c->version!=CAL_VERSION || c->crc!=calibration_crc(c) ||
       c->dc_count>DC_POINTS_MAX || c->oled_type>1u || !isfinite(c->frequency_trim) ||
       c->frequency_trim<0.8f || c->frequency_trim>1.2f || !isfinite(c->vref_at_cal) ||
       (c->vref_at_cal!=0.0f && (c->vref_at_cal<800.0f || c->vref_at_cal>2000.0f))) return 0;
    for(unsigned i=0;i<c->dc_count;i++) {
        if(!isfinite(c->dc[i].code)||!isfinite(c->dc[i].mv)||c->dc[i].code<20.0f||
           c->dc[i].code>4075.0f||fabsf(c->dc[i].mv)>5500.0f) return 0;
        if(i) {
            float dx=c->dc[i].code-c->dc[i-1].code,dy=c->dc[i].mv-c->dc[i-1].mv;
            if(dx<8.0f || dy/dx<2.0f || dy/dx>4.5f) return 0;
        }
    }
    for(unsigned ch=0;ch<2;ch++) {
        if(c->ac_valid[ch]&~15u) return 0;
        for(unsigned i=0;i<AC_POINTS;i++)
            if(!isfinite(c->ac_trim[ch][i]) || c->ac_trim[ch][i]<0.5f || c->ac_trim[ch][i]>2.0f) return 0;
    }
    return 1;
}
const Calibration *calibration_select(const Calibration *a,const Calibration *b) {
    int va=calibration_valid(a),vb=calibration_valid(b);
    if(va && vb)return (int32_t)(b->generation-a->generation)>0?b:a;
    return va?a:(vb?b:NULL);
}
static float zero_code(const Calibration *c) {
    if(c->dc_count<2) return MAIN_OFFSET_V*ADC_FULL_SCALE/3.3f;
    unsigned i=0;
    while(i+1<c->dc_count && c->dc[i+1].mv<0.0f) i++;
    if(i+1==c->dc_count) i--;
    const DcPoint *a=&c->dc[i],*b=&c->dc[i+1];
    return a->code-a->mv*(b->code-a->code)/(b->mv-a->mv);
}
int calibration_add_dc(Calibration *c,float raw_code,float mv,float vref) {
    if(!isfinite(raw_code)||!isfinite(mv)||fabsf(mv)>5500.0f||raw_code<20||raw_code>4075 ||
       !isfinite(vref)||vref<800.0f||vref>2000.0f) return 0;
    Calibration next=*c;
    if(!next.dc_count) next.vref_at_cal=vref;
    float z=zero_code(&next);
    float normalized=z+(raw_code-z)*reference_ratio(&next,vref);
    unsigned i;
    for(i=0;i<next.dc_count;i++) if(fabsf(next.dc[i].mv-mv)<0.5f) break;
    if(i==next.dc_count) {
        if(next.dc_count==DC_POINTS_MAX) return 0;
        next.dc_count++;
    }
    next.dc[i].code=normalized; next.dc[i].mv=mv;
    for(unsigned a=1;a<next.dc_count;a++) {
        DcPoint p=next.dc[a];unsigned b=a;
        while(b && next.dc[b-1].code>p.code) {next.dc[b]=next.dc[b-1];b--;}
        next.dc[b]=p;
    }
    /* A changed DC scale/reference invalidates earlier AC trims. Calibrate DC first. */
    for(unsigned ch=0;ch<2;ch++) {
        next.ac_valid[ch]=0;
        for(unsigned k=0;k<AC_POINTS;k++)next.ac_trim[ch][k]=1.0f;
    }
    next.crc=calibration_crc(&next);
    if(!calibration_valid(&next)) return 0;
    *c=next;return 1;
}
float calibration_dc(const Calibration *c,float code,float vref) {
    if(c->dc_count<2) return (code*NOMINAL_VDDA_MV/ADC_FULL_SCALE-MAIN_OFFSET_V*1000.0f)/MAIN_GAIN;
    float z=zero_code(c);code=z+(code-z)*reference_ratio(c,vref);
    unsigned i=0;
    while(i+1<c->dc_count && c->dc[i+1].code<code) i++;
    if(i+1==c->dc_count) i--;
    const DcPoint *a=&c->dc[i],*b=&c->dc[i+1];
    return a->mv+(code-a->code)*(b->mv-a->mv)/(b->code-a->code);
}
static float interp(const float *x,const float *y,unsigned n,float f) {
    if(f<=x[0]) return y[0];
    for(unsigned i=1;i<n;i++) if(f<=x[i]) {
        /* Linear in reciprocal frequency resolves the coupling corner better than linear Hz. */
        float t=(1.0f/x[i-1]-1.0f/f)/(1.0f/x[i-1]-1.0f/x[i]);
        return y[i-1]+t*(y[i]-y[i-1]);
    }
    return y[n-1];
}
float small_gain_at(float hz) {
    static const float gains[]={2.4257053f,2.5169893f,2.5179756f,2.5179831f};
    return interp(ac_frequencies,gains,AC_POINTS,fmaxf(hz,100.0f));
}
float calibration_ac_factor(const Calibration *c,uint8_t channel,float hz,float vref) {
    if(channel>1) return 0.0f;
    float per_code=NOMINAL_VDDA_MV/ADC_FULL_SCALE/(channel?small_gain_at(hz):MAIN_GAIN);
    if(channel==0 && c->dc_count>=2) {
        const DcPoint *a=&c->dc[0],*b=&c->dc[c->dc_count-1];
        per_code=(b->mv-a->mv)/(b->code-a->code);
    }
    return per_code*reference_ratio(c,vref)*interp(ac_frequencies,c->ac_trim[channel],AC_POINTS,hz);
}
static void stats(const uint16_t *p,size_t n,uint8_t ch,float *mean,float *std,uint16_t *lo,uint16_t *hi) {
    uint32_t sum=0;*lo=4095;*hi=0;
    for(size_t i=0;i<n;i++) { uint16_t v=p[2*i+ch];sum+=v;if(v<*lo)*lo=v;if(v>*hi)*hi=v; }
    *mean=(float)sum/(float)n;float v=0;
    for(size_t i=0;i<n;i++) {float d=p[2*i+ch]-*mean;v+=d*d;}
    *std=sqrtf(v/(float)n);
}
float estimate_frequency(const uint16_t *p,size_t n,uint8_t ch,float fs) {
    if(!p || n<8 || ch>1 || fs<=0) return 0;
    float mean,std;uint16_t lo,hi;stats(p,n,ch,&mean,&std,&lo,&hi);
    if(hi-lo<12 || std<3.0f) return 0;
    float threshold=mean,low=mean-0.15f*(hi-lo),first=0,last=0;
    unsigned count=0;int armed=0;
    for(size_t i=1;i<n;i++) {
        float a=p[2*(i-1)+ch],b=p[2*i+ch];
        if(a<=low) armed=1;
        if(armed && a<threshold && b>=threshold && b>a) {
            float pos=(float)(i-1)+(threshold-a)/(b-a);
            if(!count) {first=last=pos;count=1;}
            else if(pos-last>=fs/23000.0f) {last=pos;count++;}
            armed=0;
        }
    }
    if(count<3 || last<=first) return 0;
    float hz=fs*(float)(count-1)/(last-first);
    return hz>=80.0f && hz<=23000.0f ? hz : 0.0f;
}
static float fit_sine(const uint16_t *p,size_t n,uint8_t ch,float fs,float hz,float *residual) {
    if(hz<80.0f || hz>fs*0.45f || (float)n*hz/fs<0.8f) return 0;
    float step=6.28318530718f*hz/fs,cs=cosf(step),sn=sinf(step),c=1.0f,s=0;
    float sc=0,ss=0,scc=0,sss=0,scs=0,sy=0,syc=0,sys=0;
    float center=p[ch];
    for(size_t i=0;i<n;i++) {
        float y=p[2*i+ch]-center;
        sc+=c;ss+=s;scc+=c*c;sss+=s*s;scs+=c*s;sy+=y;syc+=y*c;sys+=y*s;
        float nc=c*cs-s*sn;s=s*cs+c*sn;c=nc;
        if((i&63u)==63u) {float norm=1.0f/sqrtf(c*c+s*s);c*=norm;s*=norm;}
    }
    float inv=1.0f/(float)n,cc=scc-sc*sc*inv,tt=sss-ss*ss*inv,ct=scs-sc*ss*inv;
    float yc=syc-sy*sc*inv,ys=sys-sy*ss*inv,det=cc*tt-ct*ct;
    if(det<1.0f) return 0;
    float a=(yc*tt-ys*ct)/det,b=(ys*cc-yc*ct)/det,offset=(sy-a*sc-b*ss)*inv;
    float power=0;c=1;s=0;
    for(size_t i=0;i<n;i++) {
        float error=p[2*i+ch]-center-offset-a*c-b*s;power+=error*error;
        float nc=c*cs-s*sn;s=s*cs+c*sn;c=nc;
        if((i&63u)==63u) {float norm=1.0f/sqrtf(c*c+s*s);c*=norm;s*=norm;}
    }
    float amp=sqrtf(a*a+b*b);
    *residual=sqrtf(power*inv)/fmaxf(amp,1.0f);
    return 2.0f*amp;
}
void measure_frame(const uint16_t *p,size_t n,uint32_t fs,float captured_hz,float vref,
                   const Calibration *cal,int sine_mode,Measurement *m) {
    memset(m,0,sizeof(*m));m->sample_rate=fs;
    if(!p||n<8||fs==0||!cal) {m->flags=MEAS_NO_SIGNAL;return;}
    for(uint8_t ch=0;ch<2;ch++) {
        stats(p,n,ch,&m->mean[ch],&m->stddev[ch],&m->min[ch],&m->max[ch]);
        m->raw_pp[ch]=(float)(m->max[ch]-m->min[ch]);
        if(m->min[ch]<24 || m->max[ch]>4071) m->flags|=ch?MEAS_SMALL_CLIP:MEAS_MAIN_CLIP;
    }
    m->dc_mv=calibration_dc(cal,m->mean[0],vref);
    /* Use the coupled gain channel only when it is comfortably inside the rails. */
    m->channel=(!(m->flags&MEAS_SMALL_CLIP) && m->raw_pp[1]>=24.0f && m->raw_pp[1]<3000.0f)?1u:0u;
    float f_adc=estimate_frequency(p,n,m->channel,(float)fs);
    float f_capture=captured_hz*cal->frequency_trim;
    if(!isfinite(f_capture) || f_capture<80.0f || f_capture>23000.0f) f_capture=0;
    /* Oscillator trim also corrects the sample-rate based fallback. */
    f_adc*=cal->frequency_trim;
    float f=f_capture>0?f_capture:f_adc;
    if(f_capture>0 && f_adc>0 && fabsf(f_capture-f_adc)>0.10f*f_adc) f=f_adc;
    int noise=m->raw_pp[m->channel]<12.0f || m->stddev[m->channel]<3.0f;
    if(noise) {f=0;m->flags|=MEAS_NO_SIGNAL;}
    if(f>0) {m->frequency_hz=f;m->flags|=MEAS_FREQ_OK;}
    /* Fit in untrimmed timer/sample-clock coordinates so clock calibration does not alter phase. */
    float fit_frequency=f/cal->frequency_trim;
    for(uint8_t ch=0;ch<2;ch++) {
        float residual=0;
        if(sine_mode && !(m->flags&(ch?MEAS_SMALL_CLIP:MEAS_MAIN_CLIP)))
            m->fitted_pp[ch]=fit_sine(p,n,ch,(float)fs,fit_frequency,&residual);
        if(ch==m->channel) m->residual_ratio=residual;
    }
    if(sine_mode) {
        float pp=m->fitted_pp[m->channel];
        if(pp==0) pp=m->raw_pp[m->channel];
        m->vpp_mv=pp*calibration_ac_factor(cal,m->channel,fmaxf(f,100.0f),vref);
        if(m->residual_ratio>0.15f) m->flags|=MEAS_DISTORTED;
    } else {
        m->channel=0;
        m->vpp_mv=calibration_dc(cal,m->max[0],vref)-calibration_dc(cal,m->min[0],vref);
    }
    if(m->flags&MEAS_NO_SIGNAL) m->vpp_mv=0;
    if(calibration_dc(cal,m->min[0],vref)<-5100 || calibration_dc(cal,m->max[0],vref)>5100)
        m->flags|=MEAS_OVER_RANGE;
}
static int16_t sample_mv(const Calibration *c,float code,float vref) {
    return (int16_t)lroundf(clampf(calibration_dc(c,code,vref),-30000.0f,30000.0f));
}
void measure_waveform(const uint16_t *p,size_t n,const Measurement *m,const Calibration *cal,float vref,Waveform *w) {
    memset(w,0,sizeof(*w));if(!p||n<2)return;
    float physical_fs=(float)m->sample_rate*cal->frequency_trim;
    float period=m->frequency_hz>0?physical_fs/m->frequency_hz:0;
    float span=period>=2.0f?1.25f*period:(float)(n-2);
    span=clampf(span,2.0f,(float)(n-2));float start=0;
    for(size_t i=1;i<n;i++) {
        float a=p[2*(i-1)],b=p[2*i];
        if(a<m->mean[0]&&b>=m->mean[0]&&b>a) {
            float candidate=(float)(i-1)+(m->mean[0]-a)/(b-a);
            if(candidate+span<(float)(n-1)) {start=candidate;break;}
        }
    }
    w->periods=period>0?span/period:0;
    for(unsigned x=0;x<128;x++) {
        float pos=start+span*(float)x/127.0f;size_t i=(size_t)pos;
        if(i>=n-1)i=n-2;
        float code=p[2*i]+(pos-(float)i)*(p[2*(i+1)]-p[2*i]);
        w->low[x]=w->high[x]=w->center[x]=sample_mv(cal,code,vref);
        if(span/127.0f>=1.0f) {
            size_t end=(size_t)(start+span*(float)(x+1)/127.0f);if(end>=n)end=n-1;
            for(size_t j=i;j<=end;j++) {
                int16_t v=sample_mv(cal,p[2*j],vref);
                if(v<w->low[x])w->low[x]=v;
                if(v>w->high[x])w->high[x]=v;
            }
        }
    }
}
