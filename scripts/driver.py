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
import logging

FLEX_PERIPHERAL_ADDRESS=0x52

from buspirate import *
__all__ = ('Util', 'BusError', 'Log', 'LedDevice', 'IoInterface', 'wrapHandler')

#todo: Use a proper logger!
"""
L = logging.getLogger("driver")

def setupLogging ():
    logging.basicConfig(
        level=logging.DEBUG,
        format='%(asctime)s %(name)-20s %(levelname)-8s %(message)s',
        datefmt='%m-%d %H:%M',
    )

"""
class Util:
    @staticmethod
    def toHex (b):
        return "Hex{" + " ".join(map(lambda x: f"{x:02X}", b)) + "}"

class BusError(Exception):
    def __init__ (self, msg, *args, context = None, **kwargs):
        super().__init__(msg,*args, **kwargs)
        self.msg = msg
        self.context = context
        Log.d(context, msg)
        
    def __str__ (self):
        return f"{self.__class__.__name__}: {self.msg}"


class Log:
    SHOW_RAW = False
    SHOW_DEBUG = False
    @staticmethod
    def _log(level, taggedObj, msg):
        prefix = None
        if taggedObj is not None and isinstance(taggedObj, object):
            if hasattr(taggedObj, '__tag__'):
                prefix = taggedObj.__tag__()
            elif hasattr(taggedObj.__class__, '__class_tag__'):
                prefix = taggedObj.__class__.__class_tag__
            else:
                prefix = taggedObj.__class__.__name___

        prefix = f"[{level}]" if prefix is None else f"[{level} {prefix}]"

        lines = msg.split("\n")
        for idx, i in enumerate(lines):
            if idx == len(lines)-1 and i == "":
                break
            sys.stderr.write(f"{prefix} {i}\n")


    @staticmethod
    def d(t,s):
        if Log.SHOW_DEBUG:
            Log._log("DEBUG", t, s)

    @staticmethod
    def raw(t,data,isWrite):
        if Log.SHOW_RAW:
            if isWrite:
                Log._log("RAW", t, f">>>> WR >>>> {Util.toHex(data)}")
            else:
                Log._log("RAW", t, f"<<<< RD <<<< {Util.toHex(data)}")

    @staticmethod
    def i(t,s):
        Log._log("INFO", t, s)

    @staticmethod
    def e(t,s):
        Log._log("ERROR", t, s)


class IoInterface():
    __class_tag__ = "BusPirate"

    def __init__ (self, path = "/dev/ttyUSB0", baud = 115200):
        super().__init__()

        self._path = path
        self._baud = baud
        Log.d(self, "open '" + path + "' with baud " + str(baud))

        self.raw = I2C(path, baud);

        Log.d(self, "config timeout")
        self.raw.timeout(0.1)
        
        for i in range(5):

            Log.d(self, "reset")
            if not self.raw.resetBP():
                raise BusError("Unable to reset BusPirate.")

            time.sleep(0.25)

            Log.d(self, "enter binary mode")
            if not self.raw.BBmode():
                if i == 4:
                    raise BusError("BusPirate unable to enter binary mode.")
                Log.d(self, "binary mode failed, try again")
                self.raw.reset()
            else:
                break

        time.sleep(0.1)

        for i in range(5):
            Log.d(self, "i2c mode")
            if not self.raw.enter_I2C():
                if i == 4:
                    raise BusError("BusPirate unable to enter i2c mode.")
                else:
                    Log.d(self, "i2c mode failed, try again")
            else:
                break


        Log.d(self, "config pins & power")
        if not self.raw.cfg_pins(I2CPins.POWER | I2CPins.PULLUPS):
            raise BusError("BusPirate unable to configure power/pullups.")

        Log.d(self, "config i2c frequency")
        if not self.raw.set_speed(I2CSpeed._400KHZ):
            raise BusError("BusPirate unable to configure i2c frequency.")


    def close (self):
        Log.d(self, "Cleaning up bus pirate.")
        self.raw.BBmode()
        self.raw.resetBP()
        #if not self.raw.reset():
        #    raise BusError("Unable to reset BusPirate.")

