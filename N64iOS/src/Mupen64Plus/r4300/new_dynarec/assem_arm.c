/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - assem_arm.c                                             *
 *   Copyright (C) 2009 Ari64                                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if 0
unsigned int next_interupt = 0;
int cycle_count = 0;
int last_count = 0;
int pending_exception = 0;
int pcaddr = 0;
int stop = 0;
char* invc_ptr = 0;
unsigned int address = 0;
uint64_t readmem_dword = 0;
uint64_t dword = 0;
unsigned int word = 0;
unsigned short hword = 0;
unsigned char byte = 0;
int FCR0 = 0;
int FCR31 = 0;
long long int reg[32] = {0}, hi = 0, lo = 0;
unsigned int reg_cop0[32] = {0};
double *reg_cop1_double[32] = {0};
float *reg_cop1_simple[32] = {0};
precomp_instr fake_pc = {0},fake_pc_float = {0};
precomp_instr* PC = 0;
//unsigned long dynarec_local[64+16+16+8+8+8+8+256+8+8+128+128+128+8+132+132] = {0};
#endif

extern int cycle_count;
extern int last_count;
extern int pcaddr;
extern int pending_exception;
extern uint64_t readmem_dword;
extern precomp_instr fake_pc,fake_pc_float;

extern void *dynarec_local;

void indirect_jump();

/* Linker */

void set_jump_target(int addr,int target)
{
    u_char *ptr=(u_char *)addr;
    assert((ptr[3]&0x0e)==0xa);
    u_int *ptr2=(u_int *)ptr;
    *ptr2=(*ptr2&0xFF000000)|(((target-(u_int)ptr2-8)<<6)>>8);
}

void kill_pointer(void *stub)
{
    int *ptr=(int *)(stub+4);
    assert((*ptr&0x0ff00000)==0x05900000);
    u_int offset=*ptr&0xfff;
    int **l_ptr=(void *)ptr+offset+8;
    int *i_ptr=*l_ptr;
    set_jump_target((int)i_ptr,(int)stub);
}

int get_pointer(void *stub)
{
    //printf("get_pointer(%x)\n",(int)stub);
    int *ptr=(int *)(stub+4);
    assert((*ptr&0x0ff00000)==0x05900000);
    u_int offset=*ptr&0xfff;
    int **l_ptr=(void *)ptr+offset+8;
    int *i_ptr=*l_ptr;
    assert((*i_ptr&0x0f000000)==0x0a000000);
    return (int)i_ptr+((*i_ptr<<8)>>6)+8;
}

/* Literal pool */
void add_literal(int addr,int val)
{
    literals[literalcount][0]=val;
    literals[literalcount][1]=addr;
    literalcount++;
}

/* Register allocation */

// Note: registers are allocated clean (unmodified state)
// if you intend to modify the register, you must call dirty_reg().
void alloc_reg(struct regstat *cur,int i,signed char reg)
{
    int r,hr;
    int preferred_reg = (reg&7);
    if(reg==CCREG) preferred_reg=HOST_CCREG;
    if(reg==PTEMP||reg==FTEMP) preferred_reg=12;
    
    // Don't allocate unused registers
    if((cur->u>>reg)&1) return;
    
    // see if it's already allocated
    for(hr=0;hr<HOST_REGS;hr++)
    {
        if(hr!=9)
        {
            if(cur->regmap[hr]==reg) return;
        }
    }
    
    // Try to allocate the preferred register
    if(cur->regmap[preferred_reg]==-1) {
        cur->regmap[preferred_reg]=reg;
        cur->dirty&=~(1<<preferred_reg);
        cur->isconst&=~(1<<preferred_reg);
        return;
    }
    r=cur->regmap[preferred_reg];
    if(r<64&&((cur->u>>r)&1)) {
        cur->regmap[preferred_reg]=reg;
        cur->dirty&=~(1<<preferred_reg);
        cur->isconst&=~(1<<preferred_reg);
        return;
    }
    if(r>=64&&((cur->uu>>(r&63))&1)) {
        cur->regmap[preferred_reg]=reg;
        cur->dirty&=~(1<<preferred_reg);
        cur->isconst&=~(1<<preferred_reg);
        return;
    }
    
    // Clear any unneeded registers
    // We try to keep the mapping consistent, if possible, because it
    // makes branches easier (especially loops).  So we try to allocate
    // first (see above) before removing old mappings.  If this is not
    // possible then go ahead and clear out the registers that are no
    // longer needed.
    for(hr=0;hr<HOST_REGS;hr++)
    {
        if(hr!=9)
        {
            r=cur->regmap[hr];
            if(r>=0) {
                if(r<64) {
                    if((cur->u>>r)&1) {cur->regmap[hr]=-1;break;}
                }
                else
                {
                    if((cur->uu>>(r&63))&1) {cur->regmap[hr]=-1;break;}
                }
            }
        }
    }
    // Try to allocate any available register
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && hr!=EXCLUDE_REG&&cur->regmap[hr]==-1) {
            cur->regmap[hr]=reg;
            cur->dirty&=~(1<<hr);
            cur->isconst&=~(1<<hr);
            return;
        }
    }
    
    // Ok, now we have to evict someone
    // Pick a register we hopefully won't need soon
    // TODO: honor register preferences
    u_char hsn[MAXREG+1];
    memset(hsn,10,sizeof(hsn));
    int j;
    lsn(hsn,i,&preferred_reg);
    //printf("eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d\n",cur->regmap[0],cur->regmap[1],cur->regmap[2],cur->regmap[3],cur->regmap[5],cur->regmap[6],cur->regmap[7]);
    //printf("hsn(%x): %d %d %d %d %d %d %d\n",start+i*4,hsn[cur->regmap[0]&63],hsn[cur->regmap[1]&63],hsn[cur->regmap[2]&63],hsn[cur->regmap[3]&63],hsn[cur->regmap[5]&63],hsn[cur->regmap[6]&63],hsn[cur->regmap[7]&63]);
    for(j=10;j>=0;j--)
    {
        for(r=1;r<=MAXREG;r++)
        {
            if(hsn[r]==j) {
                for(hr=0;hr<HOST_REGS;hr++) {
                    if(hr!=9 && cur->regmap[hr]==r+64) {
                        cur->regmap[hr]=reg;
                        cur->dirty&=~(1<<hr);
                        cur->isconst&=~(1<<hr);
                        return;
                    }
                }
                for(hr=0;hr<HOST_REGS;hr++) {
                    if(hr!=9 && cur->regmap[hr]==r) {
                        cur->regmap[hr]=reg;
                        cur->dirty&=~(1<<hr);
                        cur->isconst&=~(1<<hr);
                        return;
                    }
                }
            }
        }
    }
    fprintf(stderr, "This shouldn't happen (alloc_reg)\n");exit(1);
}

void alloc_reg64(struct regstat *cur,int i,signed char reg)
{
    int preferred_reg = 7+(reg&1);
    int r,hr;
    
    // allocate the lower 32 bits
    alloc_reg(cur,i,reg);
    
    // Don't allocate unused registers
    if((cur->uu>>reg)&1) return;
    
    // see if the upper half is already allocated
    for(hr=0;hr<HOST_REGS;hr++)
    {
        if(hr!=9)
        {
            if(cur->regmap[hr]==reg+64) return;
        }
    }
    
    // Try to allocate the preferred register
    if(cur->regmap[preferred_reg]==-1) {
        cur->regmap[preferred_reg]=reg|64;
        cur->dirty&=~(1<<preferred_reg);
        cur->isconst&=~(1<<preferred_reg);
        return;
    }
    r=cur->regmap[preferred_reg];
    if(r<64&&((cur->u>>r)&1)) {
        cur->regmap[preferred_reg]=reg|64;
        cur->dirty&=~(1<<preferred_reg);
        cur->isconst&=~(1<<preferred_reg);
        return;
    }
    if(r>=64&&((cur->uu>>(r&63))&1)) {
        cur->regmap[preferred_reg]=reg|64;
        cur->dirty&=~(1<<preferred_reg);
        cur->isconst&=~(1<<preferred_reg);
        return;
    }
    
    // Clear any unneeded registers
    // We try to keep the mapping consistent, if possible, because it
    // makes branches easier (especially loops).  So we try to allocate
    // first (see above) before removing old mappings.  If this is not
    // possible then go ahead and clear out the registers that are no
    // longer needed.
    for(hr=HOST_REGS-1;hr>=0;hr--)
    {
        if(hr!=9)
        {
            r=cur->regmap[hr];
            if(r>=0) {
                if(r<64) {
                    if((cur->u>>r)&1) {cur->regmap[hr]=-1;break;}
                }
                else
                {
                    if((cur->uu>>(r&63))&1) {cur->regmap[hr]=-1;break;}
                }
            }
        }
    }
    // Try to allocate any available register
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && hr!=EXCLUDE_REG&&cur->regmap[hr]==-1) {
            cur->regmap[hr]=reg|64;
            cur->dirty&=~(1<<hr);
            cur->isconst&=~(1<<hr);
            return;
        }
    }
    
    // Ok, now we have to evict someone
    // Pick a register we hopefully won't need soon
    // TODO: honor register preferences
    u_char hsn[MAXREG+1];
    memset(hsn,10,sizeof(hsn));
    int j;
    lsn(hsn,i,&preferred_reg);
    //printf("eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d\n",cur->regmap[0],cur->regmap[1],cur->regmap[2],cur->regmap[3],cur->regmap[5],cur->regmap[6],cur->regmap[7]);
    //printf("hsn(%x): %d %d %d %d %d %d %d\n",start+i*4,hsn[cur->regmap[0]&63],hsn[cur->regmap[1]&63],hsn[cur->regmap[2]&63],hsn[cur->regmap[3]&63],hsn[cur->regmap[5]&63],hsn[cur->regmap[6]&63],hsn[cur->regmap[7]&63]);
    for(j=10;j>=0;j--)
    {
        for(r=1;r<=MAXREG;r++)
        {
            if(hsn[r]==j) {
                for(hr=0;hr<HOST_REGS;hr++) {
                    if(hr!=9 && cur->regmap[hr]==r+64) {
                        cur->regmap[hr]=reg|64;
                        cur->dirty&=~(1<<hr);
                        cur->isconst&=~(1<<hr);
                        return;
                    }
                }
                for(hr=0;hr<HOST_REGS;hr++) {
                    if(hr!=9 && cur->regmap[hr]==r) {
                        cur->regmap[hr]=reg|64;
                        cur->dirty&=~(1<<hr);
                        cur->isconst&=~(1<<hr);
                        return;
                    }
                }
            }
        }
    }
    fprintf(stderr, "This shouldn't happen\n");exit(1);
}

// Allocate a temporary register.  This is done without regard to
// dirty status or whether the register we request is on the unneeded list
// Note: This will only allocate one register, even if called multiple times
void alloc_reg_temp(struct regstat *cur,int i,signed char reg)
{
    int r,hr;
    int preferred_reg = -1;
    
    // see if it's already allocated
    for(hr=0;hr<HOST_REGS;hr++)
    {
        if(hr!=9 && hr!=EXCLUDE_REG&&cur->regmap[hr]==reg) return;
    }
    
    // Try to allocate any available register
    for(hr=HOST_REGS-1;hr>=0;hr--) {
        if(hr!=9 && hr!=EXCLUDE_REG&&cur->regmap[hr]==-1) {
            cur->regmap[hr]=reg;
            cur->dirty&=~(1<<hr);
            cur->isconst&=~(1<<hr);
            return;
        }
    }
    
    // Find an unneeded register
    for(hr=HOST_REGS-1;hr>=0;hr--)
    {
        if(hr!=9)
        {
            r=cur->regmap[hr];
            if(r>=0) {
                if(r<64) {
                    if((cur->u>>r)&1) {
                        cur->regmap[hr]=reg;
                        cur->dirty&=~(1<<hr);
                        cur->isconst&=~(1<<hr);
                        return;
                    }
                }
                else
                {
                    if((cur->uu>>(r&63))&1) {
                        cur->regmap[hr]=reg;
                        cur->dirty&=~(1<<hr);
                        cur->isconst&=~(1<<hr);
                        return;
                    }
                }
            }
        }
    }
    
    // Ok, now we have to evict someone
    // Pick a register we hopefully won't need soon
    // TODO: we might want to follow unconditional jumps here
    // TODO: get rid of dupe code and make this into a function
    u_char hsn[MAXREG+1];
    memset(hsn,10,sizeof(hsn));
    int j;
    lsn(hsn,i,&preferred_reg);
    //printf("hsn: %d %d %d %d %d %d %d\n",hsn[cur->regmap[0]&63],hsn[cur->regmap[1]&63],hsn[cur->regmap[2]&63],hsn[cur->regmap[3]&63],hsn[cur->regmap[5]&63],hsn[cur->regmap[6]&63],hsn[cur->regmap[7]&63]);
    for(j=10;j>=0;j--)
    {
        for(r=1;r<=MAXREG;r++)
        {
            if(hsn[r]==j) {
                for(hr=0;hr<HOST_REGS;hr++) {
                    if(hr!=9 && cur->regmap[hr]==r+64) {
                        cur->regmap[hr]=reg;
                        cur->dirty&=~(1<<hr);
                        cur->isconst&=~(1<<hr);
                        return;
                    }
                }
                for(hr=0;hr<HOST_REGS;hr++) {
                    if(hr!=9 && cur->regmap[hr]==r) {
                        cur->regmap[hr]=reg;
                        cur->dirty&=~(1<<hr);
                        cur->isconst&=~(1<<hr);
                        return;
                    }
                }
            }
        }
    }
    fprintf(stderr, "This shouldn't happen");exit(1);
}
// Allocate a specific ARM register.
void alloc_arm_reg(struct regstat *cur,int i,signed char reg,char hr)
{
    int n;
    
    // see if it's already allocated (and dealloc it)
    for(n=0;n<HOST_REGS;n++)
    {
        if(n!=9 && n!=EXCLUDE_REG&&cur->regmap[n]==reg) {cur->regmap[n]=-1;}
    }
    
    cur->regmap[hr]=reg;
    cur->dirty&=~(1<<hr);
    cur->isconst&=~(1<<hr);
}

// Alloc cycle count into dedicated register
void alloc_cc(struct regstat *cur,int i)
{
    alloc_arm_reg(cur,i,CCREG,HOST_CCREG);
}

/* Special alloc */


/* Assembler */

char regname[16][4] = {
    "r0",
    "r1",
    "r2",
    "r3",
    "r4",
    "r5",
    "r6",
    "r7",
    "r8",
    "r9",
    "r10",
    "fp",
    "r12",
    "sp",
    "lr",
    "pc"};

void output_byte(u_char byte)
{
    *(out++)=byte;
}
void output_modrm(u_char mod,u_char rm,u_char ext)
{
    assert(mod<4);
    assert(rm<8);
    assert(ext<8);
    u_char byte=(mod<<6)|(ext<<3)|rm;
    *(out++)=byte;
}
void output_sib(u_char scale,u_char index,u_char base)
{
    assert(scale<4);
    assert(index<8);
    assert(base<8);
    u_char byte=(scale<<6)|(index<<3)|base;
    *(out++)=byte;
}
void output_w32(u_int word)
{
    *((u_int *)out)=word;
    out+=4;
}
u_int rd_rn_rm(u_int rd, u_int rn, u_int rm)
{
    assert(rd<16);
    assert(rn<16);
    assert(rm<16);
    return((rn<<16)|(rd<<12)|rm);
}
u_int rd_rn_imm_shift(u_int rd, u_int rn, u_int imm, u_int shift)
{
    assert(rd<16);
    assert(rn<16);
    assert(imm<256);
    assert((shift&1)==0);
    return((rn<<16)|(rd<<12)|(((32-shift)&30)<<7)|imm);
}
u_int genimm(u_int imm,u_int *encoded)
{
    if(imm==0) {*encoded=0;return 1;}
    int i=32;
    while(i>0)
    {
        if(imm<256) {
            *encoded=((i&30)<<7)|imm;
            return 1;
        }
        imm=(imm>>2)|(imm<<30);i-=2;
    }
    return 0;
}
u_int genjmp(u_int addr)
{
    int offset=addr-(int)out-8;
    if(offset<-33554432||offset>=33554432) return 0;
    return ((u_int)offset>>2)&0xffffff;
}

