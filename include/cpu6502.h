
#ifndef CPU6502_H_
#define CPU6502_H_

#include "exnes.h"

#ifndef INLINE
#define INLINE static inline
#endif

#define MEM(OFF) \
    (cpu->mem_map[(OFF)>>0xc])

#define MEM_MASK(OFF) \
    (cpu->map_mask[(OFF)>>0xc]&(OFF))

#define RMEM_MASK(OFF) \
    (cpu->rmap_mask[(OFF)>>0xc]&(OFF))

#define MEM_PTR(OFF) \
    ((cpu->mem_map[(OFF)>>0xc]) + MEM_MASK(OFF))

#if 0
#define _RMEM_PTR(OFF) \
    ((cpu->rmem_map[(OFF)>>0xc]) + RMEM_MASK(OFF))
#define RMEM_PTR _RMEM_PTR

#else
/*有时候,会读取0x2007数据*/
#define _RMEM_PTR(OFF) \
    ((cpu->rmem_map[(OFF)>>0xc]) + RMEM_MASK(OFF))

INLINE u8* _rmem_io_ptr(exnes_t*cpu,int addr){
    if(unlikely(addr==0x2007)){
        if(!cpu->ppu_read_flag){
            /*第一次读取会读取到0*/
            cpu->ppu_read_flag++;
            return &cpu->error_mem[0];
        }
        exnes_t*nes = cpu;
        u8 *ptr = PPU_MEM_PTR(cpu->ppu_write_addr);
        cpu->ppu_write_addr++;
        return ptr;
    }
    else if(unlikely(addr>=0x4016&&addr<=0x4019)){
        u8 *iptr = &cpu->input[sizeof(cpu->input)-1];
        if(cpu->input_state){
        *iptr = (cpu->input[addr-0x4016]>>(cpu->input_state-1))&1;
        cpu->input_state &= 0x7;        /*如果当前值是8,则变为0,+=1则为1*/
        cpu->input_state += 1;
        }
        else{
            *iptr = 0;
        }
        return iptr;
    }
    return _RMEM_PTR(addr);
}

INLINE u8* _rmem_ptr(exnes_t*cpu,int addr,u32 *state,u32 *read_dat){
    #if USE_MEMMAP_FUNC
    return cpu->rmem_func[addr>>0xc](cpu,addr);
    #else
    *state |= (addr==0x2007)?CPU_STATE_PPURAM_READ:0;
    *read_dat = addr;
    return _rmem_io_ptr(cpu,addr);
    #endif
}
#define RMEM_PTR(OFF) \
    _rmem_ptr(cpu,OFF,&state,&write_addr_value)

#endif

#define ZERO_PAGE(OFF) \
    (cpu->ram[(OFF)&0xff])

#define ZERO_PAGE_PTR(OFF) \
    (&cpu->ram[(OFF)&0xff])

#define ABS_DAT(OFF) \
    (cpu->ram[(OFF)])

#define ABS_PTR(OFF) \
    (cpu->ram + (OFF))

#define INSN_DAT16(OFF) \
    *((u16*)(cpu->cur_insn+(OFF)))
#define INSN_DAT8(OFF) \
    *((u8*)(cpu->cur_insn+(OFF)))

#define INSN_OPR \
    (INSN_DAT8(1))

#define INSN_OPR16 \
    (INSN_DAT16(1))

/* A = A + imm */
#define _OPC_IMM(REG,OPR) \
    REG = OPR(REG,INSN_OPR)

/* A = A + mem[opr+X] */
#define _OPC_ZERO_PAGE(REG,OPR) \
    REG = OPR(REG,(ZERO_PAGE(INSN_OPR)))

/* A = A + mem[(opr+X) &0xff] */
#define _OPC_ZERO_PAGE_X(REG,OPR) \
    REG = OPR(REG,(ZERO_PAGE(INSN_OPR+X)))

/* A = A + mem[opr]*/
#define _OPC_ABS(REG,OPR) \
    REG = OPR(REG,*(u8*)(RMEM_PTR(INSN_OPR16)))

/* A = A + mem[opr+X]*/
#define _OPC_ABS_X(REG,OPR) \
    REG = OPR(REG,*(u8*)(RMEM_PTR(INSN_OPR16+X)))

/* A = A + mem[opr+Y]*/
#define _OPC_ABS_Y(REG,OPR) \
    REG = OPR(REG,*(u8*)(RMEM_PTR(INSN_OPR16+Y)))

/*  A = A + mem[mem[opr+X]]  */
#define _OPC_INDEX_X(REG,OPR) \
    REG = OPR(REG,*(u8*)RMEM_PTR((*(u16*)&ZERO_PAGE(INSN_OPR+X))+0))

/* A = A + mem[mem[opr]+Y]]*/
#define _OPC_INDEX_Y(REG,OPR) \
    REG = OPR(REG,*(u8*)RMEM_PTR((*(u16*)&ZERO_PAGE(INSN_OPR+0))+Y))

#if USE_GNUC_JUMP_TABLE
#define SETV(n)  [0x##n] = &&INSN0x##n
#define SWITCH_INIT static const void*INSN_PTR[0x100] = { \
        SETV(00),SETV(01),SETV(02),SETV(03),SETV(04),SETV(05),SETV(06),SETV(07),SETV(08),SETV(09),SETV(0a),SETV(0b),SETV(0c),SETV(0d),SETV(0e),SETV(0f), \
        SETV(10),SETV(11),SETV(12),SETV(13),SETV(14),SETV(15),SETV(16),SETV(17),SETV(18),SETV(19),SETV(1a),SETV(1b),SETV(1c),SETV(1d),SETV(1e),SETV(1f), \
        SETV(20),SETV(21),SETV(22),SETV(23),SETV(24),SETV(25),SETV(26),SETV(27),SETV(28),SETV(29),SETV(2a),SETV(2b),SETV(2c),SETV(2d),SETV(2e),SETV(2f), \
        SETV(30),SETV(31),SETV(32),SETV(33),SETV(34),SETV(35),SETV(36),SETV(37),SETV(38),SETV(39),SETV(3a),SETV(3b),SETV(3c),SETV(3d),SETV(3e),SETV(3f), \
        SETV(40),SETV(41),SETV(42),SETV(43),SETV(44),SETV(45),SETV(46),SETV(47),SETV(48),SETV(49),SETV(4a),SETV(4b),SETV(4c),SETV(4d),SETV(4e),SETV(4f), \
        SETV(50),SETV(51),SETV(52),SETV(53),SETV(54),SETV(55),SETV(56),SETV(57),SETV(58),SETV(59),SETV(5a),SETV(5b),SETV(5c),SETV(5d),SETV(5e),SETV(5f), \
        SETV(60),SETV(61),SETV(62),SETV(63),SETV(64),SETV(65),SETV(66),SETV(67),SETV(68),SETV(69),SETV(6a),SETV(6b),SETV(6c),SETV(6d),SETV(6e),SETV(6f), \
        SETV(70),SETV(71),SETV(72),SETV(73),SETV(74),SETV(75),SETV(76),SETV(77),SETV(78),SETV(79),SETV(7a),SETV(7b),SETV(7c),SETV(7d),SETV(7e),SETV(7f), \
        SETV(80),SETV(81),SETV(82),SETV(83),SETV(84),SETV(85),SETV(86),SETV(87),SETV(88),SETV(89),SETV(8a),SETV(8b),SETV(8c),SETV(8d),SETV(8e),SETV(8f), \
        SETV(90),SETV(91),SETV(92),SETV(93),SETV(94),SETV(95),SETV(96),SETV(97),SETV(98),SETV(99),SETV(9a),SETV(9b),SETV(9c),SETV(9d),SETV(9e),SETV(9f), \
        SETV(a0),SETV(a1),SETV(a2),SETV(a3),SETV(a4),SETV(a5),SETV(a6),SETV(a7),SETV(a8),SETV(a9),SETV(aa),SETV(ab),SETV(ac),SETV(ad),SETV(ae),SETV(af), \
        SETV(b0),SETV(b1),SETV(b2),SETV(b3),SETV(b4),SETV(b5),SETV(b6),SETV(b7),SETV(b8),SETV(b9),SETV(ba),SETV(bb),SETV(bc),SETV(bd),SETV(be),SETV(bf), \
        SETV(c0),SETV(c1),SETV(c2),SETV(c3),SETV(c4),SETV(c5),SETV(c6),SETV(c7),SETV(c8),SETV(c9),SETV(ca),SETV(cb),SETV(cc),SETV(cd),SETV(ce),SETV(cf), \
        SETV(d0),SETV(d1),SETV(d2),SETV(d3),SETV(d4),SETV(d5),SETV(d6),SETV(d7),SETV(d8),SETV(d9),SETV(da),SETV(db),SETV(dc),SETV(dd),SETV(de),SETV(df), \
        SETV(e0),SETV(e1),SETV(e2),SETV(e3),SETV(e4),SETV(e5),SETV(e6),SETV(e7),SETV(e8),SETV(e9),SETV(ea),SETV(eb),SETV(ec),SETV(ed),SETV(ee),SETV(ef), \
        SETV(f0),SETV(f1),SETV(f2),SETV(f3),SETV(f4),SETV(f5),SETV(f6),SETV(f7),SETV(f8),SETV(f9),SETV(fa),SETV(fb),SETV(fc),SETV(fd),SETV(fe),SETV(ff), \
    };
