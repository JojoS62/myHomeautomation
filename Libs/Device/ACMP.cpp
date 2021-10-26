/**
 * LPC8xx Internal Analog Comparator library for mbed
 * Copyright (c) 2015 Suga
 * Released under the MIT License: http://mbed.org/license/mit
 */
/** @file
 * @brief LPC8xx Internal Analog Comparator library for mbed
 */

#include "ACMP.h"

ACMP *ACMP::_acmp;

ACMP::ACMP (VSEL vp, VSEL vn, HYS hys, int lad) {

    _acmp = this;

    LPC_SYSCON->PDRUNCFG      &= ~(1UL<<3);   	// Enable power to BOD block
    LPC_SYSCON->PDRUNCFG      &= ~(1UL<<15);   	// Enable power to CMP block
    LPC_SYSCON->SYSAHBCLKCTRL |=  (1UL<<19);   	// Enable clock to CMP block
    LPC_SYSCON->PRESETCTRL    &= ~(1UL<<12);   	// reset CMP
    LPC_SYSCON->PRESETCTRL    |=  (1UL<<12);

    set(vp, vn, hys, lad);

    LPC_CMP->CTRL |= (1UL<<20); 	// EDGECLR
    LPC_CMP->CTRL &= ~(1UL<<20); 	// EDGECLR
    // IRQ will be enabled when rise or fall callback is set
}

extern "C"
void CMP_IRQHandler (void) {
    ACMP::_acmp->isrAcmp();
}

void ACMP::isrAcmp () {
    if (read()) {
        if (_rise)
        	_rise.call();
    } else {
    	if (_fall)
    		_fall.call();
    }

    LPC_CMP->CTRL |= (1UL<<20); 	// EDGECLR
    LPC_CMP->CTRL &= ~(1UL<<20); 	// EDGECLR
}

int ACMP::read () {
    return LPC_CMP->CTRL & (1UL<<21) ? 1 : 0;
}

void ACMP::enableCompOut(PinName pinCompOut) {
    gpio_init_out(&gpio, pinCompOut);
#if defined(TARGET_LPC82X)
    LPC_SWM->PINASSIGN11 |= 0xff00;
    LPC_SWM->PINASSIGN11 &= (pinCompOut & 0xff00);
#elif defined(TARGET_LPC81X)
    LPC_SWM->PINASSIGN8 |= 0xff00;
    LPC_SWM->PINASSIGN8 &= (pinCompOut & 0xff00);
#endif
}

void ACMP::disableCompOut() {
#if defined(TARGET_LPC82X)
    LPC_SWM->PINASSIGN11 |= 0xff00;
#elif defined(TARGET_LPC81X)
    LPC_SWM->PINASSIGN8 |= 0xff00;
#endif
 }

void ACMP::set(VSEL vp, VSEL vn, HYS hys, int lad) {

    if (vp == ACMP_I1 || vn == ACMP_I1) {
        LPC_SYSCON->SYSAHBCLKCTRL |= (1<<7)|(1<<6); // enable clock for SWM, GPIO
        LPC_IOCON->PIO0_0   &= ~(3<<3); // no pull up/down
        LPC_GPIO_PORT->DIR0 &= ~(1<<0); // configure GPIO as input
        LPC_SWM->PINENABLE0 &= ~(1<<0); // P0.0 is ACMP_I1

    }
    if (vp == ACMP_I2 || vn == ACMP_I2) {
        LPC_SYSCON->SYSAHBCLKCTRL |= (1<<7)|(1<<6); // enable clock for SWM, GPIO
        LPC_IOCON->PIO0_1   &= ~(3<<3); // no pull up/down
        LPC_GPIO_PORT->DIR0 &= ~(1<<1); // configure GPIO as input
        LPC_SWM->PINENABLE0 &= ~(1<<1); // P0.1 is ACMP_I2
    }
#if defined(TARGET_LPC82X)
    if (vp == ACMP_I3 || vn == ACMP_I3) {
        LPC_SYSCON->SYSAHBCLKCTRL |= (1<<7)|(1<<6); // enable clock for SWM, GPIO
        LPC_IOCON->PIO0_14  &= ~(3<<3); // no pull up/down
        LPC_GPIO_PORT->DIR0 &= ~(1<<14); // configure GPIO as input
        LPC_SWM->PINENABLE0 &= ~(1<<2); // P0.14 is ACMP_I3
    }
    if (vp == ACMP_I4 || vn == ACMP_I4) {
        LPC_SYSCON->SYSAHBCLKCTRL |= (1<<7)|(1<<6); // enable clock for SWM, GPIO
        LPC_IOCON->PIO0_23  &= ~(3<<3); // no pull up/down
        LPC_GPIO_PORT->DIR0 &= ~(1UL<<23); // configure GPIO as input
        LPC_SWM->PINENABLE0 &= ~(1<<3); // P0.23 is ACMP_I4
    }
#endif
    if (lad >= 0) {
        LPC_CMP->LAD = (0<<6) | ((lad & 0x1fUL) << 1) | (1<<0); // Vref=VDD
    }

    LPC_CMP->CTRL = (hys << 25) | (vn << 11)| (vp << 8) | (2 << 3); // Both edges
}

