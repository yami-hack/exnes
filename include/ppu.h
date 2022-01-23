#ifndef PPU_H_
#define PPU_H_

#include "exnes.h"

#ifndef INLINE
#define INLINE static inline
#endif

INLINE u8 _get_map_att(exnes_t*nes,int x,int y){
    int map_idx = ((exnes_ppu2000_t*)(&nes->ppu[0]))->name_tab_addr;
    u8 *map = PPU_MEM_PTR(0x2000+map_idx*0x400);
    u8 *map_att = map + 0x3c0;  /*一般在map结尾*/

    x &= 0xff;
    y &= 0xff;
    map_att += (y/32)*0x8;      /**/
    map_att += (x/32);          /*32x32像素*/
    int tx = (x/16) & 1;        /*16*16使用一个调色板*/
    int ty = (y/16) & 1;        /*下半部分*/
    u8 att = map_att[0];
    att >>= (ty*4);
    return (att>>(tx*2)) & 0x3;
}

INLINE u8 _get_map(exnes_t*nes,int x,int y){
    int map_idx = ((exnes_ppu2000_t*)(&nes->ppu[0]))->name_tab_addr;
    u8 *map = PPU_MEM_PTR(0x2000+map_idx*0x400);
    u8 *map_att = map + 0x3c0;  /*一般在map结尾*/

    x &= 0xff;
    y &= 0xff;
    x /= 0x8;
    y /= 0x8;
    map += y*0x20;
    map += x;
    return *map;
}

/*调色板颜色都是固定的*/
INLINE void exnes_vram_render(exnes_t*nes,uint16_t *line_pixel,int line){
    u8 *bg_pal = PPU_MEM_PTR(0x3f00);
    u8 *sprite_pal = PPU_MEM_PTR(0x3f10);
    //命名表
    //命名表的位置(应该使用map来命名)

    //pattern表
    //应该命名为(tile)
    u8 *tile = PPU_MEM_PTR(0x0000);

    //绘制的y坐标
    int y = line + nes->PPUSCROLL[1];
    int x = nes->PPUSCROLL[0];
    int y_mask = 0x1ff;  /*一般最大是512*/
    int x_mask = 0x1ff;  /*一般最大是512*/
    int draw_x = 0;
    int draw_width = nes->screen_width;
    for(;draw_x<draw_width;draw_x += 8){
        u8 pal = _get_map_att(nes,x,y);
        u8 *curpal = bg_pal + pal * 4;
        int xx;
        u8 m = _get_map(nes,x,y);

        u16 tile_data  = tile_data + 0x10 * m;
        u16 tile_data2 = tile_data + 0x10 * m + 0x8;
        tile_data  = nes->tileTable[tile_data];
        tile_data2 = nes->tileTable[tile_data2];
        tile_data += tile_data2<<1;             /*tile一行的数据*/
        for(xx=0;xx<8;xx++){
            int pixel_data = tile_data & 0x3;
            tile_data >>= 2;
            if(pixel_data){
                /*不是透明颜色*/
                line_pixel[draw_x] = nes->pal[curpal[pixel_data]];
            }
            draw_x++;
            x++;
        }
    }

    /*绘制精灵*/
    
}


#endif
