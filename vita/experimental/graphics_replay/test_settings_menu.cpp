#include "settings_menu.h"
#include "paused_clock.h"
#include <cassert>
#include <cstdlib>
#include <unistd.h>
int main(){
    DuckGraphicsOptions current;current.option("aspect","wide");DuckSettingsMenu m;
    assert(m.poll(1,0,current)==DuckSettingsMenu::None);
    assert(m.poll(1,599999,current)==DuckSettingsMenu::None&&!m.active);
    assert(m.poll(1,600000,current)==DuckSettingsMenu::Opened&&m.active&&m.pending.width==424);
    m.poll(0,600001,current);m.poll(DuckSettingsMenu::Right,600002,current);
    assert(m.pending.quality==1&&m.pending.width==848&&m.pending.height==480&&current.quality==0);
    m.poll(0,600003,current);m.poll(DuckSettingsMenu::Down,600004,current);
    m.poll(0,600005,current);m.poll(DuckSettingsMenu::Right,600006,current);assert(m.pending.antialias);
    m.poll(0,600007,current);assert(m.poll(DuckSettingsMenu::Circle,600008,current)==DuckSettingsMenu::Cancelled&&!m.active);
    assert(!current.antialias&&current.quality==0);
    DuckSettingsMenu chord;
    chord.poll(9,0,current);chord.poll(9,2000000,current);assert(!chord.active);
    chord.poll(0,2000001,current);chord.poll(1,2000002,current);chord.poll(1,2600002,current);assert(chord.active);
    chord.row=5;chord.poll(0,2600003,current);assert(chord.poll(DuckSettingsMenu::Cross,2600004,current)==DuckSettingsMenu::Apply&&chord.active);
    chord.applied();assert(!chord.active);
    char path[]="/tmp/duck-menu-XXXXXX";int fd=mkstemp(path);assert(fd>=0);assert(!close(fd));
    assert(duck_save_graphics(path,m.pending));DuckGraphicsOptions reloaded;FILE *f=std::fopen(path,"r");assert(f);reloaded.load(f);std::fclose(f);
    assert(reloaded.quality==1&&reloaded.width==848&&reloaded.antialias);
    assert(duck_save_graphics(path,current));f=std::fopen(path,"r");assert(f);reloaded.load(f);std::fclose(f);
    assert(reloaded.quality==0&&!reloaded.antialias);
    assert(!duck_save_graphics("/nonexistent/duck/settings.ini",current));
    char backup[256];std::snprintf(backup,sizeof backup,"%s.bak",path);std::remove(path);std::remove(backup);
    DuckPausedClock clock;assert(clock.read(100)==100);clock.set(true,100);assert(clock.read(10000)==100);
    clock.set(true,20000);assert(clock.read(20000)==100);clock.set(false,20100);assert(clock.read(20100)==100&&clock.read(20101)==101);
    clock.set(true,21000);clock.set(false,22000);assert(clock.read(22001)==1001);
    assert(current.option("aa","edge")&&current.antialias);assert(!current.option("aa","msaa")&&current.antialias);
    assert(current.option("aa","off")&&!current.antialias);
    std::puts("PASS settings gesture, exit chord, edits/cancel/apply, config persistence and guest clock pause");
}