void emit_mov(int rs,int rt)
{
    assem_debug("mov %s,%s\n",regname[rt],regname[rs]);
    output_w32(0xe1a00000|rd_rn_rm(rt,0,rs));
}

void emit_movs(int rs,int rt)
{
    assem_debug("movs %s,%s\n",regname[rt],regname[rs]);
    output_w32(0xe1b00000|rd_rn_rm(rt,0,rs));
}

void emit_add(int rs1,int rs2,int rt)
{
    assem_debug("add %s,%s,%s\n",regname[rt],regname[rs1],regname[rs2]);
    output_w32(0xe0800000|rd_rn_rm(rt,rs1,rs2));
}

void emit_adds(int rs1,int rs2,int rt)
{
    assem_debug("adds %s,%s,%s\n",regname[rt],regname[rs1],regname[rs2]);
    output_w32(0xe0900000|rd_rn_rm(rt,rs1,rs2));
}

void emit_adcs(int rs1,int rs2,int rt)
{
    assem_debug("adcs %s,%s,%s\n",regname[rt],regname[rs1],regname[rs2]);
    output_w32(0xe0b00000|rd_rn_rm(rt,rs1,rs2));
}

void emit_sbc(int rs1,int rs2,int rt)
{
    assem_debug("sbc %s,%s,%s\n",regname[rt],regname[rs1],regname[rs2]);
    output_w32(0xe0c00000|rd_rn_rm(rt,rs1,rs2));
}

void emit_sbcs(int rs1,int rs2,int rt)
{
    assem_debug("sbcs %s,%s,%s\n",regname[rt],regname[rs1],regname[rs2]);
    output_w32(0xe0d00000|rd_rn_rm(rt,rs1,rs2));
}

void emit_neg(int rs, int rt)
{
    assem_debug("rsb %s,%s,#0\n",regname[rt],regname[rs]);
    output_w32(0xe2600000|rd_rn_rm(rt,rs,0));
}

void emit_negs(int rs, int rt)
{
    assem_debug("rsbs %s,%s,#0\n",regname[rt],regname[rs]);
    output_w32(0xe2700000|rd_rn_rm(rt,rs,0));
}

void emit_sub(int rs1,int rs2,int rt)
{
    assem_debug("sub %s,%s,%s\n",regname[rt],regname[rs1],regname[rs2]);
    output_w32(0xe0400000|rd_rn_rm(rt,rs1,rs2));
}

void emit_subs(int rs1,int rs2,int rt)
{
    assem_debug("subs %s,%s,%s\n",regname[rt],regname[rs1],regname[rs2]);
    output_w32(0xe0500000|rd_rn_rm(rt,rs1,rs2));
}

void emit_zeroreg(int rt)
{
    assem_debug("mov %s,#0\n",regname[rt]);
    output_w32(0xe3a00000|rd_rn_rm(rt,0,0));
}

void emit_loadreg(int r, int hr)
{
    if((r&63)==0)
        emit_zeroreg(hr);
    else {
        int addr=((int)reg)+((r&63)<<3)+((r&64)>>4);
        if((r&63)==HIREG) addr=(int)&hi+((r&64)>>4);
        if((r&63)==LOREG) addr=(int)&lo+((r&64)>>4);
        if(r==CCREG) addr=(int)&cycle_count;
        if(r==CSREG) addr=(int)&Status;
        if(r==FSREG) addr=(int)&FCR31;
        if(r==INVCP) addr=(int)&invc_ptr;
        u_int offset = addr-(u_int)&dynarec_local;
        assert(offset<4096);
        assem_debug("ldr %s,fp+%d\n",regname[hr],offset);
        output_w32(0xe5900000|rd_rn_rm(hr,FP,0)|offset);
    }
}
void emit_storereg(int r, int hr)
{
    int addr=((int)reg)+((r&63)<<3)+((r&64)>>4);
    if((r&63)==HIREG) addr=(int)&hi+((r&64)>>4);
    if((r&63)==LOREG) addr=(int)&lo+((r&64)>>4);
    if(r==CCREG) addr=(int)&cycle_count;
    u_int offset = addr-(u_int)&dynarec_local;
    assert(offset<4096);
    assem_debug("str %s,fp+%d\n",regname[hr],offset);
    output_w32(0xe5800000|rd_rn_rm(hr,FP,0)|offset);
}

void emit_test(int rs, int rt)
{
    assem_debug("tst %s,%s\n",regname[rs],regname[rt]);
    output_w32(0xe1100000|rd_rn_rm(0,rs,rt));
}

void emit_testimm(int rs,int imm)
{
    u_int armval;
    assem_debug("tst %s,$%d\n",regname[rs],imm);
    assert(genimm(imm,&armval));
    output_w32(0xe3100000|rd_rn_rm(0,rs,0)|armval);
}

void emit_not(int rs,int rt)
{
    assem_debug("mvn %s,%s\n",regname[rt],regname[rs]);
    output_w32(0xe1e00000|rd_rn_rm(rt,0,rs));
}

void emit_and(u_int rs1,u_int rs2,u_int rt)
{
    assem_debug("and %s,%s,%s\n",regname[rt],regname[rs1],regname[rs2]);
    output_w32(0xe0000000|rd_rn_rm(rt,rs1,rs2));
}

void emit_or(u_int rs1,u_int rs2,u_int rt)
{
    assem_debug("orr %s,%s,%s\n",regname[rt],regname[rs1],regname[rs2]);
    output_w32(0xe1800000|rd_rn_rm(rt,rs1,rs2));
}
void emit_or_and_set_flags(int rs1,int rs2,int rt)
{
    assem_debug("orrs %s,%s,%s\n",regname[rt],regname[rs1],regname[rs2]);
    output_w32(0xe1900000|rd_rn_rm(rt,rs1,rs2));
}

void emit_xor(u_int rs1,u_int rs2,u_int rt)
{
    assem_debug("eor %s,%s,%s\n",regname[rt],regname[rs1],regname[rs2]);
    output_w32(0xe0200000|rd_rn_rm(rt,rs1,rs2));
}

void emit_loadlp(u_int imm,u_int rt)
{
    add_literal((int)out,imm);
    assem_debug("ldr %s,pc+? [=%x]\n",regname[rt],imm);
    output_w32(0xe5900000|rd_rn_rm(rt,15,0));
}
void emit_movw(u_int imm,u_int rt)
{
    assert(imm<65536);
    assem_debug("movw %s,#%d (0x%x)\n",regname[rt],imm,imm);
    output_w32(0xe3000000|rd_rn_rm(rt,0,0)|(imm&0xfff)|((imm<<4)&0xf0000));
}
void emit_movt(u_int imm,u_int rt)
{
    assem_debug("movt %s,#%d (0x%x)\n",regname[rt],imm&0xffff0000,imm&0xffff0000);
    output_w32(0xe3400000|rd_rn_rm(rt,0,0)|((imm>>16)&0xfff)|((imm>>12)&0xf0000));
}
void emit_movimm(u_int imm,u_int rt)
{
    u_int armval;
    if(genimm(imm,&armval)) {
        assem_debug("mov %s,#%d\n",regname[rt],imm);
        output_w32(0xe3a00000|rd_rn_rm(rt,0,0)|armval);
    }else if(genimm(~imm,&armval)) {
        assem_debug("mvn %s,#%d\n",regname[rt],imm);
        output_w32(0xe3e00000|rd_rn_rm(rt,0,0)|armval);
    }else if(imm<65536) {
#ifdef ARMV5_ONLY
        assem_debug("mov %s,#%d\n",regname[rt],imm&0xFF00);
        output_w32(0xe3a00000|rd_rn_imm_shift(rt,0,imm>>8,8));
        assem_debug("add %s,%s,#%d\n",regname[rt],regname[rt],imm&0xFF);
        output_w32(0xe2800000|rd_rn_imm_shift(rt,rt,imm&0xff,0));
#else
        emit_movw(imm,rt);
#endif
    }else{
#ifdef ARMV5_ONLY
        emit_loadlp(imm,rt);
#else
        emit_movw(imm&0x0000FFFF,rt);
        emit_movt(imm&0xFFFF0000,rt);
#endif
    }
}

void emit_addimm(u_int rs,int imm,u_int rt)
{
    assert(rs<16);
    assert(rt<16);
    if(imm!=0) {
        assert(imm>-65536&&imm<65536);
        u_int armval;
        if(genimm(imm,&armval)) {
            assem_debug("add %s,%s,#%d\n",regname[rt],regname[rs],imm);
            output_w32(0xe2800000|rd_rn_rm(rt,rs,0)|armval);
        }else if(genimm(-imm,&armval)) {
            assem_debug("sub %s,%s,#%d\n",regname[rt],regname[rs],imm);
            output_w32(0xe2400000|rd_rn_rm(rt,rs,0)|armval);
        }else if(imm<0) {
            assem_debug("sub %s,%s,#%d\n",regname[rt],regname[rs],(-imm)&0xFF00);
            assem_debug("sub %s,%s,#%d\n",regname[rt],regname[rs],(-imm)&0xFF);
            output_w32(0xe2400000|rd_rn_imm_shift(rt,rs,(-imm)>>8,8));
            output_w32(0xe2400000|rd_rn_imm_shift(rt,rt,(-imm)&0xff,0));
        }else{
            assem_debug("add %s,%s,#%d\n",regname[rt],regname[rs],imm&0xFF00);
            assem_debug("add %s,%s,#%d\n",regname[rt],regname[rs],imm&0xFF);
            output_w32(0xe2800000|rd_rn_imm_shift(rt,rs,imm>>8,8));
            output_w32(0xe2800000|rd_rn_imm_shift(rt,rt,imm&0xff,0));
        }
    }
    else if(rs!=rt) emit_mov(rs,rt);
}

void emit_addimm_and_set_flags(int imm,int rt)
{
    assert(imm>-65536&&imm<65536);
    u_int armval;
    if(genimm(imm,&armval)) {
        assem_debug("adds %s,%s,#%d\n",regname[rt],regname[rt],imm);
        output_w32(0xe2900000|rd_rn_rm(rt,rt,0)|armval);
    }else if(genimm(-imm,&armval)) {
        assem_debug("subs %s,%s,#%d\n",regname[rt],regname[rt],imm);
        output_w32(0xe2500000|rd_rn_rm(rt,rt,0)|armval);
    }else if(imm<0) {
        assem_debug("sub %s,%s,#%d\n",regname[rt],regname[rt],(-imm)&0xFF00);
        assem_debug("subs %s,%s,#%d\n",regname[rt],regname[rt],(-imm)&0xFF);
        output_w32(0xe2400000|rd_rn_imm_shift(rt,rt,(-imm)>>8,8));
        output_w32(0xe2500000|rd_rn_imm_shift(rt,rt,(-imm)&0xff,0));
    }else{
        assem_debug("add %s,%s,#%d\n",regname[rt],regname[rt],imm&0xFF00);
        assem_debug("adds %s,%s,#%d\n",regname[rt],regname[rt],imm&0xFF);
        output_w32(0xe2800000|rd_rn_imm_shift(rt,rt,imm>>8,8));
        output_w32(0xe2900000|rd_rn_imm_shift(rt,rt,imm&0xff,0));
    }
}
void emit_addimm_no_flags(u_int imm,u_int rt)
{
    emit_addimm(rt,imm,rt);
}

void emit_adcimm(int imm,u_int rt)
{
    u_int armval;
    assert(genimm(imm,&armval));
    assem_debug("adc %s,%s,#%d\n",regname[rt],regname[rt],imm);
    output_w32(0xe2a00000|rd_rn_rm(rt,rt,0)|armval);
}
/*void emit_sbcimm(int imm,u_int rt)
 {
 u_int armval;
 assert(genimm(imm,&armval));
 assem_debug("sbc %s,%s,#%d\n",regname[rt],regname[rt],imm);
 output_w32(0xe2c00000|rd_rn_rm(rt,rt,0)|armval);
 }*/
void emit_sbbimm(int imm,u_int rt)
{
    assem_debug("sbb $%d,%%%s\n",imm,regname[rt]);
    assert(rt<8);
    if(imm<128&&imm>=-128) {
        output_byte(0x83);
        output_modrm(3,rt,3);
        output_byte(imm);
    }
    else
    {
        output_byte(0x81);
        output_modrm(3,rt,3);
        output_w32(imm);
    }
}
void emit_rscimm(int rs,int imm,u_int rt)
{
    assert(0);
    u_int armval;
    assert(genimm(imm,&armval));
    assem_debug("rsc %s,%s,#%d\n",regname[rt],regname[rs],imm);
    output_w32(0xe2e00000|rd_rn_rm(rt,rs,0)|armval);
}

void emit_addimm64_32(int rsh,int rsl,int imm,int rth,int rtl)
{
    if(rsh==rth&&rsl==rtl) {
        assem_debug("add $%d,%%%s\n",imm,regname[rtl]);
        if(imm<128&&imm>=-128) {
            output_byte(0x83);
            output_modrm(3,rtl,0);
            output_byte(imm);
        }
        else
        {
            output_byte(0x81);
            output_modrm(3,rtl,0);
            output_w32(imm);
        }
        assem_debug("adc $%d,%%%s\n",imm>>31,regname[rth]);
        output_byte(0x83);
        output_modrm(3,rth,2);
        output_byte(imm>>31);
    }
    else {
        emit_mov(rsh,rth);
        emit_mov(rsl,rtl);
        emit_addimm64_32(rth,rtl,imm,rth,rtl);
    }
}

void emit_sbb(int rs1,int rs2)
{
    assem_debug("sbb %%%s,%%%s\n",regname[rs2],regname[rs1]);
    output_byte(0x19);
    output_modrm(3,rs1,rs2);
}

void emit_andimm(int rs,int imm,int rt)
{
    u_int armval;
    if(genimm(imm,&armval)) {
        assem_debug("and %s,%s,#%d\n",regname[rt],regname[rs],imm);
        output_w32(0xe2000000|rd_rn_rm(rt,rs,0)|armval);
    }else if(genimm(~imm,&armval)) {
        assem_debug("bic %s,%s,#%d\n",regname[rt],regname[rs],imm);
        output_w32(0xe3c00000|rd_rn_rm(rt,rs,0)|armval);
    }else if(imm==65535) {
#ifdef ARMV5_ONLY
        assem_debug("bic %s,%s,#FF000000\n",regname[rt],regname[rs]);
        output_w32(0xe3c00000|rd_rn_rm(rt,rs,0)|0x4FF);
        assem_debug("bic %s,%s,#00FF0000\n",regname[rt],regname[rt]);
        output_w32(0xe3c00000|rd_rn_rm(rt,rt,0)|0x8FF);
#else
        assem_debug("uxth %s,%s\n",regname[rt],regname[rs]);
        output_w32(0xe6ff0070|rd_rn_rm(rt,0,rs));
#endif
    }else{
        assert(imm>0&&imm<65535);
#ifdef ARMV5_ONLY
        assem_debug("mov r14,#%d\n",imm&0xFF00);
        output_w32(0xe3a00000|rd_rn_imm_shift(HOST_TEMPREG,0,imm>>8,8));
        assem_debug("add r14,r14,#%d\n",imm&0xFF);
        output_w32(0xe2800000|rd_rn_imm_shift(HOST_TEMPREG,HOST_TEMPREG,imm&0xff,0));
#else
        emit_movw(imm,HOST_TEMPREG);
#endif
        assem_debug("and %s,%s,r14\n",regname[rt],regname[rs]);
        output_w32(0xe0000000|rd_rn_rm(rt,rs,HOST_TEMPREG));
    }
}

