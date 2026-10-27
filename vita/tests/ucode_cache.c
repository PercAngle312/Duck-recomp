#include "duck_vita_ucode_cache.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 uint8_t ram[8192]={0};DuckVitaUcodeCache cache={0};
 const uint8_t text[]="12345678";const uint32_t expected=0x9ae0daafu;
 for(unsigned i=0;i<8;i++)ram[(0x100+i)^3u]=ram[(0x200+i)^3u]=text[i];
 assert(duck_vita_ucode_matches(&cache,ram,sizeof ram,0x100,8,expected));
 assert(cache.checks==1&&cache.hashes==1&&!cache.hits);
 for(unsigned i=0;i<1000;i++)assert(duck_vita_ucode_matches(&cache,ram,sizeof ram,0x100,8,expected));
 assert(cache.hashes==1&&cache.hits==1000);
 ram[0x100]^=1;assert(!duck_vita_ucode_matches(&cache,ram,sizeof ram,0x100,8,expected));assert(cache.hashes==2&&!cache.valid);
 ram[0x100]^=1;assert(duck_vita_ucode_matches(&cache,ram,sizeof ram,0x100,8,expected)&&cache.hashes==3);
 assert(!duck_vita_ucode_matches(&cache,ram,sizeof ram,0x100,8,expected^1)&&cache.hashes==4);
 assert(duck_vita_ucode_matches(&cache,ram,sizeof ram,0x200,8,expected)&&cache.hashes==5);
 unsigned checks=cache.checks;
 assert(!duck_vita_ucode_matches(&cache,ram,sizeof ram,8192,8,expected));
 assert(!duck_vita_ucode_matches(&cache,ram,sizeof ram,8193,8,expected));
 assert(!duck_vita_ucode_matches(&cache,ram,sizeof ram,0x100,4100,expected));
 assert(!duck_vita_ucode_matches(&cache,ram,sizeof ram,0x101,8,expected));
 assert(!duck_vita_ucode_matches(&cache,ram,sizeof ram,0x100,7,expected));
 assert(cache.checks==checks);
 puts("PASS: exact live-byte microcode identity, one CRC for repeated tasks, mutation/revision rejection and bounded word-swapped reads");
}
