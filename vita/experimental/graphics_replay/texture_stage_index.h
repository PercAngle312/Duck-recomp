#pragma once
#include <cstdint>
#include <cstddef>
struct DuckTextureStageKey {
    uint32_t address,size,row_bytes;uint8_t swizzle_group;
    bool operator==(const DuckTextureStageKey &b)const{
        return address==b.address&&size==b.size&&row_bytes==b.row_bytes&&swizzle_group==b.swizzle_group;
    }
    struct Hasher {
        size_t operator()(const DuckTextureStageKey &k)const noexcept{
            uint32_t h=k.address*0x9e3779b1u;
            h=(h^(k.size*0x85ebca6bu))*0xc2b2ae35u;
            return h^(k.row_bytes*0x27d4eb2du)^k.swizzle_group;
        }
    };
};
