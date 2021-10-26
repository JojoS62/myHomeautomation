/*
 * Relay.cpp
 *
 *  Created on: 08.05.2017
 *      Author: sn
 */

#include "Relay.h"


Relay::Relay(int index, PinName pinRelay) :
	RCObject(),
	_outRelay(pinRelay)
{
	strcpy(_deviceName, "Relay");
	_deviceName[5] = 0x30 + index;
	_deviceName[6] = 0;
}

bool Relay::onMessageReceived(Json &json)
{
	// check if this is my address
	int addressKeyIndex = json.findKeyIndexIn ( _deviceName, 0 );
	if ((addressKeyIndex) < 1)
		return false;

	// get cmd value
	int cmdKeyIndex = json.findKeyIndex("cmd", addressKeyIndex);
	int cmdValueIndex = json.findChildIndexOf ( cmdKeyIndex, -1 );
	int cmdValue = 0;
	if (json.tokenIntegerValue(cmdValueIndex, cmdValue) != 0)
		return false;

	_outRelay = cmdValue;

	return false;
}

void Relay::sendMessage(const char* msg, int msgLength)
{
}



