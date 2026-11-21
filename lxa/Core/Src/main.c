/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "OLED.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RX_BUF_SIZE  32
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

uint8_t  g_run  = 1;
uint8_t  g_dir  = 0;
uint16_t g_step = 300;
uint8_t  g_led  = 0;
uint32_t g_last_tick = 0;

uint8_t  g_led_display = 0;
uint32_t g_oled_tick   = 0;

uint8_t  g_dir_key_last = 1;
uint8_t  g_run_key_last = 1;
uint32_t g_dir_tick = 0;
uint32_t g_run_tick = 0;
uint8_t  g_dir_wait = 0;
uint8_t  g_run_wait = 0;

uint8_t  g_rx_byte = 0;
uint8_t  g_rx_buf[RX_BUF_SIZE];
uint8_t  g_rx_len = 0;
uint8_t  g_pkt_ok = 0;

uint8_t  g_txt[24];

uint8_t  preset_run[4]  = {1, 1, 0, 0};
uint8_t  preset_dir[4]  = {0, 1, 0, 1};
uint16_t preset_step[4] = {300, 500, 300, 500};
uint8_t  preset_led[4]  = {0, 1, 2, 0};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void led_show(uint8_t i);
void oled_update(void);
void oled_full_update(void);
void oled_line(uint8_t line, uint8_t col, uint8_t *s);
void key_scan(void);
void app_tick(void);
void apply_packet(void);
void start_uart_receive(void);
void uart_send_str(uint8_t *s);
void uart_send_state(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void led_show(uint8_t i)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_RESET);

    if (i == 0)      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
    else if (i == 1) HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
    else if (i == 2) HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);

    if (g_led_display != i)
    {
        g_led_display = i;
        oled_update();
    }
}

void oled_line(uint8_t line, uint8_t col, uint8_t *s)
{
    uint8_t i;

    for (i = col; i <= 16; i++)
    {
        OLED_ShowChar(line, i, ' ');
    }
    OLED_ShowString(line, col, (char *)s);
}

void oled_update(void)
{
    uint8_t led_val;

    if (g_led > 2) g_led = 0;
    led_val = g_led + 1;

    if (g_run == 1) oled_line(1, 1, (uint8_t *)"STATE:RUN ");
    else            oled_line(1, 1, (uint8_t *)"STATE:STOP");

    if (g_dir == 0) oled_line(2, 1, (uint8_t *)"DIR:FWD ");
    else            oled_line(2, 1, (uint8_t *)"DIR:REV ");

    g_txt[0] = 'S';
    g_txt[1] = 'T';
    g_txt[2] = 'E';
    g_txt[3] = 'P';
    g_txt[4] = ':';
    g_txt[5] = '0' + (uint8_t)((g_step / 100) % 10);
    g_txt[6] = '0' + (uint8_t)((g_step / 10) % 10);
    g_txt[7] = '0' + (uint8_t)(g_step % 10);
    g_txt[8] = 'm';
    g_txt[9] = 's';
    g_txt[10] = ' ';
    g_txt[11] = '\0';
    oled_line(3, 1, g_txt);

    g_txt[0] = 'L';
    g_txt[1] = 'E';
    g_txt[2] = 'D';
    g_txt[3] = ':';
    g_txt[4] = '0' + led_val;
    g_txt[5] = ' ';
    g_txt[6] = '\0';
    oled_line(4, 1, g_txt);
}

void oled_full_update(void)
{
    OLED_Clear();
    oled_update();
}

