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
#include "esp_log.h"
#include "esp_eth.h"
#include "eth_phy_regs_struct.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static const char *TAG = "spinet_phy";
#define PHY_CHECK(a, str, goto_tag, ...)                                          \
    do                                                                            \
    {                                                                             \
        if (!(a))                                                                 \
        {                                                                         \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                        \
        }                                                                         \
    } while (0)

typedef struct {
    esp_eth_phy_t parent;
    esp_eth_mediator_t *eth;
    uint32_t addr;
    uint32_t reset_timeout_ms;
    eth_link_t link_status;
    int reset_gpio_num;
} phy_spinet_t;

static esp_err_t spinet_phy_reset(esp_eth_phy_t *phy)
{
    return ESP_OK;
}

static esp_err_t spinet_phy_reset_hw(esp_eth_phy_t *phy)
{
    return ESP_OK;
}

static esp_err_t spinet_phy_init(esp_eth_phy_t *phy)
{
    return ESP_OK;
}

static esp_err_t spinet_phy_deinit(esp_eth_phy_t *phy)
{
    return ESP_OK;
}

static esp_err_t spinet_phy_set_mediator(esp_eth_phy_t *phy, esp_eth_mediator_t *eth)
{
    PHY_CHECK(eth, "can't set mediator for enc28j60 to null", err);
    phy_spinet_t *spinet_phy = __containerof(phy, phy_spinet_t, parent);
    spinet_phy->eth = eth;
    return ESP_OK;
err:
    return ESP_ERR_INVALID_ARG;
}

static esp_err_t spinet_phy_negotiate(esp_eth_phy_t *phy)
{
    return ESP_OK;
}

static esp_err_t spinet_phy_get_link(esp_eth_phy_t *phy)
{
    return ESP_OK;
}

static esp_err_t spinet_phy_pwrctl(esp_eth_phy_t *phy, bool enable)
{
    return ESP_OK;
}

static esp_err_t spinet_phy_get_addr(esp_eth_phy_t *phy, uint32_t *addr)
{
    PHY_CHECK(addr, "addr can't be null", err);
    phy_spinet_t *spinet_phy = __containerof(phy, phy_spinet_t, parent);
    *addr = spinet_phy->addr;
    return ESP_OK;
err:
    return ESP_ERR_INVALID_ARG;
}

static esp_err_t spinet_phy_set_addr(esp_eth_phy_t *phy, uint32_t addr)
{
    phy_spinet_t *spinet_phy = __containerof(phy, phy_spinet_t, parent);
    spinet_phy->addr = addr;
    return ESP_OK;
}

static esp_err_t spinet_phy_del(esp_eth_phy_t *phy)
{
    phy_spinet_t *spinet_phy = __containerof(phy, phy_spinet_t, parent);
    free(spinet_phy);
    return ESP_OK;
}

esp_eth_phy_t *esp_eth_phy_new_spinet(const eth_phy_config_t *config)
{
    PHY_CHECK(config, "can't set phy config to null", err);
    phy_spinet_t *spinet_phy = calloc(1, sizeof(phy_spinet_t));
    PHY_CHECK(spinet_phy, "calloc phy failed", err);
	
    spinet_phy->addr = config->phy_addr; // although PHY addr is meaningless to SPINET
    spinet_phy->reset_timeout_ms = config->reset_timeout_ms;
    spinet_phy->reset_gpio_num = config->reset_gpio_num;
    spinet_phy->link_status = ETH_LINK_DOWN;
    spinet_phy->parent.reset = spinet_phy_reset;
    spinet_phy->parent.reset_hw = spinet_phy_reset_hw;
    spinet_phy->parent.init = spinet_phy_init;
    spinet_phy->parent.deinit = spinet_phy_deinit;
    spinet_phy->parent.set_mediator = spinet_phy_set_mediator;
    spinet_phy->parent.negotiate = spinet_phy_negotiate;
    spinet_phy->parent.get_link = spinet_phy_get_link;
    spinet_phy->parent.pwrctl = spinet_phy_pwrctl;
    spinet_phy->parent.get_addr = spinet_phy_get_addr;
    spinet_phy->parent.set_addr = spinet_phy_set_addr;
    spinet_phy->parent.del = spinet_phy_del;
    return &(spinet_phy->parent);
err:
    return NULL;
}
