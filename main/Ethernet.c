/*
 * Ethernet.c
 *
 *  Created on: 17 mai 2023
 *      Author: StayInfo-NicolasDedo
 */


#include <stdio.h>
#include <string.h>
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/spi_master.h"


esp_netif_t* init_ethernet() {

	esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
	esp_netif_config_t cfg_spi = {
			.base = &esp_netif_config,
			.stack = ESP_NETIF_NETSTACK_DEFAULT_ETH
	};
	esp_netif_config.if_key = "ETH_SPI_1";
	esp_netif_config.if_desc = "eth0";
	esp_netif_config.route_prio = 30;
	esp_netif_t *eth_netif_spi = esp_netif_new(&cfg_spi);

	eth_mac_config_t mac_config_spi = ETH_MAC_DEFAULT_CONFIG();
	eth_phy_config_t phy_config_spi = ETH_PHY_DEFAULT_CONFIG();

	spi_device_interface_config_t spi_devcfg = {
			.mode = 0,
			.clock_speed_hz = CONFIG_SID_ETH_SPI_CLOCK_MHZ * 1000 * 1000,  // Clock speed at 1 Mhz to force quickly fail
			.spics_io_num = CONFIG_SID_ETH_SPI_CS_GPIO,
			.queue_size = 20
	};

	phy_config_spi.phy_addr = CONFIG_SID_ETH_SPI_PHY_ADDR;
	phy_config_spi.reset_gpio_num = CONFIG_SID_ETH_SPI_PHY_RST_GPIO;

	eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(CONFIG_SID_ETH_SPI_HOST, &spi_devcfg);
	w5500_config.int_gpio_num = CONFIG_SID_ETH_SPI_INT_GPIO;
	esp_eth_mac_t *mac_spi = esp_eth_mac_new_w5500(&w5500_config, &mac_config_spi);
	esp_eth_phy_t *phy_spi = esp_eth_phy_new_w5500(&phy_config_spi);

	esp_eth_config_t eth_config_spi = ETH_DEFAULT_CONFIG(mac_spi, phy_spi);
	esp_eth_handle_t eth_handle_spi;
	ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config_spi, &eth_handle_spi));

	uint8_t mac_addr[8] = {0};
	ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac_addr));
	ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle_spi, ETH_CMD_S_MAC_ADDR, mac_addr));

	ESP_ERROR_CHECK(esp_netif_attach(eth_netif_spi, esp_eth_new_netif_glue(eth_handle_spi)));
	ESP_ERROR_CHECK(esp_eth_start(eth_handle_spi));
	return eth_netif_spi;
}



