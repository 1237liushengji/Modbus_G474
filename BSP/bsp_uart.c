/**
  ******************************************************************************
  * @file    bsp_uart.c
  * @brief   Register-level USART3 driver for RS485 link.
  *
  *  Why register-level instead of HAL_UART_* ?
  *   - keeps the Keil project free of extra HAL source files
  *   - full control of TXE/TC/ORE handling (RS485 timing is critical)
  *   - matches the "self-init BSP" style already used by led.c
  *
  *  USART3 clock = PCLK1 (APB1, no divider in SystemClock_Config).
  ******************************************************************************
  */
#include "bsp_uart.h"
#include "bsp_board_cfg.h"

/*====================================================================*/
/* Local objects                                                       */
/*====================================================================*/
static bsp_uart_rx_cb_t s_rx_cb = 0;

/*====================================================================*/
/* Static helpers                                                      */
/*====================================================================*/
static void UART_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    RS485_GPIO_CLK_ENABLE();
    RS485_UART_CLK_ENABLE();

    /* TX: PB10 -> USART3_TX (AF7), push-pull, high speed */
    gpio.Pin       = RS485_TX_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = RS485_GPIO_AF;
    HAL_GPIO_Init(RS485_TX_GPIO_PORT, &gpio);

    /* RX: PB11 <- USART3_RX (AF7); no pull, transceiver drives the line */
    gpio.Pin   = RS485_RX_PIN;
    gpio.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(RS485_RX_GPIO_PORT, &gpio);
}

static uint32_t UART_ComputeBRR(uint32_t baudrate)
{
    uint32_t pclk;

    /* USART3 kernel clock = PCLK1; runtime queried so it stays valid
       if the CubeMX clock tree changes. BRR = fck / baud (16x over-sample,
       OVER8=0), rounded. */
    pclk = HAL_RCC_GetPCLK1Freq();
    return (pclk + (baudrate / 2U)) / baudrate;
}

/*====================================================================*/
/* Public API                                                          */
/*====================================================================*/
int32_t BSP_UART_Init(uint32_t baudrate)
{
    UART_GPIO_Init();

    /* Disable USART first while configuring */
    RS485_UART_PERIPH->CR1 = 0U;

    /* Oversampling 16 (OVER8=0), 8 data bits, no parity (default after reset) */
    RS485_UART_PERIPH->BRR = UART_ComputeBRR(baudrate);

    /* UE=1, TE=1, RE=1; 1 stop bit is default */
    RS485_UART_PERIPH->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;

    /* RXNE interrupt enable */
    RS485_UART_PERIPH->CR1 |= USART_CR1_RXNEIE;

    /* NVIC: medium priority, enable USART3 IRQ */
    HAL_NVIC_SetPriority(RS485_UART_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(RS485_UART_IRQn);

    return 0;
}

void BSP_UART_SetBaudrate(uint32_t baudrate)
{
    /* Recompute BRR; UART is kept enabled, byte in flight may be corrupted,
       caller must change baud only in idle state. */
    RS485_UART_PERIPH->BRR = UART_ComputeBRR(baudrate);
}

void BSP_UART_SetRxCallback(bsp_uart_rx_cb_t cb)
{
    s_rx_cb = cb;
}

void BSP_UART_SendBytes(const uint8_t *data, uint16_t len)
{
    /* Clear TC so the TC-wait at frame end refers to THIS frame. */
    RS485_UART_PERIPH->ICR = USART_ICR_TCCF;

    while (len > 0U)
    {
        /* Wait TXE (transmit data register empty) */
        while ((RS485_UART_PERIPH->ISR & USART_ISR_TXE_TXFNF) == 0U)
        {
        }
        RS485_UART_PERIPH->TDR = *data;
        data++;
        len--;
    }
}

void BSP_UART_WaitTxComplete(void)
{
    /* TC = last byte fully shifted out including stop bit.
       TXE alone is NOT enough before releasing the RS485 bus. */
    while ((RS485_UART_PERIPH->ISR & USART_ISR_TC) == 0U)
    {
    }
    RS485_UART_PERIPH->ICR = USART_ICR_TCCF;
}

void BSP_UART_IRQHandler(void)
{
    uint32_t isr = RS485_UART_PERIPH->ISR;

    /* Overrun error: read RDR to discard, keep receiver alive */
    if ((isr & USART_ISR_ORE) != 0U)
    {
        (void)RS485_UART_PERIPH->RDR;      /* clears ORE */
        RS485_UART_PERIPH->ICR = USART_ICR_ORECF;
    }

    if ((isr & USART_ISR_RXNE) != 0U)
    {
        uint8_t byte = (uint8_t)(RS485_UART_PERIPH->RDR & 0xFFU); /* clears RXNE */
        if (s_rx_cb != 0)
        {
            s_rx_cb(byte);
        }
    }
}

/*====================================================================*/
/* ISR symbol wired into the startup vector table                      */
/*====================================================================*/
void RS485_UART_IRQHandler(void)
{
    BSP_UART_IRQHandler();
}
