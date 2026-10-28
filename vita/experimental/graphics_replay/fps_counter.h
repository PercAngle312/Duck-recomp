#pragma once
#include <cstdint>
struct DuckFpsCounter {
    uint64_t since=0;unsigned frames=0,value=0,frame_us=0;bool active=false,ready=false;
    void reset(){since=0;frames=value=frame_us=0;active=ready=false;}
    void presented(uint64_t us){
        if(!active){since=us;active=true;return;}
        ++frames;uint64_t elapsed=us-since;
        if(elapsed>=500000){value=unsigned((uint64_t(frames)*1000000+elapsed/2)/elapsed);frame_us=unsigned(elapsed/frames);since=us;frames=0;ready=true;}
    }
};

// Stage wall times; DRAW is renderer submission, READ includes GPU wait.
struct DuckFrameCosts {
    uint64_t previous_end=0,since=0,guest=0,translate=0,draw=0,read=0;
    unsigned count=0,guest_us=0,translate_us=0,draw_us=0,read_us=0;
    void reset(){*this=DuckFrameCosts{};}
    void sample(uint64_t begin,uint64_t translated,uint64_t drawn,uint64_t readback,uint64_t end){
        if(!previous_end){previous_end=end;since=end;return;}
        guest+=begin>=previous_end?begin-previous_end:0;
        translate+=translated-begin;draw+=drawn-translated;read+=readback-drawn;count++;
        previous_end=end;
        if(end-since>=500000){guest_us=unsigned(guest/count);translate_us=unsigned(translate/count);draw_us=unsigned(draw/count);read_us=unsigned(read/count);
            since=end;guest=translate=draw=read=0;count=0;}
    }
};
