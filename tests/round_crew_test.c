#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "gui/round_demo.h"
#include "gui/round_expand.h"
static uint16_t full[240*240],strip[1202];
int main(void) {
    uint8_t source[256],out[2052];uint16_t pal[16];
    for(unsigned i=0;i<256;i++) source[i]=(uint8_t)i;
    for(unsigned i=0;i<16;i++) pal[i]=(uint16_t)(i*4079+31);
    void (*kernels[])(void *,const uint8_t *,size_t,const uint16_t *)={round_expand4_c,round_expand4_os,round_expand4_o2,round_expand4_iram,round_expand4_asm};
    for(unsigned k=0;k<5;k++) for(unsigned alignment=0;alignment<4;alignment++) {
        memset(out,0xa5,sizeof(out));
        kernels[k](out+alignment,source,256,pal);
        for(unsigned i=0;i<1024;i++) {
            unsigned b=source[i/4],index=(i%4<2)?b>>4:b&15;
            uint16_t color;memcpy(&color,out+alignment+i*2,2);assert(color==pal[index]);
        }
        for(unsigned i=0;i<alignment;i++) assert(out[i]==0xa5);
        for(unsigned i=alignment+2048;i<sizeof(out);i++) assert(out[i]==0xa5);
        kernels[k](NULL,NULL,0,NULL);
    }
    const uint32_t phases[]={0,1999,10000,16384,123456,UINT32_MAX};
    for(unsigned t=0;t<sizeof(phases)/sizeof(*phases);t++) {
        round_crew_render(full,240,0,0,239,239,phases[t],false);
        for(int y=0;y<240;y+=5) {
            memset(strip,0xa5,sizeof(strip));
            round_crew_render(strip+1,240,0,y,239,y+4,phases[t],false);
            assert(strip[0]==0xa5a5 && strip[1201]==0xa5a5);
            assert(!memcmp(strip+1,full+y*240,240*5*2));
        }
        for(int y=1;y<235;y+=17) for(int x=1;x<222;x+=19) {
            memset(strip,0xa5,sizeof(strip));
            round_crew_render(strip+1,19,x,y,x+16,y+2,phases[t],true);
            for(int j=0;j<3;j++) for(int i=0;i<17;i++) {
                uint16_t c=full[(y+j)*240+x+i];
                assert(strip[1+j*19+i]==(uint16_t)((c<<8)|(c>>8)));
            }
            assert(strip[18]==0xa5a5 && strip[19]==0xa5a5 && strip[58]==0xa5a5);
        }
    }
    puts("Copper Dream: packed expansion, alignments, canaries, clipped strips and phases passed");
}
