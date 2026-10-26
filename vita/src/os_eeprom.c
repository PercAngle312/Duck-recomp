/* Synchronous EEPROM HLE, preserving guest block and word-swapped byte ABI.
 * Storage failure is a guest failure, never an acknowledged volatile save.
 * SI/PIF bit timing and the original per-block programming delay are not
 * emulated by this synchronous storage boundary. */
#include "duck_vita_eeprom.h"
#include <string.h>
static uint8_t image[2048];
static unsigned capacity;
static DuckEepromStorage backend;
void duck_vita_eeprom_detach(void){capacity=0;memset(&backend,0,sizeof backend);}
int duck_vita_eeprom_init(unsigned bytes,const DuckEepromStorage *storage){
 uint8_t initial[2048];
 duck_vita_eeprom_detach();
 if((bytes!=512&&bytes!=2048)||!storage||!storage->load||!storage->commit)return -1;
 memset(initial,0xff,sizeof initial);int result=storage->load(storage->opaque,initial,bytes);
 if(result!=0&&result!=1)return -1;
 if(result==1)memset(initial,0xff,bytes);
 memcpy(image,initial,bytes);capacity=bytes;backend=*storage;return 0;
}
static int span(uint64_t value,uint32_t bytes,uint32_t *physical){
 uint32_t a=(uint32_t)value,p=a&0x1fffffffu;
 if(value!=(uint64_t)a&&value!=(uint64_t)(int64_t)(int32_t)a)return -1;
 if((a&0xe0000000u)!=0x80000000u&&(a&0xe0000000u)!=0xa0000000u)return -1;
 if(p>0x800000u||bytes>0x800000u-p)return -1;
 *physical=p;return 0;
}
static int transfer(const recomp_context *c,uint32_t *start,uint32_t *bytes,uint32_t *p){
 uint32_t block=(uint32_t)c->r5,length=(uint32_t)c->r7;
 if(!capacity||c->r5!=block||block>255||c->r7!=length||(length&7)||
    block>=capacity/8||length>capacity-block*8||span(c->r6,length,p))return -1;
 *start=block*8;*bytes=length;return 0;
}
void osEepromProbe_recomp(uint8_t *ram,recomp_context *c){
 (void)ram;if(c)c->r2=capacity==512?1:capacity==2048?2:0;
}
void osEepromLongRead_recomp(uint8_t *ram,recomp_context *c){
 if(!c)return;
 c->r2=(gpr)-1;uint32_t start,bytes,p;
 if(!ram||transfer(c,&start,&bytes,&p))return;
 for(uint32_t i=0;i<bytes;i++)ram[(p+i)^3u]=image[start+i];
 c->r2=0;
}
void osEepromLongWrite_recomp(uint8_t *ram,recomp_context *c){
 if(!c)return;
 c->r2=(gpr)-1;uint32_t start,bytes,p;uint8_t next[2048];
 if(!ram||transfer(c,&start,&bytes,&p))return;
 if(!bytes){c->r2=0;return;}
 memcpy(next,image,capacity);
 for(uint32_t i=0;i<bytes;i++)next[start+i]=ram[(p+i)^3u];
 if(backend.commit(backend.opaque,next,capacity)!=0)return;
 memcpy(image,next,capacity);c->r2=0;
}
