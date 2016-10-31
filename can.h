#ifndef HAPCAN_H
#define HAPCAN_H

#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

struct hapcan_frame {
	__u16 type; // actually 12 bits
	__u8 flags; // actually 4
	__u8 module;
	__u8 group;
	__u8 data[8] __attribute__((aligned(8)));
};

int can_open();
int hapcan_read(int, struct hapcan_frame*);
int hapcan_write(int, __u16, __u8, __u8, ...);

#endif