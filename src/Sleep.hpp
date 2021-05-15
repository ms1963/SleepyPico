/*
 Class Sleep provides basic support for operating
 a Raspberry Pi Pico in sleep or dormant mode.

 (c) 2021, by Michael Stal
 This library is published under GPL 3.0 license. 
*/

#pragma once 

#include "pico/stdlib.h"
#include "pico/sleep.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "hardware/clocks.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"
#include "hardware/pll.h"



class Sleep {
public:
    enum  MODE { NORMAL = 1, SLEEP = 2, DORMANT = 4 }; 
    // class implemented using the
    // Singleton design pattern.
    // Singleton makes sense here.
    static Sleep& instance() {
        static Sleep _instance;
        return _instance;
    }
    // No copy constructor or operator = 
    Sleep(Sleep const&)           = delete;
    void operator=(Sleep const&)  = delete;

    // used to (re-)configure 
    void configure(MODE mode, void (*setupfnc)(), void (*loopfnc)(), 
                   datetime_t startTime, datetime_t endTime, 
                   uint8_t WAKEUP_PIN);
    // get current mode
    MODE get_mode();
    void measure_freqs();
    // saves clock registers
    // Initialize RTC for sleep mode
    void before_sleep();
    // this function is responsible for sleep
    // sleep ends with high edge (DORMANT) or when t_alarm is reached (SLEEP)
    void start_sleep(); 
    // sleep recovery
    void after_sleep();

    // kind of run shell: calls _setup once, and 
    // implements infinite loop where sleep phases
    // are initiated and _loop is being called
    // in each iteration.
    // Actually, this function implements an
    // event loop
    void run();

private:
    uint _scb_orig;     // clock registers saved before MODE::DORMANT or MODE:SLEEP
    uint _en0_orig;     // ""
    uint _en1_orig;     // ""
    MODE _mode;         // can be SLEEP, DORMANT or NORMAL

    uint _wakeup_pin; // used to trigger wake-up in dormant mode MODE::DORMANT
    datetime_t _init_time;    // initial time set
    datetime_t _alarm_time;   // alarm time
    void (* _setup)();        // user-defined setup function - called once
    void (* _loop) ();        // user-defined loop function: called in each iteration
    Sleep(){};                // private constructor
};

