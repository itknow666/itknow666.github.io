# UART_CirBuffer 用户代码摘录

从 STM32Cube 生成的 main.c 的 USER CODE 区域摘录，仅供阅读；外设初始化、头文件和驱动未包含，不能直接编译。

## Includes

```c
#include "command.h"
```

## PD

```c
uint8_t readBuffer[10];
```

## 0

```c
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size){
	if (huart == &huart2){
		Command_Write(readBuffer, Size);
		HAL_UARTEx_ReceiveToIdle_IT(&huart2, readBuffer, sizeof(readBuffer));
	}
}
```

## 2

```c
HAL_UARTEx_ReceiveToIdle_IT(&huart2, readBuffer, sizeof(readBuffer));
	uint8_t command[50];
	int commandLength = 0;
```

## WHILE

```c
while (1)
  {
		commandLength = Command_GetCommand(command);
    if (commandLength != 0){
        HAL_UART_Transmit(&huart2, command, commandLength, HAL_MAX_DELAY);
        for (int i = 2; i < commandLength - 1; i += 2){
            GPIO_PinState state = GPIO_PIN_SET;
            if (command[i + 1] == 0x00){
                state = GPIO_PIN_RESET;
            }
            if (command[i] == 0x01){
                HAL_GPIO_WritePin(RED_GPIO_Port, RED_Pin, state);
            }else if (command[i] == 0x02){
                HAL_GPIO_WritePin(GREEN_GPIO_Port, GREEN_Pin, state);
            }else if (command[i] == 0x03){
                HAL_GPIO_WritePin(BLUE_GPIO_Port, BLUE_Pin, state);
            }
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
