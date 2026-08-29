/*
Driver for Pixart PAJ7025R2 (Wiimote IR Camera)
Adapted lightly by pknessness from https://github.com/ybasviel/wiiIRcam/

Huge help from Kako's blog at http://www.kako.com/neta/2008-009/2008-009.html
*/

#pragma once
#include <esp_err.h>
#include <driver/i2c_master.h>

#define PIXART_ID  0x58
#define PIXART_X_MAX 1023
#define PIXART_X_MIN 0

#define PIXART_Y_MAX 754
#define PIXART_Y_MIN 0

//NOT SURE IF THIS EVEN EXISTS
#define PIXART_IR_WHO_AM_I 0xF 

typedef struct
{
	uint16_t x;
	uint16_t y;
	uint8_t size;
} ir_data_t;

typedef struct {
	i2c_master_dev_handle_t i2c_handle; /*!< I2C device handle for PIXART_IR */
    uint8_t address;
} pixart_ir_handle_t;

esp_err_t pixart_ir_init(i2c_master_bus_handle_t bus_handle, pixart_ir_handle_t *handle);
esp_err_t pixart_ir_get_data(pixart_ir_handle_t *handle);
esp_err_t pixart_ir_set_sensitivity(pixart_ir_handle_t *handle);
esp_err_t pixart_ir_get_id(pixart_ir_handle_t *handle, uint8_t* whoami);


/*
http://www.kako.com/neta/2008-009/2008-009.html

How to use the infrared sensor on a Wii Remote

I'll try to write a simple explanation of how to use the sensor.

In the I2C bus protocol, the device's address number must be sent first in order to communicate with the device.
The address of the infrared sensor used in the Wii Remote is 0x58.
The source code might show 0xB0, which is because the bit is shifted so that the upper 7 bits are the address and the lower 1 bit is the R/W specification bit.


Initializing the sensors accessing the sensors inside the Wii Remote via Bluetooth . is easier than
This is because it does not perform chip enable control or ON/OFF control of the clock oscillation circuit.

Commands for initialization are sent in the following format:
[START condition] [0xB0] [Control register address] [Write data] [STOP condition]
The address of the control register is specified using 8 bits.
The exact function of each address is still not fully understood.
For now, you can make the sensor work by writing to it using a specific set of values ​​and procedure.


Initialization command procedure
(1) Write data 0x01 to control register address 0x30.
(2) Write data 0x08 to control register address 0x30.
(3) Write data 0x90 to control register address 0x06.
(4) Write data 0xC0 to control register address 0x08.
(5) Write data 0x40 to control register address 0x1A.
(6) Write data 0x33 to control register address 0x33.
This appears to be a simplified version of the sensitivity setting procedure described below.


How to set the sensitivity of the infrared sensor
The settings above appear to be simplified, but if you want to configure them properly, specify the four parameters p0, p1, p2, and p3 and write them using the following procedure.
(1) Write data 0x01 to control register address 0x30.
(2) Write data 0x02, 0x00, 0x00, 0x71, 0x01, 0x00, p0 to control register address 0x00 (write 7 bytes)
(3) Write data 0x00,p1 to control register address 0x07 (write 2 bytes)
(4) Write data p2 and p3 to control register address 0x1A (write 2 bytes)
(5) Write data 0x03 to control register address 0x33.
(6) Write data 0x08 to control register address 0x30.
When writing multiple bytes, the register address is automatically incremented, allowing for continuous writing.

The sensitivity parameters are:
When sensitivity is 1, p0=0x72, p1=0x20, p2=0x1F, p3=0x03
For sensitivity 2, p0=0xC8, p1=0x36, p2=0x35, p3=0x03
For sensitivity 3, p0=0xAA, p1=0x64, p2=0x63, p3=0x03
For sensitivity 4, p0=0x96, p1=0xB4, p2=0xB3, p3=0x04
For a sensitivity of 5, p0=0x96, p1=0xFE, p2=0xFE, p3=0x05


After initialization (after setting the sensitivity), the coordinates of the light points detected by the sensor can be read out.

Method for reading sensor output
[START Condition] [0xB0] [0x36] [STOP Condition]
After sending the command, the following is read:
[START condition] [0xB1] [Data read] x 16 bytes


Format of the retrieved data
The data is stored in groups of 3 bytes in the following order.
[First byte] [Coordinate 1... 3 bytes] [Coordinate 2... 3 bytes] [Coordinate 3... 3 bytes] [Coordinate 4... 3 bytes]
　　
If we denote these three bytes as XX, YY, and SS,
X coordinate = (SS & 0x30) << 4 + XX
　　Y座標 = (SS & 0xC0) <<2 + YY
By performing this calculation, we can obtain the coordinate values.

0x3FF is returned if the coordinate values ​​are invalid.


Role of control registers
Address 0x00 to 0x08 --- Parameter settings for detection
Address 0x1A to 0x1B --- Parameter settings for detection (2)
Address 0x30 --- Sensor operating mode (?) First write 0x01, then write 0x08.
Address 0x33 --- Write sensor operating mode (?) 0x03 (or 0x33?) 
*/





