// Warning!
// Arduino usually ships a very old and buggy avr-gcc version!
// Before you try to compile this, make sure you're using avr-gcc 9.2 or better!
// Upgrade instructions + binaries here: https://blog.zakkemble.net/avr-gcc-builds/

#include <avr/wdt.h>
#include "demo.h"

// Stub out some libcxx shit that's never supposed to be called anyway
extern void __cxa_pure_virtual() { while(1); }
namespace __gnu_cxx { void __verbose_terminate_handler() { while(1); }}

int main()
{
    //wdt_disable();
    sei();
    demoinit();

    for(;;)
        demomain();
}

// Note to self: Enable printing floats properly:
// Find in Arduino/hardware/arduino/avr/platform.txt:
// -lm (should occur only once at the end of a line)
// replace with:
//  -Wl,-u,vfprintf -lprintf_flt -lm
// Makes all compiles that use printf() and friends much bigger but
// is the only sane way to printf() floats for debugging.
// This is NOT required for the demo to function!
