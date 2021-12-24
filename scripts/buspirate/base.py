#!/usr/bin/env python
# encoding: utf-8
"""
Created by Sean Nelson on 2009-10-14.
Copyright 2009 Sean Nelson <audiohacked@gmail.com>

This file is part of pyBusPirate.

pyBusPirate is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

pyBusPirate is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pyBusPirate.  If not, see <http://www.gnu.org/licenses/>.
"""

import select
import serial

"""
PICSPEED = 24MHZ / 16MIPS
"""

class PinCfg:
    POWER = 0x8
    PULLUPS = 0x4
    AUX = 0x2
    CS = 0x1

class BBIOPins:
    # Bits are assigned as such:
    MOSI = 0x01;
    CLK = 0x02;
    MISO = 0x04;
    CS = 0x08;
    AUX = 0x10;
    PULLUP = 0x20;
    POWER = 0x40;

class BBIO:
    def __init__(self, p="/dev/bus_pirate", s=115200, t=1):
        self.port = serial.Serial(p, s, timeout=t)
    
    def BBmode(self):
        self.port.flushInput();
        for i in range(20):
            self.port.write(b"\x00");
            r,w,e = select.select([self.port], [], [], 0.05);
            if (r): break;
        rsp = self.response(5)
        #print(f"BBmode {repr(rsp)}")
        if rsp == b"BBIO1": return 1
        else: return 0

    def reset(self):
        self.port.write(b"\x00")
        self.timeout(0.1)
        self.port.flushInput()

    def enter_I2C(self):
        self.port.write(b"\x02")
        self.timeout(0.2)
        if self.response(4) == b"I2C1": return 1
        else: return 0

    def resetBP(self):
        self.reset()
        self.port.write(b"\x0F")
        self.timeout(0.1)
        #self.port.read(2000)
        self.port.flushInput()
        return 1

    def raw_cfg_pins(self, config):
        self.port.write(bytes((0x40 | config, )))
        self.timeout(0.1)
        return self.response(1)

    def raw_set_pins(self, pins):
        self.port.write(bytes((0x80 | config, )))
        self.timeout(0.1)
        return self.response(1)

    def cfg_pins(self, pins=0):
        self.port.write(bytes((0x40 | pins, )))
        self.timeout(0.1)
        return self.response()
        
    def raw_cfg_pins(self, config):
        self.port.write(bytes((0x40 | config, )))
        self.timeout(0.1)
        return self.response(1)

    def timeout(self, timeout=0.1):
        select.select([], [], [], timeout)

    def response(self, byte_count=1, return_data=False):
        data = self.port.read(byte_count)
        if byte_count == 1 and return_data == False:
            if data == bytes((0x01, )): return 1
            else: return 0
        else:
            return data

    def bulk_trans(self, byte_count=1, byte_string=None):
        if byte_string == None: pass
        if (byte_count > 16):
            raise ValueError
        self.port.write(bytes((0x10 | (byte_count-1),)))
        #self.timeout(0.1)
        for i in range(byte_count):
            self.port.write(bytes((byte_string[i],)))
            #self.timeout(0.1)
        data = self.response(byte_count+1, True)
        return data[0] == 1, data[1:]

    def set_speed(self, spi_speed=0):
        self.port.write(bytes((0x60 | spi_speed, )))
        self.timeout(0.1)
        return self.response()
