#include "duck_vita_ai.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static void pattern(int16_t *p,unsigned n){for(unsigned i=0;i<n;i++){p[2*i]=12000;p[2*i+1]=-12000;}}
int main(void){
 int16_t p[128],original[128];DuckAudioGapFade state={0};
 pattern(p,64);memcpy(original,p,sizeof p);
 for(unsigned i=0;i<10;i++){duck_vita_audio_gap_fade(&state,p,64,64);assert(!memcmp(p,original,sizeof p)&&!state.resume);}
 // Silence is already owned by the caller. Taper only real samples.
 memset(p+48*2,0,16*2*sizeof *p);duck_vita_audio_gap_fade(&state,p,48,64);
 assert(state.resume==16);
 for(unsigned i=0;i<64;i++){
  int expected=i<32?12000:i<48?12000*(47-(int)i)/15:0;
  assert(p[2*i]==expected&&p[2*i+1]==-expected);
 }
 // Resume across small blocks; ramp position must carry correctly.
 pattern(p,64);duck_vita_audio_gap_fade(&state,p,8,8);assert(state.resume==8);
 for(unsigned i=0;i<8;i++)assert(p[2*i]==12000*(int)i/15&&p[2*i+1]==-p[2*i]);
 pattern(p,64);duck_vita_audio_gap_fade(&state,p,64,64);assert(!state.resume);
 for(unsigned i=0;i<64;i++)assert(p[2*i]==(i<8?12000*(int)(i+8)/15:12000)&&p[2*i+1]==-p[2*i]);
 for(unsigned n=1;n<64;n++){
  state.resume=0;pattern(p,64);duck_vita_audio_gap_fade(&state,p,n,64);
  assert(!p[(n-1)*2]&&!p[(n-1)*2+1]&&state.resume==16);
  for(unsigned i=0;i<n;i++)assert(p[2*i]>=0&&p[2*i]<=12000&&p[2*i+1]==-p[2*i]);
 }
 puts("PASS audio gap ramps: continuous PCM unchanged, fade endpoints, resume carry and 1..63 real frames");
}
