/*
 * Adc.c
 *
 *  Created on: 17 mai 2023
 *      Author: StayInfo-NicolasDedo
 */

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_check.h"
#include <string.h>
#include "freertos/task.h"

#define ADC_ADDRESS 0x40
#define ADC_WRITE 0x02  // Incremental write
#define ADC_READ 0x03

#define FAST_CONV 0x28
#define FAST_STANDBY 0x2C
#define FAST_SHUTDOWN 0x30
#define FAST_FULL_SHUTDOWN 0x34
#define FAST_FULL_RESET 0x38

#define DATA_ADDR 0x0
#define CONFIG0_ADDR 0x1
#define CONFIG1_ADDR 0x2
#define CONFIG2_ADDR 0x3
#define CONFIG3_ADDR 0x4
#define IRQ_ADDR 0x5
#define MUX_ADDR 0x6

/**
 * Shutdown à 0 (00 xx xx xx) (no effect unless all other bits are 0)
 * Using the internal clock (xx 11 xx xx)
 * Current source disabled (xx xx 00 xx)
 */
#define BASE_VALUE_CONFIG0 0x30
#define STANDBY 0x2
#define CONVERSION 0x3
#define CONFIG0_STANDBY (BASE_VALUE_CONFIG0 | STANDBY)
#define CONFIG0_CONVERSION (BASE_VALUE_CONFIG0 | CONVERSION)

/**
 * Prescale = 0 (00 xx xx xx)
 * OSR = 24576 (xx 10 11 xx)
 * OSR = 98304 (xx 11 11 xx) => selected
 * Reserved always 0 (xx xx xx 00)
 */
#define CONFIG1_VALUE 0x3C

/**
 * BOOST = x2 (11 xx xx xx)
 * GAIN = 1 (xx 00 1x xx)
 * AZ_MUX enable (xx xx x1 xx)
 * Reserved always 1 (xx xx xx 11)
 */
#define CONFIG2_BASE_VALUE 0xCF

/**
 * CONV_MODE = One shot mode + autostandby (10 xx xx xx)
 * DATA_FORMAT = 16 bit (xx 00 xx xx)
 * CRC_FORMAT not used (xx xx 0x xx)
 * EN_CRCCOM not used (xx xx x0 xx)
 * EN_OFFCAL disabled (xx xx xx 0x)
 * EN_GAINCAL disabled (xx xx xx x0)
 */
#define CONFIG3_VALUE 0x40

/**
 * Unused (0x xx xx xx)
 * NEW_DATA_STATUS = 1 (x1 xx xx xx) (1= no data, 0 = new data)
 * ERROR_STATUS = 1 (xx 1x xx xx) (1 = no error, 0 = error)
 * POR_STATUS = 1 (xx x1 xx xx)
 * MDAT disable = 0 (xx xx 0x xx)
 * Inactive state is logic high = 1 (xx xx x1 xx) if 1 do not need pull-up resistor
 * Enable Fast command = 1 (xx xx xx 1x)
 * Disable conversion interrupt = 0 (xx xx xx x0)
 */
#define IRQ_INITIAL_STATE 0x76
#define IRQ_NEW_DATA_MASK 0x40
#define IRQ_ERROR_MASK 0x20

#define AGND 0x8

static spi_device_handle_t spi;



static void fastCommand(uint8_t command){
	spi_transaction_t transaction = {};
	transaction.length = 8; // size in bits
	command |= ADC_ADDRESS;
	transaction.tx_buffer = &command;
	ESP_ERROR_CHECK_WITHOUT_ABORT(spi_device_transmit(spi, &transaction));
}

static void writeReg(uint8_t reg, const uint8_t *data, size_t length) {
	spi_transaction_t transaction = {};
	transaction.length = (length + 1) * 8; // size in bits include address
	uint8_t tx_buffer[length + 1];
	tx_buffer[0] = ADC_ADDRESS | (reg<<2) | ADC_WRITE;
	memcpy(tx_buffer + 1, data, length);
	transaction.tx_buffer = tx_buffer;
	ESP_ERROR_CHECK_WITHOUT_ABORT(spi_device_transmit(spi, &transaction));
}

static void applyDefaultConfig() {
	fastCommand(FAST_FULL_RESET);
	uint8_t config[] = {
			CONFIG0_STANDBY,
			CONFIG1_VALUE,
			CONFIG2_BASE_VALUE,
			CONFIG3_VALUE
	};
	writeReg(CONFIG0_ADDR, config, sizeof(config));
	uint8_t irq = IRQ_INITIAL_STATE;
	writeReg(IRQ_ADDR, &irq, 1);
}

void init_adc() {
	spi_device_interface_config_t devcfg={
			.mode=0,       //SPI mode 0
			.clock_speed_hz= CONFIG_SID_ADC_SPI_CLOCK_MHZ * 1000 * 1000,
			.spics_io_num = CONFIG_SID_ADC_SPI_CS_GPIO,   //CS pin
			.queue_size=7,
	};
	spi_host_device_t spiHost = CONFIG_SID_ADC_SPI_HOST;
	ESP_ERROR_CHECK(spi_bus_add_device(spiHost, &devcfg, &spi));
	applyDefaultConfig();
}

static uint16_t readReg(uint8_t reg) {
	uint8_t command = ADC_ADDRESS | (reg<<2) | ADC_READ;
	uint32_t buffer = 0;
	spi_transaction_t transaction = {};
	transaction.length = 24;
	transaction.rx_buffer = &buffer;
	transaction.tx_buffer = &command;
	ESP_ERROR_CHECK_WITHOUT_ABORT(spi_device_transmit(spi, &transaction));
	return (buffer & 0xFFFF00) >>8;
}

static uint16_t swapBytes(uint16_t value) {
    uint16_t byte1 = (value & 0xFF00) >> 8;
    uint16_t byte2 = (value & 0x00FF) << 8;
    return byte1 | byte2;
}

int16_t read_adc_channel(uint8_t channel) {
	int waitTime = 10;  // En millisecondes
	uint8_t value = channel <<4 | AGND;
	writeReg(MUX_ADDR, &value, sizeof(value));
	vTaskDelay(pdMS_TO_TICKS(waitTime));
	fastCommand(FAST_CONV);
	uint8_t irq;
	int counter = 0;
	do{
		counter += waitTime;
		if (counter >= 500){
			ESP_LOGE("ADC","Timeout for ADC conversion => return 0");
			return 0;
		}
		vTaskDelay(pdMS_TO_TICKS(waitTime));
		irq = readReg(IRQ_ADDR) & 0xFF;
	}while(irq & IRQ_NEW_DATA_MASK);
	vTaskDelay(pdMS_TO_TICKS(10));
	uint16_t result = readReg(DATA_ADDR);
	return swapBytes(result);
}