#define SWITCH(cond) goto *INSN_PTR[cond];
#define CASE(v) INSN##v:
#define BREAK goto INSN_END
#define DEFAULT
#define SWITCH_END INSN_END:{}
#else
#define SWITCH_INIT
#define SWITCH(cond) switch(cond)
#define CASE(v) case (v):
#define BREAK break
#define DEFAULT default:
#define SWITCH_END
#endif

#define DEF_BASE1(name,OPR,DEST_REG,NEXT_OPR,h1,h2,h3,h4,h5,h6,h7,h8) \
    CASE(h1){ /*imm   */_OPC_IMM(DEST_REG,OPR);         NEXT_OPR; }BREAK; \
    CASE(h2){ /*zero  */_OPC_ZERO_PAGE(DEST_REG,OPR);   NEXT_OPR; }BREAK; \
    CASE(h3){ /*zero,x*/_OPC_ZERO_PAGE_X(DEST_REG,OPR); NEXT_OPR; }BREAK; \
    CASE(h4){ /*abs   */_OPC_ABS(DEST_REG,OPR);         NEXT_OPR; }BREAK; \
    CASE(h5){ /*abs, x*/_OPC_ABS_X(DEST_REG,OPR);       NEXT_OPR; }BREAK; \
    CASE(h6){ /*abs, y*/_OPC_ABS_Y(DEST_REG,OPR);       NEXT_OPR; }BREAK; \
    CASE(h7){ /*(d), x*/_OPC_INDEX_X(DEST_REG,OPR);     NEXT_OPR; }BREAK; \
    CASE(h8){ /*(d), y*/_OPC_INDEX_Y(DEST_REG,OPR);     NEXT_OPR; }BREAK;

#define ADC_OPR_NEXT \
    P &= ~N6502_CLG;\
    P |= (A==0x00)?N6502_ZLG:0;   \
    P |= (A&0x80)?N6502_NLG:0;    \
    P |= (tmp&0x100)?N6502_CLG:0; \
    P |= (tmp&0x200)?N6502_CLG:0; \
    A = A & 0Xff;

#define UPDATE_FLAGE_ZNCV(tmp) \
    P &= ~(N6502_ZLG|N6502_NLG|N6502_CLG|N6502_VLG);\
    P |= ((tmp&0xff)==0x00)?N6502_ZLG:0;   \
    P |= (tmp&0x80)?N6502_NLG:0;    \
    P |= (tmp&0x100)?N6502_CLG:0;/*注意sbc和cmp指令*/ \
    P |= (tmp&0x200)?N6502_VLG:0;

#define UPDATE_FLAGE_ZNC(tmp) \
    P &= ~(N6502_ZLG|N6502_NLG|N6502_CLG);\
    P |= ((tmp&0xff)==0x00)?N6502_ZLG:0;   \
    P |= (tmp&0x80)?N6502_NLG:0;    \
    P |= (tmp&0x100)?N6502_CLG:0;/*注意sbc和cmp指令*/ \

#define UPDATE_FLAGE_ZN(tmp) \
    P &= ~(N6502_ZLG|N6502_NLG);\
    P |= ((tmp&0xff)==0x00)?N6502_ZLG:0;   \
    P |= (tmp&0x80)?N6502_NLG:0;    \


#define ADC_OPR(AA,BB) \
    (AA) + (BB) + (P&1)

#define AND_OPR(AA,BB) \
    (AA) & (BB)

#define CMP_OPR(AA,BB) \
    (A) - (BB)

#define CMP_OPR_NEXT \
    P &= ~(N6502_NLG|N6502_ZLG|N6502_CLG); \
    P |= tmp==0?N6502_ZLG:0; \
    P |= tmp&0x80?N6502_NLG:0; \
    P |= tmp>0xff?N6502_CLG:0;

#define DEC_NEXT \
    P &= ~(N6502_NLG|N6502_ZLG);\
    P |= tmp==0?N6502_ZLG:0; \
    P |= tmp&0x80?N6502_NLG:0;

#define SBC_OPR_NEXT \
    P &= ~(N6502_NLG|N6502_ZLG|N6502_CLG); \
    P |= A==0?N6502_ZLG:0; \
    P |= A&0x80?N6502_NLG:0; \
    P |= A>0xff?N6502_CLG:0;

#define EOR_OPR(AA,BB) \
    (AA) ^ (BB)

#define ORA_OPR(AA,BB) \
    (AA) | (BB)

#define SBC_OPR(AA,BB) \
    (AA) + (P&1) - 1 - (BB)

#define LDA_OPR(AA,BB) \
    BB

#define FLAG_UPDATE_NZ(AA) \
    P &= ~(N6502_NLG|N6502_ZLG); \
    P |= AA==0?N6502_ZLG:0; \
    P |= (AA&0x80)?N6502_NLG:0; \
    AA &= 0xff

#define A_FLAG_UPDATE_NZ \
    FLAG_UPDATE_NZ(A)

#define Y_FLAG_UPDATE_NZ \
    FLAG_UPDATE_NZ(Y)

#define X_FLAG_UPDATE_NZ \
    FLAG_UPDATE_NZ(X)

/*
dec zeropage
dec zeropage,x
dec abs
dec abs,x
*/
#define DEF_BASE2(name,opr,NEXT_OPR,h1,h2,h3,h4) \
    CASE(h1){ u8* ptr = &ZERO_PAGE(INSN_DAT8(1)+0);  tmp = *ptr opr 1; *ptr = tmp; tmp&=0xff;/*不更新cflag?*/  ;NEXT_OPR; }BREAK; \
    CASE(h2){ u8* ptr = &ZERO_PAGE(INSN_DAT8(1)+X);  tmp = *ptr opr 1; *ptr = tmp; tmp&=0xff;/*不更新cflag?*/  ;NEXT_OPR; }BREAK; \
    CASE(h3){ u16 addr = INSN_DAT16(1)+0; u8* ptr = RMEM_PTR(addr); tmp = *ptr opr 1;MEM_WRITE(addr,tmp); tmp&=0xff;/*不更新cflag?*/  ;NEXT_OPR; }BREAK; \
    CASE(h4){ u16 addr = INSN_DAT16(1)+X; u8* ptr = RMEM_PTR(addr); tmp = *ptr opr 1;MEM_WRITE(addr,tmp); tmp&=0xff;/*不更新cflag?*/  ;NEXT_OPR; }BREAK;

#define DEF_BASE3(name,opr,NEXT_OPR,h1,h2,h3,h4,h5) \
    CASE(h1){ u8* ptr = &ZERO_PAGE(INSN_DAT8(1)+0); int b0 = *ptr &1; tmp = opr(*ptr); *ptr = tmp;                              ;NEXT_OPR; }BREAK; \
    CASE(h2){ u8* ptr = &ZERO_PAGE(INSN_DAT8(1)+X); int b0 = *ptr &1; tmp = opr(*ptr); *ptr = tmp;                              ;NEXT_OPR; }BREAK; \
    CASE(h3){ u16 addr = INSN_DAT16(1)+0; u8* ptr = RMEM_PTR(addr);int b0 = *ptr &1;    tmp = opr(*ptr); MEM_WRITE(addr,tmp);   ;NEXT_OPR; }BREAK; \
    CASE(h4){ u16 addr = INSN_DAT16(1)+X; u8* ptr = RMEM_PTR(addr);int b0 = *ptr &1;    tmp = opr(*ptr); MEM_WRITE(addr,tmp);   ;NEXT_OPR; }BREAK; \
    CASE(h5){ int b0 = A&1;tmp = opr(A);A = tmp&0xff;NEXT_OPR; }BREAK;

#define ASL_OPR(AA) \
    (AA<<1)

