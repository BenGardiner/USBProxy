/*
 * This file is part of USBProxy.
 */

#ifndef USBPROXY_HOSTPROXYNULL_H
#define USBPROXY_HOSTPROXYNULL_H

#include "HostProxy.h"

class HostProxy_Null: public HostProxy {
private:
	bool connected=false;
public:
	HostProxy_Null();
	virtual ~HostProxy_Null();

	int connect(Device* device, int timeout=50) {connected=true;return 0;}
	void disconnect() {connected=false;}
	void reset() {}
	bool is_connected() {return connected;}

	int control_request(usb_ctrlrequest *setup_packet, int *nbytes, __u8** dataptr, int timeout=50) {setup_packet->bRequestType=0;return 0;}
	void send_data(__u8 endpoint,__u8 attributes,__u16 maxPacketSize,__u8* dataptr,int length) {}
	void receive_data(__u8 endpoint,__u8 attributes,__u16 maxPacketSize,__u8** dataptr, int* length, int timeout=50) {*length=0;}

	void control_ack() {}
	void stall_ep(__u8 endpoint) {}
	void setConfig(Configuration* fs_cfg,Configuration* hs_cfg,bool hs) {}
	char* toString() {return (char*)"Null Host";}
};

#endif /* USBPROXY_HOSTPROXYNULL_H */
