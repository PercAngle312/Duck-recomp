/* Fatal translated instruction/dispatch faults. These stop execution with
 * context rather than skipping invalid guest code or inventing return values. */
#include "recomp.h"
#include <stdio.h>
#include <stdlib.h>
void do_break(uint32_t pc){
 fprintf(stderr,"[guest] BREAK at %08x\n",pc);abort();
}
void switch_error(const char *function,uint32_t pc,uint32_t table){
 fprintf(stderr,"[guest] Invalid jump table in %s at %08x (table %08x)\n",function?function:"(unknown)",pc,table);abort();
}
