//******************************************************************************
// @file      : si5351.c
// @brief     : Si5351 Control
//******************************************************************************
// @attention
//
// Copyright (c) 2024 MultiTracks.com, LLC.
// All rights reserved.
//
//******************************************************************************

//******************************************************************************
// Includes
//******************************************************************************
#include "si5351.h"
#include <linux/delay.h> 
#include <stdio.h>
#include <common.h>
#include <dm.h>
#include <i2c.h>
#include <dm/uclass.h>

//******************************************************************************
// Private defines
//******************************************************************************
#define SI5351_DEVICE               1
#define SI5351_I2C_ADDRESS          0x60

// status register
#define SI5351_DEVICE_STATUS        0

// control registers
#define SI5351_OUTPUT_ENABLE        3
#define SI5351_OEB_PIN_ENABLE       9
#define SI5351_PLL_INPUT_SOURCE     15
#define SI5351_CLK0_CTRL            16
#define SI5351_CLK1_CTRL            17
#define SI5351_CLK2_CTRL            18
#define SI5351_CLK3_CTRL            19
#define SI5351_CLK4_CTRL            20
#define SI5351_CLK5_CTRL            21
#define SI5351_CLK6_CTRL            22
#define SI5351_CLK7_CTRL            23
#define SI5351_CLK3_0_DISABLE_STATE 24
#define SI5351_CLK7_4_DISABLE_STATE 25

// multisynth NA parameter registers
#define SI5351_MSNA_P1_17_16        28
#define SI5351_MSNA_P1_15_8         29
#define SI5351_MSNA_P1_7_0          30
#define SI5351_MSNA_P2_19_16        31
#define SI5351_MSNA_P2_15_8         32
#define SI5351_MSNA_P2_7_0          33
#define SI5351_MSNA_P3_19_16        31
#define SI5351_MSNA_P3_15_8         26
#define SI5351_MSNA_P3_7_0          27

// multisynth NB parameter registers
#define SI5351_MSNB_P1_17_16        36
#define SI5351_MSNB_P1_15_8         37
#define SI5351_MSNB_P1_7_0          38
#define SI5351_MSNB_P2_19_16        39
#define SI5351_MSNB_P2_15_8         40
#define SI5351_MSNB_P2_7_0          41
#define SI5351_MSNB_P3_19_16        39
#define SI5351_MSNB_P3_15_8         34
#define SI5351_MSNB_P3_7_0          35

// multisynth 0 paramater registers
#define SI5351_MS0_P1_17_16         44
#define SI5351_MS0_P1_15_8          45
#define SI5351_MS0_P1_7_0           46

#define SI5351_MS0_P2_19_16         47
#define SI5351_MS0_P2_15_8          48
#define SI5351_MS0_P2_7_0           49

#define SI5351_MS0_P3_19_16         47
#define SI5351_MS0_P3_15_8          42
#define SI5351_MS0_P3_7_0           43

#define SI5351_R0                   44
#define SI5351_MS0_DIVBY4           44

// multisynth 1-7 parameter register offset from multisynth 0 parameter register
#define SI5351_MS1_OFFSET           8
#define SI5351_MS2_OFFSET           16
#define SI5351_MS3_OFFSET           24
#define SI5351_MS4_OFFSET           32

// vcxo
#define SI5351_VCXO_21_16           164
#define SI5351_VCXO_15_8            163
#define SI5351_VCXO_7_0             162

// reset
#define SI5351_PLL_RESET            177

// crystal internal load capacitance
#define SI5351_XTAL_LOAD_CAP        183

// fanout enable
#define SI5351_FANOUT_ENABLE        187

// device status mask
#define SI5351_SYS_INIT_MASK        0x80

// output enable masks
#define SI5351_CLK0_OEB_MASK        0x01
#define SI5351_CLK1_OEB_MASK        0x02
#define SI5351_CLK2_OEB_MASK        0x04
#define SI5351_CLK3_OEB_MASK        0x08
#define SI5351_CLK4_OEB_MASK        0x10
#define SI5351_CLK5_OEB_MASK        0x20
#define SI5351_CLK6_OEB_MASK        0x40
#define SI5351_CLK7_OEB_MASK        0x80

// PLL input source masks
#define SI5351_PLLA_SRC_MASK        0x04
#define SI5351_PLLB_SRC_MASK        0x08

