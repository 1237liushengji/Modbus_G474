/**
  ******************************************************************************
  * @file    bsp_eeprom.c
  * @brief   AT24C02 driver over I2C1 (PA15=SCL, PB9=SDA, AF4).
  *
  *  AT24C02 facts:
  *   - 256 x 8 bit, 8-byte write page, write cycle ~5 ms
  *   - device address 0xA0 (A2A1A0 = 000 on board)
  *   - no auto-increment across page boundary on write
  ******************************************************************************
  */
#include "bsp_eeprom.h"
#include "bsp_board_cfg.h"

/* add EEPROM pins to the board config header namespace */
#ifndef EEPROM_I2C
#define EEPROM_I2C              I2C1
#define EEPROM_I2C_CLK_ENABLE() __HAL_RCC_I2C1_CLK_ENABLE()
#define EEPROM_SCL_PORT         GPIOA
#define EEPROM_SCL_PIN          GPIO_PIN_15
#define EEPROM_SDA_PORT         GPIOB
#define EEPROM_SDA_PIN          GPIO_PIN_9
#define EEPROM_GPIO_CLK_ENABLE() \
    do { __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOB_CLK_ENABLE(); } while (0)
#define EEPROM_I2C_AF           GPIO_AF4_I2C1
#endif

#define EEPROM_DEV_ADDR         (0xA0U >> 1)   /* 7-bit: 0x50 */
#define EEPROM_PAGE_SIZE        8U
#define EEPROM_SIZE             256U
#define EEPROM_WRITE_CYCLE_MS   5U

static I2C_HandleTypeDef s_hi2c;

int32_t EEPROM_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    EEPROM_GPIO_CLK_ENABLE();
    EEPROM_I2C_CLK_ENABLE();

    /* SCL: open-drain AF, pull-up (board may also have external PU) */
    gpio.Pin       = EEPROM_SCL_PIN;
    gpio.Mode      = GPIO_MODE_AF_OD;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = EEPROM_I2C_AF;
    HAL_GPIO_Init(EEPROM_SCL_PORT, &gpio);

    gpio.Pin       = EEPROM_SDA_PIN;
    HAL_GPIO_Init(EEPROM_SDA_PORT, &gpio);

    s_hi2c.Instance             = EEPROM_I2C;
    /* 100 kHz standard mode timing for PCLK1 = 170 MHz (tI2CCLK = 58.8 ns,
       PRESC=9). Conservative margins -> actual SCL ~ 50 kHz, fine for 24C02.
       Layout: PRESC[31:28] SCLDEL[27:16] SDADEL[15:12] SCLH[11:8] SCLL[7:0] */
    s_hi2c.Init.Timing           = (9U << I2C_TIMINGR_PRESC_Pos) |
                                   (80U << I2C_TIMINGR_SCLDEL_Pos) |
                                   (6U << I2C_TIMINGR_SDADEL_Pos) |
                                   (170U << I2C_TIMINGR_SCLH_Pos) |
                                   (170U << I2C_TIMINGR_SCLL_Pos);
    s_hi2c.Init.OwnAddress1      = 0U;
    s_hi2c.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    s_hi2c.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    s_hi2c.Init.OwnAddress2      = 0U;
    s_hi2c.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    s_hi2c.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&s_hi2c) != HAL_OK)
    {
        return -1;
    }
    return 0;
}

static int32_t EEPROM_PollWriteDone(void)
{
    /* after each write the chip needs ~5 ms internal cycle;
       polling ACK is faster and robust, but HAL has no direct API,
       so use a small delay - perfectly fine at this rate. */
    HAL_Delay(EEPROM_WRITE_CYCLE_MS);
    return 0;
}

int32_t EEPROM_WriteBytes(uint16_t addr, const uint8_t *data, uint16_t len)
{
    uint16_t done = 0U;

    if ((addr >= EEPROM_SIZE) || ((uint32_t)addr + len > EEPROM_SIZE))
    {
        return -1;
    }

    while (done < len)
    {
        /* bytes remaining in current 8-byte page */
        uint16_t page_room = (uint16_t)(EEPROM_PAGE_SIZE - (addr % EEPROM_PAGE_SIZE));
        uint16_t chunk = len - done;
        HAL_StatusTypeDef st;

        if (chunk > page_room)
        {
            chunk = page_room;
        }

        st = HAL_I2C_Mem_Write(&s_hi2c, EEPROM_DEV_ADDR, addr,
                               I2C_MEMADD_SIZE_8BIT,
                               (uint8_t *)(data + done), chunk, 100U);
        if (st != HAL_OK)
        {
            return -1;
        }
        EEPROM_PollWriteDone();

        addr += chunk;
        done += chunk;
    }
    return 0;
}

int32_t EEPROM_ReadBytes(uint16_t addr, uint8_t *data, uint16_t len)
{
    if ((addr >= EEPROM_SIZE) || ((uint32_t)addr + len > EEPROM_SIZE))
    {
        return -1;
    }
    if (HAL_I2C_Mem_Read(&s_hi2c, EEPROM_DEV_ADDR, addr,
                         I2C_MEMADD_SIZE_8BIT, data, len, 100U) != HAL_OK)
    {
        return -1;
    }
    return 0;
}
