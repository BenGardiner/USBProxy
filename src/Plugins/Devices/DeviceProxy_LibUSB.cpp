/*
 * Copyright 2013 Dominic Spill
 * Copyright 2013 Adam Stasiak
 *
 * This file is part of USBProxy.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include "DeviceProxy_LibUSB.h"
#include "TRACE.h"
#include "HexString.h"

#include "myDebug.h"

int DeviceProxy_LibUSB::debugLevel=0;

//CLEANUP hotplug support

DeviceProxy_LibUSB::DeviceProxy_LibUSB(int vendorId,int productId,bool includeHubs)
{
	dbgMessage("");
	context=NULL;
	dev_handle=NULL;
	privateContext=true;
	privateDevice=true;
	desired_vid=vendorId;
	desired_pid=productId;
	desired_hubs=includeHubs;
}

DeviceProxy_LibUSB::DeviceProxy_LibUSB(ConfigParser *cfg)
{
	int vendorId, productId;
	
	dbgMessage("");
	std::string vid_str = cfg->get("vendorId");
	if(vid_str == "")
		vendorId = LIBUSB_HOTPLUG_MATCH_ANY;
	else
		vendorId = std::stoi(vid_str, nullptr, 16);
	fprintf(stderr, "vendorId=%04x\n", vendorId);
	
	std::string pid_str = cfg->get("productId");
	if(pid_str == "")
		productId = LIBUSB_HOTPLUG_MATCH_ANY;
	else
		productId = std::stoi(pid_str, nullptr, 16);
	fprintf(stderr, "productId=%04x\n", productId);
	
	bool includeHubs = false;
	
	context=NULL;
	dev_handle=NULL;
	privateContext=true;
	privateDevice=true;
	desired_vid=vendorId;
	desired_pid=productId;
	desired_hubs=includeHubs;
}

DeviceProxy_LibUSB::~DeviceProxy_LibUSB() {
	dbgMessage("");
	if (privateDevice && dev_handle) {libusb_close(dev_handle);}
	if (privateContext && context) {libusb_exit(context);}
}

int DeviceProxy_LibUSB::connect(int timeout) {
	dbgMessage("");
	return connect(desired_vid,desired_pid,desired_hubs);
}

int DeviceProxy_LibUSB::connect(libusb_device* dvc, libusb_context* _context) {
	dbgMessage("");
	if (dev_handle) {fprintf(stderr,"LibUSB already connected.\n"); return 0;}
	privateContext=false;
	context=_context;
	int rc=libusb_open(dvc,&dev_handle);
	if (rc) {
		if (debugLevel) {fprintf(stderr,"Error %d opening device handle.\n",rc);}
		dev_handle=NULL;
		return rc;
	}
	if (debugLevel) {fprintf(stdout,"Connected to device: %s\n",toString());}
	return 0;
}

int DeviceProxy_LibUSB::connect(libusb_device_handle* devh,libusb_context* _context) {
	dbgMessage("");
	if (dev_handle) {fprintf(stderr,"LibUSB already connected.\n"); return 0;}
	privateContext=false;
	privateDevice=false;
	context=_context;
	dev_handle=devh;
	if (debugLevel) {fprintf(stdout,"Connected to device: %s\n",toString());}
	return 0;
}

int DeviceProxy_LibUSB::connect(int vendorId,int productId,bool includeHubs) {
	dbgMessage("");
	if (dev_handle) {fprintf(stderr,"LibUSB already connected.\n"); return 0;}
	privateContext=true;
	privateDevice=true;
	libusb_init(&context);
	libusb_device **list=NULL;
	libusb_device *found=NULL;

	ssize_t cnt=libusb_get_device_list(context,&list);
	if (cnt<0) {
		if (debugLevel) {fprintf(stderr,"Error %d retrieving device list.\n", (int)cnt);}
		return cnt;
	}

	ssize_t i=0;

	struct libusb_device_descriptor desc;
	int rc=0;

	for(i = 0; i < cnt; i++){
		libusb_device *dvc = list[i];
		rc = libusb_get_device_descriptor(dvc,&desc);
		if (rc) {
			if (debugLevel) {fprintf(stderr,"Error %d retrieving device descriptor.\n",rc);}
		} else {
			if (
					(includeHubs || desc.bDeviceClass!=LIBUSB_CLASS_HUB) &&
					(vendorId==desc.idVendor || vendorId==LIBUSB_HOTPLUG_MATCH_ANY) &&
					(productId==desc.idProduct || productId==LIBUSB_HOTPLUG_MATCH_ANY)
				) {
				found=dvc;
				break;
			}
		}
	}

	if (found==NULL) {
		if (debugLevel) {fprintf(stderr,"No devices found.\n");}
		libusb_free_device_list(list,1);
		return -1;
	} else {
		rc=libusb_open(found,&dev_handle);
		if (rc) {
			if (debugLevel) {fprintf(stderr,"Error %d opening device handle.\n",rc);}
			dev_handle=NULL;
			libusb_free_device_list(list,1);
			return rc;
		}

	}

	libusb_free_device_list(list,1);
	libusb_set_auto_detach_kernel_driver(dev_handle,1);

	//check that device is responsive
	rc=libusb_get_string_descriptor(dev_handle,0,0,(unsigned char*)&rc,4);
	if (rc<0) {
		fprintf(stderr,"Device unresponsive.\n");
		return rc;
	}

	if (debugLevel) {
		char *device_desc=toString();
		fprintf(stdout,"Connected to device: %s\n",device_desc);
		free(device_desc);
	}
	return 0;
}

void DeviceProxy_LibUSB::disconnect() {
	dbgMessage("");
	if (privateDevice && dev_handle) {libusb_close(dev_handle);}
	dev_handle=NULL;
	if (privateContext && context) {libusb_exit(context);}
	context=NULL;
}

void DeviceProxy_LibUSB::reset() {
	dbgMessage("");
	int rc=libusb_reset_device(dev_handle);
	if (rc==LIBUSB_ERROR_NOT_FOUND) {disconnect();}
	if (rc) {fprintf(stderr,"Error %d resetting device.\n",rc);}
}

bool DeviceProxy_LibUSB::is_connected() {
	dbgMessage("");
	if (dev_handle) {return true;} else {return false;}
}

bool DeviceProxy_LibUSB::is_highspeed() {
	dbgMessage("");
	libusb_device* dvc=libusb_get_device(dev_handle);
	int speed=libusb_get_device_speed(dvc);
	return (speed==LIBUSB_SPEED_HIGH) || (speed==LIBUSB_SPEED_SUPER);
}

char* DeviceProxy_LibUSB::toString() {
	dbgMessage("");
	unsigned char* str_mfr=NULL;
	unsigned char* str_prd=NULL;
	struct libusb_device_descriptor desc;
	libusb_device* dvc=libusb_get_device(dev_handle);
	int rc=libusb_get_device_descriptor (dvc,&desc);
	if (rc) {
		if (debugLevel) {fprintf(stderr,"Error %d retrieving device descriptor.\n",rc);}
		return NULL;
	}
	uint8_t address=libusb_get_device_address(dvc);
	if (desc.iManufacturer) {
		str_mfr=(unsigned char  *)malloc(126);
		rc=libusb_get_string_descriptor_ascii(dev_handle,desc.iManufacturer,str_mfr,126);
		if (rc<0) {
			if (debugLevel) {fprintf(stderr,"Error %d retrieving string descriptor.\n",rc);}
			return NULL;
		}
	}
	if (desc.iProduct) {
		str_prd=(unsigned char  *)malloc(126);
		rc=libusb_get_string_descriptor_ascii(dev_handle,desc.iProduct,str_prd,126);
		if (rc<0) {
			if (debugLevel) {fprintf(stderr,"Error %d retrieving string descriptor.\n",rc);}
			return NULL;
		}
	}
	size_t length=snprintf(NULL,0,"%04x:%04x@%02x %s - %s",desc.idVendor,desc.idProduct,address,(unsigned char*)(str_mfr?str_mfr:(unsigned char*)"N/A"),(unsigned char*)(str_prd?str_prd:(unsigned char*)"N/A"));
	char *buf=(char  *)malloc(length+1);
	sprintf(buf,"%04x:%04x@%02x %s - %s",desc.idVendor,desc.idProduct,address,(unsigned char*)(str_mfr?str_mfr:(unsigned char*)"N/A"),(unsigned char*)(str_prd?str_prd:(unsigned char*)"N/A"));
	if (str_mfr) {free(str_mfr);/*not needed str_mfr=NULL;*/}
	if (str_prd) {free(str_prd);/*not needed str_prd=NULL;*/}
	return buf;
}

