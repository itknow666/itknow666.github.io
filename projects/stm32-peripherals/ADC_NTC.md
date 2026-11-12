# ADC_NTC 用户代码摘录

从 STM32Cube 生成的 main.c 的 USER CODE 区域摘录，仅供阅读；外设初始化、头文件和驱动未包含，不能直接编译。

## Includes

```c
#include "stdio.h"
#include "string.h"
#include "math.h"
```

## 0

```c
/**
 * @brief 通过ADC值计算NTC电阻
 *
 * @param adc_value ADC原始值 [0, 4095]
 * @retval 返回NTC电阻值，浮点数类型，单位Ω
 */
float ADC2Resistance(uint32_t adc_value)
{
  return (adc_value / (4096.0f - adc_value)) * 10000.0f;
}
/**
 * @brief 通过NTC电阻反推温度
 *
 * @param R1 NTC电阻值
 * @retval 返回温度，float类型，单位摄氏度
 */
float resistance2Temperature(float R1)
{
  float B = 3950.0f;
  float R2 = 10000.0f;
  float T2 = 25.0f;
  return (1.0 / ((1.0 / B) * log(R1 / R2) + (1.0 / (T2 + 273.15))) - 273.15);
}
```

## 2

```c
// 定义所需变量
  char send_buf[50] = {0};
  uint16_t adc_result = 0;
  float ntc_resistance = 0.0f;
  float temperature = 0.0f;
  // 开始连续ADC转换
  HAL_ADC_Start(&hadc1);
  // 等待ADC稳定
  HAL_Delay(500);
```

## WHILE

```c
while (1)
  {
		// 获取ADC值
    adc_result = HAL_ADC_GetValue(&hadc1);
    // 计算NTC阻值
    ntc_resistance = ADC2Resistance(adc_result);
    // 计算温度
    temperature = resistance2Temperature(ntc_resistance);
    // 将变量打印为字符串
    sprintf(send_buf, "阻值%.1f Ω，温度: %.2f ℃\r\n", ntc_resistance, temperature);
    // 通过串口2发送
    HAL_UART_Transmit(&huart2, (uint8_t *)send_buf, strlen(send_buf), 10);
    HAL_Delay(1000);
```

## 3

```c
}
```

## Error_Handler_Debug

```c
/* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
```

## 6

```c
/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
```
