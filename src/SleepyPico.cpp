/*
 SleepyPico uses sleep and dormant mode to put the Raspberry Pi Pico
 into sleep for low power consumption.
 The system frequency is reduced additionally.
 In the circuit a BME280 is used for measuring temperature, pressure,
 humidity. The BME280 is accessed using forced mode to let the sensor
 sleep between measurements.
 Information is displayed via a SSD1306 OLED display. Other similar 
 displays might be used instead. The OLED is also put into sleep mode
 between sleep phases.
 To change the GPIO Pins respectively hardware components for I2C (SSD1306)
 or SPI (BME280) change the definitions in the code.
 You may also set the mode (SLLEP, DORMANT, NORMAL), the system frequency,
 the sleep time using the definitions in the beginning of this file.
 Unfortunately, the sleep and dormant modes do not operate correctly so 
 that the Pico gets stuck at some point of time. So far, I could not
 find any hints what is the reason for this problem.

 (c) 2021, by Michael Stal
 This example is published under the GNU GPLv3.0 license. 
*/


#include "sleep.h"
#include "pico/stdlib.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "hardware/clocks.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"
#include "hardware/pll.h"
#include "bme280_spi.hpp"
#include "ss_oled.hpp"


// Set the time after which the Pico should wakes up.
// using the RTC - valid for MODE::SLEEP 
//
// Or select the GPIO-Pin on which a high edge 
// wakes the Pico up - MODE::DORMANT
//
// Change these values according to your needs.
// Maximum wait time is 59 mminutes and 59 seconds.
// If you need longer sleep time, change the two
// datetime_t structs t and alarm_t below

enum  MODE { SLEEP = 0, DORMANT = 1, NORMAL = 2}; 
const MODE mode = MODE::DORMANT;

#define MINUTES_TO_WAIT         1       // MODE::SLEEP only: sleeping for <MINUTES_TO_WAIT> minutes
#define SECONDS_TO_WAIT         0       //                   and <SECONDS_TO_WAIT> seconds
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
const uint WAKEUP_PIN   = 15; // used to trigger wake-up in dormant mode MODE::DORMANT

// SLEEP MODE
uint scb_orig;     // clock registers saved before MODE::DORMANT or MODE:SLEEP
uint clock0_orig;  // ""
uint clock1_orig;  // ""

datetime_t t = {   // time to which RTC is set
    .year  = 2021,
    .month = 05,
    .day   = 01,
    .dotw  = 6, // 0 is Sunday, so 5 is Friday
    .hour  = 00,
    .min   = 00,
    .sec   = 00
 };

void measure_freqs(void) {
    uint f_pll_sys   = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
    uint f_pll_usb   = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
    uint f_rosc      = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
    uint f_clk_sys   = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint f_clk_peri  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    uint f_clk_usb   = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
    uint f_clk_adc   = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
    uint f_clk_rtc   = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);

    printf("pll_sys  = %dkHz\n", f_pll_sys);
    printf("pll_usb  = %dkHz\n", f_pll_usb);
    printf("rosc     = %dkHz\n", f_rosc);
    printf("clk_sys  = %dkHz\n", f_clk_sys);
    printf("clk_peri = %dkHz\n", f_clk_peri);
    printf("clk_usb  = %dkHz\n", f_clk_usb);
    printf("clk_adc  = %dkHz\n", f_clk_adc);
    printf("clk_rtc  = %dkHz\n", f_clk_rtc);
   
    uart_default_tx_wait_blocking();
}

// saves clock registers
// Initialize RTC for sleep mode
void before_sleep() {
    // save current values for clocks
    scb_orig = scb_hw->scr;
    clock0_orig = clocks_hw->sleep_en0;
    clock1_orig = clocks_hw->sleep_en1;

    // Initialize real-time-clock
    rtc_init();
    rtc_set_datetime(&t);
    measure_freqs();
}

// this method is called in MODE::SLEEP
// when RTC triggers wake-up-event
static void onWakeUp(void) {
  // put wake up actions here
}

// this function is responsible for sleep
// sleep ends with high edge (DORMANT) or when t_alarm is reached (SLEEP)
static void start_sleep() {
    // Crystal oscillator drives RTC during sleep
    sleep_run_from_xosc();
    // Reset real time clock to a value
    // see the defines for MINUTES_TO_WAIT, SECONDS_TO_WAIT
    if (mode == MODE::SLEEP) { // sleep until RTC triggers alarm
        datetime_t RTC_alarm = {
            .year  = 2021,
            .month = 05,
            .day   = 01,
            .dotw  = 6, // 0 is Sunday, so 5 is Friday
            .hour  = 00,
            .min   = MINUTES_TO_WAIT,
            .sec   = SECONDS_TO_WAIT
        };
        sleep_goto_sleep_until(&RTC_alarm, &onWakeUp);
    } 
    else 
    if (mode == MODE::DORMANT) { 
        //Go to sleep until we see a high edge on WAKEUP_PIN
        sleep_goto_dormant_until_edge_high(WAKEUP_PIN);
    }
    
    // LED turned on to signal wake-up
    gpio_put(LED_PIN, 1);
}

// sleep recovery
void after_sleep(uint scb_orig, uint clock0_orig, uint clock1_orig) {
    // Re-enable Ring Oscillator control
    rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);

    // restore clock registers
    scb_hw->scr = scb_orig;
    clocks_hw->sleep_en0 = clock0_orig;
    clocks_hw->sleep_en1 = clock1_orig;
}

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
        myOled.power(false); // display off
    }
}

/*
* prints welcome screen to OLED display
*/
void welcome(picoSSOLED myOled) {  
    myOled.write_string(0,0,1,(char *)" Weather Today ", FONT_8x8, 0, 1);
    switch(mode) {
        case MODE::SLEEP:
            myOled.write_string(0,0,3, (char*)" SLEEP mode", FONT_8x8, 0, 1); 
            break;
        case MODE::DORMANT:
            myOled.write_string(0,0,3, (char*)" DORMANT mode", FONT_8x8, 0, 1); 
            break;       
        default:
            myOled.write_string(0,0,3, (char*)" NORMAL MODE", FONT_8x8, 0, 1); 
    }
    sleep_ms(3000);
    myOled.power(false);
}

int main() {
    stdio_init_all();
    sleep_ms(3000); // required by some OSses to make Pico visible

    gpio_init(LED_PIN); // Use built-in LED to signal wake time
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Using BME280 with default GPIO - Ports and 500 KHz frequency
    BME280 myBME280(0, 
            PICO_DEFAULT_SPI_RX_PIN, 
            PICO_DEFAULT_SPI_TX_PIN, 
            PICO_DEFAULT_SPI_SCK_PIN, 
            PICO_DEFAULT_SPI_CSN_PIN, 
            500 * 1000,
            BME280::MODE::MODE_FORCED); // using BME280 in forced mode to reduce energy consumption

    BME280::Measurement_t result; // variable storing BME280 measurement

    // ss16xx OLED is initialized
    uint8_t ucBuffer[1024]; // buffer used for OLED
    picoSSOLED myOled(OLED_128x64, 0x3c, 0, 0, PICO_I2C, SDA_PIN, SCL_PIN, I2C_SPEED);
    oled_rc = myOled.init();
    myOled.set_back_buffer(ucBuffer);
    myOled.fill(0,1);

    // Welcome screen
    welcome(myOled);

    // empty read as a warm-up
    myBME280.measure();
    sleep_ms(100);
    
    printf("Starting\n");
    // Change frequency of Pico to a lower value
    printf("Changing system clock to lower frequency: %d KHz\n", SYSTEM_FREQUENCY_KHZ);
    set_sys_clock_khz(SYSTEM_FREQUENCY_KHZ, true);
    uart_default_tx_wait_blocking();
    
    while (true) {
        if (mode != MODE::NORMAL) {
            before_sleep();
            start_sleep();
            // reset processor and clocks back to defaults
            after_sleep(scb_orig, clock0_orig, clock1_orig);
            // obtaining clock frequencies
            measure_freqs();
        }

        // get measurement from BME280
        result = myBME280.measure();
        draw_on_oled(myOled, result);
    }
    return 0;
}
