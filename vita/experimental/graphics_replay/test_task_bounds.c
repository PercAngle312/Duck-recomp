#include "task_bounds.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 const uint32_t ram=0x800000u;
 assert(duck_task_span_valid(0x80001000u,64,4,ram));
 assert(duck_task_span_valid(0xa0001000u,64,4,ram));
 assert(!duck_task_span_valid(0x807fffc0u,128,4,ram));
 assert(!duck_task_span_valid(0x80800000u,4,4,ram));
 assert(!duck_task_span_valid(0x80001001u,8,8,ram));
 assert(!duck_task_span_valid(0x80001000u,8,3,ram));
 assert(!duck_task_span_valid(0xffffffffu,0xffffffffu,8,ram));
 puts("PASS: mapped guest spans, alignment, end-of-RAM and wrap guards");
 return 0;
}
