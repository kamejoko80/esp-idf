/* Inter Process Comunication

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "spinet.h"
#include "driver/gpio.h"
#include "driver/spi_slave.h"

/* Connection
   Master(STM32F4)    <--->  Slaver(ESP32)

   PB6 (SPI1_NSS)     ----> GPIO14 (A5) (SPI1_NSS)
   PB3 (SPI1_SCK)     ----> GPIO15 (A7) (SPI_SCK)
   PB4 (SPI1_MISO)   <----  GPIO13 (A8) (SPI_MISO)
   PB5 (SPI1_MOSI)    ----> GPIO12 (A6) (SPI_MOSI)

   PE4 (m_status_out) ----> GPIO16 (D4) (m_status_in)
   PE5 (s_status_in) <----- GPIO17 (D3) (s_status_out)
   PB0 (m_irq_in)    <----- GPIO5  (D5) (s_send_rqst)
   PE1 (m_send_rqst)  ----> GPIO2  (A9) (s_irq_in)
*/

/* IPC definitcation */
#define GPIO_PIN_SET               (1)
#define GPIO_PIN_RESET             (0)
 
#define READ_M_STATUS()            (gpio_get_level(CONFIG_SPINET_STATUS_IN_GPIO))
#define set_slaver_busy()          (gpio_set_level(CONFIG_SPINET_STATUS_OUT_GPIO, 1))
#define set_slaver_ready()         (gpio_set_level(CONFIG_SPINET_STATUS_OUT_GPIO, 0))
#define is_master_busy()           (gpio_get_level(CONFIG_SPINET_STATUS_IN_GPIO) == GPIO_PIN_SET)
#define slaver_send_request()      (gpio_set_level(CONFIG_SPINET_SEND_REQUEST_GPIO, 1), gpio_set_level(CONFIG_SPINET_SEND_REQUEST_GPIO, 0))

#define LED_ON()                   (gpio_set_level(CONFIG_SPINET_USER_LED_GPIO, 0))
#define LED_OFF()                  (gpio_set_level(CONFIG_SPINET_USER_LED_GPIO, 1))

#define CPU_MHZ                    (240)
#define DELAY_UNIT                 (1)
#define WAITTIME                   (1000)

/*!
 * IPC parameter configuration
 */
#define IPC_TRANSFER_LEN (1524)
#define IPC_DATA_MAX_LEN (IPC_TRANSFER_LEN - 5)
#define IPC_FRAME_SOH    (0xA5)

#define FIFO_DEPTH       (10)
#define FIFO_BUF_SIZE    (FIFO_DEPTH*IPC_TRANSFER_LEN)

/*
 * IPC Frame structure:
 *  _____ _____ ______ ______ ____________ ______________
 * |     |     |      |      |            |              |
 * | SOH | LEN | CRC8 | CRC8 |    DATA    | ZERO PADDING |
 * |_____|_____|______|______|____________|______________|
 *
 * |<----------------- IPC_TRANSFER_LEN ---------------->|
 *
 *  SOH  : uint8_t  : Start of header
 *  LEN  : uint16_t : Data length (not include data CRC8)
 *  CRC8 : uint8_t  : Header CRC checksum
 *  CRC8 : uint8_t  : Frame data checksum
 *  DATA : uint8_t  : IPC frame data
 *
 */

/* IPC frame header structure */
typedef struct
{
    uint8_t  soh;    /* Start of header     */
    uint16_t len;    /* Payload length      */
    uint8_t  crc8;   /* Header CRC checksum */
} __attribute__((packed, aligned(1))) ipc_frame_header_t;

/*!
 * IPC frame structure
 */
typedef struct
{
	ipc_frame_header_t header;      /* Frame header      */
    uint8_t crc8;                   /* Data CRC checksum */
    uint8_t data[IPC_DATA_MAX_LEN]; /* Frame data        */
} __attribute__((packed, aligned(1))) ipc_frame_t;

typedef enum
{
	IPC_OK         = 0x00U,
	IPC_BUSY       = 0x01U,
	IPC_ERR        = 0x02U
} ipc_status_t;

void ipc_init(void);
bool ipc_frame_check(ipc_frame_t *frame);
bool ipc_possible_to_send(void);
void ipc_send_sync_cmd(void);
ipc_status_t ipc_send(uint8_t *data, uint16_t len);
void ipc_receive_callback(spi_slave_transaction_t *trans);

#ifdef __cplusplus
}
#endif
