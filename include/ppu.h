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
    if(x&0x100){
        //镜像
        map_idx = ~map_idx&1;
    }
    u8 *map = PPU_MEM_PTR(0x2000+map_idx*0x400);
    u8 *map_att = map + 0x3c0;  /*一般在map结尾*/


    x &= 0xff;
    y &= 0xff;
    //每格8x8像素
    x /= 0x8;
    y /= 0x8;
    map += y*0x20;
    map += x;
    return *map;
}

INLINE void exnes_video_setmode(exnes_t*nes,int ispal){
    if(ispal){
        nes->vblank_clock = clock_vblank_pal;
        nes->hblank_clock = clock_hblank_pal;
        nes->fps = FRAME_PAL;
        nes->mode = 1;
        nes->frame_scanlines = SCANLINES_PAL;
    }
    else{
        nes->vblank_clock = clock_vblank_ntsc;
        nes->hblank_clock = clock_hblank_ntsc;
        nes->fps = FRAME_NTFC;
        nes->mode = 0;
        nes->frame_scanlines = SCANLINES_NTSC;
    }
}

INLINE void exnes_ppu_render_name_table(exnes_t*nes,uint16_t *line_pixel,int line){
    u8 *bg_pal = PPU_MEM_PTR(0x3f00);
    u8 *sprite_pal = PPU_MEM_PTR(0x3f10);
    //命名表
    //命名表的位置(应该使用map来命名)

    //pattern表
    //应该命名为(tile)
    ((exnes_ppu2000_t*)&nes->ppu[0])->pattab_invram_1000 = 1;
    int taddr = 0x0000+(0x1000*((exnes_ppu2000_t*)&nes->ppu[0])->pattab_invram_1000);
    u8 *tile = PPU_MEM_PTR(taddr);

    //绘制的y坐标
    int y = line + nes->PPUSCROLL[1];
    int x = nes->PPUSCROLL[0];
    int xoff = x&0x7;
    int y_mask = 0x1ff;  /*一般最大是512*/
    int x_mask = 0x1ff;  /*一般最大是512*/
    int draw_x = 0;
    int draw_width = nes->screen_width;
    u16 pixel_buf[0x200];
    u16 *pixel_ptr = pixel_buf;
    if(xoff==0)pixel_ptr = line_pixel;
    memset(pixel_buf,0,sizeof(pixel_buf));
    for(;draw_x<draw_width;draw_x){
        u8 pal = _get_map_att(nes,x,y);
        u8 *curpal = bg_pal + pal * 4;
        int xx;
        u8 m = _get_map(nes,x,y);

        u16 tile_data  = *(tile + 0x10 * m + (y&0x7/*每一行偏移1个字节*/));
        u16 tile_data2 = *(tile + 0x10 * m + (y&0x7/*每一行偏移1个字节*/) + 0x8);
        // tile_data  = nes->tileTable[tile_data];
        // tile_data2 = nes->tileTable[tile_data2];
        // tile_data += tile_data2<<1;             /*tile一行的数据*/
        for(xx=7;xx>=0;xx--){
            int pixel_data = tile_data&1;
            pixel_data |= (tile_data2&1)<<1;
            tile_data>>=1;
            tile_data2>>=1;
            if(pixel_data){
                /*不是透明颜色*/
                pixel_ptr[draw_x+xx] = nes->pal[curpal[pixel_data]];
            }
            pixel_ptr[draw_x+xx] = nes->pal[curpal[pixel_data]];
            x++;
        }
        draw_x += 8;
    }
    if(xoff!=0){
        /*实现滚动*/
        draw_x = 0;
        while(draw_x<0x100){
            line_pixel[draw_x++] = pixel_buf[xoff++];
        }
    }
}

/*调色板颜色都是固定的*/
INLINE void exnes_ppu_render(exnes_t*nes,uint16_t *line_pixel,int line){
    int draw_x = 0;

    if(((exnes_ppu2001_t*)(&nes->ppu[0x1]))->bg_display){
        exnes_ppu_render_name_table(nes,line_pixel,line);
    }

    if(!(((exnes_ppu2001_t*)(&nes->ppu[0x1]))->sprite_display)){
        return;
    }

    /*绘制精灵*/

    int i=0;
    int is8x16 = ((exnes_ppu2000_t*)(&nes->ppu[0]))->sprite_is_8x16;
    u8 *oam = nes->oam;
    for(i=0;i<64;i++){
        int y =   oam[0+i*4];
        int x =   oam[3+i*4];
        int tid = oam[1+i*4];
        int att = oam[2+i*4];
        u8 pal = att&3;
        int bg_nofront = (att>>5)&1;
        int hflip = (att>>6) & 1;
        int vflip = (att>>7) & 1;
        int hadd = hflip?1:-1;  /*水平翻转*/
        int vadd = vflip?1:-1;
        u8 *curpal = PPU_MEM_PTR(0x3f10) + pal * 4;

        y -= line;
        if(y<=0&&y>-8){
            y = -y;
            if(vflip){
                //垂直翻转
                y = 7-y;
            }
            //刚好在扫描线上
            if(is8x16){
                int bank = tid&1; tid >>= 1;
                u8 *tile = nes->vram + bank * 0x1000 + tid * 0x20; /*tile*/
                tile += y*1;    /*第y行*/
                draw_x=x + (vflip?15:0);

                if(bg_nofront){
                    u32 t1 = tile[0];
                    u32 t2 = tile[8];
                    u32 t3 = tile[0x10];
                    u32 t4 = tile[0x18];
                    t1 = nes->tileTable[t1];
                    t1 |= nes->tileTable[t2]<<1;
                    t1 |= nes->tileTable[t3]<<0x10;
                    t1 |= nes->tileTable[t4]<<0x11;

                    /**/
                    for(t2=0;t2<16;t2++){
                        int color = t1&3;
                        if(color){
                            u8 c = curpal[color];
                            line_pixel[draw_x] = curpal[c];
                            if(i==0){ /* sprite 0 hit */
                                nes->rppu[0x2] |= (1<<6);
                            }
                        }
                        /*是否为水平翻转*/
                        draw_x += hadd;
                    }
                }
            }
            else{
                //类似map
                u16 addr = ((exnes_ppu2000_t*)(&nes->ppu[0]))->pattab_invram_1000_sprite*0x1000 + tid * 0x10;
                u8 *tile = PPU_MEM_PTR(addr); /*tile*/
                draw_x = x + (hflip?0:7);
                tile += y*1;    /*第y行*/
                if(1||bg_nofront){
                    u32 t1 = tile[0];
                    u32 t2 = tile[8];
                    int xx = 0;

                    for(xx=0;xx<8;xx++){
                        int color = t1&1;
                        color |= (t2&1)<<1;
                        t1>>=1;
                        t2>>=1;
                        if(color){
                            u8 c = curpal[color];
                            line_pixel[draw_x] = nes->pal[c&0x3f];
                            if(i==0){ /* sprite 0 hit */
                                nes->rppu[0x2] |= (1<<6);
                            }
                        }
                        /*是否为水平翻转*/
                        draw_x += hadd;
                    }
                }

            }
        }
    }
}


#endif
