/*
 SleepyPico uses sleep or dormant mode to put the Raspberry Pi Pico
 into sleep for low power consumption.
 The system frequency is reduced additionally.
 In the circuit a BME280 is used for measuring temperature, pressure,
 humidity. The BME280 is accessed using forced mode to let the sensor
 sleep between measurements.
 Information is displayed via a SSD1306 OLED display. Other similar 
 displays might be used instead. The OLED is also put into sleep mode
 between sleep phases (power off).
 
 You may also set the mode (SLEEP, DORMANT, NORMAL), the system frequency,
 the sleep time using the definitions in the beginning of this file.
 Unfortunately, the sleep and dormant modes do not operate correctly so 
 that the Pico gets stuck from time to time. So far, I could not
 find any hints indicating the reason for this problem.

 (c) 2021, by Michael Stal
 This example is published under GPL 3.0 license. 
*/


#include "Sleep.hpp"
#include "bme280_spi.hpp"
#include "ss_oled.hpp"




// MODE::DORMANT: Select the GPIO-Pin on which 
// a high edge wakes the Pico up. In
// addition, set whether waiting for leading
// or trailing edge, and whether the Pin is
// active low or high.
//
// MODE::SLEEP: Set the time after which the Pico 
// should wake up using the RTC - valid for  
// Change these values according to your needs.
// Maximum wait time is 59 mminutes and 59 seconds.
// If you need longer sleep time, change the two
// datetime_t structs _start_time & and _init_time below
//
// MODE::NORMAL



#define MINUTES_TO_WAIT         0       // MODE::SLEEP only: sleeping for <MINUTES_TO_WAIT> minutes
#define SECONDS_TO_WAIT         20      //                   and <SECONDS_TO_WAIT> seconds
#define SYSTEM_FREQUENCY_KHZ    60000   // target frequency of Pico
#define DISPLAY_TIME            10000   // time in milliseconds to show the measurement


// OLED SSD1306 (I2C) and RPI Pico
#define SDA_PIN         4
#define SCL_PIN         5
#define PICO_I2C        i2c0
#define I2C_SPEED       100 * 1000
#define OLED_WIDTH      128
#define OLED_HEIGHT     64

int oled_rc; // stores return codes of OLED driver

// BUILT-IN LED
const uint LED_PIN      = 25; // used to signal measurement
// WAKE-UP PIN 
const uint WAKEUP_PIN   = 15; // used to trigger wake-up in dormant mode MODE::DORMANT


// !!!!!!!!! The following datetime_t structures
// are only required for SLEEP mode

// start time to which RTC is initially set
datetime_t start = {   // time to which RTC is set
    .year  = 2021,
    .month = 05,
    .day   = 01,
    .dotw  = 6, // 0 is Sunday, so 5 is Friday
    .hour  = 00,
    .min   = 00,
    .sec   = 00
 };

// time when the RTC shall wake us up
 datetime_t end = {   // time of alarm
    .year  = 2021,
    .month = 05,
    .day   = 01,
    .dotw  = 6, // 0 is Sunday, so 5 is Friday
    .hour  = 00,
    .min   = MINUTES_TO_WAIT,
    .sec   = SECONDS_TO_WAIT
 };
 //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

 uint8_t ucBuffer[1024]; // buffer used for OLED

/*
* prints measurements to OLED display
*/
void draw_on_oled(picoSSOLED myOled, BME280::Measurement_t values) {  
    myOled.fill(0,1);
    myOled.power(true); // display on
    char tem[30]; // buffer for displaying temperature on oled
    char hum[30]; // buffer for displaying humidity    on oled
    char prs[30]; // buffer for displaying pressure    on oled
    char alt[30]; // buffer for displaying altitude    on oled

    // create formatted strings with measurement results
    sprintf(tem, "tem: %6.1f C",   values.temperature);
    sprintf(hum, "hum: %6.1f %c",  values.humidity, '%');
    sprintf(prs, "prs: %6.1f hPa", values.pressure);
    sprintf(alt, "alt: %6.1f m",   values.altitude);

    if (oled_rc != OLED_NOT_FOUND)
    { 
        myOled.set_contrast(127);
        myOled.write_string(0,0,1,(char *)" Weather Today ", FONT_8x8, 0, 1);
        myOled.write_string(0,0,3,tem, FONT_8x8, 0, 1); // write temperature
        myOled.write_string(0,0,4,hum, FONT_8x8, 0, 1); // write humidity
        myOled.write_string(0,0,5,prs, FONT_8x8, 0, 1); // write pressure
        myOled.write_string(0,0,6,alt, FONT_8x8, 0, 1); // write altitude

        gpio_put(LED_PIN, 0); // Turn off LED
         
        sleep_ms(DISPLAY_TIME);    // wait so that user can read the display
        myOled.power(false);       // display off
    }
}