void emit_orimm(int rs,int imm,int rt)
{
    u_int armval;
    if(genimm(imm,&armval)) {
        assem_debug("orr %s,%s,#%d\n",regname[rt],regname[rs],imm);
        output_w32(0xe3800000|rd_rn_rm(rt,rs,0)|armval);
    }else{
        assert(imm>0&&imm<65536);
        assem_debug("orr %s,%s,#%d\n",regname[rt],regname[rs],imm&0xFF00);
        assem_debug("orr %s,%s,#%d\n",regname[rt],regname[rs],imm&0xFF);
        output_w32(0xe3800000|rd_rn_imm_shift(rt,rs,imm>>8,8));
        output_w32(0xe3800000|rd_rn_imm_shift(rt,rt,imm&0xff,0));
    }
}

void emit_xorimm(int rs,int imm,int rt)
{
    assert(imm>0&&imm<65536);
    u_int armval;
    if(genimm(imm,&armval)) {
        assem_debug("eor %s,%s,#%d\n",regname[rt],regname[rs],imm);
        output_w32(0xe2200000|rd_rn_rm(rt,rs,0)|armval);
    }else{
        assert(imm>0);
        assem_debug("eor %s,%s,#%d\n",regname[rt],regname[rs],imm&0xFF00);
        assem_debug("eor %s,%s,#%d\n",regname[rt],regname[rs],imm&0xFF);
        output_w32(0xe2200000|rd_rn_imm_shift(rt,rs,imm>>8,8));
        output_w32(0xe2200000|rd_rn_imm_shift(rt,rt,imm&0xff,0));
    }
}

void emit_shlimm(int rs,u_int imm,int rt)
{
    assert(imm>0);
    assert(imm<32);
    //if(imm==1) ...
    assem_debug("lsl %s,%s,#%d\n",regname[rt],regname[rs],imm);
    output_w32(0xe1a00000|rd_rn_rm(rt,0,rs)|(imm<<7));
}

void emit_shrimm(int rs,u_int imm,int rt)
{
    assert(imm>0);
    assert(imm<32);
    assem_debug("lsr %s,%s,#%d\n",regname[rt],regname[rs],imm);
    output_w32(0xe1a00000|rd_rn_rm(rt,0,rs)|0x20|(imm<<7));
}

void emit_sarimm(int rs,u_int imm,int rt)
{
    assert(imm>0);
    assert(imm<32);
    assem_debug("asr %s,%s,#%d\n",regname[rt],regname[rs],imm);
    output_w32(0xe1a00000|rd_rn_rm(rt,0,rs)|0x40|(imm<<7));
}

void emit_rorimm(int rs,u_int imm,int rt)
{
    assert(imm>0);
    assert(imm<32);
    assem_debug("ror %s,%s,#%d\n",regname[rt],regname[rs],imm);
    output_w32(0xe1a00000|rd_rn_rm(rt,0,rs)|0x60|(imm<<7));
}

void emit_shldimm(int rs,int rs2,u_int imm,int rt)
{
    assem_debug("shld %%%s,%%%s,%d\n",regname[rt],regname[rs2],imm);
    assert(imm>0);
    assert(imm<32);
    //if(imm==1) ...
    assem_debug("lsl %s,%s,#%d\n",regname[rt],regname[rs],imm);
    output_w32(0xe1a00000|rd_rn_rm(rt,0,rs)|(imm<<7));
    assem_debug("orr %s,%s,%s,lsr #%d\n",regname[rt],regname[rt],regname[rs2],32-imm);
    output_w32(0xe1800020|rd_rn_rm(rt,rt,rs2)|((32-imm)<<7));
}

void emit_shrdimm(int rs,int rs2,u_int imm,int rt)
{
    assem_debug("shrd %%%s,%%%s,%d\n",regname[rt],regname[rs2],imm);
    assert(imm>0);
    assert(imm<32);
    //if(imm==1) ...
    assem_debug("lsr %s,%s,#%d\n",regname[rt],regname[rs],imm);
    output_w32(0xe1a00020|rd_rn_rm(rt,0,rs)|(imm<<7));
    assem_debug("orr %s,%s,%s,lsl #%d\n",regname[rt],regname[rt],regname[rs2],32-imm);
    output_w32(0xe1800000|rd_rn_rm(rt,rt,rs2)|((32-imm)<<7));
}

void emit_shl(u_int rs,u_int shift,u_int rt)
{
    assert(rs<16);
    assert(rt<16);
    assert(shift<16);
    //if(imm==1) ...
    assem_debug("lsl %s,%s,%s\n",regname[rt],regname[rs],regname[shift]);
    output_w32(0xe1a00000|rd_rn_rm(rt,0,rs)|0x10|(shift<<8));
}
void emit_shr(u_int rs,u_int shift,u_int rt)
{
    assert(rs<16);
    assert(rt<16);
    assert(shift<16);
    assem_debug("lsr %s,%s,%s\n",regname[rt],regname[rs],regname[shift]);
    output_w32(0xe1a00000|rd_rn_rm(rt,0,rs)|0x30|(shift<<8));
}
void emit_sar(u_int rs,u_int shift,u_int rt)
{
    assert(rs<16);
    assert(rt<16);
    assert(shift<16);
    assem_debug("asr %s,%s,%s\n",regname[rt],regname[rs],regname[shift]);
    output_w32(0xe1a00000|rd_rn_rm(rt,0,rs)|0x50|(shift<<8));
}
void emit_shlcl(int r)
{
    assem_debug("shl %%%s,%%cl\n",regname[r]);
    assert(0);
}
void emit_shrcl(int r)
{
    assem_debug("shr %%%s,%%cl\n",regname[r]);
    assert(0);
}
void emit_sarcl(int r)
{
    assem_debug("sar %%%s,%%cl\n",regname[r]);
    assert(0);
}

void emit_shldcl(int r1,int r2)
{
    assem_debug("shld %%%s,%%%s,%%cl\n",regname[r1],regname[r2]);
    assert(0);
}
void emit_shrdcl(int r1,int r2)
{
    assem_debug("shrd %%%s,%%%s,%%cl\n",regname[r1],regname[r2]);
    assert(0);
}
void emit_orrshl(u_int rs,u_int shift,u_int rt)
{
    assert(rs<16);
    assert(rt<16);
    assert(shift<16);
    assem_debug("orr %s,%s,%s,lsl %s\n",regname[rt],regname[rt],regname[rs],regname[shift]);
    output_w32(0xe1800000|rd_rn_rm(rt,rt,rs)|0x10|(shift<<8));
}
void emit_orrshr(u_int rs,u_int shift,u_int rt)
{
    assert(rs<16);
    assert(rt<16);
    assert(shift<16);
    assem_debug("orr %s,%s,%s,lsr %s\n",regname[rt],regname[rt],regname[rs],regname[shift]);
    output_w32(0xe1800000|rd_rn_rm(rt,rt,rs)|0x30|(shift<<8));
}

void emit_cmpimm(int rs,int imm)
{
    u_int armval;
    if(genimm(imm,&armval)) {
        assem_debug("cmp %s,$%d\n",regname[rs],imm);
        output_w32(0xe3500000|rd_rn_rm(0,rs,0)|armval);
    }else if(genimm(-imm,&armval)) {
        assem_debug("cmn %s,$%d\n",regname[rs],imm);
        output_w32(0xe3700000|rd_rn_rm(0,rs,0)|armval);
    }else if(imm>0) {
        assert(imm<65536);
#ifdef ARMV5_ONLY
        emit_movimm(imm,HOST_TEMPREG);
#else
        emit_movw(imm,HOST_TEMPREG);
#endif
        assem_debug("cmp %s,r14\n",regname[rs]);
        output_w32(0xe1500000|rd_rn_rm(0,rs,HOST_TEMPREG));
    }else{
        assert(imm>-65536);
#ifdef ARMV5_ONLY
        emit_movimm(-imm,HOST_TEMPREG);
#else
        emit_movw(-imm,HOST_TEMPREG);
#endif
        assem_debug("cmn %s,r14\n",regname[rs]);
        output_w32(0xe1700000|rd_rn_rm(0,rs,HOST_TEMPREG));
    }
}

void emit_cmovne(u_int *addr,int rt)
{
    assem_debug("cmovne %x,%%%s",(int)addr,regname[rt]);
    assert(0);
}
void emit_cmovl(u_int *addr,int rt)
{
    assem_debug("cmovl %x,%%%s",(int)addr,regname[rt]);
    assert(0);
}
void emit_cmovs(u_int *addr,int rt)
{
    assem_debug("cmovs %x,%%%s",(int)addr,regname[rt]);
    assert(0);
}
void emit_cmovne_imm(int imm,int rt)
{
    assem_debug("movne %s,#%d\n",regname[rt],imm);
    u_int armval;
    assert(genimm(imm,&armval));
    output_w32(0x13a00000|rd_rn_rm(rt,0,0)|armval);
}
void emit_cmovl_imm(int imm,int rt)
{
    assem_debug("movlt %s,#%d\n",regname[rt],imm);
    u_int armval;
    assert(genimm(imm,&armval));
    output_w32(0xb3a00000|rd_rn_rm(rt,0,0)|armval);
}
void emit_cmovb_imm(int imm,int rt)
{
    assem_debug("movcc %s,#%d\n",regname[rt],imm);
    u_int armval;
    assert(genimm(imm,&armval));
    output_w32(0x33a00000|rd_rn_rm(rt,0,0)|armval);
}
void emit_cmovs_imm(int imm,int rt)
{
    assem_debug("movmi %s,#%d\n",regname[rt],imm);
    u_int armval;
    assert(genimm(imm,&armval));
    output_w32(0x43a00000|rd_rn_rm(rt,0,0)|armval);
}
void emit_cmovne_reg(int rs,int rt)
{
    assem_debug("movne %s,%s\n",regname[rt],regname[rs]);
    output_w32(0x11a00000|rd_rn_rm(rt,0,rs));
}
void emit_cmovl_reg(int rs,int rt)
{
    assem_debug("movlt %s,%s\n",regname[rt],regname[rs]);
    output_w32(0xb1a00000|rd_rn_rm(rt,0,rs));
}
void emit_cmovs_reg(int rs,int rt)
{
    assem_debug("movmi %s,%s\n",regname[rt],regname[rs]);
    output_w32(0x41a00000|rd_rn_rm(rt,0,rs));
}

void emit_slti32(int rs,int imm,int rt)
{
    if(rs!=rt) emit_zeroreg(rt);
    emit_cmpimm(rs,imm);
    if(rs==rt) emit_movimm(0,rt);
    emit_cmovl_imm(1,rt);
}
void emit_sltiu32(int rs,int imm,int rt)
{
    if(rs!=rt) emit_zeroreg(rt);
    emit_cmpimm(rs,imm);
    if(rs==rt) emit_movimm(0,rt);
    emit_cmovb_imm(1,rt);
}
void emit_slti64_32(int rsh,int rsl,int imm,int rt)
{
    assert(rsh!=rt);
    emit_slti32(rsl,imm,rt);
    if(imm>=0)
    {
        emit_test(rsh,rsh);
        emit_cmovne_imm(0,rt);
        emit_cmovs_imm(1,rt);
    }
    else
    {
        emit_cmpimm(rsh,-1);
        emit_cmovne_imm(0,rt);
        emit_cmovl_imm(1,rt);
    }
}
void emit_sltiu64_32(int rsh,int rsl,int imm,int rt)
{
    assert(rsh!=rt);
    emit_sltiu32(rsl,imm,rt);
    if(imm>=0)
    {
        emit_test(rsh,rsh);
        emit_cmovne_imm(0,rt);
    }
    else
    {
        emit_cmpimm(rsh,-1);
        emit_cmovne_imm(1,rt);
    }
}

void emit_cmp(int rs,int rt)
{
    assem_debug("cmp %s,%s\n",regname[rs],regname[rt]);
    output_w32(0xe1500000|rd_rn_rm(0,rs,rt));
}
void emit_set_gz32(int rs, int rt)
{
    //assem_debug("set_gz32\n");
    emit_cmpimm(rs,1);
    emit_movimm(1,rt);
    emit_cmovl_imm(0,rt);
}
void emit_set_nz32(int rs, int rt)
{
    //assem_debug("set_nz32\n");
    if(rs!=rt) emit_movs(rs,rt);
    else emit_test(rs,rs);
    emit_cmovne_imm(1,rt);
}
void emit_set_gz64_32(int rsh, int rsl, int rt)
{
    //assem_debug("set_gz64\n");
    assert(rsl!=rt);
    emit_set_gz32(rsl,rt);
    emit_test(rsh,rsh);
    emit_cmovne_imm(1,rt);
    emit_cmovs_imm(0,rt);
}
void emit_set_nz64_32(int rsh, int rsl, int rt)
{
    //assem_debug("set_nz64\n");
    emit_or_and_set_flags(rsh,rsl,rt);
    emit_cmovne_imm(1,rt);
}
void emit_set_if_less32(int rs1, int rs2, int rt)
{
    //assem_debug("set if less (%%%s,%%%s),%%%s\n",regname[rs1],regname[rs2],regname[rt]);
    if(rs1!=rt&&rs2!=rt) emit_zeroreg(rt);
    emit_cmp(rs1,rs2);
    if(rs1==rt||rs2==rt) emit_movimm(0,rt);
    emit_cmovl_imm(1,rt);
}
void emit_set_if_carry32(int rs1, int rs2, int rt)
{
    //assem_debug("set if carry (%%%s,%%%s),%%%s\n",regname[rs1],regname[rs2],regname[rt]);
    if(rs1!=rt&&rs2!=rt) emit_zeroreg(rt);
    emit_cmp(rs1,rs2);
    if(rs1==rt||rs2==rt) emit_movimm(0,rt);
    emit_cmovb_imm(1,rt);
}
void emit_set_if_less64_32(int u1, int l1, int u2, int l2, int rt)
{
    //assem_debug("set if less64 (%%%s,%%%s,%%%s,%%%s),%%%s\n",regname[u1],regname[l1],regname[u2],regname[l2],regname[rt]);
    assert(u1!=rt);
    assert(u2!=rt);
    emit_cmp(l1,l2);
    emit_movimm(0,rt);
    emit_sbcs(u1,u2,HOST_TEMPREG);
    emit_cmovl_imm(1,rt);
}
void emit_set_if_carry64_32(int u1, int l1, int u2, int l2, int rt)
{
    //assem_debug("set if carry64 (%%%s,%%%s,%%%s,%%%s),%%%s\n",regname[u1],regname[l1],regname[u2],regname[l2],regname[rt]);
    assert(u1!=rt);
    assert(u2!=rt);
    emit_cmp(l1,l2);
    emit_movimm(0,rt);
    emit_sbcs(u1,u2,HOST_TEMPREG);
    emit_cmovb_imm(1,rt);
}

void emit_call(int a)
{
    assem_debug("bl %x (%x+%x)\n",a,(int)out,a-(int)out-8);
    u_int offset=genjmp(a);
    output_w32(0xeb000000|offset);
}

