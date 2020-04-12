#pragma once

#include "chip_mipi_dcs.h"

namespace fglcd {

struct ILI9481_Specs : public MIPI_DCS_Specs
{
    // this chip doesn't seem to need a wait time after a reset signal
    template<typename PIN> struct ResetTriggerType : public TriggerHoldMS<LowTrigger<PIN>, 10> {};

    // safer to use a LowTrigger, but this is a tad faster
    template<typename PIN> struct DCxTriggerType : public ToggleTrigger<PIN> {};

    typedef u16 DimType;
    static const DimType LONG_SIZE = 480;
    static const DimType SHORT_SIZE = 320;

    enum ChipSpecific
    {
        power_setting = 0xD0,
        vcom_control = 0xD1,
        set_gamma_table = 0xC8,
        power_setting_normal_mode = 0xD2,
        power_setting_idle_mode = 0xd4,
        panel_driving_setting = 0xC0,
        low_power_mode_control = 0xb1,
        set_display_and_memory_mode = 0xb4,
        display_timing_setting_normal_mode = 0xc1,
        frame_rate_control = 0xc5,
    };
};

template<typename Iface>
struct ILI9481 : public MIPI_DCS_Generic<ILI9481_Specs, Iface>
{
    typedef ILI9481_Specs S;
    typedef MIPI_DCS_Generic<S, Iface> Base;
    using Op = S::Opcode;
    using X = S::ChipSpecific;

    enum Framerate : u8
    {
        HZ_125,
        HZ_100,
        HZ_85,
        HZ_72,
        HZ_56,
        HZ_50,
        HZ_45,
        HZ_42,

        HZ_DEFAULT = HZ_85
    };

    static void init()
    {
        static const constexpr u8 tab[] PROGMEM =
        {
            4, X::vcom_control, 0x00, 0x07, 0x10,
            3, X::power_setting_normal_mode, 0x01, 0x02,
            3, X::power_setting_idle_mode, 0x00, 0x77,
            6, X::panel_driving_setting, 0x10, 0x3B, 0x00, 0x02, 0x11,
            13, X::set_gamma_table, 0x00, 0x32, 0x36, 0x45, 0x06, 0x16, 0x37, 0x75, 0x77, 0x54, 0x0C, 0x00,
            2, Op::set_address_mode, uint8_t(S::ADDRMODE_LANDSCAPE) | uint8_t(S::ADDRMODE_BGR),
            2, Op::set_pixel_format, S::PIXELFORMAT_16_BPP,
            //1, Op::set_display_and_memory_mode, (1<<0) | (0 << 4),
            0
        };

        Base::init_sequence(&tab[0]);
        Base::enableCS();
        enableDisplay();
        Base::disableCS();
    }

    static void enableDisplay()
    {
        static const u8 tab[] PROGMEM =
        {
            4, X::power_setting, 0x07, 0x42, 0x18,
            2, X::low_power_mode_control, 0x00,
            0
        };
        Base::enableDisplay();
        Base::template sendtable<Progmem>(&tab[0]);
    }

    static void disableDisplay()
    {
        Base::disableDisplay();
        static const u8 tab[] PROGMEM =
        {
            4, X::power_setting, 0x00, 0x02, 0x10,
            2, X::low_power_mode_control, 0x01,
            0
        };
        Base::template sendtable<Progmem>(&tab[0]);
    }

    static FORCEINLINE void slow_refresh() { Base::sendcmd3(X::display_timing_setting_normal_mode, (1<<4)|3, 31, 0xff); }
    static FORCEINLINE void fast_refresh() { Base::sendcmd3(X::display_timing_setting_normal_mode, (1<<4), 16, (8 << 4) | 8); }

    static FORCEINLINE void set_refresh_rate(Framerate hz) { Base::sendcmd1(X::frame_rate_control, hz); }

    /*
    static void normalPowerMode()
    {
        static const u8 tab[] PROGMEM =
        {
            4, Op::vcom_control, 0x00, 0x07, 0x10,
            3, Op::power_setting_normal_mode, 0x01, 0x02,
            4, Op::power_setting, 0x07, 0x42, 0x18,
            0
        };
        sendtable<Progmem>(&tab[0]);
    }

    static void lowPowerMode()
    {
        static const u8 tab[] PROGMEM =
        {
            4, Op::vcom_control, 0x00, 0x00, 0x00,
            3, Op::power_setting_normal_mode, 0x07, 0x02,
            4, Op::power_setting, 0x01, 0x47, 0x11,
            0
        };
        sendtable<Progmem>(&tab[0]);
    }*/
};

} // end namespace fglcd
