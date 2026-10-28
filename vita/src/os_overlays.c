/* Static native functions, dynamic checked guest addresses. Catalog data is
 * generated privately. Guest loader applies its own relocations before the
 * registration hook; this layer updates native dispatch/section addresses. */
#include "duck_vita_overlays.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
enum { SECTIONS=512 };
static int32_t addresses[SECTIONS];int32_t *section_addresses=addresses;
static struct {uint32_t base,allocation,end;int state;} loaded[SECTIONS];
static const DuckCodeCatalog *catalog;
static uint8_t *ram;
/* Indirect game calls repeatedly visit the same native functions. Cache exact
 * normalized PCs, and invalidate whenever a module mapping is removed/loaded.
 * Freed code remains mapped for destructor calls until allocation reuse. */
static struct {uint32_t pc;recomp_func_t *function;} dispatch_cache[1024];
static void invalidate_dispatch(void){memset(dispatch_cache,0,sizeof dispatch_cache);}
static int pointer(uint32_t address,uint32_t size,uint32_t *physical){
 uint32_t p=address&0x1fffffffu;
 if((address&0xe0000000u)!=0x80000000u&&(address&0xe0000000u)!=0xa0000000u)return -1;
 if((p&3)||p>0x800000u||size>0x800000u-p)return -1;
 *physical=p;return 0;
}
static int functions(const DuckCodeFunction *f,size_t count,uint32_t limit){
 if(count&&!f)return -1;
 for(size_t i=0;i<count;i++)if(!f[i].function||(f[i].offset&3)||f[i].offset>=limit||
   (i&&f[i-1].offset>=f[i].offset))return -1;
 return 0;
}
int duck_vita_overlays_init(uint8_t *rdram,const DuckCodeCatalog *c){
 uint32_t start,end;
 if(!rdram||!c||!c->resident_count||c->module_count>=SECTIONS||
    (c->module_count&&(!c->modules||!c->allocation_observer))||pointer(c->resident_begin,4,&start)||
    pointer(c->resident_end,0,&end)||start>=end||functions(c->resident,c->resident_count,UINT32_MAX))return -1;
 for(size_t i=0;i<c->resident_count;i++)if(c->resident[i].offset<c->resident_begin||c->resident[i].offset>=c->resident_end)return -1;
 for(size_t i=0;i<c->module_count;i++){
  const DuckCodeModule *m=&c->modules[i];
  if(!m->section||m->section>=SECTIONS||!m->initialized||m->initialized>0x800000u||
    m->bss>0x800000u-m->initialized||pointer(m->default_base,m->initialized+m->bss,&start)||
    functions(m->functions,m->function_count,m->initialized))return -1;
  for(size_t j=0;j<i;j++)if(c->modules[j].id==m->id||c->modules[j].section==m->section)return -1;
 }
 invalidate_dispatch();catalog=c;ram=rdram;memset(loaded,0,sizeof loaded);memset(addresses,0,sizeof addresses);
 addresses[0]=(int32_t)c->resident_begin;
 for(size_t i=0;i<c->module_count;i++)addresses[c->modules[i].section]=(int32_t)c->modules[i].default_base;
 return 0;
}
static int overlaps(uint32_t a,uint32_t end,uint32_t b,uint32_t b_end){return a<b_end&&b<end;}
static void remove_module(size_t i){invalidate_dispatch();loaded[i].state=0;addresses[catalog->modules[i].section]=(int32_t)catalog->modules[i].default_base;}
int duck_vita_overlay_load(uint32_t id,uint32_t base,uint32_t allocation,uint32_t initialized,uint32_t bss){
 uint32_t p,owner;size_t index;
 if(!catalog||initialized>0x800000u||bss>0x800000u-initialized||
   pointer(base,initialized+bss,&p)||pointer(allocation,4,&owner)||owner>p)return -1;
 if(overlaps(p,p+initialized+bss,catalog->resident_begin&0x1fffffffu,catalog->resident_end&0x1fffffffu))return -1;
 for(index=0;index<catalog->module_count&&catalog->modules[index].id!=id;index++);
 if(index==catalog->module_count||catalog->modules[index].initialized!=initialized||catalog->modules[index].bss!=bss)return -1;
 for(size_t i=0;i<catalog->module_count;i++)if(loaded[i].state==1&&
   (i==index||loaded[i].allocation==owner||overlaps(p,p+initialized+bss,loaded[i].base,loaded[i].end)))return -1;
 for(size_t i=0;i<catalog->module_count;i++)if(loaded[i].state==2&&
   (i==index||overlaps(p,p+initialized+bss,loaded[i].base,loaded[i].end)))remove_module(i);
 loaded[index].base=p;loaded[index].allocation=owner;loaded[index].end=p+initialized+bss;loaded[index].state=1;
 invalidate_dispatch();
 addresses[catalog->modules[index].section]=(int32_t)(0x80000000u|p);
 return 0;
}
int duck_vita_overlay_free(uint32_t allocation){
 uint32_t p;if(!catalog)return -1;if(!allocation)return 0;
 if(pointer(allocation,0,&p))return -1;
 for(size_t i=0;i<catalog->module_count;i++)if(loaded[i].state==1&&loaded[i].allocation==p){loaded[i].state=2;return 1;}
 return 0; /* A data-only allocation has no native code mapping. */
}
int duck_vita_overlay_reuse(uint32_t allocation,uint32_t size){
 uint32_t p;if(!catalog)return -1;if(!allocation||!size)return 0;
 if(pointer(allocation,size,&p))return -1;
 for(size_t i=0;i<catalog->module_count;i++)if(loaded[i].state==1&&overlaps(p,p+size,loaded[i].base,loaded[i].end))return -1;
 for(size_t i=0;i<catalog->module_count;i++)if(loaded[i].state==2&&overlaps(p,p+size,loaded[i].base,loaded[i].end))remove_module(i);
 return 0;
}
static recomp_func_t *find(const DuckCodeFunction *f,size_t n,uint32_t offset){
 size_t left=0,right=n;
 while(left<right){size_t mid=left+(right-left)/2;if(f[mid].offset<offset)left=mid+1;else right=mid;}
 return left<n&&f[left].offset==offset?f[left].function:NULL;
}
recomp_func_t *duck_vita_overlay_find(uint32_t pc){
 uint32_t p;if(!catalog||pointer(pc,4,&p))return NULL;
 unsigned slot=((p>>2)^(p>>12))&1023;
 if(dispatch_cache[slot].function&&dispatch_cache[slot].pc==p)return dispatch_cache[slot].function;
 recomp_func_t *f=find(catalog->resident,catalog->resident_count,0x80000000u|p);
 if(!f)for(size_t i=0;i<catalog->module_count;i++)if(loaded[i].state&&p>=loaded[i].base&&p<loaded[i].end){
  f=find(catalog->modules[i].functions,catalog->modules[i].function_count,p-loaded[i].base);break;
 }
 if(f){dispatch_cache[slot].pc=p;dispatch_cache[slot].function=f;}
 return f;
}
recomp_func_t *get_function(int32_t pc){
 recomp_func_t *f=duck_vita_overlay_find((uint32_t)pc);
 if(!f){fprintf(stderr,"[vita] unmapped guest PC %08x\n",(uint32_t)pc);abort();}return f;
}
static uint32_t rd(uint32_t p){uint32_t v;memcpy(&v,ram+p,4);return v;}
void duck_register_loaded_module(uint8_t *r,recomp_context *ctx){
 uint32_t sp;
 if(r!=ram||!ctx||pointer((uint32_t)ctx->r29,0x70,&sp))abort();
 uint64_t initialized=(uint64_t)rd(sp+0x50)+rd(sp+0x54)+rd(sp+0x58);
 if(initialized>UINT32_MAX||duck_vita_overlay_load(rd(sp+0x64),(uint32_t)ctx->r17,rd(sp+0x6c),(uint32_t)initialized,rd(sp+0x5c)))abort();
}
void duck_before_free(uint8_t *r,recomp_context *ctx){
 if(r!=ram||!ctx||duck_vita_overlay_free((uint32_t)ctx->r4)<0)abort();
}
void duck_vita_after_allocation(uint8_t *r,uint32_t allocation,uint32_t size){
 if(r!=ram||duck_vita_overlay_reuse(allocation,size))abort();
}
