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

    ledInfo = device.read(0,16)
    Log.i(None, f"Led info: {Util.toHex(ledInfo)}")

    device.setTxCommand(8)
    maxColor = 16
    stepSize = 1
    index = 0
    while True:
        if (index & (0x2|0x4|0x8)) == 0:
            index |= 0x2
        for color in range(0,maxColor,stepSize):
            color = maxColor - 1 - color if index & 1 else color

            r = color if index & 0x2 else 0
            g = color if index & 0x4 else 0
            b = color if index & 0x8 else 0
            device.write(3, bytes((g,r,b))*32)
        index += 1


if __name__ == '__main__':
    wrapHandler(run_test)