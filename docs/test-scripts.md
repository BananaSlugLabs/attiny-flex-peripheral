# Test Scripts

Test scripts are provided in `scripts/` folder. It uses BusPirate with a I2C clock stretching patch.[^1] The
implementation is based on the bus pirate lite reference python library (stripped down).

  - [scripts/ledtest.py](../scripts/ledtest.py): Fades LEDs in and out across a variety of colors.
  - [scripts/ledtest.py](../scripts/keypad.py): Polls keypad peripheral and reports to console.
  - [scripts/ledtest.py](../scripts/traffic.py): Generates pointless I2C traffic (used during test & validation).

Modifications may be needed since it always uses /dev/ttyUSB0. There doesn't seem to be a good way to auto detect
BusPirate 3.x devices as they report as standard FTDI devices.

[^1]: [BusPirate firmware with I2C clock stretching patch.](https://github.com/BananaSlugLabs/Bus_Pirate)