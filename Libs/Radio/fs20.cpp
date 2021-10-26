/*
 * fs20.cpp
 *
 *  Created on: 27.03.2016
 *      Author: sn
 */

#include "fs20.h"

// FS20 Time constants in µs
#define T1	 40
#define T2	 1000
#define T3	 1500
#define T4	 2500
#define TMAX 60000

uint8_t FS20::parity_even_bit(uint8_t d)
{
  int i,j=0;
  for (i = 0; i < 8; i++)
    if (d & (1 << i))
      j++;
  return j&1;
}

FS20::FS20(InterruptIn &inData) :
	_inData(inData),
	receivedCodes(sizeof(fs20data_t), 5)
{
	_inData.mode(PullUp);
	bitcount = 0;
	bytecount = 0;
	recbyte = 0;
}

void FS20::startReceiver()
{
	fssend = 5;
	resetSequence(); // Variablen initialisieren

	timer.start();

	_inData.rise(callback(this, &FS20::StartPulseHandler));
	_inData.fall(callback(this, &FS20::StopPulseHandler));
}

void FS20::StopPulseHandler()
{
	int t = timer.read_us();

	th = t - tStart;
	tStart = t;
}

void FS20::StartPulseHandler()
{
	int t = timer.read_us();

	int tDiff = t - tStart;
	tStart = t;

	if (th < T1)		//*** Stör-Peak detected
		return;

	tp = th + tDiff;	// calc pulse period
	th = 0;

	// Beginn der Auswertung
	if (tp > T4)				// drop pause bit
		return;

	if (tp < T1)				// suppress short peaks
		return;

	if (tp < T2)
	{
		//*** FS20: 0 wurde erkannt
		if (!fsrec)
		{
			fsrec = true;			// Empfang der FS20-sequenz beginnt
			fspre = true;
			//OCR1A = tstart + TMAX;	// set timer for max sequence length
			//TIMSK |= (1<<OCIE1A);
		}

		if (fspre)
		{
			bitcount++;		// Präambel-Bit empfangen
			return;
		}

		addBit();
		return;
		//*** Ende: FS20: 0 wurde erkannt
	}

	if (tp < T3)
	{
		//*** FS20: 1 wurde erkannt
		if (fsrec)
		{
			if (!fspre)
			{
				// 1 kommt in Prdambel nicht vor
				addBit();
				return;
			}

			if (bitcount > 6)
			{
				// Ende der Präambel
				//PORTB |= (1<<PB5); // REC Signalisierung einschalten
				fspre = false;
				bitcount = 0;

				return;
			}
		}

		resetSequence();

		return;
		//*** Ende: FS20: 1 wurde erkannt
	}

}

void FS20::resetSequence()
{
	if (fsrec && !fspre)
	{
		// PORTB |= (1<<PB5); // REC Signalisierung einschalten
	}

	fsrec = false;
	fspre = false;
	bitcount = 0;
	bytecount = 0;
	th = 0;
	tp = 0;
	timer.reset();
}

void FS20::addBit()
{
	if (bitcount == 8)		// byte complete?
	{

		if (parity_even_bit(recbyte) != (tp >= T2))		// parity check
		{
			resetSequence();					// parity error
			return;
		}
		fsseq[bytecount++] = recbyte;	// store received byte
		bitcount = 0;

		if (bytecount == 4)
		{
			fssend = 5;
			if (recbyte & 0x20)
			{
				fssend++;			// if extension byte set
			}
			return;
		}

		if (bytecount == fssend)
		{
			int sum = 6;
			int i = 0;

			// calc checksum
			do
			{
				sum += fsseq[i++];
			} while (i < fssend - 1);

			if (fsseq[i] == sum)		// if checksum ok
			{
				// enquue received code
				fs20data_t	d;
				d.hc = ((uint16_t)fsseq[1] << 8) + fsseq[0];
				d.addr = fsseq[2];
				d.cmd = fsseq[3];
				if (d.cmd & 0x20)
					d.cmd += (fsseq[4] << 8);
				receivedCodes.PutIrq(&d);
			}

			resetSequence();				// reset sequence
		}
		return;
	}

	recbyte <<= 1;

	if (tp >= T2)
		recbyte |= 1; // 1 reinschieben

	bitcount++;
}

bool FS20::getData(fs20data_t *data)
{
	return receivedCodes.Get(data);
}

void FS20::sendBits(uint16_t data, uint8_t bits)
{
    if (bits == 8)
    {
        ++bits;
        data = (data << 1) | parity_even_bit(data);
    }

    for (uint16_t mask = 1 << (bits-1); mask != 0; mask >>= 1)
    {
        // Timing values empirically obtained, and used to adjust for on/off
        // delay in the RF12. The actual on-the-air bit timing we're after is
        // 600/600us for 1 and 400/400us for 0 - but to achieve that the RFM12B
        // needs to be turned on a bit longer and off a bit less. In addition
        // there is about 25 uS overhead in sending the on/off command over SPI.
        // With thanks to JGJ Veken for his help in getting these values right.
        int width = data & mask ? 600 : 400;
        _txOn.call();
        //wait_us(width + 100);
        wait_us(width);
        _txOff.call();
        //wait_us(width - 100);
        wait_us(width);
    }
}

void FS20::sendCmd(uint16_t house, uint8_t addr, uint8_t cmd)
{
	uint8_t sum = 6 + (house >> 8) + house + addr + cmd;
	for (uint8_t i = 0; i < 3; ++i)
	{
		sendBits(1, 13);
		sendBits(house >> 8, 8);
		sendBits(house, 8);
		sendBits(addr, 8);
		sendBits(cmd, 8);
		sendBits(sum, 8);
		sendBits(0, 1);
		wait_ms(10);
	}
}

void FS20::txOn(void (*fptr)(void)) {
    if (fptr) {
        _txOn = fptr;
    } else {
        _txOn = NULL;
    }
}

void FS20::txOff(void (*fptr)(void)) {
    if (fptr) {
        _txOff = fptr;
    } else {
        _txOff = NULL;
    }
}

