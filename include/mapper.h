
#ifndef MAPPER_H_
#define MAPPER_H_

#include <exnes.h>
#include <cpu6502.h>
#include <stdlib.h>

INLINE void mapper2_wmem(exnes_t*nes,u16 addr,u8 val);
INLINE void exnes_mapper_init(exnes_t*nes){
    int i;
    exnes_t *cpu = nes;
    //mapper_02
    if(
        nes->rom_header->mapper==2&&
        nes->rom_header->mapper2==0
    ){
        /*处理映射mapper2*/
        for(i=0x8000;i<0x10000;i+=0x1000){
            nes->wmem_func[MMAP(i)] = mapper2_wmem;
        }

        /*设置最后一页*/
        u8 *rom_bin = nes->rom_bin + (nes->rom_header->PRG_page-1) * 0x4000;
        for(i=0xc000;i<0x10000;i+=0x1000){
            nes->rmem_map[MMAP(i)] = rom_bin;
            rom_bin += 0x1000;
        }

        /*可读写chr*/
        u8 *vram = (u8*)calloc(1,0x2000);
        nes->ppu_mmap[MMAP(0x0000)] = vram + 0x0000;
        nes->ppu_mmap[MMAP(0x1000)] = vram + 0x1000;

        //获得PC
        nes->PC = *(u16*)_RMEM_PTR(0xfffc);
    }
}

INLINE void mapper2_wmem(exnes_t*nes,u16 addr,u8 val){
    int P = val&0x7;
    int p = (val>>3)&1;
    int i;
    u8 *rom_bin = nes->rom_bin + P * 0x4000;
    for(i=0x8000;i<0xc000;i+=0x1000){
        nes->rmem_map[MMAP(i)] = rom_bin;
        rom_bin+= 0x1000;
    }
}

#endif
