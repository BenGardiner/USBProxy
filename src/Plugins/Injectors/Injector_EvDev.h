#ifndef USBPROXY_INJECTOREVDEV_H
#define USBPROXY_INJECTOREVDEV_H

#include "Injector.h"
#include <sys/socket.h>
#include <poll.h>

#define EVDEV_BUFFER_SIZE 1472

class Injector_EVDEV: public Injector {
private:
	std::string input_device;
	int input_fd;
	struct pollfd spoll;

protected:
	void start_injector();
	void stop_injector();
	int* get_pollable_fds();
	void full_pipe(Packet* p);

	void get_packets(Packet** packet,SetupPacket** setup,int timeout=500);

public:
	Injector_EVDEV(ConfigParser *cfg);
	virtual ~Injector_EVDEV();
};

#endif /* USBPROXY_INJECTOREVDEV_H */
