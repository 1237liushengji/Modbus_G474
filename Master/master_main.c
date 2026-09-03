/**
  ******************************************************************************
  * @file    master_main.c
  * @brief   Master node (A board) application entry.
  * @note    Compiled only when MODBUS_NODE_ROLE == NODE_ROLE_MASTER.
  *
  *  v0.7: periodic poll of the slave (0x03 read of 5 measurands).
  *  v0.8: write-through demo every N polls (0x06), fault LED indication
  *        and comm statistics tracking via the master engine.
  *        LED1 (blue): pulse per successful read transaction
  *        LED2 (green): fast blink while link is down (timeouts), solid
  *                       during exception storms (unused yet)
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
/* Tunables                                                            */
/*====================================================================*/
#define MASTER_SLAVE_ID      1U
#define POLL_PERIOD_MS       500U
#define MASTER_TIMEOUT_MS    100U
#define MASTER_RETRY_MAX     2U
#define WRITE_EVERY_N_POLLS  10U    /* demo: write TempLimit every 10th poll */

/*====================================================================*/
/* Local                                                                */
/*====================================================================*/
static volatile uint32_t s_poll_last_ms = 0U;
static volatile uint32_t s_fault_led_ms  = 0U;
static volatile uint32_t s_poll_count    = 0U;
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
        /* payload: [byteCount][regHi regLo ...] - blink blue */
        LED1_OFF;
        LED2_OFF;
        LED1_ON;
        s_fault_led_ms = HAL_GetTick();
    }
    else if (r->timed_out != 0U)
    {
        /* link down: green fast blink via main loop */
        LED1_OFF;
        LED2_ON;
        s_fault_led_ms = HAL_GetTick();
    }
    MB_Master_GetStats(&s_stats_disp);
}

/* execute one 0x06 demo write (also validates write path on the slave) */
static void Master_DoDemoWrite(void)
{
    static uint16_t s_demo_value = 300U;

    s_demo_value = (uint16_t)(300U + (s_poll_count % 20U) * 5U); /* 300..395 */
    MB_Master_WriteSingle(MASTER_SLAVE_ID, 0x09 /* 40010 TempLimit */,
                          s_demo_value);
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
                s_poll_count++;
                if ((s_poll_count % WRITE_EVERY_N_POLLS) == 0U)
                {
                    Master_DoDemoWrite();      /* write transaction */
                }
                else
                {
                    /* read temperature..status (5 regs) */
                    MB_Master_ReadHolding(MASTER_SLAVE_ID, 0x00, 5U);
                }
            }
        }

        if (MB_Master_Poll(BSP_Tick_GetUs()))
        {
            Master_OnFinished();
        }

        /* LED decay: blink blue = ok pulse, green fast blink = link down */
        if ((HAL_GetTick() - s_fault_led_ms) >= 100U)
        {
            LED1_OFF;
        }
        if (MB_Master_GetConsecutiveFails() >= 3U)
        {
            /* link considered down: green fast blink */
            if (((HAL_GetTick() / 120U) & 1U) != 0U)
            {
                LED2_ON;
            }
            else
            {
                LED2_OFF;
            }
        }

        HAL_Delay(1);
    }
}