int DeviceProxy_LibUSB::control_request(const usb_ctrlrequest *setup_packet, int *nbytes, __u8* dataptr,int timeout) {
	dbgMessage("");
	if (debugLevel>1) {
		char* hex=hex_string((void*)setup_packet,sizeof(*setup_packet));
		printf("LibUSB> %s\n",hex);
		free(hex);
	}
	dbgMessage("");
	int rc=libusb_control_transfer(dev_handle,setup_packet->bRequestType,setup_packet->bRequest,setup_packet->wValue,setup_packet->wIndex,dataptr,setup_packet->wLength,timeout);
	dbgMessage(""); fprintf( stderr, "%d=libusb_control_transfer(%x, %02x, %02x, %04x, %04x, %04x, %d)\n", rc,dev_handle,setup_packet->bRequestType,setup_packet->bRequest,setup_packet->wValue,setup_packet->wIndex,dataptr,setup_packet->wLength,timeout); myDump( dataptr, rc);
	if (rc<0) {
		if (debugLevel) {fprintf(stderr,"Error %d[%s] sending setup packet.\n",rc,libusb_error_name(rc));}
		if (rc==-9) return -1;
		return rc;
	}
	if (debugLevel>1) {
		char* hex=hex_string((void*)dataptr,rc);
		printf("LibUSB< %s\n",hex);
		free(hex);
	}
	*nbytes=rc;
	return 0;
}

