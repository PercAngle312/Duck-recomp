#include "graphics_options.h"
#include <cassert>
#include <cmath>
int main(){
    DuckGraphicsOptions o;
    assert(o.width==320&&o.height==240&&o.display_x==117&&o.aspect_delta==1);
    assert(o.option("aspect","wide")&&o.width==424&&o.display_width==960&&o.display_x==0);
    assert(std::fabs(o.aspect_delta-0.75555556f)<0.00001f);
    // Equal-size objects keep their physical screen proportions after the
    // widened clip-space projection and the final display transform.
    assert(std::fabs(o.aspect_delta*960/544-4.0f/3.0f)<0.00001f);
    assert(o.option("quality","enhanced")&&o.width==848&&o.height==480);
    assert(o.option("quality","display")&&o.width==960&&o.height==544);
    assert(o.option("aspect","original")&&o.width==725&&o.display_x==117);
    assert(!o.option("quality","999999")&&o.quality==2);
    assert(!o.option("aspect","stretch")&&!o.widescreen);
    assert(o.option("presentation","smooth")&&o.smooth);
    assert(o.option("presentation","sharp")&&!o.smooth);
    assert(o.option("borders","original")&&!o.edge_borders);
    assert(o.option("borders","edge")&&o.edge_borders);
    assert(!o.option("borders","off")&&o.edge_borders);
    int left=0,right=424;duck_graphics_readback_crop(left,right,0.75555556f);
    assert(left==52&&right==372);
    left=117;right=842;duck_graphics_readback_crop(left,right,1);assert(left==117&&right==842);
    left=0;right=848;duck_graphics_readback_crop(left,right,0.75555556f);assert(left==103&&right==744);
    FILE *f=std::tmpfile();assert(f);
    std::fputs("# graphics\naspect = wide\nquality=enhanced\nfilter=bilinear\nquality=invalid\n",f);
    for(int i=0;i<300;i++)std::fputc('x',f);
    std::fputs("aspect=original\nfilter=nearest\n",f);std::rewind(f);o.load(f);std::fclose(f);
    assert(o.widescreen&&o.width==848&&o.height==480&&o.filter==2);
    const float delta=0.75555556f;
    int edge=duck_widescreen_border_anchor(0,0,7*4,239*4,320,240,delta,true);
    assert(edge==-1);
    float shift=duck_widescreen_border_shift(edge,delta);
    assert(std::fabs(-delta+shift+1.0f)<0.00001f);
    assert(std::fabs(((-1.0f+8.0f/160)*delta+shift)-(-1.0f+8.0f/160*delta))<0.00001f);
    edge=duck_widescreen_border_anchor(312*4,0,319*4,239*4,320,240,delta,true);
    assert(edge==1&&std::fabs(delta+duck_widescreen_border_shift(edge,delta)-1)<0.00001f);
    assert(!duck_widescreen_border_anchor(0,0,7*4,239*4,320,240,1,true)); // 4:3
    assert(!duck_widescreen_border_anchor(0,0,7*4,239*4,320,240,delta,false)); // textured/nonblack/offscreen
    assert(!duck_widescreen_border_anchor(0,0,319*4,239*4,320,240,delta,true)); // full-screen fade
    assert(duck_widescreen_border_anchor(0,0,319*4,7*4,320,240,delta,true)==-2);
    // Actual FILLRECT bounds recovered from the private first-task RAM capture.
    assert(duck_widescreen_border_anchor(0,17*4,23*4,222*4,320,240,delta,true)==-1);
    assert(duck_widescreen_border_anchor(295*4,17*4,319*4,222*4,320,240,delta,true)==1);
    assert(duck_widescreen_border_anchor(0,0,319*4,17*4,320,240,delta,true)==-2);
    assert(duck_widescreen_border_anchor(0,221*4,319*4,239*4,320,240,delta,true)==2);
    assert(!duck_widescreen_border_anchor(0,17*4,23*4,222*4,320,240,1,true));
    assert(!duck_widescreen_border_anchor(0,17*4,23*4,222*4,320,240,delta,false));
    assert(duck_widescreen_border_shift(-2,delta)==0);
    assert(!duck_widescreen_border_anchor(0,20*4,7*4,80*4,320,240,delta,true)); // HUD panel
    assert(!duck_widescreen_border_anchor(120*4,0,128*4,239*4,320,240,delta,true)); // central quad
    assert(!duck_widescreen_border_anchor(0,0,100*4,239*4,320,240,delta,true)); // wide panel
    assert(!duck_widescreen_border_anchor(0,0,7*4,239*4,0,240,delta,true));
    std::puts("PASS graphics options, bounded presets, projection ratio, guest readback crop and malformed input");
}
