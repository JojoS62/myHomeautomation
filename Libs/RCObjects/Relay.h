/*
 * Relay.h
 *
 *  Created on: 08.05.2017
 *      Author: sn
 */

#ifndef SRC_RELAY_H_
#define SRC_RELAY_H_


#include "mbed.h"
#include "RCObject.h"

class Relay : public RCObject {
public:
	Relay(int index, PinName pinRelay);

	virtual void sendMessage(const char* msg, int msgLength);
	virtual bool onMessageReceived(Json &json);

private:
	DigitalOut 	_outRelay;
	char		_deviceName[8];
};




#endif /* SRC_RELAY_H_ */
