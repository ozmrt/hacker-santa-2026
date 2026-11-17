#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "gui/round_demo.h"
static uint16_t reference[240*240], strip[240*5+2];
int main(void) {
    const uint32_t times[]={0,1,999,123456,UINT32_MAX,0x80000000};
    const uint64_t golden[]={UINT64_C(0x5b5f5c88e28326e5),UINT64_C(0x5b5f5c88e28326e5),UINT64_C(0x6b9899576e7ed6dd),UINT64_C(0xc0f9f1f641aabb6d),UINT64_C(0x0b112740592e86bd),UINT64_C(0x5b5f5c88e28326e5)};
    for(unsigned t=0;t<sizeof(times)/sizeof(*times);t++) {
        round_demo_render(reference,240,0,0,239,239,times[t],false);
        uint64_t hash=UINT64_C(14695981039346656037);
        for(unsigned i=0;i<240*240;i++) {
            hash=(hash^(reference[i]&255))*UINT64_C(1099511628211);
            hash=(hash^(reference[i]>>8))*UINT64_C(1099511628211);
        }
        assert(hash==golden[t]); /* Match the original signed-division kernel. */
        for(int y=0;y<240;y+=5) {
            memset(strip,0xa5,sizeof(strip));
            round_demo_render(strip+1,240,0,y,239,y+4,times[t],false);
            assert(strip[0]==0xa5a5 && strip[1201]==0xa5a5);
            assert(memcmp(strip+1,reference+y*240,240*5*2)==0);
        }
        /* Odd origins and padded strides must match a full-frame render. */
        for(int y=1;y<235;y+=11) for(int x=1;x<235;x+=13) {
            memset(strip,0xa5,sizeof(strip));
            round_demo_render(strip+1,19,x,y,x+16,y+2,times[t],true);
            for(int j=0;j<3;j++) for(int i=0;i<17;i++) {
                uint16_t c=reference[(y+j)*240+x+i];
                assert(strip[1+j*19+i]==(uint16_t)((c<<8)|(c>>8)));
            }
            assert(strip[18]==0xa5a5 && strip[19]==0xa5a5 && strip[58]==0xa5a5);
        }
    }
    memset(strip,0xa5,sizeof(strip));
    round_demo_render(strip,1,-1,0,239,1,0,false);
    for(unsigned i=0;i<sizeof(strip)/sizeof(*strip);i++) assert(strip[i]==0xa5a5);
    puts("Raster: exact strip/partial redraw equivalence, byte order and canaries passed");
}