#define LSR_OPR(AA) \
    (AA>>1)

#define ROL_OPR(AA) \
    ((AA<<1)|(P&N6502_CLG))

#define ROR_OPR(AA) \
    ((AA>>1)|((P&N6502_CLG)<<7))

#define LSR_OPR_NEXT \
    tmp |= (b0<<8); \
    CMP_OPR_NEXT

#define UPDATE_PC \
    cpu->cur_insn = RMEM_PTR(PC)

#define UPDATE_SP \
    /*cpu->sp_ptr = MEM_PTR((SP+0x100))*/

#define PUSH(v) \
    sp_mem[SP--] = (v)&0xff;

#define PUSH_nes(nes,v) \
    (nes->ram + 0x100)[nes->SP--] = (v)&0xff;

#define POP() \
    (sp_mem[++SP])

#define POP_dest(dest) \
    /*cpu->sp_ptr++; SP++ dest = *cpu->sp_ptr;*/

/*

*
    add 1 to cycles if page boundary is crossed
**
    add 1 to cycles if branch occurs on same page
    add 2 to cycles if branch occurs to different page
*/

/*第一个字节为长度,第二个字节为cycle*/
static const u8 insn_info[0x200] = {
    1,7, //0x00
    2,6, //0x01
    0,0, //0x02
    0,0, //0x03
    0,0, //0x04
    2,3, //0x05
    2,5, //0x06
    0,0, //0x07
    1,3, //0x08
    2,2, //0x09
    1,2, //0x0a
    0,0, //0x0b
    0,0, //0x0c
    3,4, //0x0d
    3,6, //0x0e
    0,0, //0x0f

    2,2, //0x10
    2,5, //0x11
    0,0, //0x12
    0,0, //0x13
    0,0, //0x14
    2,4, //0x15
    2,6, //0x16
    0,0, //0x17
    1,2, //0x18
    3,4, //0x19
    1,2, //0x1a
    0,0, //0x1b
    0,0, //0x1c
    3,4, //0x1d
    3,7, //0x1e
    0,0, //0x1f

    3,6, //0x20
    2,6, //0x21
    0,0, //0x22
    0,0, //0x23
    2,3, //0x24
    2,3, //0x25
    2,5, //0x26
    0,0, //0x27
    1,4, //0x28
    2,2, //0x29
    1,2, //0x2a
    0,0, //0x2b
    3,4, //0x2c
    3,4, //0x2d
    3,6, //0x2e
    0,0, //0x2f

    2,2, //0x30
    2,5, //0x31
    0,0, //0x32
    0,0, //0x33
    0,0, //0x34
    2,4, //0x35
    2,6, //0x36
    0,0, //0x37
    1,2, //0x38
    3,4, //0x39
    0,0, //0x3a
    0,0, //0x3b
    0,0, //0x3c
    3,4, //0x3d
    3,7, //0x3e
    0,0, //0x3f

    1,6, //0x40
    2,6, //0x41
    0,0, //0x42
    0,0, //0x43
    0,0, //0x44
    2,3, //0x45
    2,5, //0x46
    0,0, //0x47
    1,3, //0x48
    2,2, //0x49
    1,2, //0x4a
    0,0, //0x4b
    3,3, //0x4c
    3,4, //0x4d
    3,6, //0x4e
    0,0, //0x4f

    2,2, //0x50
    2,5, //0x51
    0,0, //0x52
    0,0, //0x53
    0,0, //0x54
    2,4, //0x55
    2,5, //0x56
    0,0, //0x57
    1,2, //0x58
    3,4, //0x59
    0,0, //0x5a
    0,0, //0x5b
    0,0, //0x5c
    3,4, //0x5d
    3,7, //0x5e
    0,0, //0x5f

    1,6, //0x60
    2,6, //0x61
    0,0, //0x62
    0,0, //0x63
    0,0, //0x64
    2,3, //0x65
    2,5, //0x66
    0,0, //0x67
    1,4, //0x68
    2,2, //0x69
    1,2, //0x6a
    0,0, //0x6b
    3,5, //0x6c
    3,4, //0x6d
    3,6, //0x6e
    0,0, //0x6f

    2,2, //0x70
    2,5, //0x71
    0,0, //0x72
    0,0, //0x73
    0,0, //0x74
    2,4, //0x75
    2,6, //0x76
    0,0, //0x77
    1,2, //0x78
    3,4, //0x79
    0,0, //0x7a
    0,0, //0x7b
    0,0, //0x7c
    3,4, //0x7d
    3,7, //0x7e
    0,0, //0x7f

    0,0, //0x80
    2,6, //0x81
    0,0, //0x82
    0,0, //0x83
    2,3, //0x84
    2,3, //0x85
    2,3, //0x86
    0,0, //0x87
    1,2, //0x88
    0,0, //0x89
    1,2, //0x8a
    0,0, //0x8b
    3,4, //0x8c
    3,4, //0x8d
    3,4, //0x8e
    0,0, //0x8f

    2,2, //0x90
    2,6, //0x91
    0,0, //0x92
    0,0, //0x93
    2,4, //0x94
    2,4, //0x95
    2,4, //0x96
    0,0, //0x97
    1,2, //0x98
    3,5, //0x99
    1,2, //0x9a
    0,0, //0x9b
    0,0, //0x9c
    3,5, //0x9d
    0,0, //0x9e
    0,0, //0x9f

    2,2, //0xa0
    2,6, //0xa1
    2,2, //0xa2
    0,0, //0xa3
    2,3, //0xa4
    2,3, //0xa5
    2,3, //0xa6
    0,0, //0xa7
    1,2, //0xa8
    2,2, //0xa9
    1,2, //0xaa
    0,0, //0xab
    3,4, //0xac
    3,4, //0xad
    3,4, //0xae
    0,0, //0xaf

    2,2, //0xb0
    2,5, //0xb1
    0,0, //0xb2
    0,0, //0xb3
    2,4, //0xb4
    2,4, //0xb5
    2,4, //0xb6
    0,0, //0xb7
    1,2, //0xb8
    3,4, //0xb9
    1,2, //0xba
    0,0, //0xbb
    3,4, //0xbc
    3,4, //0xbd
    3,4, //0xbe
    0,0, //0xbf

    2,2, //0xc0
    2,6, //0xc1
    0,0, //0xc2
    0,0, //0xc3
    2,3, //0xc4
    2,3, //0xc5
    2,5, //0xc6
    0,0, //0xc7
    1,2, //0xc8
    2,2, //0xc9
    1,2, //0xca
    0,0, //0xcb
    3,4, //0xcc
    3,4, //0xcd
    3,6, //0xce
    0,0, //0xcf

    2,2, //0xd0
    2,5, //0xd1
    0,0, //0xd2
    0,0, //0xd3
    0,0, //0xd4
    2,4, //0xd5
    2,6, //0xd6
    0,0, //0xd7
    1,2, //0xd8
    3,4, //0xd9
    0,0, //0xda
    0,0, //0xdb
    0,0, //0xdc
    3,4, //0xdd
    3,7, //0xde
    0,0, //0xdf

    2,2, //0xe0
    2,6, //0xe1
    0,0, //0xe2
    0,0, //0xe3
    2,3, //0xe4
    2,3, //0xe5
    2,5, //0xe6
    0,0, //0xe7
    1,2, //0xe8
    2,2, //0xe9
    1,2, //0xea
    0,0, //0xeb
    3,4, //0xec
    3,4, //0xed
    3,6, //0xee
    0,0, //0xef

    2,2, //0xf0
    2,5, //0xf1
    0,0, //0xf2
    0,0, //0xf3
    0,0, //0xf4
    2,4, //0xf5
    2,6, //0xf6
    0,0, //0xf7
    1,2, //0xf8
    3,4, //0xf9
    0,0, //0xfa
    0,0, //0xfb
    0,0, //0xfc
    3,4, //0xfd
    3,7, //0xfe
    0,0, //0xff
};

#define MEM_WRITE(addr,value) \
    exnes_write(cpu,&state,&write_addr_value,addr,value)

INLINE i32 _adc_(exnes_t*nes,u8 a,u8 b,u32 p){
    u32 tmp = a+(p&N6502_CLG)+b;
    i32 tmp2 = (i8)a+(p&N6502_CLG)+(i8)b;
    tmp |= (tmp2>128||tmp2<-127)?0x200:0;
    return tmp;
}

INLINE i32 _and_(exnes_t*nes,u8 a,u8 b,u32 p){
    u32 tmp = a&b;
    return tmp;
}

