# Design & Validation on `ATTINY402`

As 2021 comes to a close, the firmware in this project is something approximating an almost feature
complete milestone. Prior to refining the firmware further, tests were conducted to evaluate power
management, resource utilization, and robustness. A total of 30 tests were conducted covering 6
firmware configurations. The results of those tests were largely positive though more extensive
stress testing of the the I2C peripheral implementation is necessary.

This section will describe the approach to power management, certain implementation details, and 
will end by discussing the findings and possible improvements.

*Update: Since this was prepared, I have updated my test fixture and can now isolate the current
to only what is consumed by the MCU itself. Hopefully I'll take a 2nd look at this and include
mode tests now that I am not limited by setup.*

*MCU with default configuration, 800KHz sleep frequency, etc consumes \~750uA.*

## Power Management Methods

Prior to discussing test results this section describes the implementation used in this project.
The mechanism and implementation details for tinyAVR Series-0/1 are described below. Depending on
the configuration, the microcontroller will consume only 1% to 40% of the amount of power used
in the always active case.

The following section covers the power management modes. Immediately following will be test the
sequences used to perform measurements.

### Modes of Operation

There are two strategies used to lower power consumption. The use of the sleep modes and reducing
the frequency of the system clock. When the system clock is reduced, the ADC sampling rating is
also effectively reduced.

Adjusting the sampling rate of the ADC at the peripheral had a marginal impact. As documented in
the datasheet, the electrical characteristics of the `ATTINY402` seems to support this onservation.
Unfortunately the datasheet lacks good characterization across a wider set of operating conditions.

<table>

<tr> <th> Group <th> PM Mode <th> Slowclock <th>
<th> Description

<tr> <td> 1 <td> None <td> Disabled <td> 🔴 <td>

No power management functionality has been included in the firmware. This reduces the firmware
size, 

<tr> <td> 2 <td> Idle <td> Disabled <td> 🔴 <td>

The CPU stops executing code. Active peripherals remain active and the CPU will begin executing
again when an interrupt is asserted. `ATTINY402` characteristics are reported to be typically 2.8mA
and 6.3mA worst case at 5V, 20MHz.

Entry Conditions:

  - No signals have been asserted. See `Sys_Signal` and `sys_signal`.
  - Typically this means no pending I2C transactions, keypad is steady state,
    and no pending LED updates are occurring.

Wake source exit conditions:

  - **I2C/TWI Address Match** \
    New I2C transaction to process from the controller. Signals `Sys_SignalWakeLock` to ensure
    the MCU does not enter standby during the middle of an I2C transaction. If a command
    is to be executed, `Sys_SignalWorkerPending` is also signaled.
  - **ADC Window Comparator / Sample Available** \
    Captured a new sample to process OR is alerting MCU that input signal has changed sufficiently
    enough that new input is being provided. When a new sample is captured, `Sys_SignalWakeLock
    | Sys_SignalWorkerPending` is asserted to notify the system that post processing may be
    necessary.
  - **RTC** \
    During normal operation when either `Sys_SignalWakeLock | Sys_SignalWorkerPending` is 
    asserted, the RTC's compare register is updated to `RTC.CNT + CONFIG_SLEEP_TIMEOUT`
    periodically. This is done so that, once the MCU enters IDLE, if no other interrupts
    are raised when the compare match occurs, it will fire and signal `Sys_SignalEnterStandby` to
    enter standby mode.
  - **USART** \
    Handle WS2812 Serialization interrupts (and go back to sleep)

<tr> <td> 3 <td> Standby <td> Disabled <td> 🔴 <td>

A subset of of peripherals are able to continue operating in standby. As a result, power
consumption is a function of the application and the choice of peripherals, and their
configurations, etc.

Entry Conditions:

  - The only signal asserted is `Sys_SignalEnterStandby` (which was fired by an RTC compare match).

The following peripherals are active in standby:

  - WDT
  - BOD
  - EVSYS
  - **I2C/TWI**[^3]

If requested the following will be active in standby:

  - RTC
  - CCL
  - AC
  - **ADC**
  - TCB

Wake source exit conditions:

  - **I2C/TWI Address Match** (See previous remarks.)
  - **ADC Window Comparator** (See previous remarks.)


Inactive peripherals:

  - CCL
  - USART
  - TCB0

<tr> <td> 4 <td> Standby <td> 800KHz <td> 🟢 <td>

To reduce current consumption further the F<sub>cpu</sub> is reduced to approximately 1MHz.

<tr> <td> 5 <td> Powerdown <td> Disabled <td> 🔴 <td>

