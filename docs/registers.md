# Register Files

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
- `3: KeyPad Peripheral`

## System Control Registers

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

### System Control Commands

- `SetPage(1) page`
- `SetDeviceAddress(2) address`
- `Reset(3)`

## Device Information Registers

```
Addr        Register            Default Value
--------------------------------------------------------------------------------
00:01       mfg                 0xBA02
02:03       ident               0x2812
04:05       capabilities        (None Currently)
06          version             1
07          reserved
```

## LED Peripheral Registers

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

## KeyPad Peripheral Registers

Keypad support is highly preliminary.

```
Addr        Register            Value
--------------------------------------------------------------------------------
00          status              Unused
01          ctrl                Unused
02          raw                 RAW ADC reading
03          key                 Key Index (0 == No Key Active)
04          state               KeyPad state machine.
                                (Dianostic only.)
05          candidate           RAW ADC Baseline value used for filtering/windowing
                                (Dianostic only.)
06          cal.threshold       Idle voltage reference value.
07          cal.offset          ADC value subtracted prior to calculation of the key index.
08          cal.vstep           Voltage step per key.
09          cal.filter          Number of successive samples needed prior to accepting key.
0A          cal.steadyState     Key is considered stable so long as it remains within this
                                windows (RAW +/- steadyState).
```

Default calibration is:
- Threshold: 0.9V
- Offset: 1V
- VStep: 260mV
- Filter: 64
- Steady State Window: +/- 0.0325V

This is appropriate for a 4x4 keypad[^1] constructed as described in
[sgmne's current sense keypad](https://github.com/sgmne/AnalogKeypad). This provides
a more linear result. Recommend keypads have solid rear ground plane to reduce noise.

There is an CONFIG_KP_HISTORY option that changes this layout, but this feature was
used primarily to for testing.

See [this spreadsheet](../assets/keypad_worksheet.ods) for a worksheet to help with
calibration.

### KeyPad Commands

- None

[^1]: [See ATTINY402 implementation for reference design.](./device-attiny40x.md)
