# SleepyPico
## What it is
Weather Station Example for using sleep and dormant mode of Raspberry Pi Pico and SSD1306, BME280.

The source code comprises:
- the Raspberry Pi Pico application SleepyPico that uses the Pico's sleep and dormant modes. 
- an OLED driver for the SSD1306 display (by Larry Bank)
- a driver for the BME280 sensor (Bosch) that measures humidity, temperature, pressure. 

## Notice about authors
- The OLED-Library was developed by Larry Bank.
- The BME280 SPI driver in C   was developed by Raspberry Pi Organization.
- The BME280 SPI driver in C++ was created by Michael Stal.
- The main application SleepyPico was developed by Michael Stal using code from the Raspberry Pi Organization and also using code from Githubcoder.

## Some remarks on the code
The mode is determined by a constant within SleepyPico.cpp
- SLEEP        => Pico wakes up upon RTC timer alarm. Sleep time can be changed  in source code
- DORMANT => Pico wakes up when a high edge is detected on the WAKEUP_PIN
- NORMAL    => Pico does not use the sleep modes
In either mode the system frequency is reduced to 60 MHz to reduce consumption.
The BME280 is executed in forced mode to increase power savings.
The OLED SSD1306 display is turned off and on to reduce energy consumption.

## Warning
Warning: The current official SDKs for the Raspberry Pi Pico seem to work incorrectly. For example, the Pico freezes after a couple of minutes when using SLEEP mode.

  
## How to build the app
To build this example application, execute "cmake ." and then "make" in the src directory.

## Circuit diagram
On the bottom left the BME280 sensor is depicted. The SSD1306 resides on the bottom right.
The push button connected to 3.3V and GPIO 15 is used in DORMANT mode to wake up the Pico.
The Pico is fed by a power supply (not shown) such as a battery (LiPo, LiIon or any other battery).
![Circuit diagram](https://github.com/ms1963/SleepyPico/blob/main/sleepypico_steckplatine.svg)

