#pragma once

#include "chip.h"
#include "trigger.h"

namespace fglcd {

struct MIPI_DCS_Specs
{
    // by default, wait a bit after a reset signal until things have stabilized
    template<typename PIN> struct ResetTriggerType : public TriggerWaitMS<TriggerHoldMS<LowTrigger<PIN>, 10>, 10> {};
    template<typename PIN> struct WrxCmdTriggerType : public ToggleTrigger<PIN> {};
    template<typename PIN> struct WrxDataTriggerType : public ToggleTrigger<PIN> {};
    template<typename PIN> struct DCxTriggerType : public LowTrigger<PIN> {}; // ToggleTrigger would be faster but might not work, depending on hardware state
    template<typename PIN> struct CSTriggerType : public LowTrigger<PIN> {};

    // from the MIPI SPEC
    enum Opcode : u8
    {
        enter_idle_mode       = 0x39,
        enter_invert_mode     = 0x21,
        enter_normal_mode     = 0x13,
        enter_partial_mode    = 0x12,
        enter_sleep_mode      = 0x10,
        exit_idle_mode        = 0x38,
        exit_invert_mode      = 0x20,
        exit_sleep_mode       = 0x11,
        get_address_mode      = 0x0B,
        get_blue_channel      = 0x08,
        get_diagnostic_result = 0x0F,
        get_display_mode      = 0x0D,
        get_green_channel     = 0x07,
        get_pixel_format      = 0x0C,
        get_power_mode        = 0x0A,
        get_red_channel       = 0x06,
        get_scanline          = 0x45,
        get_signal_mode       = 0x0E,
        nop                   = 0x00,
        read_DDB_continue     = 0xA8,
        read_DDB_start        = 0xA1,
        read_memory_continue  = 0x3E,
        read_memory_start     = 0x2E,
        set_address_mode      = 0x36,
        set_column_address    = 0x2A,
        set_display_off       = 0x28,
        set_display_on        = 0x29,
        set_gamma_curve       = 0x26, 
        set_page_address      = 0x2B, 
        set_partial_columns   = 0x31, 
        set_partial_rows      = 0x30, 
        set_pixel_format      = 0x3A, 
        set_scroll_area       = 0x33, 
        set_scroll_start      = 0x37, 
        set_tear_off          = 0x34, 
        set_tear_on           = 0x35, 
        set_tear_scanline     = 0x44, 
        soft_reset            = 0x01, 
        write_LUT             = 0x2D, 
        write_memory_continue = 0x3C, 
        write_memory_start    = 0x2C, 
    };

    enum AddressMode : u8 // bits 5,6,7
    {
        // normal mode
        ADDRMODE_LR_TB = 0 << 5,
        ADDRMODE_TB_LR = 1 << 5,
        ADDRMODE_RL_TB = 2 << 5,
        ADDRMODE_TB_RL = 3 << 5,
        ADDRMODE_LR_BT = 4 << 5,
        ADDRMODE_BT_LR = 5 << 5,
        ADDRMODE_RL_BT = 6 << 5,
        ADDRMODE_BT_RL = 7 << 5,

        ADDRMODE_LCD_REFRESH_TOP_TO_BOTTOM = (0 << 4), // default
        ADDRMODE_LCD_REFRESH_BOTTOM_TO_TOP = (1 << 4),

        ADDRMODE_RGB = (0 << 3),
        ADDRMODE_BGR = (1 << 3), // unconditionally set below

        ADDRMODE_FLIP_H = (1 << 1),
        ADDRMODE_FLIP_V = (1 << 0),

        ADDRMODE_LANDSCAPE = ADDRMODE_TB_LR, // the default
    };

    enum PixelFormat : u8
    {
        PIXELFORMAT_3_BPP = 0x11,
        PIXELFORMAT_16_BPP = 0x55,
        PIXELFORMAT_18_BPP = 0x66,
    };

