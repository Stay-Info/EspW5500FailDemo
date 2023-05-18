#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/spi_master.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "Ethernet.c"
#include "Modbus.c"
#include "Adc.c"


static esp_netif_t *eth_netif_spi;

void initSpi1(){
	spi_bus_config_t buscfg = {
			.mosi_io_num = CONFIG_SID_SPI1_MOSI_GPIO,
			.miso_io_num = CONFIG_SID_SPI1_MISO_GPIO,
			.sclk_io_num = CONFIG_SID_SPI1_SCLK_GPIO,
			.quadwp_io_num = -1,
			.quadhd_io_num = -1,
	};
	ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO)); //Because SPI2_HOST has value to 1
}

void readMCP346xTask(void *parameter) {

	TickType_t xPeriod = pdMS_TO_TICKS(1000);
	TickType_t xLastWakeTime = xTaskGetTickCount();

	while (1) {
		int16_t result = read_adc_channel(0);
		update_register(0,result);
		result = read_adc_channel(1);
		update_register(1,result);
		result = read_adc_channel(2);
		update_register(2,result);
		result = read_adc_channel(3);
		update_register(3,result);
		vTaskDelayUntil(&xLastWakeTime, xPeriod);
	}
}

void app_main(void)
{
	ESP_ERROR_CHECK(gpio_install_isr_service(0));
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	initSpi1();
	eth_netif_spi = init_ethernet();
	init_modbus(eth_netif_spi);
	init_adc();
	xTaskCreate(readMCP346xTask, "Read MCP346x Task", 4096, NULL, 5, NULL); // If adc not used, ethernet don't block
}

