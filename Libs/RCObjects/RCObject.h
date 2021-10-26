/*
 * RCObject.h
 *
 *  Created on: 27.11.2016
 *      Author: sn
 */

#ifndef SRC_RCOBJECT_H_
#define SRC_RCOBJECT_H_

#include "Json.h"

class RCObject {
public:
	RCObject();

	virtual bool onMessageReceived(Json &json) { return false; };
	virtual void sendMessage(const char* msg, int msgLength) {};
};


#endif /* SRC_RCOBJECT_H_ */