    FORCEINLINE static constexpr u16 gencolor_inl(u8 r, u8 g, u8 b)
    {
        return (((r & 248) | g >> 5) << 8) | ((g & 28) << 3 | b >> 3);
    }
    static constexpr u16 gencolor(u8 r, u8 g, u8 b) // let the compiler decide whether to inline this
    {
        return gencolor_inl(r, g, b);
    }

    FORCEINLINE static void splitcolor_inl(u16 col, u8 *p)
    {
        p[0] = uhi8(col) & 248; // 5 upper bits
        p[1] = u8(col >> 3u) & 252; // 3+3 bits
        p[2] = u8(col) << 3u; // 5 bits
    }
    NOINLINE static void splitcolor(u16 col, u8 *p)
    {
        splitcolor_inl(col, p);
    }

};

template<typename Specs, typename Iface>
struct MIPI_DCS_Generic : public ChipBase<Specs, Iface>
{
    typedef Specs S;
    typedef ChipBase<S, Iface> HW;
    typedef typename HW::Ctrl Ctrl;
    typedef typename HW::DimType DimType;
    typedef typename HW::DataConnection DataConnection;
    using Op = typename S::Opcode;
    using AddressMode = typename S::AddressMode;
    using S::gencolor;
    using S::gencolor_inl;

    static void init_sequence(const u8 *tab)
    {
        HW::init();
        HW::reset();
        HW::enableCS();
        //disableDisplay();
        exit_sleep_mode();
        sendtable<Progmem>(tab);
        HW::disableCS();
        //delay_ms(40);
    }

    static void enableDisplay()
    {
        static const u8 tab[] PROGMEM =
        {
            1, Op::exit_idle_mode,
            1, Op::set_display_on,
            0
        };
        sendtable<Progmem>(&tab[0]);
    }
    static void disableDisplay()
    {
        static const u8 tab[] PROGMEM =
        {
            1, Op::enter_idle_mode,
            1, Op::set_display_off,
            0
        };
        sendtable<Progmem>(&tab[0]);
    }

    static FORCEINLINE void soft_reset() { sendcmd(Op::soft_reset); }
    static FORCEINLINE void enter_sleep_mode() { sendcmd(Op::enter_sleep_mode); }
    static FORCEINLINE void exit_sleep_mode() { sendcmd(Op::exit_sleep_mode); delay_ms(5); }
    static FORCEINLINE void enter_partial_mode() { sendcmd(Op::enter_partial_mode); }
    static FORCEINLINE void enter_invert_mode() { sendcmd(Op::enter_invert_mode); }
    static FORCEINLINE void exit_invert_mode() { sendcmd(Op::exit_invert_mode   ); }
    static FORCEINLINE void enter_idle_mode() { sendcmd(Op::enter_idle_mode); }
    static FORCEINLINE void exit_idle_mode() { sendcmd(Op::exit_idle_mode); }
    static FORCEINLINE void beginwrite() { sendcmd(Op::write_memory_start); }
    static FORCEINLINE void resumewrite() { sendcmd(Op::write_memory_continue); }
    static FORCEINLINE void set_tear_on(bool with_hblank = false) { sendcmd1(Op::set_tear_on, with_hblank); }
    static FORCEINLINE void set_tear_off() { sendcmd(Op::set_tear_off); }
    static FORCEINLINE void set_display_on() { sendcmd(Op::set_display_on); }
    static FORCEINLINE void set_display_off() { sendcmd(Op::set_display_off); }


    static constexpr DimType CONST_scroll_area_sides(DimType a, DimType b)
    {
        return (S::LONG_SIZE) - (a + b);
    }
    static NOINLINE DimType set_scroll_area_sides(DimType a, DimType b)
    {
        const DimType middle = CONST_scroll_area_sides(a, b);
        FGLCD_ASSERT(a + b + middle == S::LONG_SIZE, "ssa");
        NoInterrupt no;
        _cwru(Op::set_scroll_area, a, middle, b);
        return middle;
    }

