#pragma once

#include "demo_def.h"
#include "scratch.h"

namespace meminfo {

#ifdef __AVR__

inline constexpr ptrdiff_t RamSize() { return RAMEND - RAMSTART + 1; }
FORCEINLINE uint8_t *RamBegin() { return (uint8_t*)(uintptr_t)RAMSTART; }
FORCEINLINE ptrdiff_t RamAvail(uint8_t *p)
{
    return (RamBegin() + RamSize()) - p;
}
FORCEINLINE ptrdiff_t RamAdvance(uint8_t *& p, uint32_t bytes)
{
    p += (ptrdiff_t)bytes;
    return RamAvail(p);
}

inline constexpr uint32_t ROMSize() { return FLASHEND + 1; }
FORCEINLINE fglcd::FarPtr ROMBegin() { return (fglcd::FarPtr)0; }
FORCEINLINE uint32_t RomAvail(fglcd::FarPtr p)
{
    return (RomBegin() + ROMSize()) - p;
}
FORCEINLINE uint8_t ROMAdvance(fglcd::FarPtr& p, uint32_t bytes)
{
    p += bytes;
    return RomAvail(p);
}

#elif defined(MCU_IS_PC)

extern "C" int main(int argc, char *argv[]); // some readonly and large enough global symbol

inline constexpr unsigned RamSize() { return CFG_MAXRAM; }
FORCEINLINE uint8_t *RamBegin() { return reinterpret_cast<uint8_t*>(&G); }
FORCEINLINE ptrdiff_t RamAvail(uint8_t *p)
{
    return (RamBegin() + sizeof(AllGlobals)) - p;
}
FORCEINLINE ptrdiff_t RamAdvance(uint8_t *& p, uint32_t bytes)
{
    p += (ptrdiff_t)bytes;
    if(p >= RamBegin() + sizeof(AllGlobals))
        p = RamBegin();
    return RamAvail(p);
}

inline constexpr uint32_t ROMSize() { return 0x3FFFF; }
FORCEINLINE fglcd::FarPtr ROMBegin() { return (fglcd::FarPtr)&main; } // whatev
FORCEINLINE uint32_t ROMAvail(fglcd::FarPtr p)
{
    // no idea how large main() is but the entire page is readable so at least 4k on $OS
    return (ROMBegin() + 4096) - p;
}
FORCEINLINE uint32_t ROMAdvance(fglcd::FarPtr& p, uint32_t bytes)
{
    fglcd::FarPtr const end = ROMBegin() + 4096;
    p += bytes; // must wraparound internally otherwise we'll crash hard
    if(p >= end)
        p -= 4096;
    return ROMAvail(p);
}

#else
#error unknown platform
#endif

} // end namespace meminfo 