void emit_jmp(int a)
{
    assem_debug("b %x (%x+%x)\n",a,(int)out,a-(int)out-8);
    u_int offset=genjmp(a);
    output_w32(0xea000000|offset);
}
void emit_jne(int a)
{
    assem_debug("bne %x\n",a);
    u_int offset=genjmp(a);
    output_w32(0x1a000000|offset);
}
void emit_jeq(int a)
{
    assem_debug("beq %x\n",a);
    u_int offset=genjmp(a);
    output_w32(0x0a000000|offset);
}
void emit_js(int a)
{
    assem_debug("bmi %x\n",a);
    u_int offset=genjmp(a);
    output_w32(0x4a000000|offset);
}
void emit_jns(int a)
{
    assem_debug("bpl %x\n",a);
    u_int offset=genjmp(a);
    output_w32(0x5a000000|offset);
}
void emit_jl(int a)
{
    assem_debug("blt %x\n",a);
    u_int offset=genjmp(a);
    output_w32(0xba000000|offset);
}
void emit_jge(int a)
{
    assem_debug("bge %x\n",a);
    u_int offset=genjmp(a);
    output_w32(0xaa000000|offset);
}
void emit_jno(int a)
{
    assem_debug("bvc %x\n",a);
    u_int offset=genjmp(a);
    output_w32(0x7a000000|offset);
}
void emit_jcc(int a)
{
    assem_debug("bcc %x\n",a);
    u_int offset=genjmp(a);
    output_w32(0x3a000000|offset);
}

void emit_pushimm(int imm)
{
    assem_debug("push $%x\n",imm);
    assert(0);
}
void emit_pusha()
{
    assem_debug("pusha\n");
    assert(0);
}
void emit_popa()
{
    assem_debug("popa\n");
    assert(0);
}
void emit_pushreg(u_int r)
{
    assem_debug("push %%%s\n",regname[r]);
    assert(0);
}
void emit_popreg(u_int r)
{
    assem_debug("pop %%%s\n",regname[r]);
    assert(0);
}
void emit_callreg(u_int r)
{
    assem_debug("call *%%%s\n",regname[r]);
    assert(0);
}
void emit_jmpreg(u_int r)
{
    assem_debug("jmp *%%%s\n",regname[r]);
    assert(0);
}

void emit_readword_indexed(int offset, int rs, int rt)
{
    assert(offset>-4096&&offset<4096);
    assem_debug("ldr %s,%s+%d\n",regname[rt],regname[rs],offset);
    if(offset>=0) {
        output_w32(0xe5900000|rd_rn_rm(rt,rs,0)|offset);
    }else{
        output_w32(0xe5100000|rd_rn_rm(rt,rs,0)|(-offset));
    }
}
void emit_readword_dualindexedx4(int rs1, int rs2, int rt)
{
    assem_debug("ldr %s,%s+%s<<2\n",regname[rt],regname[rs1],regname[rs2]);
    output_w32(0xe7900000|rd_rn_rm(rt,rs1,rs2)|0x100);
}
void emit_movsbl_indexed(int offset, int rs, int rt)
{
    assert(offset>-256&&offset<256);
    assem_debug("ldrsb %s,%s+%d\n",regname[rt],regname[rs],offset);
    if(offset>=0) {
        output_w32(0xe1d000d0|rd_rn_rm(rt,rs,0)|((offset<<4)&0xf00)|(offset&0xf));
    }else{
        output_w32(0xe15000d0|rd_rn_rm(rt,rs,0)|(((-offset)<<4)&0xf00)|((-offset)&0xf));
    }
}
void emit_movswl_indexed(int offset, int rs, int rt)
{
    assert(offset>-256&&offset<256);
    assem_debug("ldrsh %s,%s+%d\n",regname[rt],regname[rs],offset);
    if(offset>=0) {
        output_w32(0xe1d000f0|rd_rn_rm(rt,rs,0)|((offset<<4)&0xf00)|(offset&0xf));
    }else{
        output_w32(0xe15000f0|rd_rn_rm(rt,rs,0)|(((-offset)<<4)&0xf00)|((-offset)&0xf));
    }
}
void emit_movzbl_indexed(int offset, int rs, int rt)
{
    assert(offset>-4096&&offset<4096);
    assem_debug("ldrb %s,%s+%d\n",regname[rt],regname[rs],offset);
    if(offset>=0) {
        output_w32(0xe5d00000|rd_rn_rm(rt,rs,0)|offset);
    }else{
        output_w32(0xe5500000|rd_rn_rm(rt,rs,0)|(-offset));
    }
}
void emit_movzwl_indexed(int offset, int rs, int rt)
{
    assert(offset>-256&&offset<256);
    assem_debug("ldrh %s,%s+%d\n",regname[rt],regname[rs],offset);
    if(offset>=0) {
        output_w32(0xe1d000b0|rd_rn_rm(rt,rs,0)|((offset<<4)&0xf00)|(offset&0xf));
    }else{
        output_w32(0xe15000b0|rd_rn_rm(rt,rs,0)|(((-offset)<<4)&0xf00)|((-offset)&0xf));
    }
}

void emit_readword_indexed_rdram(int offset, int rs, int rt)
{
    assert(offset>-4096&&offset<4096);
    assem_debug("ldr %s,%s+%d\n",regname[rt],regname[rs],offset);
    
    if(offset>=0)
    {
        output_w32(0xe88d0000|(1<<9));
        emit_movimm((u_int)(rdram), 9);
        emit_add(rs, 9, 9);
        output_w32(0xe3c00000|rd_rn_rm(9,9,0)|0x2|(1<<8));
#if 0
        save_regs_debug(0x100f);
        emit_movimm(0x42, 0);
        emit_call((int)testthat);
        restore_regs_debug(0x100f);
        
        save_regs_debug(0x100f);
        emit_movimm(rs, 0);
        emit_call((int)testthat);
        restore_regs_debug(0x100f);
        
        save_regs_debug(0x100f);
        emit_movimm(9, 0);
        emit_call((int)testthat);
        restore_regs_debug(0x100f);
#endif
        output_w32(0xe5900000|rd_rn_rm(rt,9,0)|offset);
        
        output_w32(0xe89d0000|(1<<9));
    }
    else
    {
        output_w32(0xe88d0000|(1<<9));
        emit_movimm((u_int)(rdram), 9);
        emit_add(rs, 9, 9);
        output_w32(0xe3c00000|rd_rn_rm(9,9,0)|0x2|(1<<8));
        
        output_w32(0xe5100000|rd_rn_rm(rt,9,0)|(-offset));
        
        output_w32(0xe89d0000|(1<<9));
    }
}
void emit_readword_dualindexedx4_rdram(int rs1, int rs2, int rt)
{
    assem_debug("NA!!! ldr %s,%s+%s<<2\n",regname[rt],regname[rs1],regname[rs2]);
    output_w32(0xe7900000|rd_rn_rm(rt,rs1,rs2)|0x100);
}
void emit_movsbl_indexed_rdram(int offset, int rs, int rt)
{
    assert(offset>-256&&offset<256);
    assem_debug("ldrsb %s,%s+%d\n",regname[rt],regname[rs],offset);
    
    
    if(offset>=0) {
        output_w32(0xe88d0000|(1<<9));
        emit_movimm((u_int)(rdram), 9);
        emit_add(rs, 9, 9);
        output_w32(0xe3c00000|rd_rn_rm(9,9,0)|0x2|(1<<8));
        
        output_w32(0xe1d000d0|rd_rn_rm(rt,9,0)|(((offset<<4)&0xf00)|(offset&0xf)));
        
        output_w32(0xe89d0000|(1<<9));
    }else{
        output_w32(0xe88d0000|(1<<9));
        emit_movimm((u_int)(rdram), 9);
        emit_add(rs, 9, 9);
        output_w32(0xe3c00000|rd_rn_rm(9,9,0)|0x2|(1<<8));
        
        output_w32(0xe15000d0|rd_rn_rm(rt,9,0)|(((-offset)<<4)&0xf00)|((-offset)&0xf));
        
        output_w32(0xe89d0000|(1<<9));
    }
}
void emit_movswl_indexed_rdram(int offset, int rs, int rt)
{
    assert(offset>-256&&offset<256);
    assem_debug("ldrsh %s,%s+%d\n",regname[rt],regname[rs],offset);
    if(offset>=0) {
        output_w32(0xe88d0000|(1<<9));
        emit_movimm((u_int)(rdram), 9);
        emit_add(rs, 9, 9);
        output_w32(0xe3c00000|rd_rn_rm(9,9,0)|0x2|(1<<8));
        
        output_w32(0xe1d000f0|rd_rn_rm(rt,9,0)|((offset<<4)&0xf00)|(offset&0xf));
        
        output_w32(0xe89d0000|(1<<9));
    }else{
        output_w32(0xe88d0000|(1<<9));
        emit_movimm((u_int)(rdram), 9);
        emit_add(rs, 9, 9);
        output_w32(0xe3c00000|rd_rn_rm(9,9,0)|0x2|(1<<8));
        
        output_w32(0xe15000f0|rd_rn_rm(rt,9,0)|(((-offset)<<4)&0xf00)|((-offset)&0xf));
        
        output_w32(0xe89d0000|(1<<9));
    }
}
void emit_movzbl_indexed_rdram(int offset, int rs, int rt)
{
    assert(offset>-4096&&offset<4096);
    assem_debug("ldrb %s,%s+%d\n",regname[rt],regname[rs],offset);
    if(offset>=0) {
        output_w32(0xe88d0000|(1<<9));
        emit_movimm((u_int)(rdram), 9);
        emit_add(rs, 9, 9);
        output_w32(0xe3c00000|rd_rn_rm(9,9,0)|0x2|(1<<8));
        
        output_w32(0xe5d00000|rd_rn_rm(rt,9,0)|offset);
        
        output_w32(0xe89d0000|(1<<9));
    }else{
        output_w32(0xe88d0000|(1<<9));
        emit_movimm((u_int)(rdram), 9);
        emit_add(rs, 9, 9);
        output_w32(0xe3c00000|rd_rn_rm(9,9,0)|0x2|(1<<8));
        
        output_w32(0xe5500000|rd_rn_rm(rt,9,0)|(-offset));
        
        output_w32(0xe89d0000|(1<<9));
    }
}
void emit_movzwl_indexed_rdram(int offset, int rs, int rt)
{
    assert(offset>-256&&offset<256);
    assem_debug("ldrh %s,%s+%d\n",regname[rt],regname[rs],offset);
    if(offset>=0) {
        output_w32(0xe88d0000|(1<<9));
        emit_movimm((u_int)(rdram), 9);
        emit_add(rs, 9, 9);
        output_w32(0xe3c00000|rd_rn_rm(9,9,0)|0x2|(1<<8));
        
        output_w32(0xe1d000b0|rd_rn_rm(rt,9,0)|((offset<<4)&0xf00)|(offset&0xf));
        
        output_w32(0xe89d0000|(1<<9));
    }else{
        output_w32(0xe88d0000|(1<<9));
        emit_movimm((u_int)(rdram), 9);
        emit_add(rs, 9, 9);
        output_w32(0xe3c00000|rd_rn_rm(9,9,0)|0x2|(1<<8));
        
        output_w32(0xe15000b0|rd_rn_rm(rt,9,0)|(((-offset)<<4)&0xf00)|((-offset)&0xf));
        
        output_w32(0xe89d0000|(1<<9));
    }
}
void emit_readword(int addr, int rt)
{
    u_int offset = addr-(u_int)&dynarec_local;
    assert(offset<4096);
    assem_debug("ldr %s,fp+%d\n",regname[rt],offset);
    output_w32(0xe5900000|rd_rn_rm(rt,FP,0)|offset);
}
void emit_movsbl(int addr, int rt)
{
    u_int offset = addr-(u_int)&dynarec_local;
    assert(offset<256);
    assem_debug("ldrsb %s,fp+%d\n",regname[rt],offset);
    output_w32(0xe1d000d0|rd_rn_rm(rt,FP,0)|((offset<<4)&0xf00)|(offset&0xf));
}
void emit_movswl(int addr, int rt)
{
    u_int offset = addr-(u_int)&dynarec_local;
    assert(offset<256);
    assem_debug("ldrsh %s,fp+%d\n",regname[rt],offset);
    output_w32(0xe1d000f0|rd_rn_rm(rt,FP,0)|((offset<<4)&0xf00)|(offset&0xf));
}
void emit_movzbl(int addr, int rt)
{
    u_int offset = addr-(u_int)&dynarec_local;
    assert(offset<4096);
    assem_debug("ldrb %s,fp+%d\n",regname[rt],offset);
    output_w32(0xe5d00000|rd_rn_rm(rt,FP,0)|offset);
}
void emit_movzwl(int addr, int rt)
{
    u_int offset = addr-(u_int)&dynarec_local;
    assert(offset<256);
    assem_debug("ldrh %s,fp+%d\n",regname[rt],offset);
    output_w32(0xe1d000b0|rd_rn_rm(rt,FP,0)|((offset<<4)&0xf00)|(offset&0xf));
}
void emit_movzwl_reg(int rs, int rt)
{
    assem_debug("movzwl %%%s,%%%s\n",regname[rs]+1,regname[rt]);
    assert(0);
}

void emit_xchg(int rs, int rt)
{
    assem_debug("xchg %%%s,%%%s\n",regname[rs],regname[rt]);
    assert(0);
}



void emit_writeword_indexed_rdram(int rt, int offset, int rs)
{
    assert(offset>-4096&&offset<4096);
    assem_debug("str %s,%s+%d\n",regname[rt],regname[rs],offset);
    if(offset>=0) {
        output_w32(0xe88d0000|(1<<9));
        emit_movimm((u_int)(rdram), 9);
        emit_add(rs, 9, 9);
        output_w32(0xe3c00000|rd_rn_rm(9,9,0)|0x2|(1<<8));
        
        output_w32(0xe5800000|rd_rn_rm(rt,9,0)|offset);
        
        output_w32(0xe89d0000|(1<<9));
    }else{
        output_w32(0xe88d0000|(1<<9));
        emit_movimm((u_int)(rdram), 9);
        emit_add(rs, 9, 9);
        output_w32(0xe3c00000|rd_rn_rm(9,9,0)|0x2|(1<<8));
        
        output_w32(0xe5000000|rd_rn_rm(rt,9,0)|(-offset));
        
        output_w32(0xe89d0000|(1<<9));
    }
}
void emit_writehword_indexed_rdram(int rt, int offset, int rs)
{
    assert(offset>-256&&offset<256);
    assem_debug("strh %s,%s+%d\n",regname[rt],regname[rs],offset);
    if(offset>=0) {
        output_w32(0xe88d0000|(1<<9));
        emit_movimm((u_int)(rdram), 9);
        emit_add(rs, 9, 9);
        output_w32(0xe3c00000|rd_rn_rm(9,9,0)|0x2|(1<<8));
        
        output_w32(0xe1c000b0|rd_rn_rm(rt,9,0)|((offset<<4)&0xf00)|(offset&0xf));
        
        output_w32(0xe89d0000|(1<<9));
    }else{
        output_w32(0xe88d0000|(1<<9));
        emit_movimm((u_int)(rdram), 9);
        emit_add(rs, 9, 9);
        output_w32(0xe3c00000|rd_rn_rm(9,9,0)|0x2|(1<<8));
        
        output_w32(0xe14000b0|rd_rn_rm(rt,9,0)|(((-offset)<<4)&0xf00)|((-offset)&0xf));
        
        output_w32(0xe89d0000|(1<<9));
    }
}
void emit_writebyte_indexed_rdram(int rt, int offset, int rs)
{
    assert(offset>-4096&&offset<4096);
    assem_debug("strb %s,%s+%d\n",regname[rt],regname[rs],offset);
    if(offset>=0) {
        output_w32(0xe88d0000|(1<<9));
        emit_movimm((u_int)(rdram), 9);
        emit_add(rs, 9, 9);
        output_w32(0xe3c00000|rd_rn_rm(9,9,0)|0x2|(1<<8));
        
        output_w32(0xe5c00000|rd_rn_rm(rt,9,0)|offset);
        
        output_w32(0xe89d0000|(1<<9));
    }else{
        output_w32(0xe88d0000|(1<<9));
        emit_movimm((u_int)(rdram), 9);
        emit_add(rs, 9, 9);
        output_w32(0xe3c00000|rd_rn_rm(9,9,0)|0x2|(1<<8));
        
        output_w32(0xe5400000|rd_rn_rm(rt,9,0)|(-offset));
        
        output_w32(0xe89d0000|(1<<9));
    }
}

