/*
 * fs20.h
 *
 *  Created on: 27.03.2016
 *      Author: sn
 */

#ifndef SRC_FS20_H_
#define SRC_FS20_H_

#include "mbed.h"
#include "queue.h"

typedef struct
{
	uint16_t	hc;
	uint16_t	addr;
	uint16_t	cmd;
} fs20data_t;

class FS20
{
public:
	FS20(InterruptIn &inData);
	void startReceiver();
	bool getData(fs20data_t *data);
	void sendCmd(uint16_t house, uint8_t addr, uint8_t cmd);

	void txOn(void (*fptr)(void));
    template<typename T>
    void txOn(T* tptr, void (T::*mptr)(void)) {
        _txOn.attach(tptr, mptr);
    }

	void txOff(void (*fptr)(void));
    template<typename T>
    void txOff(T* tptr, void (T::*mptr)(void)) {
        _txOff.attach(tptr, mptr);
    }


private:
	int tStart;			// rising edge timestamp
	int bitcount;
	int bytecount;
	int th;				// pulse time
	int tp;				// period time
	bool fsrec; 		// flag: fs20 data receive is active
	bool fspre; 		// flag: preambel receive is active
	uint8_t fssend; 	// no of fs20 bytes to receive
	uint8_t fsseq[6]; 	// received raw FS20 sequence
	uint32_t recbyte;
	Callback<void()> _txOn;
    Callback<void()> _txOff;

	InterruptIn &_inData;
	Timer	timer;
	Queue	receivedCodes;

	void StartPulseHandler();				// handler for rising edge Interrupt
	void StopPulseHandler();				// handler for falling edge Interrupt
	void resetSequence();					// invalid code received, exit statemachine
	void addBit();
	void sendBits(uint16_t data, uint8_t bits);
	uint8_t parity_even_bit(uint8_t d);

};



#endif /* SRC_FS20_H_ */