__u8 DeviceProxy_LibUSB::get_address() {
	libusb_device* dvc=libusb_get_device(dev_handle);
	return libusb_get_device_address(dvc);
}

void DeviceProxy_LibUSB::send_data(__u8 endpoint,__u8 attributes,__u16 maxPacketSize,__u8* dataptr,int length) {
	dbgMessage("");
	int transferred;
	int rc;
	switch (attributes & USB_ENDPOINT_XFERTYPE_MASK) {
		case USB_ENDPOINT_XFER_CONTROL:
			fprintf(stderr,"Can't send on a control endpoint.");
			return;
			break;
		case USB_ENDPOINT_XFER_ISOC:
			//TODO handle isochronous
			fprintf(stderr,"Isochronous endpoints unhandled.");
			return;
			break;
		case USB_ENDPOINT_XFER_BULK:
			rc=libusb_bulk_transfer(dev_handle,endpoint,dataptr,length,&transferred,0);
			dbgMessage(""); fprintf( stderr, "%d=libusb_bulk_transfer(%x,%02x,%x,%x,%x,0);\n",rc,dev_handle,endpoint,dataptr,length,&transferred); myDump( dataptr, length);
			if (rc) {fprintf(stderr,"Transfer error (%d) on Device EP%d\n",rc,endpoint);}
			//TODO retry transfer if incomplete
			if (transferred!=length) {fprintf(stderr,"Incomplete Bulk transfer on EP%02x\n",endpoint);}
			break;
		case USB_ENDPOINT_XFER_INT:
			rc=libusb_interrupt_transfer(dev_handle,endpoint,dataptr,length,&transferred,0);
			dbgMessage(""); fprintf( stderr, "%d=libusb_interrupt_transfer(%x,%02x,%x,%x,%x,0);\n",rc,dev_handle,endpoint,dataptr,length,&transferred); myDump( dataptr, length);
			if (rc) {fprintf(stderr,"Transfer error (%d) on Device EP%d\n",rc,endpoint);}
			//TODO retry transfer if incomplete
			if (transferred!=length) {fprintf(stderr,"Incomplete Interrupt transfer on EP%02x\n",endpoint);}
			break;
	}
}

