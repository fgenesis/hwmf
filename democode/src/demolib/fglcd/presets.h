#pragma once

#include "chip_ili9341.h"
#include "chip_ili9481.h"
#include "chip_ili9486.h"
#include "chip_hx8357.h"

namespace fglcd {
namespace preset {

struct Dummy_LCD_config
{
    typedef DummyPin WrxPin;
    typedef DummyPin DCxPin;
    typedef DummyPin CSPin;
    typedef DummyPin ResetPin;
    typedef DummyPort CmdPort;
    typedef DummyPort DataPort;
    typedef Con_Port<CmdPort, WrxPin> CmdCon;
    typedef Con_Port<DataPort, WrxPin> DataCon;
    typedef Interface<CmdCon, DataCon, DCxPin, CSPin, ResetPin> Iface;
    typedef LcdBase<ILI9481, Iface> lcdtype;
};
typedef typename Dummy_LCD_config::lcdtype Dummy_LCD;


#if defined(PORTC) || defined(MCU_IS_PC) // FIXME: HACK
template<template<typename> class Chip>
struct Uno_8Bit_Shield_templ
{
    //typedef Pin<PortC, 0> RdPin;
    typedef Pin<PortC, 1> WrxPin;
    typedef Pin<PortC, 2> DCxPin;
    typedef Pin<PortC, 3> CSPin;
    typedef Pin<PortC, 4> ResetPin;
    // On an Uno, The first 2 bits come from PORTC, the rest is PORTD
    typedef SplitPort_Dirty<u8, PortB, 0x03, PortD, 0xFC> Port8Bit;
    // But the data port still uses 16 bit colors, so need to use the 8bit port twice
    typedef Port2x<u16, Port8Bit, Port8Bit> Port16Bit;
    typedef Con_Port<Port8Bit, WrxPin> CmdCon;
    typedef Con_Port<Port16Bit, WrxPin> DataCon;
    typedef Interface<CmdCon, DataCon, DCxPin, CSPin, ResetPin> Iface;
    typedef LcdBase<Chip, Iface> lcdtype;
};

typedef typename Uno_8Bit_Shield_templ<ILI9341>::lcdtype ILI9341_Uno_Shield;
typedef typename Uno_8Bit_Shield_templ<ILI9486>::lcdtype ILI9486_Uno_Shield;
#endif

#if defined(PORTG) || defined(MCU_IS_PC) // FIXME: HACK
template<template<typename> class Chip>
struct Mega_16Bit_Shield_templ
{
    typedef Pin<PortG, 2> WrxPin;
    typedef Pin<PortD, 7> DCxPin;
    typedef Pin<PortG, 1> CSPin;
    typedef Pin<PortG, 0> ResetPin;
    typedef PortC CmdPort;
    typedef Port2x<u16, PortC, PortA> DataPort;
    typedef Con_Port<CmdPort, WrxPin> CmdCon;
    typedef Con_Port<DataPort, WrxPin> DataCon;
    typedef Interface<CmdCon, DataCon, DCxPin, CSPin, ResetPin> Iface;
    typedef LcdBase<Chip, Iface> lcdtype;
};

typedef typename Mega_16Bit_Shield_templ<ILI9481>::lcdtype ILI9481_Mega_Shield;
typedef typename Mega_16Bit_Shield_templ<ILI9486>::lcdtype ILI9486_Mega_Shield;
typedef typename Mega_16Bit_Shield_templ<HX8357C>::lcdtype HX8357C_Mega_Shield;
#endif
/*
struct ST7783_SPI_LCD_Shield_2_4inch_UNO_config
{
    typedef Pin<PortG, 2> WrxPin;
    typedef Pin<PortD, 7> DCxPin;
    typedef Pin<PortG, 1> CSPin;
    typedef Pin<PortG, 0> ResetPin;
    typedef SpiPort<u8> CmdPort;
    typedef SpiPort<u16> DataPort;
    struct SPIInit
    {
        FORCEINLINE void init()
        {
            SPI::enable(SPI::MSB_FIRST);
        }
    };
    typedef Interface<WrxPin, DCxPin,CSPin, ResetPin, PortC, DataPort, SPIInit> Iface;
    typedef LcdBase<ST7783, Iface> lcdtype;

};
typedef typename ST7783_SPI_LCD_Shield_2_4inch_UNO_config::lcdtype T7783_SPI_LCD_Shield_2_4inch_UNO;
*/

/*
struct ILI9481_Mega_Shield_XMEM_config
{
    typedef DummyPin WrxPin;
    typedef Pin<PortD, 7> DCxPin;
    typedef Pin<PortG, 1> CSPin;
    typedef Pin<PortG, 0> ResetPin;
    typedef Port2x<u16, PortC, PortA> DataPort;
    typedef Interface<WrxPin, DCxPin,CSPin, ResetPin, PortC, DataPort, Connection_XMEM_8, Connection_XMEM_16> Iface;
    typedef LcdBase<ILI9481, Iface> lcdtype;
};
typedef typename ILI9481_Mega_Shield_XMEM_config::lcdtype ILI9481_Mega_Shield_XMEM;
*/

} // end namespace preset
} // end namespace fglcd
