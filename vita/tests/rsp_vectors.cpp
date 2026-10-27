// ARM32 scalar/NEON differential fixture for the operations used by game audio.
// Logical lane/accumulator/flag serialization avoids comparing struct padding.
#include "librecomp/rsp.hpp"
#include "librecomp/rsp_vu_impl.hpp"
#ifndef DUCK_VECTOR_TEST_NAME
#define DUCK_VECTOR_TEST_NAME duck_rsp_vector_case
#endif
static uint32_t next(uint32_t &s){s=s*1664525u+1013904223u;return s;}
template<unsigned E>static unsigned exercise(uint32_t seed,uint8_t *out){
 const uint16_t edge[]={0,1,0x7fff,0x8000,0xffff,0x7ffe,0x8001,0x4000};
 uint32_t random=seed;RSP rsp{};
 for(unsigned r=0;r<32;r++)for(unsigned lane=0;lane<8;lane++)
  rsp.vpu.r[r].u16(lane)=seed<16?edge[(seed+r+lane)&7]:(uint16_t)next(random);
 for(unsigned lane=0;lane<8;lane++){
  uint32_t high=next(random),low=next(random);
  rsp.accumulatorSet(lane,((uint64_t)high<<32)|low);
  rsp.vpu.vcol.set(lane,next(random)&1);rsp.vpu.vcoh.set(lane,next(random)&1);
  rsp.vpu.vccl.set(lane,next(random)&1);rsp.vpu.vcch.set(lane,next(random)&1);rsp.vpu.vce.set(lane,next(random)&1);
 }
 for(unsigned i=0;i<4096;i++)dmem[i]=(uint8_t)next(random);
 auto *vd=&rsp.vpu.r[seed%3+1];const auto &vs=rsp.vpu.r[1],&vt=rsp.vpu.r[2];
 uint8_t *p=out;
 auto save=[&](){
  for(unsigned lane=0;lane<8;lane++){
   uint16_t v=vd->u16(lane);*p++=v>>8;*p++=(uint8_t)v;
   uint64_t acc=rsp.accumulatorGet(lane);for(unsigned b=0;b<6;b++)*p++=(uint8_t)(acc>>(b*8));
   *p++=rsp.vpu.vcol.get(lane);*p++=rsp.vpu.vcoh.get(lane);*p++=rsp.vpu.vccl.get(lane);*p++=rsp.vpu.vcch.get(lane);*p++=rsp.vpu.vce.get(lane);
  }
 };
#define CHECK_OP(op) rsp.op<E>(*vd,vs,vt);save()
 CHECK_OP(VADDC);CHECK_OP(VADD);CHECK_OP(VSUB);CHECK_OP(VLT);CHECK_OP(VGE);
 CHECK_OP(VAND);CHECK_OP(VXOR);CHECK_OP(VMUDL);CHECK_OP(VMUDM);CHECK_OP(VMUDN);
 CHECK_OP(VMUDH);CHECK_OP(VMULF);CHECK_OP(VMACF);CHECK_OP(VMADM);CHECK_OP(VMADN);CHECK_OP(VMADH);rsp.VSAR<E>(*vd,vs);save();
#undef CHECK_OP
 uint32_t address=next(random)&4095;
 rsp.LDV<E>(*vd,address,-1);save();rsp.LQV<E>(*vd,address,1);save();rsp.LRV<E>(*vd,address,0);save();rsp.LSV<E>(*vd,address,-2);save();
 rsp.SDV<E>(*vd,address,1);rsp.SQV<E>(*vd,address,-1);rsp.SSV<E>(*vd,address,0);
 for(unsigned i=0;i<4096;i++)*p++=dmem[i];
 return (unsigned)(p-out);
}
extern "C" unsigned DUCK_VECTOR_TEST_NAME(unsigned seed,uint8_t *out){
 switch(seed&15){
#define CASE(e) case e:return exercise<e>(seed,out)
 CASE(0);CASE(1);CASE(2);CASE(3);CASE(4);CASE(5);CASE(6);CASE(7);CASE(8);CASE(9);CASE(10);CASE(11);CASE(12);CASE(13);CASE(14);CASE(15);
#undef CASE
 }
 return 0;
}
