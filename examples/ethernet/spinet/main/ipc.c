/* Inter Process Comunication

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "driver/spi_slave.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "spinet.h"
#include "ipc.h"

#define DMA_CHAN 2

static const char *TAG = "ipc";

#define IPC_CHECK(a, str, goto_tag,...)                                           \
    do                                                                            \
    {                                                                             \
        if (!(a))                                                                 \
        {                                                                         \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                        \
        }                                                                         \
    } while (0)


static bool ipc_transfer_complete = true;
//static bool is_sending = false;

static spi_slave_transaction_t spi_transfer;
TaskHandle_t spi_queue_task_hdl;

/*
 * IPC buffers definition
 */
DMA_ATTR uint8_t tx_buff[IPC_TRANSFER_LEN] = {0};
DMA_ATTR uint8_t rx_buff[IPC_TRANSFER_LEN] = {0};

void delay_us(uint32_t us)
{
    uint32_t i, j;

    for(i = 0; i < (1000000/CPU_MHZ); i++) {
        for(j = 0; j < us; j++);
    }
}

uint8_t ipc_frame_crc8(uint8_t *data, uint16_t len)
{
  unsigned crc = 0;
  int i, j;

  /* Using x^8 + x^2 + x + 1 polynomial */
  for (j = len; j; j--, data++) {
    crc ^= (*data << 8);

    for(i = 8; i; i--) {
      if (crc & 0x8000) {
        crc ^= (0x1070 << 3);
      }
      crc <<= 1;
    }
  }
  return (uint8_t)(crc >> 8);
}

bool ipc_frame_create(ipc_frame_t *frame, uint8_t *data, uint16_t len)
{
    bool ret = false;

    if(len <= IPC_DATA_MAX_LEN) {
        /* Reset IPC frame */
        memset((void *)frame, 0, sizeof(ipc_frame_t));

        /* Creates IPC frame header */
        frame->header.soh  = IPC_FRAME_SOH;
        frame->header.len  = len;
        frame->header.crc8 = ipc_frame_crc8((uint8_t *)&frame->header, 3);

        /* Create IPC frame data and CRC8 */
        memcpy(frame->data, data, len);
        frame->crc8 = ipc_frame_crc8(frame->data, len);

        ret = true;
    } else {
        printf("Error data length exceeds allow max length\r\n");
    }
    
    return ret;
}

bool ipc_frame_check(ipc_frame_t *frame)
{
    bool ret = false;

    if(frame->header.soh != IPC_FRAME_SOH) {
        //printf("Error IPC frame header SOH\r\n");
        return ret;
    } else if(ipc_frame_crc8((uint8_t *)&frame->header, 3) != frame->header.crc8) {
        //printf("Error IPC frame header\r\n");
        return ret;
    } else if(ipc_frame_crc8(frame->data, frame->header.len) != frame->crc8) {
        //printf("Error IPC frame data\r\n");
        return ret;
    } else {
        ret = true;
    }

    return ret;
}

void ipc_frame_print(ipc_frame_t *frame)
{
    int i;

    printf("\r\n===== IPC Frame Info =====\r\n");
    printf("Header:\r\n");
    printf("  Soh : 0x%X\r\n", frame->header.soh);
    printf("  Len : %d\r\n", frame->header.len);
    printf("  Data:\r\n");

    for (i = 0; i < frame->header.len; i++) {
        if(frame->data[i] <= 0x0F) {
            printf("0%X ", frame->data[i]);
        } else {
            printf("%X ", frame->data[i]);
        }
    }

    printf("\r\n");
}

void start_dma_spi_transfer(uint8_t *buff)
{
    if(buff == NULL) {
        memset(tx_buff, 0, IPC_TRANSFER_LEN);
    }

    spi_slave_queue_trans(CONFIG_SPINET_SPI_HOST, &spi_transfer, portMAX_DELAY);
}

void __attribute__((weak)) ipc_receive_callback(spi_slave_transaction_t *trans)
{

}

static void IRAM_ATTR Led_Toggle(void)
{
    static int flag = 1;
    if(flag){
       LED_ON();
       flag = 0;
    } else {
       LED_OFF();
       flag = 1;
    }
}

static void IRAM_ATTR ipc_post_trans_cb(spi_slave_transaction_t *trans)
{
    set_slaver_busy();
    Led_Toggle();
    ipc_receive_callback(trans);
    ipc_transfer_complete = true;
    set_slaver_ready();   
}

static void spinet_spi_queue_task_wake(void)
{
    BaseType_t high_task_wakeup = pdFALSE;
    vTaskNotifyGiveFromISR(spi_queue_task_hdl, &high_task_wakeup);
    if (high_task_wakeup != pdFALSE) {
        portYIELD_FROM_ISR();
    }
}

static void IRAM_ATTR ipc_gpio_isr_handler(void* arg)
{
    if(!is_master_busy()) {
        set_slaver_ready();
        ipc_transfer_complete = true;
        //ipc_init();
    } else if(ipc_transfer_complete) {
        set_slaver_busy();
        Led_Toggle();
        ipc_transfer_complete = false;
        spinet_spi_queue_task_wake();
    }
}

