#!/usr/bin/env python3
# encoding: utf-8
"""
This is based on pyBusPirateLite but has been stripped down to
its minimal size. It also includes a couple fixes needed to
avoid a buffer overflow in the firmware.
"""
import sys
import itertools
import time
import traceback
import collections
import collections.abc
import struct

from driver import *

def run_test(device):
    device.setPage(LedDevice.PAGE_KEYPAD)

    for i in range(5):
        kpdata = device.read(0,20)
        Log.i(None, f"Raw: {kpdata[0]:02x} Voltage: {(kpdata[0]/255) * 5:0.2f} Key: {kpdata[1]} State: {kpdata[2]} Candidate: {kpdata[3]} History: {Util.toHex(kpdata[4:])}")

if __name__ == '__main__':
    wrapHandler(run_test)













