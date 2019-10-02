/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "py/runtime.h"
#include "py/stream.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "lib/utils/interrupt_char.h"
#include "py/ringbuf.h"
#include "spi.h"


/* Private variables ---------------------------------------------------------*/

/**
*@brief SPI master query SPI slave's idle status until it's idle.
*@param None.
*@return None.
*/
void spim_wait_slave_idle(void)
{
    uint8_t command_buffer[4];
    uint32_t temp_buffer;
    hal_spi_master_send_and_receive_config_t spi_send_and_receive_config;

    do {
        command_buffer[0] = SPI_SLAVER_CONFIG_READ;
        command_buffer[1] = SPI_SLAVER_REG_STATUS;
        spi_send_and_receive_config.send_data = command_buffer;
        spi_send_and_receive_config.send_length = 2;
        spi_send_and_receive_config.receive_buffer = (uint8_t *)&temp_buffer;
        spi_send_and_receive_config.receive_length = 4;
        hal_spi_master_send_and_receive_polling(SPI_TEST_MASTER, &spi_send_and_receive_config);
    } while (temp_buffer & 0x01000000);
}

/**
*@brief SPI master issue address to SPI slave.
*@param[in] address: address ready to write to SPI slave.
*@return None.
*/
void spim_write_slave_address(uint32_t address)
{
    uint8_t write_low_addr[4];
    uint8_t write_high_addr[4];

    write_low_addr[0] = SPI_SLAVER_CONFIG_WRITE;
    write_low_addr[1] = SPI_SLAVER_REG_LOW_ADDRESS;
    write_low_addr[2] = (address >> 8) & 0xff;
    write_low_addr[3] = address & 0xff;
    write_high_addr[0] = SPI_SLAVER_CONFIG_WRITE;
    write_high_addr[1] = SPI_SLAVER_REG_HIGH_ADDRESS;
    write_high_addr[2] = (address >> 24) & 0xff;
    write_high_addr[3] = (address >> 16) & 0xff;
    hal_spi_master_send_polling(SPI_TEST_MASTER, write_low_addr, 4);
    hal_spi_master_send_polling(SPI_TEST_MASTER, write_high_addr, 4);
}

/**
*@brief SPI master issue read/write command to SPI slave.
*@param[in] is_read: identify read or write command.
*@return None.
*/
void spim_write_slave_command(bool is_read)
{
    uint8_t command_buffer[4];

    command_buffer[0] = SPI_SLAVER_CONFIG_WRITE;
    command_buffer[1] = SPI_SLAVER_REG_CONFIG;
    command_buffer[2] = 0x00;
    if (is_read == true) {
        command_buffer[3] = SPI_SLAVER_CONFIG_BUS_SIZE | SPI_SLAVER_CONFIG_BUS_R;
    } else {
        command_buffer[3] = SPI_SLAVER_CONFIG_BUS_SIZE | SPI_SLAVER_CONFIG_BUS_W;
    }
    hal_spi_master_send_polling(SPI_TEST_MASTER, command_buffer, 4);
}

/**
*@brief SPI master write data to SPI slave.
*@param[in] data: the data ready to sent to SPI slave.
*@return None.
*/
void spim_write_slave_data(uint32_t data)
{
    uint8_t write_low_data[4];
    uint8_t write_high_data[4];

    write_low_data[0] = SPI_SLAVER_CONFIG_WRITE;
    write_low_data[1] = SPI_SLAVER_REG_WRITE_LOW_DATA;
    write_low_data[2] = (data >> 8) & 0xff;
    write_low_data[3] = data & 0xff;
    write_high_data[0] = SPI_SLAVER_CONFIG_WRITE;
    write_high_data[1] = SPI_SLAVER_REG_WRITE_HIGH_DATA;
    write_high_data[2] = (data >> 24) & 0xff;
    write_high_data[3] = (data >> 16) & 0xff;

    hal_spi_master_send_polling(SPI_TEST_MASTER, write_low_data, 4);
    hal_spi_master_send_polling(SPI_TEST_MASTER, write_high_data, 4);
}

