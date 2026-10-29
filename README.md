
# 电控组第二次培训作业

**刘栋**
***20263005056***

# 代码原理

在 while 循环之前，先声明变量 a 是 32 位，变量 b 是 8 位。然后进入 while （1 ）循环，用 if，通电刚开始 500 毫秒不会执行，然后往后每 500 毫秒会执行一次。

第一次到 500 毫秒，a被赋值成 500 ，然后执行下面的命令。

由于单片机执行速度极快，下面的命令执行完之后，现在的距通电时间假设还是 500 毫秒。然后就没有任务，只是在等 500 毫秒。这 500 毫秒之内，上一轮循环被点亮的灯会一直亮。

当距通电时间为 1000 毫秒时，再减掉 a 上一轮被赋值的 500 毫秒，这样刚好大于等于 500，然后再执行第二轮。

中间流水灯的逻辑是：
1. 先把三个灯全部设置为低电平，让它们全灭。
2. 第一轮 B 等于 0，先让引脚为 PA1 的流水灯变成高电平亮起来。
3. 下面 B 被复制为 1，1 取 3 的余数还是 1。
4. 第二轮 B 等于 1，三个灯全灭，把上一轮亮的那个灯弄灭，其他两个灯原本是灭的，现在还是灭的。
5. 下面 B 等于 1，引脚为 PA3 的灯再次被设置为高电平亮起来。
6. 以此类推，当 B 等于 2 时，再取 3 的余数就变成了 0，然后再次变成 PA1 的引脚开始亮。


由于 A 储存的是时间，可能数值较大，所以使用 uint32_t进行储存。B 的值在 0~2 之间不断循环，比较小，就用 uint8_t 进行储存。


核心代码片段
/* USER CODE BEGIN 2 */  
uint32_t a = 0;  
uint8_t  b     = 0;  
/* USER CODE END 2 */  
  
/* Infinite loop */  
/* USER CODE BEGIN WHILE */  
while (1)  
{  
  /* USER CODE END WHILE */  
  
  /* USER CODE BEGIN 3 */  if (HAL_GetTick() - a >= 500)  
  {  
       a= HAL_GetTick();  
  
      HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);  
      HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);  
      HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);  
  
      if      (b == 0) HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);  
      else if (b == 1) HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);  
      else                HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);  
  
      b = (b + 1) % 3;  
  }  
}  
/* USER CODE END 3 */

