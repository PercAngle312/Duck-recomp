#pragma once
#include <stdint.h>
// A collision only loses the fast path; it never substitutes another FBO's fence.
typedef struct {uintptr_t target;uint32_t serial;} DuckFramebufferFence;
typedef struct {DuckFramebufferFence entries[64];} DuckFramebufferFences;
static inline unsigned duck_fence_slot(uintptr_t p){return (unsigned)((p>>4)^(p>>11))&63u;}
static inline void duck_fence_record(DuckFramebufferFences *f,const void *target,uint32_t serial){
 unsigned slot=duck_fence_slot((uintptr_t)target);f->entries[slot].target=(uintptr_t)target;f->entries[slot].serial=serial;
}
static inline uint32_t duck_fence_lookup(const DuckFramebufferFences *f,const void *target,uint32_t latest){
 const DuckFramebufferFence *e=&f->entries[duck_fence_slot((uintptr_t)target)];
 return target&&e->target==(uintptr_t)target&&e->serial&&e->serial<=latest?e->serial:0;
}
