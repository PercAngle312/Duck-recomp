#pragma once
/* VitaSDK has memalign/free rather than posix_memalign. These are the allocator
 * hooks supported by the pinned SSE2NEON header; RSP vector operations otherwise
 * use that header unchanged. Only compiled for ARM32 NEON builds. */
#include <cstdlib>
#include <malloc.h>
#define SSE2NEON_ALLOC_DEFINED
static inline void *_mm_malloc(size_t size,size_t alignment){
    if(alignment<sizeof(void*))alignment=sizeof(void*);
    return memalign(alignment,size);
}
static inline void _mm_free(void *pointer){free(pointer);}
#include <sse2neon.h>