void emit_writeword_indexed(int rt, int offset, int rs)
{
    assert(offset>-4096&&offset<4096);
    assem_debug("str %s,%s+%d\n",regname[rt],regname[rs],offset);
    if(offset>=0) {
        output_w32(0xe5800000|rd_rn_rm(rt,rs,0)|offset);
    }else{
        output_w32(0xe5000000|rd_rn_rm(rt,rs,0)|(-offset));
    }
}
void emit_writehword_indexed(int rt, int offset, int rs)
{
    assert(offset>-256&&offset<256);
    assem_debug("strh %s,%s+%d\n",regname[rt],regname[rs],offset);
    if(offset>=0) {
        output_w32(0xe1c000b0|rd_rn_rm(rt,rs,0)|((offset<<4)&0xf00)|(offset&0xf));
    }else{
        output_w32(0xe14000b0|rd_rn_rm(rt,rs,0)|(((-offset)<<4)&0xf00)|((-offset)&0xf));
    }
}
void emit_writebyte_indexed(int rt, int offset, int rs)
{
    assert(offset>-4096&&offset<4096);
    assem_debug("strb %s,%s+%d\n",regname[rt],regname[rs],offset);
    if(offset>=0) {
        output_w32(0xe5c00000|rd_rn_rm(rt,rs,0)|offset);
    }else{
        output_w32(0xe5400000|rd_rn_rm(rt,rs,0)|(-offset));
    }
}

void emit_writeword(int rt, int addr)
{
    u_int offset = addr-(u_int)&dynarec_local;
    assert(offset<4096);
    assem_debug("str %s,fp+%d\n",regname[rt],offset);
    output_w32(0xe5800000|rd_rn_rm(rt,FP,0)|offset);
}
void emit_writehword(int rt, int addr)
{
    u_int offset = addr-(u_int)&dynarec_local;
    assert(offset<256);
    assem_debug("strh %s,fp+%d\n",regname[rt],offset);
    output_w32(0xe1c000b0|rd_rn_rm(rt,FP,0)|((offset<<4)&0xf00)|(offset&0xf));
}
void emit_writebyte(int rt, int addr)
{
    u_int offset = addr-(u_int)&dynarec_local;
    assert(offset<4096);
    assem_debug("str %s,fp+%d\n",regname[rt],offset);
    output_w32(0xe5c00000|rd_rn_rm(rt,FP,0)|offset);
}
void emit_writeword_imm(int imm, int addr)
{
    assem_debug("movl $%x,%x\n",imm,addr);
    assert(0);
}
void emit_writebyte_imm(int imm, int addr)
{
    assem_debug("movb $%x,%x\n",imm,addr);
    assert(0);
}

void emit_mul(int rs)
{
    assem_debug("mul %%%s\n",regname[rs]);
    assert(0);
}
void emit_imul(int rs)
{
    assem_debug("imul %%%s\n",regname[rs]);
    assert(0);
}
void emit_umull(u_int rs1, u_int rs2, u_int hi, u_int lo)
{
    assem_debug("umull %s, %s, %s, %s\n",regname[lo],regname[hi],regname[rs1],regname[rs2]);
    assert(rs1<16);
    assert(rs2<16);
    assert(hi<16);
    assert(lo<16);
    output_w32(0xe0800090|(hi<<16)|(lo<<12)|(rs2<<8)|rs1);
}
void emit_smull(u_int rs1, u_int rs2, u_int hi, u_int lo)
{
    assem_debug("smull %s, %s, %s, %s\n",regname[lo],regname[hi],regname[rs1],regname[rs2]);
    assert(rs1<16);
    assert(rs2<16);
    assert(hi<16);
    assert(lo<16);
    output_w32(0xe0c00090|(hi<<16)|(lo<<12)|(rs2<<8)|rs1);
}

void emit_div(int rs)
{
    assem_debug("div %%%s\n",regname[rs]);
    assert(0);
}
void emit_idiv(int rs)
{
    assem_debug("idiv %%%s\n",regname[rs]);
    assert(0);
}
void emit_cdq()
{
    assem_debug("cdq\n");
    assert(0);
}

void emit_clz(int rs,int rt)
{
    assem_debug("clz %s,%s\n",regname[rt],regname[rs]);
    output_w32(0xe16f0f10|rd_rn_rm(rt,0,rs));
}

void emit_subcs(int rs1,int rs2,int rt)
{
    assem_debug("subcs %s,%s,%s\n",regname[rt],regname[rs1],regname[rs2]);
    output_w32(0x20400000|rd_rn_rm(rt,rs1,rs2));
}

void emit_shrcc_imm(int rs,u_int imm,int rt)
{
    assert(imm>0);
    assert(imm<32);
    assem_debug("lsrcc %s,%s,#%d\n",regname[rt],regname[rs],imm);
    output_w32(0x31a00000|rd_rn_rm(rt,0,rs)|0x20|(imm<<7));
}

void emit_negmi(int rs, int rt)
{
    assem_debug("rsbmi %s,%s,#0\n",regname[rt],regname[rs]);
    output_w32(0x42600000|rd_rn_rm(rt,rs,0));
}

void emit_negsmi(int rs, int rt)
{
    assem_debug("rsbsmi %s,%s,#0\n",regname[rt],regname[rs]);
    output_w32(0x42700000|rd_rn_rm(rt,rs,0));
}

void emit_teq(int rs, int rt)
{
    assem_debug("teq %s,%s\n",regname[rs],regname[rt]);
    output_w32(0xe1300000|rd_rn_rm(0,rs,rt));
}

void emit_rsbimm(int rs, int imm, int rt)
{
    u_int armval;
    assert(genimm(imm,&armval));
    assem_debug("rsb %s,%s,#%d\n",regname[rt],regname[rs],imm);
    output_w32(0xe2600000|rd_rn_rm(rt,rs,0)|armval);
}

// special case for checking invalid_code
void emit_cmpmem_indexed_rdramsr12_imm(int addr,int r,int imm)
{
    assert(imm<128&&imm>=-127);
    assert(r>=0&&r<8);
    emit_shrimm(r,12,r);
    assem_debug("cmp %x+%%%s,%d\n",addr,regname[r],imm);
    output_byte(0x80);
    output_modrm(2,r,7);
    output_w32(((u_int)rdram)+((u_int)addr&(~0x80000000)));
    output_byte(imm);
}

// special case for checking invalid_code
void emit_cmpmem_indexed_rdramsr12_reg(int base,int r,int imm)
{
#if 0
    assert(imm<128&&imm>=0);
    assert(r>=0&&r<16);
    assem_debug("ldrb lr,%s,%s lsr #12\n",regname[base],regname[r]);
    output_w32(0xe7d00000|rd_rn_rm(HOST_TEMPREG,base,r)|0x620);
    emit_cmpimm(HOST_TEMPREG,imm);
#else
    assert(imm<128&&imm>=0);
    assert(r>=0&&r<16);
    assem_debug("ldrb lr,%s,%s lsr #12\n",regname[base],regname[r]);
    
    // Assumes memory is less than 0x80000000
    // Uses r9 as temp reg
    output_w32(0xe88d0000|(1<<9));
    emit_movimm((u_int)(rdram), 9);
    emit_add(base, 9, 9);
    output_w32(0xe3c00000|rd_rn_rm(9,9,0)|0x2|(1<<8));
    
    output_w32(0xe7d00000|rd_rn_rm(HOST_TEMPREG,9,r)|0x620);
    output_w32(0xe89d0000|(1<<9));
    
    emit_cmpimm(HOST_TEMPREG,imm);
#endif
}

// special case for checking hash_table
void emit_cmpmem_indexed_rdram(int addr,int rs,int rt)
{
    assert(rs>=0&&rs<8);
    assert(rt>=0&&rt<8);
    assem_debug("cmp %x+%%%s,%%%s\n",addr,regname[rs],regname[rt]);
    output_byte(0x39);
    output_modrm(2,rs,rt);
    output_w32(((u_int)rdram)+((u_int)addr&(~0x80000000)));
}

// special case for checking invalid_code
void emit_cmpmem_indexedsr12_imm(int addr,int r,int imm)
{
    assert(imm<128&&imm>=-127);
    assert(r>=0&&r<8);
    emit_shrimm(r,12,r);
    assem_debug("cmp %x+%%%s,%d\n",addr,regname[r],imm);
    output_byte(0x80);
    output_modrm(2,r,7);
    output_w32(addr);
    output_byte(imm);
}

// special case for checking invalid_code
void emit_cmpmem_indexedsr12_reg(int base,int r,int imm)
{
    assert(imm<128&&imm>=0);
    assert(r>=0&&r<16);
    assem_debug("ldrb lr,%s,%s lsr #12\n",regname[base],regname[r]);
    output_w32(0xe7d00000|rd_rn_rm(HOST_TEMPREG,base,r)|0x620);
    emit_cmpimm(HOST_TEMPREG,imm);
}

// special case for checking hash_table
void emit_cmpmem_indexed(int addr,int rs,int rt)
{
    assert(rs>=0&&rs<8);
    assert(rt>=0&&rt<8);
    assem_debug("cmp %x+%%%s,%%%s\n",addr,regname[rs],regname[rt]);
    output_byte(0x39);
    output_modrm(2,rs,rt);
    output_w32(addr);
}

// Used to preload hash table entries
void emit_prefetch(void *addr)
{
    assem_debug("prefetch %x\n",(int)addr);
    output_byte(0x0F);
    output_byte(0x18);
    output_modrm(0,5,1);
    output_w32((u_int)addr);
}
void emit_prefetchreg(int r)
{
    assem_debug("pld %s\n",regname[r]);
    output_w32(0xf5d0f000|rd_rn_rm(0,r,0));
}

void emit_submem(int r,int addr)
{
    assert(r>=0&&r<8);
    assem_debug("sub %x,%%%s\n",addr,regname[r]);
    output_byte(0x2B);
    output_modrm(0,5,r);
    output_w32((u_int)addr);
}

// Save registers before function call
void save_regs(u_int reglist)
{
    reglist&=0x100f; // only save the caller-save registers, r0-r3, r12
    if(!reglist) return;
    assem_debug("stmia fp,{");
    if(reglist&1) assem_debug("r0, ");
    if(reglist&2) assem_debug("r1, ");
    if(reglist&4) assem_debug("r2, ");
    if(reglist&8) assem_debug("r3, ");
    if(reglist&0x1000) assem_debug("r12");
    assem_debug("}\n");
    output_w32(0xe88b0000|reglist);
}
// Restore registers after function call
void restore_regs(u_int reglist)
{
    reglist&=0x100f; // only restore the caller-save registers, r0-r3, r12
    if(!reglist) return;
    assem_debug("ldmia fp,{");
    if(reglist&1) assem_debug("r0, ");
    if(reglist&2) assem_debug("r1, ");
    if(reglist&4) assem_debug("r2, ");
    if(reglist&8) assem_debug("r3, ");
    if(reglist&0x1000) assem_debug("r12");
    assem_debug("}\n");
    output_w32(0xe89b0000|reglist);
}

void save_regs_debug(u_int reglist)
{
    reglist&=0x100f; // only save the caller-save registers, r0-r3, r12
    if(!reglist) return;
    assem_debug("stmia fp,{");
    if(reglist&1) assem_debug("r0, ");
    if(reglist&2) assem_debug("r1, ");
    if(reglist&4) assem_debug("r2, ");
    if(reglist&8) assem_debug("r3, ");
    if(reglist&0x1000) assem_debug("r12");
    assem_debug("}\n");
    output_w32(0xe88d0000|reglist);
}
// Restore registers after function call
void restore_regs_debug(u_int reglist)
{
    reglist&=0x100f; // only restore the caller-save registers, r0-r3, r12
    if(!reglist) return;
    assem_debug("ldmia fp,{");
    if(reglist&1) assem_debug("r0, ");
    if(reglist&2) assem_debug("r1, ");
    if(reglist&4) assem_debug("r2, ");
    if(reglist&8) assem_debug("r3, ");
    if(reglist&0x1000) assem_debug("r12");
    assem_debug("}\n");
    output_w32(0xe89d0000|reglist);
}


/* Special assem */

void shift_assemble_arm(int i,signed char regmap[])
{
    if(rt1[i]) {
        if(opcode2[i]<=0x07) // SLLV/SRLV/SRAV
        {
            signed char s,t,shift;
            t=get_reg(regmap,rt1[i]);
            s=get_reg(regmap,rs1[i]);
            shift=get_reg(regmap,rs2[i]);
            if(t>=0){
                if(rs1[i]==0)
                {
                    emit_zeroreg(t);
                }
                else if(rs2[i]==0)
                {
                    assert(s>=0);
                    if(s!=t) emit_mov(s,t);
                }
                else
                {
                    emit_andimm(shift,31,HOST_TEMPREG);
                    if(opcode2[i]==4) // SLLV
                    {
                        emit_shl(s,HOST_TEMPREG,t);
                    }
                    if(opcode2[i]==6) // SRLV
                    {
                        emit_shr(s,HOST_TEMPREG,t);
                    }
                    if(opcode2[i]==7) // SRAV
                    {
                        emit_sar(s,HOST_TEMPREG,t);
                    }
                }
            }
        } else { // DSLLV/DSRLV/DSRAV
            signed char sh,sl,th,tl,shift;
            th=get_reg(regmap,rt1[i]|64);
            tl=get_reg(regmap,rt1[i]);
            sh=get_reg(regmap,rs1[i]|64);
            sl=get_reg(regmap,rs1[i]);
            shift=get_reg(regmap,rs2[i]);
            if(tl>=0){
                if(rs1[i]==0)
                {
                    emit_zeroreg(tl);
                    if(th>=0) emit_zeroreg(th);
                }
                else if(rs2[i]==0)
                {
                    assert(sl>=0);
                    if(sl!=tl) emit_mov(sl,tl);
                    if(th>=0&&sh!=th) emit_mov(sh,th);
                }
                else
                {
                    // FIXME: What if shift==tl ?
                    assert(shift!=tl);
                    int temp=get_reg(regmap,-1);
                    int real_th=th;
                    if(th<0&&opcode2[i]!=0x14) {th=temp;} // DSLLV doesn't need a temporary register
                    assert(sl>=0);
                    assert(sh>=0);
                    emit_andimm(shift,31,HOST_TEMPREG);
                    if(opcode2[i]==0x14) // DSLLV
                    {
                        if(th>=0) emit_shl(sh,HOST_TEMPREG,th);
                        emit_rsbimm(HOST_TEMPREG,32,HOST_TEMPREG);
                        emit_orrshr(sl,HOST_TEMPREG,th);
                        emit_andimm(shift,31,HOST_TEMPREG);
                        emit_testimm(shift,32);
                        emit_shl(sl,HOST_TEMPREG,tl);
                        if(th>=0) emit_cmovne_reg(tl,th);
                        emit_cmovne_imm(0,tl);
                    }
                    if(opcode2[i]==0x16) // DSRLV
                    {
                        assert(th>=0);
                        emit_shr(sl,HOST_TEMPREG,tl);
                        emit_rsbimm(HOST_TEMPREG,32,HOST_TEMPREG);
                        emit_orrshl(sh,HOST_TEMPREG,tl);
                        emit_andimm(shift,31,HOST_TEMPREG);
                        emit_testimm(shift,32);
                        emit_shr(sh,HOST_TEMPREG,th);
                        emit_cmovne_reg(th,tl);
                        if(real_th>=0) emit_cmovne_imm(0,th);
                    }
                    if(opcode2[i]==0x17) // DSRAV
                    {
                        assert(th>=0);
                        emit_shr(sl,HOST_TEMPREG,tl);
                        emit_rsbimm(HOST_TEMPREG,32,HOST_TEMPREG);
                        if(real_th>=0) {
                            assert(temp>=0);
                            emit_sarimm(th,31,temp);
                        }
                        emit_orrshl(sh,HOST_TEMPREG,tl);
                        emit_andimm(shift,31,HOST_TEMPREG);
                        emit_testimm(shift,32);
                        emit_sar(sh,HOST_TEMPREG,th);
                        emit_cmovne_reg(th,tl);
                        if(real_th>=0) emit_cmovne_reg(temp,th);
                    }
                }
            }
        }
    }
}
#define shift_assemble shift_assemble_arm

