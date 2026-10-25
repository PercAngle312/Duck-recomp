#include <cstdio>
#include <cstdlib>
#include "recomp.h"
#include "librecomp/overlays.hpp"
#include "retired_code.h"

static void original(uint8_t*,recomp_context* ctx) {ctx->r2=17;}
static void replacement(uint8_t*,recomp_context* ctx) {ctx->r2=23;}
static void require(bool value) {if(!value)std::abort();}
int main() {
    FuncEntry a[]={{original,0x10,0x20}};
    FuncEntry b[]={{replacement,0x10,0x20}};
    SectionTableEntry sections[]={
        {0x10000000,0x80100000,0x100,a,1,nullptr,0,1},
        {0x10001000,0x80101000,0x80,b,1,nullptr,0,2}};
    recomp::overlays::register_overlays({sections,2,3},{nullptr,0});
    recomp::overlays::init_overlays();
    constexpr uint32_t base=0x80200000;
    load_overlays(0x10000000,static_cast<int32_t>(base),0x100);
    RetiredCode retired;retired.retain({1,base,0x100});
    recomp_context ctx{};
    // Regression: a virtual destructor callback after free still dispatches.
    get_function(static_cast<int32_t>(base+0x10))(nullptr,&ctx);
    require(ctx.r2==17);
    unsigned unloaded=0;
    auto unload=[&](auto r) {unloaded++;unload_overlays(static_cast<int32_t>(r.base),r.size);};
    retired.before_load(2,base+0x100,0x80,unload);
    require(unloaded==0); // Adjacent half-open regions do not replace the code.
    retired.before_load(2,base+0x40,0x80,unload);
    require(unloaded==1 && uint32_t(section_addresses[1])==0x80100000);
    load_overlays(0x10001000,static_cast<int32_t>(base+0x40),0x80);
    get_function(static_cast<int32_t>(base+0x50))(nullptr,&ctx);
    require(ctx.r2==23);
    retired.retain({2,base+0x40,0x80});
    retired.before_load(2,base+0x1000,0x80,unload);
    require(unloaded==2 && uint32_t(section_addresses[2])==0x80101000);
    load_overlays(0x10001000,static_cast<int32_t>(base+0x1000),0x80);
    get_function(static_cast<int32_t>(base+0x1010))(nullptr,&ctx);
    require(ctx.r2==23);
    puts("PASS: callback after free, adjacent ranges, replacement and relocated reload use actual upstream dispatch");
}