INLINE i32 _eor_(exnes_t*nes,u8 a,u8 b,u32 p){
    u32 tmp = a^b;
    return tmp;
}

INLINE i32 _ora_(exnes_t*nes,u8 a,u8 b,u32 p){
    u32 tmp = a|b;
    return tmp;
}

INLINE i32 _sbc_(exnes_t*nes,u8 a,u8 b,u32 p){
    /*sbc,cmp指令,需要大于或等于时,设置c位,其他的没有,取反就行了*/
    u32 tmp = a+(p&N6502_CLG)-1-b; tmp ^= 0x100;
    i32 tmp2 = (i8)a-(i8)b;
    tmp &= 0x1ff;
    tmp |= (tmp2<-127||tmp2>127)?0x200:0;
    return tmp;
}

INLINE i32 _cmp_(exnes_t*nes,u8 a,u8 b,u32 p){
    /*sbc,cmp指令,需要大于或等于时,设置c位,其他的没有*/
    u32 tmp = a-b; tmp ^= 0x100;
    tmp &= 0x1ff;
    return tmp;
}

#define DEF_BASE_E1(name,dest_reg,mask,func,h1,h2,h3,h4,h5,h6,h7,h8,NEXT_OPR) \
    CASE(h1){ /*imm   */u8 a=A; u8 b=INSN_DAT8(1);                                              tmp = func(nes,a,b,P); NEXT_OPR(tmp); dest_reg = tmp&mask; }BREAK; \
    CASE(h2){ /*zero  */u8 a=A; u8 b=ZERO_PAGE(INSN_DAT8(1)+0);                                 tmp = func(nes,a,b,P); NEXT_OPR(tmp); dest_reg = tmp&mask; }BREAK; \
    CASE(h3){ /*zero,x*/u8 a=A; u8 b=ZERO_PAGE(INSN_DAT8(1)+X);                                 tmp = func(nes,a,b,P); NEXT_OPR(tmp); dest_reg = tmp&mask; }BREAK; \
    CASE(h4){ /*abs   */u8 a=A; u8 b=*(u8*)RMEM_PTR(INSN_DAT16(1));                             tmp = func(nes,a,b,P); NEXT_OPR(tmp); dest_reg = tmp&mask; }BREAK; \
    CASE(h5){ /*abs, x*/u8 a=A; u8 b=*(u8*)RMEM_PTR(INSN_DAT16(1)+X);                           tmp = func(nes,a,b,P); NEXT_OPR(tmp); dest_reg = tmp&mask; }BREAK; \
    CASE(h6){ /*abs, y*/u8 a=A; u8 b=*(u8*)RMEM_PTR(INSN_DAT16(1)+Y);                           tmp = func(nes,a,b,P); NEXT_OPR(tmp); dest_reg = tmp&mask; }BREAK; \
    CASE(h7){ /*(d, x)*/u8 a=A; u8 b=*(u8*)RMEM_PTR((*(u16*)ZERO_PAGE_PTR(INSN_DAT8(1))+X));    tmp = func(nes,a,b,P); NEXT_OPR(tmp); dest_reg = tmp&mask; }BREAK; \
    CASE(h8){ /*(d), y*/u8 a=A; u8 b=*(u8*)RMEM_PTR((*(u16*)ZERO_PAGE_PTR(INSN_DAT8(1)))+Y);    tmp = func(nes,a,b,P); NEXT_OPR(tmp); dest_reg = tmp&mask; }BREAK;

INLINE int exnes_write(exnes_t*nes,u32 *state,u32 *write_addr_value,uint16_t addr,u8 value){
    exnes_t *cpu = nes;
    #if USE_MEMMAP_FUNC
        *(u8*)nes->wmem_func[addr>>0xc](nes,addr) = value;
        *write_addr_value = (addr<<16) | value;
        *state |= nes->cpu_state;
        nes->cpu_state = 0;
    #else
    *state |= (addr>=0x2000&&addr<0x4020)?CPU_STATE_IO:0;
    #if _DEBUG_
    if(addr>=0x2000&&addr<0x4020&&(nes->emu_state&EXNES_DEBUG_IO)){
        nes->emu_state |= EXNES_OUT_PC;
        EXNES_ERRORF(nes,"%04X:%02x-",addr,value);
    }
    #endif
    *write_addr_value = (addr<<16) | value;
    *(u8*)(MEM_PTR(addr)) = value;
    #endif
    return 0;
}