/**
*@brief SPI master read data from SPI slave.
*@param None.
*@return Return the data read from SPI slave.
*/
uint32_t spim_read_slave_data(void)
{
    uint8_t command_buffer[4];
    hal_spi_master_send_and_receive_config_t spi_send_and_receive_config;
    uint32_t data, temp_buffer;

    command_buffer[0] = SPI_SLAVER_CONFIG_READ;
    command_buffer[1] = SPI_SLAVER_REG_READ_LOW_DATA;
    spi_send_and_receive_config.send_data = command_buffer;
    spi_send_and_receive_config.send_length = 2;
    spi_send_and_receive_config.receive_buffer = (uint8_t *)&temp_buffer;
    spi_send_and_receive_config.receive_length = 4;
    hal_spi_master_send_and_receive_polling(SPI_TEST_MASTER, &spi_send_and_receive_config);

    data = (temp_buffer >> 24) & 0x000000ff;
    data |= (temp_buffer >> 8) & 0x0000ff00;
    command_buffer[0] = SPI_SLAVER_CONFIG_READ;
    command_buffer[1] = SPI_SLAVER_REG_READ_HIGH_DATA;
    spi_send_and_receive_config.send_data = command_buffer;
    spi_send_and_receive_config.send_length = 2;
    spi_send_and_receive_config.receive_buffer = (uint8_t *)&temp_buffer;
    spi_send_and_receive_config.receive_length = 4;
    hal_spi_master_send_and_receive_polling(SPI_TEST_MASTER, &spi_send_and_receive_config);

    data |= (temp_buffer >> 8) & 0x00ff0000;
    data |= (temp_buffer << 8) & 0xff000000;

    return data;
}

/**
*@brief SPI master query status of SPI slave's ACK flag until it's set.
*@param None.
*@return None.
*/
void spim_wait_slave_ack(void)
{
    uint32_t ack_flag;

    do {
        spim_write_slave_address(SPI_TEST_SLV_BUFFER_ADDRESS + SPI_TEST_CONTROL_SIZE);
        spim_write_slave_command(true);
        spim_wait_slave_idle();
        ack_flag = spim_read_slave_data();
    } while (ack_flag != SPI_TEST_ACK_FLAG);

    spim_write_slave_address(SPI_TEST_SLV_BUFFER_ADDRESS + SPI_TEST_CONTROL_SIZE);
    spim_write_slave_data(0);
    spim_write_slave_command(false);
    spim_wait_slave_idle();
}

/**
*@brief SPI master issue IRQ command to SPI slave.
*@param None.
*@return None.
*/
void spim_active_slave_irq(void)
{
    uint8_t command_buffer[4];

    command_buffer[0] = SPI_SLAVER_CONFIG_WRITE;
    command_buffer[1] = SPI_SLAVER_REG_SLV_IRQ;
    command_buffer[2] = 0x00;
    command_buffer[3] = 0x01;
    hal_spi_master_send_polling(SPI_TEST_MASTER, command_buffer, 4);
}


/**
*@brief  This function is used to send data from SPI master to SPI slave.
*@param None.
*@return None.
*/
void spim_send_data_to_spis(void)
{
    uint32_t i;

    /* Step1: wait SPI slave to be idle status. */
    spim_wait_slave_idle();

    /* Step2: tell slaver that master want to write date to slave. */
    spim_write_slave_address(SPI_TEST_SLV_BUFFER_ADDRESS);
    spim_write_slave_data(SPI_TEST_WRITE_DATA_BEGIN);
    spim_write_slave_command(false);
    spim_wait_slave_idle();
    spim_active_slave_irq();
    spim_wait_slave_ack();

    /* Step3: SPI master send data to SPI slaver. */

    spim_write_slave_address(SPI_TEST_SLV_BUFFER_ADDRESS + SPI_TEST_CONTROL_SIZE + SPI_TEST_STATUS_SIZE);
    for (i = 0; i < SPI_TEST_DATA_SIZE / 4; i++) {
        spim_write_slave_data(SPI_TEST_TX_DATA_PATTERN);
        spim_write_slave_command(false);
        spim_wait_slave_idle();
    }

    /* Step4: notice slave send completely and wait slave to ack. */
    spim_write_slave_address(SPI_TEST_SLV_BUFFER_ADDRESS);
    spim_write_slave_data(SPI_TEST_WRITE_DATA_END);
    spim_write_slave_command(false);
    spim_active_slave_irq();
    spim_wait_slave_ack();
}


