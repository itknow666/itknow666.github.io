# UART_BLE_COMMAND 用户代码摘录

从 STM32Cube 生成的 main.c 的 USER CODE 区域摘录，仅供阅读；外设初始化、头文件和驱动未包含，不能直接编译。

## Includes

```c
#include "command.h"
```

## PV

```c
#define ReceivedDataSize 50
uint8_t receivedData[ReceivedDataSize];
```

## 0

```c
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart == &huart3)
  {
    // 将数据写入命令缓冲区
    uint8_t length = CommandBuffer_Write(receivedData, Size);
    // 继续接收
    HAL_UARTEx_ReceiveToIdle_DMA(&huart3, receivedData, ReceivedDataSize);
    __HAL_DMA_DISABLE_IT(&hdma_usart3_rx, DMA_IT_HT);
  }
}
```

## 2

```c
// 开启第一次DMA接收
  HAL_UARTEx_ReceiveToIdle_DMA(&huart3, receivedData, ReceivedDataSize);
  __HAL_DMA_DISABLE_IT(&hdma_usart3_rx, DMA_IT_HT);
```

## WHILE

```c
uint8_t command[30];
  while (1)
  {
		// 从命令缓冲区获取命令
    uint8_t length = CommandBuffer_GetCommand(command);
    if (length > 0)
    {
      HAL_UART_Transmit_DMA(&huart3, command, length);
      for (int i = 2; i < length - 1; i += 2)
      {
        GPIO_PinState state = GPIO_PIN_SET;
        // 先判断第二个字节，是开灯还是关灯，0x00关灯，0x01开灯
        if (command[i + 1] == 0x00)
        {
          state = GPIO_PIN_RESET;
        }
        // 再判断第一个字节，是红灯还是绿灯还是蓝灯
        if (command[i] == 0x01) // 红灯
        {
          HAL_GPIO_WritePin(RED_GPIO_Port, RED_Pin, state);
        }
        else if (command[i] == 0x02) // 绿灯
        {
          HAL_GPIO_WritePin(GREEN_GPIO_Port, GREEN_Pin, state);
        }
        else if (command[i] == 0x03) // 蓝灯
        {
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
