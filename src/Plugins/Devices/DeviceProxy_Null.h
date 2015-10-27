/*
 * This file is part of USBProxy.
 */

#ifndef USBPROXY_DEVICEPROXYNULL_H
#define USBPROXY_DEVICEPROXYNULL_H

#include "DeviceProxy.h"

class DeviceProxy_Null: public DeviceProxy {
private:
	bool connected=false;
public:
	DeviceProxy_Null();
	~DeviceProxy_Null();

	int connect(int timeout=250) {connected=true;return 0;}
	void disconnect() {connected=false;}
	void reset() {}
	bool is_connected() {return connected;}
	bool is_highspeed() {return false;}

	//this should be done synchronously
	int control_request(const usb_ctrlrequest *setup_packet, int *nbytes, __u8* dataptr, int timeout=500) {return 0;}
	void send_data(__u8 endpoint,__u8 attributes,__u16 maxPacketSize,__u8* dataptr,int length) {}
	void receive_data(__u8 endpoint,__u8 attributes,__u16 maxPacketSize,__u8** dataptr, int* length, int timeout=500) {*length=0;}

	void set_endpoint_interface(__u8 endpoint, __u8 interface) {}
	void claim_interface(__u8 interface) {}
	void release_interface(__u8 interface) {}

	__u8 get_address() {return 0;}
	void setConfig(Configuration* fs_cfg, Configuration* hs_cfg, bool hs) {}
	char* toString() {return (char*)"Null Device";}

};

#endif /* USBPROXY_DEVICEPROXYNULL_H */
