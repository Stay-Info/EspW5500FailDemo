/*
 * Modbus.C
 *
 *  Created on: 17 mai 2023
 *      Author: StayInfo-NicolasDedo
 */

#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mbcontroller.h"
#include "esp_netif.h"

#define MB_READ_MASK                        (MB_EVENT_INPUT_REG_RD \
		| MB_EVENT_HOLDING_REG_RD \
		| MB_EVENT_DISCRETE_RD \
		| MB_EVENT_COILS_RD)

#define MB_WRITE_MASK                       (MB_EVENT_HOLDING_REG_WR | MB_EVENT_COILS_WR)

#define MB_READ_WRITE_MASK                  (MB_READ_MASK | MB_WRITE_MASK)

static const char *TAG = "MODBUS SLAVE";

static uint16_t registers[30];

static mb_param_info_t reg_info;

static TaskHandle_t cleanTask;

/**
 * If queue is not purged that cause a latency about 100 ms
 * I don't find how to disable the queue if not used
 */
static void purgeQueue(void* nothing){
	while(1){
		mbc_slave_check_event(MB_READ_WRITE_MASK);
		mbc_slave_get_param_info(&reg_info, 5000);
		uint32_t value = uxTaskGetStackHighWaterMark(cleanTask);
	}
}

esp_err_t init_modbus(esp_netif_t *eth_netif_spi) {
	void* slave_handler = NULL;

	// Initialization of Modbus controller
	ESP_RETURN_ON_ERROR(mbc_slave_init_tcp(&slave_handler),TAG,"Unbale to init tcp modbus slave");

	mb_communication_info_t comm_info;
	comm_info.ip_addr_type = true ? MB_IPV4 : MB_IPV6; // TODO replace by a config option "ipv4 ou ipv6"
	comm_info.ip_mode = MB_MODE_TCP;
	comm_info.ip_port = CONFIG_FMB_TCP_PORT_DEFAULT;
	comm_info.ip_addr = NULL; // Bind to any address
	comm_info.ip_netif_ptr = eth_netif_spi;

	// Setup communication parameters and start stack
	ESP_RETURN_ON_ERROR(mbc_slave_setup((void*) &comm_info), TAG,"Unable to setup modbus slave");

	// Initialization of Input Registers area
	mb_register_area_descriptor_t reg_area; // Modbus register area descriptor structure
	reg_area.type = MB_PARAM_INPUT;
	reg_area.start_offset = 0;
	reg_area.address = (void*)registers;
	reg_area.size = sizeof(registers);
	ESP_RETURN_ON_ERROR(mbc_slave_set_descriptor(reg_area), TAG, "Unable to configure modbus registers");
	ESP_RETURN_ON_ERROR(mbc_slave_start(), TAG, "Unable to start modbus server");

	xTaskCreatePinnedToCore(purgeQueue,"Purge Modbus queue event",1024,NULL,
			CONFIG_FMB_PORT_TASK_PRIO,&cleanTask,1);
	return ESP_OK;
}

esp_err_t update_register(int address, uint16_t value){
	if(address<0 || address>29) return ESP_ERR_INVALID_ARG;
	registers[address] = value;
	return ESP_OK;
}



