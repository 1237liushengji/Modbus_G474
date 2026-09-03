/**
  ******************************************************************************
  * @file    modbus_slave.h
  * @brief   Modbus RTU slave engine (platform independent).
  *
  *  Architecture (matches docs/04):
  *   - UART ISR feeds bytes via MB_Slave_OnRxByte(byte, now_us)
  *   - the app main loop calls MB_Slave_Poll(now_us) frequently;
  *     frame end is detected by t3.5 idle time, then CRC/addr checked
  *     and the function code is dispatched
  *   - responses are emitted through a TX function pointer set by the app
  *
  *  No HAL / BSP dependency: time is injected by the caller (us ticks).
  ******************************************************************************
  */
#ifndef __MODBUS_SLAVE_H
#define __MODBUS_SLAVE_H

#include <stdint.h>
#include "types.h"   /* comm_stats_t */

/** Response/exception transmitter (set by application, e.g. RS485_SendFrame) */
typedef void (*mb_slave_tx_func_t)(const uint8_t *data, uint16_t len);

/**
  * @brief  Initialize slave engine.
  * @param  slave_id  own Modbus address (1..247)
  * @param  baudrate  used to compute t3.5 frame timeout
  */
void MB_Slave_Init(uint8_t slave_id, uint32_t baudrate);

/** Update t3.5 after a baudrate change. */
void MB_Slave_SetBaudrate(uint32_t baudrate);

/** Change own slave id at runtime (after register write). */
void MB_Slave_SetSlaveId(uint8_t slave_id);

/** Set the function used to transmit responses. */
void MB_Slave_SetTxFunc(mb_slave_tx_func_t f);

/**
  * @brief  Feed one received byte (call from UART RX ISR context).
  * @param  byte
  * @param  now_us  current us timestamp (BSP_Tick_GetUs())
  */
void MB_Slave_OnRxByte(uint8_t byte, uint32_t now_us);

/**
  * @brief  Poll the receive state machine. Call very often from main loop.
  * @param  now_us  current us timestamp
  * @retval 1 when a response was sent during this poll, 0 otherwise
  */
uint8_t MB_Slave_Poll(uint32_t now_us);

/** Number of bytes currently buffered (debug). */
uint16_t MB_Slave_GetRxLen(void);

/** Snapshot of local comm statistics (exceptions counted too). */
void MB_Slave_GetStats(comm_stats_t *out);

#endif /* __MODBUS_SLAVE_H */
