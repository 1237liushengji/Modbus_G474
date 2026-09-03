/**
  ******************************************************************************
  * @file    slave_main.c
  * @brief   Slave node (B board) application entry.
  * @note    Compiled only when MODBUS_NODE_ROLE == NODE_ROLE_SLAVE.
  *
  *  Flow (v0.9):
  *   - CONFIG_Init: load parameters from 24C02 (defaults on first boot)
  *   - registers seeded from config
  *   - run Modbus slave engine; a write to 40010..40015 marks the config
  *     dirty -> persisted back to 24C02 in the main loop (auto-save)
  *   - SlaveID / baudrate register changes are applied immediately
  *
  *  LEDs: LED2 (green) toggles per response sent.
  ******************************************************************************
  */
#include "slave_main.h"

#include "led.h"
#include "bsp_rs485.h"
#include "bsp_uart.h"
#include "bsp_tick.h"
#include "bsp_board_cfg.h"
#include "bsp_eeprom.h"
#include "modbus_slave.h"
#include "modbus_register.h"
#include "config.h"
#include "types.h"

static void RSCB_SlaveRx(uint8_t byte)
{
    MB_Slave_OnRxByte(byte, BSP_Tick_GetUs());
}

/* re-apply protocol-side settings that depend on config registers */
static void Slave_ApplyConfigChange(void)
{
    config_param_t p;

    CONFIG_ReadFromRegisters(&p);
    if (CONFIG_Save(&p))
    {
        MB_REG_ClearConfigDirty();
    }

    /* live apply: slave id + baudrate */
    MB_Slave_SetSlaveId(MB_REG_GetSlaveId());
    {
        uint32_t baud = MB_REG_BaudFromIdx((uint8_t)MB_REG_GetBaudIdx());
        if (baud != RS485_DEFAULT_BAUDRATE)
        {
            BSP_UART_SetBaudrate(baud);
        }
        MB_Slave_SetBaudrate(baud);
    }
}

void Slave_Main(void)
{
    config_param_t cfg;
    uint32_t baud;
    uint8_t  slave_id;
    uint32_t last_save_check = 0U;

    MB_REG_Init();

    /* load parameters from 24C02 */
    (void)CONFIG_Init(&cfg);
    CONFIG_ApplyToRegisters(&cfg);
    MB_REG_ClearConfigDirty();

    slave_id = cfg.slave_id;
    baud     = MB_REG_BaudFromIdx(cfg.baud_idx);

    RS485_Init(baud);
    RS485_SetRxCallback(RSCB_SlaveRx);

    MB_Slave_Init(slave_id, baud);
    MB_Slave_SetTxFunc(RS485_SendFrame);

    LED1_OFF;   /* blue */
    LED2_OFF;   /* green */

    while (1)
    {
        if (MB_Slave_Poll(BSP_Tick_GetUs()) != 0U)
        {
            LED2_Toggle;   /* response sent */
        }

        /* auto-save config changed by Modbus writes (40010..40015) */
        if (MB_REG_ConfigDirty() &&
            ((HAL_GetTick() - last_save_check) >= 20U))
        {
            last_save_check = HAL_GetTick();
            Slave_ApplyConfigChange();
        }

        HAL_Delay(1);
    }
}
