/**
  ******************************************************************************
  * @file    bsp_tick.c
  * @brief   DWT cycle-counter based microsecond tick.
  ******************************************************************************
  */
#include "bsp_tick.h"
#include "main.h"

static uint32_t s_cpu_mhz = 170U;

void BSP_Tick_Init(void)
{
    /* Enable DWT access (TRCENA in DEMCR), then start the cycle counter */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;

    s_cpu_mhz = (uint32_t)(SystemCoreClock / 1000000UL);
    if (s_cpu_mhz == 0U)
    {
        s_cpu_mhz = 170U;
    }
}

uint32_t BSP_Tick_GetUs(void)
{
    /* us = cycles / MHz ; use division to keep range large enough */
    return (uint32_t)(DWT->CYCCNT / s_cpu_mhz);
}