    static FORCEINLINE void set_scroll_pos(DimType p)
    {
        FGLCD_ASSERT(int(p) >= 0 && p < S::LONG_SIZE, "ssp");
        NoInterrupt no;
        _cwru(Op::set_scroll_start, p);
    }

    static FORCEINLINE void set_tear_scanline(DimType scanline)
    {
        NoInterrupt no;
        _cwru(Op::set_tear_scanline, scanline);
    }

    static FORCEINLINE void set_address_mode(AddressMode mode)
    {
        const u8 bits = mode | S::ADDRMODE_BGR;
        NoInterrupt no;
        sendcmd1(Op::set_address_mode, bits);
    }

    //-------------------------------------

    static NOINLINE void sendcmd(u8 com)
    {
        NoInterrupt no;
        Ctrl::sendcmd(com);
    }
    static NOINLINE void sendcmd1(u8 com, u8 param)
    {
        NoInterrupt no;
        Ctrl::sendcmd(com);
        Ctrl::sendparam8(param);
    }
    static NOINLINE void sendcmd3(u8 com, u8 param, u8 param2, u8 param3)
    {
        NoInterrupt no;
        Ctrl::sendcmd(com);
        Ctrl::sendparam8(param);
        Ctrl::sendparam8(param2);
        Ctrl::sendparam8(param3);
    }
    template<typename FROM>
    static NOINLINE void sendtable(typename FROM::VoidPointer tab)
    {
        NoInterrupt no;
        Ctrl::template sendtable<FROM>(tab);
    }

    static FORCEINLINE void _setx(DimType x1, DimType x2)
    {
        //FGLCD_ASSERT((int)x1 <= (int)x2 && int(x1) >= 0 && unsigned(x2) <= S::LONG_SIZE, "setx");
        _cwru(Op::set_column_address, x1, x2);
    }
    static FORCEINLINE void _sety(DimType y1, DimType y2)
    {
        //FGLCD_ASSERT((int)y1 <= (int)y2 && int(y1) >= 0 && unsigned(y2) <= S::SHORT_SIZE, "sety");
        _cwru(Op::set_page_address, y1, y2);
    }

    static FORCEINLINE void setxy_inl(DimType x1, DimType y1, DimType x2, DimType y2)
    {
        NoInterrupt no;
        _setx(x1, x2);
        _sety(y1, y2);
        Ctrl::sendcmd(Op::write_memory_start);
    }

    static FORCEINLINE void setxywh_inl(DimType x, DimType y, DimType w, DimType h)
    {
        setxy(x, y, x+w-1, y+h-1);
    }

    static NOINLINE void setxy(DimType x1, DimType y1, DimType x2, DimType y2)
    {
        setxy_inl(x1, y1, x2, y2);
    }

    static NOINLINE void setxywh(DimType x, DimType y, DimType w, DimType h)
    {
        setxywh_inl(x,y,w,h);
    }
    
    static FORCEINLINE void _cwru(u8 cmd, DimType a)
    {
        Ctrl::sendcmd(cmd);
        Ctrl::sendparam16(a);
    }

    static FORCEINLINE void _cwru(u8 cmd, DimType a, DimType b)
    {
        Ctrl::sendcmd(cmd);
        Ctrl::sendparam16(a);
        Ctrl::sendparam16(b);
    }

    static FORCEINLINE void _cwru(u8 cmd, DimType a, DimType b, DimType c)
    {
        Ctrl::sendcmd(cmd);
        Ctrl::sendparam16(a);
        Ctrl::sendparam16(b);
        Ctrl::sendparam16(c);
    }
};

} // end namespace fglcd

FORCEINLINE fglcd::MIPI_DCS_Specs::AddressMode operator|(fglcd::MIPI_DCS_Specs::AddressMode a, fglcd::MIPI_DCS_Specs::AddressMode b)
{
    return static_cast<fglcd::MIPI_DCS_Specs::AddressMode>(uint8_t(a) | uint8_t(b));
}
