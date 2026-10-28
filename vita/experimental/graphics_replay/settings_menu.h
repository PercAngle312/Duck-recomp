#pragma once
#include "graphics_options.h"
#include <cerrno>

struct DuckSettingsMenu {
    enum {Select=1,Start=8,Up=0x10,Right=0x20,Down=0x40,Left=0x80,Circle=0x2000,Cross=0x4000};
    enum Action {None,Opened,Apply,Cancelled};
    bool active=false,holding=false;
    unsigned row=0,previous=0;
    uint64_t hold_start=0;
    DuckGraphicsOptions pending;
    Action poll(unsigned buttons,uint64_t us,const DuckGraphicsOptions &current){
        unsigned pressed=buttons&~previous;previous=buttons;
        if(!active){
            if((buttons&(Select|Start))==Select){
                if(!holding){holding=true;hold_start=us;}
                if(us>=hold_start&&us-hold_start>=600000){
                    active=true;pending=current;row=0;holding=false;return Opened;
                }
            }else holding=false;
            return None;
        }
        if(pressed&(Circle|Select)){active=false;holding=false;return Cancelled;}
        if(pressed&Up)row=(row+7)%8;
        if(pressed&Down)row=(row+1)%8;
        int step=(pressed&Right)?1:(pressed&Left)?-1:0;
        if(step){
            if(row==0)pending.quality=(pending.quality+(step>0?1:2))%3;
            if(row==1)pending.antialias=!pending.antialias;
            if(row==2)pending.filter=(pending.filter+(step>0?1:2))%3;
            if(row==3)pending.widescreen=!pending.widescreen;
            if(row==4)pending.smooth=!pending.smooth;
            if(row==5)pending.live_stats=!pending.live_stats;
            pending.resolve();
        }
        if(pressed&Cross){
            if(row==6)return Apply;
            if(row==7){active=false;return Cancelled;}
        }
        return None;
    }
    void applied(){active=false;holding=false;}
};

// Commit via temporary + backup so a failed rename leaves the old settings.
// The renderer owns these known graphics fields; game saves are separate.
inline bool duck_save_graphics(const char *path,const DuckGraphicsOptions &o){
    char temp[512],backup[512];
    if(std::snprintf(temp,sizeof temp,"%s.tmp",path)>=int(sizeof temp)||
       std::snprintf(backup,sizeof backup,"%s.bak",path)>=int(sizeof backup))return false;
    FILE *f=std::fopen(temp,"w");if(!f)return false;
    int written=std::fprintf(f,"aspect=%s\nquality=%s\nfilter=%s\npresentation=%s\nborders=%s\naa=%s\nstats=%s\n",
        o.widescreen?"wide":"original",o.quality==0?"native":o.quality==1?"enhanced":"display",
        o.filter==0?"n64":o.filter==1?"bilinear":"nearest",o.smooth?"smooth":"sharp",
        o.edge_borders?"edge":"original",o.antialias?"edge":"off",o.live_stats?"on":"off");
    bool ok=written>0&&std::fflush(f)==0&&!std::ferror(f);
    if(std::fclose(f))ok=false;
    if(!ok){std::remove(temp);return false;}
    bool old=false;f=std::fopen(path,"r");
    if(!f&&errno!=ENOENT){std::remove(temp);return false;}
    if(f){old=true;if(std::fclose(f)){std::remove(temp);return false;}
        std::remove(backup);if(std::rename(path,backup)){std::remove(temp);return false;}}
    if(std::rename(temp,path)){if(old)std::rename(backup,path);std::remove(temp);return false;}
    return true;
}
