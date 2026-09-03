/**
  ******************************************************************************
  * @file    bsp_uart.h
  * @brief   Register-level USART driver for RS485 link (USART3)
  * @note    Self-contained init (clock + GPIO + NVIC), no CubeMX dependency.
  *          RX is interrupt driven; RX byte is delivered to a callback.
  ******************************************************************************
  */
#ifndef __BSP_UART_H
#define __BSP_UART_H

#include <stdint.h>

/* RX byte callback, invoked in USART ISR context. */
typedef void (*bsp_uart_rx_cb_t)(uint8_t byte);

/**
  * @brief  Initialize USART (8N1, baud configurable).
  * @param  baudrate: e.g. 9600/19200/38400/57600/115200/230400
  * @retval 0 ok, -1 error
  */
int32_t BSP_UART_Init(uint32_t baudrate);

/**
  * @brief  Change baudrate on the fly.
  */
void BSP_UART_SetBaudrate(uint32_t baudrate);

/**
  * @brief  Register RX byte callback (only one, called in ISR).
  */
void BSP_UART_SetRxCallback(bsp_uart_rx_cb_t cb);

/**
  * @brief  Blocking transmit of len bytes (waits TXE for each byte).
  * @note   Does NOT wait TC - caller (RS485 layer) decides direction timing.
  */
void BSP_UART_SendBytes(const uint8_t *data, uint16_t len);

/**
  * @brief  Wait until last byte fully shifted out (TC flag).
  * @note   TXE != frame sent; RS485 direction switch MUST wait TC.
  */
void BSP_UART_WaitTxComplete(void);

/**
  * @brief  USART3 global ISR entry (called from vector table).
  */
void BSP_UART_IRQHandler(void);

#endif /* __BSP_UART_H */
