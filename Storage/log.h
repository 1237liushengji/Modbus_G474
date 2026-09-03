/**
  ******************************************************************************
  * @file    log.h
  * @brief   Ring log on W25Q128 (Storage layer).
  *
  *  Design (docs/08-Storage设计.md):
  *   - dedicated log area at the END of the 16 MB flash, LOG_AREA_SIZE bytes
  *   - fixed-size records (LOG_RECORD_SIZE), one sector holds many records
  *   - a 16-byte header at the area start keeps: magic, write index,
  *     sequence number and CRC (torn-write recovery -> fallback scan)
  *   - when full, the oldest sector is erased first (simple wear rotation)
  ******************************************************************************
  */
#ifndef __LOG_H
#define __LOG_H

#include <stdint.h>
#include "types.h"   /* log_event_t */

/* Configuration: 64 KB log area = 16 sectors, record 32 B -> 2048 recs */
#define LOG_AREA_SIZE      (64UL * 1024UL)
#define LOG_RECORD_SIZE    32U
#define LOG_SECTOR_SIZE    4096U

typedef struct {
    uint32_t seq;           /* monotonically increasing                */
    uint32_t timestamp_ms;  /* HAL_GetTick() at event time             */
    uint8_t  event;         /* log_event_t                             */
    uint8_t  param[3];      /* extra context (e.g. exception code)     */
    uint8_t  reserved[LOG_RECORD_SIZE - 12U];
} log_record_t;

/** Init log subsystem (reads header, locates write position). */
void LOG_Init(void);

/** Provide the ms tick source (e.g. HAL_GetTick). Call before LOG_Init. */
void LOG_SetTickSource(uint32_t (*tick_ms)(void));

/**
  * @brief  Append one event to the ring log (blocking flash write,
  *         call from the main loop context only, not from ISR).
  */
void LOG_WriteEvent(log_event_t ev, uint8_t p0, uint8_t p1, uint8_t p2);

/** Total number of records currently stored (approximate after wrap). */
uint32_t LOG_GetCount(void);

/** Last sequence number written. */
uint32_t LOG_GetLastSeq(void);

/**
  * @brief  Read a stored record by index (0 = oldest, count-1 = newest).
  * @retval 1 ok, 0 index out of range
  */
uint8_t LOG_Read(uint32_t index, log_record_t *rec);

#endif /* __LOG_H */
