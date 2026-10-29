/* Native controller snapshots with asynchronous SI completion. No PIF bus
 * emulation is claimed. Status is 4 bytes/port, pad data is 6 bytes/port. */
#include "duck_vita_input.h"
#include "duck_vita_os_io.h"
#include <string.h>
typedef struct {uint16_t buttons;int8_t x,y;int connected;} Snapshot;
static Snapshot current,read_data,query_data;
static int pending,read_ready,query_ready,have_snapshot;
static int span(uint64_t address,uint32_t size,uint32_t alignment,uint32_t *out){
 uint32_t a=(uint32_t)address,p=a&0x1fffffffu;
 if(address!=(uint64_t)a&&address!=(uint64_t)(int64_t)(int32_t)a)return -1;
 if((a&0xe0000000u)!=0x80000000u&&(a&0xe0000000u)!=0xa0000000u)return -1;
 if((p&(alignment-1))||p>0x800000u||size>0x800000u-p)return -1;
 *out=p;return 0;
}
static void byte(uint8_t *r,uint32_t p,uint8_t value){r[p^3u]=value;}
void duck_pad_reset(void){memset(&current,0,sizeof current);read_data=query_data=current;pending=read_ready=query_ready=have_snapshot=0;}
int duck_pad_needs_poll(void){return !have_snapshot||pending!=0;}
void duck_pad_update(int connected,uint16_t buttons,int8_t x,int8_t y){
 have_snapshot=1;current.connected=!!connected;current.buttons=connected?buttons:0;current.x=connected?x:0;current.y=connected?y:0;
}
static void status(uint8_t *r,uint32_t p,const Snapshot *s){
 for(unsigned i=0;i<4;i++){
  int present=i==0&&s->connected;
  byte(r,p+i*4,0);byte(r,p+i*4+1,present?5:0);
  byte(r,p+i*4+2,0);byte(r,p+i*4+3,present?0:8);
 }
}
void osContInit_recomp(uint8_t *r,recomp_context *c){
 uint32_t mask,dest;if(!c)return;c->r2=(gpr)-1;
 if(!r||span(c->r5,1,1,&mask)||span(c->r6,16,2,&dest))return;
 query_data=current;query_ready=1;byte(r,mask,current.connected?1:0);status(r,dest,&query_data);c->r2=0;
}
static void start(uint8_t *r,recomp_context *c,int kind){
 uint32_t p;if(!c)return;c->r2=(gpr)-1;
 if(!r||pending||span(c->r4,24,4,&p))return;
 pending=kind;if(kind==1)read_ready=0;else query_ready=0;c->r2=0;
}
void osContStartReadData_recomp(uint8_t *r,recomp_context *c){start(r,c,1);}
void osContStartQuery_recomp(uint8_t *r,recomp_context *c){start(r,c,2);}
int duck_pad_service(void){
 if(!pending)return 0;
 int kind=pending;pending=0;
 if(kind==1){read_data=current;read_ready=1;}else{query_data=current;query_ready=1;}
 /* OS_EVENT_SI=5. Queue-full/unregistered notifications may be dropped, as
    with nonblocking libultra interrupt delivery; data has still completed. */
 duck_vita_os_event_emit(5);return 1;
}
void osContGetQuery_recomp(uint8_t *r,recomp_context *c){
 uint32_t p;if(!c)return;c->r2=(gpr)-1;
 if(!r||!query_ready||span(c->r4,16,2,&p))return;
 status(r,p,&query_data);c->r2=0;
}
void osContGetReadData_recomp(uint8_t *r,recomp_context *c){
 uint32_t p;if(!c)return;c->r2=(gpr)-1;
 if(!r||!read_ready||span(c->r4,24,2,&p))return;
 for(unsigned i=0;i<4;i++){
  int present=i==0&&read_data.connected;uint16_t buttons=present?read_data.buttons:0;
  byte(r,p+i*6,(uint8_t)(buttons>>8));byte(r,p+i*6+1,(uint8_t)buttons);
  byte(r,p+i*6+2,present?(uint8_t)read_data.x:0);byte(r,p+i*6+3,present?(uint8_t)read_data.y:0);
  byte(r,p+i*6+4,present?0:8);byte(r,p+i*6+5,0);
 }
 c->r2=0;
}
