/*
 * This file is part of USBProxy
 */

#include "HostProxy_Null.h"

HostProxy_Null::HostProxy_Null() {
}

HostProxy_Null::~HostProxy_Null() {
}

static HostProxy_Null *proxy;

extern "C" {
	HostProxy * get_hostproxy_plugin(ConfigParser *cfg) {
		proxy = new HostProxy_Null();
		return (HostProxy *) proxy;
	}

	void destroy_plugin() {
		delete proxy;
	}
}
