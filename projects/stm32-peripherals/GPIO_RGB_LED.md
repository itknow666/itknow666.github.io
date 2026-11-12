# GPIO_RGB_LED 用户代码摘录

从 STM32Cube 生成的 main.c 的 USER CODE 区域摘录，仅供阅读；外设初始化、头文件和驱动未包含，不能直接编译。

## WHILE

```c
while (1)
  {
		HAL_GPIO_WritePin(RED_GPIO_Port, RED_Pin, GPIO_PIN_SET); //��ɫ������
	  HAL_GPIO_WritePin(BLUE_GPIO_Port, BLUE_Pin, GPIO_PIN_RESET); //��ɫ��Ϩ��
	  HAL_GPIO_WritePin(GREEN_GPIO_Port, GREEN_Pin, GPIO_PIN_RESET); //��ɫ��Ϩ��
	  HAL_Delay(200);
		HAL_GPIO_WritePin(RED_GPIO_Port, RED_Pin, GPIO_PIN_RESET); //��ɫ��Ϩ��
	  HAL_GPIO_WritePin(BLUE_GPIO_Port, BLUE_Pin, GPIO_PIN_RESET); //��ɫ��Ϩ��
	  HAL_GPIO_WritePin(GREEN_GPIO_Port, GREEN_Pin, GPIO_PIN_SET); //��ɫ������
	  HAL_Delay(200);
	  HAL_GPIO_WritePin(RED_GPIO_Port, RED_Pin, GPIO_PIN_RESET); //��ɫ��Ϩ��
	  HAL_GPIO_WritePin(BLUE_GPIO_Port, BLUE_Pin, GPIO_PIN_SET); //��ɫ������
	  HAL_GPIO_WritePin(GREEN_GPIO_Port, GREEN_Pin, GPIO_PIN_RESET); //��ɫ��Ϩ��
	  HAL_Delay(200);
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
