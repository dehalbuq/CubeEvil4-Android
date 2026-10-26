// Property tests for the R36S GX display-list memcpy fast path.
// Compare actual optimized helper against the ORIGINAL byte-wise algorithm.
// No commercial game data. Compile using host g++ or AArch64 clang/QEMU.
#include "gx_fifo_fast_copy.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <algorithm>

using u8=std::uint8_t;
using u32=std::uint32_t;
struct Storage {
    std::array<u8,1024> bytes{};
    // Each physical range is valid/contiguous; 640..703 is intentionally
    // unmapped to model the end of a host alias window.
    u8* map(u32 address,u32* available) {
        *available=0;
        if(address<256){*available=256-address;return bytes.data()+address;}
        if(address<512){*available=512-address;return bytes.data()+address;}
        if(address<640){*available=640-address;return bytes.data()+address;}
        if(address>=704 && address<864){
            *available=864-address;return bytes.data()+address;
        }
        if(address>=864 && address<1024){
            *available=1024-address;return bytes.data()+address;
        }
        return nullptr;
    }
    u8* byte(u32 address) {
        u32 span=0;
        return map(address,&span);
    }
};

bool original(Storage& store,u32& pos,u32 end,const u8* data,int n) {
    for(int i=0;i<n;i++){
        u8* p=pos<end ? store.byte(pos) : nullptr;
        if(!p)return false;
        *p=data[i];
        pos++;
    }
    return true;
}
bool optimized(Storage& store,u32& pos,u32 end,const u8* data,int n) {
    return re4gx_fastfifo::copy(pos,end,data,n,
      [&store](u32 address,u32* span){return store.map(address,span);});
}
u32 state=0x71635DA9u;
u32 rnd() {
    state^=state<<13;state^=state>>17;state^=state<<5;
    return state;
}
int main() {
    constexpr int kCases=75000;
    Storage oldmem,newmem;
    std::array<u8,256> payload;
    for(int run=0;run<kCases;run++) {
        for(auto& b:oldmem.bytes)b=static_cast<u8>(rnd());
        newmem.bytes=oldmem.bytes;
        for(auto& b:payload)b=static_cast<u8>(rnd());
        // Exercise contiguous regions, windows, holes and out-of-bounds.
        const u32 start=rnd()%1152;
        const u32 end=rnd()%1180;
        const int n=static_cast<int>(rnd()%257);
        u32 oldpos=start,newpos=start;
        const bool oldok=original(oldmem,oldpos,end,payload.data(),n);
        const bool newok=optimized(newmem,newpos,end,payload.data(),n);
        if(oldok!=newok || oldpos!=newpos || oldmem.bytes!=newmem.bytes) {
            std::cerr<<"FAIL run="<<run<<" start="<<start<<" end="<<end
                     <<" bytes="<<n<<" oldpos="<<oldpos<<" newpos="<<newpos
                     <<" oldok="<<oldok<<" newok="<<newok<<"\n";
            return 1;
        }
    }
    // The GPIO/PI GX write position is stored at 32-byte granularity.
    // Verify that the two implementations also agree after quantization.
    for(u32 start=0;start<1024;start++) {
        oldmem.bytes.fill(0);
        newmem.bytes.fill(0);
        payload.fill(1);
        for(int n: {0,1,2,4,8,16,32,64,128,256}) {
            u32 a=start,b=start;
            bool oldok=original(oldmem,a,1024,payload.data(),n);
            bool newok=optimized(newmem,b,1024,payload.data(),n);
            assert(oldok==newok && a==b && (a&0x03ffffe0u)==(b&0x03ffffe0u));
        }
    }
    std::cout<<"R36S_GX_FAST_FIFO_REFERENCE_TEST_OK CASES="<<kCases
             <<" EXACT_BYTES_AND_OVERFLOW=YES\n";
    return 0;
}
