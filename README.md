# SleepyPico
## What it is
Weather Station Example for using sleep and dormant mode of Raspberry Pi Pico and SSD1306, BME280.

The source code comprises:
- the Raspberry Pi Pico application SleepyPico that uses the Pico's sleep and dormant modes. 
- the library class Sleep that is responsible for the sleep functionality and provides an Arduino-like event loop.
- an OLED driver for the SSD1306 display (by Larry Bank).
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
    #define WAKEUP_PIN 15   // GPIO Pin to wait for high signal edges in DORMANT mode.
    
    void setup() {
      // called once in the beginning: put your initialization code here
    }
    
    void loop() {
      // called in each iteration of the event loop: put your code
      // here to read sensors, control actuators, etc.
    }
    
    int main() {
      stdio_init_all();
      sleep_ms(3000); // required by some OSs to make Pico visible in UARTs
      
      // Change frequency of Pico to a lower value (60 MHz)
      
      printf("Changing system clock to lower frequency: %d KHz\n", 60000);
      set_sys_clock_khz(60000, true);
    
    
      // Configure Sleep instance
      //     MODE (NORMAL, SLEEP, DORMANT)
      //     Pointers to your setup() and loop() functions
      //     For SLEEP mode: Start and end of alarm period for SLEEP
      //     For DORMANT mode: WAKEUP_PIN where high edges are detected
      
      Sleep::instance().configure(Sleep::MODE::DORMANT, &setup, &loop,
                                  start, end, WAKEUP_PIN);
                                  
      // Show clock frequencies:
      
      Sleep::instance().measure_freqs();
      
      // Start event loop.
      // This will call setup()
      // then put the Pico to sleep
      // and finally call your loop() function
      
      Sleep::instance().run(); 
      
      return 0;
     }


## Warning
The current official SDKs for the Raspberry Pi Pico seem to work incorrectly. For example, the Pico freezes after a some time when using sleep modes. UART output is not visible after sleeping. This might also be due to missing documentation of the sleep modes, e.g., what needs to be done to recover from sleep.

  
## How to build the app
To build this example application, execute "cmake ." and then "make" in the src directory.

## Circuit diagram
On the bottom left the BME280 sensor is depicted. The SSD1306 resides on the bottom right.
The push button connected to 3.3V and GPIO 15 is used in DORMANT mode to wake up the Pico.
The Pico is fed by a power supply (not shown) such as a battery (LiPo, LiIon or any other battery).
![Circuit diagram](https://github.com/ms1963/SleepyPico/blob/main/sleepypico_steckplatine.svg)

There is a small video cast on the operation of the circuit here. [Movie-File]: https://www.dropbox.com/s/l9ga78pulkpua55/SleepyPico-Demo.MOV?dl=0

