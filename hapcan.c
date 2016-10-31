#include "hapcan.h"
extern struct connectivity global;
void dbgprintf (int , const char*, ...);

void HCRelay::init(std::string addr) {
	this->addr = addr;
	std::string topic = ""; std::string data = "";
	// common
	topic = "/devices/hc-" + addr + "/meta/name";
	dbgprintf(3, "HCRelay %s\n", topic.c_str());
	data = "hapcan relay " + addr;
	mosquitto_publish(global.mosq, NULL, topic.c_str(), data.size(), data.c_str(), 0, true);
	topic = "/devices/hc-" + addr + "/controls/+/on";
	dbgprintf(3, "HCRelay %s\n", topic.c_str());
	mosquitto_subscribe(global.mosq, NULL, topic.c_str(), 0);
	// per control
	for(auto i = 0; i < 6; i++) {
		topic = "/devices/hc-" + addr + "/controls/Relay_" + std::to_string(i) + "/meta/type";
		dbgprintf(3, "HCRelay, %s\n", topic.c_str());
		data = "switch";
		mosquitto_publish(global.mosq, NULL, topic.c_str(), data.size(), data.c_str(), 0, true);
		topic = "/devices/hc-" + addr + "/controls/Relay_" + std::to_string(i) + "/meta/order";
		data = std::to_string(i);
		mosquitto_publish(global.mosq, NULL, topic.c_str(), data.size(), data.c_str(), 0, true);
	}
};

HCRelay::~HCRelay() {
	std::string topic = "/devices/hc-" + addr + "/controls/+/on";
	mosquitto_unsubscribe(global.mosq, NULL, topic.c_str());
	// printf("dstructor\n");
};

void HCRelay::processHapCan(struct hapcan_frame* hcf){
	if(hcf->type == 0x302) {// relay coil status
		__u8 coilno = hcf->data[2] -1; // in capcan numbers 1 to 6
		__u8 status = hcf->data[3];
		if((bool)coil[coilno].curr != (bool)status) {// при изменении статуса обновляем
			std::string topic = "/devices/hc-" + addr + "/controls/Relay_" + std::to_string(coilno);
			std::string data = std::to_string((bool)status);
			dbgprintf(3, "HCRelay %s %s\n", topic.c_str(), data.c_str());
			mosquitto_publish(global.mosq, NULL, topic.c_str(), data.size(), data.c_str(), 0, true);
		}
		coil[coilno].curr = (bool)status;
	}
};
int char2int(char c) {
	if('0' <= c && c <= '9') return c - '0';
	if('a' <= c && c <= 'f') return c - 'a' + 10;
	return 0x1;
}
void HCRelay::processMqt(std::string topic, std::string data) {
	// '/devices/hc-06eb/controls/Relay_2/on'
	__u8 cno = topic[32] - '0'; cno = 0x1 << cno;
	
	__u8 node  = (char2int(addr[0]) << 4) + char2int(addr[1]);
	__u8 group = (char2int(addr[2]) << 4) + char2int(addr[3]);
	__u8 instr = data == "1" ? 1 : 0;
	hapcan_write(global.can, 0x10A, 0xF0, 0xF0,  
		instr, cno, // isntr channel
		node, group, // node group
		0x00, //timer
		0xFF, 0xFF, 0xFF );
};