// clock control masks
#define SI5351_CLKX_PDN_MASK        0x80
#define SI5351_MSX_INT_MASK         0x40
#define SI5351_MSX_SRC_MASK         0x20
#define SI5351_CLKX_INV_MASK        0x10
#define SI5351_CLKX_SRC_MASK        0xA0
#define SI5351_CLKX_IRDV_MASK       0x03

// multisynth masks
#define SI5351_MSNX_P1_MASK         0x03
#define SI5351_MSNX_P2_MASK         0x0F
#define SI5351_MSNX_P3_MASK         0xF0
#define SI5351_MSX_P1_MASK          0x03
#define SI5351_MSX_P2_MASK          0x0F
#define SI5351_MSX_P3_MASK          0xF0
#define SI5351_MSX_RX_DIV_MASK      0x70
#define SI5351_MSX_DIVBY4_MASK      0x0C

//******************************************************************************
// Private typedefs
//******************************************************************************

//******************************************************************************
// Private macros
//******************************************************************************

//******************************************************************************
// Private variables
//******************************************************************************

//******************************************************************************
// Private function prototypes
//******************************************************************************
static bool SI5351_ReadRegister(struct udevice *dev, uint8_t reg, uint8_t *value);
static bool SI5351_WriteRegister(struct udevice *dev, uint8_t reg, uint8_t value);
static bool SI5351_WriteRegisterWithMask(struct udevice *dev, uint8_t reg, uint8_t mask, uint8_t value);

//******************************************************************************
// Private user code
//******************************************************************************

