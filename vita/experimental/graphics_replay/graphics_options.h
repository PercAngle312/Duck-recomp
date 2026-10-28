#pragma once
#include <cstdio>
#include <cstring>
#include <cstdint>

// Configuration applies only at task boundaries, including the paused menu.
struct DuckGraphicsOptions {
    bool widescreen=false;
    bool smooth=false;
    bool edge_borders=true;
    bool antialias=false;
    unsigned quality=0; // 0 native, 1 enhanced, 2 display
    unsigned filter=0;  // 0 N64 three-point, 1 bilinear, 2 nearest
    unsigned width=320,height=240;
    unsigned display_width=725,display_height=544,display_x=117;
    float aspect_delta=1.0f;
    void resolve(){
        display_width=widescreen?960:725;display_x=widescreen?0:117;
        aspect_delta=widescreen?(4.0f/3.0f)/(960.0f/544.0f):1.0f;
        height=quality==0?240:quality==1?480:544;
        width=widescreen?(quality==0?424:quality==1?848:960):
                         (quality==0?320:quality==1?640:725);
    }
    bool option(const char *key,const char *value){
        if(!std::strcmp(key,"aspect")){
            if(!std::strcmp(value,"wide"))widescreen=true;
            else if(!std::strcmp(value,"original"))widescreen=false;else return false;
        }else if(!std::strcmp(key,"quality")){
            if(!std::strcmp(value,"native"))quality=0;
            else if(!std::strcmp(value,"enhanced"))quality=1;
            else if(!std::strcmp(value,"display"))quality=2;else return false;
        }else if(!std::strcmp(key,"aa")){
            if(!std::strcmp(value,"off"))antialias=false;
            else if(!std::strcmp(value,"edge"))antialias=true;else return false;
        }else if(!std::strcmp(key,"borders")){
            if(!std::strcmp(value,"edge"))edge_borders=true;
            else if(!std::strcmp(value,"original"))edge_borders=false;else return false;
        }else if(!std::strcmp(key,"presentation")){
            if(!std::strcmp(value,"smooth"))smooth=true;
            else if(!std::strcmp(value,"sharp"))smooth=false;else return false;
        }else if(!std::strcmp(key,"filter")){
            if(!std::strcmp(value,"n64"))filter=0;
            else if(!std::strcmp(value,"bilinear"))filter=1;
            else if(!std::strcmp(value,"nearest"))filter=2;else return false;
        }else return false;
        resolve();return true;
    }
    void load(FILE *file){
        if(!file)return;
        char line[256],key[32],value[32],extra;
        while(std::fgets(line,sizeof line,file)){
            // Drain truncated lines rather than parsing their tail as a key.
            if(!std::strchr(line,'\n')&&!std::feof(file)){
                int c;while((c=std::fgetc(file))!=EOF&&c!='\n'){}continue;
            }
            if(line[0]=='#'||line[0]==';')continue;
            if(std::sscanf(line," %31[^= \t] = %31s %c",key,value,&extra)==2)
                if(!option(key,value))std::fprintf(stderr,"Ignored graphics option: %s=%s\n",key,value);
        }
    }
};

// Maintain the original 4:3 center in the guest framebuffer mirror. The extra
// wide-view pixels belong to presentation, not the game's 320x240 RAM layout.
inline void duck_graphics_readback_crop(int &left,int &right,float delta){
    if(delta>0.0f&&delta<1.0f){
        int width=right-left,crop=int(width*delta+0.5f);
        if(crop<1)crop=1;
        if(crop>width)crop=width;
        left+=(width-crop)/2;right=left+crop;
    }
}
extern "C" int duck_fast3d_smooth_presentation(void);
extern "C" int duck_fast3d_edge_borders(void);

// Opaque-black fill-cycle overscan strips. Captured game commands include
// 24/25-pixel side strips at y=17..222, bounded by top/bottom overscan fills.
// +/-1 anchors side strips; +/-2 expands top/bottom strips across the display.
// Inputs use inclusive U10.2 fill bounds. Other quads retain their placement.
inline int duck_widescreen_border_anchor(int32_t left,int32_t top,int32_t right,int32_t bottom,
                                        unsigned width,unsigned height,float delta,bool allowed){
    if(!allowed||!width||!height||!(delta>0.0f&&delta<1.0f))return 0;
    const int64_t w=(int64_t)width*4,h=(int64_t)height*4;
    const int64_t r=(int64_t)right+4,b=(int64_t)bottom+4;
    if(top<0||left<0||b>h+4||r>w+4||r<=left||b<=top)return 0;
    if(r-left<=w/8&&top<=h/8&&b>=h*7/8){
        if(left<=4)return -1;
        if(r>=w-4)return 1;
    }
    if(left<=4&&r>=w-4&&b-top<=h/8){
        if(top<=4)return -2;
        if(b>=h-4)return 2;
    }
    return 0;
}
inline float duck_widescreen_border_shift(int edge,float delta){
    return (edge==-1||edge==1)?edge*(1.0f-delta):0.0f;
}
