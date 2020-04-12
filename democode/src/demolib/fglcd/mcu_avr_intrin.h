#pragma once

namespace intrin {

static FORCEINLINE uint8_t _avr_lpm_postinc(const void ** p)
{
    uint8_t ret;
    asm volatile(
        "lpm %[ret],%a[p]+   \n\t"
        : [ret] "=&r" (ret), [p] "+z" (*p) /*outputs*/
    );
    return ret;
}

static FORCEINLINE uint8_t _avr_elpm_postinc(const void ** p)
{
    uint8_t ret;
    asm volatile(
        "elpm %[ret],%a[p]+   \n\t"
        : [ret] "=&r" (ret), [p] "+z" (*p) /*outputs*/
    );
    return ret;
}

static FORCEINLINE uint8_t _avr_ldx_postinc(const void ** p)
{
    uint8_t ret;
    asm volatile(
        "ld %[ret],%a[p]+   \n\t"
        : [ret] "=&r" (ret), [p] "+x" (*p) /*outputs*/
    );
    return ret;
}

static FORCEINLINE uint8_t _avr_ldy_postinc(const void ** p)
{
    uint8_t ret;
    asm volatile(
        "ld %[ret],%a[p]+   \n\t"
        : [ret] "=&r" (ret), [p] "+y" (*p) /*outputs*/
    );
    return ret;
}

static FORCEINLINE uint8_t _avr_ld_postinc(const void ** p)
{
    uint8_t ret;
    asm volatile(
        "ld %[ret],%a[p]+   \n\t"
        : [ret] "=&r" (ret), [p] "+e" (*p) /*outputs*/
    );
    return ret;
}

static FORCEINLINE void _avr_stx_postinc(void ** p, uint8_t v)
{
    asm volatile(
        "st %a[p]+,%[v]   \n\t"
        : [p] "+x" (*p) /*outputs*/
        : [v] "r" (v)
    );
}

static FORCEINLINE void _avr_sty_postinc(void ** p, uint8_t v)
{
    asm volatile(
        "st %a[p]+,%[v]   \n\t"
        : [p] "+y" (*p) /*outputs*/
        : [v] "r" (v)
    );
}

static FORCEINLINE void _avr_st_postinc(void ** p, uint8_t v)
{
    asm volatile(
        "st %a[p]+,%[v]   \n\t"
        : [p] "+e" (*p) /*outputs*/
        : [v] "r" (v)
    );
}

//---------------------------------------------------------------------------

#if 0

// BOTH UNTESTED

// count leading ones and modify input
static FORCEINLINE uint8_t _avr_clo8_modify(uint8_t& v)
{
    uint8_t ret = 0;
    asm volatile(
        "loop_%=:  \n\t"
            "bst %[v], 7   \n\t"     /* remember bit we're going to shift into carry */
            "lsl %[v]   \n\t"        /* shift highest bit into carry */
            "adc %[ret], 0   \n\t"   /* accumulate */
            "brts loop_%=   \n\t"    /* if bit was set, keep going*/
        "lsr %[v]  \n\t"             /* we shifted one too many, undo that */
        : [ret] "=&r" (ret), [v] "+r" (v) /*outputs*/
    );
    return ret;
}


static FORCEINLINE uint8_t _avr_clz8_modify(uint8_t& v)
{
    uint8_t ret = 0;
    asm volatile(
        "loop_%=:  \n\t"
            "bst %[v], 7   \n\t"
            "lsl %[v]   \n\t"
            "subi %[ret],lo8(-(1))  \n\t" // ret must be upper register
            "brtc loop_%=   \n\t"
        "lsr %[v]  \n\t"
        : [ret] "=&d" (ret), [v] "+r" (v) /*outputs*/
    );
    return ret;
}
#endif



} // end namespace intrin
