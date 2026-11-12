# PWM_Servo 用户代码摘录

从 STM32Cube 生成的 main.c 的 USER CODE 区域摘录，仅供阅读；外设初始化、头文件和驱动未包含，不能直接编译。

## 2

```c
HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
```

## WHILE

```c
while (1)
  {
		// 舵机控制占空比范围2.5% ~ 12.5%
    // Counter Period设置的是2000，因此占空比设置范围是50 ~ 250

    // 中点，占空比7.5%，即2000 * 7.5% = 150
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, 150);
    HAL_Delay(1000);

    // 向左转，占空比2.5%，即2000 * 2.5% = 50
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, 50);
    HAL_Delay(1000);

    // 向右转，占空比12.5%，即2000 * 12.5% = 250
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, 250);
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
