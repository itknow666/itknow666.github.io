#include "board.h"
#include <string.h>

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim2,htim3;
UART_HandleTypeDef huart1;
uint16_t adc_pairs[MEAS_SAMPLES*2];
static volatile uint8_t frame_done,frame_error;
static uint8_t adc_single;
static volatile uint32_t key_events;
typedef struct { uint8_t raw,stable,debounce,long_sent; uint16_t held; } KeyState;
static KeyState keys[2];
#define PERIOD_WINDOW 32u
static volatile uint16_t periods[PERIOD_WINDOW];
static volatile uint32_t period_sum,period_count,period_index,last_capture;
static volatile uint8_t capture_primed;
static uint8_t uart_byte;
static volatile uint8_t uart_rx[256];
static volatile uint16_t uart_head,uart_tail;
static volatile uint8_t uart_overflow;
#define CAL_PAGE0 0x0800F800u
#define CAL_PAGE1 0x0800FC00u

static void check(HAL_StatusTypeDef s) { if(s!=HAL_OK)Error_Handler(); }
static void clock_init(void) {
    RCC_OscInitTypeDef o={0};RCC_ClkInitTypeDef c={0};RCC_PeriphCLKInitTypeDef p={0};
    o.OscillatorType=RCC_OSCILLATORTYPE_HSE;o.HSEState=RCC_HSE_ON;
    o.HSIState=RCC_HSI_ON;o.PLL.PLLState=RCC_PLL_ON;o.PLL.PLLSource=RCC_PLLSOURCE_HSE;
    o.PLL.PLLMUL=RCC_PLL_MUL9;check(HAL_RCC_OscConfig(&o));
    c.ClockType=RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    c.SYSCLKSource=RCC_SYSCLKSOURCE_PLLCLK;c.AHBCLKDivider=RCC_SYSCLK_DIV1;
    c.APB1CLKDivider=RCC_HCLK_DIV2;c.APB2CLKDivider=RCC_HCLK_DIV1;
    check(HAL_RCC_ClockConfig(&c,FLASH_LATENCY_2));
    p.PeriphClockSelection=RCC_PERIPHCLK_ADC;p.AdcClockSelection=RCC_ADCPCLK2_DIV6;
    check(HAL_RCCEx_PeriphCLKConfig(&p));
}
static void gpio_init(void) {
    GPIO_InitTypeDef g={0};
    __HAL_RCC_GPIOA_CLK_ENABLE();__HAL_RCC_GPIOB_CLK_ENABLE();__HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();__HAL_AFIO_REMAP_SWJ_NOJTAG();
    HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_RESET);
    g.Pin=GPIO_PIN_13;g.Mode=GPIO_MODE_OUTPUT_PP;g.Speed=GPIO_SPEED_FREQ_LOW;HAL_GPIO_Init(GPIOC,&g);
    g.Pin=GPIO_PIN_0|GPIO_PIN_1;g.Mode=GPIO_MODE_INPUT;g.Pull=GPIO_PULLUP;HAL_GPIO_Init(GPIOB,&g);
    g.Pin=GPIO_PIN_2|GPIO_PIN_3;g.Mode=GPIO_MODE_ANALOG;g.Pull=GPIO_NOPULL;HAL_GPIO_Init(GPIOA,&g);
    g.Pin=GPIO_PIN_0;g.Mode=GPIO_MODE_INPUT;g.Pull=GPIO_NOPULL;HAL_GPIO_Init(GPIOA,&g);
    g.Pin=GPIO_PIN_6|GPIO_PIN_7;g.Mode=GPIO_MODE_AF_OD;g.Speed=GPIO_SPEED_FREQ_HIGH;HAL_GPIO_Init(GPIOB,&g);
    g.Pin=GPIO_PIN_9;g.Mode=GPIO_MODE_AF_PP;HAL_GPIO_Init(GPIOA,&g);
    g.Pin=GPIO_PIN_10;g.Mode=GPIO_MODE_INPUT;g.Pull=GPIO_PULLUP;HAL_GPIO_Init(GPIOA,&g);
}
static void adc_init(void) {
    __HAL_RCC_ADC1_CLK_ENABLE();__HAL_RCC_DMA1_CLK_ENABLE();
    hdma_adc1.Instance=DMA1_Channel1;hdma_adc1.Init.Direction=DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc=DMA_PINC_DISABLE;hdma_adc1.Init.MemInc=DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment=DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment=DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode=DMA_NORMAL;hdma_adc1.Init.Priority=DMA_PRIORITY_HIGH;
    check(HAL_DMA_Init(&hdma_adc1));__HAL_LINKDMA(&hadc1,DMA_Handle,hdma_adc1);
    hadc1.Instance=ADC1;hadc1.Init.ScanConvMode=ADC_SCAN_ENABLE;
    hadc1.Init.ContinuousConvMode=DISABLE;hadc1.Init.DiscontinuousConvMode=DISABLE;
    hadc1.Init.ExternalTrigConv=ADC_EXTERNALTRIGCONV_T3_TRGO;
    hadc1.Init.DataAlign=ADC_DATAALIGN_RIGHT;hadc1.Init.NbrOfConversion=2;
    check(HAL_ADC_Init(&hadc1));
    ADC_ChannelConfTypeDef a={0};a.Channel=ADC_CHANNEL_3;a.Rank=ADC_REGULAR_RANK_1;
    a.SamplingTime=ADC_SAMPLETIME_13CYCLES_5;check(HAL_ADC_ConfigChannel(&hadc1,&a));
    a.Channel=ADC_CHANNEL_2;a.Rank=ADC_REGULAR_RANK_2;check(HAL_ADC_ConfigChannel(&hadc1,&a));
    ADC_InjectionConfTypeDef inj={0};inj.InjectedChannel=ADC_CHANNEL_VREFINT;
    inj.InjectedRank=ADC_INJECTED_RANK_1;inj.InjectedNbrOfConversion=1;
    inj.InjectedSamplingTime=ADC_SAMPLETIME_239CYCLES_5;
    inj.ExternalTrigInjecConv=ADC_INJECTED_SOFTWARE_START;
    inj.AutoInjectedConv=DISABLE;inj.InjectedDiscontinuousConvMode=DISABLE;
    check(HAL_ADCEx_InjectedConfigChannel(&hadc1,&inj));
    check(HAL_ADCEx_Calibration_Start(&hadc1));
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn,0,0);HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}
static void timers_init(void) {
    __HAL_RCC_TIM2_CLK_ENABLE();__HAL_RCC_TIM3_CLK_ENABLE();
    htim3.Instance=TIM3;htim3.Init.Prescaler=0;htim3.Init.CounterMode=TIM_COUNTERMODE_UP;
    htim3.Init.Period=359;htim3.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload=TIM_AUTORELOAD_PRELOAD_DISABLE;check(HAL_TIM_Base_Init(&htim3));
    TIM_MasterConfigTypeDef master={0};master.MasterOutputTrigger=TIM_TRGO_UPDATE;
    master.MasterSlaveMode=TIM_MASTERSLAVEMODE_DISABLE;check(HAL_TIMEx_MasterConfigSynchronization(&htim3,&master));
    htim2.Instance=TIM2;htim2.Init.Prescaler=71;htim2.Init.Period=65535;
    htim2.Init.CounterMode=TIM_COUNTERMODE_UP;htim2.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload=TIM_AUTORELOAD_PRELOAD_DISABLE;check(HAL_TIM_IC_Init(&htim2));
    TIM_IC_InitTypeDef ic={0};ic.ICPolarity=TIM_INPUTCHANNELPOLARITY_RISING;
    ic.ICSelection=TIM_ICSELECTION_DIRECTTI;ic.ICPrescaler=TIM_ICPSC_DIV1;ic.ICFilter=3;
    check(HAL_TIM_IC_ConfigChannel(&htim2,&ic,TIM_CHANNEL_1));
    TIM_SlaveConfigTypeDef slave={0};slave.SlaveMode=TIM_SLAVEMODE_RESET;
    slave.InputTrigger=TIM_TS_TI1FP1;slave.TriggerPolarity=TIM_TRIGGERPOLARITY_RISING;
    slave.TriggerPrescaler=TIM_TRIGGERPRESCALER_DIV1;slave.TriggerFilter=3;
    check(HAL_TIM_SlaveConfigSynchro(&htim2,&slave));
    HAL_NVIC_SetPriority(TIM2_IRQn,1,0);HAL_NVIC_EnableIRQ(TIM2_IRQn);
    check(HAL_TIM_IC_Start_IT(&htim2,TIM_CHANNEL_1));
}
void board_init(void) {
    HAL_Init();gpio_init();clock_init();
    adc_init();timers_init();
    __HAL_RCC_I2C1_CLK_ENABLE();hi2c1.Instance=I2C1;hi2c1.Init.ClockSpeed=400000;
    hi2c1.Init.DutyCycle=I2C_DUTYCYCLE_2;hi2c1.Init.OwnAddress1=0;
    hi2c1.Init.AddressingMode=I2C_ADDRESSINGMODE_7BIT;hi2c1.Init.DualAddressMode=I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode=I2C_GENERALCALL_DISABLE;hi2c1.Init.NoStretchMode=I2C_NOSTRETCH_DISABLE;
    check(HAL_I2C_Init(&hi2c1));
    __HAL_RCC_USART1_CLK_ENABLE();huart1.Instance=USART1;huart1.Init.BaudRate=115200;
    huart1.Init.WordLength=UART_WORDLENGTH_8B;huart1.Init.StopBits=UART_STOPBITS_1;
    huart1.Init.Parity=UART_PARITY_NONE;huart1.Init.Mode=UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl=UART_HWCONTROL_NONE;huart1.Init.OverSampling=UART_OVERSAMPLING_16;
    check(HAL_UART_Init(&huart1));HAL_NVIC_SetPriority(USART1_IRQn,2,0);HAL_NVIC_EnableIRQ(USART1_IRQn);
    check(HAL_UART_Receive_IT(&huart1,&uart_byte,1));
}
void board_tick_1ms(void) {
    for(unsigned i=0;i<2;i++) {
        uint8_t raw=HAL_GPIO_ReadPin(GPIOB,i?GPIO_PIN_1:GPIO_PIN_0)==GPIO_PIN_RESET;
        KeyState *k=&keys[i];
        if(raw!=k->raw) {k->raw=raw;k->debounce=0;}
        else if(k->debounce<20)k->debounce++;
        if(k->debounce==20 && raw!=k->stable) {
            k->stable=raw;
            if(raw) {k->held=0;k->long_sent=0;}
            else if(!k->long_sent) key_events|=i?KEY_HOLD_SHORT:KEY_MODE_SHORT;
        }
        if(k->stable && k->held<1500) {
            if(++k->held==1500) {k->long_sent=1;key_events|=i?KEY_HOLD_LONG:KEY_MODE_LONG;}
        }
    }
}
uint32_t board_keys(void) {
    uint32_t primask=__get_PRIMASK();__disable_irq();uint32_t e=key_events;key_events=0;
    __set_PRIMASK(primask);return e;
}
void board_acquire_stop(void) {
    __HAL_TIM_DISABLE(&htim3);(void)HAL_ADC_Stop_DMA(&hadc1);
    HAL_NVIC_ClearPendingIRQ(DMA1_Channel1_IRQn);frame_done=frame_error=0;
}
HAL_StatusTypeDef board_acquire(uint32_t fs) {
    if(fs!=20000u && fs!=200000u && fs!=400000u) return HAL_ERROR;
    board_acquire_stop();
    uint8_t single=fs==400000u;
    if(single!=adc_single) {
        hadc1.Init.ScanConvMode=single?ADC_SCAN_DISABLE:ADC_SCAN_ENABLE;
        hadc1.Init.NbrOfConversion=single?1u:2u;
        if(HAL_ADC_Init(&hadc1)!=HAL_OK)return HAL_ERROR;
        adc_single=single;
    }
    __HAL_TIM_SET_AUTORELOAD(&htim3,72000000u/fs-1u);__HAL_TIM_SET_COUNTER(&htim3,0);
    /* Reload PSC/ARR before enabling ADC EXTTRIG; UG itself is a TRGO event. */
    if(HAL_TIM_GenerateEvent(&htim3,TIM_EVENTSOURCE_UPDATE)!=HAL_OK)return HAL_ERROR;
    __HAL_TIM_CLEAR_FLAG(&htim3,TIM_FLAG_UPDATE);
    HAL_StatusTypeDef s=HAL_ADC_Start_DMA(&hadc1,(uint32_t *)adc_pairs,adc_single?MEAS_SAMPLES:MEAS_SAMPLES*2u);
    if(s!=HAL_OK)return s;
    __HAL_DMA_DISABLE_IT(&hdma_adc1,DMA_IT_HT);
    return HAL_TIM_Base_Start(&htim3);
}
int board_frame_ready(void) { return frame_done!=0; }
void board_normalize_frame(void) {
    /* Wave mode samples PA3 alone at 400 kS/s; expand backwards after DMA has stopped. */
    if(adc_single) for(size_t i=MEAS_SAMPLES;i--;) {
        adc_pairs[2*i]=adc_pairs[i];adc_pairs[2*i+1]=2047;
    }
}
int board_acquisition_error(void) { return frame_error!=0; }
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *h) {
    if(h->Instance==ADC1) {__HAL_TIM_DISABLE(&htim3);__DMB();frame_done=1;}
}
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *h) {
    if(h->Instance==ADC1) {__HAL_TIM_DISABLE(&htim3);frame_error=1;}
}
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *h) {
    if(h->Instance!=TIM2 || h->Channel!=HAL_TIM_ACTIVE_CHANNEL_1)return;
    uint32_t p=HAL_TIM_ReadCapturedValue(h,TIM_CHANNEL_1),now=HAL_GetTick();
    if(!capture_primed || now-last_capture>35u) {
        period_count=period_sum=period_index=0;capture_primed=1;last_capture=now;return;
    }
    last_capture=now;
    if(p<43u || p>12500u) {period_count=period_sum=period_index=0;return;}
    if(period_count==PERIOD_WINDOW)period_sum-=periods[period_index];else period_count++;
    periods[period_index]=(uint16_t)p;period_sum+=p;period_index=(period_index+1u)%PERIOD_WINDOW;
}
void board_frequency_reset(void) {
    uint32_t mask=__get_PRIMASK();__disable_irq();period_count=period_sum=period_index=0;capture_primed=0;
    __HAL_TIM_SET_COUNTER(&htim2,0);__HAL_TIM_CLEAR_FLAG(&htim2,TIM_FLAG_CC1|TIM_FLAG_CC1OF);
    __set_PRIMASK(mask);
}
float board_frequency(void) {
    uint32_t mask=__get_PRIMASK();__disable_irq();uint32_t n=period_count,sum=period_sum,last=last_capture;
    __set_PRIMASK(mask);
    if(n<2 || !sum || HAL_GetTick()-last>35u)return 0;
    return 1000000.0f*(float)n/(float)sum;
}
float board_vref_read(void) {
    board_acquire_stop();HAL_Delay(1);uint32_t sum=0;
    for(unsigned i=0;i<16;i++) {
        if(HAL_ADCEx_InjectedStart(&hadc1)!=HAL_OK)return 0;
        if(HAL_ADCEx_InjectedPollForConversion(&hadc1,5)!=HAL_OK) {
            (void)HAL_ADCEx_InjectedStop(&hadc1);return 0;
        }
        sum+=HAL_ADCEx_InjectedGetValue(&hadc1,ADC_INJECTED_RANK_1);
        (void)HAL_ADCEx_InjectedStop(&hadc1);
    }
    return (float)sum/16.0f;
}
static const Calibration *active_cal(void) {
    const Calibration *a=(const Calibration *)CAL_PAGE0,*b=(const Calibration *)CAL_PAGE1;
    return calibration_select(a,b);
}
int board_cal_load(Calibration *c) {
    const Calibration *a=active_cal();if(!a) {calibration_defaults(c);return 0;}
    memcpy(c,a,sizeof(*c));return 1;
}
int board_cal_save(Calibration *c) {
    Calibration next=*c;const Calibration *old=active_cal();
    next.generation=old?old->generation+1u:1u;next.crc=calibration_crc(&next);
    if(!calibration_valid(&next) || sizeof(next)>1024u)return 0;
    uint32_t address=old==(const Calibration *)CAL_PAGE0?CAL_PAGE1:CAL_PAGE0;
    board_acquire_stop();(void)HAL_TIM_IC_Stop_IT(&htim2,TIM_CHANNEL_1);
    FLASH_EraseInitTypeDef e={0};e.TypeErase=FLASH_TYPEERASE_PAGES;e.PageAddress=address;e.NbPages=1;
    uint32_t error=0;HAL_StatusTypeDef s=HAL_FLASH_Unlock();
    if(s==HAL_OK)s=HAL_FLASHEx_Erase(&e,&error);
    /* Commit the magic word last. The previous page remains valid if power is lost. */
    for(size_t i=4;s==HAL_OK && i<sizeof(next);i+=2) {
        uint16_t v;memcpy(&v,(const uint8_t *)&next+i,2);s=HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,address+(uint32_t)i,v);
    }
    if(s==HAL_OK)s=HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,address,(uint16_t)next.magic);
    if(s==HAL_OK)s=HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,address+2u,(uint16_t)(next.magic>>16));
    (void)HAL_FLASH_Lock();board_frequency_reset();(void)HAL_TIM_IC_Start_IT(&htim2,TIM_CHANNEL_1);
    if(s!=HAL_OK || !calibration_valid((const Calibration *)address))return 0;
    *c=next;return 1;
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *u) {
    if(u->Instance!=USART1)return;
    uint16_t next=(uart_head+1u)&255u;
    if(next!=uart_tail) {uart_rx[uart_head]=uart_byte;uart_head=next;}else uart_overflow=1;
    (void)HAL_UART_Receive_IT(u,&uart_byte,1);
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *u) {
    if(u->Instance==USART1) {uart_overflow=1;(void)HAL_UART_Receive_IT(u,&uart_byte,1);}
}
int board_uart_getchar(void) {
    if(uart_overflow) {
        uint32_t mask=__get_PRIMASK();__disable_irq();uart_tail=uart_head;uart_overflow=0;__set_PRIMASK(mask);return -2;
    }
    if(uart_tail==uart_head)return -1;
    uint8_t b=uart_rx[uart_tail];uart_tail=(uart_tail+1u)&255u;return b;
}
void board_uart_puts(const char *s) {
    size_t len=strlen(s);if(len<65536u)(void)HAL_UART_Transmit(&huart1,(uint8_t *)s,(uint16_t)len,100);
}
void Error_Handler(void) {
    if(htim3.Instance)__HAL_TIM_DISABLE(&htim3);
    for(;;) {HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);HAL_Delay(150);}
}