INLINE int exnes_exec(exnes_t*nes){
    DEF_FAST_REG(nes);
    exnes_t*cpu = nes;
    UPDATE_PC;
    UPDATE_SP;
    state |= CPU_STATE_SET_CYCLES_HBLANK;
    SWITCH_INIT;
    while((nes->emu_state&EXNES_QUIT)==0){
        while(!(state&(CPU_STATE_CYCLES|CPU_STATE_IO|CPU_STATE_PPURAM_READ))){
            /*快速模拟6502*/
            u8 opcode = *cpu->cur_insn;
            u8 insn_cycles = insn_info[(*cpu->cur_insn<<1)+1];
            u8 insn_len    = insn_info[(*cpu->cur_insn<<1)+0];
            state += insn_cycles;

            #ifdef _DEBUG_
            nes->oldPc[nes->oldPc_set&(_OLDPC_LOG_MAX_-1)] = PC;
            nes->oldPc_set++;
            nes->oldPc_set&=_OLDPC_LOG_MAX_-1;
            #endif

            SWITCH (opcode)
            {
                DEF_BASE_E1(adc,A,  0xff,_adc_,0x69,0x65,0x75,0x6d,0x7d,0x79,0x61,0x71,UPDATE_FLAGE_ZNCV)
                DEF_BASE_E1(and,A,  0xff,_and_,0x29,0x25,0x35,0x2d,0x3d,0x39,0x21,0x31,UPDATE_FLAGE_ZN)
                DEF_BASE_E1(cmp,tmp,0xff,_cmp_,0xc9,0xc5,0xd5,0xcd,0xdd,0xd9,0xc1,0xd1,UPDATE_FLAGE_ZNC)
                DEF_BASE_E1(eor,A,  0xff,_eor_,0x49,0x45,0x55,0x4d,0x5d,0x59,0x41,0x51,UPDATE_FLAGE_ZN)
                DEF_BASE_E1(ora,A,  0xff,_ora_,0x09,0x05,0x15,0x0d,0x1d,0x19,0x01,0x11,UPDATE_FLAGE_ZN)
                DEF_BASE_E1(sbc,A,  0xff,_sbc_,0xe9,0xe5,0xf5,0xed,0xfd,0xf9,0xe1,0xf1,UPDATE_FLAGE_ZNCV)
                /*lda*/
                //DEF_BASE1(lda,LDA_OPR,A,A_FLAG_UPDATE_NZ,0xa9,0xa5,0xb5,0xad,0xbd,0xb9,0xa1,0xb1)
                CASE(0xa9){ A = INSN_DAT8(1);                                           A_FLAG_UPDATE_NZ;}BREAK;
                CASE(0xa5){ A = ZERO_PAGE(INSN_DAT8(1));                                A_FLAG_UPDATE_NZ;}BREAK;
                CASE(0xb5){ A = ZERO_PAGE(INSN_DAT8(1)+X);                              A_FLAG_UPDATE_NZ;}BREAK;
                CASE(0xad){ A = *(u8*)RMEM_PTR(INSN_DAT16(1)+0);                        A_FLAG_UPDATE_NZ;}BREAK;
                CASE(0xbd){ A = *(u8*)RMEM_PTR(INSN_DAT16(1)+X);                        A_FLAG_UPDATE_NZ;}BREAK;
                CASE(0xb9){ A = *(u8*)RMEM_PTR(INSN_DAT16(1)+Y);                        A_FLAG_UPDATE_NZ;}BREAK;
                CASE(0xa1){ A = *(u8*)RMEM_PTR((*(u16*)&ZERO_PAGE(INSN_DAT8(1)+X))+0);  A_FLAG_UPDATE_NZ;}BREAK;
                CASE(0xb1){ A = *(u8*)RMEM_PTR((*(u16*)&ZERO_PAGE(INSN_DAT8(1)+0))+Y);; A_FLAG_UPDATE_NZ;}BREAK;

                DEF_BASE2(dec,-,DEC_NEXT,0xc6,0xd6,0xce,0xde); BREAK;
                DEF_BASE2(inc,+,DEC_NEXT,0xe6,0xf6,0xee,0xfe); BREAK;

                DEF_BASE3(asl,ASL_OPR,CMP_OPR_NEXT,0x06,0x16,0x0e,0x1e,0x0a);
                DEF_BASE3(lsr,LSR_OPR,LSR_OPR_NEXT,0x46,0x56,0x4e,0x5e,0x4a);
                DEF_BASE3(rol,ROL_OPR,CMP_OPR_NEXT,0x26,0x36,0x2e,0x3e,0x2a);
                DEF_BASE3(ror,ROR_OPR,LSR_OPR_NEXT,0x66,0x76,0x6e,0x7e,0x6a);

                /*ldx*/
                CASE(0xa2){X=INSN_DAT8(1);                     UPDATE_FLAGE_ZN(X);}BREAK;
                CASE(0xa6){X=ZERO_PAGE(INSN_DAT8(1)+0);        UPDATE_FLAGE_ZN(X);}BREAK;
                CASE(0xb6){X=ZERO_PAGE(INSN_DAT8(1)+Y);        UPDATE_FLAGE_ZN(X);}BREAK;
                CASE(0xae){X=*(u8*)(RMEM_PTR(INSN_DAT16(1)))  ;UPDATE_FLAGE_ZN(X);}BREAK;
                CASE(0xbe){X=*(u8*)(RMEM_PTR(INSN_DAT16(1)+Y));UPDATE_FLAGE_ZN(X);}BREAK;
                /*ldy*/
                CASE(0xa0){Y=INSN_DAT8(1);                     UPDATE_FLAGE_ZN(Y);}BREAK;
                CASE(0xa4){Y=ZERO_PAGE(INSN_DAT8(1)+0);        UPDATE_FLAGE_ZN(Y);}BREAK;
                CASE(0xb4){Y=ZERO_PAGE(INSN_DAT8(1)+X);        UPDATE_FLAGE_ZN(Y);}BREAK;
                CASE(0xac){Y=*(u8*)(RMEM_PTR(INSN_DAT16(1)))  ;UPDATE_FLAGE_ZN(Y);}BREAK;
                CASE(0xbc){Y=*(u8*)(RMEM_PTR(INSN_DAT16(1)+X));UPDATE_FLAGE_ZN(Y);}BREAK;


                /*JSR, pc+1 = PCL,pc+2=PCH*/
                CASE(0x20){PUSH((PC+2)>>8);PUSH((PC+2)>>0)}
                CASE(0x4c){PC= INSN_DAT16(1);UPDATE_PC;insn_len=0;/*为0*/}BREAK;
                CASE(0x6c){u16 addr = INSN_DAT16(1);PC=*(u16*)RMEM_PTR(addr);UPDATE_PC;insn_len=0;/*为0*/}BREAK;

                CASE(0xea){ /*NOP*/ }BREAK;
                CASE(0xca){ X = X - 1;X &= 0xff;X_FLAG_UPDATE_NZ;}BREAK;
                CASE(0x88){ Y = Y - 1;Y &= 0xff;Y_FLAG_UPDATE_NZ;}BREAK;
                CASE(0xe8){ X = X + 1;X &= 0xff;X_FLAG_UPDATE_NZ;}BREAK;
                CASE(0xc8){ Y = Y + 1;Y &= 0xff;Y_FLAG_UPDATE_NZ;}BREAK;

                /*sta*/
                CASE(0x85){ ZERO_PAGE(INSN_DAT8(1)+0)    =A; }BREAK;
                CASE(0x95){ ZERO_PAGE(INSN_DAT8(1)+X)    =A; }BREAK;
                CASE(0x8d){ MEM_WRITE(((INSN_DAT16(1)))  ,A);}BREAK;
                CASE(0x9d){ MEM_WRITE(((INSN_DAT16(1)+X)),A);}BREAK;
                CASE(0x99){ MEM_WRITE(((INSN_DAT16(1)+Y)),A);}BREAK;
                CASE(0x81){ u16 addr = *(u16*)(&ZERO_PAGE(INSN_DAT8(1)+X)); MEM_WRITE(addr,A);}BREAK;
                CASE(0x91){ u16 addr = *(u16*)(&ZERO_PAGE(INSN_DAT8(1)+0)); MEM_WRITE(addr+Y,A);}BREAK;
                /*stx*/
                CASE(0x86){ ZERO_PAGE(INSN_DAT8(1))   = X;}BREAK;
                CASE(0x96){ ZERO_PAGE(INSN_DAT8(1)+Y) = X;}BREAK;
                CASE(0x8e){ MEM_WRITE(INSN_DAT16(1),X);   }BREAK;
                /*sty*/
                CASE(0x84){ ZERO_PAGE(INSN_DAT8(1))   = Y;}BREAK;
                CASE(0x94){ ZERO_PAGE(INSN_DAT8(1)+X) = Y;}BREAK;
                CASE(0x8c){ MEM_WRITE(INSN_DAT16(1),Y);   }BREAK;

                /*tax,tay,tsx,txa,txs,tya*/
                CASE(0xaa){ X = A; X_FLAG_UPDATE_NZ;}BREAK;
                CASE(0xa8){ Y = A; Y_FLAG_UPDATE_NZ;}BREAK;
                CASE(0xba){ X = SP;X_FLAG_UPDATE_NZ;}BREAK;
                CASE(0x8a){ A = X; A_FLAG_UPDATE_NZ;}BREAK;
                CASE(0x9a){ SP =X; UPDATE_SP; }BREAK;
                CASE(0x98){ A = Y; A_FLAG_UPDATE_NZ;}BREAK;

                /*cpx*/
                CASE(0xe0){tmp=_cmp_(nes,X,INSN_DAT8(1),P);                 UPDATE_FLAGE_ZNC(tmp);}BREAK;
                CASE(0xe4){tmp=_cmp_(nes,X,ZERO_PAGE(INSN_DAT8(1)),P);      UPDATE_FLAGE_ZNC(tmp);}BREAK;
                CASE(0xec){tmp=_cmp_(nes,X,*(u8*)RMEM_PTR(INSN_DAT16(1)),P);UPDATE_FLAGE_ZNC(tmp);}BREAK;
                /*cpy*/
                CASE(0xc0){tmp=_cmp_(nes,Y,INSN_DAT8(1),P);                 UPDATE_FLAGE_ZNC(tmp);}BREAK;
                CASE(0xc4){tmp=_cmp_(nes,Y,ZERO_PAGE(INSN_DAT8(1)),P);      UPDATE_FLAGE_ZNC(tmp);}BREAK;
                CASE(0xcc){tmp=_cmp_(nes,Y,*(u8*)RMEM_PTR(INSN_DAT16(1)),P);UPDATE_FLAGE_ZNC(tmp);}BREAK;
                /*bit*/
                CASE(0x24){u8 M =      ZERO_PAGE(INSN_DAT8(1));M&=A; P &= ~(N6502_VLG|N6502_NLG); P|=(M&(1<<6))?N6502_VLG:0;FLAG_UPDATE_NZ(M);}BREAK;
                CASE(0x2c){u8 M =*(u8*)RMEM_PTR(INSN_DAT16(1));M&=A; P &= ~(N6502_VLG|N6502_NLG); P|=(M&(1<<6))?N6502_VLG:0;FLAG_UPDATE_NZ(M);}BREAK;

                /*push,pha*/
                CASE(0x48){PUSH(A)}BREAK;
                CASE(0x08){PUSH(P|(N6502_BLG|(1<<5)));}BREAK;
                /*pla,plp*/
                CASE(0x68){A=POP(); A_FLAG_UPDATE_NZ;}BREAK;
                CASE(0x28){P=POP(); P &= ~(N6502_BLG|(1<<5));}BREAK;

                /*RTI,RTS*/
                CASE(0x40){P=POP();P&=~(N6502_BLG);} /*执行0x60*/
                CASE(0x60){PC=POP(); PC|=POP()<<8; PC += *cpu->cur_insn==0x60; insn_len =0; /*如果是0x60则+1*/ UPDATE_PC; }BREAK;

                CASE(0x18){P &= ~N6502_CLG;}BREAK;
                CASE(0xd8){P &= ~N6502_DLG;}BREAK;
                CASE(0x58){P &= ~N6502_ILG;}BREAK;
                CASE(0xb8){P &= ~N6502_VLG;}BREAK;
                CASE(0x38){P |= N6502_CLG;}BREAK;
                CASE(0xf8){P |= N6502_DLG;}BREAK;
                CASE(0x78){P |= N6502_ILG;}BREAK;
                CASE(0x70){if   (P&N6502_VLG){PC += (int8_t)INSN_DAT8(1) /*跳转后再加2*/; UPDATE_PC;}}BREAK;
                CASE(0x50){if((~P)&N6502_VLG){PC += (int8_t)INSN_DAT8(1) /*跳转后再加2*/; UPDATE_PC;}}BREAK;
                CASE(0x10){if(~P&N6502_NLG)  {PC += (int8_t)INSN_DAT8(1) /*跳转后再加2*/; UPDATE_PC;}}BREAK;
                CASE(0xd0){if(~P&N6502_ZLG)  {PC += (int8_t)INSN_DAT8(1) /*跳转后再加2*/; UPDATE_PC;}}BREAK;
                CASE(0x30){if( P&N6502_NLG)  {PC += (int8_t)INSN_DAT8(1) /*跳转后再加2*/; UPDATE_PC;}}BREAK;
                CASE(0xf0){if( P&N6502_ZLG)  {PC += (int8_t)INSN_DAT8(1) /*跳转后再加2*/; UPDATE_PC;}}BREAK;
                CASE(0x90){if(~P&N6502_CLG)  {PC += (int8_t)INSN_DAT8(1) /*跳转后再加2*/; UPDATE_PC;}}BREAK;
                CASE(0xb0){if( P&N6502_CLG)  {PC += (int8_t)INSN_DAT8(1) /*跳转后再加2*/; UPDATE_PC;}}BREAK;

                /*错误指令*/
                #define GOTO_ERROR goto ERROR_LABEL
                CASE(0x02){GOTO_ERROR;}BREAK;
                CASE(0x03){GOTO_ERROR;}BREAK;
                CASE(0x04){GOTO_ERROR;}BREAK;
                CASE(0x07){GOTO_ERROR;}BREAK;
                CASE(0x0b){GOTO_ERROR;}BREAK;
                CASE(0x0c){GOTO_ERROR;}BREAK;
                CASE(0x0f){GOTO_ERROR;}BREAK;

                CASE(0x12){GOTO_ERROR;}BREAK;
                CASE(0x13){GOTO_ERROR;}BREAK;
                CASE(0x14){GOTO_ERROR;}BREAK;
                CASE(0x17){GOTO_ERROR;}BREAK;
                CASE(0x1a){GOTO_ERROR;}BREAK;
                CASE(0x1b){GOTO_ERROR;}BREAK;
                CASE(0x1c){GOTO_ERROR;}BREAK;
                CASE(0x1f){GOTO_ERROR;}BREAK;

                CASE(0x22){GOTO_ERROR;}BREAK;
                CASE(0x23){GOTO_ERROR;}BREAK;
                CASE(0x27){GOTO_ERROR;}BREAK;
                CASE(0x2b){GOTO_ERROR;}BREAK;
                CASE(0x2f){GOTO_ERROR;}BREAK;

                CASE(0x32){GOTO_ERROR;}BREAK;
                CASE(0x33){GOTO_ERROR;}BREAK;
                CASE(0x34){GOTO_ERROR;}BREAK;
                CASE(0x37){GOTO_ERROR;}BREAK;
                CASE(0x3a){GOTO_ERROR;}BREAK;
                CASE(0x3b){GOTO_ERROR;}BREAK;
                CASE(0x3c){GOTO_ERROR;}BREAK;
                CASE(0x3f){GOTO_ERROR;}BREAK;

                CASE(0x42){GOTO_ERROR;}BREAK;
                CASE(0x43){GOTO_ERROR;}BREAK;
                CASE(0x44){GOTO_ERROR;}BREAK;
                CASE(0x47){GOTO_ERROR;}BREAK;
                CASE(0x4b){GOTO_ERROR;}BREAK;
                CASE(0x4f){GOTO_ERROR;}BREAK;

                CASE(0x52){GOTO_ERROR;}BREAK;
                CASE(0x53){GOTO_ERROR;}BREAK;
                CASE(0x54){GOTO_ERROR;}BREAK;
                CASE(0x57){GOTO_ERROR;}BREAK;
                CASE(0x5a){GOTO_ERROR;}BREAK;
                CASE(0x5b){GOTO_ERROR;}BREAK;
                CASE(0x5c){GOTO_ERROR;}BREAK;
                CASE(0x5f){GOTO_ERROR;}BREAK;

                CASE(0x62){GOTO_ERROR;}BREAK;
                CASE(0x63){GOTO_ERROR;}BREAK;
                CASE(0x64){GOTO_ERROR;}BREAK;
                CASE(0x67){GOTO_ERROR;}BREAK;
                CASE(0x6b){GOTO_ERROR;}BREAK;
                CASE(0x6f){GOTO_ERROR;}BREAK;

                CASE(0x72){GOTO_ERROR;}BREAK;
                CASE(0x73){GOTO_ERROR;}BREAK;
                CASE(0x74){GOTO_ERROR;}BREAK;
                CASE(0x77){GOTO_ERROR;}BREAK;
                CASE(0x7a){GOTO_ERROR;}BREAK;
                CASE(0x7b){GOTO_ERROR;}BREAK;
                CASE(0x7c){GOTO_ERROR;}BREAK;
                CASE(0x7f){GOTO_ERROR;}BREAK;

                CASE(0x80){GOTO_ERROR;}BREAK;
                CASE(0x82){GOTO_ERROR;}BREAK;
                CASE(0x83){GOTO_ERROR;}BREAK;
                CASE(0x87){GOTO_ERROR;}BREAK;
                CASE(0x89){GOTO_ERROR;}BREAK;
                CASE(0x8b){GOTO_ERROR;}BREAK;
                CASE(0x8f){GOTO_ERROR;}BREAK;

                CASE(0x92){GOTO_ERROR;}BREAK;
                CASE(0x93){GOTO_ERROR;}BREAK;
                CASE(0x97){GOTO_ERROR;}BREAK;
                CASE(0x9b){GOTO_ERROR;}BREAK;
                CASE(0x9c){GOTO_ERROR;}BREAK;
                CASE(0x9e){GOTO_ERROR;}BREAK;
                CASE(0x9f){GOTO_ERROR;}BREAK;

                CASE(0xa3){GOTO_ERROR;}BREAK;
                CASE(0xa7){GOTO_ERROR;}BREAK;
                CASE(0xab){GOTO_ERROR;}BREAK;
                CASE(0xaf){GOTO_ERROR;}BREAK;

                CASE(0xb2){GOTO_ERROR;}BREAK;
                CASE(0xb3){GOTO_ERROR;}BREAK;
                CASE(0xb7){GOTO_ERROR;}BREAK;
                CASE(0xbb){GOTO_ERROR;}BREAK;
                CASE(0xbf){GOTO_ERROR;}BREAK;

                CASE(0xc2){GOTO_ERROR;}BREAK;
                CASE(0xc3){GOTO_ERROR;}BREAK;
                CASE(0xc7){GOTO_ERROR;}BREAK;
                CASE(0xcb){GOTO_ERROR;}BREAK;
                CASE(0xcf){GOTO_ERROR;}BREAK;

                CASE(0xd2){GOTO_ERROR;}BREAK;
                CASE(0xd3){GOTO_ERROR;}BREAK;
                CASE(0xd4){GOTO_ERROR;}BREAK;
                CASE(0xd7){GOTO_ERROR;}BREAK;
                CASE(0xda){GOTO_ERROR;}BREAK;
                CASE(0xdb){GOTO_ERROR;}BREAK;
                CASE(0xdc){GOTO_ERROR;}BREAK;
                CASE(0xdf){GOTO_ERROR;}BREAK;

                CASE(0xe2){GOTO_ERROR;}BREAK;
                CASE(0xe3){GOTO_ERROR;}BREAK;
                CASE(0xe7){GOTO_ERROR;}BREAK;
                CASE(0xeb){GOTO_ERROR;}BREAK;
                CASE(0xef){GOTO_ERROR;}BREAK;

                CASE(0xf2){GOTO_ERROR;}BREAK;
                CASE(0xf3){GOTO_ERROR;}BREAK;
                CASE(0xf4){GOTO_ERROR;}BREAK;
                CASE(0xf7){GOTO_ERROR;}BREAK;
                CASE(0xfa){GOTO_ERROR;}BREAK;
                CASE(0xfb){GOTO_ERROR;}BREAK;
                CASE(0xfc){GOTO_ERROR;}BREAK;
                CASE(0xff){GOTO_ERROR;}BREAK;

                CASE(0x00){
                    if(P&N6502_ILG){
                        PUSH((PC+1)>>8);
                        PUSH((PC+1));
                        PUSH(P);
                        P |= N6502_BLG;
                        P |= N6502_ILG;
                        PC=*(u16*)RMEM_PTR(0xfffe);UPDATE_PC;
                    }
                    }BREAK;
            DEFAULT
                ERROR_LABEL:
                EXNES_ERRORF(nes,"error insn:%04X-%02X\n",PC,opcode);
                exit(-1);
                BREAK;
            }
            SWITCH_END;
            PC += insn_len;
            nes->cur_insn += insn_len;
        }

        if(state&CPU_STATE_IO){
            //IO操作
            state &= ~CPU_STATE_IO;
            u16 addr = write_addr_value>>0x10;
            u16 value = write_addr_value&0xff;
            if(addr==0x2006){
                /*第一次写入,不会改变vram的地址*/
                nes->ppu_vram_ptr[nes->ppu_write_addr_flg&1] = value;
                if(nes->ppu_write_addr_flg&1){
                    nes->ppu_write_addr = nes->ppu_vram_ptr[0]<<8;
                    nes->ppu_write_addr |= nes->ppu_vram_ptr[1];
                    nes->ppu_read_flag = 0;
                }
                nes->ppu_write_addr_flg++;
            }
            else if(addr==0x2007){
                /*一般需要往0x2006写入两个数据,第一次写入为1,第二次写入为0*/
                u16 addr = nes->ppu_write_addr;
                nes->vram[addr&0x3fff] = value;
                exnes_ppu2000_t*_2000 = (exnes_ppu2000_t*)&nes->ppu[0];
                nes->ppu_write_addr += 1<<(_2000->_2007_inc_32*5);  //32的增长
            }
            else if(addr==0x2005){
                //设置滚动
                //(X*1)
                //(Y*1)
                //2000 b0: X*256
                //2000 b1: Y*240
                nes->PPUSCROLL[nes->PPUSCROLL_state&1] = value;
                nes->PPUSCROLL_state++;
            }
            else if(addr==0x2004){
                //写入目标数据
                if(nes->scanline>240){
                    /*只能在vblank中访问*/
                    nes->oam[nes->oam_addr++] = value;
                    nes->oam_addr &= 0xff;      //范围是0~0xff?
                }
            }
            else if(addr==0x2003){
                nes->oam_addr = value;
            }
            else if(addr==0x4014
                &&nes->scanline>=240
                ){
                //OAMDMA
                //传送到内部OAM
                u16 src_addr = (value&0xff)<<8;
                u8* dat = (u8*)RMEM_PTR(src_addr);
                u8* dest = nes->oam;
                int  dest_addr = nes->oam_addr;

                src_addr = 0;   /*已经不用了*/
                while(dest_addr<0x100){
                    dest[dest_addr] = dat[src_addr];
                    src_addr++;
                    dest_addr++;
                }
                state += 513;   /*和514,主要是看当前时钟是偶数还是奇数*/
            }
            else if(addr>=0x2000&&addr<0x2010){
                nes->ppu[addr&0xf] = value;
            }
            else if(addr==0x4016){
                /*输入时只能写这个寄存器*/
                if((value&1)==1){
                    nes->input_state = 1;
                }
                else{
                    /*重置位置?*/
                    nes->input_state = 1;
                }
            }
        }
        if(state&CPU_STATE_CYCLES){
            //hblank时钟溢出
            int c = (state&CPU_STATE_CYCLES_MASK) - CPU_STATE_CYCLES;
            state &= ~CPU_STATE_CYCLES_MASK;
            state |= CPU_STATE_SET_CYCLES_HBLANK;
            state += c;
            u16 pixbuf[0x200];
            u16 *pixel = pixbuf;
            if(nes->get_pixels_line){
                pixel = nes->get_pixels_line(nes,nes->scanline);
            }
            else{
                /*没有绘制的像素*/
                EXNES_ERRORF(nes,"no draw pixels!\n");
            }
            if(nes->scanline<240){
                exnes_ppu_render(nes,pixel,nes->scanline);
            }
            if(nes->process_other){
                nes->process_other(nes);
            }

            /*绘制屏幕*/
            nes->scanline++;

            if(nes->scanline>=nes->frame_scanlines){
                //绘制了一帧
                nes->scanline = 0;
                //设置其他状态
                //nes->rppu[0x2] &= ~(1<<6);      //sprite 0 hit
                ((exnes_ppu2002_t*)(&nes->rppu[0x2]))->sprite_0hit = 0;
                ((exnes_ppu2002_t*)(&nes->rppu[0x2]))->vblank_flg = 0;
            }
            if(nes->scanline==240){
                //设置cpu为中断状态
                if(((exnes_ppu2000_t*)(&nes->ppu[0]))->nmi){
                    //cpu中断处理
                    PUSH(PC>>8);
                    PUSH(PC);
                    PUSH(P);
                    PC = *(u16*)RMEM_PTR(0xfffa);
                    UPDATE_PC;
                }
                if(nes->render){
                    nes->render(nes);
                }
            }

            if(nes->scanline==240){
                /*垂直状态开始时,应该设置为0*/
                nes->oam_addr = 0;

                /*应该初始化*/
                nes->ppu_write_addr_flg = 0;
                nes->ppu_write_addr = 0;
            }

            if(nes->scanline>=240){
                //水平中断
                ((exnes_ppu2002_t*)(&nes->rppu[0x2]))->vblank_flg = 1;
            }
            else{
                //非中断状态
                ((exnes_ppu2002_t*)(&nes->rppu[0x2]))->vblank_flg = 0;
            }

        }
        if(CPU_STATE_PPURAM_READ&state){
            state &= ~CPU_STATE_PPURAM_READ;
            u16 addr = write_addr_value;
            //暂时不处理
        }
    }

    SAVE_FAST_REG(cpu);
    return 0;
}

