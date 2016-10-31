#ifndef GLOB_H
#define GLOB_H
	#include <string>
	#include <map>

	#include <mosquitto.h>
	#include "device.h"

	struct connectivity {
		struct mosquitto *mosq;
		int can;
		std::map<std::string, Device*> devices;
		int verbose;
	};
#endif