/*
 * This file is part of USBProxy.
 */

#include <unistd.h>
#include <stdio.h>
#include <memory.h>
#include <poll.h>
#include "errno.h"
#include "netinet/in.h"

extern "C" {
#include <linux/input.h>
}

#include "Injector_EvDev.h"
#include "Packet.h"
#include "HexString.h"

Injector_EVDEV::Injector_EVDEV(ConfigParser *cfg) {
	input_device = "/dev/input/event1";
	/* TODO: get it from cfg
	* std::string input_device = cfg->get("Injector_EVDEVHID::port");
	*/
	if(input_device == "") {
		fprintf(stderr, "Error: no device found for Injector_EVDEV\n");
		return;
	}
	
	input_fd=0;
	spoll.events=POLLIN;
}

Injector_EVDEV::~Injector_EVDEV() {
	if (input_fd) {close(input_fd);input_fd=0;}
}

void Injector_EVDEV::start_injector() {
	fprintf(stderr,"Opening injection EVDEV file on device %s.\n",input_device.c_str());
	input_fd=open(input_device.c_str(), O_RDONLY);
	spoll.fd=input_fd;
	if (input_fd<0) {
		fprintf(stderr,"Error opening device.\n");
		input_fd=0;
	}
}

int* Injector_EVDEV::get_pollable_fds() {
	int* tmp=(int*)calloc(2,sizeof(int));
	tmp[0]=input_fd;
	return tmp;
}

void Injector_EVDEV::stop_injector() {
	if (input_fd) {close(input_fd);input_fd=0;}
}
void Injector_EVDEV::get_packets(Packet** packet,SetupPacket** setup,int timeout) {
	*packet=NULL;
	*setup=NULL;
	struct input_event ev;

	if (!poll(&spoll, 1, timeout) || !(spoll.revents&POLLIN)) {
		return;
	}

	ssize_t len=read(input_fd,&ev,sizeof(ev));
	if (len<0) {
		fprintf(stderr,"input device read error [%s].\n",strerror(errno));
		return;
	}
	
	if (len==sizeof(ev)) {
		if (ev.type != EV_KEY) return;
		if (ev.code != 1 && ev.code != 2) return;

		fprintf(stderr, "input: code %s %s\n",
				ev.code == 1 ? "<0>":"<1>",
				ev.value == 0 ? "key-released":"key-pressed");

		/*TODO fill-out USB packets*/
	}
	else {
		fprintf(stderr,"input device short read: %d, expected: %d \n", len, sizeof(ev));
		return;
	}

	return;
}

void Injector_EVDEV::full_pipe(Packet* p) {fprintf(stderr,"Packet returned due to full pipe & buffer\n");}

static Injector_EVDEV *injector;

extern "C" {
	int plugin_type = PLUGIN_INJECTOR;
	
	Injector * get_plugin(ConfigParser *cfg) {
		injector = new Injector_EVDEV(cfg);
		return (Injector *) injector;
	}
	
	void destroy_plugin() {
		delete injector;
	}
}
