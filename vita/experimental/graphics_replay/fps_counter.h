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