/**
*@brief  This function is used to receive data from spi slave to spi master.
*@param None.
*@return None.
*/
void spim_receive_data_from_spis(void)
{
    uint32_t i, data;

    /* Step1: wait SPI slave to be idle status. */
    spim_wait_slave_idle();

    /* Step2: tell slaver that master want to read date from slave. */
    spim_write_slave_address(SPI_TEST_SLV_BUFFER_ADDRESS);
    spim_write_slave_data(SPI_TEST_READ_DATA_BEGIN);
    spim_write_slave_command(false);
    spim_wait_slave_idle();
    spim_active_slave_irq();
    spim_wait_slave_ack();

    /* Step3: SPI master read data from SPI slaver and verify it. */
    spim_write_slave_address(SPI_TEST_SLV_BUFFER_ADDRESS + SPI_TEST_CONTROL_SIZE + SPI_TEST_STATUS_SIZE);
    for (i = 0; i < SPI_TEST_DATA_SIZE / 4; i++) {
        spim_write_slave_command(true);
        spim_wait_slave_idle();
        data = spim_read_slave_data();
        if (data != SPI_TEST_RX_DATA_PATTERN) {
            return;
        }
    }

    /* Step4: notice SPI slave master has read completely. */
    spim_write_slave_address(SPI_TEST_SLV_BUFFER_ADDRESS);
    spim_write_slave_data(SPI_TEST_READ_DATA_END);
    spim_write_slave_command(false);
    spim_wait_slave_idle();
    spim_active_slave_irq();

}

/**
*@brief  Example of spi send datas. In this function, SPI master send various commonds to slave.
*@param  None.
*@return None.
*/
void spi_master_transfer_data_two_boards_example(void)
{
    hal_spi_master_config_t spi_config;


    /*Step1: init GPIO, set SPIM pinmux(if EPT tool hasn't been used to configure the related pinmux).*/
    hal_gpio_init(SPI_PIN_NUMBER_CS);
    hal_gpio_init(SPI_PIN_NUMBER_CLK);
    hal_gpio_init(SPI_PIN_NUMBER_MOSI);
    hal_gpio_init(SPI_PIN_NUMBER_MISO);
    hal_pinmux_set_function(SPI_PIN_NUMBER_CS, SPIM_PIN_FUNC_CS);
    hal_pinmux_set_function(SPI_PIN_NUMBER_CLK, SPIM_PIN_FUNC_CLK);
    hal_pinmux_set_function(SPI_PIN_NUMBER_MOSI, SPIM_PIN_FUNC_MOSI);
    hal_pinmux_set_function(SPI_PIN_NUMBER_MISO, SPIM_PIN_FUNC_MISO);

    /*Step2: initialize SPI master. */
    spi_config.bit_order = HAL_SPI_MASTER_MSB_FIRST;
    spi_config.slave_port = HAL_SPI_MASTER_SLAVE_0;
    spi_config.clock_frequency = SPI_TEST_FREQUENCY;
    spi_config.phase = HAL_SPI_MASTER_CLOCK_PHASE0;
    spi_config.polarity = HAL_SPI_MASTER_CLOCK_POLARITY0;
    if (HAL_SPI_MASTER_STATUS_OK != hal_spi_master_init(SPI_TEST_MASTER, &spi_config)) {
        return;
    } else {
        printf("hal_spi_master_init pass\r\n");
    }

    /* Step3: SPI master send data to SPI slaver and verify whether received data is correct. */
    spim_send_data_to_spis();

    /* Step4: SPI master receive data from SPI slaver and verify whether received data is correct. */
    spim_receive_data_from_spis();

    /* Step5: deinit spi master and related GPIO. */
    if (HAL_SPI_MASTER_STATUS_OK != hal_spi_master_deinit(SPI_TEST_MASTER)) {
        return;
    }
    hal_gpio_deinit(SPI_PIN_NUMBER_CS);
    hal_gpio_deinit(SPI_PIN_NUMBER_CLK);
    hal_gpio_deinit(SPI_PIN_NUMBER_MOSI);
    hal_gpio_deinit(SPI_PIN_NUMBER_MISO);

}