//******************************************************************************
// @brief  Si5351 Init
// @param  None
// @retval True if the Si5351 was initialized, false otherwise
//******************************************************************************
bool SI5351_Init(void)
{
    int ret;
    uint8_t value;
    struct udevice *bus, *dev;

    ret = uclass_get_device_by_seq(UCLASS_I2C, SI5351_DEVICE, &bus);
    if (ret)
    {
        printf("Failed to get I2C bus: %d\n", ret);
        return false;
    }

    ret = dm_i2c_probe(bus, SI5351_I2C_ADDRESS, SI5351_DEVICE, &dev);
    if (ret)
    {
        printf("Failed to probe I2C device: %d\n", ret);
        return false;
    }

    // read device status register
    if (SI5351_ReadRegister(dev, SI5351_DEVICE_STATUS, &value) == false)
    {
        // error
        return false;
    }

    // wait for Si5351 to be ready
    while (value & SI5351_SYS_INIT_MASK)
    {
        // device not ready, wait 100ms before checking again
        mdelay(100);

        // read device status register
        if (SI5351_ReadRegister(dev, SI5351_DEVICE_STATUS, &value) == false)
        {
            // error
            return false;
        }
    }

    // configure Output Enable and OEB pin enable control (OEB enabled but OEB pin pulled low)
    SI5351_WriteRegister(dev, SI5351_OUTPUT_ENABLE, 0xFF);
    SI5351_WriteRegister(dev, SI5351_OEB_PIN_ENABLE, 0x00);

    // configure PLL input source (CLKIN_DIV = 00b, PLLA_SRC = XTAL, PLLB_SRC = XTAL)
    SI5351_WriteRegister(dev, SI5351_PLL_INPUT_SOURCE, 0x00);

    // configure all clock controls
    SI5351_WriteRegister(dev, SI5351_CLK0_CTRL, 0x3F); // dante SCLK
    SI5351_WriteRegister(dev, SI5351_CLK1_CTRL, 0x2F); // dante LRCLK
    SI5351_WriteRegister(dev, SI5351_CLK2_CTRL, 0x2F); // dante MCLK
    SI5351_WriteRegister(dev, SI5351_CLK3_CTRL, 0x2F); // dante FBCLK
    SI5351_WriteRegister(dev, SI5351_CLK4_CTRL, 0x0F); // ethernet
    SI5351_WriteRegister(dev, SI5351_CLK5_CTRL, 0x8C); // disabled
    SI5351_WriteRegister(dev, SI5351_CLK6_CTRL, 0x8C); // disabled
    SI5351_WriteRegister(dev, SI5351_CLK7_CTRL, 0x8C); // disabled

    // configure clock disable state (CLK3-CLK0 LOW)
    SI5351_WriteRegister(dev, SI5351_CLK3_0_DISABLE_STATE, 0x00);
    // configure clock disable state (CLK7-CLK4 LOW)
    SI5351_WriteRegister(dev, SI5351_CLK7_4_DISABLE_STATE, 0xA8);

    // f(vco) = f(xtal) * (a + b/c)
    // f(vco) = (f(CLKIN) / CLKIN_DIV) * (a + b/c)

    // configure Multisynth NA P1 parameters = (128 * a) + Floor(128 * b/c) - 512
    SI5351_WriteRegister(dev, SI5351_MSNA_P1_17_16, 0x00);
    SI5351_WriteRegister(dev, SI5351_MSNA_P1_15_8, 0x0E);
    SI5351_WriteRegister(dev, SI5351_MSNA_P1_7_0, 0xAA);
    // configure Multisynth NA P2 parameters = (128 * b) - (c * Floor(128 * b/c))
    SI5351_WriteRegisterWithMask(dev, SI5351_MSNA_P2_19_16, SI5351_MSX_P2_MASK, 0x00);
    SI5351_WriteRegister(dev, SI5351_MSNA_P2_15_8, 0x00);
    SI5351_WriteRegister(dev, SI5351_MSNA_P2_7_0, 0x02);
    // configure Multisynth NA P3 parameters = c
    SI5351_WriteRegisterWithMask(dev, SI5351_MSNA_P3_19_16, SI5351_MSX_P3_MASK, 0x00);
    SI5351_WriteRegister(dev, SI5351_MSNA_P3_15_8, 0x00);
    SI5351_WriteRegister(dev, SI5351_MSNA_P3_7_0, 0x03);

    // configure Multisynth NB P1 parameters = (128 * a) + Floor(128 * b/c) - 512
    SI5351_WriteRegister(dev, SI5351_MSNB_P1_17_16, 0x00);
    SI5351_WriteRegister(dev, SI5351_MSNB_P1_15_8, 0x0E);
    SI5351_WriteRegister(dev, SI5351_MSNB_P1_7_0, 0x9C);
    // configure Multisynth NB P2 parameters = (128 * b) - (c * Floor(128 * b/c))
    SI5351_WriteRegisterWithMask(dev, SI5351_MSNB_P2_19_16, SI5351_MSX_P2_MASK, 0x08);
    SI5351_WriteRegister(dev, SI5351_MSNB_P2_15_8, 0x84);
    SI5351_WriteRegister(dev, SI5351_MSNB_P2_7_0, 0xD4);
    // configure Multisynth NB P3 parameters = c
    SI5351_WriteRegisterWithMask(dev, SI5351_MSNB_P3_19_16, SI5351_MSX_P3_MASK, 0xF0);
    SI5351_WriteRegister(dev, SI5351_MSNB_P3_15_8, 0x42);
    SI5351_WriteRegister(dev, SI5351_MSNB_P3_7_0, 0xBD);

    //******************************************************************************
    // MS0 - Multisynth 0 - SCLK = 12.288MHz = LRCLK * N * B = 48KHz * 8 * 32
    //******************************************************************************
    // configure Multisynth 0 P1 parameters = (128 * a) + Floor(128 * b/c) - 512
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_P1_17_16, SI5351_MSX_P1_MASK, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P1_15_8, 0x22);
    SI5351_WriteRegister(dev, SI5351_MS0_P1_7_0, 0x80);
    // configure Multisynth 0 P2 parameters = (128 * b) - (c * Floor(128 * b/c))
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_P2_19_16, SI5351_MSX_P2_MASK, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P2_15_8, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P2_7_0, 0x00);
    // configure Multisynth 0 P3 parameters = c
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_P3_19_16, SI5351_MSX_P3_MASK, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P3_15_8, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P3_7_0, 0x01);
    // configure Multisynth 0 parameters
    SI5351_WriteRegisterWithMask(dev, SI5351_R0, SI5351_MSX_RX_DIV_MASK, 0x00);
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_DIVBY4, SI5351_MSX_DIVBY4_MASK, 0x00);

    //******************************************************************************
    // MS1 - Multisynth 1 - LRCLK = 48KHz
    //******************************************************************************
    // configure Multisynth 1 P1 parameters = (128 * a) + Floor(128 * b/c) - 512
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_P1_17_16 + SI5351_MS1_OFFSET, SI5351_MSX_P1_MASK, 0x02);
    SI5351_WriteRegister(dev, SI5351_MS0_P1_15_8 + SI5351_MS1_OFFSET, 0x46);
    SI5351_WriteRegister(dev, SI5351_MS0_P1_7_0 + SI5351_MS1_OFFSET, 0x00);
    // configure Multisynth 1 P2 parameters = (128 * b) - (c * Floor(128 * b/c))
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_P2_19_16 + SI5351_MS1_OFFSET, SI5351_MSX_P2_MASK, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P2_15_8 + SI5351_MS1_OFFSET, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P2_7_0 + SI5351_MS1_OFFSET, 0x00);
    // configure Multisynth 1 P3 parameters = c
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_P3_19_16 + SI5351_MS1_OFFSET, SI5351_MSX_P3_MASK, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P3_15_8 + SI5351_MS1_OFFSET, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P3_7_0 + SI5351_MS1_OFFSET, 0x01);
    // configure Multisynth 1 parameters
    SI5351_WriteRegisterWithMask(dev, SI5351_R0 + SI5351_MS1_OFFSET, SI5351_MSX_RX_DIV_MASK, 0x40);
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_DIVBY4 + SI5351_MS1_OFFSET, SI5351_MSX_DIVBY4_MASK, 0x00);

    //******************************************************************************
    // MS2 - Multisynth 2 - MCLK = 24.576MHz
    //******************************************************************************
    // configure Multisynth 2 P1 parameters = (128 * a) + Floor(128 * b/c) - 512
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_P1_17_16 + SI5351_MS2_OFFSET, SI5351_MSX_P1_MASK, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P1_15_8 + SI5351_MS2_OFFSET, 0x10);
    SI5351_WriteRegister(dev, SI5351_MS0_P1_7_0 + SI5351_MS2_OFFSET, 0x40);
    // configure Multisynth 2 P2 parameters = (128 * b) - (c * Floor(128 * b/c))
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_P2_19_16 + SI5351_MS2_OFFSET, SI5351_MSX_P2_MASK, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P2_15_8 + SI5351_MS2_OFFSET, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P2_7_0 + SI5351_MS2_OFFSET, 0x00);
    // configure Multisynth 2 P3 parameters = c
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_P3_19_16 + SI5351_MS2_OFFSET, SI5351_MSX_P3_MASK, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P3_15_8 + SI5351_MS2_OFFSET, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P3_7_0 + SI5351_MS2_OFFSET, 0x02);
    // configure Multisynth 2 parameters
    SI5351_WriteRegisterWithMask(dev, SI5351_R0 + SI5351_MS2_OFFSET, SI5351_MSX_RX_DIV_MASK, 0x00);
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_DIVBY4 + SI5351_MS2_OFFSET, SI5351_MSX_DIVBY4_MASK, 0x00);

    //******************************************************************************
    // MS3 - Multisynth 3 - FBCLK = 10MHz
    //******************************************************************************
    // configure Multisynth 3 P1 parameters = (128 * a) + Floor(128 * b/c) - 512
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_P1_17_16 + SI5351_MS3_OFFSET, SI5351_MSX_P1_MASK, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P1_15_8 + SI5351_MS3_OFFSET, 0x2A);
    SI5351_WriteRegister(dev, SI5351_MS0_P1_7_0 + SI5351_MS3_OFFSET, 0xD9);
    // configure Multisynth 3 P2 parameters = (128 * b) - (c * Floor(128 * b/c))
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_P2_19_16 + SI5351_MS3_OFFSET, SI5351_MSX_P2_MASK, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P2_15_8 + SI5351_MS3_OFFSET, 0x02);
    SI5351_WriteRegister(dev, SI5351_MS0_P2_7_0 + SI5351_MS3_OFFSET, 0x37);
    // configure Multisynth 3 P3 parameters = c
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_P3_19_16 + SI5351_MS3_OFFSET, SI5351_MSX_P3_MASK, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P3_15_8 + SI5351_MS3_OFFSET, 0x02);
    SI5351_WriteRegister(dev, SI5351_MS0_P3_7_0 + SI5351_MS3_OFFSET, 0x71);
    // configure Multisynth 3 parameters
    SI5351_WriteRegisterWithMask(dev, SI5351_R0 + SI5351_MS3_OFFSET, SI5351_MSX_RX_DIV_MASK, 0x00);
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_DIVBY4 + SI5351_MS3_OFFSET, SI5351_MSX_DIVBY4_MASK, 0x00);

    //******************************************************************************
    // MS4 - Multisynth 4 - Ethernet = 25MHz
    //******************************************************************************
    // configure Multisynth 4 P1 parameters = (128 * a) + Floor(128 * b/c) - 512
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_P1_17_16 + SI5351_MS4_OFFSET, SI5351_MSX_P1_MASK, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P1_15_8 + SI5351_MS4_OFFSET, 0x10);
    SI5351_WriteRegister(dev, SI5351_MS0_P1_7_0 + SI5351_MS4_OFFSET, 0x00);
    // configure Multisynth 4 P2 parameters = (128 * b) - (c * Floor(128 * b/c))
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_P2_19_16 + SI5351_MS4_OFFSET, SI5351_MSX_P2_MASK, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P2_15_8 + SI5351_MS4_OFFSET, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P2_7_0 + SI5351_MS4_OFFSET, 0x00);
    // configure Multisynth 4 P3 parameters = c
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_P3_19_16 + SI5351_MS4_OFFSET, SI5351_MSX_P3_MASK, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P3_15_8 + SI5351_MS4_OFFSET, 0x00);
    SI5351_WriteRegister(dev, SI5351_MS0_P3_7_0 + SI5351_MS4_OFFSET, 0x01);
    // configure Multisynth 4 parameters
    SI5351_WriteRegisterWithMask(dev, SI5351_R0 + SI5351_MS4_OFFSET, SI5351_MSX_RX_DIV_MASK, 0x00);
    SI5351_WriteRegisterWithMask(dev, SI5351_MS0_DIVBY4 + SI5351_MS4_OFFSET, SI5351_MSX_DIVBY4_MASK, 0x00);

    // vcxo parameters = 1.03 * (128 * a + b/1e6) * APR
    SI5351_WriteRegister(dev, SI5351_VCXO_21_16, 0x0F);
    SI5351_WriteRegister(dev, SI5351_VCXO_15_8, 0xEF);
    SI5351_WriteRegister(dev, SI5351_VCXO_7_0, 0x03);

    // configure crystal load capacitance (10pF)
    SI5351_WriteRegister(dev, SI5351_XTAL_LOAD_CAP, 0xD2);

    // disable fanout
    SI5351_WriteRegister(dev, SI5351_FANOUT_ENABLE, 0xC0);

    // configure output enable control (enable CLK0-CLK4)
    SI5351_WriteRegister(dev, SI5351_OUTPUT_ENABLE, 0xE0);

    // success
    return true;
}

