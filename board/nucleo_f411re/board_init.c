/*
 * board_init.c — Nucleo-F411RE 时钟/引脚初始化
 *
 * 参考 ST 官方示例的标准配置：HSI(16MHz) + PLL -> 84MHz，不依赖 HSE，
 * 拿到板子插上 USB 就能跑，不需要额外配置晶振焊盘。
 */
#include "board_config.h"

#include "ef_common.h"
#include "hal_gpio.h"
#include "hal_uart.h"

static void system_clock_config(void)
{
    RCC_OscInitTypeDef osc_init = {0};
    RCC_ClkInitTypeDef clk_init = {0};

    osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc_init.HSIState = RCC_HSI_ON;
    osc_init.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc_init.PLL.PLLState = RCC_PLL_ON;
    osc_init.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    osc_init.PLL.PLLM = 16;            /* 16MHz / 16 = 1MHz PLL 输入 */
    osc_init.PLL.PLLN = 336;           /* 1MHz * 336 = 336MHz VCO */
    osc_init.PLL.PLLP = RCC_PLLP_DIV4; /* 336MHz / 4 = 84MHz SYSCLK */
    osc_init.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&osc_init) != HAL_OK) {
        EF_ASSERT(0);
    }

    clk_init.ClockType =
        RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk_init.AHBCLKDivider = RCC_SYSCLK_DIV1; /* HCLK = 84MHz */
    clk_init.APB1CLKDivider = RCC_HCLK_DIV2;  /* PCLK1 = 42MHz（F411 APB1 上限 50MHz） */
    clk_init.APB2CLKDivider = RCC_HCLK_DIV1;  /* PCLK2 = 84MHz（F411 APB2 上限 100MHz） */
    if (HAL_RCC_ClockConfig(&clk_init, FLASH_LATENCY_2) != HAL_OK) {
        EF_ASSERT(0);
    }
}

static void console_uart_pins_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin = BOARD_CONSOLE_UART_TX_PIN | BOARD_CONSOLE_UART_RX_PIN;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = BOARD_CONSOLE_UART_AF;
    HAL_GPIO_Init(BOARD_CONSOLE_UART_TX_PORT, &gpio_init);

    HAL_NVIC_SetPriority(BOARD_CONSOLE_UART_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(BOARD_CONSOLE_UART_IRQn);
}

static void button_exti_init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();

    hal_gpio_config_t cfg = {
        .port = BOARD_BUTTON_PORT,
        .pin = BOARD_BUTTON_PIN,
        .mode = EF_GPIO_MODE_IT_FALLING, /* B1 按下拉低 */
        .pull = EF_GPIO_PULL_UP,
    };
    hal_gpio_init(&cfg);

    HAL_NVIC_SetPriority(BOARD_BUTTON_EXTI_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(BOARD_BUTTON_EXTI_IRQn);
}

void board_init(void)
{
    HAL_Init();
    system_clock_config();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    hal_gpio_config_t led_cfg = {
        .port = BOARD_LED_PORT,
        .pin = BOARD_LED_PIN,
        .mode = EF_GPIO_MODE_OUTPUT_PP,
        .pull = EF_GPIO_PULL_NONE,
    };
    hal_gpio_init(&led_cfg);

    console_uart_pins_init();
    button_exti_init();

    hal_uart_config_t uart_cfg = {
        .baudrate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 0,
    };
    hal_uart_init(BOARD_CONSOLE_UART_PORT, &uart_cfg);
}
