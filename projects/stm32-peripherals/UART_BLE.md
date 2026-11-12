# UART_BLE 用户代码摘录

从 STM32Cube 生成的 main.c 的 USER CODE 区域摘录，仅供阅读；外设初始化、头文件和驱动未包含，不能直接编译。

## PV

```c
// 串口接收缓冲区
uint8_t rx_data[256] = {0};
```

## 0

```c
// 不定长数据接收完成回调函数
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART3)
    {
        // 使用DMA将接收到的数据发送回去
        HAL_UART_Transmit_DMA(&huart3, rx_data, Size);
        // 重新启动接收，使用Ex函数，接收不定长数据
        HAL_UARTEx_ReceiveToIdle_DMA(&huart3, rx_data, sizeof(rx_data));
        // 关闭DMA传输过半中断（HAL库默认开启，但我们只需要接收完成中断）
        __HAL_DMA_DISABLE_IT(huart3.hdmarx, DMA_IT_HT);
    }
}
```

## 2

```c
// 使用Ex函数，接收不定长数据
	HAL_UARTEx_ReceiveToIdle_DMA(&huart3, rx_data, sizeof(rx_data));
	// 关闭DMA传输过半中断（HAL库默认开启，但我们只需要接收完成中断）
	__HAL_DMA_DISABLE_IT(huart3.hdmarx, DMA_IT_HT);
```

## WHILE

```c
while (1)
  {
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