//******************************************************************************
// @brief  Reads a value from the specified Si5351 register
// @param  dev: I2C device
// @param  reg: the register to read from
// @param  value: pointer to where the read value will be stored
// @retval True if the read succeeded, false otherwise
//******************************************************************************
static bool SI5351_ReadRegister(struct udevice *dev, uint8_t reg, uint8_t *value)
{
    // read from register
    if (dm_i2c_read(dev, reg, value, 1) != 0)
    {
        // error
        return false;
    }

    // success
    return true;
}

//******************************************************************************
// @brief  Writes the value to the specified Si5351 register
// @param  dev: I2C device
// @param  reg: the register to write to
// @param  value: the value to write
// @retval True if the write succeeded, false otherwise
//******************************************************************************
static bool SI5351_WriteRegister(struct udevice *dev, uint8_t reg, uint8_t value)
{
    // write to register
    if (dm_i2c_write(dev, reg, &value, 1) != 0)
    {
        // error
        return false;
    }

    // success
    return true;
}

//******************************************************************************
// @brief  Writes the masked value to the specified Si5351 register
// @param  dev: I2C device
// @param  reg: the register to write to
// @param  mask: the mask to apply
// @param  value: the value to write
// @retval True if the write succeeded, false otherwise
//******************************************************************************
static bool SI5351_WriteRegisterWithMask(struct udevice *dev, uint8_t reg, uint8_t mask, uint8_t value)
{
    uint8_t data;

    // get current register value
    if (SI5351_ReadRegister(dev, reg, &data) == false)
    {
        // error
        return false;
    }

    // clear bits to be modified
    data &= ~mask;

    // prepare new value
    value = data | (value & mask);

    // write to register
    if (SI5351_WriteRegister(dev, reg, value) == false)
    {
        // error
        return false;
    }

    // success
    return true;
}
