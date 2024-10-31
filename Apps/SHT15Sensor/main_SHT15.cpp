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
#include "json.h"
#include "Relay.h"
#include "sht31.h"

//****************************************************************************************************************
//**** IMPORTANT RADIO SETTINGS - YOU MUST CHANGE/CONFIGURE TO MATCH YOUR HARDWARE TRANSCEIVER CONFIGURATION! ****
//****************************************************************************************************************
#define NODEID         31                   //unique for each node on same network
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
ACMP acmp(ACMP::LADDER, ACMP::BANDGAP, ACMP::NONE, 11);

SPI     spi(P0_9, P0_1, P0_15);		// init SPI: mosi, miso, sck, ss
RFM69   rfm(spi, P0_8, P0_16);

//Sht31   shtSensor(P0_14, P0_7);
//SHTx::SHT15 shtSensor(P0_7, P0_6);

//Json json(10);		// Json object for received messages

//Relay relay1(1, P0_4);
//Relay relay2(2, P0_0);

bool goSleep = !false;

const char* msgHello = "{\"Sensor\":\"Hello\"}";
const char* msgBtn1 = "{\"Switch\":{\"cmd\":\"btn1\"}}";
const char* msgBtn2 = "{\"Switch\":{\"cmd\":\"btn2\"}}";
const char* msgLowPower = "{\"Battery\":{\"status\":\"lowPower\"}}";
//const char* msgTemperature = "{\"Sensor\":{\"Temp\":%i, \"Hum\":%i}}";
//const char* msgTemperature = "{"Sensor":{"Temp":     , "Hum":     }}";
const char* msgTemperature = "{\"Sensor\":{\"Temp\":     , \"Hum\":     }}";


/* A utility function to reverse a string  */
void reverse(char str[], int length)
{
    int start = 0;
    int end = length;
    char c;
    while (start < end)
    {
        //swap(*(str+start), *(str+end));
        c = *(str+end);
        *(str+end) = *(str+start);
        *(str+start) = c;
        start++;
        end--;
    }
}

// Implementation of itoa()
char* itoa_dec(int num, char* str)
{
    int i = 0;
    bool isNegative = false;

    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num == 0)
    {
        str[i] = '0';
        //str[i++] = '0';
        //str[i] = '\0';
        return str;
    }

    // In standard itoa(), negative numbers are handled only with
    // base 10. Otherwise numbers are considered unsigned.
    if (num < 0)
    {
        isNegative = true;
        num = -num;
    }

    // Process individual digits
    while (num != 0)
    {
        int rem = num % 10;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/10;
    }

    // If number is negative, append '-'
    if (isNegative)
        str[i++] = '-';

    //str[i] = '\0'; // Append string terminator

    // Reverse the string
    reverse(str, i-1);

    return str;
}

void progMode() {
    while (btn2) {
        ledRed = LED_ON;
        wait_ms(100);
        ledRed = LED_OFF;
        wait_ms(100);
    }
}

int main() {
    char msg[64];

    // test blinky
    //if (!WakeUp::wokeUpFromDeepSleep()) {
        ledRed = LED_ON;
        wait_ms(50);
        ledRed = LED_OFF;
        wait_ms(50);
        ledRed = LED_ON;
        wait_ms(50);
        ledRed = LED_OFF;

        while(1){
            rfm.send(1, msgHello, strlen(msgHello));
            wait_ms(5000);
        }
   // }
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

    // configure sht temperature sensor

    // send temperature and humidity
    {
        // int t = (int) (shtSensor.readTemperature() * 100.0f);
        // int h = (int) (shtSensor.readHumidity() * 100.0f);

        strcpy(msg, msgTemperature);
        itoa_dec(t, &msg[18]);
        itoa_dec(h, &msg[31]);

        //sprintf(msg, "test %d", t);
        rfm.send(1, msg, strlen(msg));

        ledRed = LED_ON;
        wait_ms(1);
        ledRed = LED_OFF;
    }

#if 1
    // check supply voltage
    {
        bool lowPower = !acmp.read();
        if (lowPower) {
            wait_ms(50);
            rfm.send(1, msgLowPower, strlen(msgLowPower));
        }
    }
#endif

    // if btn1 is pressed, skip power down mode
    // for debugger or testing
    while (1) {
        goSleep = btn1;
        if (goSleep) {
            wait_us(50);
            rfm.sleep();
            wait_us(50);

            //WakeUp::calibrate();
            WakeUp::set(60);                // set wakeup time to 90 s
            WakeUp::deepPowerDown();        // cpu standby
            // after wakeup, a reset is performed
        }
    }

}
