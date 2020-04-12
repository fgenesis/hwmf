#pragma once

// Most of this sourced from https://github.com/Bodmer/TFT_HX8357

// UNTESTED because my display broke

#include "chip_mipi_dcs.h"

namespace fglcd {

struct HX8357_Specs : public MIPI_DCS_Specs
{
    // reset needs a bit of wait time before the screen is fully initialized
    template<typename PIN> struct ResetTriggerType : public TriggerWaitMS<TriggerHoldMS<LowTrigger<PIN>, 10>, 10> {};
    // safer to use a LowTrigger, but this is a tad faster
    //template<typename PIN> struct DCxTriggerType : public ToggleTrigger<PIN> {};

    typedef u16 DimType;
    static const DimType LONG_SIZE = 480;
    static const DimType SHORT_SIZE = 320;

    enum ChipSpecific
    {
        enable_extension = 0xB9,
        set_vcom_voltage = 0xB6,
        set_power_control = 0xB1,
        setstba = 0xC0,
        set_display_cycle = 0xB4,
        set_gamma_curve = 0xE0,
        memory_access_control = 0x36,
    };
};

template<typename Iface>
struct HX8357_Base : public MIPI_DCS_Generic<HX8357_Specs, Iface>
{
    typedef HX8357_Specs S;
    typedef MIPI_DCS_Generic<S, Iface> Base;
    typedef ChipBase<S, Iface> HW;
    using Op = S::Opcode;
    using X = S::ChipSpecific;

    static void _init_HX8357(const u8 *revtab)
    {
        static const u8 generictab[] PROGMEM =
        {
           2, Op::set_address_mode, S::ADDRMODE_LANDSCAPE | S::ADDRMODE_BGR,
           2, Op::set_pixel_format, S::PIXELFORMAT_16_BPP,
           0
        };

        Base::init_sequence(&revtab[0]);
        Base::init_sequence(&generictab[0]);
        Base::enableCS();
        enableDisplay();
        Base::disableCS();
    }

    static FORCEINLINE void enableDisplay()
    {
        Base::enableDisplay();
    }

    static FORCEINLINE void disableDisplay()
    {
        Base::disableDisplay();
    }

    // TODO: write me
    static FORCEINLINE void slow_refresh() { }
    static FORCEINLINE void fast_refresh() { }


};


template<typename Iface>
struct HX8357C : public HX8357_Base<Iface>
{
    typedef HX8357_Specs S;
    typedef HX8357_Base<Iface> Base;
    using X = S::ChipSpecific;

    static void init()
    {
        static const u8 tab[] PROGMEM =
        {
            4, X::enable_extension, 0xFF, 0x83, 0x57, // 3 magic bytes according to datasheet
            2, X::set_vcom_voltage, 0x2C,
            7, X::set_power_control, 0x00, 0x15, 0x0D, 0x0D, 0x83, 0x48,
            7, X::setstba, 0x24, 0x24, 0x01, 0x3C, 0xC8, 0x08,
            8, X::set_display_cycle, 0x02, 0x40, 0x00, 0x2A, 0x2A, 0x0D, 0x4F,
            35, X::set_gamma_curve, 0x00, 0x15, 0x1D, 0x2A, 0x31, 0x42, 0x4C, 0x53, 0x45, 0x40, 0x3B, 0x32, 0x2E, 0x28,
                                    0x24, 0x03, 0x00, 0x15, 0x1D, 0x2A, 0x31, 0x42, 0x4C, 0x53, 0x45, 0x40, 0x3B, 0x32,
                                    0x2E, 0x28, 0x24, 0x03, 0x00, 0x01,
            0
        };

        Base::_init_HX8357(tab);
    }

    static FORCEINLINE void enableDisplay()
    {
        Base::enableDisplay();
    }

    static FORCEINLINE void disableDisplay()
    {
        Base::disableDisplay();
    }

    // TODO: write me
    static FORCEINLINE void slow_refresh() { }
    static FORCEINLINE void fast_refresh() { }


};


} // end namespace fglcd
