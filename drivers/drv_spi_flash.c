/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-08-17     zylx         first implementation.
 */
 
#include <board.h>
#include <drv_spi.h>
#include <rtdevice.h>
#include <rthw.h>

#ifdef BSP_USING_FLASH_SPI_GD25Q64

#include "spi_flash.h"
#include "spi_flash_sfud.h"

#define SPI_SPEED_2MHZ_DEBUG

static struct rt_spi_device *spi_dev_gd25q64;

int rt_hw_spi_flash_with_sfud_init(void)
{   
    __HAL_RCC_GPIOB_CLK_ENABLE();
    rt_hw_spi_device_attach("spi2", "spi20", GPIOB, GPIO_PIN_12);

    /* find spi device */
    spi_dev_gd25q64 = (struct rt_spi_device *) rt_device_find("spi20");
    if(!spi_dev_gd25q64)
    {
        rt_kprintf("spi flash with sfud init failed! can't find spi device.");
        return -RT_ERROR;
    }

#ifdef SPI_SPEED_2MHZ_DEBUG
    /* config spi */
    {
        struct rt_spi_configuration cfg;
        cfg.data_width = 8;
        cfg.mode = RT_SPI_MODE_0 | RT_SPI_MSB; /* SPI Compatible Modes 0 */
        cfg.max_hz = 1 * 1000 * 1000;          /* SPI Interface with Clock Speeds Up to 1 MHz */
        rt_spi_configure(spi_dev_gd25q64, &cfg);
    } /* config spi */
#endif

    /* init gd25q64 */
    if (RT_NULL == rt_sfud_flash_probe("gd25q64", "spi20"))
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rt_hw_spi_flash_with_sfud_init);

#endif/* BSP_USING_FLASH_SPI_GD25Q64 */