static void spinet_spi_queue_task(void *arg)
{
    while (1) {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        //ESP_LOGI(TAG, "spi_queue_task waken");
        start_dma_spi_transfer(NULL);
    }
   
    vTaskDelete(NULL);
}

void spi_init(void)
{
    esp_err_t ret;

    /* configuration for the SPI bus */
    spi_bus_config_t buscfg = {
        .mosi_io_num = CONFIG_SPINET_MOSI_GPIO,
        .miso_io_num = CONFIG_SPINET_MISO_GPIO,
        .sclk_io_num = CONFIG_SPINET_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    /* Configuration for the SPI slave interface */
    spi_slave_interface_config_t slvcfg = {
        .mode = 1, /* Notice me */
        .spics_io_num = CONFIG_SPINET_CS_GPIO,
        .queue_size = 3,
        .flags = 0,
        .post_trans_cb = ipc_post_trans_cb,
    };

    /*
     * enable pull-ups on SPI lines so we
     * don't detect rogue pulses when no master is connected.
     */
    gpio_set_pull_mode(CONFIG_SPINET_MOSI_GPIO, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(CONFIG_SPINET_SCLK_GPIO, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(CONFIG_SPINET_CS_GPIO, GPIO_PULLUP_ONLY);

    /* Initialize SPI slave interface */
    ret = spi_slave_initialize(CONFIG_SPINET_SPI_HOST, &buscfg, &slvcfg, DMA_CHAN);
    assert(ret == ESP_OK);
}

void gpio_init(void)
{
    gpio_config_t io_conf;

    /* init user LED */
    io_conf.intr_type    = GPIO_INTR_DISABLE;
    io_conf.mode         = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1 << CONFIG_SPINET_USER_LED_GPIO);
    gpio_config(&io_conf);

    /* init status out */
    io_conf.intr_type    = GPIO_INTR_DISABLE;
    io_conf.mode         = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1 << CONFIG_SPINET_STATUS_OUT_GPIO);
    gpio_config(&io_conf);

    /* init status in */
    io_conf.intr_type    = GPIO_INTR_DISABLE;
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pull_up_en   = 1;
    io_conf.pin_bit_mask = (1 << CONFIG_SPINET_STATUS_IN_GPIO);
    gpio_config(&io_conf);

    /* init send request out */
    io_conf.intr_type    = GPIO_INTR_DISABLE;
    io_conf.mode         = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1 << CONFIG_SPINET_SEND_REQUEST_GPIO);
    gpio_config(&io_conf);

    /* init interrupt gpio */
    io_conf.intr_type    = GPIO_PIN_INTR_NEGEDGE;
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pull_up_en   = 1;
    io_conf.pin_bit_mask = (1 << CONFIG_SPINET_INT_GPIO);
    gpio_config(&io_conf);

    /* register gpio interrup */
    gpio_install_isr_service(0);
    gpio_set_intr_type(CONFIG_SPINET_INT_GPIO, GPIO_PIN_INTR_NEGEDGE);
    gpio_isr_handler_add(CONFIG_SPINET_INT_GPIO, ipc_gpio_isr_handler, NULL);
}

void ipc_init(void)
{
    spi_transfer.tx_buffer = tx_buff;
    spi_transfer.rx_buffer = rx_buff;
    spi_transfer.length    = IPC_TRANSFER_LEN;
    
    /* create spinet task */
    BaseType_t xReturned = xTaskCreate(spinet_spi_queue_task, 
                                       "spi_queue_task", 
                                       2048, 
                                       NULL,
                                       15, 
                                       &spi_queue_task_hdl);
    
    IPC_CHECK(xReturned == pdPASS, "create spi queue task failed", err);  
    
    spi_init();
    gpio_init();

    set_slaver_ready();
    ipc_transfer_complete = true;
      
    LED_OFF();
    
    ipc_send_sync_cmd();
    
    return;
    
err:

    if (spi_queue_task_hdl) {
        vTaskDelete(spi_queue_task_hdl);
    }
}

void ipc_send_sync_cmd(void)
{
    set_slaver_ready();
	slaver_send_request();
	delay_us(WAITTIME);
}

ipc_status_t ipc_send(uint8_t *data, uint16_t len)
{
    ipc_frame_t *frame = (ipc_frame_t *)tx_buff;
    ipc_status_t ret = IPC_OK;

    if(!ipc_transfer_complete) {
        ESP_LOGI(TAG, "SPI not yet completed");
        return IPC_BUSY;
    }

    set_slaver_busy();

    if(!is_master_busy()) {
        if(ipc_frame_create(frame, data, len)) {
            ipc_transfer_complete = false;
            start_dma_spi_transfer((uint8_t *)frame);
            ret = IPC_OK;
            slaver_send_request();
        } else {
            set_slaver_ready();
            ret = IPC_ERR;
        }
    } else {
        ESP_LOGI(TAG, "SPI master is not ready");
        set_slaver_ready();
        ret = IPC_BUSY;
    }

    return ret;
}

bool ipc_possible_to_send(void)
{
    if(!ipc_transfer_complete) {
        return false;
    }

    if(!is_master_busy()) {
        set_slaver_busy();
        return true;
    } else {
        return false;
    }
}


