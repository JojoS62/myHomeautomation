/**
 * LPC8xx Internal Analog Comparator library for mbed
 * Copyright (c) 2015 Suga
 * Released under the MIT License: http://mbed.org/license/mit
 */
/** @file
 * @brief LPC8xx Internal Analog Comparator library for mbed
 */

#ifndef _ACMP_H_
#define _ACMP_H_

#include "mbed.h"

#if !defined(TARGET_LPC81X) && !defined(TARGET_LPC82X)
#error "supported for LPC8xx"
#endif

/** ACMP class
 */
class ACMP {
public:
    enum VSEL {
        LADDER  = 0, // voltage ladder
        ACMP_I1 = 1, // P0.0
        ACMP_I2 = 2, // P0.1
#if defined(TARGET_LPC81X)
        BANDGAP = 6,
#elif defined(TARGET_LPC82X)
        ACMP_I3 = 3, // P0.14
        ACMP_I4 = 4, // P0.23
        BANDGAP = 5,
        ADC_0   = 6,
#endif
    };
    enum HYS {
        NONE    = 0,
        HYS5mV  = 1,
        HYS10mV = 2,
        HYS20mV = 3,
    };

    static ACMP *_acmp;

    /** 
     * @param ain1 Selects positive voltage input
     * @param ain2 Selects negative voltage input
     * @param hys Selects hysteresis of the comparator
     * @param lad Selects voltage ladder (0-31)
     */
    ACMP (VSEL ain1, VSEL ain2, HYS hys = NONE, int lad = -1);

    void set(VSEL ain1, VSEL ain2, HYS hys = NONE, int lad = -1);

    void isrAcmp ();

    int read ();

    void rise (Callback<void()> cb) {
    	_rise = cb;
    	if (_rise || _fall)
            NVIC_EnableIRQ(CMP_IRQn);
    	else
    		NVIC_DisableIRQ(CMP_IRQn);
    }

    void fall (Callback<void()> cb) {
    	_fall = cb;
    	if (_rise || _fall)
            NVIC_EnableIRQ(CMP_IRQn);
    	else
    		NVIC_DisableIRQ(CMP_IRQn);
    }


    void enableCompOut(PinName pinCompOut);
    void disableCompOut();

protected:
    Callback<void()> _rise;
    Callback<void()> _fall;
    gpio_t gpio;
};

#endif
