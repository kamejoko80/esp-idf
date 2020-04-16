// Copyright 2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <string.h>
#include <stdlib.h>
#include <sys/cdefs.h>
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_eth.h"
#include "esp_system.h"
#include "esp_intr_alloc.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "spinet.h"
#include "ncovif.h"
#include "sdkconfig.h"
#include "ipc.h"

static const char *TAG = "spinet_mac";

emac_spinet_t *g_emac = NULL;

#define SPINET_MUTEX_LOCK_TIMEOUT_MS (50)

#define MAC_CHECK(a, str, goto_tag, ret_value, ...)                               \
    do                                                                            \
    {                                                                             \
        if (!(a))                                                                 \
        {                                                                         \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            ret = ret_value;                                                      \
            goto goto_tag;                                                        \
        }                                                                         \
    } while (0)

#ifndef NCOVIF_ENABLE
typedef struct
{
    uint16_t len;
    uint8_t data[IPC_DATA_MAX_LEN];
} ethii_frame_info_t;

ethii_frame_info_t ethii_input;
#endif

static inline bool spinet_lock(emac_spinet_t *emac)
{
    return xSemaphoreTake(emac->mutex_lock, pdMS_TO_TICKS(SPINET_MUTEX_LOCK_TIMEOUT_MS)) == pdTRUE;
}

static inline bool spinet_unlock(emac_spinet_t *emac)
{
    return xSemaphoreGive(emac->mutex_lock) == pdTRUE;
}

static esp_err_t emac_spinet_set_mediator(esp_eth_mac_t *mac, esp_eth_mediator_t *eth)
{
    esp_err_t ret = ESP_OK;
    MAC_CHECK(eth, "can't set mac's mediator to null", out, ESP_ERR_INVALID_ARG);
    emac_spinet_t *emac = __containerof(mac, emac_spinet_t, parent);
    emac->eth = eth;
out:
    return ret;
}

static esp_err_t emac_spinet_init(esp_eth_mac_t *mac)
{
    esp_err_t ret = ESP_OK;

#ifndef NCOVIF_ENABLE
    ESP_LOGI(TAG, "Initialize IPC...");
    ipc_init();
#endif

    return ret;
}

static esp_err_t emac_spinet_deinit(esp_eth_mac_t *mac)
{
    return ESP_OK;
}

static esp_err_t emac_spinet_del(esp_eth_mac_t *mac)
{
    emac_spinet_t *emac = __containerof(mac, emac_spinet_t, parent);
    vTaskDelete(emac->rx_task_hdl);
    vSemaphoreDelete(emac->mutex_lock);
    free(emac);
    return ESP_OK;
}

static esp_err_t emac_spinet_write_phy_reg(esp_eth_mac_t *mac, uint32_t phy_addr,
        uint32_t phy_reg, uint32_t reg_value)
{
    esp_err_t ret = ESP_OK;
    return ret;
}

static esp_err_t emac_spinet_read_phy_reg(esp_eth_mac_t *mac, uint32_t phy_addr,
        uint32_t phy_reg, uint32_t *reg_value)
{
    esp_err_t ret = ESP_OK;

    *reg_value = 0;

    return ret;
}

static esp_err_t spinet_set_mac_addr(emac_spinet_t *emac)
{
    esp_err_t ret = ESP_OK;

    return ret;
}

static esp_err_t emac_spinet_set_addr(esp_eth_mac_t *mac, uint8_t *addr)
{
    esp_err_t ret = ESP_OK;
    MAC_CHECK(addr, "can't set mac addr to null", out, ESP_ERR_INVALID_ARG);
    emac_spinet_t *emac = __containerof(mac, emac_spinet_t, parent);
    memcpy(emac->addr, addr, 6);
    MAC_CHECK(spinet_set_mac_addr(emac) == ESP_OK, "set mac address failed", out, ESP_FAIL);
out:
    return ret;
}

static esp_err_t emac_spinet_get_addr(esp_eth_mac_t *mac, uint8_t *addr)
{
    esp_err_t ret = ESP_OK;
    MAC_CHECK(addr, "can't set mac addr to null", out, ESP_ERR_INVALID_ARG);
    emac_spinet_t *emac = __containerof(mac, emac_spinet_t, parent);
    memcpy(addr, emac->addr, 6);
out:
    return ret;
}

static esp_err_t emac_spinet_set_speed(esp_eth_mac_t *mac, eth_speed_t speed)
{
    esp_err_t ret = ESP_OK;
    switch (speed) {
    case ETH_SPEED_10M:
        ESP_LOGI(TAG, "working in 10Mbps");
        break;
    default:
        MAC_CHECK(false, "100Mbps unsupported", out, ESP_ERR_NOT_SUPPORTED);
        break;
    }
out:
    return ret;
}

static esp_err_t emac_spinet_set_duplex(esp_eth_mac_t *mac, eth_duplex_t duplex)
{
    esp_err_t ret = ESP_OK;

    return ret;
}

static esp_err_t emac_spinet_set_link(esp_eth_mac_t *mac, eth_link_t link)
{
    esp_err_t ret = ESP_OK;
    switch (link) {
    case ETH_LINK_UP:
        ESP_LOGI(TAG, "link up");
        break;
    case ETH_LINK_DOWN:
        ESP_LOGI(TAG, "link down");
        break;
    default:
        MAC_CHECK(false, "unknown link status", out, ESP_ERR_INVALID_ARG);
        break;
    }
out:
    return ret;
}

static esp_err_t emac_spinet_set_promiscuous(esp_eth_mac_t *mac, bool enable)
{
    esp_err_t ret = ESP_OK;

    return ret;
}


