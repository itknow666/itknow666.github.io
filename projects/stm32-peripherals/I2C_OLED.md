# I2C_OLED 用户代码摘录

从 STM32Cube 生成的 main.c 的 USER CODE 区域摘录，仅供阅读；外设初始化、头文件和驱动未包含，不能直接编译。

## Includes

```c
#include "string.h"
#include "oled.h"
#include "stdio.h"
```

## 2

```c
HAL_Delay(20);
	OLED_Init();
```

## WHILE

```c
while (1)
  {
		OLED_NewFrame();
    OLED_PrintString(0, 0, "感谢关注", &font16x16, OLED_COLOR_REVERSED);
    OLED_ShowFrame();
    HAL_Delay(500);
    OLED_PrintString(0, 22, "B站-KEYSKING", &font16x16, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
    HAL_Delay(500);
    OLED_PrintString(0, 44, "\\^o^/", &font16x16, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
    HAL_Delay(1500);
    // 显示变量值
    int count = 0;
    char buf[20] = {0};
    OLED_NewFrame();
    for (;;)
    {
      sprintf(buf, "%d %%", count);
      OLED_PrintASCIIString(40, 20, buf, &afont24x12, OLED_COLOR_NORMAL);
      OLED_ShowFrame();
      HAL_Delay(15);
      if (count++ > 99)
      {
        break;
      }
    }
    HAL_Delay(1000);
    // 直线绘制
    OLED_NewFrame();
    for (int i = 0; i < 128; i += 8)
    {
      OLED_DrawLine(0, 0, i, 63, OLED_COLOR_NORMAL);
      OLED_DrawLine(127, 0, 127 - i, 63, OLED_COLOR_NORMAL);
      OLED_ShowFrame();
      HAL_Delay(30);
    }
    HAL_Delay(1500);
    // 矩形绘制
    OLED_NewFrame();
    for (int i = 0; i < 64; i += 8)
    {
      OLED_DrawRectangle(i, i / 2, 127 - i * 2, 63 - i, OLED_COLOR_NORMAL);
      OLED_ShowFrame();
      HAL_Delay(35);
    }
    HAL_Delay(1500);
    // 矩形圆形
    OLED_NewFrame();
    for (int i = 63; i > 0; i -= 8)
    {
      OLED_DrawCircle(64, 32, i / 2, OLED_COLOR_NORMAL);
      OLED_ShowFrame();
      HAL_Delay(40);
    }
    HAL_Delay(1500);
    // 图片显示1
    OLED_NewFrame();
    OLED_DrawImage((128 - (bilibiliImg.w)) / 2, 0, &bilibiliImg, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
    HAL_Delay(1700);
    // 图片显示2
    OLED_NewFrame();
    OLED_DrawImage((128 - (bilibiliTVImg.w)) / 2, 0, &bilibiliTVImg, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
    HAL_Delay(1700);
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
