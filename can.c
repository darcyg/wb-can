#include "can.h"
void dbgprintf (int , const char*, ...);

int can_open() {
	int can_sock;
	/* open socket */
	if ((can_sock = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("can socket");
		return -1;
	}
	/* find device */
	struct ifreq ifr;
	strcpy(ifr.ifr_name, "can0");
	if (ioctl(can_sock, SIOCGIFINDEX, &ifr) < 0) {
		perror("SIOCGIFINDEX");
		return -1;
	}

	/* bind */
	struct sockaddr_can can_addr;
	can_addr.can_family = AF_CAN;
	can_addr.can_ifindex = ifr.ifr_ifindex;
	if (bind(can_sock, (struct sockaddr *)&can_addr, sizeof(can_addr)) < 0) {
		perror("bind");
		return -1;
	}
	return can_sock;
}

int hapcan_read(int sock, struct hapcan_frame* hcf){
	struct can_frame cf;
	int nbytes = read(sock, &cf, sizeof(struct can_frame));
	if (nbytes < 0) {
		perror("can raw socket read");
		return 1;
	}
	/* paranoid check ... */
	if (nbytes < sizeof(struct can_frame)) {
		perror("read: incomplete CAN frame\n");
		return 1;
	}
	hcf -> type   = (cf.can_id >> 17) & 0xFFF;
	hcf -> flags  = (cf.can_id >> 16) & 0x1;
	hcf -> module = (cf.can_id & 0xFF00) >> 8;
	hcf -> group  = (cf.can_id & 0xFF);
	for(auto i = 0; i < 8; i++) {
		hcf -> data[i] = cf.data[i];
	}
	dbgprintf(1, "readcan type %03x flags %01x addr %02x%02x data %02x%02x%02x%02x%02x%02x%02x%02x\n",
		hcf->type, hcf->flags, hcf->module, hcf->group,
		hcf->data[0], hcf->data[1], hcf->data[2], hcf->data[3],
		hcf->data[4], hcf->data[5], hcf->data[6], hcf->data[7]
	);
	return 0;
}
int hapcan_write(int sock, __u16 type, __u8 module, __u8 group, ...) {
	struct can_frame cf;
	cf.can_id = type;
	cf.can_id <<= 1;
	// without flags. assume 0
	cf.can_id <<= 8; cf.can_id += module;
	cf.can_id <<= 8; cf.can_id += group;
	// extended frame format
	cf.can_id |= CAN_EFF_FLAG;

	cf.can_dlc = 8;
	va_list args;
	va_start(args, group);
	for(auto i = 0; i < 8; i++) {
		cf.data[i] = va_arg(args, int);
	}
	va_end(args);
	dbgprintf(1, "writecan id %08x data %02x%02x%02x%02x%02x%02x%02x%02x\n",
		cf.can_id, 
		cf.data[0], cf.data[1], cf.data[2], cf.data[3],
		cf.data[4], cf.data[5], cf.data[6], cf.data[7]
	);
	int nbytes = write(sock, &cf, sizeof(struct can_frame));
	if (nbytes < sizeof(struct can_frame)) {
		perror("can raw socket write\n");
		return 1;
	}
	return 0;
}
