# PWM_BUZZER 用户代码摘录

从 STM32Cube 生成的 main.c 的 USER CODE 区域摘录，仅供阅读；外设初始化、头文件和驱动未包含，不能直接编译。

## 2

```c
HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
```

## WHILE

```c
while (1)
  {
		 // KEY1按下: 输出2kHz声波
    if (!HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin))
    {
        htim4.Instance->ARR = 500; // 2kHz = 72MHz / 72 / 500
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, htim4.Instance->ARR / 5); // 20%占空比
    }
    // KEY2按下: 输出3kHz声波
    else if (!HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin))
    {
        htim4.Instance->ARR = 334; // 3kHz = 72MHz / 72 / 334
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, htim4.Instance->ARR / 5); // 20%占空比
    }
    // 否则: 关闭声波输出
    else
    {
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 0);
    }
    HAL_Delay(100);
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
