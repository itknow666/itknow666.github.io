# UART_RGB 用户代码摘录

从 STM32Cube 生成的 main.c 的 USER CODE 区域摘录，仅供阅读；外设初始化、头文件和驱动未包含，不能直接编译。

## 2

```c
uint8_t rx_data[2];
```

## WHILE

```c
while (1)
  {
		HAL_UART_Receive(&huart2, rx_data, 2, HAL_MAX_DELAY);
		// 根据接收到的数据控制 LED
		GPIO_PinState state = GPIO_PIN_SET;
		if (rx_data[1] == '0')
		{
				state = GPIO_PIN_RESET;
		}
		if (rx_data[0] == 'R')
		{
				HAL_GPIO_WritePin(RED_GPIO_Port, RED_Pin, state); 
		}
		else if (rx_data[0] == 'G')
		{
				HAL_GPIO_WritePin(GREEN_GPIO_Port, GREEN_Pin, state); 
		}
		else if (rx_data[0] == 'B')
		{
				HAL_GPIO_WritePin(BLUE_GPIO_Port, BLUE_Pin, state);
		}
		// 将收到的数据返回
		HAL_UART_Transmit(&huart2, rx_data, 2, HAL_MAX_DELAY);
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
