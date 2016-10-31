#include <stdlib.h>
#include <signal.h>

#include <string>
#include <map>

#include <sys/time.h>
#include <sys/types.h>

#include <mosquitto.h>

#include "glob.h"
#include "can.h"
#include "device.h"
#include "hapcan.h"

struct connectivity global;
void dbgprintf(int sever, const char* fmt, ...) {
	if (global.verbose < sever) return;
	va_list argptr;
	va_start(argptr, fmt);
	vprintf(fmt, argptr);
	va_end(argptr);
}
/*
Стандартов шин несколько и их проще захардкодить.
Если мы знаем что это hapcan - то мы знаем и адрес в device list.
Пока нет вычленения инфы - будут устройства будем думать.
*/
int hapcan_process(struct hapcan_frame* hcf) { // uses mosquitto & device list
	if(hcf->type != 0x302) return 0; // только поддерживаемые фреймы
	// чтобы не перебирать все устройства угадаем номер
	char addr[5]; sprintf(addr, "%02x%02x", hcf->module, hcf->group);
	auto it = global.devices.find(addr);
	Device *device;
	if( it == global.devices.end() ) { // нужен новый класс
		if(hcf->type == 0x302) {// relay coil status
			device = global.devices[addr] = new HCRelay();
			device -> init(addr);
		}
	} else {
		device = it->second;
	}
	device->processHapCan(hcf);
	return 0;
}

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
	bool match = 0;
	dbgprintf(1, "got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);
	// '/devices/hc-06eb/controls/Relay_2/on'
	if(strncmp(&(message->topic[8]), "/hc-", 4) == 0) {// наш клиент
		// поиск адреса
		std::string addr(message->topic, 12,4);
		auto it = global.devices.find(addr);
		if( it != global.devices.end() ) 
			it->second->processMqt(message->topic, std::string((char*) message->payload, 1));
	}
}
int mqt_open(struct mosquitto** mosq, const char* host, int port) {
	mosquitto_lib_init();
	*mosq = mosquitto_new("wb-can~icukeng1", 0,0);//save session 
	mosquitto_message_callback_set(*mosq, message_callback);
	mosquitto_connect(*mosq, host, port, 60);
	return mosquitto_socket(*mosq);
}

static int run = 1;
void handle_signal(int s) { run = 0; }

static int init = 1;

int main(int argc, char **argv) {
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	int mqtt_port = 1883;
	std::string mqtt_host = "localhost";
	// -------------------------------
	int c;
	while ( (c = getopt(argc, argv, "h:p:v:")) != -1) {
		switch (c) {
			case 'p':
				printf ("option p with value '%s'\n", optarg);
				mqtt_port = std::stoi(optarg);
				break;
			case 'h':
				printf ("option h with value '%s'\n", optarg);
				mqtt_host = optarg;
				break;
			case 'v':
				global.verbose =  std::stoi(optarg);
				break;
			case '?': break;
			default:
				printf ("?? getopt returned character code 0%o ??\n", c);
		}
	}	
	// -------------------------------


	int can_sock = global.can = can_open();
	if(can_sock < 0) return 1;

	int mqt_sock = mqt_open(&global.mosq, mqtt_host.c_str(), mqtt_port);
	if(mqt_sock < 0) return 1;

	/* the loop */
	fd_set rfds;
	fd_set wfds;
	struct timeval tv;
	int retval;
	struct hapcan_frame hcf;
	int rc = 0;
	while(run) {
		if(init == 1) {
			init = 0;
			hapcan_write(can_sock, 0x105, 0xF0, 0xF0,  0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF );
			hapcan_write(can_sock, 0x108, 0xF0, 0xF0,  0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF );
		}
		FD_ZERO(&rfds);
		FD_SET(mqt_sock, &rfds);
		FD_SET(can_sock, &rfds);
		FD_ZERO(&wfds);
		if(mosquitto_want_write(global.mosq))
			FD_SET(mqt_sock, &wfds);
		tv.tv_sec = 2; tv.tv_usec = 0;
		dbgprintf(3, "socket %i %i \n", can_sock, mqt_sock);
		retval = select(mqt_sock + 1, &rfds, &wfds, NULL, &tv);
		if (retval == -1) {
			perror("select()");
			exit(1);
		}
		if(FD_ISSET(can_sock, &rfds)) {
			hapcan_read(can_sock, &hcf);
			hapcan_process(&hcf);
		}
		if(FD_ISSET(mqt_sock, &rfds) || FD_ISSET(mqt_sock, &wfds)) {
			if(FD_ISSET(mqt_sock, &rfds))	rc = mosquitto_loop_read(global.mosq, 1);
			if(FD_ISSET(mqt_sock, &wfds)) rc = mosquitto_loop_write(global.mosq, 1);
			dbgprintf(3, "rc %i\n", rc);
			if(MOSQ_ERR_CONN_LOST == rc) {
				mosquitto_reconnect(global.mosq);
				mqt_sock = mosquitto_socket(global.mosq);
			}
			mosquitto_loop_misc(global.mosq);
			dbgprintf(3,"mqt\n");
		}
		dbgprintf(3,"Timeloop\n");
	}
	mosquitto_destroy(global.mosq);
	mosquitto_lib_cleanup();
	exit(EXIT_SUCCESS);
}

