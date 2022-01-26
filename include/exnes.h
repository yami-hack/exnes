
#ifndef EXNES_H_
#define EXNES_H_

#include <stdint.h>

#ifndef PACKED
#define PACKED __attribute__ ((packed))
#endif


#ifndef INLINE
#define INLINE static inline
#endif

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;

#define N6502_NLG (1<<7)
#define N6502_VLG (1<<6)
#define N6502_ULG (1<<5)
#define N6502_BLG (1<<4)
#define N6502_DLG (1<<3)
#define N6502_ILG (1<<2)
#define N6502_ZLG (1<<1)
#define N6502_CLG (1<<0)

#ifdef _DEBUG_
/*如果是调试模式,会被优化掉*/
#define REGISTER
#else
#ifndef REGISTER
#define REGISTER register
#endif
#endif

#define DEF_FAST_REG(cpu) \
    /*定义u16是可能遇到溢出状态 tmp是作为比较计算*/ \
    REGISTER i32 tmp = 0; \
    REGISTER u16 A = cpu->A; \
    REGISTER u16 X = cpu->X; \
    REGISTER u16 Y = cpu->Y; \
    REGISTER u16 P = cpu->P; \
    REGISTER u16 SP = cpu->SP; \
    REGISTER u16 PC = cpu->PC; \
    REGISTER u8 *sp_mem = cpu->ram+0x100; \
    /*register*/ u32 state = cpu->cpu_state; /*传地址的指令,不应该使用寄存器变量.但是是inline函数.应该可以传寄存器变量才是*/ \
    /*register*/ u32 write_addr_value; /*高16位为地址,低16位为数据*/

#define SAVE_FAST_REG(cpu) \
    cpu->A = A;  \
    cpu->X = X;  \
    cpu->Y = Y;  \
    cpu->P = P;  \
    cpu->SP = SP; \
    cpu->PC = PC; \
    cpu->cpu_state = state;

/*CPU频率*/
#define CPU_CLOCK_NTSC (17897725 / 10)
#define CPU_CLOCK_PAL  (166260703/100)
#define SCANLINES_NTSC 262
#define SCANLINES_PAL  312
#define FRAME_NTFC 60
#define FRAME_PAL  50
#define VBLANK_LINE_NTSC 240
#define VBLANK_LINE_PAL  239

enum{
    /*一些特殊情况下,会有不同变化*/
    clock_vblank_ntsc = CPU_CLOCK_NTSC / FRAME_NTFC,
    clock_vblank_pal =  CPU_CLOCK_PAL / FRAME_PAL,
    clock_hblank_ntsc = CPU_CLOCK_NTSC / FRAME_NTFC / SCANLINES_NTSC,
    clock_hblank_pal  = CPU_CLOCK_PAL / FRAME_PAL / SCANLINES_PAL,
};

enum{
    video_mode_ntsc,
    video_mode_pal,
};

