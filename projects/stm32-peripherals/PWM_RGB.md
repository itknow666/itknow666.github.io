# PWM_RGB 用户代码摘录

从 STM32Cube 生成的 main.c 的 USER CODE 区域摘录，仅供阅读；外设初始化、头文件和驱动未包含，不能直接编译。

## 2

```c
HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
```

## WHILE

```c
while (1)
  {
		 // PWM通道CH1-3分别对应三个颜色，下面示例三个颜色一起呼吸灯
    // 0-99为占空比，0为最小亮度，99为最大亮度
    // 每7ms调整一次占空比，从0逐渐增加到99
    for (int period = 0; period < 100; period++) {
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, period);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, period);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, period);
        HAL_Delay(7);
    }
    // 从99逐渐减小到0
    for (int period = 99; period >= 0; period--) {
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, period);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, period);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, period);
        HAL_Delay(7);
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