void dump_package(uint8_t *package, uint32_t len)
{
    uint32_t i;

    ESP_LOGI(TAG, "===== Package dumping =====\r\n");

    for (i = 0; i < len; i++){
        if(package[i] <= 0x0F){
            printf("0%X ", package[i]);
        } else {
            printf("%X ", package[i]);
        }
    }

    printf("\r\n");
}

static esp_err_t emac_spinet_transmit(esp_eth_mac_t *mac, uint8_t *buf, uint32_t length)
{
    esp_err_t ret = ESP_OK;

#ifdef NCOVIF_ENABLE
    emac_spinet_t *emac = __containerof(mac, emac_spinet_t, parent);

    /* internal received */
    eth_frame_process(emac, (struct ethIIhdr *)buf);
#else
 
    //ESP_LOGI(TAG, "emac data transmit");
    //dump_package(buf, length);

   if(!ipc_possible_to_send()) {
      return ESP_FAIL;
   }

   ipc_status_t status = ipc_send(buf, (uint16_t)length);

   if(status != IPC_OK) {
      return ESP_FAIL;
   }
   
#endif

    return ret;
}

static esp_err_t emac_spinet_receive(esp_eth_mac_t *mac, uint8_t *buf, uint32_t *length)
{
    esp_err_t ret = ESP_FAIL;

    return ret;
}

void emac_spinet_task_wake(emac_spinet_t *emac)
{
    BaseType_t high_task_wakeup = pdFALSE;
    vTaskNotifyGiveFromISR(emac->rx_task_hdl, &high_task_wakeup);
    if (high_task_wakeup != pdFALSE) {
        portYIELD_FROM_ISR();
    }
}

#ifndef NCOVIF_ENABLE
void ipc_receive_callback(spi_slave_transaction_t *trans)
{
    ipc_frame_t *frame = (ipc_frame_t *)trans->rx_buffer;

    if(ipc_frame_check(frame)) {
        if (spinet_lock(g_emac)) {
            memcpy(ethii_input.data, frame->data, frame->header.len);
            spinet_unlock(g_emac);
        } else {
            return;
        }
        ethii_input.len = frame->header.len;
        emac_spinet_task_wake(g_emac);    
    }    
}
#endif

static void emac_spinet_task(void *arg)
{
    emac_spinet_t *emac = (emac_spinet_t *)arg;

#ifndef NCOVIF_ENABLE
    uint8_t *buffer = NULL;
    uint16_t length;
#endif    

    while (1) {
        // block indefinitely until some task notifies me
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

#ifdef NCOVIF_ENABLE
        send_arp_response(emac);
        send_icmp_echo(emac);
#else    
        length = ethii_input.len;
        buffer = heap_caps_malloc(length, MALLOC_CAP_DMA);

        if (!buffer) {
            ESP_LOGE(TAG, "no mem for receive buffer");
        } else {
            /* pass the buffer to stack (e.g. TCP/IP layer) */
            if (length) {
                //ESP_LOGI(TAG, "emac data received");
                //dump_package(ethii_input.data, length);
                if (spinet_lock(emac)) {
                    memcpy(buffer, ethii_input.data, length);
                    spinet_unlock(emac);
                } else {
                    free(buffer);
                    continue;
                }
                emac->eth->stack_input(emac->eth, buffer, length);
            } else {
                free(buffer);
            }
        }      
#endif
    }
    vTaskDelete(NULL);
}

esp_eth_mac_t *esp_eth_mac_new_spinet(const eth_spinet_config_t *spinet_config, const eth_mac_config_t *mac_config)
{
    esp_eth_mac_t *ret = NULL;
    emac_spinet_t *emac = NULL;

    //MAC_CHECK(spinet_config, "can't set spinet specific config to null", err, NULL);
    MAC_CHECK(mac_config, "can't set mac config to null", err, NULL);

    emac = calloc(1, sizeof(emac_spinet_t));
    MAC_CHECK(emac, "calloc emac failed", err, NULL);

    /* bind methods and attributes */
    emac->parent.set_mediator = emac_spinet_set_mediator;
    emac->parent.init = emac_spinet_init;
    emac->parent.deinit = emac_spinet_deinit;
    emac->parent.del = emac_spinet_del;
    emac->parent.write_phy_reg = emac_spinet_write_phy_reg;
    emac->parent.read_phy_reg = emac_spinet_read_phy_reg;
    emac->parent.set_addr = emac_spinet_set_addr;
    emac->parent.get_addr = emac_spinet_get_addr;
    emac->parent.set_speed = emac_spinet_set_speed;
    emac->parent.set_duplex = emac_spinet_set_duplex;
    emac->parent.set_link = emac_spinet_set_link;
    emac->parent.set_promiscuous = emac_spinet_set_promiscuous;
    emac->parent.transmit = emac_spinet_transmit;
    emac->parent.receive = emac_spinet_receive;

    /* create mutex */
    emac->mutex_lock = xSemaphoreCreateMutex();
    MAC_CHECK(emac->mutex_lock, "create lock failed", err, NULL);

    /* create spinet task */
    BaseType_t xReturned = xTaskCreate(emac_spinet_task, "spinet_tsk", mac_config->rx_task_stack_size, emac,
                                       mac_config->rx_task_prio, &emac->rx_task_hdl);
    MAC_CHECK(xReturned == pdPASS, "create spinet task failed", err, NULL);

    /* store private driver */
    g_emac = emac;

    return &(emac->parent);

err:
    if (emac) {
        if (emac->rx_task_hdl) {
            vTaskDelete(emac->rx_task_hdl);
        }
        if (emac->mutex_lock) {
            vSemaphoreDelete(emac->mutex_lock);
        }
        free(emac);
    }

    return ret;
}
