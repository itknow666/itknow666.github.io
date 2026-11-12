# Timer_UltraSonic_Base 用户代码摘录

从 STM32Cube 生成的 main.c 的 USER CODE 区域摘录，仅供阅读；外设初始化、头文件和驱动未包含，不能直接编译。

## Includes

```c
#include <stdio.h> // sprintf
#include <string.h> // strlen
```

## WHILE

```c
while (1)
  {
		 // 触发
    HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_SET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_RESET);
    // 清零
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    // 等待高电平
    while (HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin) == GPIO_PIN_RESET);
    // 启动TIM1
    HAL_TIM_Base_Start(&htim1);
    // 等待低电平
    while (HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin) == GPIO_PIN_SET);
    // 停止TIM1
    HAL_TIM_Base_Stop(&htim1);
    // 计算距离
    uint32_t cnt = __HAL_TIM_GET_COUNTER(&htim1);
    float distance = cnt * 0.017;
    // 发送到串口
    char buf[32];
    sprintf(buf, "Distance: %.2f cm\r\n", distance);
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), 1000);
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
