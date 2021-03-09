/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-08-16     armink       first implementation
 * 2018-08-28     MurphyZhao   add fal
 */

#include <rtthread.h>
#include <fal.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>
#include "drv_spi.h"

#define BUF_SIZE 1024

static int fal_test(const char *partiton_name);

static uint8_t recv_data[1024 * 4] = {0};

uint8_t cmd_RDID[] = {0x9F}; // read flash RDID
uint8_t recv_cnt_RDID = 3;

uint8_t cmd_SFDP[] = {0x5A, 0x00, 0x00, 0x00, 0x00}; // SFDP
uint8_t recv_cnt_SFDP = 36;

static struct rt_spi_device *spi_dev_gd25q64;
int spi_flash_test(void)
{
    /* find qspi device */
    spi_dev_gd25q64 = (struct rt_spi_device *)rt_device_find("spi20");
    if (!spi_dev_gd25q64)
    {
        rt_kprintf("spi flash init failed! can't find spi device.");
        return -RT_ERROR;
    }

    /* config spi */
    {
        struct rt_spi_configuration cfg;
        cfg.data_width = 8;
        cfg.mode = RT_SPI_MODE_0 | RT_SPI_MSB; /* SPI Compatible Modes 0 */
        cfg.max_hz = 1 * 1000 * 1000;          /* SPI Interface with Clock Speeds Up to 1 MHz */
        rt_spi_configure(spi_dev_gd25q64, &cfg);
    } /* config spi */

    rt_spi_send_then_recv(spi_dev_gd25q64, cmd_RDID, sizeof(cmd_RDID) / sizeof(cmd_RDID[0]), recv_data, recv_cnt_RDID);
    rt_kprintf("\r\ncmd_RDID receive data length: %d", recv_cnt_RDID);
    for (uint32_t i = 0; i < recv_cnt_RDID; i++)
    {
        if (i % 4 == 0)
        {
            rt_kprintf("\r\nrecv_data ");
        }
        rt_kprintf("0x%02x ", recv_data[i]);
    }

    rt_thread_mdelay(10);
    rt_kprintf("\r\n");

    // read_sfdp_header
    rt_spi_send_then_recv(spi_dev_gd25q64, cmd_SFDP, sizeof(cmd_SFDP) / sizeof(cmd_SFDP[0]), recv_data, recv_cnt_SFDP);
    rt_kprintf("\r\ncmd_SFDP receive data length: %d", recv_cnt_SFDP);
    for (uint32_t i = 0; i < recv_cnt_SFDP; i++)
    {
        if (i % 4 == 0)
        {
            rt_kprintf("\r\nrecv_data ");
        }
        rt_kprintf("0x%02x ", recv_data[i]);
    }

    /* wait for enter qspi mode */
    rt_thread_mdelay(10);

    return RT_EOK;
}

int main(void)
{
    spi_flash_test();

    fal_init();

    if (fal_test("param") == 0)
    {
        LOG_I("Fal partition (%s) test success!", "param");
    }
    else
    {
        LOG_E("Fal partition (%s) test failed!", "param");
    }

    if (fal_test("download") == 0)
    {
        LOG_I("Fal partition (%s) test success!", "download");
    }
    else
    {
        LOG_E("Fal partition (%s) test failed!", "download");
    }

    return 0;
}

static int fal_test(const char *partiton_name)
{
    int ret;
    int i, j, len;
    uint8_t buf[BUF_SIZE];
    const struct fal_flash_dev *flash_dev = RT_NULL;
    const struct fal_partition *partition = RT_NULL;

    if (!partiton_name)
    {
        LOG_E("Input param partition name is null!");
        return -1;
    }

    partition = fal_partition_find(partiton_name);
    if (partition == RT_NULL)
    {
        LOG_E("Find partition (%s) failed!", partiton_name);
        ret = -1;
        return ret;
    }

    flash_dev = fal_flash_device_find(partition->flash_name);
    if (flash_dev == RT_NULL)
    {
        LOG_E("Find flash device (%s) failed!", partition->flash_name);
        ret = -1;
        return ret;
    }

    LOG_I("Flash device : %s   "
          "Flash size : %dK   "
          "Partition : %s   "
          "Partition size: %dK",
          partition->flash_name,
          flash_dev->len / 1024,
          partition->name,
          partition->len / 1024);

    /* 擦除 `partition` 分区上的全部数据 */
    ret = fal_partition_erase_all(partition);
    if (ret < 0)
    {
        LOG_E("Partition (%s) erase failed!", partition->name);
        ret = -1;
        return ret;
    }
    LOG_I("Erase (%s) partition finish!", partiton_name);

    /* 循环读取整个分区的数据，并对内容进行检验 */
    for (i = 0; i < partition->len;)
    {
        rt_memset(buf, 0x00, BUF_SIZE);
        len = (partition->len - i) > BUF_SIZE ? BUF_SIZE : (partition->len - i);

        /* 从 Flash 读取 len 长度的数据到 buf 缓冲区 */
        ret = fal_partition_read(partition, i, buf, len);
        if (ret < 0)
        {
            LOG_E("Partition (%s) read failed!", partition->name);
            ret = -1;
            return ret;
        }
        for (j = 0; j < len; j++)
        {
            /* 校验数据内容是否为 0xFF */
            if (buf[j] != 0xFF)
            {
                LOG_E("The erase operation did not really succeed!");
                ret = -1;
                return ret;
            }
        }
        i += len;
    }

    /* 把 0 写入指定分区 */
    for (i = 0; i < partition->len;)
    {
        /* 设置写入的数据 0x00 */
        rt_memset(buf, 0x00, BUF_SIZE);
        len = (partition->len - i) > BUF_SIZE ? BUF_SIZE : (partition->len - i);

        /* 写入数据 */
        ret = fal_partition_write(partition, i, buf, len);
        if (ret < 0)
        {
            LOG_E("Partition (%s) write failed!", partition->name);
            ret = -1;
            return ret;
        }
        i += len;
    }
    LOG_I("Write (%s) partition finish! Write size %d(%dK).", partiton_name, i, i / 1024);

    /* 从指定的分区读取数据并校验数据 */
    for (i = 0; i < partition->len;)
    {
        /* 清空读缓冲区，以 0xFF 填充 */
        rt_memset(buf, 0xFF, BUF_SIZE);
        len = (partition->len - i) > BUF_SIZE ? BUF_SIZE : (partition->len - i);

        /* 读取数据到 buf 缓冲区 */
        ret = fal_partition_read(partition, i, buf, len);
        if (ret < 0)
        {
            LOG_E("Partition (%s) read failed!", partition->name);
            ret = -1;
            return ret;
        }
        for (j = 0; j < len; j++)
        {
            /* 校验读取的数据是否为步骤 3 中写入的数据 0x00 */
            if (buf[j] != 0x00)
            {
                LOG_E("The write operation did not really succeed!");
                ret = -1;
                return ret;
            }
        }
        i += len;
    }

    ret = 0;
    return ret;
}
