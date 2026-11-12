# SPI_Soft 用户代码摘录

从 STM32Cube 生成的 main.c 的 USER CODE 区域摘录，仅供阅读；外设初始化、头文件和驱动未包含，不能直接编译。

## Includes

```c
#include "softSPI.h"
```

## 2

```c
// 分配 SCLK 引脚
  SoftSPI1.SCLK_GPIO = SPI_SCLK_GPIO_Port;
  SoftSPI1.SCLK_Pin = SPI_SCLK_Pin;
  // 分配 MOSI 引脚
  SoftSPI1.MOSI_GPIO = SPI_MOSI_GPIO_Port;
  SoftSPI1.MOSI_Pin = SPI_MOSI_Pin;
  // 分配 MISO 引脚
  SoftSPI1.MISO_GPIO = SPI_MISO_GPIO_Port;
  SoftSPI1.MISO_Pin = SPI_MISO_Pin;
  // 分配 CS 引脚
  SoftSPI1.CS_GPIO = SPI_CS_GPIO_Port;
  SoftSPI1.CS_Pin = SPI_CS_Pin;
  // 设置 SPI 时钟频率
  SoftSPI1.Delay_Time = SPI_FREQ_10KHZ;
  // 设置 SPI 时钟极性和相位
  SoftSPI1.CPOL = 0;
  SoftSPI1.CPHA = 0;

  // 初始化 SPI
  softSPI_Init(&SoftSPI1);

  // 发送缓冲区
  uint8_t tx_buffer[4] = {0x01, 0x02, 0x03, 0x04};
  // 接收缓冲区
  uint8_t rx_buffer[4] = {0};
```

## WHILE

```c
while (1)
  {
		// 使能片选
    SoftSPI_CS_Low(&SoftSPI1);
    // 读写数据
    SoftSPI_WriteReadBuff(&SoftSPI1, tx_buffer, rx_buffer, 4);
    // 关闭片选
    SoftSPI_CS_High(&SoftSPI1);

    HAL_Delay(50);
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
