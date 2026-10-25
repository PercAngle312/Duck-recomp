/* Guest ABI implementation, based on the contracts in libreultra pidma.c,
 * pirawdma.c, devmgr.c and seteventmesg.c. The PI worker is cooperatively
 * serviced by the host CPU loop. Guest blocking/thread wakeup is separate.
 * This file uses word-swapped RDRAM, not the separate big-endian bus model.
 */
#include "duck_vita_os_io.h"
#define RAM_BYTES 0x800000u
#define EVENT_COUNT 23u
extern void osSendMesg_recomp(uint8_t*, recomp_context*);
extern void osJamMesg_recomp(uint8_t*, recomp_context*);
extern void osRecvMesg_recomp(uint8_t*, recomp_context*);
static uint8_t *ram;
static const uint8_t *cart;
static size_t cart_size;
static uint32_t command_queue;
static int manager_active;
static struct { uint32_t queue, message; } events[EVENT_COUNT];
static recomp_context scratch;

static int span(uint64_t address, uint32_t size, uint32_t *offset) {
    uint32_t a=(uint32_t)address, p=a&0x1fffffffu;
    if(address!=(uint64_t)a && address!=(uint64_t)(int64_t)(int32_t)a)return -1;
    if((a&0xe0000000u)!=0x80000000u && (a&0xe0000000u)!=0xa0000000u)return -1;
    if(p>RAM_BYTES || size>RAM_BYTES-p)return -1;
    *offset=p;return 0;
}
static int words(uint64_t address,uint32_t size,uint32_t *offset) {
    if((uint32_t)address&3u)return -1;
    return span(address,size,offset);
}
static uint32_t rd(uint32_t p) {
    return (uint32_t)ram[p]|((uint32_t)ram[p+1]<<8)|((uint32_t)ram[p+2]<<16)|((uint32_t)ram[p+3]<<24);
}
static void wr(uint32_t p,uint32_t value) {
    ram[p]=(uint8_t)value;ram[p+1]=(uint8_t)(value>>8);
    ram[p+2]=(uint8_t)(value>>16);ram[p+3]=(uint8_t)(value>>24);
}
static int queue(uint64_t address,uint32_t *offset) {
    uint32_t p,buffer;
    if(!ram||words(address,24,&p))return -1;
    uint32_t count=rd(p+8),first=rd(p+12),capacity=rd(p+16);
    if(!capacity||capacity>RAM_BYTES/4||count>capacity||first>=capacity||
       words(rd(p+20),capacity*4,&buffer))return -1;
    *offset=p;return 0;
}
static int cart_span(uint32_t address,uint32_t size,size_t *offset) {
    uint32_t physical=address&0x1fffffffu;
    size_t off=physical>=0x10000000u ? physical-0x10000000u : physical;
    if(!cart || off>cart_size || size>cart_size-off)return -1;
    *offset=off;return 0;
}
static int send(uint32_t q,uint32_t message,int jam) {
    uint32_t p;
    if(queue(q,&p))return -1;
    scratch.r4=q;scratch.r5=message;scratch.r6=0;
    if(jam)osJamMesg_recomp(ram,&scratch);else osSendMesg_recomp(ram,&scratch);
    return scratch.r2==0 ? 0 : -1;
}
int duck_vita_os_io_reset(uint8_t *rdram,const uint8_t *rom,size_t size) {
    ram=0;cart=0;cart_size=0;command_queue=0;manager_active=0;
    for(unsigned i=0;i<EVENT_COUNT;i++){events[i].queue=0;events[i].message=0;}
    if(!rdram||!rom||!size)return -1;
    ram=rdram;cart=rom;cart_size=size;return 0;
}
void osCreatePiManager_recomp(uint8_t *rdram,recomp_context *ctx) {
    uint32_t q,buffer;
    if(!ctx||rdram!=ram||!ram||manager_active)return;
    uint32_t capacity=(uint32_t)ctx->r7;
    if(!capacity||capacity>RAM_BYTES/4 || words(ctx->r5,24,&q)||
       words(ctx->r6,capacity*4,&buffer))return;
    if(q<buffer+capacity*4 && buffer<q+24)return;
    wr(q,0);wr(q+4,0);wr(q+8,0);wr(q+12,0);wr(q+16,capacity);wr(q+20,(uint32_t)ctx->r6);
    command_queue=(uint32_t)ctx->r5;manager_active=1;
}
void osPiReadIo_recomp(uint8_t *rdram,recomp_context *ctx) {
    uint32_t dest;size_t off;
    if(!ctx)return;
    ctx->r2=(gpr)-1;
    if(!ram||rdram!=ram||((uint32_t)ctx->r4&3u)||words(ctx->r5,4,&dest)||
       cart_span((uint32_t)ctx->r4,4,&off))return;
    uint32_t value=((uint32_t)cart[off]<<24)|((uint32_t)cart[off+1]<<16)|
                   ((uint32_t)cart[off+2]<<8)|cart[off+3];
    wr(dest,value);ctx->r2=0;
}
void osPiStartDma_recomp(uint8_t *rdram,recomp_context *ctx) {
    uint32_t stack,mb,dest,q;size_t off;
    if(!ctx)return;
    ctx->r2=(gpr)-1;
    if(!ram||rdram!=ram||!manager_active||ctx->r6!=0||ctx->r5>1||
       words(ctx->r29,28,&stack)||words(ctx->r4,24,&mb))return;
    uint32_t dram=rd(stack+16),size=rd(stack+20),ret=rd(stack+24);
    if(!size||(dram&7u)||((uint32_t)ctx->r7&1u)||span(dram,size,&dest)||
       queue(ret,&q)||cart_span((uint32_t)ctx->r7,size,&off))return;
    /* OS_MESG_TYPE_DMAREAD=11. Message fields are guest bytes, not host layout. */
    wr(mb,(11u<<16)|((uint32_t)ctx->r5<<8));wr(mb+4,ret);
    wr(mb+8,dram);wr(mb+12,(uint32_t)ctx->r7);wr(mb+16,size);wr(mb+20,0);
    ctx->r2=send(command_queue,(uint32_t)ctx->r4,ctx->r5==1)==0 ? 0 : (gpr)-1;
}
int duck_vita_os_pi_service(void) {
    uint32_t q,buffer,mb,dest,retq;size_t off;
    if(!ram||!manager_active)return 0;
    if(queue(command_queue,&q))return -1;
    if(!rd(q+8))return 0;
    if(words(rd(q+20),rd(q+16)*4,&buffer))return -1;
    uint32_t message=rd(buffer+rd(q+12)*4);
    scratch.r4=command_queue;scratch.r5=0;scratch.r6=0;
    osRecvMesg_recomp(ram,&scratch);
    if(scratch.r2 || words(message,24,&mb))return -1;
    uint32_t dram=rd(mb+8),size=rd(mb+16),ret=rd(mb+4);
    if((rd(mb)>>16)!=11||!size||(dram&7u)||(rd(mb+12)&1u)||
       span(dram,size,&dest)||cart_span(rd(mb+12),size,&off)||queue(ret,&retq))return -1;
    for(uint32_t i=0;i<size;i++)ram[(dest+i)^3u]=cart[off+i];
    /* libultra sends the OSIoMesg pointer, not a fabricated zero completion. */
    return send(ret,message,0)==0 ? 1 : -1;
}
void osSetEventMesg_recomp(uint8_t *rdram,recomp_context *ctx) {
    uint32_t p;
    if(!ctx||!ram||rdram!=ram||ctx->r4>=EVENT_COUNT)return;
    if(ctx->r5 && queue(ctx->r5,&p))return;
    events[ctx->r4].queue=(uint32_t)ctx->r5;events[ctx->r4].message=(uint32_t)ctx->r6;
}
int duck_vita_os_event_emit(unsigned event) {
    if(!ram||event>=EVENT_COUNT||!events[event].queue)return -1;
    return send(events[event].queue,events[event].message,0);
}
