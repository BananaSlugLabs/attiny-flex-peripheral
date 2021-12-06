# tinyAVR Series-0/1 Multipurpose Peripheral

![Example Demonstration](./assets/top.png)

*Figure: Demo board of a key pad & RGB leds. The keypad uses
[Snaptron BL10280](https://www.snaptron.com/part-number/bl10280/) back lit
domes (the first switch is populated).*

## Purpose

I wanted a framework to build modular components in to my design. For example,
a controller dedicated to keypad input or a driver for those WS2812 LEDs. I
also wanted to see if I could exploit the AVR architecture to build a WS2812
driver using the hardware found in `ATTINY402` without resorting to bitbanging.

Ideally, you'd drop the chip in to your design, load the firmware, and
provision the settings in the EEPROM or custom firmware.

This was also a project where I explored certain methods for making the software
more modular with fewer inter-dependencies without the overhead.

## Status

### Features

  - LED Support: Yes
  - GPIO/KeyPad: Not Yet
  - Configurable Address: Yes

### Supported Devices

  - `ATTINY402`
  - Future: `ATTINY40x`, `ATTINY8xx`, `ATTINY16xx`

Incompatible Devices:
  - Some tinyAVR Series-1 devices may be supported. `ATTINY412` does not have a
    LUT0 output. May have been able to use event system but this conflicts with
    I2C.

### Future Work:

  - Encoders & keypads feature
    - Analog Keypad, Matrix Keypad, Simple Keypad
  - Sleep/Standby
  - Fancy LED Encoding
    - Color spaces (RGB565, Pallet)
    - LED effects feature?
    - State based colors?
  - Improve EEPROM support to persist defaults (beyond device ID)
  - Transparent SPI bridge variant (direct SPI to WS2812 only)

## Test Script (using buspirate)

A test script is provided in `scripts/ledtest.py`. It uses BusPirate with a
[I2C clock stretching patch](https://github.com/BananaSlugLabs/Bus_Pirate).

## Register Files

**Note:** Accesses to this device will require clock stretching. If using a
BusPirate, one must patch the firmware to add support for clock stretching.

The default I2C address is 0x82.

Memory Map

- `0xF0:0xFF System Control Registers`
- `0x00:0x7F Mapped Memory Bank`

Banks

- `0: System Control Registers` (Always mapped to 0xF0 to 0xFF, but mappable to 0x00 to 0x0F.)
- `1: Device Information (Default)`
- `2: LED Peripheral`

### System Control Registers

```
Addr        Register            Default Value
--------------------------------------------------------------------------------
00          status              0
            See below...
01          txCommand           0
            When a transaction is completed this command will be executed.
            Used to, for example, trigger updating of the LEDs after memory
            writes. May also be executed using the command register below.
02          command             0
            Explicit command to be executed when writing to the control memory.
03:06       parameters          0
```

Status Register:

```
Bits        Field               Meaning
--------------------------------------------------------------------------------
0           Reserved
1           Bus Error           A command was already in progress. Reset when 
                                the after active command completes & a new one
                                is written.
4:7         Command Status      Used to determine the results of an executed
                                command.
                                0: Success
                                1: In Progress
                                2: Error - Busy
                                3: Error - Access
                                4: Error - Bad Command
                                5: Error - Bad Argument
```

#### System Control Commands

- `SetPage(1) page`
- `SetDeviceAddress(2) address`
- `Reset(3)`

### Device Information Registers

```
Addr        Register            Default Value
--------------------------------------------------------------------------------
00:01       mfg                 0xBA02
02:03       ident               0x2812
04:05       capabilities        (None Currently)
06          version             1
07          reserved
```

### LED Peripheral Registers

```
Addr        Register            Value
--------------------------------------------------------------------------------
00          controlA            0
            bit 0:              Update
            but 1:              Busy
01          controlB            (Not used)
02          count               CONFIG_LED_COUNT
03:n        LedData[GRB888 * count]
```

### LED Commands

- `RefreshLed(8)`

## Device Support

### `ATTINY402` Information

![ATTINY402 Schematic](./assets/attiny402.png)
*Figure: Reference design for `ATTINY402`.*

Pinout:

  1. VCC
  2. WS2812 LED Output
  3. (currently allocated, may be possible to reuse)
  4. SDA
  5. SCL
  6. UPDI (may be possible to use as input)
  7. (currently allocated to SR reset pulse, may be possible to reuse)
  8. GND

## Implementation Notes

### Memory

Using a typical build configuration, the software as of Dec 2nd 2021, consumed
the following memory resources:

- SRAM memory: 177 bytes (supports 48 LEDs)
  - Stack: 26 (32-bytes est. worst case?)
  - Available Memory: 223 bytes
- Program memory: 2297 bytes
- EEPROM: 1 byte

### KeyPad Notes

Feature not yet implemented.

### WS2812 Notes

To support the WS2812, use the USART, Event System, CCL and TCB to generate the
appropriate WS2812 waveforms.

Although not shown in this project, one can extract the essential elements of
the WS2812 driver and make a transparent simple SPI to WS2812 bridge chip for
those who would rather directly drive the waveform from another microcontroller.

#### USART

The USART is configured in SPI mode. USART was selected primarily due to the
fractional baud rate. This provides more flexibility than the SPI peripheral.

The USART peripheral data register empty interrupt is selected as the **high
priority vector**. This is required to ensure timing constraints are met. The
implementation also supports updating with interrupts disabled. The `sys_abort`
leverages this to emit blinky fault lights.

The interrupt service routine takes approximately 1-2uSec to execute.

#### TCB

TCB timer is configured in single pulse mode that triggers on both edges of
SCK. TCB.WO is fed to the CCL to control the SR latch SET signal. The pulse is
200ns.

![ATTINY402 TCB](./assets/attiny402-tcb.png)

#### CCL

Implemented as a SET/RESET latch.

The latch is asserted when `!Event(TCB) & XCK & TCB.WO`.

The latch is cleared when `(Event(TCB) & USART.XCK & USART.TX) | (Event(TCB) & 
!USART.XCK)`. (This may be off. Need to revisit. See image below.)

![ATTINY402 CCL LUT0](./assets/attiny402-lut0.png)

![ATTINY402 CCL LUT1](./assets/attiny402-lut1.png)

#### Event System

`USART.XCK` is routed to PORTA3 which is then fed back in to the event system
to trigger TCB0. The CCL Event1 is not used and can be removed.

![ATTINY402 Event System](./assets/attiny402-evsys.png)

#### GPIO Restrictions

This implementation minimizes the PINs that are required to be allocated for
this purpose. The following restrictions apply:

  - XCK (unavailable):
    Potentially available when LEDs are not updating.

#### A word of warning...

This implementation relies on the I2C peripheral having a higher priority than
the USART peripheral since USART is shared with the I2C pins. This behavior
may work across multiple devices, but it relies on undocumented behavior of
the microcontroller. This approach cuts down on wasted IO pins, but it was also
necessary on the '402 chip as CCL and USART share pins. In that case, USART was
prioritized over CCL.

## Build Configuration Options

TBD

## Contact Information

Feel free to contact opensource@bananasluglabs.com.

Please reach out to let me know if you find this useful or have included it in
a product or project.