/*
* prints welcome screen to OLED display
*/
void welcome(picoSSOLED myOled) {  
    myOled.write_string(0,0,1,(char *)" Weather Today ", FONT_8x8, 0, 1);
    switch(Sleep::instance().get_mode()) {
        case Sleep::MODE::SLEEP:
            myOled.write_string(0,0,3, (char*)" SLEEP mode", FONT_8x8, 0, 1); 
            break;
        case Sleep::MODE::DORMANT:
            myOled.write_string(0,0,3, (char*)" DORMANT mode", FONT_8x8, 0, 1); 
            break;       
        default:
            myOled.write_string(0,0,3, (char*)" NORMAL MODE", FONT_8x8, 0, 1); 
    }
    sleep_ms(3000);
    myOled.power(false);
}

// initializing the BME280
BME280 myBME280(0, 
            PICO_DEFAULT_SPI_RX_PIN, 
            PICO_DEFAULT_SPI_TX_PIN, 
            PICO_DEFAULT_SPI_SCK_PIN, 
            PICO_DEFAULT_SPI_CSN_PIN, 
            500 * 1000,
            BME280::MODE::MODE_FORCED); // using BME280 in forced mode to reduce energy consumption

// initializing the OLED display
picoSSOLED myOled(OLED_128x64, 0x3c, 0, 0, PICO_I2C, SDA_PIN, SCL_PIN, I2C_SPEED);

BME280::Measurement_t result;

// runs once
void setup() {
    gpio_init(LED_PIN); // Use built-in LED to signal wake time
    gpio_set_dir(LED_PIN, GPIO_OUT); // it is an output pin

    // ss1306-OLED is initialized
    oled_rc = myOled.init();
    myOled.set_back_buffer(ucBuffer);
    myOled.fill(0,1);

    // Welcome screen
    welcome(myOled);

    // empty read as a warm-up
    myBME280.measure();
    sleep_ms(100);
}

// runs in each iteration
void loop() { 
    // get measurement from BME280
    // start of measurement => LED HIGH
    gpio_put(LED_PIN, 1);
    // start actual measurement
    result = myBME280.measure();
    // end of measurement => LED LOW
    gpio_put(LED_PIN, 0);
    // write to OLED
    draw_on_oled(myOled, result);
}

int main() {
    // execution might hang after a few sleep cycles
    // when uncommenting the following line:
    // stdio_init_all(); 

    sleep_ms(3000); // required by some OSses to make Pico visible
        
    // Change frequency of Pico to a lower value
    printf("Changing system clock to lower frequency: %d KHz\n", SYSTEM_FREQUENCY_KHZ);
    set_sys_clock_khz(SYSTEM_FREQUENCY_KHZ, true);
    
    
    // configure Sleep instance with Dormant mode
    // Parameters
    // - loop() and setup() functions are passed as lambdas
    // - WAKEUP_PIN where GPIO signals are detected
    // - in this case (true, true) means =>  
    //   detect leading edge with WAKEUP_PIN  being active high
    Sleep::instance().configure(setup, loop, WAKEUP_PIN, true, true);

/*  // using Sleep mode instead
    Sleep::instance().configure(setup, loop, start, end);
*/

/*  // using Normal mode instead
    Sleep::instance().configure(setup, loop);
*/

    // show clock frequencies
    Sleep::instance().measure_freqs();

    // start event loop
    Sleep::instance().run();

    return 0;
}
