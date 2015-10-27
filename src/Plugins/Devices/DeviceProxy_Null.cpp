/*
 * This file is part of USBProxy
 */

#include "DeviceProxy_Null.h"

DeviceProxy_Null::DeviceProxy_Null() {
}

DeviceProxy_Null::~DeviceProxy_Null() {
}

static DeviceProxy_Null *proxy;

extern "C" {
	DeviceProxy * get_deviceproxy_plugin(ConfigParser *cfg) {
		proxy = new DeviceProxy_Null();
		return (DeviceProxy *) proxy;
	}

	void destroy_plugin() {
		delete proxy;
	}
}