void loadlr_assemble_arm(int i,signed char regmap[])
{
    int s,th,tl,temp,temp2;
    int offset;
    int jaddr;
    int memtarget,c=0;
    u_int hr,reglist=0;
    th=get_reg(regmap,rt1[i]|64);
    tl=get_reg(regmap,rt1[i]);
    s=get_reg(regmap,rs1[i]);
    temp=get_reg(regmap,-1);
    temp2=get_reg(regmap,FTEMP);
    offset=imm[i];
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9)
        {
            if(regmap[hr]>=0) reglist|=1<<hr;
        }
    }
    reglist|=1<<temp;
    if(s>=0) {
        c=(isconst[i]>>s)&1;
        memtarget=((signed int)(constmap[i][s]+offset))<(signed int)0x80800000;
    }
    if(tl>=0) {
        //assert(tl>=0);
        //assert(rt1[i]);
        if(s>=0) {
            if(c) emit_movimm(constmap[i][s]+offset,temp2);
            else emit_addimm(s,offset,temp2);
        }else{
            emit_loadreg(rs1[i],temp2);
            emit_addimm(temp2,offset,temp2);
        }
        emit_shlimm(temp2,3,temp);
        if (opcode[i]==0x22||opcode[i]==0x26) {
            emit_andimm(temp2,0xFFFFFFFC,temp2); // LWL/LWR
        }else{
            emit_andimm(temp2,0xFFFFFFF8,temp2); // LDL/LDR
        }
        emit_cmpimm(temp2,0x800000);
        jaddr=(int)out;
        emit_jno(0);
        if (opcode[i]==0x22||opcode[i]==0x26) { // LWL/LWR
            emit_readword_indexed_rdram((INT)0x80000000-0x80000000,temp2,temp2);
            add_stub(LOADW_STUB,jaddr,(int)out,(int)regmap,FTEMP,ccadj[i],reglist);
            emit_andimm(temp,24,temp);
            if (opcode[i]==0x26) emit_xorimm(temp,24,temp); // LWR
            emit_movimm(-1,HOST_TEMPREG);
            if (opcode[i]==0x26) {
                emit_shr(HOST_TEMPREG,temp,HOST_TEMPREG);
                emit_shr(temp2,temp,temp2);
            }else{
                emit_shl(HOST_TEMPREG,temp,HOST_TEMPREG);
                emit_shl(temp2,temp,temp2);
            }
            emit_not(HOST_TEMPREG,temp);
            emit_and(temp,tl,tl);
            emit_or(temp2,tl,tl);
            //emit_storereg(rt1[i],tl); // DEBUG
        }
        if (opcode[i]==0x1A||opcode[i]==0x1B) { // LDL/LDR
            assert(opcode[i]!=0x1A&&opcode[i]!=0x1B);
            if(s>=0)
                if((dirty[i]>>s)&1)
                    emit_storereg(rs1[i],s);
            if(get_reg(regmap,rs1[i]|64)>=0)
                if((dirty[i]>>get_reg(regmap,rs1[i]|64))&1)
                    emit_storereg(rs1[i]|64,get_reg(regmap,rs1[i]|64));
            int temp2h=get_reg(regmap,FTEMP|64);
            if(th>=0) emit_readword_indexed_rdram((INT)0x80000000-0x80000000,temp2,temp2h);
            emit_readword_indexed_rdram((INT)0x80000000-0x7FFFFFFC,temp2,temp2);
            add_stub(LOADD_STUB,jaddr,(int)out,(int)regmap,FTEMP,ccadj[i],reglist);
            emit_andimm(temp,56,temp);
            emit_pushreg(temp);
            emit_pushreg(temp2h);
            emit_pushreg(temp2);
            emit_pushreg(th);
            emit_pushreg(tl);
            if(opcode[i]==0x1A) emit_call((int)ldl_merge);
            if(opcode[i]==0x1B) emit_call((int)ldr_merge);
            emit_addimm(ESP,20,ESP);
            if(tl!=EDX) {
                if(tl!=EAX) emit_mov(EAX,tl);
                if(th!=EDX) emit_mov(EDX,th);
            } else
                if(th!=EAX) {
                    if(th!=EDX) emit_mov(EDX,th);
                    if(tl!=EAX) emit_mov(EAX,tl);
                } else {
                    emit_xchg(EAX,EDX);
                }
            if(s>=0) emit_loadreg(rs1[i],s);
            if(get_reg(regmap,rs1[i]|64)>=0)
                emit_loadreg(rs1[i]|64,get_reg(regmap,rs1[i]|64));
        }
    }
}
#define loadlr_assemble loadlr_assemble_arm

void cop0_assemble(int i,signed char regmap[])
{
    if(opcode2[i]==0) // MFC0
    {
        signed char t=get_reg(regmap,rt1[i]);
        char copr=(source[i]>>11)&0x1f;
        //assert(t>=0); // Why does this happen?  OOT is weird
        if(t>=0) {
            emit_addimm(FP,(int)&fake_pc-(int)&dynarec_local,0);
            emit_movimm((source[i]>>11)&0x1f,1);
            emit_writeword(0,(int)&PC);
            emit_writebyte(1,(int)&(fake_pc.f.r.nrd));
            if(copr==9) {
                emit_readword((int)&last_count,ECX);
                emit_loadreg(CCREG,HOST_CCREG); // TODO: do proper reg alloc
                emit_add(HOST_CCREG,ECX,HOST_CCREG);
                emit_addimm(HOST_CCREG,2*ccadj[i],HOST_CCREG);
                emit_writeword(HOST_CCREG,(int)&Count);
            }
            emit_call((int)MFC0);
            emit_readword((int)&readmem_dword,t);
        }
    }
    else if(opcode2[i]==4) // MTC0
    {
        signed char s=get_reg(regmap,rs1[i]);
        char copr=(source[i]>>11)&0x1f;
        assert(s>=0);
        emit_writeword(s,(int)&readmem_dword);
        wb_register(rs1[i],regmap,dirty_post[i],is32[i]);
        emit_addimm(FP,(int)&fake_pc-(int)&dynarec_local,0);
        emit_movimm((source[i]>>11)&0x1f,1);
        emit_writeword(0,(int)&PC);
        emit_writebyte(1,(int)&(fake_pc.f.r.nrd));
        if(copr==9||copr==11||copr==12) {
            emit_readword((int)&last_count,ECX);
            emit_loadreg(CCREG,HOST_CCREG); // TODO: do proper reg alloc
            emit_add(HOST_CCREG,ECX,HOST_CCREG);
            emit_addimm(HOST_CCREG,2*ccadj[i],HOST_CCREG);
            emit_writeword(HOST_CCREG,(int)&Count);
        }
        emit_call((int)MTC0);
        if(copr==9||copr==11||copr==12) {
            emit_readword((int)&Count,HOST_CCREG);
            emit_readword((int)&next_interupt,ECX);
            emit_addimm(HOST_CCREG,-2*ccadj[i],HOST_CCREG);
            emit_sub(HOST_CCREG,ECX,HOST_CCREG);
            emit_writeword(ECX,(int)&last_count);
            emit_storereg(CCREG,HOST_CCREG);
        }
        emit_loadreg(rs1[i],s);
        cop1_usable=0;
    }
    else
    {
        assert(opcode2[i]==0x10);
        if((source[i]&0x3f)==0x01) // TLBR
            emit_call((int)TLBR);
        if((source[i]&0x3f)==0x02) // TLBWI
            emit_call((int)TLBWI);
        assert((source[i]&0x3f)!=0x06); // FIXME
        //if((source[i]&0x3f)==0x06) // TLBWR
        //  emit_call((int)TLBWR);
        if((source[i]&0x3f)==0x08) // TLBP
            emit_call((int)TLBP);
        if((source[i]&0x3f)==0x18) // ERET
        {
            int count=ccadj[i];
            emit_loadreg(CCREG,HOST_CCREG);
            //emit_readword((int)&Status,1);
            emit_addimm(HOST_CCREG,2*count,HOST_CCREG); // TODO: Should there be an extra cycle here?
            emit_jmp((int)jump_eret);
            /* FIXME: Check for interrupt
             emit_readword((int)&EPC,0);
             // FIXME: Check bit 2 before clearing bit 1
             emit_andimm(1,~2,1);
             emit_writeword(1,(int)&Status);
             emit_call((int)get_addr_ht);
             emit_mov(0,15); /* Jump */
        }
    }
}

void cop1_assemble(int i,signed char regmap[])
{
    signed char rs=get_reg(regmap,CSREG);
    assert(rs>=0);
    // Check cop1 unusable
    if(!cop1_usable) {
        emit_testimm(rs,0x20000000);
        int jaddr=(int)out;
        emit_jeq(0);
        add_stub(FP_STUB,jaddr,(int)out,i,rs,is_delayslot,0);
        cop1_usable=1;
    }
    if(opcode2[i]==2) // MFC1/DMFC1/CFC1
    {
        signed char tl=get_reg(regmap,rt1[i]);
        signed char th=get_reg(regmap,rt1[i]|64);
        //assert(tl>=0);
        if(tl>=0) {
            emit_addimm(FP,(int)&fake_pc-(int)&dynarec_local,0);
            emit_movimm((source[i]>>11)&0x1f,1);
            emit_writeword(0,(int)&PC);
            emit_writebyte(1,(int)&(fake_pc.f.r.nrd));
            if(opcode2[i]==0) emit_call((int)MFC1);
            if(opcode2[i]==1) emit_call((int)DMFC1);
            if(opcode2[i]==2) emit_call((int)CFC1);
            emit_readword((int)&readmem_dword,tl);
            if(opcode2[i]==1&&th>=0) emit_readword((int)&readmem_dword+4,th);
            emit_loadreg(CSREG,rs);
        }
    }
    else if(opcode2[i]==6) // MTC1/DMTC1/CTC1
    {
        signed char sl=get_reg(regmap,rs1[i]);
        signed char sh=get_reg(regmap,rs1[i]|64);
        char copr=(source[i]>>11)&0x1f;
        assert(sl>=0);
        emit_writeword(sl,(int)&readmem_dword);
        if(opcode2[i]==5) {
            assert(sh>=0);
            emit_writeword(sh,(int)&readmem_dword+4);
        }
        wb_register(rs1[i],regmap,dirty_post[i],is32[i]);
        emit_addimm(FP,(int)&fake_pc-(int)&dynarec_local,0);
        emit_movimm((source[i]>>11)&0x1f,1);
        emit_writeword(0,(int)&PC);
        emit_writebyte(1,(int)&(fake_pc.f.r.nrd));
        if(opcode2[i]==4) emit_call((int)MTC1);
        if(opcode2[i]==5) emit_call((int)DMTC1);
        if(opcode2[i]==6) emit_call((int)CTC1);
        emit_loadreg(rs1[i],sl);
        if(sh>=0) emit_loadreg(rs1[i]|64,sh);
        emit_loadreg(CSREG,rs);
    }
    else if (opcode2[i]==0) { // MFC1
        signed char tl=get_reg(regmap,rt1[i]);
        if(tl>=0) {
            emit_readword((int)&reg_cop1_simple[(source[i]>>11)&0x1f],tl);
            emit_readword_indexed(0,tl,tl);
        }
    }
    else if (opcode2[i]==1) { // DMFC1
        signed char tl=get_reg(regmap,rt1[i]);
        signed char th=get_reg(regmap,rt1[i]|64);
        if(tl>=0) {
            emit_readword((int)&reg_cop1_double[(source[i]>>11)&0x1f],tl);
            if(th>=0) emit_readword_indexed(4,tl,th);
            emit_readword_indexed(0,tl,tl);
        }
    }
    else if (opcode2[i]==4) { // MTC1
        signed char sl=get_reg(regmap,rs1[i]);
        signed char temp=get_reg(regmap,-1);
        emit_readword((int)&reg_cop1_simple[(source[i]>>11)&0x1f],temp);
        emit_writeword_indexed(sl,0,temp);
    }
    else if (opcode2[i]==5) { // DMTC1
        signed char sl=get_reg(regmap,rs1[i]);
        signed char sh=get_reg(regmap,rs1[i]|64);
        signed char temp=get_reg(regmap,-1);
        emit_readword((int)&reg_cop1_double[(source[i]>>11)&0x1f],temp);
        emit_writeword_indexed(sh,4,temp);
        emit_writeword_indexed(sl,0,temp);
    }
}

