// R36S/A35 CPU FIFO display-list write helper.
// This is *not* a GPU draw filter. The memory mapping callback supplies
// exactly the contiguous bytes backed by GameCube MEM1 or an aliased range.
// Return false on the first invalid byte, just like the baseline byte loop.
#pragma once
#ifndef RE4_GX_FIFO_FAST_COPY_H
#define RE4_GX_FIFO_FAST_COPY_H
#include <cstdint>
#include <cstring>
namespace re4gx_fastfifo {
template<class MapSpan>
inline bool copy(std::uint32_t& write, std::uint32_t end,
                 const std::uint8_t* source, int n, MapSpan map) {
    int left=n;
    while(left>0) {
        if(write>=end)return false;
        std::uint32_t span=0;
        std::uint8_t* dest=map(write,&span);
        if(!dest || !span)return false;
        std::uint32_t amount=static_cast<std::uint32_t>(left);
        if(amount>span)amount=span;
        if(amount>end-write)amount=end-write;
        std::memcpy(dest,source,amount);
        source+=amount;
        left-=static_cast<int>(amount);
        write+=amount;
    }
    return true;
}
}
#endif
