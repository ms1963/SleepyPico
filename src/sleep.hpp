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
    // modes of operation:
    enum  MODE { NORMAL = 1, SLEEP = 2, DORMANT = 4 }; 
 
    // class implemented using the
    // Singleton design pattern, 
    // since only one instance is 
    // required resp. useful
    static Sleep& instance() {
        static Sleep _instance;
        return _instance;
    }

    // No copy constructor or assignment operator 
    Sleep(Sleep const&)           = delete;
    void operator=(Sleep const&)  = delete;

    // used to (re-)configure 
    void configureSleep(void (*setupfnc)(), void (*loopfnc)(),  datetime_t startTime, datetime_t endTime);
    void configureDormant(void (*setupfnc)(), void (*loopfnc)(), uint WAKEUP_PIN, bool edge, bool active);
    void configureNormal(void (*setupfnc)(), void (*loopfnc)());

    // get current mode
    MODE get_mode();

    void measure_freqs();
    // kind of run shell: calls _setup once, and 
    // implements infinite loop where sleep phases
    // are initiated and _loop is being called
    // in each iteration.
    // Actually, this function implements an
    // event loop
    void run();

private:
    // saves clock registers
    // Initialize RTC for sleep mode
    void before_sleep();

    // this function is responsible for sleep
    // sleep ends with high edge (DORMANT) or when t_alarm is reached (SLEEP)
    void start_sleep(); 

    // sleep recovery
    void after_sleep();

    // private constructor
    Sleep(){};   

private:
// Data members:
    // registers to be restored after sleep
    uint _scb_orig;           // clock registers saved before sleeping
    uint _en0_orig;           // ""
    uint _en1_orig;           // ""

    // maintains current mode of operation
    MODE _mode;               // can be SLEEP, DORMANT or NORMAL

    // data members for dormant mode
    uint _wakeup_pin;         // used to trigger wake-up in dormant mode MODE::DORMANT
    bool _edge;               // Interrupt triggered on leading edge (true) or trailing edge (false)
    bool _active;             // GPIO Pin on active HIGH (true) or active LOW (false)

    // data members for sleep mode
    datetime_t _init_time;    // initial time set
    datetime_t _alarm_time;   // alarm time

    // pointers to user-defined setup() and loop() functions
    void (* _setup)();        // user-defined setup function - called once
    void (* _loop) ();        // user-defined loop function: called in each iteration
};


