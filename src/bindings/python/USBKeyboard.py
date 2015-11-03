# USBKeyboard.py
#
# Contains class definitions to implement a USB keyboard.

from USB import *
from USBDevice import *
from USBConfiguration import *
from USBInterface import *
from USBEndpoint import *
from keymap import get_keycode
from evdev import InputDevice, categorize, ecodes
from select import select

class USBKeyboardInterface(USBInterface):
    name = "USB keyboard interface"

    hid_descriptor = b'\x09\x21\x10\x01\x00\x01\x22\x2b\x00'
    report_descriptor = b'\x05\x01\x09\x06\xA1\x01\x05\x07\x19\xE0\x29\xE7\x15\x00\x25\x01\x75\x01\x95\x08\x81\x02\x95\x01\x75\x08\x81\x01\x19\x00\x29\x65\x15\x00\x25\x65\x75\x08\x95\x01\x81\x00\xC0'

    def __init__(self, verbose=0, text=None):
        descriptors = { 
                USB.desc_type_hid    : self.hid_descriptor,
                USB.desc_type_report : self.report_descriptor
        }

        self.endpoint = USBEndpoint(
                3,          # endpoint number
                USBEndpoint.direction_in,
                USBEndpoint.transfer_type_interrupt,
                USBEndpoint.sync_type_none,
                USBEndpoint.usage_type_data,
                16384,      # max packet size
                10,         # polling interval, see USB 2.0 spec Table 9-13
                self.handle_buffer_available    # handler function
        )

        # TODO: un-hardcode string index (last arg before "verbose")
        USBInterface.__init__(
                self,
                0,          # interface number
                0,          # alternate setting
                3,          # interface class
                0,          # subclass
                0,          # protocol
                0,          # string index
                verbose,
                [ self.endpoint ],
                descriptors
        )
        
        self.devices = map(InputDevice, ('/dev/input/event0', '/dev/input/event1'))
        self.devices = {dev.fd: dev for dev in self.devices}
        
        self.letter_a = bytes([0, 0, 4])
        self.letter_q = bytes([0, 0, 14])
        
        self.current_keys = bytes([0])
        
    def handle_buffer_available(self):
        r,x,y = select(devices, [], [])
        for fd in r:
            for event in devices[fd].read():
            	if event.type != ecode.EV_KEY:
            		return
                if event.code != 1 and event.code != 2:
                	return

		if event.value == 1: #if pressed
                	if event.code == 1: #if button 1 is pressed
                		self.current_keys.append(4)
                		print("anti-brake key pressed")
                	elif event.code == 2: #if button 2 is pressed
                		self.current_keys.append(14)
                		print("full throttle key pressed")
                	
        	elif event.value == 0: # if key released
        		if event.code == 1:
        			self.current_keys.remove(4)
        			print("anti-brake key released")
        		elif event.code == 2:
        			self.current_keys.remove(14)
        			print("full throttle key released")
        	
        	if not self.current_keys:
        		self.current_keys = bytes([0])
        	
        	self.endpoint.send(bytes([0,0] + self.current_keys))

    def type_letter(self, keycode, modifiers=0):
        data = bytes([ modifiers, 0, keycode ])

        if self.verbose > 2:
            print(self.name, "sending keypress 0x%02x" % keycode)

        self.endpoint.send(data)


class USBKeyboardDevice(USBDevice):
    name = "USB keyboard device"

    def __init__(self, maxusb_app, verbose=0, text=None):
        config = USBConfiguration(
                1,                                          # index
                "Emulated Keyboard",    # string desc
                [ USBKeyboardInterface(text=text) ]         # interfaces
        )

        USBDevice.__init__(
                self,
                maxusb_app,
                0,                      # device class
                0,                      # device subclass
                0,                      # protocol release number
                64,                     # max packet size for endpoint 0
                0x610b,                 # vendor id
                0x4653,                 # product id
                0x3412,                 # device revision
                "Move along",                # manufacturer string
                "This is not the USB device you're looking for",   # product string
                "00001",             # serial number string
                [ config ],
                verbose=verbose
        )

