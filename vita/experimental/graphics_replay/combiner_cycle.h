#pragma once
#include <cstdint>
// The interpreter stores two packed 28-bit mux sets. N64 one-cycle rendering
// uses the SECOND set; two-cycle rendering evaluates both in order. Keep the
// register untouched and normalize only the shader key. COPY/FILL use their
// existing synthetic first-set combiners instead of the hardware mux.
inline uint64_t duck_combiner_shader_mux(uint64_t packed,bool oneCycle){
    return oneCycle?(packed>>28)&0x0fffffffull:packed;
}
