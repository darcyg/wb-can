#ifndef HAPCAN_DEVICE_H
#define HAPCAN_DEVICE_H

#include "glob.h"
#include "can.h"

class HCRelay : public Device{
	struct {
		unsigned int curr :1; // current status by can
		// unsigned int next :1; // next status cheange
	} coil[6]; // coil status bitfield

	void init(std::string addr);
	~HCRelay();

	void processHapCan(struct hapcan_frame* hcf);
	void processMqt(std::string topic, std::string data);
};


#endif