volatile bool transfer_data_finished = false;
/* 1.For this example project, we define slaver_data_buffer layout as below:
 *   The frist word is used to store command request from SPI master.
 *     - SPI_TEST_READ_DATA_BEGIN: when SPI master want to read data from SPI slave,
 *       it send this command to SPI slave to prepare the data.
 *     - SPI_TEST_READ_DATA_END: when SPI master has read all needed data,
 *       it send this command to notice the SPI slave.
 *     - SPI_TEST_WRITE_DATA_BEGIN: when SPI master want write data to SPI slave,
 *       it send this command to SPI slave to prepare the buffer.
 *     - SPI_TEST_WRITE_DATA_END: when SPI master has sent out of data,
 *       it send this command to notice the SPI slave.
 *   The second word is used to store ACK to respond to SPI master's command request.
 *     - define SPI_TEST_ACK_FLAG as the ACK flag.
 *   The rest space is used to store actual data transfer between SPI master and SPI slave.
 * 2.The buffer defined here is fix at specific address according declare both in linkscript
 *   and here.
 * 3.Define of the buffer structure above is just an example for demonstrate,
 *   User should implement it's own buffer structure define based on it's requirement.
 * 4.The buffer defined here should be located at non-cacheable area.
 */
#ifndef __ICCARM__
__attribute__((__section__(".spi_section"))) uint8_t slaver_data_buffer[SPI_TEST_DATA_SIZE + SPI_TEST_CONTROL_SIZE + SPI_TEST_STATUS_SIZE] = {0};
#else
_Pragma("location=\".spi_section\"") _Pragma("data_alignment=512") uint8_t slaver_data_buffer[SPI_TEST_DATA_SIZE + SPI_TEST_CONTROL_SIZE + SPI_TEST_STATUS_SIZE] = {0};
#endif

/**
*@brief SPI slave set the second word to notice SPI master.
*@param None.
*@return None.
*/
void spis_notice_spim(void)
{
    slaver_data_buffer[4] = SPI_TEST_ACK_FLAG;
}

/**
*@brief user's callback function registered to SPI slave driver.
*@param[in] user_data: user's data used in this callback function.
*@return None.
*/
void spis_user_callback(void *from_upy_data)
{
    uint32_t *ptr, i;
    hw_spi_trans_slave_data *transfer_data = (hw_spi_trans_slave_data*) from_upy_data;
    uint32_t *src, *dest;
    if (transfer_data->src != NULL)
        src = transfer_data->src;
    if (transfer_data->dest != NULL)
        dest = transfer_data->dest;

    switch (slaver_data_buffer[0]) {
        case SPI_TEST_WRITE_DATA_BEGIN:
            /* master want to write data to us, so we prepare data buffer frist. */
            ptr = (uint32_t *)(&slaver_data_buffer[SPI_TEST_CONTROL_SIZE + SPI_TEST_STATUS_SIZE]);
            memset(ptr, 0, SPI_TEST_DATA_SIZE);
            /* then notice SPI master. */
            spis_notice_spim();
            break;
        case SPI_TEST_WRITE_DATA_END:
            /* master has written data completely, let's check the data. */
            ptr = (uint32_t *)(&slaver_data_buffer[SPI_TEST_CONTROL_SIZE + SPI_TEST_STATUS_SIZE]);
            if (transfer_data->len > SPI_TEST_DATA_SIZE)
                transfer_data->len = SPI_TEST_DATA_SIZE;
            /* then notice SPI master. */
            for (int i = 0; i < transfer_data->len ; i++) {
                dest[i] = ptr[i];    
            }
            spis_notice_spim();
            transfer_data_finished = true;
            break;
        case SPI_TEST_READ_DATA_BEGIN:
            /* master want to read data, so we prepare data frist. */
            ptr = (uint32_t *)(&slaver_data_buffer[SPI_TEST_CONTROL_SIZE + SPI_TEST_STATUS_SIZE]);
            memset(ptr, 0, SPI_TEST_DATA_SIZE);
            
            if (transfer_data->len > SPI_TEST_DATA_SIZE)
                transfer_data->len = SPI_TEST_DATA_SIZE;

            for (i = 0; i < transfer_data->len; i++) {
                ptr[i] = src[i];
            }
            /* then notice SPI master. */
            spis_notice_spim();
            break;
        case SPI_TEST_READ_DATA_END:
            /* master has read data completely, so we end up the example. */
            if (transfer_data->dest == NULL)
                transfer_data_finished = true;
            break;
        default:
            break;
    }
}

