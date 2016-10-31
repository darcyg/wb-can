#ifndef DEVICE_H
#define DEVICE_H

#include <string>

class Device {
	public:
	std::string addr;
	virtual void init(std::string addr) = 0; // sort of constructor

	virtual void processHapCan(struct hapcan_frame* hcf) = 0;
	virtual void processMqt(std::string topic, std::string data) = 0;
};

#endif