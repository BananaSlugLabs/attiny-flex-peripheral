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
    device.setPage(LedDevice.PAGE_LED)

    while True:
        device.write(3, bytes((0x00,0x00,0x00))*24)


if __name__ == '__main__':
    wrapHandler(run_test)