class LedDevice:
    CHUNK_SIZE                  = 8
    STATUS_REGISTER             = 0xF0
    TX_CMD_REGISTER             = 0xF1
    CMD_REGISTER                = 0xF2
    CMD_PARAM0_REGISTER         = 0xF3
    CMD_PARAM1_REGISTER         = 0xF4
    CMD_PARAM2_REGISTER         = 0xF5
    CMD_PARAM3_REGISTER         = 0xF6

    STATUS_PENDING              = 1<<4
    STATUS_SUCCESS              = 0

    PAGE_DEVICE_INFO            = 1
    PAGE_LED                    = 2
    PAGE_KEYPAD                 = 3

    def __init__ (self, intf, addr = FLEX_PERIPHERAL_ADDRESS):
        self._intf = intf
        self._addr = addr

    def __tag__ (self):
        return f"LedDevice({self._addr})"

    def _encodeWriteBuffer (self, index, data = None):
        buf = bytes((self._addr, index & 0xFF))
        if data is not None:
            buf = buf + data
        Log.raw(self, buf, True)
        return buf

    def write(self, index, data):
        Log.d(self, f"Writing 0x{index:02X} .. 0x{index + len(data) - 1:02X} (length {len(data)}).")

        buf = self._encodeWriteBuffer(index, data)

        self._intf.raw.send_start_bit()
        for chunk in [buf[i:i + LedDevice.CHUNK_SIZE] for i in range(0, len(buf), LedDevice.CHUNK_SIZE)]:
            rc, data = self._intf.raw.bulk_trans(len(chunk),chunk)
            if rc != 1 or any(map(lambda x: x == 1, data)):
                raise BusError(f"Failed to write buffer. (Size: {len(buf)}; Chunk Size: {len(chunk)})")
        self._intf.raw.send_stop_bit()

    def read(self, index, size):
        if size <= 0:
            raise BusError(f"Must read 1 or more bytes.")

        # Setup Transfer EP & Index
        Log.d(self, f"Setup Address: 0x{index:02X}.")
        buf = self._encodeWriteBuffer(index)
        self._intf.raw.send_start_bit()
        rc, data = self._intf.raw.bulk_trans(len(buf),buf)
        self._intf.raw.send_stop_bit()
        if rc != 1 or any(map(lambda x: x == 1, data)):
            raise BusError("Failed to write Index.")

        Log.d(self, f"Reading 0x{index:02X} .. 0x{index + size - 1:02X} (length {size}).")

        # Start Read Transaction
        self._intf.raw.send_start_bit()
        buf = bytes((self._addr|1,))
        self._intf.raw.bulk_trans(len(buf), buf)

        # Read Memory
        buf = bytearray()
        currentOffset = 0
        while True:
            buf.append(self._intf.raw.read_byte()[0])
            if currentOffset < size - 1:
                self._intf.raw.send_ack()
            else:
                break
            size-=1
        self._intf.raw.send_nack()
        self._intf.raw.send_stop_bit()
        Log.raw(self, buf, False)

        return buf

    def setTxCommand(self, cmd):
        buf = bytes((cmd,))
        self.write(LedDevice.TX_CMD_REGISTER, buf)

    def setPage (self, page):
        self.runCommand(1, bytes((page,)))

    def runCommand (self, cmd, data = None, resultSize = 0):
        buf = bytes((cmd,))
        if data is not None:
            if len(data) > 4:
                raise BusError("Command can only have 4 parameters.")
            buf += data
        self.write(LedDevice.CMD_REGISTER, buf)

        for i in range(10):
            status = self.getStatus() & 0xF0
            if status == LedDevice.STATUS_PENDING:
                Log.d(self, f"Command {cmd:02X} pending...")
                time.sleep(0.1)
            elif status != 0:
                raise BusError(f"Command {cmd:02X} finished with status {status:02X}.")
            else:
                return
        raise BusError(f"Command {cmd} timed out.")

    def getStatus(self):
        return self.read(LedDevice.STATUS_REGISTER, 1)[0]


def wrapHandler (handler, arguments = None):
    intf = None
    try:
        intf = IoInterface()
        device = LedDevice(intf, 0x52)
        device.setPage(LedDevice.PAGE_DEVICE_INFO)
        devInfo = device.read(0,8)
        
        Log.i(None, f"Device info: {Util.toHex(devInfo)}")

        handler(device);
    except Exception as e:
        if isinstance(e, BusError):
            Log.e(e.context, f"Error processing configuration.")
            Log.e(e.context, f"Cause:        {e.msg}")

            Log.d(e.context, f"--Full Exception--")
            Log.d(e.context, "".join(traceback.format_exception(None, e, e.__traceback__)))
        elif isinstance(e, OSError):
            Log.e(None, f"I/O Error. ({errno})")
            Log.e(None, f"Cause:        {e.strerror}")
            Log.e(None, f"File:         {e.filename}")
            Log.d(None, f"--Full Exception--")
            Log.d(None, "".join(traceback.format_exception(None, e, e.__traceback__)))
        else:
            Log.e(None, f"An exception occurred:")
            Log.e(None, "".join(traceback.format_exception(None, e, e.__traceback__)))
    except KeyboardInterrupt:
        Log.i(None, "User interrupted test.")

    if intf is not None:
        intf.close()
