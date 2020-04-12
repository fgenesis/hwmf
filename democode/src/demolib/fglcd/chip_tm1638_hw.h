#pragma once

#include "mcu.h"
#include "trigger.h"
#include "lut_7seg.h"

struct __FlashStringHelper;

namespace fglcd {

template<typename StrobePin, typename ClockPin, typename DataPin_>
class TM1638
{
    //static_assert(has_pin_tag<StrobePin>::value, "StrobePin is not a pin");
    //static_assert(has_pin_tag<ClockPin>::value, "ClockPin is not a pin");
    //static_assert(has_pin_tag<DataPin_>::value, "DataPin is not a pin");
public:
    typedef DataPin_ DataPin;
    typedef LowTrigger<ClockPin> Clock; // was pulse
    typedef LowTrigger<StrobePin> CS;

    enum Instr : u8
    {
        INSTR_DATA = 0x40,
        INSTR_DISP_CTRL = 0x80,
        INSTR_ADDR = 0xc0
    };

    enum InstrData : u8
    {
        INSTR_DATA_WRITE = 0x00,
        INSTR_DATA_READKEYS = 0x02,

        INSTR_DATA_AUTOADD = 0x00,
        INSTR_DATA_FIXED = 0x04,

        INSTR_DATA_TESTMODE = 0x08,
    };
    
    static void init()
    {
        Clock::Pin::makeOutput();
        Clock::clear();
        CS::Pin::makeOutput();
        CS::clear();
        _writemode();
    }

    static void commit()
    {
        // everything goes directly to hardware, nothing to do
    }

    template<typename FROM = memtype::RAM>
    static void setText(const char *s, u8 beg = 0)
    {
        u8 c, i = 0, w = beg;
        for( ; w < 8 && (c = FROM::template read<u8>(&s[i])); ++w, ++i)
        {
            //setChar(i, c);
            c = seg7::transform(c);
            if(FROM::template read<u8>(&s[i+1]) == '.')
            {
                c |= 0x80;
                ++i;
            }
            setRaw(w, c);
        }
    }
    static void setText(const __FlashStringHelper *s, u8 beg = 0)
    {
        setText<memtype::Progmem>(reinterpret_cast<const char*>(s), beg);
    }

    static void update(const u8 *p, u8 n, u8 ledr, u8 ledg = 0, u8 beg = 0)
    {
        NoInterrupt no;
        _instr_data(INSTR_DATA_WRITE | INSTR_DATA_AUTOADD);
        CS::set();
        _send_addr(beg << 1);
        for(u8 i = 0; i < n; ++i, ledr >>= 1, ledg >>= 1)
        {
            _send(p[i]);
            _send((ledr & 1) | ((ledg & 1) << 1));
        }
        CS::clear();
    }

    static void intensity(u8 x) // 0: off, 1: low .. 8: bright
    {
        if(x)
        {
            --x;
            if(x > 7)
                x = 7;
            x |= 8; // display on bit
        }

        NoInterrupt no;
        _instr_disp(x);

        // necessary for the TM1640
        CS::set();
        Clock::trigger();
        CS::clear();
    }

    static void setLED(u8 pos, u8 color)
    {
        NoInterrupt no;
        _instr_data(INSTR_DATA_WRITE | INSTR_DATA_FIXED);
        CS::set();
        _send_addr((pos << 1) | 1);
        _send(color);
        CS::clear();
    }

    static void setLEDs(u8 r, u8 g = 0)
    {
        for(u8 i = 0; i < 8; ++i, r >>= 1, g >>= 1)
        {
            u8 col = (r & 1) | ((g & 1) << 1);
            setLED(i, col);
        }
    }

    static u8 getButtons()
    {
        u8 keys = 0;
        NoInterrupt no;
        CS::set();
        _send(INSTR_DATA | INSTR_DATA_READKEYS | INSTR_DATA_AUTOADD);
        _readmode();
        for (u8 i = 0; i < 4; i++)
        {
            delay_us(1);
            keys |= _read() << i;
        }
        CS::clear();
        _writemode();

        return keys;
    }

    static void clear()
    {
        NoInterrupt no;
        _instr_data(INSTR_DATA_WRITE | INSTR_DATA_AUTOADD);
        CS::set();
        _send_addr(0);
        for(u8 i = 0; i < 16; ++i)
            _send(0);
        CS::clear();
    }

    static void setRaw(u8 pos, u8 data)
    {
        _senddata(pos << 1, data);
    }
    static void setChar(u8 pos, u8 data)
    {
        setRaw(pos, seg7::transform(data));
    }

private:

    static FORCEINLINE void _instr_data(u8 d)
    {
        _sendcmd(INSTR_DATA | d);
    }
    static FORCEINLINE void _send_addr(u8 addr)
    {
        _send(INSTR_ADDR | addr);
    }
    static FORCEINLINE void _instr_disp(u8 d)
    {
        _sendcmd(INSTR_DISP_CTRL | d);
    }
    
    static void _send(u8 x)
    {
        for (u8 i = 0; i < 8; i++, x >>= 1)
        {
            DataPin::set(x & 1);
            Clock::trigger();
        }
    }
    static FORCEINLINE void _sendcmd(u8 cmd)
    {
        CS::set();
        _send(cmd);
        CS::clear();
    }
    static FORCEINLINE void _senddata(u8 addr, u8 data)
    {
        _instr_data(INSTR_DATA_WRITE | INSTR_DATA_FIXED);
        CS::set();
        _send_addr(addr);
        _send(data);
        CS::clear();
    }

    static FORCEINLINE void _writemode()
    {
        DataPin::makeOutput();
        DataPin::lo();
    }
    static FORCEINLINE void _readmode()
    {
        DataPin::makeInput();
        DataPin::hi(); // enable pullup
    }

    static FORCEINLINE u8 _read()
    {
        u8 x = 0;
        for(u8 i = 0; i < 8; ++i)
        {
            Clock::trigger();
            x >>= 1;
            if(DataPin::get())
                x |= 0x80;
        }
        return x;
    }
};


} // end namespace fglcd
