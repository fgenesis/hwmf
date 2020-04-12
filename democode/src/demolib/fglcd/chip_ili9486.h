#pragma once

#include "chip_mipi_dcs.h"

namespace fglcd {

struct ILI9486_Specs : public MIPI_DCS_Specs
{
    // reset needs a bit of wait time before the screen is fully initialized
    template<typename PIN> struct ResetTriggerType : public TriggerWaitMS<TriggerHoldMS<LowTrigger<PIN>, 10>, 10> {};
    // safer to use a LowTrigger, but this is a tad faster
    template<typename PIN> struct DCxTriggerType : public ToggleTrigger<PIN> {};

    typedef u16 DimType;
    static const DimType LONG_SIZE = 480;
    static const DimType SHORT_SIZE = 320;

    enum ChipSpecific
    {
        power_control_1 = 0xC0,
        power_control_2 = 0xC1,
        power_control_3_normal_mode = 0xC2,
        power_control_4_idle_mode = 0xC3,
        power_control_5_partial_mode = 0xC4,
        vcom_control = 0xC5,
        display_function_control = 0xB6,
        positive_gamma_control = 0xE0,
        negative_gamma_control = 0xE1,
        memory_access_control = 0x36,
    };
};

template<typename Iface>
struct ILI9486 : public MIPI_DCS_Generic<ILI9486_Specs, Iface>
{
    typedef ILI9486_Specs S;
    typedef MIPI_DCS_Generic<S, Iface> Base;
    using Op = S::Opcode;
    using X = S::ChipSpecific;

    static void init()
    {
        static const constexpr u8 tab[] PROGMEM =
        {
           10, 0xF2, 0x1C, 0xA3, 0x32, 0x02, 0xb2, 0x12, 0xFF, 0x12, 0x00,
           3, 0xF1, 0x36, 0xA4,
           3, 0xF8, 0x21, 0x04,
           3, 0xF9, 0x00, 0x08,
           3, X::power_control_1, 0x0d, 0x0d,
           3, X::power_control_2, 0x43, 0x00,
           2, X::power_control_3_normal_mode, 0x00,
           3, X::vcom_control, 0x00, 0x48,
           4, X::display_function_control, 0x00, 0x22, 0x3B,
           16, X::positive_gamma_control, 0x0f, 0x24, 0x1c, 0x0a, 0x0f, 0x08, 0x43, 0x88, 0x32, 0x0f, 0x10, 0x06, 0x0f, 0x07, 0x00,
           16, X::negative_gamma_control, 0x0F, 0x38, 0x30, 0x09, 0x0f, 0x0f, 0x4e, 0x77, 0x3c, 0x07, 0x10, 0x05, 0x23, 0xb1, 0x00,
           2, X::memory_access_control, 0x0A,
           2, Op::set_address_mode, uint8_t(S::ADDRMODE_LANDSCAPE) | uint8_t(S::ADDRMODE_BGR),
           2, Op::set_pixel_format, S::PIXELFORMAT_16_BPP,
           0
        };

        Base::init_sequence(&tab[0]);
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

} // end namespace fglcd
