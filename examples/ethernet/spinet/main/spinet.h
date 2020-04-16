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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_eth_mac.h"
#include "esp_eth_phy.h"
#include "driver/spi_master.h"

//#define NCOVIF_ENABLE  /* enable ncovif */

/**
 * @brief SPINET specific configuration
 *
 */
typedef struct {
    spi_device_handle_t spi_hdl; /*!< Handle of SPI device driver */
    int int_gpio_num;            /*!< Interrupt GPIO number */
} eth_spinet_config_t;

typedef struct {
    esp_eth_mac_t parent;
    esp_eth_mediator_t *eth;
    spi_device_handle_t spi_hdl;
    SemaphoreHandle_t mutex_lock;
    TaskHandle_t rx_task_hdl;
    uint8_t addr[6];
} emac_spinet_t;

/**
 * @brief Default SPINET specific configuration
 *
 */
#define ETH_SPINET_DEFAULT_CONFIG(spi_device) \
    {                                         \
        .spi_hdl = spi_device,                \
        .int_gpio_num = 4,                    \
    }

esp_eth_mac_t *esp_eth_mac_new_spinet(const eth_spinet_config_t *spinet_config, const eth_mac_config_t *mac_config);

esp_eth_phy_t *esp_eth_phy_new_spinet(const eth_phy_config_t *config);

#ifdef __cplusplus
}
#endif
