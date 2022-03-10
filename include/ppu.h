#ifndef PPU_H_
#define PPU_H_

#include "exnes.h"

#ifndef INLINE
#define INLINE static inline
#endif

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

typedef struct PACKED{
    u8 y;
    u8 id;
    u8 att;
    u8 x;
}exnes_oam_t;

INLINE void exnes_ppu_render_oam(exnes_t*nes,uint16_t *line_pixel,int line,u8*pal_buf){
    /*暂时使用全渲染*/
    exnes_oam_t *oambase = (exnes_oam_t*)nes->oam;
    int is8x16 = ((exnes_ppu2000_t*)(&nes->ppu[0]))->sprite_is_8x16;
    int i;
    for(i=0;i<64;i++){
        exnes_oam_t *oam = oambase + i;
        int y = oam->y;
        y -= line;
        int height = -8;
        if(is8x16) height = -16;
        if(y<=0&&y>height){
            /*正在渲染线中*/
            if(i==0&&y+(is8x16?15:7)==0){
                /*精灵0命中*/
                nes->rppu[0x2] |= (1<<6);
            }
            y = -y;     //切换为正数
            int hflip = (oam->att>>6)&1;
            int vflip = (oam->att>>7)&1;
            int hadd = hflip?1:-1;  /*水平翻转*/
            int bg_nofront = (oam->att>>5)&1;
            u8 *curpal = /* PPU_MEM_PTR(0x3f10) + (oam->att&3) * 4; */
                        &nes->pal_ram[0x10 + (oam->att&3) * 4];
            if(vflip)
                y = (is8x16?15:7)-y;    //垂直翻转
            int invram_1000 = ((exnes_ppu2000_t*)(&nes->ppu[0]))->pattab_invram_1000_sprite;
            int tsize = 0x10;
            int oam_id = oam->id;
            if(is8x16){
                invram_1000 = 0;  /*忽略*/
                //tsize = 0x20;
                invram_1000 = oam->id&1;        //奇数为1000 + id*0x20
                oam_id &= ~1;
            }
            u16 addr = invram_1000 * 0x1000 + oam_id * tsize;
            if(y>=8){
                addr += 0x10;
                y -= 8;
            }
            int draw_x = oam->x + (hflip?0:7);

            u8 *tile = PPU_MEM_PTR(addr); /*tile*/
            tile += y*1;        //第y行,一个字节1行
            int t1 = tile[0];
            int t2 = tile[8];
            int xx;
            for(xx=0;xx<8;xx++){
                int color = (t1&1) | ((t2&1)<<1);
                t1 >>= 1;
                t2 >>= 1;
                if(
                    color&&
                    (!bg_nofront||
                        #if 0
                        (line_pixel[draw_x]==0/*背景是透明*/)
                        #else
                        /*bg没有覆盖掉的*/
                        (pal_buf[draw_x]>=0x80)
                        #endif
                        )
                ){
                    u8 c = curpal[color];
                    //line_pixel[draw_x] = nes->pal[c&0x3f];
                    pal_buf[draw_x] = c;
                }
                //hadd为正数,则不翻转,负数,则翻转
                draw_x += hadd;
            }

            oam->id * 0x10;
        }
    }
}