void DeviceProxy_LibUSB::receive_data(__u8 endpoint,__u8 attributes,__u16 maxPacketSize,__u8** dataptr, int* length,int timeout) {
	dbgMessage("");
	int rc;
	if (timeout<10) timeout=10;
	switch (attributes & USB_ENDPOINT_XFERTYPE_MASK) {
		case USB_ENDPOINT_XFER_CONTROL:
			fprintf(stderr,"Can't send on a control endpoint.");
			return;
			break;
		case USB_ENDPOINT_XFER_ISOC:
			//TODO handle isochronous
			fprintf(stderr,"Isochronous endpoints unhandled.");
			return;
			break;
		case USB_ENDPOINT_XFER_BULK:
			timeout=100;
			*dataptr=(__u8*)malloc(maxPacketSize*8);
			rc=libusb_bulk_transfer(dev_handle,endpoint,*dataptr,maxPacketSize,length,timeout);
			dbgMessage(""); fprintf( stderr, "%d=libusb_bulk_transfer(%x,%02x,%x,%x,%x,%d);\n",rc,dev_handle,endpoint,*dataptr,maxPacketSize,length,timeout); myDump( *dataptr, *length);
			if (rc==LIBUSB_ERROR_TIMEOUT){free(*dataptr);*dataptr=NULL;*length=0;return;}
			if (rc) {free(*dataptr);*dataptr=NULL;*length=0;fprintf(stderr,"Transfer error (%d) on Device EP%02x\n",rc,endpoint);}
			dbgMessage(""); fprintf( stderr, "%d = libusb_bulk_transfer( %x, %x, %x, %d, %x, %d);\n", rc, dev_handle, endpoint, *dataptr, maxPacketSize, length, timeout); myDump( *dataptr, *length);
			break;
		case USB_ENDPOINT_XFER_INT:
			*dataptr=(__u8*)malloc(maxPacketSize);
			rc=libusb_interrupt_transfer(dev_handle,endpoint,*dataptr,maxPacketSize,length,timeout);
			dbgMessage(""); fprintf( stderr, "%d=libusb_interrupt_transfer(%x,%02x,%x,%d,%x,%d);\n",rc,dev_handle,endpoint,*dataptr,maxPacketSize,length,timeout); myDump( *dataptr, *length);
			if (rc==LIBUSB_ERROR_TIMEOUT){free(*dataptr);*dataptr=NULL;*length=0;return;}
			if (rc) {free(*dataptr);*dataptr=NULL;*length=0;fprintf(stderr,"Transfer error (%d) on Device EP%02x\n",rc,endpoint);}
			break;
	}
}

void DeviceProxy_LibUSB::claim_interface(__u8 interface) {
	dbgMessage("");
	if (is_connected()) {
		int rc=libusb_claim_interface(dev_handle,interface);
		// modified 20140905 atsumi@aizulab.ocm
		// if (rc) {fprintf(stderr,"Error (%d) claiming interface %d\n",rc,interface);}
		if (rc) {fprintf(stderr,"Error (%d:%s) claiming interface %d\n",rc,libusb_error_name(rc),interface);}
	}
}

void DeviceProxy_LibUSB::release_interface(__u8 interface) {
	dbgMessage("");
	if (is_connected()) {
		int rc=libusb_release_interface(dev_handle,interface);
		if (rc && rc!=-5) {fprintf(stderr,"Error (%d) releasing interface %d\n",rc,interface);}
	}
}

static DeviceProxy_LibUSB *proxy;

extern "C" {
	DeviceProxy * get_deviceproxy_plugin(ConfigParser *cfg) {
		dbgMessage("");
		proxy = new DeviceProxy_LibUSB(cfg);
		return (DeviceProxy *) proxy;
	}
	
	void destroy_plugin() {
		dbgMessage("");
		delete proxy;
	}
}