struct exnes_t;
typedef struct exnes_t{
    u8 A,X,Y;
    u8 P;
    u16 SP,PC;
    u8  *cur_insn;      //单纯是为了处理cpu速度
    u8  *sp_ptr;        //栈

    /*时钟已经被包括在cpu_state*/
    //i32 cycles;

/*
IO操作,
CPU时钟溢出
*/
#define CPU_STATE_IO            (1<<31)
#define CPU_STATE_PPURAM_READ   (1<<30)
#define CPU_STATE_CYCLES        (1<<24)
#define CPU_STATE_CYCLES_MASK   (((CPU_STATE_CYCLES<<1)-1))
#define CPU_STATE_SET_CYCLES_HBLANK (CPU_STATE_CYCLES-(cpu->hblank_clock))
    u32 cpu_state;

    u8 ram[2*1024];
    u8 vram[0x4000];
    u8 oam[0x100];      //oam数据
    i8 oam_addr;        //一般为0
    /*卡带ram*/
    u8 sram[8*1024];

    u8 ppu[0x10];
    u8 rppu[0x10];              //读取ppu
    u8 apu[0x20];               //0x4000
#ifdef _DEBUG_
#define _OLDPC_LOG_MAX_ 0x20
    u16 oldPc[_OLDPC_LOG_MAX_];
    u16 oldPc_set;
#endif
#define PPU_MEM_PTR(OFF) \
    (nes->ppu_mmap[(OFF)>>0xc]+((OFF)&0xfff))
    u8 *ppu_mmap[0x10];         //PPU的内存
    const u8 *rom;              //可能是常量
    u8  error_mem[0x4];

    #define MMAP(addr) (addr>>0xc)
    u8  *mem_map[0x10];
    u16  map_mask[0x10];

    u8  *rmem_map[0x10];
    u16 rmap_mask[0x10];

    /*指向*/
    u8  *input_ptr;
    u8  ppu_vram_ptr[2];
    u16 ppu_write_addr;
    i32 ppu_write_addr_flg;
    i32 ppu_read_flag;

    /*调色板颜色*/
    u16 pal[0x40];

    i32 PPUSCROLL_state;
    i32 PPUSCROLL[2];

    i32 screen_width;

    int mode;   //pal或者ntsc
    int fps;    //帧率
    int hblank_clock;   //水平时钟
    int vblank_clock;   //垂直时钟
    int frame_scanlines;    //一帧所需的扫描线

    int scanline;   //扫描线

    u16 tileTable[0x100];

    //模拟器状态
#define EXNES_QUIT      (1<<0)
#define EXNES_OUT_PC    (1<<1)
#define EXNES_DEBUG_IO  (1<<2)
    int emu_state;

    void* render_data;
    int (*render)(struct exnes_t*nes);
    void* (*get_pixels_line)(struct exnes_t*nes,int line);
    int (*process_other)(struct exnes_t*nes);

#define EXNES_ERRORF(nes,fmt,...) \
    if(nes->errorf)nes->errorf(nes->err_data,fmt,##__VA_ARGS__)
    void* err_data;
    int (*errorf)(void*err_data,const char*fmt,...);
}exnes_t;

typedef struct PACKED{
    u8 name_tab_addr:2; //vram 2000,2400,2800,2c00
    u8 _2007_inc_32:1;
    u8 pattab_invram_1000_sprite:1;
    u8 pattab_invram_1000:1;
    u8 sprite_is_8x16:1;
    u8 ppu_slave:1;         //nes中没有使用
    u8 nmi:1;
}exnes_ppu2000_t;

typedef struct PACKED{
    u8 color_mono:1;
    u8 bg_clip:1;       //隐藏左边8个像素
    u8 sprite_clip:1;   //同上
    u8 bg_display:1;    //bg显示
    u8 sprite_display:1;//精灵显示
    u8 color_att:3;
}exnes_ppu2001_t;

typedef struct PACKED{
    u8 not_used:5;
    u8 lost_prite:1;        //1:一条扫描线超过8个精灵
    u8 sprite_0hit:1;
    u8 vblank_flg:1;        //是否处理垂直中断状态
}exnes_ppu2002_t;

typedef struct PACKED{
    u8 id[4];       //NES
    /*PRG:N*16K,CHR:N*8K*/
    u8 PRG_page;    // N*16K
    u8 CHR_page;    // N*8K
    u8 mapper:4;
    u8 _4screenVam:1;
    u8 patch:1; //512训练器补丁?
    u8 battery:1;   //存在sram设置
    u8 vmirror:1;   //0为水平镜像,1为垂直镜像
    u8 mapper2:4;   //如过0Fh为0可以忽略
    u8 zero:2;
    u8 pc10:1;  //有关街机的ROM
    u8 vs:1;    //有关街机的ROM
    u8 sram_num;        //sram数量数
    u8 zero2;   //09h
    u8 zero3;   //0ah
    u8 zero4;   //0bh
    u8 zero5;   //0ch
    u8 zero6;   //0dh
    u8 zero7;   //0eh
    u8 _0Fh;    //0fh,有关mapper2
}exnes_rom_header_t;

enum{
    /*结构体大小*/
    ppu2000_sizeof = sizeof(exnes_ppu2000_t),
    ppu2001_sizeof = sizeof(exnes_ppu2001_t),
    ppu2002_sizeof = sizeof(exnes_ppu2002_t),
    nes_header_sizeof = sizeof(exnes_rom_header_t),
    exnes_sizeof = sizeof(exnes_t),
};

static const u16 nes_pal[] = {
0x333,0x014,0x006,0x326,0x403,0x503,0x510,0x420,0x320,0x120,0x031,0x040,0x022,0x000,0x000,0x000,
0x555,0x036,0x027,0x407,0x507,0x704,0x700,0x630,0x430,0x140,0x040,0x053,0x044,0x000,0x000,0x000,
0x777,0x357,0x447,0x637,0x707,0x737,0x740,0x750,0x660,0x360,0x070,0x276,0x077,0x000,0x000,0x000,
0x777,0x567,0x657,0x757,0x747,0x755,0x764,0x772,0x773,0x572,0x473,0x276,0x467,0x000,0x000,0x000,

};

INLINE void exnes_ppu_render(exnes_t*nes,uint16_t *line_pixel,int line);
INLINE void exnes_video_setmode(exnes_t*nes,int ispal);

#endif