/*新的ppu*/
INLINE void exnes_ppu_render_nametable(exnes_t*nes,uint16_t *line_pixel,int line,u8*pal_buf){
    int map_idx = ((exnes_ppu2000_t*)(&nes->ppu[0]))->name_tab_addr;
    /*获得渲染nametable的数据*/
    int nametableX = nes->PPUSCROLL[0];
    int nametableY = nes->PPUSCROLL[1] + line + ((map_idx&2)?240:0);
    int nametableHigh;//      大于255
    int nametableXX = (nametableX&0xff) >> 3;      // /= 8;
    int nametableYY;                // /= 8;
    u8 *nametableData;
    u8 nametable;
    /*获得pat的数据*/
    int pat_addr = 0x1000*((exnes_ppu2000_t*)&nes->ppu[0])->pattab_invram_1000;
    int name_pat_addr = pat_addr + nametable * 0x10;        /*数据*/
    int nametableY_sub = (nametableY&0x7);  /*每一行为1个字节*/
    name_pat_addr += nametableY_sub;        /*每一行为1个字节*/
    u8 *pat_data_ptr = PPU_MEM_PTR(name_pat_addr);
    u8 pat_d1 = pat_data_ptr[0];
    u8 pat_d2 = pat_data_ptr[8];
    int draw_x;

    nametableYY = (nametableY&0x3ff) >> 3;      // /= 8;
    u16 nametable_addr = 0x2000;     //nametable_base的基址
    if(nes->rom_header->vmirror){
        /*垂直镜像
            A B
            A B
        */
        if(nametableYY>=60){
            nametableYY -= 60;
            nametable_addr = 0x2000;
            nametableY -= 480;
            //如果超过480高度,则取镜像0
        }
        else if(nametableYY>=30){
            nametableYY -= 30;
            nametable_addr = 0x2000;
            nametableY -= 240;
        }
    }
    else{
        /*水平镜像
            A A
            B B
        */
        if(nametableYY>=0x60){
            nametableYY -= 0x60;
            nametable_addr = 0x2000;
            //如果超过480高度,则取镜像0
        }
        else if(nametableYY>=0x30){
            nametableYY -= 0x30;
            nametable_addr = 0x2400;
            //如果超过240高度,则去镜像1
        }
    }
    int nametable_base = nametable_addr;
    nametable_addr += nametableYY * 0x20;

    for(draw_x=0;draw_x+8<256;){
        nametableHigh =  (nametableX>>8)&1;     //大于255
        //nametableHighy = nametableY>0xf0;     //应该是垂直卷轴
        nametableXX = (nametableX&0xff) >> 3;      // /= 8;
        nametableHigh ^= map_idx&1;           //如果相同,则为2000,否则为2400
        nametableHigh *= 0x400;                 //下一块nametable
        if(!nes->rom_header->vmirror) nametableHigh = 0;        //因为是水平镜像
        nametableXX += nametableHigh;

        //nametableHigh += nametableHighy;

        u16 n_addr = //nametableHigh + nametableXX + nametableYY*0x20;
                        nametable_addr + nametableXX;
        nametableData = //PPU_MEM_PTR(0x2000 + nametableHigh + nametableXX + nametableYY*0x20 /*一行为256像素,32*8块nametable*/);
                        &nes->nametable_ram[(n_addr)&0x7ff];
        nametable = *nametableData;
        name_pat_addr = pat_addr + nametable * 0x10; /*数据*/
        name_pat_addr += nametableY_sub;             /*每一行为1个字节*/
        pat_data_ptr = PPU_MEM_PTR(name_pat_addr);
        pat_d1 = pat_data_ptr[0];
        pat_d2 = pat_data_ptr[8];

        /*
        调色板处理
        注释1:
        1个字节为32*32像素的调色板
        每两位为16*16像素的调色板,如以下
        +-----低2位
        | +---高2位
        +----
        |1|2|   低4位
        +-+-+
        |3|4|   高4位
        +---+

        */
        if(nametableX>256){
            nametableX |= 0x100;
        }

        int pal_addr = nametable_base + 0x03C0;                 //3C0    调色板位置
        pal_addr += nametableHigh;     //27c0   调色板位置
        int pal_Y = (nametableY&0xff)>>5;      //32像素使用同一个调色板
        int pal_Yh = (nametableY>>4) & 1;      //

        int pal_X = (nametableX&0xff)>>5;      //32像素使用同一个调色板
        int pal_Xh = (nametableX>>4) & 1;      //
        pal_addr += pal_X;                     //32像素后为下一个字节
        pal_addr += pal_Y * (0x100/32);        //Y向下32像素为8个字节之后

        u8* pal_dataptr = &nes->nametable_ram[pal_addr&0x7ff];
        u8  pal = pal_dataptr[0];
        pal >>= (pal_Yh*4) + (pal_Xh*2);       //参考注释1
        pal &= 3;

        /*调色板数据*/
        u8 *bg_pal_ptr = //PPU_MEM_PTR(0x3f00 + pal * 4);
                        &nes->pal_ram[pal*4];
        u16 *color_pal = nes->pal;
        int pat_d;          //pat的数据
        int subx = 0;
        int x_width = 7;
        if(draw_x==0&&(nes->PPUSCROLL[0])!=0){
            /*需要移动卷轴*/
            x_width =  7 - (nametableX&0x7);
            pat_d1 <<= (nametableX&0x7);
            pat_d2 <<= (nametableX&0x7);
        }
        for(;subx<=x_width;subx++){
            pat_d = ((pat_d1&0x80)!=0) | (((pat_d2&0x80)!=0)<<1);
            u8 pixel_pal = bg_pal_ptr[pat_d];
            if(pat_d){
                pal_buf[draw_x+subx] = pixel_pal;
            }
            else{
                //如果是0,则被当成背景色
                pal_buf[draw_x+subx] = pixel_pal + 0x80;
            }
            pat_d1 <<= 1;
            pat_d2 <<= 1;
        }
        draw_x += x_width + 1;
        nametableX += 8;
    }
}

INLINE void exnes_ppu_render(exnes_t*nes,uint16_t *line_pixel,int line){
    //memset(line_pixel,0xff,256*2);
    u8 pal[256];
    memset(pal,0xff,sizeof(pal));
    if(((exnes_ppu2001_t*)(&nes->ppu[0x1]))->bg_display){
        exnes_ppu_render_nametable(nes,line_pixel,line,pal);
    }

    if((((exnes_ppu2001_t*)(&nes->ppu[0x1]))->sprite_display)){
        exnes_ppu_render_oam(nes,line_pixel,line,pal);
    }
    int i = 0;
    for(i=0;i<256;i++){
        int p = pal[i];
        if(p>=0x80){
            p -= 0x80;
            p = nes->pal_ram[0];
        }
        line_pixel[i] = nes->pal[p&0x3f];
    }
}

#endif