INLINE u8* _exnes_rmem_0000(exnes_t*nes,u16 addr){
    return &nes->ram[addr&0x7ff];
}

INLINE u8* _exnes_rmem_2000(exnes_t*nes,u16 addr){
    /*读取IO*/
    return _rmem_io_ptr(nes,addr);
}

INLINE u8* _exnes_rmem_4000(exnes_t*nes,u16 addr){
    /*读取IO*/
    return &nes->error_mem[0];
}

INLINE u8* _exnes_wmem_0000(exnes_t*nes,u16 addr){
    /*写入IO*/
    return &nes->ram[addr&0x7ff];
}

INLINE u8* _exnes_wmem_2000(exnes_t*nes,u16 addr){
    /*写入IO*/
    exnes_t*cpu = nes;
    nes->cpu_state |= CPU_STATE_IO;
    return (MEM_PTR(addr));
}

INLINE u8* _exnes_rmem_other(exnes_t*nes,u16 addr){
    /*读取R0M*/
    int idx = addr>>0xc;
    return nes->rmem_map[addr>>0xc] + (0xfff&addr);
}

INLINE u8* _exnes_wmem_other(exnes_t*nes,u16 addr){
    /*写入指令*/
    int idx = addr>>0xc;
    return nes->mem_map[addr>>0xc] + (0xfff&addr);
}

