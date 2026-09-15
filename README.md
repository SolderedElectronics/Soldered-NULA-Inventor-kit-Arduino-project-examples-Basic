# Inventor kit: Arduino examples

[![Compile examples](https://github.com/SolderedElectronics/Soldered-NULA-Inventor-kit-Arduino-project-examples/actions/workflows/compile-examples.yml/badge.svg)](https://github.com/SolderedElectronics/Soldered-NULA-Inventor-kit-Arduino-project-examples/actions/workflows/compile-examples.yml)

Arduino example sketches for the Soldered Inventor kit, built around the
[NULA DeepSleep ESP32-S3](https://soldered.com/product/nula-deepsleep-esp32-s3-board/).

In these examples the board measures its own circuits. Example 1.1 reads the voltage across the resistor in series
with an LED, then works out the LED's forward voltage and the current through it. Example 3.1 captures a capacitor
charging and prints the time constant it measured beside the one it calculated. Example 4.2 finds the two pulse
widths your servo actually stops at, which will not be the ones written in the file.

Every number the sketches print comes from your parts, not from a datasheet.

## What is on the board

An ESP32-S3 with a CH340 USB-serial chip, an onboard WS2812B RGB LED, a PCF85063A real-time clock, Li-Ion charging
and a Qwiic connector. Deep-sleep current is in the single microamps.

There are 21 examples across eight sections. Sections 0 to 4 stay on the breadboard and use the ADC to measure
resistors, capacitors, diodes and transistors. Sections 5 and 6 add the Qwiic modules and deep sleep, and 6.2 hands
the threshold to the sensor so the board only wakes to ask whether anything happened. Section 7 combines the lot
into four projects, none of which blocks.

## Getting started

### 1. Install the board

1. In the Arduino IDE, open **File -> Preferences** and add this URL to *Additional boards manager URLs*:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
2. Open **Tools -> Board -> Boards Manager**, search for `esp32` and install **esp32 by Espressif Systems**.
3. Select **Tools -> Board -> ESP32 Arduino -> Soldered NULA DeepSleep ESP32S3**.

### 2. Install the CH340 driver

The USB-C port runs through a CH340. Without its driver the board never appears as a serial port, so there is
nothing to pick in **Tools -> Port**. Windows and macOS both need it; on Linux it is already in the kernel.

Driver and troubleshooting: [docs.soldered.com/ch340-drivers](https://docs.soldered.com/ch340-drivers)

### 3. Install the libraries

Open **Tools -> Manage Libraries** and install these three, all by Soldered Electronics:

| Library | Needed by | Header it provides |
| --- | --- | --- |
| **Soldered TMP117** | 5.1, 6.2, 7.1, 7.2, 7.4 | `Soldered-TMP117.h` |
| **Soldered Library for Relays** | 5.2, 7.1, 7.4 | `Relays-SOLDERED.h` |
| **Soldered PCF85063A RTC Arduino Library** | 6.1, 7.2 | `PCF85063A-SOLDERED.h` |

Sections 0 to 4 need no libraries at all.

### 4. Open an example

Each example lives in its own folder and opens with **File -> Open**. Start at `0.1` and work down: the later
examples point back at the earlier ones rather than explaining the same thing twice.

Most sketches have a `MODE` constant near the top. Each mode is one step of the same lesson, usually with a
rewiring in between, so work through them in order instead of uploading once.

## The examples

### 0. Getting started

| Example | What you learn | Hardware |
| --- | --- | --- |
| [0.1 First blink](0_Getting_Started/0.1_First_Blink) | That the board, the driver and the toolchain all work, before anything harder | LED, 330 Ω |
| [0.2 Reading a potentiometer](0_Getting_Started/0.2_Reading_A_Potentiometer) | Analog reads in counts and in millivolts, and two traces at once on the Serial Plotter | Potentiometer |

### 1. LEDs and PWM

| Example | What you learn | Hardware |
| --- | --- | --- |
| [1.1 LED current and resistor sizing](1_LEDs_And_PWM/1.1_LED_Current_And_Resistor_Sizing) | Forward voltage and current measured from your own LED, and where the number 330 comes from | LED, 330 Ω, 10 kΩ |
| [1.2 Gamma fading and RGB colour mixing](1_LEDs_And_PWM/1.2_Gamma_Fading_And_RGB_Colour_Mixing) | Hardware PWM, why duty cycle is not brightness, and the difference between driving an LED and messaging one | LED, 330 Ω, potentiometer, onboard WS2812B |

### 2. Buttons and analog inputs

| Example | What you learn | Hardware |
| --- | --- | --- |
| [2.1 Pull-up resistors and button debouncing](2_Buttons_And_Analog_Inputs/2.1_Pull_Up_Resistors_And_Button_Debouncing) | Floating pins, pull-up against pull-down, the internal pull-up, and your button's real bounce duration | 2× pushbutton, 3× 10 kΩ, 100 nF, LED, 330 Ω |
| [2.2 ADC noise, averaging and calibration](2_Buttons_And_Analog_Inputs/2.2_ADC_Noise_Averaging_And_Calibration) | That a reading is an estimate with a spread, what averaging costs, and the limit averaging cannot pass | Potentiometer |
| [2.3 Photoresistor divider with hysteresis](2_Buttons_And_Analog_Inputs/2.3_Photoresistor_Divider_With_Hysteresis) | Picking a component value from a measurement, and why one threshold is unstable | Photoresistor, 3× 10 kΩ, LED, 330 Ω |

### 3. Capacitors, diodes and transistors

| Example | What you learn | Hardware |
| --- | --- | --- |
| [3.1 Measuring an RC time constant](3_Capacitors_Diodes_And_Transistors/3.1_Measuring_An_RC_Time_Constant) | That the formula is arithmetic you can verify, and how to tell an untrustworthy measurement from a disappointing one | 2× 10 kΩ, 100 nF, 10 µF, pushbutton |
| [3.2 Diode forward voltage](3_Capacitors_Diodes_And_Transistors/3.2_Diode_Forward_Voltage) | That 0.7 V is the middle of a range, and how to spot a reading the ADC has clipped | 1N4148, 330 Ω, potentiometer |
| [3.3 Transistor base resistor and saturation](3_Capacitors_Diodes_And_Transistors/3.3_Transistor_Base_Resistor_And_Saturation) | That a transistor is a current amplifier, what saturation is, and how to size a base resistor | NPN, 10 kΩ, LED, 330 Ω, potentiometer |

### 4. Shift register and actuators

| Example | What you learn | Hardware |
| --- | --- | --- |
| [4.1 Eight LEDs from three pins](4_Shift_Register_And_Actuators/4.1_Eight_LEDs_From_Three_Pins) | How a shift register moves data, that driver choice is worth a hundredfold, and what a latch is for | SN74HC595N, 8× LED, 8× 330 Ω, 100 nF, potentiometer |
| [4.2 Servo pulses without a library](4_Shift_Register_And_Actuators/4.2_Servo_Pulses_Without_A_Library) | What a servo signal actually is, that parts are not identical, rate limiting, and why a motor gets its own capacitors | SG90 servo, potentiometer, 10 µF, 100 nF |
| [4.3 Driving a buzzer through a transistor](4_Shift_Register_And_Actuators/4.3_Driving_A_Buzzer_Through_A_Transistor) | Why a passive buzzer needs an alternating signal, mechanical resonance, and what a flyback diode is for | Buzzer, NPN, 10 kΩ, 1N4148, potentiometer |

### 5. Qwiic modules

| Example | What you learn | Hardware |
| --- | --- | --- |
| [5.1 Scanning the bus and reading temperature](5_Qwiic_Modules/5.1_Scanning_The_Bus_And_Reading_Temperature) | What is really on the I2C bus, and how to tell a reading from a number a missing sensor handed back | TMP117, Qwiic cable |
| [5.2 Relay switching with hysteresis and lockout](5_Qwiic_Modules/5.2_Relay_Switching_With_Hysteresis_And_Lockout) | That a mechanical output needs rules a logic output does not, and separating what you want from what you may do | Qwiic relay, Qwiic cable, potentiometer, LED, 330 Ω |

### 6. Low power

| Example | What you learn | Hardware |
| --- | --- | --- |
| [6.1 Deep sleep, wake sources and RTC memory](6_Low_Power/6.1_Deep_Sleep_Wake_Sources_And_RTC_Memory) | That deep sleep is stopping and starting, not pausing, and that an interval and a schedule are different things | Pushbutton, 10 kΩ, LED, 330 Ω |
| [6.2 Checking a sensor alert flag](6_Low_Power/6.2_Checking_A_Sensor_Alert_Flag) | That the sensor can hold the threshold and decide when the processor is needed, and what a wake costs when it finds nothing | TMP117, Qwiic cable, LED, 330 Ω |

### 7. Projects

| Project | What it combines | Hardware |
| --- | --- | --- |
| [7.1 Thermostat with relay and alarm](7_Projects/7.1_Thermostat_With_Relay_And_Alarm) | Setpoint, hysteresis, relay lockout, LED bar readout and a fault alarm, in one non-blocking state machine | TMP117, Qwiic relay, 2× Qwiic cable, SN74HC595N, 8× LED, 8× 330 Ω, 100 nF, potentiometer, buzzer, NPN, 10 kΩ, 1N4148 |
| [7.2 Wi-Fi temperature logger](7_Projects/7.2_WiFi_Temperature_Logger) | Wake on RTC alarm or sensor alert, an RTC-memory ring buffer, retries, and where the energy actually goes | TMP117, Qwiic cable, 1 jumper wire, 10 kΩ, pushbutton, onboard RGB LED and RTC |
| [7.3 Two-sensor sun tracker](7_Projects/7.3_Two_Sensor_Sun_Tracker) | Closed-loop control taught through its failure: dead band, rate limit, and per-sensor calibration | 2× photoresistor, 2× 10 kΩ, SG90 servo, 10 µF, 100 nF, SN74HC595N, 8× LED, 8× 330 Ω, potentiometer |
| [7.4 Bench controller](7_Projects/7.4_Bench_Controller) | Architecture. Five independent jobs, none of which blocks, plus the same thing written the wrong way | Everything in the kit |

## Pins used by each example

Every example is standalone and picks whichever pins suit it, so you will rewire between them. All numbers are the
`IO` numbers printed on the board.

| Purpose | Pin | Notes |
| --- | --- | --- |
| Single LED | IO38 | Always through its own 330 Ω resistor |
| Analog inputs | IO4, IO5, IO6, IO7 | Pots, photoresistors and circuit taps |
| Wake-capable button | IO21 | The only button that can wake the board from deep sleep |
| Other buttons | IO39, IO40, IO41, IO42 | 7.4 uses all five at once |
| Shift register | IO11 data, IO12 clock, IO10 latch, IO13 output enable | IO11 and IO12 are also hardware MOSI and SCK |
| Servo signal | IO47 | Power from VCC, never 3V3 |
| Buzzer, through the transistor | IO48 | |
| TMP117 alert | IO14 | RTC-capable, so it can wake the board |
| Qwiic / I2C | IO8 SDA, IO9 SCL | No configuration needed, `Wire.begin()` picks these |

Two more are wired on the board itself and need nothing from you: the WS2812B RGB LED is on **IO2** through a 1 kΩ
series resistor, and the PCF85063A RTC's interrupt output is on **IO17**, already pulled up to 3V3. Neither is
brought out to a header.

### Pins to leave alone

| Pin | Why |
| --- | --- |
| IO19, IO20 | The ESP32-S3's native USB data lines |
| IO43, IO44 | UART0, the serial console |
| IO0 | Boot strap pin, and the onboard USER button |
| IO2 | The onboard WS2812B RGB LED, not brought out to a header |

### Why every analog pin is IO4 to IO10

The ESP32-S3 has two ADCs, and ADC2 shares hardware with the radio. Once Wi-Fi starts, an `analogRead()` on an ADC2
pin stops returning anything useful, and on this board IO11 upwards are ADC2. Section 7 turns Wi-Fi on, so every
analog input in this kit stays on ADC1. IO8 and IO9 are ADC1 as well but belong to the Qwiic bus, which leaves IO4,
IO5, IO6, IO7 and IO10.

### Buttons and pull-ups

Buttons use the chip's **internal pull-up resistors**, switched on with `INPUT_PULLUP`, so no resistor is needed on
the breadboard: one side to the pin, the other straight to GND. The readings come out the other way round from what
you might expect, which is called active low: the pin reads **HIGH while released** and **LOW while pressed**.

The exception is any button that has to wake the board from deep sleep. The internal pull-ups switch off during
sleep, so those need a **10 kΩ to 3V3** on the breadboard as well, or the pin floats and wakes the board at random.

## Kit contents

The *At once* column is the most any single example needs at the same time, so it tells you how much of each
part is spare and how much is working stock.

| Component | Quantity | At once | Notes |
| --- | --- | --- | --- |
| NULA DeepSleep ESP32-S3 | 1 | 1 | Male headers fitted. ESP32-S3, CH340, PCF85063A RTC, WS2812B, Qwiic |
| Large breadboard | 1 | 1 | |
| Jumper wire set | 1 | | |
| [TMP117 temperature sensor](https://soldered.com/product/temperature-sensor-tmp117-breakout/) | 1 | 1 | Qwiic, with the ALR pad broken out |
| 1-channel relay | 1 | 1 | Qwiic |
| Qwiic cable, 10 cm | 5 | 2 | Both needed in 5.1 and 7.1 |
| SN74HC595N shift register | 1 | 1 | DIP-16 |
| SG90 servo | 1 | 1 | Powered from VCC, never 3V3 |
| Buzzer QMB-09B-03 | 1 | 1 | Passive, so it needs an alternating signal |
| 5 mm LED | pack | 8 | Eight at once in 4.1, 7.1, 7.3 and 7.4 |
| 10 kΩ potentiometer | 3 | 3 | **No spares.** All three in 7.4 |
| Small THT pushbutton | 5 | 5 | **No spares.** All five in 7.4 |
| 10 kΩ photoresistor | 5 | 2 | |
| NPN transistor, TO-92 | 5 | 1 | |
| 1N4148 diode | 5 | 2 | Band is the cathode |
| 330 Ω resistor | 25 | 8 | One in series with every LED |
| 10 kΩ resistor | 15 | 3 | |
| 100 nF capacitor | 5 | 1 | |
| 10 µF 25 V electrolytic capacitor | 5 | 1 | Polarised, stripe to GND |

No battery is included. The board charges a Li-Ion cell and sleeps in the single microamps, but measuring that
needs a cell you supply yourself, because USB current tells you nothing about it.

## Constants to measure for yourself

`RESONANT_HZ` in 4.3 and the servo pulse limits in 4.2 are starting values, not answers. Measure your own on the
pages that teach you how, then carry them into 7.1, 7.3 and 7.4.

## Continuous integration

Every example is compiled for the NULA DeepSleep ESP32-S3 on every push by the
[Compile examples](.github/workflows/compile-examples.yml) workflow, so anything on the main branch is known to
build.

## Need help?

- Board documentation: [docs.soldered.com/deepsleep-esp32-s3/overview](https://docs.soldered.com/deepsleep-esp32-s3/overview)
- Contact Soldered: [soldered.com/contact](https://soldered.com/contact)
- Found a mistake in an example? Open an issue or a pull request, both are welcome.
