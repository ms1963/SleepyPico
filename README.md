# SleepyPico
Weather Station Example for using sleep and dormant mode of Raspberry Pi Pico and SSD1306, BME280.

The source code comprises:
- the Raspberry Pi Pico application SleepyPico that uses the Pico's sleep and dormant modes
- an OLED driver for the SSD1306 display
- a driver for the BME280 sensor (Bosch) that measures humidity, temperature, pressure

The mode is determined by a constant within SlewepyPico.cpp
SLEEP        => Pico wakes up upon RTC timer alarm. Sleep time can be changed  in source code
DORMANT => Pico wakes up when a high edge is detected on the WAKEUP_PIN
NORMA    => Pico does not use the sleep modes
In either mode the system frequency is reduced to 60 MHz to reduce consumption.
The BME280 is executed in forced mode to increase power savings.
The OLED SSD1306 display is turned off and on to reduce energy consumption.

Warning: The current official SDKs for the Raspberry Pi Pico seem to work incorrectly. For example, the Pico freezes after a couple of minutes when using SLEEP mode.

<to do> I will add a circuit diagram soon
  
To build this example application execute "cmake ." and then "make" in the SleepyPico directory.