A small number of peripherals continue operate in this mode. Power consumption is a
function of the peripherals that have been configured to operate in powerdown mode. `ATTINY402`
characteristics are reported in the sub-10uA range at 3V. No characteristics were available at 5V.

Entry Conditions:

  - The only signal asserted is `Sys_SignalEnterStandby` (which was fired by an RTC compare match).

The following peripherals are active in powerdown mode:

  - WDT
  - BOD
  - EVSYS
  - **I2C/TWI**[^3]

Wake source exit conditions:

  - **I2C/TWI Address Match** (See previous remarks.)

As ADC is not available in powerdown mode, it is not possible for a user to wake the peripheral
by the keypad.

<tr> <td> 6 <td> Powerdown <td> 800KHz <td> 🟢 <td>

Same as above (4). The clock reduction provides no benefit in this example because the the clock
is stopped entirely shortly there after. Worse yet, when it resumes, the I2C peripheral will be
sluggish and slow to respond initially. In the prior example, the ADC continued operating and
could respond to key presses and the I2C bus.

</table>

See [^ATTINY402] for more details.

<table>

<tr> <th> Sequence <th> Symbol <th> <th> Use Case

<tr> <td> 1 <td> Disabled <td>

No power management functionality has been included in the firmware. This reduces the firmware
size, 

</table>

### Test Sequences


| Symbol                | Meaning                                                                                              |
| --------------------: | ---------------------------------------------------------------------------------------------------- |
| 🔴                    | Function disabled at compile time.                                                                   |
| 🔵                    | Function enabled, but not actively in use.                                                           |
| 🟢                    | Function enabled, and actively in use. (i.e. I2C traffic, key presses.)                              |

| Seq. | Variable      | Test Case                                                                    | RGBLed | Keypad | I2C  |
| :--: | :-----------: | ---------------------------------------------------------------------------: | :----: | :----: | :--: | 
|  1A  | I<sub>idle    | Default config; No I2C, LED, or Key activity.                                |   🔵   |   🔵   |  🔵 |
|  1B  | I<sub>i2c     | Default config; I2C actively writing, no LED or Key activity.                |   🔵   |   🔵   |  🟢 |
|  1C  | I<sub>keypad  | Default config; Sequence through key presses, I2C and LED inactive.          |   🔵   |   🟢   |  🔵 |
|  2   | I<sub>nKeyPad | Disable KeyPad at compile time; otherwise similar to I<sub>idle</sub>.       |   🔵   |   🔴   |  🔵 |
|  3   | I<sub>nLow    | Disable all functionality; Used to obtain effective lower limit.             |   🔴   |   🔴   |  🔴 |


## Result Summary

| PM Support   | F<sub>stby       |     Flash    |   RAM     | I<sub>idle     |  % ⬇️  | I<sub>i2c |  % ⬇️  | I<sub>keypad |  % ⬇️  |
| :----------: | ---------------: | -----------: | --------: | :------------: | :----: | :-------: | :----: | :----------: | :----: |
| None         |               🔴 |   2811 bytes | 185 bytes |    10.46mA     |        |   12.54mA |        |   10.56mA    |        |
| Idle         |               🔴 |   2843 bytes | 185 bytes |     4.04mA     | 38.62% |    5.90mA | 47.05% |   4.04mA[^1] | 38.26% |
| Standby      |               🔴 |   3023 bytes | 185 bytes |     3.06mA     | 29.25% |    5.94mA | 47.37% |   4.06mA     | 38.45% |
| Standby      |        800 KHz🟢 |   3039 bytes | 185 bytes |      904uA[^4] | 8.64%  |    5.94mA | 47.37% |   4.06mA     | 38.45% |
| ... adjusted |                  |              |           |    \~850uA[^4] |        |           |        |              |        |
| Powerdown    |               🔴 |   3023 bytes | 185 bytes |     68.2uA[^4] | 0.65%  |    5.94mA | 47.37% |   72.0uA[^2] |  0.68% |
| ... adjusted |                  |              |           |     \<10uA[^4] |        |           |        |              |        |
| Powerdown    |        800 KHz🟢 |   3039 bytes | 185 bytes |     68.2uA[^4] | 0.65%  |    5.94mA | 47.37% |   72.0uA[^2] |  0.68% |
| ... adjusted |                  |              |           |     \<10uA[^4] |        |           |        |              |        |


Some results have been adjusted to compensate for a measurement error. The error applies to all measurements, but select
measurements have been shown as adjusted above.[^4] For complete results covering all test cases see [^5].

## Further Investigations

  - There appear to be an issues with I2C errors when exiting standby. It may be due to a BusPirate's poor clock stretching
    support (which was a quick and dirty patch applied to bus pirate).
  - Update test procedure to isolate I2C and other sources impacting power consumption.

## Test Plan

### Preparation

Equipment Setup:

  - Benchtop Power Supply: 5V, limit 200mA
  - Multimeter: High side, set to mA range initially.
  - BusPirate: To generate I2C traffic for test 1B.

Bench Configuration:

  - ATTiny402
    - Mounted on an SMT protoboard.
  - 4x4 Keypad
    - Powered & tested 
  - Microchip Snap4
    - To load firmware
  - RGB LEDs are unpowered

Limitations & Improvements:

  - Isolate `ATTINY402` power rails because I<sub>i2c</sub> is measuring the I2C open drain losses.
    - Snap4 and BusPirate contributed a sizable error to the measurements.[^4]
  - Buy a decent multimeter. I have good test equipment... but not a good multimeter.
  - Need to replace BusPirate with something better.

### Sequence 1: Typical Performance

  1. Load firmware for sequence 1.
  2. Disconnect BusPirate & Snap4.[^4]
  3. Apply power and wait 10 seconds.
  4. Select MAX mode on the multimeter, wait 10 seconds.
  5. **Record result for I<sub>idle</sub> (1A).**
  6. Connect buspirate; (Record offset, if any?)
  7. Run script to generate I2C traffic. (See scripts/traffic.py for example.)
    - I adapted the ledtest.py script to send 72 bytes to the LED page indefinitely.
  8. Reset statistics on multimeter, select MAX and wait 10 seconds
  9. **Record the result for I<sub>i2c</sub> (1B).**
  10. Disconnect buspirate.
  11. Reset statistics on multimeter, select MAX.
  12. Press each key a couple times.
  13. **Record the result for I<sub>keypad</sub> (1C).**

### Sequence 2 KeyPad Disabled / Sequence 3: Everything Disabled

  1. Load firmware for sequence 2 or 3.
  2. Disconnect BusPirate & Snap4.[^4]
  3. Apply power and wait 10 seconds.
  4. Select MAX mode on the multimeter, wait 10 seconds.
  5. **Record the result for I<sub>nKeyPad</sub> (2) or I<sub>nLow</sub> (3).**

[^ATTINY402]: [DS40002318A ATTINY402 Datasheet, 11. Sleep Controller, Page 90 - 96](https://www.microchip.com/en-us/product/ATTINY402)

[^1]:<p>Typical keypad entry. To optimize computational performance the ADC's window comparator
    function is activated when the keypad input has reached a steady state. When rapidly alternating between
    a high value key and a low value key, I<sub>keypad</sub> will be between 4.04mA (I<sub>keypad(idle)</sub>)
    and 10.56mA (I<sub>keypad(none)</sub>). In practice, however, exceeding I<sub>keypad(idle)</sub> involves
    some level of button mashing.

[^2]: Increase in current consumption attributed to current mirror in keypad.

[^3]: <p>TWI peripheral is clocked using the `SCL` input. This imposes a limitation on certain use cases that are not used in
    this project.

[^4]: <p>Due to test setup when measurements were performed, there is an offset. It is particularly pronounced for the
    measurements in the uA. About 55uA to 65uA can be attributed to the Snap4 ICD. Thus, I<sub>idle</sub> (and to a lesser
    extent I<sub>keypad</sub>) is overstated. For group 4, I<sub>idle</sub> was \~850uA. For group 6 & 5, I<sub>idle</sub>
    read out was \~4uA. It's sufficiently low that I do not place much faith the accuracy of the multimeter used.

[^5]: Full Results:

    | Grp.[^1] | Seq.[^2] | Vcc  | F<sub>on | F<sub>stby | RGBLed | Keypad | I2C | PM Config | I<sub>typ | Flash | RAM |
    | :------: | :------: | ---: | -------: | ---------: | :----: | :----: | :-: | :-------: | --------: | ----: | ---: |
    |     1    |    1A    | 5.0V |  20MHz   |            |   🔵   |   🔵   |  🔵 |   None    |  10.46mA    |  2811 |  185 |
    |     1    |    1B    | 5.0V |  20MHz   |            |   🔵   |   🔵   |  🟢 |   None    |  12.54mA    |  2811 |  185 |
    |     2    |    1C    | 5.0V |  20MHz   |            |   🔵   |   🟢   |  🔵 |   None    |  10.56mA    |  2843 |  185 |
    |     1    |    2     | 5.0V |  20MHz   |            |   🔵   |   🔴   |  🔵 |   None    |   9.77mA    |  2234 |  176 |
    |     1    |    3     | 5.0V |  20MHz   |            |   🔴   |   🔴   |  🔴 |   None    |   8.93mA    |   432 |    4 |
    |     2    |    1A    | 5.0V |  20MHz   |            |   🔵   |   🔵   |  🔵 |   Idle    |   4.04mA    |  2843 |  185 |
    |     2    |    1B    | 5.0V |  20MHz   |            |   🔵   |   🔵   |  🟢 |   Idle    |   5.90mA    |  2843 |  185 |
    |     2    |    1C    | 5.0V |  20MHz   |            |   🔵   |   🟢   |  🔵 |   Idle    |   4.04mA    |  2843 |  185 |
    |     2    |    2     | 5.0V |  20MHz   |            |   🔵   |   🔴   |  🔵 |   Idle    |   3.57mA    |  2266 |  176 |
    |     2    |    3     | 5.0V |  20MHz   |            |   🔴   |   🔴   |  🔴 |   Idle    |   2.57mA    |   464 |    2 |
    |     3    |    1A    | 5.0V |  20MHz   |            |   🔵   |   🔵   |  🔵 |  Standby  |   3.06mA    |  3023 |  185 | 
    |     3    |    1B    | 5.0V |  20MHz   |            |   🔵   |   🔵   |  🟢 |  Standby  |   5.94mA    |  3023 |  185 | 
    |     3    |    1C    | 5.0V |  20MHz   |            |   🔵   |   🟢   |  🔵 |  Standby  |   4.06mA    |  3023 |  185 | 
    |     3    |    2     | 5.0V |  20MHz   |            |   🔵   |   🔴   |  🔵 |  Standby  |   68.2uA    |  2446 |  176 | 
    |     3    |    3     | 5.0V |  20MHz   |            |   🔴   |   🔴   |  🔴 |  Standby  |   68.2uA    |   630 |    2 |
    |     4    |    1A    | 5.0V |  20MHz   |     800KHz |   🔵   |   🔵   |  🔵 |  Standby  |    904uA    |  3039 |  185 | 
    |     4    |    1B    | 5.0V |  20MHz   |     800KHz |   🔵   |   🔵   |  🟢 |  Standby  |   5.94mA    |  3039 |  185 |
    |     4    |    1C    | 5.0V |  20MHz   |     800KHz |   🔵   |   🟢   |  🔵 |  Standby  |   4.06mA    |  3039 |  185 |
    |     4    |    2     | 5.0V |  20MHz   |     800KHz |   🔵   |   🔴   |  🔵 |  Standby  |   68.2uA    |  2462 |  176 |
    |     4    |    3     | 5.0V |  20MHz   |     800KHz |   🔴   |   🔴   |  🔴 |  Standby  |   68.2uA    |   646 |    2 |
    |     5    |    1A    | 5.0V |  20MHz   |            |   🔵   |   🔵   |  🔵 | Powerdown |   68.2uA    |  3023 |  185 |
    |     5    |    1B    | 5.0V |  20MHz   |            |   🔵   |   🔵   |  🟢 | Powerdown |   5.94mA    |  3023 |  185 |
    |     5    |    1C    | 5.0V |  20MHz   |            |   🔵   |   🟢   |  🔵 | Powerdown |   72.0uA    |  3023 |  185 |
    |     5    |    2     | 5.0V |  20MHz   |            |   🔵   |   🔴   |  🔵 | Powerdown |   68.2uA    |  2446 |  176 |
    |     5    |    3     | 5.0V |  20MHz   |            |   🔴   |   🔴   |  🔴 | Powerdown |   68.2uA    |   630 |    2 |
    |     6    |    1A    | 5.0V |  20MHz   |     800KHz |   🔵   |   🔵   |  🔵 | Powerdown |   68.2uA    |  3039 |  185 |
    |     6    |    1B    | 5.0V |  20MHz   |     800KHz |   🔵   |   🔵   |  🟢 | Powerdown |   5.94mA    |  3039 |  185 |
    |     6    |    1C    | 5.0V |  20MHz   |     800KHz |   🔵   |   🟢   |  🔵 | Powerdown |   72.0uA    |  3039 |  185 |
    |     6    |    2     | 5.0V |  20MHz   |     800KHz |   🔵   |   🔴   |  🔵 | Powerdown |   68.2uA    |  2462 |  176 |
    |     6    |    3     | 5.0V |  20MHz   |     800KHz |   🔴   |   🔴   |  🔴 | Powerdown |   68.2uA    |   646 |    2 |