void key_scan(void)
{
    uint8_t dir_now = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);
    uint8_t run_now = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5);

    if (g_dir_key_last == 1 && dir_now == 0 && g_dir_wait == 0)
    {
        g_dir_wait = 1;
        g_dir_tick = HAL_GetTick();
    }
    if (g_dir_wait == 1 && (HAL_GetTick() - g_dir_tick >= 20))
    {
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == 0)
        {
            if (g_dir == 0) g_dir = 1;
            else g_dir = 0;
            oled_full_update();
            uart_send_state();
        }
        g_dir_wait = 0;
    }
    g_dir_key_last = dir_now;

    if (g_run_key_last == 1 && run_now == 0 && g_run_wait == 0)
    {
        g_run_wait = 1;
        g_run_tick = HAL_GetTick();
    }
    if (g_run_wait == 1 && (HAL_GetTick() - g_run_tick >= 20))
    {
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == 0)
        {
            if (g_run == 1) g_run = 0;
            else g_run = 1;
            oled_full_update();
            uart_send_state();
        }
        g_run_wait = 0;
    }
    g_run_key_last = run_now;
}

void app_tick(void)
{
    uint32_t now = HAL_GetTick();

    if (g_run == 1 && (now - g_last_tick >= g_step))
    {
        g_last_tick = now;

        if (g_dir == 0)
        {
            g_led++;
            if (g_led > 2) g_led = 0;
        }
        else
        {
            if (g_led == 0) g_led = 2;
            else g_led--;
        }
    }
    led_show(g_led);
}

void start_uart_receive(void)
{
    HAL_UART_Receive_IT(&huart1, &g_rx_byte, 1);
}

void uart_send_str(uint8_t *s)
{
    HAL_UART_Transmit(&huart1, s, strlen((char *)s), 100);
}

void uart_send_state(void)
{
    g_txt[0] = 'R';
    g_txt[1] = 'U';
    g_txt[2] = 'N';
    g_txt[3] = '=';
    g_txt[4] = '0' + g_run;
    g_txt[5] = ' ';
    g_txt[6] = 'D';
    g_txt[7] = 'I';
    g_txt[8] = 'R';
    g_txt[9] = '=';
    g_txt[10] = '0' + g_dir;
    g_txt[11] = ' ';
    g_txt[12] = 'L';
    g_txt[13] = 'E';
    g_txt[14] = 'D';
    g_txt[15] = '=';
    g_txt[16] = '0' + (g_led + 1);
    g_txt[17] = '\r';
    g_txt[18] = '\n';
    g_txt[19] = '\0';
    uart_send_str(g_txt);
}

void apply_packet(void)
{
    uint8_t cmd = g_rx_buf[1];

    if (cmd >= 0x01 && cmd <= 0x04)
    {
        uint8_t idx = cmd - 1;
        g_run  = preset_run[idx];
        g_dir  = preset_dir[idx];
        g_step = preset_step[idx];
        g_led  = preset_led[idx];
        g_led_display = g_led;
        oled_full_update();
        led_show(g_led);
        uart_send_str((uint8_t *)"CMD OK\r\n");
        uart_send_state();
    }
    else
    {
        uart_send_str((uint8_t *)"CMD ERR\r\n");
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uint8_t i;
    uint8_t sum = 0;

    if (huart->Instance != USART1) return;

    if (g_rx_byte == 0xAA)
    {
        g_rx_len = 0;
    }

    if (g_rx_len < RX_BUF_SIZE)
    {
        g_rx_buf[g_rx_len] = g_rx_byte;
        g_rx_len++;
    }

    if (g_rx_len >= 6)
    {
        for (i = 0; i < 5; i++)
        {
            sum += g_rx_buf[i];
        }
        if (sum == g_rx_buf[5])
        {
            g_pkt_ok = 1;
        }
        g_rx_len = 0;
    }

    HAL_UART_Receive_IT(&huart1, &g_rx_byte, 1);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  HAL_Init();
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  MX_GPIO_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  OLED_Init();
  g_led_display = g_led;
  oled_full_update();
  led_show(g_led);
  start_uart_receive();
  uart_send_str((uint8_t *)"START\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    key_scan();
    app_tick();

    if (g_pkt_ok == 1)
    {
        g_pkt_ok = 0;
        apply_packet();
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */