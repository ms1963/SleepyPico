/*
 Class Sleep provides basic support for operating
 a Raspberry Pi Pico in sleep or dormant mode.

 (c) 2021, by Michael Stal
 This library is published under GPL 3.0 license. 
*/

#include "Sleep.hpp"

//
// configure
// mode:      either DORMANT, SLEEP, NORMAL
// loopfnc:   pointer to function being called after each
//            deep sleep period
// setupfnc:  pointer to function that is called once
//            for setting up the environment needed
// startTime: initial datetime for tze RTC
// endTime:   time when alarm should be fired in
//            SLEEP mode
// WAKEUP_PIN: Pin to use for detecting high edges
//             in DORMANT mode
//
void Sleep::configure(MODE mode, void (*setupfnc)(), void (*loopfnc)(), 
                      datetime_t startTime, datetime_t endTime, uint8_t WAKEUP_PIN) {
    _mode       = mode;
    _loop       = loopfnc;
    _setup      = setupfnc;
    _init_time  = startTime;
    _alarm_time = endTime;
    _wakeup_pin = WAKEUP_PIN;
}


// retrieve the currentz mode of operation (i.e., SLEEP, NORMAL, DORMANT)
Sleep::MODE Sleep::get_mode() {
    return _mode;
}


// helper function to display frequencies of Pico system clocks
void Sleep::measure_freqs(void) {
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
void Sleep::before_sleep() {
    // save current values for clocks
    _scb_orig = scb_hw->scr;
    _en0_orig = clocks_hw->sleep_en0;
    _en1_orig = clocks_hw->sleep_en1;
}

static void onWakeUp() {
    // actions for wake up event
}

// this function is responsible for sleep
// sleep ends with high edge (DORMANT mode) 
// or when _alarm_time is reached (SLEEP mode)
void Sleep::start_sleep() {
    // Crystal oscillator drives RTC during sleep
    sleep_run_from_xosc();
    // Reset real time clock to a value
    // see the defines for MINUTES_TO_WAIT, SECONDS_TO_WAIT
    if (_mode == MODE::SLEEP) { // sleep until RTC triggers alarm
        rtc_init();
        rtc_set_datetime(&_init_time);
        sleep_goto_sleep_until(&_alarm_time, &onWakeUp);
    } 
    else 
    if (_mode == MODE::DORMANT) { 
        //Go to sleep until we see a high edge on _wakeup_pin
        sleep_goto_dormant_until_edge_high(_wakeup_pin);
    }
}

// sleep recovery
void Sleep::after_sleep() {
    // Re-enable Ring Oscillator control
    rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);

    // restore clock registers
    scb_hw->scr             = _scb_orig;
    clocks_hw->sleep_en0    = _en0_orig;
    clocks_hw->sleep_en1    = _en1_orig;
    _loop();
}


// Implementation of event loop
// 1. _setup() is being executed
// 2. the sleep functionality is executed
//      A) begin_sleep
//      B) start_sleep
//      C) after_sleep
//    Within after_sleep _loop() is called
void Sleep::run() {
    _setup(); // called once
    while(true) {
        if (_mode != MODE::NORMAL) {
            before_sleep();
            start_sleep(); 
            after_sleep(); // here _loop gets called in each iteration
        }
        else {
            _loop(); // NORMAL mode =>
                     // must explicitly call _loop
        }
    }
}