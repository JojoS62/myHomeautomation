#if defined(TARGET_LPC812) || defined(TARGET_LPC824)

#include "WakeUp.h"

Callback<void()> WakeUp::_rise;
float WakeUp::cycles_per_ms = 10.0f;

void WakeUp::set_ms(uint32_t  ms)
{
    //Enable clock to register interface:
    LPC_SYSCON->SYSAHBCLKCTRL |= 1<<9;

    //Clear the counter:
    LPC_WKT->CTRL |= 1<<2;
    if (ms != 0) {
        //Enable clock to register interface:
        LPC_SYSCON->SYSAHBCLKCTRL |= 1<<9;

        //Set 10kHz timer as source, and just to be sure clear status bit
        LPC_WKT->CTRL = 7; //3

        //Enable the 10kHz timer
        LPC_PMU->DPDCTRL |= (1<<2) | (1<<3);

        //Set interrupts
        //NVIC_SetVector(WKT_IRQn, (uint32_t)WakeUp::irq_handler);
        //NVIC_EnableIRQ(WKT_IRQn);

        //Load the timer
        LPC_WKT->COUNT = (uint32_t)((float)ms * cycles_per_ms);
        //LPC_WKT->COUNT = ms * cycles_per_ms;

    } else {
        //Disable clock to register interface:
        LPC_SYSCON->SYSAHBCLKCTRL &= ~(1<<9);

        //Disable the 10kHz timer
        LPC_PMU->DPDCTRL &= ~((1<<2) | (1<<3));
    }
}

void WakeUp::irq_handler(void)
{
    //Clear status
    LPC_WKT->CTRL |= 2;

    //Disable clock to register interface:
    LPC_SYSCON->SYSAHBCLKCTRL &= ~(1<<9);

    //Disable the 10kHz timer
    LPC_PMU->DPDCTRL &= ~((1<<2) | (1<<3));

    if (_rise)
    	_rise.call();
}

void WakeUp::calibrate(void)
{
    cycles_per_ms = 10.0f;
    set_ms(1100);
    wait_ms(100);

    uint32_t prevread = LPC_WKT->COUNT;
    uint32_t read = LPC_WKT->COUNT;
    while( read != prevread) {
        prevread = read;
        read = LPC_WKT->COUNT;
    }

    float ticks = 11000.0f - read;

    cycles_per_ms = ticks / 100.0f;
    set_ms(0);
}

void WakeUp::deepPowerDown(void) {
    // disable brownout detection and WDT
    LPC_SYSCON->PDSLEEPCFG = 0xffff;

    //After wakeup same stuff as currently enabled:
    volatile uint32_t testReg = LPC_SYSCON->PDRUNCFG;
    LPC_SYSCON->PDAWAKECFG = testReg | (1U << 7);

    //Deep power down in PCON
    LPC_PMU->PCON &= ~0x03;
    LPC_PMU->PCON |= 0x03;

    //Deep sleep for ARM core:
    SCB->SCR = (1U << 2);

    __WFI();
}

bool WakeUp::wokeUpFromDeepSleep() {
    bool dpdFlag = (LPC_PMU->PCON & (1U << 11));

    return dpdFlag; // check DPDFLAG
}

void WakeUp::clearDeepPowerDownFlag() {
    LPC_PMU->PCON |= (1UL << 11);
}

#endif
