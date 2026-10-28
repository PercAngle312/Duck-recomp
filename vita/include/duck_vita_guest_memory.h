#pragma once
/* Checked 8 MiB guest RDRAM access; MMIO and ROM are deliberately NOT RAM.
 * N64 KSEG0/KSEG1 mappings resolve through the low 29 physical address bits.
 * Other virtual segments require TLB handling and are rejected here.
 */
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { DUCK_VITA_RDRAM_BYTES = 8 * 1024 * 1024 };
typedef struct {
    uint8_t *bytes;
    size_t size;
} DuckVitaRdram;
typedef enum {
    DUCK_GUEST_RAM_OK = 0,
    DUCK_GUEST_RAM_BAD_ARG = -1,
    DUCK_GUEST_RAM_UNMAPPED = -2,
    DUCK_GUEST_RAM_RANGE = -3,
    DUCK_GUEST_RAM_ALIGN = -4
} DuckGuestMemoryResult;
/* vaddr can be sign-extended MIPS32, zero-extended MIPS32 or a raw
 * 32-bit guest virtual address. No host pointer is ever derived unchecked. */
int duck_vita_guest_span(const DuckVitaRdram *ram, uint64_t vaddr,
                         size_t length, size_t alignment, uint8_t **out);
int duck_vita_guest_read_be32(const DuckVitaRdram *ram, uint64_t addr, uint32_t *out);
int duck_vita_guest_write_be32(const DuckVitaRdram *ram, uint64_t addr, uint32_t value);
#ifdef __cplusplus
}
#endif
