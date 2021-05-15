# SleepyPico
## What it is
Weather Station Example for using sleep and dormant mode of Raspberry Pi Pico and SSD1306, BME280.

The source code comprises:
- the Raspberry Pi Pico application SleepyPico that uses the Pico's sleep and dormant modes. 
- The library class Sleep that is responsible for the sleep functionality and provides an Arduino-like event loop.
- an OLED driver for the SSD1306 display (by Larry Bank)
- a driver for the BME280 sensor (Bosch) that measures humidity, temperature, pressure. 

## Notice about authors
- The OLED-Library was developed by Larry Bank.
- The BME280 SPI driver in C   was developed by Raspberry Pi Organization.
- The BME280 SPI driver in C++ was created by Michael Stal.
- The Sleep class was created by Michael Stal.
- The main application SleepyPico was developed by Michael Stal using code from the Raspberry Pi Organization and also using code from Githubcoder.

## Some remarks on the code
The mode is determined by an argument passed to the Sleep class (Sleep.cpp, Sleep.hpp)
- SLEEP        => Pico wakes up upon RTC timer alarm. Sleep time can be changed  is also passed as an argument.
- DORMANT => Pico wakes up when a high edge is detected on the WAKEUP_PIN
- NORMAL    => Pico does not use the sleep modes
In either mode the system frequency is reduced to 60 MHz to reduce consumption.
The BME280 is executed in forced mode to increase power savings.
The OLED SSD1306 display is turned off and on to reduce energy consumption.

## Example
Here is an example how to use the Sleep class.
A detailed example is provided by SleepyPico.cpp.
    

    datetime_t start = .... // Start date to which RTC is set
    datetime_t end   = .... // DateTime at which alarm is triggered in SLEEP mode.
    #define WAKEUP_PIN      // GPIO Pin to wait for high signal edges in DORMANT mode.
    
    void setup() {
      // called once in the beginning
    }
    
    void loop() {
      // called in each iteration of the event loop
    }
    
    int main() {
      stdio_init_all();
      sleep_ms(3000); // required by some OS
      // Change frequency of Pico to a lower value
      printf("Changing system clock to lower frequency: %d KHz\n", SYSTEM_FREQUENCY_KHZ);
      set_sys_clock_khz(SYSTEM_FREQUENCY_KHZ, true);
    
    
      // configure Sleep instance
      // Dormant mode
      // pointers to our loop() and setup() functions
      // start and end of alarm period
      // WAKEUP_PIN where high edges are detected
      Sleep::instance().configure(Sleep::MODE::DORMANT, &setup, &loop,
                           start, end, WAKEUP_PIN);
      // show clock frequencies
      Sleep::instance().measure_freqs();
      // start event loop
      Sleep::instance().run(); 
      return 0;
     }


## Warning
Warning: The current official SDKs for the Raspberry Pi Pico seem to work incorrectly. For example, the Pico freezes after a couple of minutes when using sleep modes. UART output is not visible after sleeping.

  
## How to build the app
To build this example application, execute "cmake ." and then "make" in the src directory.

## Circuit diagram
On the bottom left the BME280 sensor is depicted. The SSD1306 resides on the bottom right.
The push button connected to 3.3V and GPIO 15 is used in DORMANT mode to wake up the Pico.
The Pico is fed by a power supply (not shown) such as a battery (LiPo, LiIon or any other battery).
![Circuit diagram](https://github.com/ms1963/SleepyPico/blob/main/sleepypico_steckplatine.svg)

There is a small video cast on the operation of the circuit here. [Movie-File]: https://www.dropbox.com/s/l9ga78pulkpua55/SleepyPico-Demo.MOV?dl=0