INLINE int exnes_init_rom(exnes_t*nes,const u8 *rom){
    /*初始化*/
    exnes_t *cpu = nes;
    int i;
    /*初始化内存映射*/
    for(i=0;i<sizeof(nes->mem_map)/sizeof(nes->mem_map[0]);i++){
        /*锁定写入位置0*/
        nes->mem_map[i] = &nes->error_mem[0];
        nes->map_mask[i] = 0x0;
        /*读取内存*/
        nes->rmem_map[i] = &nes->error_mem[0];
        nes->rmap_mask[i] = 0x0;

        nes->ppu_mmap[i] = &nes->error_mem[0];
        nes->ppu_mmap[i] = 0x0;
    }

    for(i=0;i<sizeof(nes->ram);i++){
        if((i/8)&1){
            nes->ram[i] = 0xff;
        }
        else{
            nes->ram[i] = 0x00;
        }
    }

    //映射ram,区域以2KB作为镜像
    nes->mem_map[MMAP(0x0000)] = &nes->ram[0];
    nes->mem_map[MMAP(0x1000)] = &nes->ram[0];
    nes->map_mask[MMAP(0x0000)] = 0x7ff;
    nes->map_mask[MMAP(0x1000)] = 0x7ff;
    nes->rmem_map[MMAP(0x0000)] = &nes->ram[0];
    nes->rmem_map[MMAP(0x1000)] = &nes->ram[0];
    nes->rmap_mask[MMAP(0x0000)] = 0x7ff;
    nes->rmap_mask[MMAP(0x1000)] = 0x7ff;

    /*PPU,读取,写入时另起命令*/
    nes->rmem_map [MMAP(0x2000)] = &nes->rppu[0];
    nes->rmap_mask[MMAP(0x2000)] = 0xf;
    nes->ppu_mmap[MMAP(0x0000)] = &nes->vram[0x0000];
    nes->ppu_mmap[MMAP(0x1000)] = &nes->vram[0x1000];
    nes->ppu_mmap[MMAP(0x2000)] = &nes->vram[0x2000];
    nes->ppu_mmap[MMAP(0x3000)] = &nes->vram[0x3000];
    //存档
    nes->mem_map  [MMAP(0x6000)] = &nes->sram[0];
    nes->mem_map  [MMAP(0x7000)] = &nes->sram[0x1000];
    nes->map_mask [MMAP(0x6000)] = 0xfff;
    nes->map_mask [MMAP(0x7000)] = 0xfff;
    nes->rmem_map [MMAP(0x6000)] = &nes->sram[0];
    nes->rmem_map [MMAP(0x7000)] = &nes->sram[0x1000];
    nes->rmap_mask[MMAP(0x6000)] = 0xfff;
    nes->rmap_mask[MMAP(0x7000)] = 0xfff;

    /*IO读取,写入时,领取xx中断*/
    nes->rmem_map [MMAP(0x4000)] = &nes->apu[0];
    nes->rmap_mask[MMAP(0x4000)] = 0x1f;

    /*处理ROM nes文件头0x10*/
    const u8* rom_bin = rom + 0x10;
    //mario测试
    nes->rmem_map [MMAP(0x8000)] = (u8*)rom_bin + (0 + 0) * 0x1000;
    nes->rmem_map [MMAP(0x9000)] = (u8*)rom_bin + (0 + 1) * 0x1000;
    nes->rmem_map [MMAP(0xa000)] = (u8*)rom_bin + (0 + 2) * 0x1000;
    nes->rmem_map [MMAP(0xb000)] = (u8*)rom_bin + (0 + 3) * 0x1000;
    nes->rmem_map [MMAP(0xc000)] = (u8*)rom_bin + (4 + 0) * 0x1000;
    nes->rmem_map [MMAP(0xd000)] = (u8*)rom_bin + (4 + 1) * 0x1000;
    nes->rmem_map [MMAP(0xe000)] = (u8*)rom_bin + (4 + 2) * 0x1000;
    nes->rmem_map [MMAP(0xf000)] = (u8*)rom_bin + (4 + 3) * 0x1000;
    nes->rmap_mask[MMAP(0x8000)] = 0xfff;
    nes->rmap_mask[MMAP(0x9000)] = 0xfff;
    nes->rmap_mask[MMAP(0xa000)] = 0xfff;
    nes->rmap_mask[MMAP(0xb000)] = 0xfff;
    nes->rmap_mask[MMAP(0xc000)] = 0xfff;
    nes->rmap_mask[MMAP(0xd000)] = 0xfff;
    nes->rmap_mask[MMAP(0xe000)] = 0xfff;
    nes->rmap_mask[MMAP(0xf000)] = 0xfff;

    nes->ppu_mmap[MMAP(0x0000)] = (u8*)rom_bin + 0x8000;
    nes->ppu_mmap[MMAP(0x1000)] = (u8*)rom_bin + 0x9000;

    nes->rom = rom;

    //在0x4016和0x4017
    nes->input_ptr = &nes->apu[0x16];

    //初始化调色板
    for(i=0;i<sizeof(nes->pal)/sizeof(nes->pal[0]);i++){
        /*颜色需要放大2倍*/
        int r = (nes_pal[i]>>8)&0x7;
        int g = (nes_pal[i]>>4)&0x7;
        int b = (nes_pal[i]>>0)&0x7;
        r*=4;g*=4;b*=4;
        nes->pal[i] = ((r<<EXNES_PAL_R_SHIFT)|(g<<EXNES_PAL_G_SHIFT)|(b<<EXNES_PAL_B_SHIFT)) & 0xffff;
    }

    nes->screen_width = 256;
    exnes_video_setmode(nes,0);

    //初始化PC
    nes->rppu[0x2] = 0x10;
    nes->PC = *(u16*)_RMEM_PTR(0xfffc);
    nes->SP = 0xFD;
    nes->X = 0;
    nes->X = 0;
    nes->Y = 0;
    nes->P = 0x34; //禁用IRQ
    nes->apu[0x17] = 0; //帧中断开启
    nes->apu[0x15] = 0; //所有声道禁用
    for(i=0;i<sizeof(nes->apu);i++){
        nes->apu[i] = 0;
    }

    /*还不清楚当前初始化的问题*/
    PUSH_nes(nes,0x00);
    PUSH_nes(nes,0x02);
    PUSH_nes(nes,0x03);
    nes->P = 0x4;

    /*初始化tile的表*/
    for(i=0;i<0x100;i++){
        u16 t = 0;
        t |= (i&(1<<0))?1<<0x0:0;
        t |= (i&(1<<1))?1<<0x2:0;
        t |= (i&(1<<2))?1<<0x4:0;
        t |= (i&(1<<3))?1<<0x6:0;
        t |= (i&(1<<4))?1<<0x8:0;
        t |= (i&(1<<5))?1<<0xa:0;
        t |= (i&(1<<6))?1<<0xc:0;
        t |= (i&(1<<7))?1<<0xe:0;
        nes->tileTable[i]=t;
    }

    #if USE_MEMMAP_FUNC
    /*初始化读写函数指针*/
    nes->rmem_func[0x0] = _exnes_rmem_0000;
    nes->rmem_func[0x1] = _exnes_rmem_0000;
    nes->rmem_func[0x2] = _exnes_rmem_2000;
    nes->rmem_func[0x3] = _exnes_rmem_2000;
    nes->rmem_func[0x4] = _exnes_rmem_2000;
    nes->rmem_func[0x5] = _exnes_rmem_2000;

    nes->rmem_func[0x6] = _exnes_rmem_other; //一般为sram
    nes->rmem_func[0x7] = _exnes_rmem_other;
    nes->rmem_func[0x8] = _exnes_rmem_other; //rom
    nes->rmem_func[0x9] = _exnes_rmem_other;
    nes->rmem_func[0xa] = _exnes_rmem_other;
    nes->rmem_func[0xb] = _exnes_rmem_other;
    nes->rmem_func[0xc] = _exnes_rmem_other;
    nes->rmem_func[0xd] = _exnes_rmem_other;
    nes->rmem_func[0xe] = _exnes_rmem_other;
    nes->rmem_func[0xf] = _exnes_rmem_other;

    nes->wmem_func[0x0] = _exnes_wmem_0000;
    nes->wmem_func[0x1] = _exnes_wmem_0000;
    nes->wmem_func[0x2] = _exnes_wmem_2000;
    nes->wmem_func[0x3] = _exnes_wmem_2000;
    nes->wmem_func[0x4] = _exnes_wmem_2000;
    nes->wmem_func[0x5] = _exnes_wmem_2000;

    nes->wmem_func[0x6] = _exnes_wmem_other; //一般为sram
    nes->wmem_func[0x7] = _exnes_wmem_other;
    nes->wmem_func[0x8] = _exnes_wmem_other; //rom
    nes->wmem_func[0x9] = _exnes_wmem_other;
    nes->wmem_func[0xa] = _exnes_wmem_other;
    nes->wmem_func[0xb] = _exnes_wmem_other;
    nes->wmem_func[0xc] = _exnes_wmem_other;
    nes->wmem_func[0xd] = _exnes_wmem_other;
    nes->wmem_func[0xe] = _exnes_wmem_other;
    nes->wmem_func[0xf] = _exnes_wmem_other;
    #endif

    return 0;
}

INLINE void exnes_pal_update(exnes_t*nes){
    int i;
    for(i=0;i<sizeof(nes->pal)/sizeof(nes->pal[0]);i++){
        /*颜色需要放大2倍*/
        int r = (nes_pal[i]>>8)&0x7;
        int g = (nes_pal[i]>>4)&0x7;
        int b = (nes_pal[i]>>0)&0x7;
        r*=4;g*=4;b*=4;
        nes->pal[i] = ((r<<EXNES_PAL_R_SHIFT)|(g<<EXNES_PAL_G_SHIFT)|(b<<EXNES_PAL_B_SHIFT)) & 0xffff;
        if(nes->pal_isbigendian){
            //大端序
            nes->pal[i] = (((nes->pal[i]&0xff)<<8)|(nes->pal[i]>>8)) & 0xffff;
        }
    }
}

#endif
