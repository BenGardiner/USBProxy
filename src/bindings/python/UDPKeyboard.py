# UDPKeyboard.py
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
import os

class UDPKeyboardInterface(USBInterface):
    name = "USB keyboard interface"

    hid_descriptor = b'\x09\x21\x10\x01\x00\x01\x22\x2b\x00'
    report_descriptor = b'\x05\x01\x09\x06\xA1\x01\x05\x07\x19\xE0\x29\xE7\x15\x00\x25\x01\x75\x01\x95\x08\x81\x02\x95\x01\x75\x08\x81\x01\x19\x00\x29\x65\x15\x00\x25\x65\x75\x08\x95\x06\x81\x00\xC0'

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

        self.devices = map(InputDevice, ('/dev/input/event1', '/dev/input/event0'))
        self.devices = {dev.fd: dev for dev in self.devices}
        for dev in self.devices.values(): print(dev)
        self.current_keys = [0]

        #clear all the LEDs
        for ledpath in os.listdir('/sys/class/leds/'):
            contents = open('/sys/class/leds/' + ledpath + '/trigger', 'w')
            contents.write('none\n');
            contents.close()
            contents = open('/sys/class/leds/' + ledpath + '/brightness', 'w')
            contents.write('0\n')
            contents.close()

        #arduino led tubes
        #TODO: run stty to set 115200,sane
        self.led_tubes_pipe = open('/dev/ttyACM0', 'wb')
        self.NUM_TUBE_LEDS = 13.0 #set these as floats to keep increased granularity from evdev timestamps
        self.MAX_TUBE_SECONDS = 5.0

        #TODO
        self.button1_rate_led = open('/sys/class/leds/switch:green:led_A/brightness', 'w')
        #self.button1_rate_led = open('/sys/class/leds/beaglebone:green:usr3/brightness', 'w')
        self.button1_status_led = open('/sys/class/leds/beaglebone:green:usr2/brightness', 'w')
        self.button2_status_led = open('/sys/class/leds/beaglebone:green:mmc0/brightness', 'w')
        #TODO
        self.button2_rate_led = open('/sys/class/leds/switch:green:led_B/brightness', 'w')
        #self.button2_rate_led = open('/sys/class/leds/beaglebone:green:heartbeat/brightness', 'w')

        self.brakes_pressed = 0
        self.gas_pressed = 0
        self.brakes_last_timestamp = -1
        self.gas_last_timestamp = -1
        self.reset_limiter()

        self.last_send_was_nil = 0

    def reset_limiter(self):
        self.hackBrakes = 5 #5 seconds of brakes
        self.hackGas = 5 #5 seconds of gas
        self.hackTimer = 0
        self.last_timer_timestamp = -1

    def add_limiter(self):
        self.hackBrakes += 5
        self.hackGas += 5 #5 seconds of gas
        self.hackTimer = 0

    global FLAG_AUTO_DEMO_MODE
    global FLAG_TUBE1
    FLAG_AUTO_DEMO_MODE = ord(b'\x20')
    FLAG_TUBE1 = ord(b'\x80')

    def update_rate_limiter_leds(self):
        if self.hackBrakes > 0:
            self.button1_rate_led.write('255\n')
        else:
            self.button1_rate_led.write('0\n')
        self.button1_rate_led.flush()

        tubeval = max(0,min(5.0,self.hackBrakes))
        tubeval = int(tubeval*self.NUM_TUBE_LEDS/self.MAX_TUBE_SECONDS)
        tubeval |= FLAG_AUTO_DEMO_MODE | FLAG_TUBE1
        print("DEC:%d" % tubeval )
        print("HEX1:")
        print(bytes([tubeval]) )
        print("CHAR:[%s]\n" % chr(tubeval))
        #self.led_tubes_pipe.write(chr(tubeval))
        self.led_tubes_pipe.write(bytes([tubeval]))
        #the tubes are flushed by this command below: self.led_tubes_pipe.flush()

        if self.hackGas > 0:
            self.button2_rate_led.write('255\n')
        else:
            self.button2_rate_led.write('0\n')
        self.button2_rate_led.flush()
        print(bytes([tubeval]) )

        tubeval = max(0,min(5.0,self.hackGas))
        tubeval = int(tubeval*self.NUM_TUBE_LEDS/self.MAX_TUBE_SECONDS)
        print(bytes([tubeval]) )
        tubeval |= FLAG_AUTO_DEMO_MODE
        print("DEC:%d" % tubeval )
        print("HEX2:")
        print(bytes([tubeval]) )
        print("CHAR:[%s]\n" % chr(tubeval))
        #self.led_tubes_pipe.write(chr(tubeval))
        self.led_tubes_pipe.write(bytes([tubeval]))
        print(bytes([tubeval]) )
        self.led_tubes_pipe.flush()

    def handle_buffer_available(self):
        r, w, x = select(self.devices, [], [])
        for fd in r:
            for event in self.devices[fd].read():
                if event.type != ecodes.EV_KEY:
                    continue

                if event.code == 3 and event.value != 1:
                    print("reset of the rate limiter")
                    self.reset_limiter()
                    self.update_rate_limiter_leds()

                    if self.brakes_pressed == 1 or self.gas_pressed == 1:
                        self.rate_limit()
                        return #always return after writing a packet
                    continue

                if event.code == 17 and event.value == 1: #17 co-opted for timer events in our select() loop
                    if self.last_timer_timestamp > 0:
                        self.hackTimer += event.timestamp() - self.last_timer_timestamp #increase hackTimer by timer period
                    self.last_timer_timestamp = event.timestamp()

                    if self.hackTimer >= 120 : #reset timer and allow button presses after 120
                        self.add_limiter()

                    self.update_hackBrakes(event)
                    self.update_hackGas(event)

                    self.update_rate_limiter_leds()

                    if (
                            (self.hackBrakes <= 0 and self.brakes_pressed == 1)
                            or (self.hackGas <= 0 and self.gas_pressed == 1)
                            or (self.hackTimer == 0 and (self.brakes_pressed == 1 or self.gas_pressed == 1))
                        ):
                        #handle edge-transitions
                        self.rate_limit()
                        return #always return after writing a packet
                    continue

                if event.code != 1 and event.code != 2:
                    continue

                if event.code == 1:
                    self.update_hackBrakes(event)
                    self.brakes_pressed = event.value

                    if self.brakes_pressed == 1:
                        self.button1_status_led.write('255\n')
                    else:
                        self.button1_status_led.write('0\n')
                    self.button1_status_led.flush()

                if event.code == 2:
                    self.update_hackGas(event)
                    self.gas_pressed = event.value

                    if self.gas_pressed == 1:
                        self.button2_status_led.write('255\n')
                    else:
                        self.button2_status_led.write('0\n')
                    self.button2_status_led.flush()

                self.rate_limit()
                return #always return after writing a packet

    def update_hackGas(self, event):
        if self.gas_pressed == 1: #button going-to-be-released or might be rate limited
            if self.gas_last_timestamp < 0:
                print("error: gas_last_timestamp not set")
            self.hackGas -= event.timestamp() - self.gas_last_timestamp
            self.hackGas = max(0, self.hackGas)
        self.gas_last_timestamp = event.timestamp()

    def update_hackBrakes(self, event):
        if self.brakes_pressed == 1: #button going-to-be-released or might be rate limited
            if self.brakes_last_timestamp < 0:
                print("error: brakes_last_timestamp not set")
            self.hackBrakes -= event.timestamp() - self.brakes_last_timestamp
            self.hackBrakes = max(0, self.hackBrakes)
        self.brakes_last_timestamp = event.timestamp()

    def rate_limit(self):
        send_keys = []
        if self.brakes_pressed == 1 and self.hackBrakes > 0: #if brakes pressed and allowed to be so
            send_keys.append(0x4)

        if self.gas_pressed == 1 and self.hackGas > 0: #if gas pressed and allowed to be so
            send_keys.append(0x14)

        if not send_keys:
            if self.last_send_was_nil == 1:
                return
            self.last_send_was_nil = 1
        else:
            self.last_send_was_nil = 0

        self.endpoint.send(bytes([0,0] + send_keys + [0]*(6-len(send_keys)) ))

    def type_letter(self, keycode, modifiers=0):
        data = bytes([ modifiers, 0, keycode ])

        if self.verbose > 2:
            print(self.name, "sending keypress 0x%02x" % keycode)

        self.endpoint.send(data)


class UDPKeyboardDevice(USBDevice):
    name = "USB keyboard device"

    def __init__(self, maxusb_app, verbose=0, text=None):
        config = USBConfiguration(
                1,                                          # index
                "Emulated Keyboard",    # string desc
                [ UDPKeyboardInterface(text=text) ]         # interfaces
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

