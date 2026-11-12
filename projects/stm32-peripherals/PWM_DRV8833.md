# PWM_DRV8833 用户代码摘录

从 STM32Cube 生成的 main.c 的 USER CODE 区域摘录，仅供阅读；外设初始化、头文件和驱动未包含，不能直接编译。

## 2

```c
__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 99);
```

## WHILE

```c
while (1)
  {
		// KEY1按下：占空比99% 高速正转
    if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET)
    {
        // 启动PWM通道1输出（只能同时启动1个通道，两个通道对应正/反转）
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
        // 配置通道1的占空比，影响电机转速（占空比过低可能导致电机无法启动）
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 99);
    }
    // KEY2按下：占空比85% 中速正转
    else if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET)
    {
        // 启动PWM通道1输出（只能同时启动1个通道，两个通道对应正/反转）
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
        // 配置通道1的占空比，影响电机转速（占空比过低可能导致电机无法启动）
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 85);
    }
    else
    {
        // 停止PWM通道1输出
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
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