void multdiv_assemble_arm(int i,signed char regmap[])
{
    //  case 0x18: MULT
    //  case 0x19: MULTU
    //  case 0x1A: DIV
    //  case 0x1B: DIVU
    //  case 0x1C: DMULT
    //  case 0x1D: DMULTU
    //  case 0x1E: DDIV
    //  case 0x1F: DDIVU
    if(rs1[i]&&rs2[i])
    {
        if((opcode2[i]&4)==0) // 32-bit
        {
            if(opcode2[i]==0x18) // MULT
            {
                signed char m1=get_reg(regmap,rs1[i]);
                signed char m2=get_reg(regmap,rs2[i]);
                signed char hi=get_reg(regmap,HIREG);
                signed char lo=get_reg(regmap,LOREG);
                assert(m1>=0);
                assert(m2>=0);
                assert(hi>=0);
                assert(lo>=0);
                emit_smull(m1,m2,hi,lo);
            }
            if(opcode2[i]==0x19) // MULTU
            {
                signed char m1=get_reg(regmap,rs1[i]);
                signed char m2=get_reg(regmap,rs2[i]);
                signed char hi=get_reg(regmap,HIREG);
                signed char lo=get_reg(regmap,LOREG);
                assert(m1>=0);
                assert(m2>=0);
                assert(hi>=0);
                assert(lo>=0);
                emit_umull(m1,m2,hi,lo);
            }
            if(opcode2[i]==0x1A) // DIV
            {
                signed char d1=get_reg(regmap,rs1[i]);
                signed char d2=get_reg(regmap,rs2[i]);
                assert(d1>=0);
                assert(d2>=0);
                //emit_mov(d1,EAX);
                //emit_cdq();
                //emit_idiv(d2);
                signed char quotient=get_reg(regmap,LOREG);
                signed char remainder=get_reg(regmap,HIREG);
                assert(quotient>=0);
                assert(remainder>=0);
                emit_movs(d1,remainder);
                emit_negmi(remainder,remainder);
                emit_movs(d2,HOST_TEMPREG);
                emit_jeq((int)out+52); // Division by zero
                //emit_negsmi(HOST_TEMPREG,HOST_TEMPREG);
                //emit_movimm(1<<31,quotient);
                //emit_js((int)out+16); // Division by -2147483648
                //emit_adds(HOST_TEMPREG,HOST_TEMPREG,HOST_TEMPREG);
                //emit_shrimm(quotient,1,quotient);
                //emit_jns((int)out-8); // -2
                emit_negmi(HOST_TEMPREG,HOST_TEMPREG);
                emit_clz(HOST_TEMPREG,quotient);
                emit_shl(HOST_TEMPREG,quotient,HOST_TEMPREG);
                emit_orimm(quotient,1<<31,quotient);
                emit_shr(quotient,quotient,quotient);
                emit_cmp(remainder,HOST_TEMPREG);
                emit_subcs(remainder,HOST_TEMPREG,remainder);
                emit_adcs(quotient,quotient,quotient);
                emit_shrimm(HOST_TEMPREG,1,HOST_TEMPREG);
                emit_jcc((int)out-16); // -4
                emit_teq(d1,d2);
                emit_negmi(quotient,quotient);
                emit_test(d1,d1);
                emit_negmi(remainder,remainder);
            }
            if(opcode2[i]==0x1B) // DIVU
            {
                signed char d1=get_reg(regmap,rs1[i]); // dividend
                signed char d2=get_reg(regmap,rs2[i]); // divisor
                assert(d1>=0);
                assert(d2>=0);
                //emit_mov(d1,EAX);
                //emit_zeroreg(EDX);
                //emit_div(d2);
                signed char quotient=get_reg(regmap,LOREG);
                signed char remainder=get_reg(regmap,HIREG);
                assert(quotient>=0);
                assert(remainder>=0);
                emit_test(d2,d2);
                emit_jeq((int)out+44); // Division by zero
                //emit_mov(d1,remainder);
                //emit_movimm(1<<31,quotient);
                //emit_js((int)out+16); // +3
                //emit_adds(d2,d2,d2);
                //emit_shrimm(quotient,1,quotient);
                //emit_jns((int)out-8); // -2
                emit_clz(d2,HOST_TEMPREG);
                emit_movimm(1<<31,quotient);
                emit_shl(d2,HOST_TEMPREG,d2);
                emit_mov(d1,remainder);
                emit_shr(quotient,HOST_TEMPREG,quotient);
                emit_cmp(remainder,d2);
                emit_subcs(remainder,d2,remainder);
                emit_adcs(quotient,quotient,quotient);
                emit_shrcc_imm(d2,1,d2);
                emit_jcc((int)out-16); // -4
            }
        }
        else // 64-bit
        {
            if(opcode2[i]==0x1C) // DMULT
            {
                assert(opcode2[i]!=0x1C);
                signed char m1h=get_reg(regmap,rs1[i]|64);
                signed char m1l=get_reg(regmap,rs1[i]);
                signed char m2h=get_reg(regmap,rs2[i]|64);
                signed char m2l=get_reg(regmap,rs2[i]);
                assert(m1h>=0);
                assert(m2h>=0);
                assert(m1l>=0);
                assert(m2l>=0);
                emit_pushreg(m2h);
                emit_pushreg(m2l);
                emit_pushreg(m1h);
                emit_pushreg(m1l);
                emit_call((int)&mult64);
                emit_popreg(m1l);
                emit_popreg(m1h);
                emit_popreg(m2l);
                emit_popreg(m2h);
                signed char hih=get_reg(regmap,HIREG|64);
                signed char hil=get_reg(regmap,HIREG);
                if(hih>=0) emit_loadreg(HIREG|64,hih);
                if(hil>=0) emit_loadreg(HIREG,hil);
                signed char loh=get_reg(regmap,LOREG|64);
                signed char lol=get_reg(regmap,LOREG);
                if(loh>=0) emit_loadreg(LOREG|64,loh);
                if(lol>=0) emit_loadreg(LOREG,lol);
            }
            if(opcode2[i]==0x1D) // DMULTU
            {
                signed char m1h=get_reg(regmap,rs1[i]|64);
                signed char m1l=get_reg(regmap,rs1[i]);
                signed char m2h=get_reg(regmap,rs2[i]|64);
                signed char m2l=get_reg(regmap,rs2[i]);
                assert(m1h>=0);
                assert(m2h>=0);
                assert(m1l>=0);
                assert(m2l>=0);
                save_regs(0x100f);
                if(m1l!=0) emit_mov(m1l,0);
                if(m1h==0) emit_readword((int)&dynarec_local,1);
                else if(m1h>1) emit_mov(m1h,1);
                if(m2l<2) emit_readword((int)&dynarec_local+m2l*4,2);
                else if(m2l>2) emit_mov(m2l,2);
                if(m2h<3) emit_readword((int)&dynarec_local+m2h*4,3);
                else if(m2h>3) emit_mov(m2h,3);
                emit_call((int)&multu64);
                restore_regs(0x100f);
                signed char hih=get_reg(regmap,HIREG|64);
                signed char hil=get_reg(regmap,HIREG);
                signed char loh=get_reg(regmap,LOREG|64);
                signed char lol=get_reg(regmap,LOREG);
                /*signed char temp=get_reg(regmap,-1);
                 signed char rh=get_reg(regmap,HIREG|64);
                 signed char rl=get_reg(regmap,HIREG);
                 assert(m1h>=0);
                 assert(m2h>=0);
                 assert(m1l>=0);
                 assert(m2l>=0);
                 assert(temp>=0);
                 //emit_mov(m1l,EAX);
                 //emit_mul(m2l);
                 emit_umull(rl,rh,m1l,m2l);
                 emit_storereg(LOREG,rl);
                 emit_mov(rh,temp);
                 //emit_mov(m1h,EAX);
                 //emit_mul(m2l);
                 emit_umull(rl,rh,m1h,m2l);
                 emit_adds(rl,temp,temp);
                 emit_adcimm(0,rh);
                 emit_storereg(HIREG,rh);
                 //emit_mov(m2h,EAX);
                 //emit_mul(m1l);
                 emit_umull(rl,rh,m1l,m2h);
                 emit_adds(rl,temp,temp);
                 emit_adcimm(0,rh);
                 emit_storereg(LOREG|64,temp);
                 emit_mov(rh,temp);
                 //emit_mov(m2h,EAX);
                 //emit_mul(m1h);
                 emit_umull(rl,rh,m1h,m2h);
                 emit_adds(rl,temp,rl);
                 emit_loadreg(HIREG,temp);
                 emit_adcimm(0,rh);
                 emit_adds(rl,temp,rl);
                 emit_adcimm(0,rh);
                 // DEBUG
                 /*
                 emit_pushreg(m2h);
                 emit_pushreg(m2l);
                 emit_pushreg(m1h);
                 emit_pushreg(m1l);
                 emit_call((int)&multu64);
                 emit_popreg(m1l);
                 emit_popreg(m1h);
                 emit_popreg(m2l);
                 emit_popreg(m2h);
                 signed char hih=get_reg(regmap,HIREG|64);
                 signed char hil=get_reg(regmap,HIREG);
                 if(hih>=0) emit_loadreg(HIREG|64,hih);  // DEBUG
                 if(hil>=0) emit_loadreg(HIREG,hil);  // DEBUG
                 */
                // Shouldn't be necessary
                //char loh=get_reg(regmap,LOREG|64);
                //char lol=get_reg(regmap,LOREG);
                //if(loh>=0) emit_loadreg(LOREG|64,loh);
                //if(lol>=0) emit_loadreg(LOREG,lol);
            }
            if(opcode2[i]==0x1E) // DDIV
            {
                signed char d1h=get_reg(regmap,rs1[i]|64);
                signed char d1l=get_reg(regmap,rs1[i]);
                signed char d2h=get_reg(regmap,rs2[i]|64);
                signed char d2l=get_reg(regmap,rs2[i]);
                assert(d1h>=0);
                assert(d2h>=0);
                assert(d1l>=0);
                assert(d2l>=0);
                save_regs(0x100f);
                if(d1l!=0) emit_mov(d1l,0);
                if(d1h==0) emit_readword((int)&dynarec_local,1);
                else if(d1h>1) emit_mov(d1h,1);
                if(d2l<2) emit_readword((int)&dynarec_local+d2l*4,2);
                else if(d2l>2) emit_mov(d2l,2);
                if(d2h<3) emit_readword((int)&dynarec_local+d2h*4,3);
                else if(d2h>3) emit_mov(d2h,3);
                emit_call((int)&div64);
                restore_regs(0x100f);
                signed char hih=get_reg(regmap,HIREG|64);
                signed char hil=get_reg(regmap,HIREG);
                signed char loh=get_reg(regmap,LOREG|64);
                signed char lol=get_reg(regmap,LOREG);
                if(hih>=0) emit_loadreg(HIREG|64,hih);
                if(hil>=0) emit_loadreg(HIREG,hil);
                if(loh>=0) emit_loadreg(LOREG|64,loh);
                if(lol>=0) emit_loadreg(LOREG,lol);
            }
            if(opcode2[i]==0x1F) // DDIVU
            {
                //u_int hr,reglist=0;
                //for(hr=0;hr<HOST_REGS;hr++) {
                //  if(regmap[hr]>=0 && (regmap[hr]&62)!=HIREG) reglist|=1<<hr;
                //}
                signed char d1h=get_reg(regmap,rs1[i]|64);
                signed char d1l=get_reg(regmap,rs1[i]);
                signed char d2h=get_reg(regmap,rs2[i]|64);
                signed char d2l=get_reg(regmap,rs2[i]);
                assert(d1h>=0);
                assert(d2h>=0);
                assert(d1l>=0);
                assert(d2l>=0);
                save_regs(0x100f);
                if(d1l!=0) emit_mov(d1l,0);
                if(d1h==0) emit_readword((int)&dynarec_local,1);
                else if(d1h>1) emit_mov(d1h,1);
                if(d2l<2) emit_readword((int)&dynarec_local+d2l*4,2);
                else if(d2l>2) emit_mov(d2l,2);
                if(d2h<3) emit_readword((int)&dynarec_local+d2h*4,3);
                else if(d2h>3) emit_mov(d2h,3);
                emit_call((int)&divu64);
                restore_regs(0x100f);
                signed char hih=get_reg(regmap,HIREG|64);
                signed char hil=get_reg(regmap,HIREG);
                signed char loh=get_reg(regmap,LOREG|64);
                signed char lol=get_reg(regmap,LOREG);
                if(hih>=0) emit_loadreg(HIREG|64,hih);
                if(hil>=0) emit_loadreg(HIREG,hil);
                if(loh>=0) emit_loadreg(LOREG|64,loh);
                if(lol>=0) emit_loadreg(LOREG,lol);
            }
        }
    }
    else
    {
        // Multiply by zero is zero.
        // MIPS does not have a divide by zero exception.
        // The result is undefined, we return zero.
        signed char hr=get_reg(regmap,HIREG);
        signed char lr=get_reg(regmap,LOREG);
        if(hr>=0) emit_zeroreg(hr);
        if(lr>=0) emit_zeroreg(lr);
    }
}
#define multdiv_assemble multdiv_assemble_arm

void syscall_assemble(int i,signed char regmap[])
{
    signed char ccreg=get_reg(regmap,CCREG);
    assert(ccreg==HOST_CCREG);
    assert(!is_delayslot);
    emit_movimm(start+i*4,EAX); // Get PC
    emit_addimm(HOST_CCREG,2*ccadj[i],HOST_CCREG); // CHECK: is this right?  There should probably be an extra cycle...
    emit_jmp((int)jump_syscall);
}

void literal_pool(int n)
{
    if(!literalcount) return;
    if(n) {
        if((int)out-literals[0][0]<4096-n) return;
    }
    u_int *ptr;
    int i;
    for(i=0;i<literalcount;i++)
    {
        ptr[4096]=(u_int *)literals[i][0];
        u_int offset=(u_int)out-(u_int)ptr-8;
        assert(offset<4096);
        assert(!(offset&3));
        *ptr|=offset;
        output_w32(literals[i][1]);
    }
    literalcount=0;
}

void literal_pool_jumpover(int n)
{
    if(!literalcount) return;
    if(n) {
        if((int)out-literals[0][0]<4096-n) return;
    }
    int jaddr=(int)out;
    emit_jmp(0);
    literal_pool(0);
    set_jump_target(jaddr,(int)out);
}

void emit_extjump(int addr, int target)
{
    u_char *ptr=(u_char *)addr;
    assert((ptr[3]&0x0e)==0xa);
    emit_loadlp(target,0);
    emit_loadlp(addr,1);
    //assert(addr>=0x7000000&&addr<0x7FFFFFF);
    //assert((target>=0x80000000&&target<0x80800000)||(target>0xA4000000&&target<0xA4001000));
    //DEBUG >
#ifdef DEBUG_CYCLE_COUNT
    emit_readword((int)&last_count,ECX);
    emit_add(HOST_CCREG,ECX,HOST_CCREG);
    emit_readword((int)&next_interupt,ECX);
    emit_writeword(HOST_CCREG,(int)&Count);
    emit_sub(HOST_CCREG,ECX,HOST_CCREG);
    emit_writeword(ECX,(int)&last_count);
#endif
    //DEBUG <
    emit_jmp((int)dyna_linker);
}

void do_readstub(int n)
{
    literal_pool(64);
    set_jump_target(stubs[n][1],(int)out);
    int type=stubs[n][0];
    signed char *regmap=(char *)stubs[n][3];
    int rs=get_reg(regmap,stubs[n][4]);
    int rth=get_reg(regmap,stubs[n][4]|64);
    int rt=get_reg(regmap,stubs[n][4]);
    u_int reglist=stubs[n][6];
    assert(rs>=0);
    assert(rt>=0);
    int ftable=0;
    if(type==LOADB_STUB||type==LOADBU_STUB)
        ftable=(int)readmemb;
    if(type==LOADH_STUB||type==LOADHU_STUB)
        ftable=(int)readmemh;
    if(type==LOADW_STUB)
        ftable=(int)readmem;
    if(type==LOADD_STUB)
        ftable=(int)readmemd;
    emit_writeword(rs,(int)&address);
    //emit_pusha();
    save_regs(reglist);
    emit_shrimm(rs,16,1);
    int cc=get_reg(regmap,CCREG);
    if(cc<0) {
        emit_loadreg(CCREG,2);
    }
    emit_movimm(ftable,0);
    emit_addimm(cc<0?2:cc,2*stubs[n][5]+2,2);
    //emit_readword((int)&last_count,temp);
    //emit_add(cc,temp,cc);
    //emit_writeword(cc,(int)&Count);
    //emit_mov(15,14);
    emit_call((int)&indirect_jump);
    //emit_callreg(rs);
    //emit_readword_dualindexedx4(rs,HOST_TEMPREG,15);
    // We really shouldn't need to update the count here,
    // but not doing so causes random crashes...
    emit_readword((int)&Count,HOST_TEMPREG);
    emit_readword((int)&next_interupt,2);
    emit_addimm(HOST_TEMPREG,-2*stubs[n][5]-2,HOST_TEMPREG);
    emit_writeword(2,(int)&last_count);
    emit_sub(HOST_TEMPREG,2,cc<0?HOST_TEMPREG:cc);
    if(cc<0) {
        emit_storereg(CCREG,HOST_TEMPREG);
    }
    //emit_popa();
    restore_regs(reglist);
    //if((cc=get_reg(regmap,CCREG))>=0) {
    //  emit_loadreg(CCREG,cc);
    //}
    
    if(type==LOADB_STUB)
        emit_movsbl((int)&readmem_dword,rt);
    if(type==LOADBU_STUB)
        emit_movzbl((int)&readmem_dword,rt);
    if(type==LOADH_STUB)
        emit_movswl((int)&readmem_dword,rt);
    if(type==LOADHU_STUB)
        emit_movzwl((int)&readmem_dword,rt);
    if(type==LOADW_STUB)
        emit_readword((int)&readmem_dword,rt);
    if(type==LOADD_STUB) {
        emit_readword((int)&readmem_dword,rt);
        if(rth>=0) emit_readword(((int)&readmem_dword)+4,rth);
    }
    emit_jmp(stubs[n][2]); // return address
}

