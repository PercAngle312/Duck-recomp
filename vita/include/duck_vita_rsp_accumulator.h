#pragma once
#if defined(__ARM_NEON)
#include <arm_neon.h>

// Each limb stays modulo 16 bits. A carry comparison yields 0xffff, so
// subtracting it adds one to the next limb. Keep both middle-limb carries:
// adding the product and then the low carry may each overflow separately.
inline void duck_rsp_accumulate48(uint16x8_t &l, uint16x8_t &m, uint16x8_t &h,
                                 uint16x8_t pl, uint16x8_t pm, uint16x8_t ph) {
    const uint16x8_t nl = vaddq_u16(l, pl);
    const uint16x8_t lc = vcltq_u16(nl, pl);
    const uint16x8_t partial = vaddq_u16(m, pm);
    const uint16x8_t mc = vcltq_u16(partial, pm);
    const uint16x8_t nm = vsubq_u16(partial, lc);
    const uint16x8_t carry = vcltq_u16(nm, partial);
    h = vsubq_u16(vsubq_u16(vaddq_u16(h, ph), mc), carry);
    l = nl;
    m = nm;
}

// The signed upper 32 bits fit in int16 iff high is middle's sign extension.
// Otherwise choose the endpoint using the high limb's sign, without widening.
inline uint16x8_t duck_rsp_saturate_middle(uint16x8_t m, uint16x8_t h) {
    const uint16x8_t sign = vreinterpretq_u16_s16(vshrq_n_s16(vreinterpretq_s16_u16(m), 15));
    const uint16x8_t fits = vceqq_u16(h, sign);
    const uint16x8_t negative = vreinterpretq_u16_s16(vshrq_n_s16(vreinterpretq_s16_u16(h), 15));
    return vbslq_u16(fits, m, veorq_u16(vdupq_n_u16(0x7fff), negative));
}
#endif
