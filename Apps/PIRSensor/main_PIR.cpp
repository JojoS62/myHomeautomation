/*
 * main.cpp
 *
 *  Created on: 27.03.2016
 *      Author: sn
 */
#include "mbed.h"
#include "wakeup.h"
#include "rfm69.h"
#include "acmp.h"

//****************************************************************************************************************
//**** IMPORTANT RADIO SETTINGS - YOU MUST CHANGE/CONFIGURE TO MATCH YOUR HARDWARE TRANSCEIVER CONFIGURATION! ****
//****************************************************************************************************************
#define NODEID         40                   //unique for each node on same network 40 Motion 50 Doorbell
#define NETWORKID     100  	                //the same on all nodes that talk to each other
#if __has_include("credentials.h")
#	include <credentials.h>
#else
#define ENCRYPTKEY    "1234567890123456"    //exactly the same 16 characters/bytes on all nodes!
#endif
//#define IS_RFM69HW                        //uncomment only for RFM69HW! Remove/comment if you have RFM69W!
//*********************************************************************************************


#define LED_OFF 	(1)
#define LED_ON		(0)


DigitalIn	btn1(P0_13, PullUp);
DigitalIn	btn2(P0_12, PullUp);
DigitalOut	ledRed(P0_11, LED_OFF);

ACMP		aCmp(ACMP::LADDER, ACMP::BANDGAP, ACMP::NONE, 11);

SPI 	    spi(P0_9, P0_1, P0_15);		// init SPI: mosi, miso, sck, ss
RFM69 	    rfm(spi, P0_8, P0_16);
DigitalIn	motionDetector(P0_4, PullNone);

bool goSleep = true;

//RawSerial uart(P0_4, P0_0);
void progMode() {
    while (btn2) {
        ledRed = LED_ON;
        wait_ms(100);
        ledRed = LED_OFF;
        wait_ms(100);
    }
}


int main()
{
    //progMode();

	// test blinky
	if (!WakeUp::wokeUpFromDeepSleep())
	{
		for(int i=0; i<3; i++)
		{
			ledRed = LED_ON;
			wait_ms(50);
			ledRed = LED_OFF;
			wait_ms(50);
			ledRed = LED_ON;
			wait_ms(50);
			ledRed = LED_OFF;
		}
	}

	//uart.puts("FS20-PIR V0.1\r\n");

	// init rf module for OOK receive operation
	rfm.initialize(RF69_868MHZ, NODEID, NETWORKID);
#ifdef IS_RFM69HW
	rfm.setHighPower(true);
#endif
	rfm.encrypt(ENCRYPTKEY);
	rfm.promiscuous(false);

	const char* msgMotion 	= "{\"Doorbell\":{\"cmd\":\"ring\"}}";
	const char* msgLowPower	= "{\"Doorbell\":{\"status\":\"lowPower\"}}";


	if (WakeUp::wokeUpFromDeepSleep())					// triggered by PIR signal
	{
		// clear dpdFlag
	    uint32_t regVal = LPC_PMU->PCON;
	    regVal |= (1U << 11);
	    LPC_PMU->PCON = regVal;

		rfm.send(1, msgMotion, strlen(msgMotion));

        ledRed = LED_ON;
		wait_ms(2);
		ledRed = LED_OFF;

		// check battery
	    bool lowPower = !aCmp.read();
		if (lowPower)
		{
			wait_ms(100);
			rfm.send(1, msgLowPower, strlen(msgLowPower));
		}
	}
    WakeUp::clearDeepPowerDownFlag();

    // check for prog mode
    if (btn1 == 0) {
        progMode();
    }

    // if btn1 is pressed, skip power down mode
	// for debugger or testing
	goSleep = btn1;
	if (goSleep)
	{
		wait_us(50);
		rfm.sleep();
		wait_us(50);

		WakeUp::deepPowerDown();		// cpu standby
		// after wakeup, a reset is performed
	}

}
