// Compile twice against reference and patched RSP headers, with distinct
// DUCK_MEMORY_TEST_NAME symbols. Returns logical register/DMEM snapshots.
#include "librecomp/rsp.hpp"
#include "librecomp/rsp_vu_impl.hpp"
#ifndef DUCK_MEMORY_TEST_NAME
#define DUCK_MEMORY_TEST_NAME duck_rsp_memory_case
#endif
template<unsigned E>static unsigned exercise(unsigned address,unsigned op,uint8_t *out){
 RSP rsp{};
 for(unsigned i=0;i<16;i++)rsp.vpu.r[0].byte(i)=uint8_t(i*17+address);
 for(unsigned i=0;i<4096;i++)dmem[i]=uint8_t(i*29+(i>>4)*7);
 // Include negative offsets, high address bits and a 32-bit wrap at address 0.
 switch(op){
 case 0:rsp.LQV<E>(rsp.vpu.r[0],address,0);break;
 case 1:rsp.SQV<E>(rsp.vpu.r[0],address,0);break;
 case 2:rsp.LDV<E>(rsp.vpu.r[0],address,0);break;
 case 3:rsp.SDV<E>(rsp.vpu.r[0],address,0);break;
 case 4:rsp.LQV<E>(rsp.vpu.r[0],address,-1);break;
 case 5:rsp.SQV<E>(rsp.vpu.r[0],address,-1);break;
 case 6:rsp.LDV<E>(rsp.vpu.r[0],address,-128);break;
 case 7:rsp.SDV<E>(rsp.vpu.r[0],address,127);break;
 }
 for(unsigned i=0;i<16;i++)out[i]=rsp.vpu.r[0].byte(i);
 for(unsigned i=0;i<4096;i++)out[16+i]=dmem[i];
 return 4112;
}
extern "C" unsigned DUCK_MEMORY_TEST_NAME(unsigned element,unsigned address,unsigned op,uint8_t *out){
 switch(element){
#define CASE(e) case e:return exercise<e>(address,op,out)
 CASE(0);CASE(1);CASE(2);CASE(3);CASE(4);CASE(5);CASE(6);CASE(7);
 CASE(8);CASE(9);CASE(10);CASE(11);CASE(12);CASE(13);CASE(14);CASE(15);
#undef CASE
 }
 return 0;
}
