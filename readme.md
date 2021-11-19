# WS2812x and related Smart LED Driver Jellybean 
## Purpose

The goal is to create a a simple peripheral to drive WS2812x using 8-pin AVR
Tiny chips to drive WS2812x and similar LEDs. As a proof of concept, the method
used relies on quirks of the AVR Tiny architecture to minimize the number of
resources required by traditional methods. As well, the maximize the use of 
hardware acceleration by Event System, Timer Type B, USART (or SPI), and CCL.

The implementation provided here uses the USART for its more precise buad rate
generation. It also offers the opportunity to potentially use when LEDs are not
updating.

Configurations:

- I2C Bridge
- SPI Bridge
- Combined I2C/SPI

I created this approach to discover the most efficient hardware supported method
of driving the LEDS on an AVR. There are a number of approaches. By exploiting
certain traits of the microcontroller we can minimize wasted pins.

## Variants

- SPI Direct:
    - Raw SPI to WS2812B signal

- I2C Indirect
    - I2C Interface to control LEDs
    - Configurable I2C Address, defaults
    - (Future) Could support animation and other effects
    - (Future) Larger chips could potentially support off load EEPROM to store sequences
    - (Future) HID support for keypads, quadrature encoders, etc

## Variant: I2C

### Limitations (ATTiny402)

- Flash: 4k
- Pins: 8 (5 available + UPPI)
- RAM: 256 byte

Can support as many LEDs as memory will permit in theory, however, the current
limitation tops off at ~70 LEDs. It is possible to increase this value, but
given the memory constraints I considered it a trade off that allowed for
optimization.
