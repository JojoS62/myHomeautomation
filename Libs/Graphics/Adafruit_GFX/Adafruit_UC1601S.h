/*********************************************************************
This is a library for our Monochrome OLEDs based on SSD1306 drivers


Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/

/*
 *  Modified by JojoS for use in mbed
 */

#ifndef _ADAFRUIT_UC1601S_H_
#define _ADAFRUIT_UC1601S_H_

#include "mbed.h"
#include "Adafruit_GFX.h"

#include <vector>
#include <algorithm>

/** The pure base class for the UC1601S display driver.
 *
 * You should derive from this for a new transport interface type,
 * such as the SPI and I2C drivers.
 */

#define LCD_SET_COLUMN_ADDR_LSB 	0x00
#define LCD_SET_COLUMN_ADDR_MSB 	0x10
#define LCD_SET_TEMP_COMP 			0x24
#define LCD_SET_POWER_CTRL 			0x28
#define LCD_SET_LINE_ADDR 			0x40
#define LCD_SET_PAGE_ADDR 			0xB0
#define LCD_SET_BIAS 				0x81
#define LCD_SET_BIAS_RATIO			0xE8
#define LCD_SET_PARTITIAL_CTRL		0x84
#define LCD_ENABLE_DISPLAY 			0xAE
#define LCD_ENABLE_ALL 				0xA5
#define LCD_INVERT_DISPLAY 			0xA6
#define LCD_SYSTEM_RESET 			0xE2
#define LCD_SET_RAM_ADDRESS_CTRL	0x88
#define LCD_SET_FRAME_RATE			0xA0
#define LCD_SET_MAPPING_CTRL		0xC0
#define LCD_SET_COM_END				0xF1


class Adafruit_UC1601S : public Adafruit_GFX
{
public:
	Adafruit_UC1601S(PinName reset, uint8_t rawHeight = 22, uint8_t rawWidth = 132, bool flipVertical=false);

	// start sequence
	void begin();
	
	// These must be implemented in the derived transport driver
	virtual void command(uint8_t c) = 0;
	virtual void data(const uint8_t *c, int count) = 0;
	virtual void drawPixel(int16_t x, int16_t y, uint16_t color);

	/// Clear the display buffer    
	void clearDisplay(void);
	virtual void invertDisplay(bool i);
	void flipVertical(bool flip);

	/// Cause the display to be updated with the buffer content.
	void display();
	/// Fill the buffer with the AdaFruit splash screen.
	virtual void splash();
    
protected:
	virtual void sendDisplayBuffer() = 0;
	DigitalOut _reset;
	bool _flipVertical;

	// the memory buffer for the LCD
	std::vector<uint8_t> buffer;
};



/** This is the I2C UC1601S display driver transport class
 *
 */

#define I2C_ADDRESS_CMD     (0x38 << 1)
#define I2C_ADDRESS_DATA    (0x39 << 1)

class Adafruit_UC1601S_I2c : public Adafruit_UC1601S
{
public:
	/** Create a SSD1306 I2C transport display driver instance with the specified RST pin name, the I2C address, as well as the display dimensions
	 *
	 * Required parameters
	 * @param i2c - A reference to an initialized I2C object
	 * @param RST - The Reset pin name
	 *
	 * Optional parameters
	 * @param i2cAddress - The i2c address of the display
	 * @param rawHeight - The vertical number of pixels for the display, defaults to 22
	 * @param rawWidth - The horizonal number of pixels for the display, defaults to 128
	 */
	Adafruit_UC1601S_I2c(I2C &i2c, PinName reset, uint8_t i2cAddress = I2C_ADDRESS_CMD, uint8_t rawHeight = 22, uint8_t rawWidth = 132, bool flipVertical = false);

	virtual void command(uint8_t c);
	virtual void data(const uint8_t *c, int count);

protected:
	virtual void sendDisplayBuffer();
	I2C &mi2c;
	uint8_t mi2cAddress;
};

#endif
