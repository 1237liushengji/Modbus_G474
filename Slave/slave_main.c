/**
  ******************************************************************************
  * @file    slave_main.c
  * @brief   Slave node (B board) application entry.
  * @note    Compiled only when MODBUS_NODE_ROLE == NODE_ROLE_SLAVE.
  *
  *  v0.4+: runs the Modbus slave engine; LED2 (green) toggles per response.
  *         Registers are served from the MB_REG bank (MB_REG_Init).
  ******************************************************************************
  */
#include "slave_main.h"

#include "led.h"
#include "bsp_rs485.h"
#include "bsp_tick.h"
#include "bsp_board_cfg.h"
#include "modbus_slave.h"
#include "modbus_register.h"

static void RSCB_SlaveRx(uint8_t byte)
{
    MB_Slave_OnRxByte(byte, BSP_Tick_GetUs());
}

void Slave_Main(void)
{
    MB_REG_Init();
    RS485_Init(RS485_DEFAULT_BAUDRATE);
    RS485_SetRxCallback(RSCB_SlaveRx);

    MB_Slave_Init(RS485_DEFAULT_SLAVE_ID, RS485_DEFAULT_BAUDRATE);
    MB_Slave_SetTxFunc(RS485_SendFrame);

    LED1_OFF;   /* blue */
    LED2_OFF;   /* green */

    while (1)
    {
        if (MB_Slave_Poll(BSP_Tick_GetUs()) != 0U)
        {
            LED2_Toggle;   /* response sent */
        }
        HAL_Delay(1);
    }
}
