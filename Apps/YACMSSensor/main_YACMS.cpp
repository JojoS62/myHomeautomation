/*
 * main.cpp
 *
 *  Created on: 27.03.2016
 *      Author: JojoS
 */
#include "mbed.h"
#include "wakeup.h"
#include "rfm69.h"
#include "acmp.h"
#include "json.h"
#include "Relay.h"
#include "sht15.h"

//****************************************************************************************************************
//**** IMPORTANT RADIO SETTINGS - YOU MUST CHANGE/CONFIGURE TO MATCH YOUR HARDWARE TRANSCEIVER CONFIGURATION! ****
//****************************************************************************************************************
#define NODEID         40                   //unique for each node on same network
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

DigitalIn btn1(P0_13, PullUp);
DigitalIn btn2(P0_12, PullUp);
DigitalOut ledRed(P0_11, LED_OFF);
DigitalInOut outCharge(P0_4, PIN_OUTPUT, PullNone, 0);

ACMP aCmp(ACMP::ACMP_I1, ACMP::LADDER, ACMP::NONE, 16);

SPI spi(P0_9, P0_1, P0_15);     // init SPI: mosi, miso, sck, ss
RFM69 rfm(spi, P0_8, P0_16);

Timer timerChargetime;

bool goSleep = !false;
volatile bool measuring = false;

//const char* msgHello = "{\"Sensor\":\"Hello\"}";
//const char* msgBtn1 = "{\"Switch\":{\"cmd\":\"btn1\"}}";
//const char* msgBtn2 = "{\"Switch\":{\"cmd\":\"btn2\"}}";
const char* msgLowPower = "{\"status\":\"lowPower\"}";
const char* msgHumidity = "{\"Hum\":       }";

/* A utility function to reverse a string  */
void reverse(char str[], int length) {
    int start = 0;
    int end = length;
    char c;
    while (start < end) {
        //swap(*(str+start), *(str+end));
        c = *(str + end);
        *(str + end) = *(str + start);
        *(str + start) = c;
        start++;
        end--;
    }
}

/**
 * itoa_dec
 *
 * itoa implementation to save code space. Support only decimal based values.
 *
 * @param num	in, numeric value to convert
 * @param str	out, pointer to string with sufficient space must be provided
 *
 * @return		pointer to string with converted value
 */
char* itoa_dec(int num, char* str) {
    int i = 0;
    bool isNegative = false;

    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num == 0) {
        str[i] = '0';
        //str[i++] = '0';
        //str[i] = '\0';
        return str;
    }

    // In standard itoa(), negative numbers are handled only with
    // base 10. Otherwise numbers are considered unsigned.
    if (num < 0) {
        isNegative = true;
        num = -num;
    }

    // Process individual digits
    while (num != 0) {
        int rem = num % 10;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / 10;
    }

    // If number is negative, append '-'
    if (isNegative)
        str[i++] = '-';

    //str[i] = '\0'; // Append string terminator

    // Reverse the string
    reverse(str, i - 1);

    return str;
}

/**
 * isrOverAnalogLevel
 *
 * interrupt service routine, called when comparator threshold is reached
 *
 */
void cbOverAnalogLevel() {
    //timerChargetime.stop();
    //outCharge = 0;
	measuring = false;
}

/**
 * readSensor
 *
 * starts a measuring sequence and return the digital measurd value.
 *
 * @return charging time in microseconds
 */
int readSensor() {
    outCharge = 0;              	// stop capacitor charging
    wait_ms(2);						// discharge time

    timerChargetime.reset();    	// start timer, will be stopped in ISR
    timerChargetime.start();

    measuring = true;
    outCharge = 1;              	// start capacitor charging

//    while (measuring) {
//    };
    while (aCmp.read() == 0)
    	;
    //wait_ms(2);					// max. charge time
    timerChargetime.stop();
    outCharge = 0;              	// stop capacitor charging
    return timerChargetime.read_us();
#if 0
    if (outCharge == 0) {			// measure ok
        return timerChargetime.read_us();
    }
    else {							// timout
        //outCharge = 0;              // stop capacitor charging
        //timerChargetime.stop();
        return -1;					// return error
    }
#endif
}

/**
 * progMode
 *
 * simply indicate programming mode. Useful for debugging and SWM programming when device
 * is in deep power down mode. Enter prog mode and wait in an endless loop for attaching
 * the debugger.
 */
void progMode() {
    while (btn2) {
        ledRed = LED_ON;
        wait_ms(100);
        ledRed = LED_OFF;
        wait_ms(100);
    }
}

/**
 * main
 *
 * read capacitve sensor and send measure result as string over RFM69 module.
 *
 * @return
 */
int main() {
    char msg[64];

    // test blinky
    if (!WakeUp::wokeUpFromDeepSleep()) {
        ledRed = LED_ON;
        wait_ms(50);
        ledRed = LED_OFF;
        wait_ms(50);
        ledRed = LED_ON;
        wait_ms(50);
        ledRed = LED_OFF;
    }
    WakeUp::clearDeepPowerDownFlag();
    WakeUp::set_ms(0);

    // check for prog mode
    if (btn1 == 0) {
        progMode();
    }

    // init rf module
    rfm.initialize(RF69_868MHZ, NODEID, NETWORKID);
#ifdef IS_RFM69HW
    rfm.setHighPower(true);
#endif
    rfm.encrypt(ENCRYPTKEY);
    rfm.promiscuous(false);

    // attach ISR to analog comparator
    aCmp.set(ACMP::ACMP_I1, ACMP::LADDER, ACMP::NONE, 16);
    wait_ms(10);
    //aCmp.rise(&cbOverAnalogLevel);

    //outCharge.output();
    //outCharge = 0;

    ledRed = LED_ON;
    int avgCount = 16;
    int sum = 0;
    int count = 0;
    while ( count < avgCount) {
    	int val = readSensor();
    	if (val >= 0) {
            sum += val;
            count++;
    	}
    }
    int humidity = sum / avgCount;
    ledRed = LED_OFF;

    // send humidity
    {
        strcpy(msg, msgHumidity);
        itoa_dec(humidity, &msg[8]);

        rfm.send(1, msg, strlen(msg));

        //ledRed = LED_ON;
        wait_ms(2);
        //ledRed = LED_OFF;
    }

    aCmp.rise(0);           // disable comparator int

    //outCharge = 1;          // disable outCharge pin, necessary for wake up function
    outCharge.input();
    //outCharge.mode(PullNone);


#if 0
	// check battery
    {
    	aCmp.set(ACMP::LADDER, ACMP::BANDGAP, ACMP::NONE, 11);
    	wait_ms(50);
        bool lowPower = !aCmp.read();
        if (lowPower) {
            rfm.send(1, msgLowPower, strlen(msgLowPower));

            ledRed = LED_ON;
            wait_ms(10);
            ledRed = LED_OFF;
            wait_ms(50);
            ledRed = LED_ON;
            wait_ms(10);
            ledRed = LED_OFF;
        }
    }
#endif

    // if btn1 is pressed, skip power down mode
    // for debugger or testing
    {
        goSleep = btn1;
        if (goSleep) {
            wait_us(50);
            rfm.sleep();
            wait_us(50);

            //WakeUp::calibrate();
            WakeUp::set(10);                // set wakeup time to 10 s
            WakeUp::deepPowerDown();        // cpu standby
            // after wakeup, a reset is performed
        }
    }

}
