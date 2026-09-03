/**
  ******************************************************************************
  * @file    master_main.c
  * @brief   Master node (A board) application entry.
  * @note    Compiled only when MODBUS_NODE_ROLE == NODE_ROLE_MASTER.
  *
  *  v0.7: periodic poll of the slave (0x03 read of 5 measurands) with
  *        timeout/retry handled by the protocol engine; LED1 (blue) blinks
  *        on success, LED2 (green) on link failure.
  ******************************************************************************
  */
#include "master_main.h"

#include "led.h"
#include "bsp_rs485.h"
#include "bsp_tick.h"
#include "bsp_board_cfg.h"
#include "modbus_master.h"
#include "types.h"

/*====================================================================*/
/* Local                                                               */
/*====================================================================*/
#define MASTER_SLAVE_ID      1U
#define POLL_PERIOD_MS       500U
#define MASTER_TIMEOUT_MS    100U
#define MASTER_RETRY_MAX     2U

static volatile uint32_t s_poll_last_ms = 0U;
static comm_stats_t s_stats_disp;

static void RSCB_MasterRx(uint8_t byte)
{
    MB_Master_OnRxByte(byte, BSP_Tick_GetUs());
}

static void Master_OnFinished(void)
{
    const mb_master_result_t *r = MB_Master_GetResult();

    if (r->ok)
    {
        /* payload: [byteCount][regHi regLo ...] */
        LED1_OFF;
        LED2_OFF;
        LED1_ON;    /* data ok: blue LED pulse */
    }
    else
    {
        LED1_OFF;
        LED2_ON;    /* timeout / exception: green LED on */
    }
    MB_Master_GetStats(&s_stats_disp);
}

void Master_Main(void)
{
    RS485_Init(RS485_DEFAULT_BAUDRATE);
    RS485_SetRxCallback(RSCB_MasterRx);

    MB_Master_Init(MASTER_SLAVE_ID, RS485_DEFAULT_BAUDRATE,
                   MASTER_TIMEOUT_MS, MASTER_RETRY_MAX);
    MB_Master_SetTxFunc(RS485_SendFrame);

    s_poll_last_ms = HAL_GetTick();
    LED1_OFF;
    LED2_OFF;

    while (1)
    {
        if (!MB_Master_IsBusy())
        {
            if ((HAL_GetTick() - s_poll_last_ms) >= POLL_PERIOD_MS)
            {
                s_poll_last_ms = HAL_GetTick();
                /* read temperature/humidity/voltage/current/status = 5 regs */
                MB_Master_ReadHolding(MASTER_SLAVE_ID, 0x00, 5U);
            }
        }

        if (MB_Master_Poll(BSP_Tick_GetUs()))
        {
            Master_OnFinished();
        }

        HAL_Delay(1);
    }
}
