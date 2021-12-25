# `ATTINY804` Information

<img src="../assets/attiny-flex-refdesign.svg" width="95%" align="center">

# Device Overview

Please refer to [`ATTINY402`](./device-attiny40x.md) for further details about how
`ATTINY804` operates. It's functionally equivilent and there were no substantive
differences worth noting.

  - 20MHz @ 5V (10MHz @ 3.3V)
  - 8KB Flash
  - 512B SRAM
  - 128B EEPROM

# Pinout

 | Pin | Port | Function      | Usage                                                 |
 | --- | ---- | ------------- | ----------------------------------------------------- |
 | 1   |      | `VCC`         | 5V                                                    |
 | 2   | PA4  | `LUT0.OUT`    | WS2812 Serial Waveform                                |
 | 3   | PA5  | `AIN5`        | Analog Keypad                                         |
 | 4   | PA6  |               | Unused.                                               |
 | 5   | PA7  |               | Unused.                                               |
 | 6   | PB3  |               | Unused.                                               |
 | 7   | PB2  |               | Unused.                                               |
 | 8   | PB1  | `TWI.SDA`     | TWI (I2C) Data Signal                                 |
 | 9   | PB0  | `TWI.SCL`     | TWI (I2C) Clock Signal                                |
 | 10  | PA0  | `UPDI`        | Debug interface.                                      |
 | 11  | PA1  | `USART.TxD`   | Raw LED bitstream.                                    |
 | 12  | PA2  | `USART.RxD`   | Unused.                                               |
 | 13  | PA3  | `USART.XCK`   | Raw LED bitstream clock.                              |
 | 14  |      | `GND`         |                                                       |

# `ATTINY402` Differences

No substantive differences were noted. The EVSYS register has one less 