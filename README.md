# EspW5500FailDemo
Reproductible code to block W5500

### Test conditions
This code was tested with this components :
- ESP32-S3
- W5500 spi ethernet
- MC3462 spi ADC

All components was on a custom PCB.

The ESP-IDF version was 5.0.1

### Prerequisites

To run the test script, you need Java 8 or higher on your machine and the variable JAVA_HOME must be set.

### How to reproduce
1. Download the code on a ESP32-S3
2. Run command on a terminal from the root of the project to send Modbus Request:

	a) On Windows : 
	```
	test '-ip=YOUR_W5500_IP_ADDRESS' -keep=false
	```
	b) On Linux (not tested) :
	```
	find -type f -iname "*.sh" -exec chmod +x {} \;
	sudo ./test.sh '-ip=YOUR_W5500_IP_ADDRESS' -keep=false
	```
3. After some time (between 5 minutes and 10 hours), the ADC update routine should stop and the Ethernet will stop responding. The esp debug terminal will show:
	```
	E (2037620) w5500.mac: emac_w5500_read_phy_reg(335): read PHY register failed
	E (2037620) w5500.phy: w5500_update_link_duplex_speed(69): read PHYCFG failed
	E (2037620) w5500.phy: w5500_get_link(112): update link duplex speed failed
	```

### Comments
The "-keep" argument specifies whether the modbus connection will be configured with "keep alive". The problem occurs faster when keepalive is disabled, so it's easier to disable this option for testing. (But the issue also occurs with keep alive enabled)

It seem the problem does not occur if the spi is not used for the ADC. (Not sure because it is random). So, that look like a synchronisation problem, like a mutex who block forever or something like that.

For the test, a MCP3462 was used but I think that any spi device can be used instead. 