void inline_readstub(int type, u_int addr, signed char regmap[], int target, int adj, u_int reglist)
{
    int rs=get_reg(regmap,target);
    int rth=get_reg(regmap,target|64);
    int rt=get_reg(regmap,target);
    assert(rs>=0);
    assert(rt>=0);
    int ftable=0;
    if(type==LOADB_STUB||type==LOADBU_STUB)
        ftable=(int)readmemb;
    if(type==LOADH_STUB||type==LOADHU_STUB)
        ftable=(int)readmemh;
    if(type==LOADW_STUB)
        ftable=(int)readmem;
    if(type==LOADD_STUB)
        ftable=(int)readmemd;
    emit_writeword(rs,(int)&address);
    //emit_pusha();
    save_regs(reglist);
    //emit_shrimm(rs,16,1);
    int cc=get_reg(regmap,CCREG);
    if(cc<0) {
        emit_loadreg(CCREG,2);
    }
    //emit_movimm(ftable,0);
    emit_addimm(cc<0?2:cc,2*adj+2,2);
    emit_readword((int)&last_count,3);
    emit_add(3,2,2);
    emit_writeword(2,(int)&Count);
    emit_call(((u_int *)ftable)[addr>>16]);
    // We really shouldn't need to update the count here,
    // but not doing so causes random crashes...
    emit_readword((int)&Count,HOST_TEMPREG);
    emit_readword((int)&next_interupt,2);
    emit_addimm(HOST_TEMPREG,-2*adj-2,HOST_TEMPREG);
    emit_writeword(2,(int)&last_count);
    emit_sub(HOST_TEMPREG,2,cc<0?HOST_TEMPREG:cc);
    if(cc<0) {
        emit_storereg(CCREG,HOST_TEMPREG);
    }
    //emit_popa();
    restore_regs(reglist);
    if(type==LOADB_STUB)
        emit_movsbl((int)&readmem_dword,rt);
    if(type==LOADBU_STUB)
        emit_movzbl((int)&readmem_dword,rt);
    if(type==LOADH_STUB)
        emit_movswl((int)&readmem_dword,rt);
    if(type==LOADHU_STUB)
        emit_movzwl((int)&readmem_dword,rt);
    if(type==LOADW_STUB)
        emit_readword((int)&readmem_dword,rt);
    if(type==LOADD_STUB) {
        emit_readword((int)&readmem_dword,rt);
        if(rth>=0) emit_readword(((int)&readmem_dword)+4,rth);
    }
}

void do_writestub(int n)
{
    literal_pool(64);
    set_jump_target(stubs[n][1],(int)out);
    int type=stubs[n][0];
    signed char *regmap=(char *)stubs[n][3];
    int rs=get_reg(regmap,-1);
    int rth=get_reg(regmap,stubs[n][4]|64);
    int rt=get_reg(regmap,stubs[n][4]);
    u_int reglist=stubs[n][6];
    assert(rs>=0);
    assert(rt>=0);
    int ftable=0;
    if(type==STOREB_STUB)
        ftable=(int)writememb;
    if(type==STOREH_STUB)
        ftable=(int)writememh;
    if(type==STOREW_STUB)
        ftable=(int)writemem;
    if(type==STORED_STUB)
        ftable=(int)writememd;
    emit_writeword(rs,(int)&address);
    //emit_shrimm(rs,16,rs);
    //emit_movmem_indexedx4(ftable,rs,rs);
    if(type==STOREB_STUB)
        emit_writebyte(rt,(int)&byte);
    if(type==STOREH_STUB)
        emit_writehword(rt,(int)&hword);
    if(type==STOREW_STUB)
        emit_writeword(rt,(int)&word);
    if(type==STORED_STUB) {
        emit_writeword(rt,(int)&dword);
        emit_writeword(stubs[n][4]?rth:rt,(int)&dword+4);
    }
    //emit_pusha();
    save_regs(reglist);
    emit_shrimm(rs,16,1);
    int cc=get_reg(regmap,CCREG);
    if(cc<0) {
        emit_loadreg(CCREG,2);
    }
    emit_movimm(ftable,0);
    emit_addimm(cc<0?2:cc,2*stubs[n][5]+2,2);
    //emit_readword((int)&last_count,temp);
    //emit_addimm(cc,2*stubs[n][5]+2,cc);
    //emit_add(cc,temp,cc);
    //emit_writeword(cc,(int)&Count);
    emit_call((int)&indirect_jump);
    //emit_callreg(rs);
    emit_readword((int)&Count,HOST_TEMPREG);
    emit_readword((int)&next_interupt,2);
    emit_addimm(HOST_TEMPREG,-2*stubs[n][5]-2,HOST_TEMPREG);
    emit_writeword(2,(int)&last_count);
    emit_sub(HOST_TEMPREG,2,cc<0?HOST_TEMPREG:cc);
    if(cc<0) {
        emit_storereg(CCREG,HOST_TEMPREG);
    }
    //emit_popa();
    restore_regs(reglist);
    //if((cc=get_reg(regmap,CCREG))>=0) {
    //  emit_loadreg(CCREG,cc);
    //}
    emit_jmp(stubs[n][2]); // return address
}

void inline_writestub(int type, u_int addr, signed char regmap[], int target, int adj, u_int reglist)
{
    int rs=get_reg(regmap,-1);
    int rth=get_reg(regmap,target|64);
    int rt=get_reg(regmap,target);
    assert(rs>=0);
    assert(rt>=0);
    int ftable=0;
    if(type==STOREB_STUB)
        ftable=(int)writememb;
    if(type==STOREH_STUB)
        ftable=(int)writememh;
    if(type==STOREW_STUB)
        ftable=(int)writemem;
    if(type==STORED_STUB)
        ftable=(int)writememd;
    emit_writeword(rs,(int)&address);
    //emit_shrimm(rs,16,rs);
    //emit_movmem_indexedx4(ftable,rs,rs);
    if(type==STOREB_STUB)
        emit_writebyte(rt,(int)&byte);
    if(type==STOREH_STUB)
        emit_writehword(rt,(int)&hword);
    if(type==STOREW_STUB)
        emit_writeword(rt,(int)&word);
    if(type==STORED_STUB) {
        emit_writeword(rt,(int)&dword);
        emit_writeword(target?rth:rt,(int)&dword+4);
    }
    //emit_pusha();
    save_regs(reglist);
    //emit_shrimm(rs,16,1);
    int cc=get_reg(regmap,CCREG);
    if(cc<0) {
        emit_loadreg(CCREG,2);
    }
    //emit_movimm(ftable,0);
    emit_addimm(cc<0?2:cc,2*adj+2,2);
    emit_readword((int)&last_count,3);
    emit_add(3,2,2);
    emit_writeword(2,(int)&Count);
    emit_call(((u_int *)ftable)[addr>>16]);
    emit_readword((int)&Count,HOST_TEMPREG);
    emit_readword((int)&next_interupt,2);
    emit_addimm(HOST_TEMPREG,-2*adj-2,HOST_TEMPREG);
    emit_writeword(2,(int)&last_count);
    emit_sub(HOST_TEMPREG,2,cc<0?HOST_TEMPREG:cc);
    if(cc<0) {
        emit_storereg(CCREG,HOST_TEMPREG);
    }
    //emit_popa();
    restore_regs(reglist);
}

void do_unalignedwritestub(int n)
{
    set_jump_target(stubs[n][1],(int)out);
    output_w32(0xef000000);
    emit_jmp(stubs[n][2]); // return address
}

void printregs(int edi,int esi,int ebp,int esp,int b,int d,int c,int a)
{
    printf("regs: %x %x %x %x %x %x %x (%x)\n",a,b,c,d,ebp,esi,edi,(&edi)[-1]);
}

void do_invstub(int n)
{
    literal_pool(20);
    u_int reglist=stubs[n][3];
    set_jump_target(stubs[n][1],(int)out);
    save_regs(reglist);
    if(stubs[n][4]!=0) emit_mov(stubs[n][4],0);
    emit_call((int)&invalidate_addr);
    restore_regs(reglist);
    emit_jmp(stubs[n][2]); // return address
}

int do_dirty_stub(int i)
{
    assem_debug("do_dirty_stub %x\n",start+i*4);
    emit_movimm(start+i*4,0);
    emit_movimm((int)source,1);
    emit_movimm((int)copy,2);
    emit_movimm(slen*4,3);
    emit_call((int)&verify_code);
    int entry=(int)out;
    load_regs_entry(i);
    if(entry==(int)out) entry=instr_addr[i];
    emit_jmp(instr_addr[i]);
    return entry;
}

void do_cop1stub(int n)
{
    literal_pool(256);
    assem_debug("do_cop1stub %x\n",start+stubs[n][3]*4);
    set_jump_target(stubs[n][1],(int)out);
    int i=stubs[n][3];
    int rs=stubs[n][4];
    int ds=stubs[n][5];
    wb_dirtys(regmap_entry[i],is32[i],dirty[i]);
    if(regmap_entry[i][HOST_CCREG]!=CCREG) emit_loadreg(CCREG,HOST_CCREG);
    emit_movimm(start+(i-ds)*4,EAX); // Get PC
    emit_addimm(HOST_CCREG,2*ccadj[i],HOST_CCREG); // CHECK: is this right?  There should probably be an extra cycle...
    emit_jmp(ds?(int)fp_exception_ds:(int)fp_exception);
    
    /* This works, but uses a lot of memory...
     emit_orimm(rs,2,rs); // Set status
     wb_dirtys(regmap_entry[i],is32[i],dirty[i]);
     // Get program counter
     emit_movimm(start+i*4,!rs);
     // Save status
     emit_writeword(rs,(int)&Status);
     // Load cause
     emit_movimm((11 << 2) | 0x10000000,EDX);
     // Get cycle count if necessary
     if(regmap_entry[i][HOST_CCREG]!=CCREG) emit_loadreg(CCREG,HOST_CCREG);
     // Save PC as exception return address
     emit_writeword(!rs,(int)&EPC);
     // Save cause
     emit_writeword(EDX,(int)&Cause);
     // Do cycle count
     emit_addimm(HOST_CCREG,2*(ccadj[i]),HOST_CCREG); // CHECK: is this right?
     // There should probably be an extra cycle...
     add_to_linker((int)out,0x80000180,0);
     emit_jmp(0);*/
}

// Sign-extend to 64 bits and write out upper half of a register
// This is useful where we have a 32-bit value in a register, and want to
// keep it in a 32-bit register, but can't guarantee that it won't be read
// as a 64-bit value later.
void wb_sx(signed char pre[],signed char entry[],uint64_t dirty,uint64_t is32_pre,uint64_t is32,uint64_t u,uint64_t uu)
{
    if(is32_pre==is32) return;
    int hr,reg;
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && hr!=EXCLUDE_REG) {
            //if(pre[hr]==entry[hr]) {
            if((reg=pre[hr])>=0) {
                if((dirty>>hr)&1) {
                    if( ((is32_pre&~is32&~uu)>>reg)&1 ) {
                        emit_sarimm(hr,31,HOST_TEMPREG);
                        emit_storereg(reg|64,HOST_TEMPREG);
                    }
                }
            }
            //}
        }
    }
}

void wb_valid(signed char pre[],signed char entry[],u_int dirty_pre,u_int dirty,uint64_t is32_pre,uint64_t u,uint64_t uu)
{
    //if(dirty_pre==dirty) return;
    int hr,reg,new_hr;
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && hr!=EXCLUDE_REG) {
            reg=pre[hr];
            if(reg==entry[hr]) {
                if(((dirty_pre&~dirty)>>hr)&1) {
                    if(reg>=0&&reg<34) {
                        emit_storereg(reg,hr);
                        if( ((is32_pre&~uu)>>reg)&1 ) {
                            emit_sarimm(hr,31,HOST_TEMPREG);
                            emit_storereg(reg|64,HOST_TEMPREG);
                        }
                    }
                    else if(reg>=64) {
                        emit_storereg(reg,hr);
                    }
                }
            }
            else // Check if register moved to a different register
                if((new_hr=get_reg(entry,reg))>=0) {
                    if((dirty_pre>>hr)&(~dirty>>new_hr)&1) {
                        if(reg>=0&&reg<34) {
                            emit_storereg(reg,hr);
                            if( ((is32_pre&~uu)>>reg)&1 ) {
                                emit_sarimm(hr,31,HOST_TEMPREG);
                                emit_storereg(reg|64,HOST_TEMPREG);
                            }
                        }
                        else if(reg>=64) {
                            emit_storereg(reg,hr);
                        }
                    }
                }
        }
    }
}


/* using strd could possibly help but you'd have to allocate registers in pairs
 void wb_invalidate_arm(signed char pre[],signed char entry[],uint64_t dirty,uint64_t is32,uint64_t u,uint64_t uu)
 {
 int hr;
 int wrote=-1;
 for(hr=HOST_REGS-1;hr>=0;hr--) {
 if(hr!=EXCLUDE_REG) {
 if(pre[hr]!=entry[hr]) {
 if(pre[hr]>=0) {
 if((dirty>>hr)&1) {
 if(get_reg(entry,pre[hr])<0) {
 if(pre[hr]<64) {
 if(!((u>>pre[hr])&1)) {
 if(hr<10&&(~hr&1)&&(pre[hr+1]<0||wrote==hr+1)) {
 if( ((is32>>pre[hr])&1) && !((uu>>pre[hr])&1) ) {
 emit_sarimm(hr,31,hr+1);
 emit_strdreg(pre[hr],hr);
 }
 else
 emit_storereg(pre[hr],hr);
 }else{
 emit_storereg(pre[hr],hr);
 if( ((is32>>pre[hr])&1) && !((uu>>pre[hr])&1) ) {
 emit_sarimm(hr,31,hr);
 emit_storereg(pre[hr]|64,hr);
 }
 }
 }
 }else{
 if(!((uu>>(pre[hr]&63))&1) && !((is32>>(pre[hr]&63))&1)) {
 emit_storereg(pre[hr],hr);
 }
 }
 wrote=hr;
 }
 }
 }
 }
 }
 }
 for(hr=0;hr<HOST_REGS;hr++) {
 if(hr!=EXCLUDE_REG) {
 if(pre[hr]!=entry[hr]) {
 if(pre[hr]>=0) {
 int nr;
 if((nr=get_reg(entry,pre[hr]))>=0) {
 emit_mov(hr,nr);
 }
 }
 }
 }
 }
 }
 #define wb_invalidate wb_invalidate_arm
 */
