#pragma once
#include <cstdint>
#include <vector>

// Freeing a guest allocation does not erase its machine code. Destructors and
// callbacks can still execute it until another module replaces those bytes.
// Calls are serialized by the host loader's mutex.
class RetiredCode {
public:
    struct Region { uint32_t id,base,size; };
    void retain(Region region) {regions.push_back(region);}
    template<class Unload>
    void before_load(uint32_t id,uint32_t base,uint32_t size,Unload unload) {
        for(auto it=regions.begin();it!=regions.end();) {
            const bool overlap=uint64_t(base)<uint64_t(it->base)+it->size && uint64_t(base)+size>it->base;
            // The upstream section table has one relocation base per module.
            if(it->id==id || overlap) {unload(*it);it=regions.erase(it);}
            else ++it;
        }
    }
private:
    std::vector<Region> regions;
};
