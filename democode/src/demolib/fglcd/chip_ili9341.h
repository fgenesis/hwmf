#pragma once

#if 0
// TODO check this init sequence:
// Via https://github.com/tuupola/esp-ili9341/blob/master/ili9341.c
DRAM_ATTR static const lcd_init_cmd_t lcd_init_cmds[]={
    /* Power contorl B, power control = 0, DC_ENA = 1 */
    {ILI9341_PWCTRB, {0x00, 0x83, 0X30}, 3},
    /* Power on sequence control,
     * cp1 keeps 1 frame, 1st frame enable
     * vcl = 0, ddvdh=3, vgh=1, vgl=2
     * DDVDH_ENH=1
     */
    {ILI9341_PWONCTR, {0x64, 0x03, 0X12, 0X81}, 4},
    /* Driver timing control A,
     * non-overlap=default +1
     * EQ=default - 1, CR=default
     * pre-charge=default - 1
     */
    {ILI9341_DRVTCTRA, {0x85, 0x01, 0x79}, 3},
    /* Power control A, Vcore=1.6V, DDVDH=5.6V */
    {ILI9341_PWCTRA, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
    /* Pump ratio control, DDVDH=2xVCl */
    {ILI9341_PUMPRTC, {0x20}, 1},
    /* Driver timing control, all=0 unit */
    {ILI9341_TMCTRA, {0x00, 0x00}, 2},
    /* Power control 1, GVDD=4.75V */
    {ILI9341_PWCTR1, {0x26}, 1},
    /* Power control 2, DDVDH=VCl*2, VGH=VCl*7, VGL=-VCl*3 */
    {ILI9341_PWCTR2, {0x11}, 1},
    /* VCOM control 1, VCOMH=4.025V, VCOML=-0.950V */
    {ILI9341_VMCTR1, {0x35, 0x3E}, 2},
    /* VCOM control 2, VCOMH=VMH-2, VCOML=VML-2 */
    {ILI9341_VMCTR2, {0xBE}, 1},
    /* Memory access contorl, MX=MY=0, MV=1, ML=0, BGR=1, MH=0 */
    //{ILI9341_MADCTL, {0x28}, 1},
    {ILI9341_MADCTL, {0x08}, 1}, // for M5Stack
    /* Pixel format, 16bits/pixel for RGB/MCU interface */
    {ILI9341_PIXFMT, {0x55}, 1}, // 0b01010101 ie. 16 bits per pixel
    /* Frame rate control, f=fosc, 70Hz fps */
    {ILI9341_FRMCTR1, {0x00, 0x1B}, 2},
    /* Enable 3 gamma control, disabled */
    {ILI9341_3GENABLE, {0x08}, 1},
    /* Gamma set, curve 1 */
    {ILI9341_GAMMASET, {0x01}, 1},
    /* Positive gamma correction */
    {ILI9341_GMCTRP1, {0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0X87, 0x32, 0x0A, 0x07, 0x02, 0x07, 0x05, 0x00}, 15},
    /* Negative gamma correction */
    {ILI9341_GMCTRN1, {0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78, 0x4D, 0x05, 0x18, 0x0D, 0x38, 0x3A, 0x1F}, 15},
    /* Column address set, SC=0, EC=0xEF */
    {ILI9341_CASET, {0x00, 0x00, 0x00, 0xEF}, 4},
    /* Page address set, SP=0, EP=0x013F */
    {ILI9341_PASET, {0x00, 0x00, 0x01, 0x3f}, 4},
    /* Memory write */
    {ILI9341_RAMWR, {0}, 0},
    /* Entry mode set, Low vol detect disabled, normal display */
    {ILI9341_ENTRYMODE, {0x07}, 1},
    /* Display function control */
    {ILI9341_DFUNCTR, {0x0A, 0x82, 0x27, 0x00}, 4},
    /* Sleep out */
    {ILI9341_SLPOUT, {0}, 0x80},
    /* Display on */
    {ILI9341_DISPON, {0}, 0x80},
    /* End of commands . */
    {0, {0}, 0xff},
};
#endif

#include "chip_mipi_dcs.h"

namespace fglcd {

struct ILI9341_Specs : public MIPI_DCS_Specs
{
    template<typename PIN> struct ResetTriggerType : public TriggerWaitMS<TriggerHoldMS<LowTrigger<PIN>, 10>, 120> {};
    // safer to use a LowTrigger, but this is a tad faster
    template<typename PIN> struct DCxTriggerType : public ToggleTrigger<PIN> {};

    typedef u16 DimType;
    static const DimType LONG_SIZE = 320;
    static const DimType SHORT_SIZE = 240;

    enum ChipSpecific
    {
        todo = 0
        /*power_control_1 = 0xC0,
        power_control_2 = 0xC1,
        power_control_3_normal_mode = 0xC2,
        power_control_4_idle_mode = 0xC3,
        power_control_5_partial_mode = 0xC4,
        vcom_control = 0xC5,
        display_function_control = 0xB6,
        positive_gamma_control = 0xE0,
        negative_gamma_control = 0xE1,
        memory_access_control = 0x36,*/
    };
};

template<typename Iface>
struct ILI9341 : public MIPI_DCS_Generic<ILI9341_Specs, Iface>
{
    typedef ILI9341_Specs S;
    typedef MIPI_DCS_Generic<S, Iface> Base;
    using Op = S::Opcode;
    using X = S::ChipSpecific;

    static void init()
    {
        static const u8 tab[] PROGMEM =
        {
            1+3, 0xF6, 0x01, 0x01, 0x00,  //Interface Control needs EXTC=1 MV_EOR=0, TM=0, RIM=0
            1+3, 0xCF, 0x00, 0x81, 0x30,  //Power Control B [00 81 30]
            1+4, 0xED, 0x64, 0x03, 0x12, 0x81,    //Power On Seq [55 01 23 01]
            1+3, 0xE8, 0x85, 0x10, 0x78,  //Driver Timing A [04 11 7A]
            1+5, 0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02,      //Power Control A [39 2C 00 34 02]
            1+1, 0xF7, 0x20,      //Pump Ratio [10]
            1+2, 0xEA, 0x00, 0x00,        //Driver Timing B [66 00]
            1+1, 0xB0, 0x00,      //RGB Signal [00] 
            1+2, 0xB1, 0x00, 0x1B,        //Frame Control [00 1B]
            //         0xB6, 2, 0x0A, 0xA2, 0x27, //Display Function [0A 82 27 XX]    .kbv SS=1  
            1+1, 0xB4, 0x00,      //Inversion Control [02] .kbv NLA=1, NLB=1, NLC=1
            1+1, 0xC0, 0x21,      //Power Control 1 [26]
            1+1, 0xC1, 0x11,      //Power Control 2 [00]
            1+2, 0xC5, 0x3F, 0x3C,        //VCOM 1 [31 3C]
            1+1, 0xC7, 0xB5,      //VCOM 2 [C0]
            1+1, 0x36, 0x48,      //Memory Access [00]
            1+1, 0xF2, 0x00,      //Enable 3G [02]
            1+1, 0x26, 0x01,      //Gamma Set [01]
            1+15,0xE0,  0x0f, 0x26, 0x24, 0x0b, 0x0e, 0x09, 0x54, 0xa8, 0x46, 0x0c, 0x17, 0x09, 0x0f, 0x07, 0x00,
            1+15,0xE1,  0x00, 0x19, 0x1b, 0x04, 0x10, 0x07, 0x2a, 0x47, 0x39, 0x03, 0x06, 0x06, 0x30, 0x38, 0x0f,


           //2, Op::set_address_mode, S::ADDRMODE_LANDSCAPE | (1<<3),
           2, Op::set_pixel_format, S::PIXELFORMAT_16_BPP,
           0
        };

        Base::init_sequence(&tab[0]);
        delay_ms(120);
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
