#pragma once
#include <cstdint>
// Freeze the clock supplied to guest timers/VI/Count while a native menu owns
// input. Native audio-device completion and UI timing keep their real clocks.
struct DuckPausedClock {
    bool paused=false;uint64_t begin=0,offset=0;
    uint64_t read(uint64_t raw) const {return (paused?begin:raw)-offset;}
    void set(bool stop,uint64_t raw){
        if(stop&&!paused){begin=raw;paused=true;}
        if(!stop&&paused){offset+=raw-begin;paused=false;}
    }
};
