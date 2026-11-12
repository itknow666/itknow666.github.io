# GPIO_KEY 用户代码摘录

从 STM32Cube 生成的 main.c 的 USER CODE 区域摘录，仅供阅读；外设初始化、头文件和驱动未包含，不能直接编译。

## PV

```c
uint8_t key2Handled = 0;
```

## WHILE

```c
while (1)
  {
		if(HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET){
    HAL_GPIO_WritePin(GREEN_GPIO_Port, GREEN_Pin, GPIO_PIN_SET);
		}
		else{
    HAL_GPIO_WritePin(GREEN_GPIO_Port, GREEN_Pin, GPIO_PIN_RESET);
		}
		
	if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET) {
    if (key2Handled == 0) {
        HAL_Delay(50);

        if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET) {
            HAL_GPIO_TogglePin(BLUE_GPIO_Port, BLUE_Pin);
            key2Handled = 1;
        }
    }
  } else {
    key2Handled = 0;
  }
		
		if (!HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin)) {
    HAL_Delay(50);
			if (!HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin)) {
					HAL_GPIO_TogglePin(RED_GPIO_Port, RED_Pin);
					while (!HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin));
			}
		}
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
