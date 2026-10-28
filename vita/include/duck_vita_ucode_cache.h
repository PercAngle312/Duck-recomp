#pragma once
#include <stdint.h>
#include <string.h>
/* Host-thread cache of an exact, word-swapped microcode byte sequence.
 * Every hit compares live bytes; pointer equality alone never authorizes code.
 * Changed content must pass the expected CRC before its native function runs. */
typedef struct {
 uint8_t bytes[4096];
 uint32_t address,length,expected_crc,checks,hashes,hits;
 int valid;
} DuckVitaUcodeCache;
static inline int duck_vita_ucode_matches(DuckVitaUcodeCache *cache,const uint8_t *ram,
 uint32_t ram_size,uint32_t address,uint32_t length,uint32_t expected_crc){
 if(!cache||!ram||!length||length>sizeof cache->bytes||(address&3u)||(length&3u)||
    address>ram_size||length>ram_size-address)return 0;
 cache->checks++;
 if(cache->valid&&cache->address==address&&cache->length==length&&
    cache->expected_crc==expected_crc&&!memcmp(cache->bytes,ram+address,length)){
  cache->hits++;return 1;
 }
 cache->hashes++;uint32_t crc=~0u;
 for(uint32_t i=0;i<length;i++){
  crc^=ram[(address+i)^3u];
  for(unsigned bit=0;bit<8;bit++)crc=(crc>>1)^((crc&1)?0xedb88320u:0);
 }
 if(~crc!=expected_crc){cache->valid=0;return 0;}
 memcpy(cache->bytes,ram+address,length);cache->address=address;
 cache->length=length;cache->expected_crc=expected_crc;cache->valid=1;return 1;
}
