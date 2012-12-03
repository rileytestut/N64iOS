/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - new_dynarec.c                                           *
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

#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h> //include for uint64_t
#include <assert.h>

#include "../recomp.h"
#include "../recomph.h" //include for function prototypes
#include "../macros.h"
#include "../r4300.h"
#include "../ops.h"
#include "../interupt.h"

#include "../../memory/memory.h"

#include <sys/mman.h>

#ifdef __i386__
#include "assem_x86.h"
#endif
#ifdef __x86_64__
#include "assem_x64.h"
#endif
#ifdef __arm__
#include "assem_arm.h"
#endif

#define MAXBLOCK 4096
#define TARGET_SIZE_2 24 // 2^24 = 16 megabytes
#define MAX_OUTPUT_BLOCK_SIZE 262144

#if defined(__APPLE__) && defined(__arm__)
// iPhone specific
extern void sys_icache_invalidate(const void* Addr, size_t len);
#define INT u_int
extern unsigned char translation_cache_static_iphone[(1<<TARGET_SIZE_2) + 4096];
unsigned char* translation_cache_iphone = NULL; //translation_cache_static_iphone;
#define BASE_ADDR ((unsigned long)translation_cache_iphone)
#else
#define BASE_ADDR 0x7000000 // Code generator target address
#endif

struct regstat
{
    uint64_t is32;
    uint64_t dirty;
    signed char regmap[HOST_REGS];
    uint64_t u;
    uint64_t uu;
    u_int isconst;
    uint64_t constmap[HOST_REGS];
};

struct ll_entry
{
    u_int vaddr;
    u_int reg32;
    void *addr;
    struct ll_entry *next;
};

u_int start;
u_int *source;
char insn[MAXBLOCK][10];
u_char itype[MAXBLOCK];
u_char opcode[MAXBLOCK];
u_char opcode2[MAXBLOCK];
u_char bt[MAXBLOCK];
u_char rs1[MAXBLOCK];
u_char rs2[MAXBLOCK];
u_char rt1[MAXBLOCK];
u_char rt2[MAXBLOCK];
u_char us1[MAXBLOCK];
u_char us2[MAXBLOCK];
u_char dep1[MAXBLOCK];
u_char dep2[MAXBLOCK];
u_char likely[MAXBLOCK];
u_int ba[MAXBLOCK];
int imm[MAXBLOCK];
uint64_t unneeded_reg[MAXBLOCK];
uint64_t unneeded_reg_upper[MAXBLOCK];
uint64_t branch_unneeded_reg[MAXBLOCK];
uint64_t branch_unneeded_reg_upper[MAXBLOCK];
uint64_t is32[MAXBLOCK];
uint64_t dirty[MAXBLOCK];
uint64_t is32_post[MAXBLOCK];
uint64_t dirty_post[MAXBLOCK];
signed char regmap_pre[MAXBLOCK][HOST_REGS];
signed char regmap[MAXBLOCK][HOST_REGS];
signed char regmap_entry[MAXBLOCK][HOST_REGS];
uint64_t constmap[MAXBLOCK][HOST_REGS];
uint64_t known_value[HOST_REGS];
u_int isconst[MAXBLOCK];
u_int wasconst[MAXBLOCK];
u_int known_reg;
struct regstat branch_regs[MAXBLOCK];
u_int needed_reg[MAXBLOCK];
uint64_t requires_32bit[MAXBLOCK];
u_int wont_dirty[MAXBLOCK];
u_int will_dirty[MAXBLOCK];
int ccadj[MAXBLOCK];
int slen;
u_int instr_addr[MAXBLOCK];
u_int link_addr[MAXBLOCK][3];
int linkcount;
u_int stubs[MAXBLOCK*3][7];
int stubcount;
u_int literals[1024][2];
int literalcount;
int is_delayslot;
int cop1_usable;
u_char *out;
struct ll_entry *jump_in[2049];
struct ll_entry *jump_out[2049];
struct ll_entry *jump_dirty[2049];
u_int hash_table[65536][4]  __attribute__((aligned(16)));
char shadow[1048576]  __attribute__((aligned(16)));
void *copy;
int expirep;

int testing = 0;

/* registers that may be allocated */
/* 1-31 gpr */
#define HIREG 32 // hi
#define LOREG 33 // lo
#define FSREG 34 // FPU status (FCSR)
#define CSREG 35 // Coprocessor status
#define CCREG 36 // Cycle count
#define FTEMP 37 // FPU temporary register
#define PTEMP 38 // Prefetch temporary register
#define INVCP 39 // Pointer to invalid_code
#define MAXREG 39

/* instruction types */
#define NOP 0     // No operation
#define LOAD 1    // Load
#define STORE 2   // Store
#define LOADLR 3  // Unaligned load
#define STORELR 4 // Unaligned store
#define MOV 5     // Move
#define ALU 6     // Arithmetic/logic
#define MULTDIV 7 // Multiply/divide
#define SHIFT 8   // Shift by register
#define SHIFTIMM 9// Shift by immediate
#define IMM16 10  // 16-bit immediate
#define RJUMP 11  // Unconditional jump to register
#define UJUMP 12  // Unconditional jump
#define CJUMP 13  // Conditional branch (BEQ/BNE/BGTZ/BLEZ)
#define SJUMP 14  // Conditional branch (regimm format)
#define COP0 15   // Coprocessor 0
#define COP1 16   // Coprocessor 1
#define C1LS 17   // Coprocessor 1 load/store
#define FJUMP 18  // Conditional branch (floating point)
#define FLOAT 19  // Floating point unit
#define FCONV 20  // Convert integer to float
#define FCOMP 21  // Floating point compare (sets FSREG)
#define SYSCALL 22// SYSCALL
#define OTHER 23  // Other
#define NI 24     // Not implemented

/* stubs */
#define CC_STUB 1
#define FP_STUB 2
#define LOADB_STUB 3
#define LOADH_STUB 4
#define LOADW_STUB 5
#define LOADD_STUB 6
#define LOADBU_STUB 7
#define LOADHU_STUB 8
#define STOREB_STUB 9
#define STOREH_STUB 10
#define STOREW_STUB 11
#define STORED_STUB 12
#define STORELR_STUB 13
#define INVCODE_STUB 14

void add_stub(int type,int addr,int retaddr,int a,int b,int c,int d);

/* branch codes */
#define TAKEN 1
#define NOTTAKEN 2
#define NULLDS 3

// asm linkage
void new_recompile_block(int addr);
void invalidate_block(u_int block);
void invalidate_addr(u_int addr);
void jump_vaddr();
void dyna_linker();
void verify_code();
void cc_interrupt();
void fp_exception();
void fp_exception_ds();
void jump_syscall();
void jump_eret();

// Needed by assembler
void wb_register(signed char r,signed char regmap[],uint64_t dirty,uint64_t is32);
void wb_dirtys(signed char i_regmap[],uint64_t i_is32,uint64_t i_dirty);
void wb_needed_dirtys(signed char i_regmap[],uint64_t i_is32,uint64_t i_dirty,int addr);
void load_all_regs(signed char i_regmap[]);
void load_needed_regs(signed char i_regmap[],signed char next_regmap[]);
void load_regs_entry(int t);

int tracedebug=0;

//#define DEBUG_CYCLE_COUNT 1

void nullf() {}
//#define assem_debug printf
//#define inv_debug printf
#define assem_debug nullf
#define inv_debug nullf

extern void testthis();

void testthat(int i)
{
    //fprintf(stderr, "dis_insn[%d]: %s\n",i,dis_insn[i]); fflush(stderr);
    fprintf(stderr, "MADE IT TO 0x%x ADDR !!!\n", i); fflush(stderr);
    //while(1){}
}

// Get address from virtual address
// This is called from the recompiled JR/JALR instructions
void *get_addr(u_int vaddr)
{
    u_int page=(vaddr^0x80000000)>>12;
    if(page>2048) page=2048;
    struct ll_entry *head;
    //printf("TRACE: count=%d next=%d (get_addr %x,page %d)\n",Count,next_interupt,vaddr,page);
    head=jump_in[page];
    while(head!=NULL) {
        if(head->vaddr==vaddr&&head->reg32==0) {
            //printf("TRACE: count=%d next=%d (get_addr match %x: %x)\n",Count,next_interupt,vaddr,(int)head->addr);
            int *ht_bin=hash_table[((vaddr>>16)^vaddr)&0xFFFF];
            ht_bin[3]=ht_bin[1];
            ht_bin[2]=ht_bin[0];
            ht_bin[1]=(int)head->addr;
            ht_bin[0]=vaddr;
            return head->addr;
        }
        head=head->next;
    }
    head=jump_dirty[page];
    while(head!=NULL) {
        if(head->vaddr==vaddr) {
            // FIXME: Should verify the block before returning the address
            int *ht_bin=hash_table[((vaddr>>16)^vaddr)&0xFFFF];
            ht_bin[3]=ht_bin[1];
            ht_bin[2]=ht_bin[0];
            ht_bin[1]=(int)head->addr;
            ht_bin[0]=vaddr;
            return head->addr;
        }
        head=head->next;
    }
    new_recompile_block(vaddr);
    return get_addr(vaddr);
}
// Look up address in hash table first
void *get_addr_ht(u_int vaddr)
{
    int *ht_bin=hash_table[((vaddr>>16)^vaddr)&0xFFFF];
    if(ht_bin[0]==vaddr) return (void *)ht_bin[1];
    if(ht_bin[2]==vaddr) return (void *)ht_bin[3];
    return get_addr(vaddr);
}

void *get_addr_32(u_int vaddr,u_int flags)
{
    //printf("TRACE: count=%d next=%d (get_addr_32 %x,flags %x)\n",Count,next_interupt,vaddr,flags);
    int *ht_bin=hash_table[((vaddr>>16)^vaddr)&0xFFFF];
    if(ht_bin[0]==vaddr) return (void *)ht_bin[1];
    if(ht_bin[2]==vaddr) return (void *)ht_bin[3];
    u_int page=(vaddr^0x80000000)>>12;
    if(page>2048) page=2048;
    struct ll_entry *head;
    head=jump_in[page];
    while(head!=NULL) {
        if(head->vaddr==vaddr&&(head->reg32&flags)==0) {
            //printf("TRACE: count=%d next=%d (get_addr_32 match %x: %x)\n",Count,next_interupt,vaddr,(int)head->addr);
            if(head->reg32==0) {
                int *ht_bin=hash_table[((vaddr>>16)^vaddr)&0xFFFF];
                ht_bin[3]=ht_bin[1];
                ht_bin[2]=ht_bin[0];
                ht_bin[1]=(int)head->addr;
                ht_bin[0]=vaddr;
            }
            return head->addr;
        }
        head=head->next;
    }
    //printf("TRACE: count=%d next=%d (get_addr_32 no-match %x,flags %x)\n",Count,next_interupt,vaddr,flags);
    return get_addr(vaddr);
}

void clear_all_regs(signed char regmap[])
{
    int i;
    for (i=0;i<HOST_REGS;i++) if(i!=9) regmap[i]=-1;
}

signed char get_reg(signed char regmap[],int r)
{
    int hr;
    for (hr=0;hr<HOST_REGS;hr++) if(hr!=9&&hr!=EXCLUDE_REG&&regmap[hr]==r) return hr;
    return -1;
}

void dirty_reg(struct regstat *cur,signed char reg)
{
    int hr;
    if(!reg) return;
    for (hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && (cur->regmap[hr]&63)==reg) {
            cur->dirty|=1<<hr;
        }
    }
}

// If we dirty the lower half of a 64 bit register which is now being
// sign-extended, we need to dump the upper half.
// Note: Do this only after completion of the instruction, because
// some instructions may need to read the full 64-bit value even if
// overwriting it (eg SLTI, DSRA32).
void flush_dirty_uppers(struct regstat *cur)
{
    int hr,reg;
    for (hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && (cur->dirty>>hr)&1) {
            reg=cur->regmap[hr];
            if(reg>=64)
                if((cur->is32>>(reg&63))&1) cur->regmap[hr]=-1;
        }
    }
}

void set_const(struct regstat *cur,signed char reg,uint64_t value)
{
    int hr;
    if(!reg) return;
    for (hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && cur->regmap[hr]==reg) {
            cur->isconst|=1<<hr;
            cur->constmap[hr]=value;
        }
        else if((cur->regmap[hr]^64)==reg) {
            cur->isconst|=1<<hr;
            cur->constmap[hr]=value>>32;
        }
    }
}

void clear_const(struct regstat *cur,signed char reg)
{
    int hr;
    if(!reg) return;
    for (hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && (cur->regmap[hr]&63)==reg) {
            cur->isconst&=~(1<<hr);
        }
    }
}

int is_const(struct regstat *cur,signed char reg)
{
    int hr;
    if(!reg) return 1;
    for (hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && (cur->regmap[hr]&63)==reg) {
            return (cur->isconst>>hr)&1;
        }
    }
    return 0;
}
uint64_t get_const(struct regstat *cur,signed char reg)
{
    int hr;
    if(!reg) return 0;
    for (hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && cur->regmap[hr]==reg) {
            return cur->constmap[hr];
        }
    }
    printf("Unknown constant in r%d\n",reg);
    exit(1);
}

// Least soon needed registers
// Look at the next ten instructions and see which registers
// will be used.  Try not to reallocate these.
void lsn(u_char hsn[], int i, int *preferred_reg)
{
    int j;
    int b=-1;
    for(j=0;j<9;j++)
    {
        if(i+j>=slen) {
            j=slen-i-1;
            break;
        }
        if(itype[i+j]==UJUMP||itype[i+j]==RJUMP||(source[i+j]>>16)==0x1000)
        {
            // Don't go past an unconditonal jump
            j++;
            break;
        }
    }
    for(;j>=0;j--)
    {
        if(rs1[i+j]) hsn[rs1[i+j]]=j;
        if(rs2[i+j]) hsn[rs2[i+j]]=j;
        if(rt1[i+j]) hsn[rt1[i+j]]=j;
        if(rt2[i+j]) hsn[rt2[i+j]]=j;
        if(itype[i+j]==STORE || itype[i+j]==STORELR) {
            // Stores can allocate zero
            hsn[rs1[i+j]]=j;
            hsn[rs2[i+j]]=j;
        }
        // On some architectures stores need invc_ptr
#if defined(HOST_IMM8)
        if(itype[i+j]==STORE || itype[i+j]==STORELR || (opcode[i+j]&0x3b)==0x39) {
            hsn[INVCP]=j;
        }
#endif
        if(i+j>=0&&(itype[i+j]==UJUMP||itype[i+j]==CJUMP||itype[i+j]==SJUMP||itype[i+j]==FJUMP))
        {
            hsn[CCREG]=j;
            b=j;
        }
    }
    if(b>=0)
    {
        if(ba[i+b]>=start && ba[i+b]<(start+slen*4))
        {
            // Follow first branch
            int t=(ba[i+b]-start)>>2;
            j=7-b;if(t+j>=slen) j=slen-t-1;
            for(;j>=0;j--)
            {
                if(rs1[t+j]) if(hsn[rs1[t+j]]>j+b+2) hsn[rs1[t+j]]=j+b+2;
                if(rs2[t+j]) if(hsn[rs2[t+j]]>j+b+2) hsn[rs2[t+j]]=j+b+2;
                if(rt1[t+j]) if(hsn[rt1[t+j]]>j+b+2) hsn[rt1[t+j]]=j+b+2;
                if(rt2[t+j]) if(hsn[rt2[t+j]]>j+b+2) hsn[rt2[t+j]]=j+b+2;
            }
        }
        // TODO: preferred register based on backward branch
    }
    // Delay slot should preferably not overwrite branch conditions or cycle count
    if(i>0&&(itype[i-1]==RJUMP||itype[i-1]==UJUMP||itype[i-1]==CJUMP||itype[i-1]==SJUMP||itype[i-1]==FJUMP)) {
        if(rs1[i-1]) if(hsn[rs1[i-1]]>1) hsn[rs1[i-1]]=1;
        if(rs2[i-1]) if(hsn[rs2[i-1]]>1) hsn[rs2[i-1]]=1;
        hsn[CCREG]=1;
    }
    // Coprocessor load/store needs FTEMP, even if not declared
    if(itype[i]==C1LS) {
        hsn[FTEMP]=0;
    }
    // Load L/R also uses FTEMP as a temporary register
    if(itype[i]==LOADLR) {
        hsn[FTEMP]=0;
    }
}

// We only want to allocate registers if we're going to use them again soon
int needed_again(int r, int i)
{
    int j;
    int b=-1;
    int rn=10;
    int hr;
    u_char hsn[MAXREG+1];
    int preferred_reg;
    
    memset(hsn,10,sizeof(hsn));
    lsn(hsn,i,&preferred_reg);
    
    for(j=0;j<9;j++)
    {
        if(i+j>=slen) {
            j=slen-i-1;
            break;
        }
        if(itype[i+j]==UJUMP||itype[i+j]==RJUMP||(source[i+j]>>16)==0x1000)
        {
            // Don't go past an unconditonal jump
            j++;
            break;
        }
    }
    for(;j>=1;j--)
    {
        if(rs1[i+j]==r) rn=j;
        if(rs2[i+j]==r) rn=j;
        if((unneeded_reg[i+j]>>r)&1) rn=10;
        if(i+j>=0&&(itype[i+j]==UJUMP||itype[i+j]==CJUMP||itype[i+j]==SJUMP||itype[i+j]==FJUMP))
        {
            b=j;
        }
    }
    if(b>=0)
    {
        if(ba[i+b]>=start && ba[i+b]<(start+slen*4))
        {
            // Follow first branch
            int t=(ba[i+b]-start)>>2;
            j=7-b;if(t+j>=slen) j=slen-t-1;
            for(;j>=0;j--)
            {
                if(!((unneeded_reg[t+j]>>r)&1)) {
                    if(rs1[t+j]==r) if(rn>j+b+2) rn=j+b+2;
                    if(rs2[t+j]==r) if(rn>j+b+2) rn=j+b+2;
                }
            }
        }
    }
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && hr!=EXCLUDE_REG) {
            if(rn<hsn[hr]) return 1;
        }
    }
    return 0;
}

// Allocate every register, preserving source/target regs
void alloc_all(struct regstat *cur,int i)
{
    int hr;
    
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && hr!=EXCLUDE_REG) {
            if(((cur->regmap[hr]&63)!=rs1[i])&&((cur->regmap[hr]&63)!=rs2[i])&&
               ((cur->regmap[hr]&63)!=rt1[i])&&((cur->regmap[hr]&63)!=rt2[i]))
            {
                cur->regmap[hr]=-1;
                cur->dirty&=~(1<<hr);
            }
            // Don't need zeros
            if((cur->regmap[hr]&63)==0)
            {
                cur->regmap[hr]=-1;
                cur->dirty&=~(1<<hr);
            }
        }
    }
}


void div64(int64_t dividend,int64_t divisor)
{
    lo=dividend/divisor;
    hi=dividend%divisor;
    //printf("TRACE: ddiv %8x%8x %8x%8x\n" ,(int)reg[HIREG],(int)(reg[HIREG]>>32)
    //                                     ,(int)reg[LOREG],(int)(reg[LOREG]>>32));
}
void divu64(uint64_t dividend,uint64_t divisor)
{
    lo=dividend/divisor;
    hi=dividend%divisor;
    //printf("TRACE: ddivu %8x%8x %8x%8x\n",(int)reg[HIREG],(int)(reg[HIREG]>>32)
    //                                     ,(int)reg[LOREG],(int)(reg[LOREG]>>32));
}

void mult64(uint64_t m1,uint64_t m2)
{
    unsigned long long int op1, op2, op3, op4;
    unsigned long long int result1, result2, result3, result4;
    unsigned long long int temp1, temp2, temp3, temp4;
    int sign = 0;
    
    if (m1 < 0)
    {
        op2 = -m1;
        sign = 1 - sign;
    }
    else op2 = m1;
    if (m2 < 0)
    {
        op4 = -m2;
        sign = 1 - sign;
    }
    else op4 = m2;
    
    op1 = op2 & 0xFFFFFFFF;
    op2 = (op2 >> 32) & 0xFFFFFFFF;
    op3 = op4 & 0xFFFFFFFF;
    op4 = (op4 >> 32) & 0xFFFFFFFF;
    
    temp1 = op1 * op3;
    temp2 = (temp1 >> 32) + op1 * op4;
    temp3 = op2 * op3;
    temp4 = (temp3 >> 32) + op2 * op4;
    
    result1 = temp1 & 0xFFFFFFFF;
    result2 = temp2 + (temp3 & 0xFFFFFFFF);
    result3 = (result2 >> 32) + temp4;
    result4 = (result3 >> 32);
    
    lo = result1 | (result2 << 32);
    hi = (result3 & 0xFFFFFFFF) | (result4 << 32);
    if (sign)
    {
        hi = ~hi;
        if (!lo) hi++;
        else lo = ~lo + 1;
    }
}

void multu64(uint64_t m1,uint64_t m2)
{
    unsigned long long int op1, op2, op3, op4;
    unsigned long long int result1, result2, result3, result4;
    unsigned long long int temp1, temp2, temp3, temp4;
    
    op1 = m1 & 0xFFFFFFFF;
    op2 = (m1 >> 32) & 0xFFFFFFFF;
    op3 = m2 & 0xFFFFFFFF;
    op4 = (m2 >> 32) & 0xFFFFFFFF;
    
    temp1 = op1 * op3;
    temp2 = (temp1 >> 32) + op1 * op4;
    temp3 = op2 * op3;
    temp4 = (temp3 >> 32) + op2 * op4;
    
    result1 = temp1 & 0xFFFFFFFF;
    result2 = temp2 + (temp3 & 0xFFFFFFFF);
    result3 = (result2 >> 32) + temp4;
    result4 = (result3 >> 32);
    
    lo = result1 | (result2 << 32);
    hi = (result3 & 0xFFFFFFFF) | (result4 << 32);
    
    //printf("TRACE: dmultu %8x%8x %8x%8x\n",(int)reg[HIREG],(int)(reg[HIREG]>>32)
    //                                      ,(int)reg[LOREG],(int)(reg[LOREG]>>32));
}

uint64_t ldl_merge(uint64_t original,uint64_t loaded,u_int bits)
{
    if(bits) {
        original<<=64-bits;
        original>>=64-bits;
        loaded<<=bits;
        original|=loaded;
    }
    else original=loaded;
    return original;
}
uint64_t ldr_merge(uint64_t original,uint64_t loaded,u_int bits)
{
    if(bits^56) {
        original>>=64-(bits^56);
        original<<=64-(bits^56);
        loaded>>=bits^56;
        original|=loaded;
    }
    else original=loaded;
    return original;
}

#ifdef __i386__
#include "assem_x86.c"
#endif
#ifdef __x86_64__
#include "assem_x64.c"
#endif
#ifdef __arm__
#include "assem_arm.c"
#endif

// Add virtual address mapping to linked list
void ll_add(struct ll_entry **head,int vaddr,void *addr)
{
    struct ll_entry *new_entry;
    new_entry=malloc(sizeof(struct ll_entry));
    assert(new_entry!=NULL);
    new_entry->vaddr=vaddr;
    new_entry->reg32=0;
    new_entry->addr=addr;
    new_entry->next=*head;
    *head=new_entry;
}

// Add virtual address mapping for 32-bit compiled block
void ll_add_32(struct ll_entry **head,int vaddr,u_int reg32,void *addr)
{
    struct ll_entry *new_entry;
    new_entry=malloc(sizeof(struct ll_entry));
    assert(new_entry!=NULL);
    new_entry->vaddr=vaddr;
    new_entry->reg32=reg32;
    new_entry->addr=addr;
    new_entry->next=*head;
    *head=new_entry;
}

// Check if an address is already compiled
void *check_addr(u_int vaddr)
{
    u_int page=(vaddr^0x80000000)>>12;
    if(page>2048) page=2048;
    struct ll_entry *head;
    head=jump_in[page];
    while(head!=NULL) {
        if(head->vaddr==vaddr&&head->reg32==0) {
            return head->addr;
        }
        head=head->next;
    }
    return 0;
}

void remove_hash(int vaddr)
{
    int *ht_bin=hash_table[(((vaddr)>>16)^vaddr)&0xFFFF];
    if(ht_bin[2]==vaddr) {
        ht_bin[2]=ht_bin[3]=-1;
    }
    if(ht_bin[0]==vaddr) {
        ht_bin[0]=ht_bin[2];
        ht_bin[1]=ht_bin[3];
    }
}

void ll_remove_matching_addrs(struct ll_entry **head,int addr,int shift)
{
    struct ll_entry *next;
    while(*head) {
        
        if(((u_int)((*head)->addr)>>shift)==(addr>>shift) ||
           ((u_int)((*head)->addr-MAX_OUTPUT_BLOCK_SIZE)>>shift)==(addr>>shift))
        {
            inv_debug("EXP: Remove pointer to %x (%x)\n",(int)(*head)->addr,(*head)->vaddr);
            next=(*head)->next;
            free(*head);
            *head=next;
        }
        else
        {
            head=&((*head)->next);
        }
    }
}

// Remove all entries from linked list
void ll_clear(struct ll_entry **head)
{
    struct ll_entry *cur;
    struct ll_entry *next;
    if(cur=*head) {
        *head=0;
        while(cur) {
            next=cur->next;
            free(cur);
            cur=next;
        }
    }
}

// Dereference the pointers and remove if it matches
void ll_kill_pointers(struct ll_entry *head,int addr,int shift)
{
    while(head) {
        int ptr=get_pointer(head->addr);
        inv_debug("EXP: Lookup pointer to %x at %x (%x)\n",(int)ptr,(int)head->addr,head->vaddr);
        if(((ptr>>shift)==(addr>>shift)) ||
           (((ptr-MAX_OUTPUT_BLOCK_SIZE)>>shift)==(addr>>shift)))
        {
            inv_debug("EXP: Kill pointer at %x (%x)\n",(int)head->addr,head->vaddr);
            kill_pointer(head->addr);
        }
        head=head->next;
    }
}

// This is called when we write to a compiled block (see do_invstub)
void invalidate_block(u_int block)
{
    u_int page=block^0x80000;
    if(page>2048) page=2048;
    inv_debug("INVALIDATE: %x (%d)\n",block<<12,page);
    struct ll_entry *head;
    struct ll_entry *next;
    head=jump_in[page];
    jump_in[page]=0;
    while(head!=NULL) {
        inv_debug("INVALIDATE: %x\n",head->vaddr);
        remove_hash(head->vaddr);
        next=head->next;
        free(head);
        head=next;
    }
    head=jump_out[page];
    jump_out[page]=0;
    while(head!=NULL) {
        inv_debug("INVALIDATE: kill pointer to %x (%x)\n",head->vaddr,(int)head->addr);
        kill_pointer(head->addr);
        next=head->next;
        free(head);
        head=next;
    }
    invalid_code[block]=1;
}
void invalidate_addr(u_int addr)
{
    invalidate_block(addr>>12);
}

// Add an entry to jump_out after making a link
void add_link(u_int vaddr,void *src)
{
    inv_debug("add_link: %x -> %x\n",(int)src,vaddr);
    u_int page=(vaddr^0x80000000)>>12;
    if(page>2048) page=2048;
    ll_add(jump_out+page,vaddr,src);
}


void mov_alloc(struct regstat *current,int i)
{
    // Note: Don't need to actually alloc the source registers
    if((~current->is32>>rs1[i])&1) {
        //alloc_reg64(current,i,rs1[i]);
        alloc_reg64(current,i,rt1[i]);
        current->is32&=~(1LL<<rt1[i]);
    } else {
        //alloc_reg(current,i,rs1[i]);
        alloc_reg(current,i,rt1[i]);
        current->is32|=(1LL<<rt1[i]);
    }
    clear_const(current,rs1[i]);
    dirty_reg(current,rt1[i]);
}

void shiftimm_alloc(struct regstat *current,int i)
{
    clear_const(current,rs1[i]);
    if(opcode2[i]<=0x3) // SLL/SRL/SRA
    {
        if(rt1[i]) {
            if(rs1[i]&&needed_again(rs1[i],i)) alloc_reg(current,i,rs1[i]);
            alloc_reg(current,i,rt1[i]);
            current->is32|=1LL<<rt1[i];
            dirty_reg(current,rt1[i]);
        }
    }
    if(opcode2[i]>=0x38&&opcode2[i]<=0x3b) // DSLL/DSRL/DSRA
    {
        if(rt1[i]) {
            if(rs1[i]) alloc_reg64(current,i,rs1[i]);
            alloc_reg64(current,i,rt1[i]);
            current->is32&=~(1LL<<rt1[i]);
            dirty_reg(current,rt1[i]);
        }
    }
    if(opcode2[i]==0x3c) // DSLL32
    {
        if(rt1[i]) {
            if(rs1[i]) alloc_reg(current,i,rs1[i]);
            alloc_reg64(current,i,rt1[i]);
            current->is32&=~(1LL<<rt1[i]);
            dirty_reg(current,rt1[i]);
        }
    }
    if(opcode2[i]==0x3e) // DSRL32
    {
        if(rt1[i]) {
            alloc_reg64(current,i,rs1[i]);
            if(imm[i]==32) {
                alloc_reg64(current,i,rt1[i]);
                current->is32&=~(1LL<<rt1[i]);
            } else {
                alloc_reg(current,i,rt1[i]);
                current->is32|=1LL<<rt1[i];
            }
            dirty_reg(current,rt1[i]);
        }
    }
    if(opcode2[i]==0x3f) // DSRA32
    {
        if(rt1[i]) {
            alloc_reg64(current,i,rs1[i]);
            alloc_reg(current,i,rt1[i]);
            current->is32|=1LL<<rt1[i];
            dirty_reg(current,rt1[i]);
        }
    }
}

void shift_alloc(struct regstat *current,int i)
{
    if(rt1[i]) {
        if(opcode2[i]<=0x07) // SLLV/SRLV/SRAV
        {
            if(rs1[i]) alloc_reg(current,i,rs1[i]);
            if(rs2[i]) alloc_reg(current,i,rs2[i]);
            alloc_reg(current,i,rt1[i]);
            if(rt1[i]==rs2[i]) alloc_reg_temp(current,i,-1);
            current->is32|=1LL<<rt1[i];
        } else { // DSLLV/DSRLV/DSRAV
            if(rs1[i]) alloc_reg64(current,i,rs1[i]);
            if(rs2[i]) alloc_reg(current,i,rs2[i]);
            alloc_reg64(current,i,rt1[i]);
            current->is32&=~(1LL<<rt1[i]);
            if(opcode2[i]==0x16||opcode2[i]==0x17) // DSRLV and DSRAV need a temporary register
                alloc_reg_temp(current,i,-1);
        }
        clear_const(current,rs1[i]);
        clear_const(current,rs2[i]);
        dirty_reg(current,rt1[i]);
    }
}

void alu_alloc(struct regstat *current,int i)
{
    if(opcode2[i]>=0x20&&opcode2[i]<=0x23) { // ADD/ADDU/SUB/SUBU
        if(rt1[i]) {
            if(rs1[i]&&rs2[i]) {
                alloc_reg(current,i,rs1[i]);
                alloc_reg(current,i,rs2[i]);
            }
            alloc_reg(current,i,rt1[i]);
        }
        current->is32|=1LL<<rt1[i];
    }
    if(opcode2[i]==0x2a||opcode2[i]==0x2b) { // SLT/SLTU
        if(rt1[i]) {
            if(!((current->is32>>rs1[i])&(current->is32>>rs2[i])&1))
            {
                alloc_reg64(current,i,rs1[i]);
                alloc_reg64(current,i,rs2[i]);
                alloc_reg(current,i,rt1[i]);
            } else {
                alloc_reg(current,i,rs1[i]);
                alloc_reg(current,i,rs2[i]);
                alloc_reg(current,i,rt1[i]);
            }
        }
        current->is32|=1LL<<rt1[i];
    }
    if(opcode2[i]>=0x24&&opcode2[i]<=0x27) { // AND/OR/XOR/NOR
        if(rt1[i]) {
            if(rs1[i]&&rs2[i]) {
                alloc_reg(current,i,rs1[i]);
                alloc_reg(current,i,rs2[i]);
            }
            else
            {
                if(rs1[i]&&needed_again(rs1[i],i)) alloc_reg(current,i,rs1[i]);
                if(rs2[i]&&needed_again(rs2[i],i)) alloc_reg(current,i,rs2[i]);
            }
            alloc_reg(current,i,rt1[i]);
            if(!((current->is32>>rs1[i])&(current->is32>>rs2[i])&1))
            {
                if(!((current->uu>>rt1[i])&1)) {
                    if(rs1[i]&&rs2[i]) {
                        alloc_reg64(current,i,rs1[i]);
                        alloc_reg64(current,i,rs2[i]);
                    }
                    else
                    {
                        // Is is really worth it to keep 64-bit values in registers?
#ifdef NATIVE_64BIT
                        if(rs1[i]&&needed_again(rs1[i],i)) alloc_reg64(current,i,rs1[i]);
                        if(rs2[i]&&needed_again(rs2[i],i)) alloc_reg64(current,i,rs2[i]);
#endif
                    }
                    alloc_reg64(current,i,rt1[i]);
                }
                current->is32&=~(1LL<<rt1[i]);
            } else {
                current->is32|=1LL<<rt1[i];
            }
        }
    }
    if(opcode2[i]>=0x2c&&opcode2[i]<=0x2f) { // DADD/DADDU/DSUB/DSUBU
        if(rt1[i]) {
            if(!((current->uu>>rt1[i])&1)) {
                if(rs1[i]&&rs2[i]) {
                    alloc_reg64(current,i,rs1[i]);
                    alloc_reg64(current,i,rs2[i]);
                    alloc_reg64(current,i,rt1[i]);
                }
                else {
                    // DADD used as move, or zeroing
                    // If we have a 64-bit source, then make the target 64 bits too
                    alloc_reg(current,i,rt1[i]);
                    if(rs1[i]&&!((current->is32>>rs1[i])&1)) {
                        if(get_reg(current->regmap,rs1[i])>=0) alloc_reg64(current,i,rs1[i]);
                        alloc_reg64(current,i,rt1[i]);
                    } else if(rs2[i]&&!((current->is32>>rs2[i])&1)) {
                        if(get_reg(current->regmap,rs2[i])>=0) alloc_reg64(current,i,rs2[i]);
                        alloc_reg64(current,i,rt1[i]);
                    }
                    if(opcode2[i]>=0x2e&&rs2[i]) {
                        // DSUB used as negation - 64-bit result
                        // If we have a 32-bit register, extend it to 64 bits
                        if(get_reg(current->regmap,rs2[i])>=0) alloc_reg64(current,i,rs2[i]);
                        alloc_reg64(current,i,rt1[i]);
                    }
                }
            } else {
                if(rs1[i]&&rs2[i]) {
                    alloc_reg(current,i,rs1[i]);
                    alloc_reg(current,i,rs2[i]);
                }
                alloc_reg(current,i,rt1[i]);
            }
            if(rs1[i]&&rs2[i]) {
                current->is32&=~(1LL<<rt1[i]);
            } else if(rs1[i]) {
                current->is32&=~(1LL<<rt1[i]);
                if((current->is32>>rs1[i])&1)
                    current->is32|=1LL<<rt1[i];
            } else if(rs2[i]) {
                current->is32&=~(1LL<<rt1[i]);
                if((current->is32>>rs2[i])&1)
                    current->is32|=1LL<<rt1[i];
            } else {
                current->is32|=1LL<<rt1[i];
            }
        }
    }
    clear_const(current,rs1[i]);
    clear_const(current,rs2[i]);
    dirty_reg(current,rt1[i]);
}

void imm16_alloc(struct regstat *current,int i)
{
    if(rs1[i]&&needed_again(rs1[i],i)) alloc_reg(current,i,rs1[i]);
    if(rt1[i]) alloc_reg(current,i,rt1[i]);
    if(opcode[i]==0x18||opcode[i]==0x19) { // DADDI/DADDIU
        alloc_reg64(current,i,rt1[i]);
        current->is32&=~(1LL<<rt1[i]);
        if(!(((current->uu>>rt1[i])|(current->is32>>rs1[i]))&1)) alloc_reg64(current,i,rs1[i]);
        clear_const(current,rs1[i]);
    }
    else if(opcode[i]==0x0a||opcode[i]==0x0b) { // SLTI/SLTIU
        if((~current->is32>>rs1[i])&1) alloc_reg64(current,i,rs1[i]);
        current->is32|=1LL<<rt1[i];
        clear_const(current,rs1[i]);
    }
    else if(opcode[i]>=0x0c&&opcode[i]<=0x0e) { // ANDI/ORI/XORI
        if(((~current->is32>>rs1[i])&1)&&opcode[i]>0x0c) {
            if(rs1[i]!=rt1[i]) {
                if(needed_again(rs1[i],i)) alloc_reg64(current,i,rs1[i]);
                alloc_reg64(current,i,rt1[i]);
                current->is32&=~(1LL<<rt1[i]);
            }
        }
        else current->is32|=1LL<<rt1[i]; // ANDI clears upper bits
        if(is_const(current,rs1[i])) {
            int v=get_const(current,rs1[i]);
            if(opcode[i]==0x0c) set_const(current,rt1[i],v&imm[i]);
            if(opcode[i]==0x0d) set_const(current,rt1[i],v|imm[i]);
            if(opcode[i]==0x0e) set_const(current,rt1[i],v^imm[i]);
        }
    }
    else if(opcode[i]==0x08||opcode[i]==0x09) { // ADDI/ADDIU
        if(is_const(current,rs1[i])) {
            int v=get_const(current,rs1[i]);
            set_const(current,rt1[i],v+imm[i]);
        }
        current->is32|=1LL<<rt1[i];
    }
    else {
        set_const(current,rt1[i],((long long)((short)imm[i]))<<16); // LUI
        current->is32|=1LL<<rt1[i];
    }
    dirty_reg(current,rt1[i]);
}

void load_alloc(struct regstat *current,int i)
{
    clear_const(current,rt1[i]);
    //if(rs1[i]!=rt1[i]&&needed_again(rs1[i],i)) clear_const(current,rs1[i]); // Does this help or hurt?
    if(!rs1[i]) current->u&=~1LL; // Allow allocating r0 if it's the source register
    if(needed_again(rs1[i],i)) alloc_reg(current,i,rs1[i]);
    if(rt1[i]) {
        alloc_reg(current,i,rt1[i]);
        if(opcode[i]==0x27||opcode[i]==0x37) // LWU/LD
        {
            current->is32&=~(1LL<<rt1[i]);
            alloc_reg64(current,i,rt1[i]);
        }
        else if(opcode[i]==0x1A||opcode[i]==0x1B) // LDL/LDR
        {
            current->is32&=~(1LL<<rt1[i]);
            alloc_reg64(current,i,rt1[i]);
            alloc_all(current,i);
            alloc_reg64(current,i,FTEMP);
        }
        else current->is32|=1LL<<rt1[i];
        dirty_reg(current,rt1[i]);
        // LWL/LWR need a temporary register for the old value
        if(opcode[i]==0x22||opcode[i]==0x26)
        {
            alloc_reg(current,i,FTEMP);
            alloc_reg_temp(current,i,-1);
        }
    }
    else
    {
        // Load to r0 (dummy load)
        // but we still need a register to calculate the address
        alloc_reg_temp(current,i,-1);
    }
}

void store_alloc(struct regstat *current,int i)
{
    clear_const(current,rs2[i]);
    if(!(rs2[i])) current->u&=~1LL; // Allow allocating r0 if necessary
    if(needed_again(rs1[i],i)) alloc_reg(current,i,rs1[i]);
    alloc_reg(current,i,rs2[i]);
    if(opcode[i]==0x2c||opcode[i]==0x2d||opcode[i]==0x3f) { // 64-bit SDL/SDR/SD
        alloc_reg64(current,i,rs2[i]);
    }
#if defined(HOST_IMM8)
    // On CPUs without 32-bit immediates we need a pointer to invalid_code
    alloc_reg(current,i,INVCP);
#endif
    // We need a temporary register for address generation
    alloc_reg_temp(current,i,-1);
}

void c1ls_alloc(struct regstat *current,int i)
{
    clear_const(current,rs1[i]); // FIXME
    if(needed_again(rs1[i],i)) alloc_reg(current,i,rs1[i]);
    alloc_reg(current,i,CSREG); // Status
    alloc_reg(current,i,FTEMP);
    if(opcode[i]==0x35||opcode[i]==0x3d) { // 64-bit LDC1/SDC1
        alloc_reg64(current,i,FTEMP);
    }
#if defined(HOST_IMM8)
    // On CPUs without 32-bit immediates we need a pointer to invalid_code
    if((opcode[i]&0x3b)==0x39) // SWC1/SDC1
        alloc_reg(current,i,INVCP);
#endif
    // We need a temporary register for address generation
    alloc_reg_temp(current,i,-1);
}

#ifndef multdiv_alloc
void multdiv_alloc(struct regstat *current,int i)
{
    //  case 0x18: MULT
    //  case 0x19: MULTU
    //  case 0x1A: DIV
    //  case 0x1B: DIVU
    //  case 0x1C: DMULT
    //  case 0x1D: DMULTU
    //  case 0x1E: DDIV
    //  case 0x1F: DDIVU
    clear_const(current,rs1[i]);
    clear_const(current,rs2[i]);
    if(rs1[i]&&rs2[i])
    {
        if((opcode2[i]&4)==0) // 32-bit
        {
            current->u&=~(1LL<<HIREG);
            current->u&=~(1LL<<LOREG);
            alloc_reg(current,i,HIREG);
            alloc_reg(current,i,LOREG);
            alloc_reg(current,i,rs1[i]);
            alloc_reg(current,i,rs2[i]);
            current->is32|=1LL<<HIREG;
            current->is32|=1LL<<LOREG;
            dirty_reg(current,HIREG);
            dirty_reg(current,LOREG);
        }
        else // 64-bit
        {
            current->u&=~(1LL<<HIREG);
            current->u&=~(1LL<<LOREG);
            current->uu&=~(1LL<<HIREG);
            current->uu&=~(1LL<<LOREG);
            alloc_reg64(current,i,HIREG);
            //if(HOST_REGS>10) alloc_reg64(current,i,LOREG);
            alloc_reg64(current,i,rs1[i]);
            alloc_reg64(current,i,rs2[i]);
            alloc_all(current,i);
            current->is32&=~(1LL<<HIREG);
            current->is32&=~(1LL<<LOREG);
            dirty_reg(current,HIREG);
            dirty_reg(current,LOREG);
        }
    }
    else
    {
        // Multiply by zero is zero.
        // MIPS does not have a divide by zero exception.
        // The result is undefined, we return zero.
        alloc_reg(current,i,HIREG);
        alloc_reg(current,i,LOREG);
        current->is32|=1LL<<HIREG;
        current->is32|=1LL<<LOREG;
        dirty_reg(current,HIREG);
        dirty_reg(current,LOREG);
    }
}
#endif

void cop0_alloc(struct regstat *current,int i)
{
    if(opcode2[i]==0) // MFC0
    {
        assert(rt1[i]);
        alloc_all(current,i);
        alloc_reg(current,i,rt1[i]);
        current->is32|=1LL<<rt1[i];
        dirty_reg(current,rt1[i]);
    }
    else if(opcode2[i]==4) // MTC0
    {
        if(rs1[i]){
            clear_const(current,rs1[i]);
            alloc_reg(current,i,rs1[i]);
            alloc_all(current,i);
        }
        else {
            alloc_all(current,i); // FIXME: Keep r0
            current->u&=~1LL;
            alloc_reg(current,i,0);
        }
    }
    else
    {
        // TLBR/TLBWI/TLBWR/TLBP/ERET
        assert(opcode2[i]==0x10);
        alloc_all(current,i);
    }
}

void cop1_alloc(struct regstat *current,int i)
{
    alloc_reg(current,i,CSREG); // Load status
    if(opcode2[i]<3) // MFC1/DMFC1/CFC1
    {
        assert(rt1[i]);
        alloc_all(current,i);
        if(opcode2[i]==1) {
            alloc_reg64(current,i,rt1[i]); // DMFC1
            current->is32&=~(1LL<<rt1[i]);
        }else{
            alloc_reg(current,i,rt1[i]); // MFC1/CFC1
            current->is32|=1LL<<rt1[i];
        }
        dirty_reg(current,rt1[i]);
    }
    else if(opcode2[i]>3) // MTC1/DMTC1/CTC1
    {
        if(rs1[i]){
            clear_const(current,rs1[i]);
            if(opcode2[i]==5)
                alloc_reg64(current,i,rs1[i]); // DMTC1
            else
                alloc_reg(current,i,rs1[i]); // MTC1/CTC1
            alloc_all(current,i);
        }
        else {
            alloc_all(current,i); // FIXME: Keep r0
            current->u&=~1LL;
            alloc_reg(current,i,0);
        }
    }
}
void fconv_alloc(struct regstat *current,int i)
{
    alloc_reg(current,i,CSREG); // Load status
    alloc_all(current,i);
}
void float_alloc(struct regstat *current,int i)
{
    alloc_reg(current,i,CSREG); // Load status
    alloc_all(current,i);
}

void syscall_alloc(struct regstat *current,int i)
{
    alloc_cc(current,i);
    dirty_reg(current,CCREG);
    alloc_all(current,i);
    current->isconst=0;
}

void delayslot_alloc(struct regstat *current,int i)
{
    switch(itype[i]) {
        case UJUMP:
        case CJUMP:
        case SJUMP:
        case RJUMP:
        case FJUMP:
        case SYSCALL:
            printf("jump in the delay slot.  this shouldn't happen.\n");exit(1);
            break;
        case IMM16:
            imm16_alloc(current,i);
            break;
        case LOAD:
        case LOADLR:
            load_alloc(current,i);
            break;
        case STORE:
        case STORELR:
            store_alloc(current,i);
            break;
        case ALU:
            alu_alloc(current,i);
            break;
        case SHIFT:
            shift_alloc(current,i);
            break;
        case MULTDIV:
            multdiv_alloc(current,i);
            break;
        case SHIFTIMM:
            shiftimm_alloc(current,i);
            break;
        case MOV:
            mov_alloc(current,i);
            break;
        case COP0:
            cop0_alloc(current,i);
            break;
        case COP1:
            cop1_alloc(current,i);
            break;
        case C1LS:
            c1ls_alloc(current,i);
            break;
        case FCONV:
            fconv_alloc(current,i);
            break;
        case FLOAT:
            float_alloc(current,i);
            break;
    }
    current->isconst=0;
}

void add_stub(int type,int addr,int retaddr,int a,int b,int c,int d)
{
    stubs[stubcount][0]=type;
    stubs[stubcount][1]=addr;
    stubs[stubcount][2]=retaddr;
    stubs[stubcount][3]=a;
    stubs[stubcount][4]=b;
    stubs[stubcount][5]=c;
    stubs[stubcount][6]=d;
    stubcount++;
}

// Write out a single register
void wb_register(signed char r,signed char regmap[],uint64_t dirty,uint64_t is32)
{
    int hr;
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && hr!=EXCLUDE_REG) {
            if((regmap[hr]&63)==r) {
                if((dirty>>hr)&1) {
                    if(regmap[hr]<64) {
                        emit_storereg(r,hr);
                        if((is32>>regmap[hr])&1) {
                            emit_sarimm(hr,31,hr);
                            emit_storereg(r|64,hr);
                        }
                    }else{
                        emit_storereg(r|64,hr);
                    }
                }
            }
        }
    }
}

int mchecksum()
{
    int i;
    int sum=0;
    for(i=0;i<2097152;i++) {
        unsigned int temp=sum;
        sum<<=1;
        sum|=(~temp)>>31;
        sum^=((u_int *)rdram)[i];
    }
    return sum;
}
int rchecksum()
{
    int i;
    int sum=0;
    for(i=0;i<64;i++)
        sum^=((u_int *)reg)[i];
    return sum;
}
int fchecksum()
{
    int i;
    int sum=0;
    for(i=0;i<64;i++)
        sum^=((u_int *)reg_cop1_fgr_64)[i];
    return sum;
}
void rlist()
{
    int i;
    printf("TRACE: ");
    for(i=0;i<32;i++)
        printf("r%d:%8x%8x ",i,((int *)(reg+i))[1],((int *)(reg+i))[0]);
    printf("\n");
    printf("TRACE: ");
    for(i=0;i<32;i++)
        printf("f%d:%8x%8x ",i,((int*)reg_cop1_simple[i])[1],*((int*)reg_cop1_simple[i]));
    printf("\n");
}

void memdebug(int i)
{
    //printf("TRACE: count=%d next=%d (checksum %x) lo=%8x%8x\n",Count,next_interupt,mchecksum(),(int)(reg[LOREG]>>32),(int)reg[LOREG]);
    //printf("TRACE: count=%d next=%d (rchecksum %x)\n",Count,next_interupt,rchecksum());
    //rlist();
    //if(/*tracedebug* /Count>=522616700*/1) {
    if(Count>=27631096) {
        printf("TRACE: count=%d next=%d (checksum %x)\n",Count,next_interupt,mchecksum());
        //printf("TRACE: count=%d next=%d (checksum %x) Status=%x\n",Count,next_interupt,mchecksum(),Status);
        //printf("TRACE: count=%d next=%d (checksum %x) hi=%8x%8x\n",Count,next_interupt,mchecksum(),(int)(reg[HIREG]>>32),(int)reg[HIREG]);
        rlist();
        printf("TRACE: %x\n",(&i)[-1]);
    }
    //printf("TRACE: %x\n",(&i)[-1]);
}

void alu_assemble(int i,signed char regmap[])
{
    if(opcode2[i]>=0x20&&opcode2[i]<=0x23) { // ADD/ADDU/SUB/SUBU
        if(rt1[i]) {
            signed char s1,s2,t;
            t=get_reg(regmap,rt1[i]);
            if(t>=0) {
                s1=get_reg(regmap,rs1[i]);
                s2=get_reg(regmap,rs2[i]);
                if(rs1[i]&&rs2[i]) {
                    assert(s1>=0);
                    assert(s2>=0);
                    if(opcode2[i]&2) emit_sub(s1,s2,t);
                    else emit_add(s1,s2,t);
                }
                else if(rs1[i]) {
                    if(s1>=0) emit_mov(s1,t);
                    else emit_loadreg(rs1[i],t);
                }
                else if(rs2[i]) {
                    if(s2>=0) {
                        if(opcode2[i]&2) emit_neg(s2,t);
                        else emit_mov(s2,t);
                    }
                    else {
                        emit_loadreg(rs2[i],t);
                        if(opcode2[i]&2) emit_neg(t,t);
                    }
                }
                else emit_zeroreg(t);
            }
        }
    }
    if(opcode2[i]>=0x2c&&opcode2[i]<=0x2f) { // DADD/DADDU/DSUB/DSUBU
        if(rt1[i]) {
            signed char s1l,s2l,s1h,s2h,tl,th;
            tl=get_reg(regmap,rt1[i]);
            th=get_reg(regmap,rt1[i]|64);
            if(tl>=0) {
                s1l=get_reg(regmap,rs1[i]);
                s2l=get_reg(regmap,rs2[i]);
                s1h=get_reg(regmap,rs1[i]|64);
                s2h=get_reg(regmap,rs2[i]|64);
                if(rs1[i]&&rs2[i]) {
                    assert(s1l>=0);
                    assert(s2l>=0);
                    if(opcode2[i]&2) emit_subs(s1l,s2l,tl);
                    else emit_adds(s1l,s2l,tl);
                    if(th>=0) {
#ifdef INVERTED_CARRY
                        if(opcode2[i]&2) {if(s1h!=th) emit_mov(s1h,th);emit_sbb(th,s2h);}
#else
                        if(opcode2[i]&2) emit_sbc(s1h,s2h,th);
#endif
                        else emit_add(s1h,s2h,th);
                    }
                }
                else if(rs1[i]) {
                    if(s1l>=0) emit_mov(s1l,tl);
                    else emit_loadreg(rs1[i],tl);
                    if(th>=0) {
                        if(s1h>=0) emit_mov(s1h,th);
                        else emit_loadreg(rs1[i]|64,th);
                    }
                }
                else if(rs2[i]) {
                    if(s2l>=0) {
                        if(opcode2[i]&2) emit_negs(s2l,tl);
                        else emit_mov(s2l,tl);
                    }
                    else {
                        emit_loadreg(rs2[i],tl);
                        if(opcode2[i]&2) emit_negs(tl,tl);
                    }
                    if(th>=0) {
#ifdef INVERTED_CARRY
                        if(s2h>=0) emit_mov(s2h,th);
                        else emit_loadreg(rs2[i]|64,th);
                        if(opcode2[i]&2) {
                            emit_adcimm(-1,th); // x86 has inverted carry flag
                            emit_not(th,th);
                        }
#else
                        if(opcode2[i]&2) {
                            if(s2h>=0) emit_rscimm(s2h,0,th);
                            else {
                                emit_loadreg(rs2[i]|64,th);
                                emit_rscimm(th,0,th);
                            }
                        }else{
                            if(s2h>=0) emit_mov(s2h,th);
                            else emit_loadreg(rs2[i]|64,th);
                        }
#endif
                    }
                }
                else {
                    emit_zeroreg(tl);
                    if(th>=0) emit_zeroreg(th);
                }
            }
        }
    }
    if(opcode2[i]==0x2a||opcode2[i]==0x2b) { // SLT/SLTU
        if(rt1[i]) {
            signed char s1l,s1h,s2l,s2h,t;
            if(!((is32[i]>>rs1[i])&(is32[i]>>rs2[i])&1))
            {
                t=get_reg(regmap,rt1[i]);
                assert(t>=0);
                if(t>=0) {
                    s1l=get_reg(regmap,rs1[i]);
                    s1h=get_reg(regmap,rs1[i]|64);
                    s2l=get_reg(regmap,rs2[i]);
                    s2h=get_reg(regmap,rs2[i]|64);
                    if(rs2[i]==0) // rx<r0
                    {
                        assert(s1h>=0);
                        if(opcode2[i]==0x2a) // SLT
                            emit_shrimm(s1h,31,t);
                        else // SLTU (unsigned can not be less than zero)
                            emit_zeroreg(t);
                    }
                    else if(rs1[i]==0) // r0<rx
                    {
                        assert(s2h>=0);
                        if(opcode2[i]==0x2a) // SLT
                            emit_set_gz64_32(s2h,s2l,t);
                        else // SLTU (set if not zero)
                            emit_set_nz64_32(s2h,s2l,t);
                    }
                    else {
                        assert(s1l>=0);assert(s1h>=0);
                        assert(s2l>=0);assert(s2h>=0);
                        if(opcode2[i]==0x2a) // SLT
                            emit_set_if_less64_32(s1h,s1l,s2h,s2l,t);
                        else // SLTU
                            emit_set_if_carry64_32(s1h,s1l,s2h,s2l,t);
                    }
                }
            } else {
                t=get_reg(regmap,rt1[i]);
                //assert(t>=0);
                if(t>=0) {
                    s1l=get_reg(regmap,rs1[i]);
                    s2l=get_reg(regmap,rs2[i]);
                    if(rs2[i]==0) // rx<r0
                    {
                        assert(s1l>=0);
                        if(opcode2[i]==0x2a) // SLT
                            emit_shrimm(s1l,31,t);
                        else // SLTU (unsigned can not be less than zero)
                            emit_zeroreg(t);
                    }
                    else if(rs1[i]==0) // r0<rx
                    {
                        assert(s2l>=0);
                        if(opcode2[i]==0x2a) // SLT
                            emit_set_gz32(s2l,t);
                        else // SLTU (set if not zero)
                            emit_set_nz32(s2l,t);
                    }
                    else{
                        assert(s1l>=0);assert(s2l>=0);
                        if(opcode2[i]==0x2a) // SLT
                            emit_set_if_less32(s1l,s2l,t);
                        else // SLTU
                            emit_set_if_carry32(s1l,s2l,t);
                    }
                }
            }
        }
    }
    if(opcode2[i]>=0x24&&opcode2[i]<=0x27) { // AND/OR/XOR/NOR
        if(rt1[i]) {
            signed char s1l,s1h,s2l,s2h,th,tl;
            tl=get_reg(regmap,rt1[i]);
            th=get_reg(regmap,rt1[i]|64);
            if(!((is32[i]>>rs1[i])&(is32[i]>>rs2[i])&1)&&th>=0)
            {
                assert(tl>=0);
                if(tl>=0) {
                    s1l=get_reg(regmap,rs1[i]);
                    s1h=get_reg(regmap,rs1[i]|64);
                    s2l=get_reg(regmap,rs2[i]);
                    s2h=get_reg(regmap,rs2[i]|64);
                    if(rs1[i]&&rs2[i]) {
                        assert(s1l>=0);assert(s1h>=0);
                        assert(s2l>=0);assert(s2h>=0);
                        if(opcode2[i]==0x24) { // AND
                            emit_and(s1l,s2l,tl);
                            emit_and(s1h,s2h,th);
                        } else
                            if(opcode2[i]==0x25) { // OR
                                emit_or(s1l,s2l,tl);
                                emit_or(s1h,s2h,th);
                            } else
                                if(opcode2[i]==0x26) { // XOR
                                    emit_xor(s1l,s2l,tl);
                                    emit_xor(s1h,s2h,th);
                                } else
                                    if(opcode2[i]==0x27) { // NOR
                                        emit_or(s1l,s2l,tl);
                                        emit_or(s1h,s2h,th);
                                        emit_not(tl,tl);
                                        emit_not(th,th);
                                    }
                    }
                    else
                    {
                        if(opcode2[i]==0x24) { // AND
                            emit_zeroreg(tl);
                            emit_zeroreg(th);
                        } else
                            if(opcode2[i]==0x25||opcode2[i]==0x26) { // OR/XOR
                                if(rs1[i]){
                                    if(s1l>=0) emit_mov(s1l,tl);
                                    else emit_loadreg(rs1[i],tl);
                                    if(s1h>=0) emit_mov(s1h,th);
                                    else emit_loadreg(rs1[i]|64,th);
                                }
                                else
                                    if(rs2[i]){
                                        if(s2l>=0) emit_mov(s2l,tl);
                                        else emit_loadreg(rs2[i],tl);
                                        if(s2h>=0) emit_mov(s2h,th);
                                        else emit_loadreg(rs2[i]|64,th);
                                    }
                                    else{
                                        emit_zeroreg(tl);
                                        emit_zeroreg(th);
                                    }
                            } else
                                if(opcode2[i]==0x27) { // NOR
                                    if(rs1[i]){
                                        if(s1l>=0) emit_not(s1l,tl);
                                        else{
                                            emit_loadreg(rs1[i],tl);
                                            emit_not(tl,tl);
                                        }
                                        if(s1h>=0) emit_not(s1h,th);
                                        else{
                                            emit_loadreg(rs1[i]|64,th);
                                            emit_not(th,th);
                                        }
                                    }
                                    else
                                        if(rs2[i]){
                                            if(s2l>=0) emit_not(s2l,tl);
                                            else{
                                                emit_loadreg(rs2[i],tl);
                                                emit_not(tl,tl);
                                            }
                                            if(s2h>=0) emit_not(s2h,th);
                                            else{
                                                emit_loadreg(rs2[i]|64,th);
                                                emit_not(th,th);
                                            }
                                        }
                                        else {
                                            emit_movimm(-1,tl);
                                            emit_movimm(-1,th);
                                        }
                                }
                    }
                }
            }
            else
            {
                // 32 bit
                if(tl>=0) {
                    s1l=get_reg(regmap,rs1[i]);
                    s2l=get_reg(regmap,rs2[i]);
                    if(rs1[i]&&rs2[i]) {
                        assert(s1l>=0);
                        assert(s2l>=0);
                        if(opcode2[i]==0x24) { // AND
                            emit_and(s1l,s2l,tl);
                        } else
                            if(opcode2[i]==0x25) { // OR
                                emit_or(s1l,s2l,tl);
                            } else
                                if(opcode2[i]==0x26) { // XOR
                                    emit_xor(s1l,s2l,tl);
                                } else
                                    if(opcode2[i]==0x27) { // NOR
                                        emit_or(s1l,s2l,tl);
                                        emit_not(tl,tl);
                                    }
                    }
                    else
                    {
                        if(opcode2[i]==0x24) { // AND
                            emit_zeroreg(tl);
                        } else
                            if(opcode2[i]==0x25||opcode2[i]==0x26) { // OR/XOR
                                if(rs1[i]){
                                    if(s1l>=0) emit_mov(s1l,tl);
                                    else emit_loadreg(rs1[i],tl); // CHECK: regmap_entry?
                                }
                                else
                                    if(rs2[i]){
                                        if(s2l>=0) emit_mov(s2l,tl);
                                        else emit_loadreg(rs2[i],tl); // CHECK: regmap_entry?
                                    }
                                    else emit_zeroreg(tl);
                            } else
                                if(opcode2[i]==0x27) { // NOR
                                    if(rs1[i]){
                                        if(s1l>=0) emit_not(s1l,tl);
                                        else {
                                            emit_loadreg(rs1[i],tl);
                                            emit_not(tl,tl);
                                        }
                                    }
                                    else
                                        if(rs2[i]){
                                            if(s2l>=0) emit_not(s2l,tl);
                                            else {
                                                emit_loadreg(rs2[i],tl);
                                                emit_not(tl,tl);
                                            }
                                        }
                                        else emit_movimm(-1,tl);
                                }
                    }
                }
            }
        }
    }
}

void imm16_assemble(int i,signed char regmap[])
{
    if (opcode[i]==0x0f) { // LUI
        if(rt1[i]) {
            signed char t;
            t=get_reg(regmap,rt1[i]);
            //assert(t>=0);
            if(t>=0) {
                if(!((isconst[i]>>t)&1))
                    emit_movimm(imm[i]<<16,t);
            }
        }
    }
    if(opcode[i]==0x08||opcode[i]==0x09) { // ADDI/ADDIU
        if(rt1[i]) {
            signed char s,t;
            t=get_reg(regmap,rt1[i]);
            s=get_reg(regmap,rs1[i]);
            if(rs1[i]) {
                //assert(t>=0);
                //assert(s>=0);
                if(t>=0) {
                    if(!((isconst[i]>>t)&1)) {
                        if(s<0) {
                            emit_loadreg(rs1[i],t);
                            emit_addimm(t,imm[i],t);
                        }else{
                            if(!((isconst[i]>>s)&1))
                                emit_addimm(s,imm[i],t);
                            else
                                emit_movimm(constmap[i][s]+imm[i],t);
                        }
                    }
                }
            } else {
                if(t>=0) {
                    if(!((isconst[i]>>t)&1))
                        emit_movimm(imm[i],t);
                }
            }
        }
    }
    if(opcode[i]==0x18||opcode[i]==0x19) { // DADDI/DADDIU
        if(rt1[i]) {
            signed char sh,sl,th,tl;
            th=get_reg(regmap,rt1[i]|64);
            tl=get_reg(regmap,rt1[i]);
            sh=get_reg(regmap,rs1[i]|64);
            sl=get_reg(regmap,rs1[i]);
            if(tl>=0) {
                if(rs1[i]) {
                    assert(sh>=0);
                    assert(sl>=0);
                    if(th>=0) {
                        emit_addimm64_32(sh,sl,imm[i],th,tl);
                    }
                    else {
                        emit_addimm(sl,imm[i],tl);
                    }
                } else {
                    emit_movimm(imm[i],tl);
                    if(th>=0) emit_movimm(((signed int)imm[i])>>31,th);
                }
            }
        }
    }
    else if(opcode[i]==0x0a||opcode[i]==0x0b) { // SLTI/SLTIU
        if(rt1[i]) {
            //assert(rs1[i]!=0); // r0 might be valid, but it's probably a bug
            signed char sh,sl,t;
            t=get_reg(regmap,rt1[i]);
            sh=get_reg(regmap,rs1[i]|64);
            sl=get_reg(regmap,rs1[i]);
            //assert(t>=0);
            if(t>=0) {
                if(rs1[i]>0) {
                    //if(sh<0) assert((is32[i]>>rs1[i])&1);
                    if(sh<0||((is32[i]>>rs1[i])&1)) {
                        if(opcode[i]==0x0a) { // SLTI
                            if(sl<0) {
                                emit_loadreg(rs1[i],t);
                                emit_slti32(t,imm[i],t);
                            }else{
                                emit_slti32(sl,imm[i],t);
                            }
                        }
                        else { // SLTIU
                            if(sl<0) {
                                emit_loadreg(rs1[i],t);
                                emit_sltiu32(t,imm[i],t);
                            }else{
                                emit_sltiu32(sl,imm[i],t);
                            }
                        }
                    }else{ // 64-bit
                        assert(sl>=0);
                        if(opcode[i]==0x0a) // SLTI
                            emit_slti64_32(sh,sl,imm[i],t);
                        else // SLTIU
                            emit_sltiu64_32(sh,sl,imm[i],t);
                    }
                }else{
                    // SLTI(U) with r0 is just stupid,
                    // nonetheless examples can be found
                    if(opcode[i]==0x0a) // SLTI
                        if(0<imm[i]) emit_movimm(1,t);
                        else emit_zeroreg(t);
                        else // SLTIU
                        {
                            if(imm[i]) emit_movimm(1,t);
                            else emit_zeroreg(t);
                        }
                }
            }
        }
    }
    else if(opcode[i]>=0x0c&&opcode[i]<=0x0e) { // ANDI/ORI/XORI
        if(rt1[i]) {
            signed char sh,sl,th,tl;
            th=get_reg(regmap,rt1[i]|64);
            tl=get_reg(regmap,rt1[i]);
            sh=get_reg(regmap,rs1[i]|64);
            sl=get_reg(regmap,rs1[i]);
            if(tl>=0 && !((isconst[i]>>tl)&1)) {
                if(opcode[i]==0x0c) //ANDI
                {
                    if(rs1[i]) {
                        if(sl<0) {
                            emit_loadreg(rs1[i],tl);
                            emit_andimm(tl,imm[i],tl);
                        }else{
                            if(!((isconst[i]>>sl)&1))
                                emit_andimm(sl,imm[i],tl);
                            else
                                emit_movimm(constmap[i][sl]&imm[i],tl);
                        }
                    }
                    else
                        emit_zeroreg(tl);
                    if(th>=0) emit_zeroreg(th);
                }
                else
                {
                    if(rs1[i]) {
                        if(sl<0) {
                            emit_loadreg(rs1[i],tl);
                        }
                        if(th>=0) {
                            if(sh<0) {
                                emit_loadreg(rs1[i]|64,th);
                            }else{
                                emit_mov(sh,th);
                            }
                        }
                        if(opcode[i]==0x0d) //ORI
                            if(sl<0) {
                                emit_orimm(tl,imm[i],tl);
                            }else{
                                if(!((isconst[i]>>sl)&1))
                                    emit_orimm(sl,imm[i],tl);
                                else
                                    emit_movimm(constmap[i][sl]|imm[i],tl);
                            }
                        if(opcode[i]==0x0e) //XORI
                            if(sl<0) {
                                emit_xorimm(tl,imm[i],tl);
                            }else{
                                if(!((isconst[i]>>sl)&1))
                                    emit_xorimm(sl,imm[i],tl);
                                else
                                    emit_movimm(constmap[i][sl]^imm[i],tl);
                            }
                    }
                    else {
                        emit_movimm(imm[i],tl);
                        if(th>=0) emit_zeroreg(th);
                    }
                }
            }
        }
    }
}

void shiftimm_assemble(int i,signed char regmap[])
{
    if(opcode2[i]<=0x3) // SLL/SRL/SRA
    {
        if(rt1[i]) {
            signed char s,t;
            t=get_reg(regmap,rt1[i]);
            s=get_reg(regmap,rs1[i]);
            //assert(t>=0);
            if(t>=0){
                if(rs1[i]==0)
                {
                    emit_zeroreg(t);
                }
                else
                {
                    if(s<0) emit_loadreg(rs1[i],t);
                    if(imm[i]) {
                        if(opcode2[i]==0) // SLL
                        {
                            emit_shlimm(s<0?t:s,imm[i],t);
                        }
                        if(opcode2[i]==2) // SRL
                        {
                            emit_shrimm(s<0?t:s,imm[i],t);
                        }
                        if(opcode2[i]==3) // SRA
                        {
                            emit_sarimm(s<0?t:s,imm[i],t);
                        }
                    }else{
                        // Shift by zero
                        if(s>=0 && s!=t) emit_mov(s,t);
                    }
                }
            }
            //emit_storereg(rt1[i],t); //DEBUG
        }
    }
    if(opcode2[i]>=0x38&&opcode2[i]<=0x3b) // DSLL/DSRL/DSRA
    {
        if(rt1[i]) {
            signed char sh,sl,th,tl;
            th=get_reg(regmap,rt1[i]|64);
            tl=get_reg(regmap,rt1[i]);
            sh=get_reg(regmap,rs1[i]|64);
            sl=get_reg(regmap,rs1[i]);
            if(tl>=0) {
                if(rs1[i]==0)
                {
                    emit_zeroreg(tl);
                    if(th>=0) emit_zeroreg(th);
                }
                else
                {
                    assert(sl>=0);
                    assert(sh>=0);
                    if(imm[i]) {
                        if(opcode2[i]==0x38) // DSLL
                        {
                            if(th>=0) emit_shldimm(sh,sl,imm[i],th);
                            emit_shlimm(sl,imm[i],tl);
                        }
                        if(opcode2[i]==0x3a) // DSRL
                        {
                            emit_shrdimm(sl,sh,imm[i],tl);
                            if(th>=0) emit_shrimm(sh,imm[i],th);
                        }
                        if(opcode2[i]==0x3b) // DSRA
                        {
                            emit_shrdimm(sl,sh,imm[i],tl);
                            if(th>=0) emit_sarimm(sh,imm[i],th);
                        }
                    }else{
                        // Shift by zero
                        if(sl!=tl) emit_mov(sl,tl);
                        if(th>=0&&sh!=th) emit_mov(sh,th);
                    }
                }
            }
        }
    }
    if(opcode2[i]==0x3c) // DSLL32
    {
        if(rt1[i]) {
            signed char sl,tl,th;
            tl=get_reg(regmap,rt1[i]);
            th=get_reg(regmap,rt1[i]|64);
            sl=get_reg(regmap,rs1[i]);
            if(th>=0||tl>=0){
                assert(tl>=0);
                assert(th>=0);
                assert(sl>=0);
                emit_mov(sl,th);
                emit_zeroreg(tl);
                if(imm[i]>32)
                {
                    emit_shlimm(th,imm[i]&31,th);
                }
            }
        }
    }
    if(opcode2[i]==0x3e) // DSRL32
    {
        if(rt1[i]) {
            signed char sh,tl,th;
            tl=get_reg(regmap,rt1[i]);
            th=get_reg(regmap,rt1[i]|64);
            sh=get_reg(regmap,rs1[i]|64);
            if(tl>=0){
                assert(sh>=0);
                emit_mov(sh,tl);
                if(th>=0) emit_zeroreg(th);
                if(imm[i]>32)
                {
                    emit_shrimm(tl,imm[i]&31,tl);
                }
            }
        }
    }
    if(opcode2[i]==0x3f) // DSRA32
    {
        if(rt1[i]) {
            signed char sh,tl;
            tl=get_reg(regmap,rt1[i]);
            sh=get_reg(regmap,rs1[i]|64);
            if(tl>=0){
                assert(sh>=0);
                emit_mov(sh,tl);
                if(imm[i]>32)
                {
                    emit_sarimm(tl,imm[i]&31,tl);
                }
            }
        }
    }
}

#ifndef shift_assemble
void shift_assemble(int i,signed char regmap[])
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
                    signed char temp=get_reg(regmap,-1);
                    assert(s>=0);
                    if(t==ECX&&s!=ECX) {
                        if(shift!=ECX) emit_mov(shift,ECX);
                        if(rt1[i]==rs2[i]) {shift=temp;}
                        if(s!=shift) emit_mov(s,shift);
                    }
                    else
                    {
                        if(rt1[i]==rs2[i]) {emit_mov(shift,temp);shift=temp;}
                        if(s!=t) emit_mov(s,t);
                        if(shift!=ECX) {
                            if(regmap[ECX]<0)
                                emit_mov(shift,ECX);
                            else
                                emit_xchg(shift,ECX);
                        }
                    }
                    if(opcode2[i]==4) // SLLV
                    {
                        emit_shlcl(t==ECX?shift:t);
                    }
                    if(opcode2[i]==6) // SRLV
                    {
                        emit_shrcl(t==ECX?shift:t);
                    }
                    if(opcode2[i]==7) // SRAV
                    {
                        emit_sarcl(t==ECX?shift:t);
                    }
                    if(shift!=ECX&&regmap[ECX]>=0) emit_xchg(shift,ECX);
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
                    int temp=get_reg(regmap,-1);
                    int real_th=th;
                    if(th<0&&opcode2[i]!=0x14) {th=temp;} // DSLLV doesn't need a temporary register
                    assert(sl>=0);
                    assert(sh>=0);
                    if(tl==ECX&&sl!=ECX) {
                        if(shift!=ECX) emit_mov(shift,ECX);
                        if(sl!=shift) emit_mov(sl,shift);
                    }
                    else if(th==ECX&&sh!=ECX) {
                        if(shift!=ECX) emit_mov(shift,ECX);
                        if(sh!=shift) emit_mov(sh,shift);
                    }
                    else
                    {
                        if(sl!=tl) emit_mov(sl,tl);
                        if(sh!=th) emit_mov(sh,th);
                        if(shift!=ECX) {
                            if(regmap[ECX]<0)
                                emit_mov(shift,ECX);
                            else
                                emit_xchg(shift,ECX);
                        }
                    }
                    if(opcode2[i]==0x14) // DSLLV
                    {
                        if(th>=0) emit_shldcl(th==ECX?shift:th,tl==ECX?shift:tl);
                        emit_shlcl(tl==ECX?shift:tl);
                        emit_testimm(ECX,32);
                        if(th>=0) emit_cmovne_reg(tl==ECX?shift:tl,th==ECX?shift:th);
                        emit_cmovne(&const_zero,tl==ECX?shift:tl);
                    }
                    if(opcode2[i]==0x16) // DSRLV
                    {
                        assert(th>=0);
                        emit_shrdcl(tl==ECX?shift:tl,th==ECX?shift:th);
                        emit_shrcl(th==ECX?shift:th);
                        emit_testimm(ECX,32);
                        emit_cmovne_reg(th==ECX?shift:th,tl==ECX?shift:tl);
                        if(real_th>=0) emit_cmovne(&const_zero,th==ECX?shift:th);
                    }
                    if(opcode2[i]==0x17) // DSRAV
                    {
                        assert(th>=0);
                        emit_shrdcl(tl==ECX?shift:tl,th==ECX?shift:th);
                        if(real_th>=0) {
                            assert(temp>=0);
                            emit_mov(th==ECX?shift:th,temp==ECX?shift:temp);
                        }
                        emit_sarcl(th==ECX?shift:th);
                        if(real_th>=0) emit_sarimm(temp==ECX?shift:temp,31,temp==ECX?shift:temp);
                        emit_testimm(ECX,32);
                        emit_cmovne_reg(th==ECX?shift:th,tl==ECX?shift:tl);
                        if(real_th>=0) emit_cmovne_reg(temp==ECX?shift:temp,th==ECX?shift:th);
                    }
                    if(shift!=ECX&&(regmap[ECX]>=0||temp==ECX)) emit_xchg(shift,ECX);
                }
            }
        }
    }
}
#endif

void load_assemble(int i,signed char regmap[])
{
    int s,th,tl;
    int offset;
    int jaddr;
    int memtarget,c=0;
    u_int hr,reglist=0;
    
    th=get_reg(regmap,rt1[i]|64);
    tl=get_reg(regmap,rt1[i]);
    s=get_reg(regmap,rs1[i]);
    offset=imm[i];
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && regmap[hr]>=0) reglist|=1<<hr;
    }
    
    if(regmap[HOST_CCREG]==CCREG) reglist&=~(1<<HOST_CCREG);
    if(s>=0) {
        c=(wasconst[i]>>s)&1;
        memtarget=((signed int)(constmap[i][s]+offset))<(signed int)0x80800000;
    }
    //fprintf(stderr, "load_assemble: c=%d\n",c); fflush(stderr);
    //if(c) fprintf(stderr, "load_assemble: const=%x\n",(int)constmap[i][s]+offset); fflush(stderr);
    if(tl>=0) {
        //assert(tl>=0);
        //assert(rt1[i]);
        reglist&=~(1<<tl);
        if(th>=0) reglist&=~(1<<th);
        if(s>=0) {
            if(c) {if(rs1[i]!=rt1[i]) emit_movimm(constmap[i][s]+offset,tl);}
            else emit_addimm(s,offset,tl);
        }else{
            emit_loadreg(rs1[i],tl);
            emit_addimm(tl,offset,tl);
        }
        if(!c) {
            emit_cmpimm(tl,0x800000);
            jaddr=(int)out;
            emit_jno(0);
        }
        
        if (opcode[i]==0x20) { // LB
            if(!c||memtarget) {
                emit_xorimm(tl,3,tl);
                emit_movsbl_indexed_rdram((INT)0x80000000-0x80000000,tl,tl);
                if(!c)
                    add_stub(LOADB_STUB,jaddr,(int)out,(int)regmap,rt1[i],ccadj[i],reglist);
            }
            else
                inline_readstub(LOADB_STUB,constmap[i][s]+offset,regmap,rt1[i],ccadj[i],reglist);
        }
        if (opcode[i]==0x21) { // LH
            if(!c||memtarget) {
                emit_xorimm(tl,2,tl);
                emit_movswl_indexed_rdram((INT)0x80000000-0x80000000,tl,tl);
                if(!c)
                    add_stub(LOADH_STUB,jaddr,(int)out,(int)regmap,rt1[i],ccadj[i],reglist);
            }
            else
                inline_readstub(LOADH_STUB,constmap[i][s]+offset,regmap,rt1[i],ccadj[i],reglist);
        }
        if (opcode[i]==0x23) { // LW
            if(!c||memtarget)
            {
                emit_readword_indexed_rdram((INT)0x80000000-0x80000000,tl,tl);
                if(!c)
                {
                    add_stub(LOADW_STUB,jaddr,(int)out,(int)regmap,rt1[i],ccadj[i],reglist);
                }
            }
            else
                inline_readstub(LOADW_STUB,constmap[i][s]+offset,regmap,rt1[i],ccadj[i],reglist);
        }
        if (opcode[i]==0x24) { // LBU
            if(!c||memtarget) {
                emit_xorimm(tl,3,tl);
                emit_movzbl_indexed_rdram((INT)0x80000000-0x80000000,tl,tl);
                if(!c)
                    add_stub(LOADBU_STUB,jaddr,(int)out,(int)regmap,rt1[i],ccadj[i],reglist);
            }
            else
                inline_readstub(LOADBU_STUB,constmap[i][s]+offset,regmap,rt1[i],ccadj[i],reglist);
        }
        if (opcode[i]==0x25) { // LHU
            if(!c||memtarget) {
                emit_xorimm(tl,2,tl);
                emit_movzwl_indexed_rdram((INT)0x80000000-0x80000000,tl,tl);
                if(!c)
                    add_stub(LOADHU_STUB,jaddr,(int)out,(int)regmap,rt1[i],ccadj[i],reglist);
            }
            else
                inline_readstub(LOADHU_STUB,constmap[i][s]+offset,regmap,rt1[i],ccadj[i],reglist);
        }
        if (opcode[i]==0x27) { // LWU
            assert(th>=0);
            if(!c||memtarget) {
                emit_readword_indexed_rdram((INT)0x80000000-0x80000000,tl,tl);
                if(!c)
                    add_stub(LOADW_STUB,jaddr,(int)out,(int)regmap,rt1[i],ccadj[i],reglist);
            }
            else {
                inline_readstub(LOADW_STUB,constmap[i][s]+offset,regmap,rt1[i],ccadj[i],reglist);
            }
            emit_zeroreg(th);
        }
        if (opcode[i]==0x37) { // LD
            if(!c||memtarget) {
                if(th>=0) emit_readword_indexed_rdram((INT)0x80000000-0x80000000,tl,th);
                emit_readword_indexed_rdram((INT)0x80000000-0x7FFFFFFC,tl,tl);
                if(!c)
                    add_stub(LOADD_STUB,jaddr,(int)out,(int)regmap,rt1[i],ccadj[i],reglist);
            }
            else
                inline_readstub(LOADD_STUB,constmap[i][s]+offset,regmap,rt1[i],ccadj[i],reglist);
        }
    }
    
    /*if(opcode[i]==0x23)
     {
     //emit_pusha();
     save_regs(0x100f);
     emit_readword((int)&last_count,ECX);
     if(get_reg(regmap,CCREG)<0)
     emit_loadreg(CCREG,HOST_CCREG);
     emit_add(HOST_CCREG,ECX,HOST_CCREG);
     emit_addimm(HOST_CCREG,2*ccadj[i],HOST_CCREG);
     emit_writeword(HOST_CCREG,(int)&Count);
     emit_call((int)memdebug);
     //emit_popa();
     restore_regs(0x100f);
     }/**/
}

#ifndef loadlr_assemble
void loadlr_assemble(int i,signed char regmap[])
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
        if(hr!=9 && regmap[hr]>=0) reglist|=1<<hr;
    }
    reglist|=1<<temp;
    if(s>=0) {
        c=(wasconst[i]>>s)&1;
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
        emit_lea8(temp2,temp);
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
            if(temp==ECX)
            {
                int temp3=EDX;
                if(temp3==temp2) temp3++;
                emit_pushreg(temp3);
                emit_movimm(-1,temp3);
                if (opcode[i]==0x26) {
                    emit_shrcl(temp3);
                    emit_shrcl(temp2);
                }else{
                    emit_shlcl(temp3);
                    emit_shlcl(temp2);
                }
                emit_mov(temp3,ECX);
                emit_not(ECX,ECX);
                emit_popreg(temp3);
            }
            else
            {
                int temp3=EBP;
                if(temp3==temp) temp3++;
                if(temp3==temp2) temp3++;
                if(temp3==temp) temp3++;
                emit_xchg(ECX,temp);
                emit_pushreg(temp3);
                emit_movimm(-1,temp3);
                if (opcode[i]==0x26) {
                    emit_shrcl(temp3);
                    emit_shrcl(temp2==ECX?temp:temp2);
                }else{
                    emit_shlcl(temp3);
                    emit_shlcl(temp2==ECX?temp:temp2);
                }
                emit_not(temp3,temp3);
                emit_mov(temp,ECX);
                emit_mov(temp3,temp);
                emit_popreg(temp3);
            }
            emit_and(temp,tl,tl);
            emit_or(temp2,tl,tl);
            //emit_storereg(rt1[i],tl); // DEBUG
        }
        if (opcode[i]==0x1A||opcode[i]==0x1B) { // LDL/LDR
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
#endif

void store_assemble(int i,signed char regmap[])
{
    int s,th,tl;
    int temp;
    int offset;
    int jaddr,jaddr2,type;
    int memtarget,c=0;
    u_int hr,reglist=0;
    th=get_reg(regmap,rs2[i]|64);
    tl=get_reg(regmap,rs2[i]);
    s=get_reg(regmap,rs1[i]);
    temp=get_reg(regmap,-1);
    offset=imm[i];
    if(s>=0) {
        c=(isconst[i]>>s)&1;
        memtarget=((signed int)(constmap[i][s]+offset))<(signed int)0x80800000;
    }
    assert(tl>=0);
    assert(rs1[i]>0);
    assert(temp>=0);
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && regmap[hr]>=0) reglist|=1<<hr;
    }
    if(regmap[HOST_CCREG]==CCREG) reglist&=~(1<<HOST_CCREG);
    if(s<0) emit_loadreg(rs1[i],temp);
    if(!c) {
        if(offset!=0) {
            if(s>=0) emit_addimm(s,offset,temp);
            else emit_addimm(temp,offset,temp);
            emit_cmpimm(temp,0x800000);
        }
        else
        {
            if(s>=0) {
                emit_mov(s,temp);
                emit_cmpimm(s,0x800000);
            }else{
                emit_cmpimm(temp,0x800000);
            }
        }
        jaddr=(int)out;
        emit_jno(0);
    }
    else
    {
        emit_movimm(constmap[i][s]+offset,temp);
    }
    if (opcode[i]==0x28) { // SB
        if(!c||memtarget) {
            emit_xorimm(temp,3,temp);
            emit_writebyte_indexed_rdram(tl,(INT)0x80000000-0x80000000,temp);
        }
        type=STOREB_STUB;
    }
    if (opcode[i]==0x29) { // SH
        if(!c||memtarget) {
            emit_xorimm(temp,2,temp);
            emit_writehword_indexed_rdram(tl,(INT)0x80000000-0x80000000,temp);
        }
        type=STOREH_STUB;
    }
    if (opcode[i]==0x2B) { // SW
        if(!c||memtarget)
            emit_writeword_indexed_rdram(tl,(INT)0x80000000-0x80000000,temp);
        type=STOREW_STUB;
    }
    if (opcode[i]==0x3F) { // SD
        if(!c||memtarget) {
            if(rs2[i]) {
                assert(th>=0);
                emit_writeword_indexed_rdram(th,(INT)0x80000000-0x80000000,temp);
                emit_writeword_indexed_rdram(tl,(INT)0x80000000-0x7FFFFFFC,temp);
            }else{
                // Store zero
                emit_writeword_indexed_rdram(tl,(INT)0x80000000-0x80000000,temp);
                emit_writeword_indexed_rdram(tl,(INT)0x80000000-0x7FFFFFFC,temp);
            }
        }
        type=STORED_STUB;
    }
    if(!c||memtarget) {
#if defined(HOST_IMM8)
        int ir=get_reg(regmap,INVCP);
        assert(ir>=0);
        emit_cmpmem_indexed_rdramsr12_reg(ir,temp,1);
#else
        emit_cmpmem_indexed_rdramsr12_imm((int)invalid_code,temp,1);
#endif
        jaddr2=(int)out;
        emit_jne(0);
        add_stub(INVCODE_STUB,jaddr2,(int)out,reglist|(1<<HOST_CCREG),temp,0,0);
    }
    if(!c) {
        add_stub(type,jaddr,(int)out,(int)regmap,rs2[i],ccadj[i],reglist);
    } else if(!memtarget) {
        inline_writestub(type,constmap[i][s]+offset,regmap,rs2[i],ccadj[i],reglist);
    }
    //if(opcode[i]==0x2B || opcode[i]==0x3F)
    //if(opcode[i]==0x2B || opcode[i]==0x28)
    /*if(opcode[i]==0x2B)
     //if(opcode[i]==0x2B || opcode[i]==0x28 || opcode[i]==0x29 || opcode[i]==0x3F)
     {
     emit_pusha();
     //save_regs(0x100f);
     emit_readword((int)&last_count,ECX);
     if(get_reg(regmap,CCREG)<0)
     emit_loadreg(CCREG,HOST_CCREG);
     emit_add(HOST_CCREG,ECX,HOST_CCREG);
     emit_addimm(HOST_CCREG,2*ccadj[i],HOST_CCREG);
     emit_writeword(HOST_CCREG,(int)&Count);
     emit_call((int)memdebug);
     emit_popa();
     //restore_regs(0x100f);
     }/**/
}

void storelr_assemble(int i,signed char regmap[])
{
    int s,th,tl;
    int temp;
    int offset;
    int jaddr,jaddr2;
    int case1,case2,case3;
    int done0,done1,done2;
    int memtarget,c=0;
    u_int hr,reglist=0;
    th=get_reg(regmap,rs2[i]|64);
    tl=get_reg(regmap,rs2[i]);
    s=get_reg(regmap,rs1[i]);
    temp=get_reg(regmap,-1);
    offset=imm[i];
    if(s>=0) {
        c=(isconst[i]>>s)&1;
        memtarget=((signed int)(constmap[i][s]+offset))<(signed int)0x80800000;
    }
    assert(tl>=0);
    assert(rs1[i]>0);
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && regmap[hr]>=0) reglist|=1<<hr;
    }
    if(tl>=0) {
        assert(temp>=0);
        if(s<0) emit_loadreg(rs1[i],temp);
        if(!c) {
            if(offset!=0) {
                if(s>=0) emit_addimm(s,offset,temp);
                else emit_addimm(temp,offset,temp);
                emit_cmpimm(temp,0x800000);
            }
            else
            {
                if(s>=0) {
                    emit_mov(s,temp);
                    emit_cmpimm(s,0x800000);
                }else{
                    emit_cmpimm(temp,0x800000);
                }
            }
            jaddr=(int)out;
            emit_jno(0);
        }
        else
        {
            emit_movimm(constmap[i][s]+offset,temp);
            jaddr=(int)out;
            if(!memtarget) {
                emit_jmp(0);
            }
        }
        
        emit_testimm(temp,2);
        emit_addimm_no_flags((u_int)0x80000000-0x80000000,temp);
        case2=(int)out;
        emit_jne(0);
        emit_testimm(temp,1);
        case1=(int)out;
        emit_jne(0);
        // 0
        if (opcode[i]==0x2A) { // SWL
            emit_writeword_indexed_rdram(tl,0,temp);
        }
        if (opcode[i]==0x2E) { // SWR
            emit_writebyte_indexed_rdram(tl,3,temp);
        }
        if (opcode[i]==0x2C||opcode[i]==0x2D) { // SDL/SDR
            assert(opcode[i]!=0x2C&&opcode[i]!=0x2D); //FIXME (SDL/SDR)
        }
        done0=(int)out;
        emit_jmp(0);
        // 1
        set_jump_target(case1,(int)out);
        if (opcode[i]==0x2A) { // SWL
            // Write 3 msb into three least significant bytes
            emit_rorimm(tl,8,tl);
            emit_writehword_indexed_rdram(tl,-1,temp);
            emit_rorimm(tl,16,tl);
            emit_writebyte_indexed_rdram(tl,1,temp);
            emit_rorimm(tl,8,tl);
        }
        if (opcode[i]==0x2E) { // SWR
            // Write two lsb into two most significant bytes
            emit_writehword_indexed_rdram(tl,1,temp);
        }
        done1=(int)out;
        emit_jmp(0);
        // 2
        set_jump_target(case2,(int)out);
        emit_testimm(temp,1);
        case3=(int)out;
        emit_jne(0);
        if (opcode[i]==0x2A) { // SWL
            // Write two msb into two least significant bytes
            emit_rorimm(tl,16,tl);
            emit_writehword_indexed_rdram(tl,-2,temp);
            emit_rorimm(tl,16,tl);
        }
        if (opcode[i]==0x2E) { // SWR
            // Write 3 lsb into three most significant bytes
            emit_writebyte_indexed_rdram(tl,-1,temp);
            emit_rorimm(tl,8,tl);
            emit_writehword_indexed_rdram(tl,0,temp);
            emit_rorimm(tl,24,tl);
        }
        done2=(int)out;
        emit_jmp(0);
        // 3
        set_jump_target(case3,(int)out);
        if (opcode[i]==0x2A) { // SWL
            // Write msb into least significant byte
            emit_rorimm(tl,24,tl);
            emit_writebyte_indexed_rdram(tl,-3,temp);
            emit_rorimm(tl,8,tl);
        }
        if (opcode[i]==0x2E) { // SWR
            // Write entire word
            emit_writeword_indexed_rdram(tl,-3,temp);
        }
    }
    assert(opcode[i]!=0x2C&&opcode[i]!=0x2D); // FIXME
    set_jump_target(done0,(int)out);
    set_jump_target(done1,(int)out);
    set_jump_target(done2,(int)out);
    emit_addimm_no_flags(0x80000000-(INT)0x80000000,temp);
#if defined(HOST_IMM8)
    int ir=get_reg(regmap,INVCP);
    assert(ir>=0);
    emit_cmpmem_indexed_rdramsr12_reg(ir,temp,1);
#else
    emit_cmpmem_indexed_rdramsr12_imm((int)invalid_code,temp,1);
#endif
    jaddr2=(int)out;
    emit_jne(0);
    if(!c||!memtarget)
        add_stub(STORELR_STUB,jaddr,(int)out,(int)regmap,rs2[i],ccadj[i],reglist);
    add_stub(INVCODE_STUB,jaddr2,(int)out,reglist|(1<<HOST_CCREG),temp,0,0);
    //  emit_pusha();
    //  emit_call((int)memdebug);
    //  emit_popa();
}

void c1ls_assemble(int i,signed char regmap[])
{
    int s,th,tl;
    int temp,ar;
    int offset;
    int jaddr,jaddr2,jaddr3,type;
    u_int hr,reglist=0;
    th=get_reg(regmap,FTEMP|64);
    tl=get_reg(regmap,FTEMP);
    s=get_reg(regmap,rs1[i]);
    temp=get_reg(regmap,-1);
    offset=imm[i];
    assert(tl>=0);
    assert(rs1[i]>0);
    assert(temp>=0);
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && regmap[hr]>=0) reglist|=1<<hr;
    }
    if(regmap[HOST_CCREG]==CCREG) reglist&=~(1<<HOST_CCREG);
    if (opcode[i]==0x31||opcode[i]==0x35) // LWC1/LDC1
    {
        // Loads use a temporary register which we need to save
        reglist|=1<<temp;
    }
    if (opcode[i]==0x39||opcode[i]==0x3D) // SWC1/SDC1
        ar=temp;
    else // LWC1/LDC1
        ar=tl;
    if(s<0) emit_loadreg(rs1[i],ar);
    // Check cop1 unusable
    signed char rs=get_reg(regmap,CSREG);
    assert(rs>=0);
    if(!cop1_usable) {
        emit_testimm(rs,0x20000000);
        jaddr=(int)out;
        emit_jeq(0);
        add_stub(FP_STUB,jaddr,(int)out,i,rs,is_delayslot,0);
        cop1_usable=1;
    }
    if (opcode[i]==0x39) { // SWC1 (get float address)
        emit_readword((int)&reg_cop1_simple[(source[i]>>16)&0x1f],tl);
    }
    if (opcode[i]==0x3D) { // SDC1 (get double address)
        emit_readword((int)&reg_cop1_double[(source[i]>>16)&0x1f],tl);
    }
    // Generate address + offset
    if(offset!=0) {
        if(s>=0) emit_addimm(s,offset,ar);
        else emit_addimm(ar,offset,ar);
        emit_cmpimm(ar,0x800000);
    }
    else
    {
        if(s>=0) {
            emit_mov(s,ar);
            emit_cmpimm(s,0x800000);
        }else{
            emit_cmpimm(ar,0x800000);
        }
    }
    if (opcode[i]==0x39) { // SWC1 (read float)
        emit_readword_indexed(0,tl,tl);
    }
    if (opcode[i]==0x3D) { // SDC1 (read double)
        emit_readword_indexed(4,tl,th);
        emit_readword_indexed(0,tl,tl);
    }
    if (opcode[i]==0x31) { // LWC1 (get target address)
        emit_readword((int)&reg_cop1_simple[(source[i]>>16)&0x1f],temp);
    }
    if (opcode[i]==0x35) { // LDC1 (get target address)
        emit_readword((int)&reg_cop1_double[(source[i]>>16)&0x1f],temp);
    }
    jaddr2=(int)out;
    emit_jno(0);
    if (opcode[i]==0x31) { // LWC1
        emit_readword_indexed_rdram((INT)0x80000000-0x80000000,tl,tl);
        type=LOADW_STUB;
    }
    if (opcode[i]==0x35) { // LDC1
        assert(th>=0);
        emit_readword_indexed_rdram((INT)0x80000000-0x80000000,tl,th);
        emit_readword_indexed_rdram((INT)0x80000000-0x7FFFFFFC,tl,tl);
        type=LOADD_STUB;
    }
    if (opcode[i]==0x39) { // SWC1
        emit_writeword_indexed_rdram(tl,(INT)0x80000000-0x80000000,temp);
        type=STOREW_STUB;
    }
    if (opcode[i]==0x3D) { // SDC1
        assert(th>=0);
        emit_writeword_indexed_rdram(th,(INT)0x80000000-0x80000000,temp);
        emit_writeword_indexed_rdram(tl,(INT)0x80000000-0x7FFFFFFC,temp);
        type=STORED_STUB;
    }
    if (opcode[i]==0x39||opcode[i]==0x3D) { // SWC1/SDC1
#if defined(HOST_IMM8)
        int ir=get_reg(regmap,INVCP);
        assert(ir>=0);
        emit_cmpmem_indexed_rdramsr12_reg(ir,temp,1);
#else
        emit_cmpmem_indexed_rdramsr12_imm((int)invalid_code,temp,1);
#endif
        jaddr3=(int)out;
        emit_jne(0);
        add_stub(INVCODE_STUB,jaddr3,(int)out,reglist|(1<<HOST_CCREG),temp,0,0);
    }
    add_stub(type,jaddr2,(int)out,(int)regmap,FTEMP,ccadj[i],reglist);
    if (opcode[i]==0x31) { // LWC1 (write float)
        emit_writeword_indexed(tl,0,temp);
    }
    if (opcode[i]==0x35) { // LDC1 (write double)
        emit_writeword_indexed(th,4,temp);
        emit_writeword_indexed(tl,0,temp);
    }
    //if(opcode[i]==0x39)
    /*if(opcode[i]==0x39||opcode[i]==0x31)
     {
     emit_pusha();
     emit_readword((int)&last_count,ECX);
     if(get_reg(regmap,CCREG)<0)
     emit_loadreg(CCREG,HOST_CCREG);
     emit_add(HOST_CCREG,ECX,HOST_CCREG);
     emit_addimm(HOST_CCREG,2*ccadj[i],HOST_CCREG);
     emit_writeword(HOST_CCREG,(int)&Count);
     emit_call((int)memdebug);
     emit_popa();
     }/**/
}

#ifndef multdiv_assemble
void multdiv_assemble(int i,signed char regmap[])
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
                assert(m1>=0);
                assert(m2>=0);
                emit_mov(m1,EAX);
                emit_imul(m2);
            }
            if(opcode2[i]==0x19) // MULTU
            {
                signed char m1=get_reg(regmap,rs1[i]);
                signed char m2=get_reg(regmap,rs2[i]);
                assert(m1>=0);
                assert(m2>=0);
                emit_mov(m1,EAX);
                emit_mul(m2);
            }
            if(opcode2[i]==0x1A) // DIV
            {
                signed char d1=get_reg(regmap,rs1[i]);
                signed char d2=get_reg(regmap,rs2[i]);
                assert(d1>=0);
                assert(d2>=0);
                emit_mov(d1,EAX);
                emit_cdq();
                emit_idiv(d2);
            }
            if(opcode2[i]==0x1B) // DIVU
            {
                signed char d1=get_reg(regmap,rs1[i]);
                signed char d2=get_reg(regmap,rs2[i]);
                assert(d1>=0);
                assert(d2>=0);
                emit_mov(d1,EAX);
                emit_zeroreg(EDX);
                emit_div(d2);
            }
        }
        else // 64-bit
        {
            if(opcode2[i]==0x1C) // DMULT
            {
                // FIXME
                assert(opcode2[i]!=0x1C);
            }
            if(opcode2[i]==0x1D) // DMULTU
            {
                signed char m1h=get_reg(regmap,rs1[i]|64);
                signed char m1l=get_reg(regmap,rs1[i]);
                signed char m2h=get_reg(regmap,rs2[i]|64);
                signed char m2l=get_reg(regmap,rs2[i]);
                signed char temp=get_reg(regmap,-1);
                assert(m1h>=0);
                assert(m2h>=0);
                assert(m1l>=0);
                assert(m2l>=0);
                assert(temp>=0);
                emit_mov(m1l,EAX);
                emit_mul(m2l);
                emit_storereg(LOREG,EAX);
                emit_mov(EDX,temp);
                emit_mov(m1h,EAX);
                emit_mul(m2l);
                emit_add(EAX,temp,temp);
                emit_adcimm(0,EDX);
                emit_storereg(HIREG,EDX);
                emit_mov(m2h,EAX);
                emit_mul(m1l);
                emit_add(EAX,temp,temp);
                emit_adcimm(0,EDX);
                emit_storereg(LOREG|64,temp);
                emit_mov(EDX,temp);
                emit_mov(m2h,EAX);
                emit_mul(m1h);
                emit_add(EAX,temp,EAX);
                emit_adcimm(0,EDX);
                emit_loadreg(HIREG,temp);
                emit_add(EAX,temp,EAX);
                emit_adcimm(0,EDX);
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
                emit_pushreg(d2h);
                emit_pushreg(d2l);
                emit_pushreg(d1h);
                emit_pushreg(d1l);
                emit_call((int)&div64);
                emit_popreg(d1l);
                emit_popreg(d1h);
                emit_popreg(d2l);
                emit_popreg(d2h);
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
                signed char d1h=get_reg(regmap,rs1[i]|64);
                signed char d1l=get_reg(regmap,rs1[i]);
                signed char d2h=get_reg(regmap,rs2[i]|64);
                signed char d2l=get_reg(regmap,rs2[i]);
                assert(d1h>=0);
                assert(d2h>=0);
                assert(d1l>=0);
                assert(d2l>=0);
                emit_pushreg(d2h);
                emit_pushreg(d2l);
                emit_pushreg(d1h);
                emit_pushreg(d1l);
                emit_call((int)&divu64);
                emit_popreg(d1l);
                emit_popreg(d1h);
                emit_popreg(d2l);
                emit_popreg(d2h);
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
#endif

void mov_assemble(int i,signed char regmap[])
{
    //if(opcode2[i]==0x10||opcode2[i]==0x12) { // MFHI/MFLO
    //if(opcode2[i]==0x11||opcode2[i]==0x13) { // MTHI/MTLO
    assert(rt1[i]>0);
    if(rt1[i]) {
        signed char sh,sl,th,tl;
        th=get_reg(regmap,rt1[i]|64);
        tl=get_reg(regmap,rt1[i]);
        //assert(tl>=0);
        if(tl>=0) {
            sh=get_reg(regmap,rs1[i]|64);
            sl=get_reg(regmap,rs1[i]);
            if(sl>=0) emit_mov(sl,tl);
            else emit_loadreg(rs1[i],tl);
            if(th>=0) {
                if(sh>=0) emit_mov(sh,th);
                else emit_loadreg(rs1[i]|64,th);
            }
        }
    }
}

void fconv_assemble(int i,signed char regmap[])
{
    //dst->f.cf.ft = (src >> 16) & 0x1F;
    //dst->f.cf.fs = (src >> 11) & 0x1F;
    //dst->f.cf.fd = (src >>  6) & 0x1F;
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
    
#ifdef IMM_WRITE
    emit_writeword_imm((int)&fake_pc_float,(int)&PC);
    //emit_writebyte_imm((source[i]>>16)&0x1f,(int)&(fake_pc_float.f.cf.ft));
    emit_writebyte_imm((source[i]>>11)&0x1f,(int)&(fake_pc_float.f.cf.fs));
    emit_writebyte_imm((source[i]>> 6)&0x1f,(int)&(fake_pc_float.f.cf.fd));
#else
    emit_addimm(FP,(int)&fake_pc_float-(int)&dynarec_local,ARG1_REG);
    //emit_movimm((source[i]>>16)&0x1f,ARG2_REG);
    emit_movimm((source[i]>>11)&0x1f,ARG3_REG);
    emit_movimm((source[i]>> 6)&0x1f,ARG4_REG);
    emit_writeword(ARG1_REG,(int)&PC);
    //emit_writebyte(ARG2_REG,(int)&(fake_pc_float.f.cf.ft));
    emit_writebyte(ARG3_REG,(int)&(fake_pc_float.f.cf.fs));
    emit_writebyte(ARG4_REG,(int)&(fake_pc_float.f.cf.fd));
#endif
    
    if(opcode2[i]==0x14&&(source[i]&0x3f)==0x20) emit_call((int)CVT_S_W);
    if(opcode2[i]==0x14&&(source[i]&0x3f)==0x21) emit_call((int)CVT_D_W);
    if(opcode2[i]==0x15&&(source[i]&0x3f)==0x20) emit_call((int)CVT_S_L);
    if(opcode2[i]==0x15&&(source[i]&0x3f)==0x21) emit_call((int)CVT_D_L);
    emit_loadreg(CSREG,rs);
}

void float_assemble(int i,signed char regmap[])
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
    
#ifdef IMM_WRITE
    emit_writeword_imm((int)&fake_pc_float,(int)&PC);
    emit_writebyte_imm((source[i]>>16)&0x1f,(int)&(fake_pc_float.f.cf.ft));
    emit_writebyte_imm((source[i]>>11)&0x1f,(int)&(fake_pc_float.f.cf.fs));
    emit_writebyte_imm((source[i]>> 6)&0x1f,(int)&(fake_pc_float.f.cf.fd));
#else
    emit_addimm(FP,(int)&fake_pc_float-(int)&dynarec_local,ARG1_REG);
    emit_movimm((source[i]>>16)&0x1f,ARG2_REG);
    emit_movimm((source[i]>>11)&0x1f,ARG3_REG);
    emit_movimm((source[i]>> 6)&0x1f,ARG4_REG);
    emit_writeword(ARG1_REG,(int)&PC);
    emit_writebyte(ARG2_REG,(int)&(fake_pc_float.f.cf.ft));
    emit_writebyte(ARG3_REG,(int)&(fake_pc_float.f.cf.fs));
    emit_writebyte(ARG4_REG,(int)&(fake_pc_float.f.cf.fd));
#endif
    
    if(opcode2[i]==0x10) { // Single precision
        switch(source[i]&0x3f)
        {
            case 0x00: emit_call((int)ADD_S);break;
            case 0x01: emit_call((int)SUB_S);break;
            case 0x02: emit_call((int)MUL_S);break;
            case 0x03: emit_call((int)DIV_S);break;
            case 0x04: emit_call((int)SQRT_S);break;
            case 0x05: emit_call((int)ABS_S);break;
            case 0x06: emit_call((int)MOV_S);break;
            case 0x07: emit_call((int)NEG_S);break;
            case 0x08: emit_call((int)ROUND_L_S);break;
            case 0x09: emit_call((int)TRUNC_L_S);break;
            case 0x0A: emit_call((int)CEIL_L_S);break;
            case 0x0B: emit_call((int)FLOOR_L_S);break;
            case 0x0C: emit_call((int)ROUND_W_S);break;
            case 0x0D: emit_call((int)TRUNC_W_S);break;
            case 0x0E: emit_call((int)CEIL_W_S);break;
            case 0x0F: emit_call((int)FLOOR_W_S);break;
            case 0x21: emit_call((int)CVT_D_S);break;
            case 0x24: emit_call((int)CVT_W_S);break;
            case 0x25: emit_call((int)CVT_L_S);break;
            case 0x30: emit_call((int)C_F_S);break;
            case 0x31: emit_call((int)C_UN_S);break;
            case 0x32: emit_call((int)C_EQ_S);break;
            case 0x33: emit_call((int)C_UEQ_S);break;
            case 0x34: emit_call((int)C_OLT_S);break;
            case 0x35: emit_call((int)C_ULT_S);break;
            case 0x36: emit_call((int)C_OLE_S);break;
            case 0x37: emit_call((int)C_ULE_S);break;
            case 0x38: emit_call((int)C_SF_S);break;
            case 0x39: emit_call((int)C_NGLE_S);break;
            case 0x3A: emit_call((int)C_SEQ_S);break;
            case 0x3B: emit_call((int)C_NGL_S);break;
            case 0x3C: emit_call((int)C_LT_S);break;
            case 0x3D: emit_call((int)C_NGE_S);break;
            case 0x3E: emit_call((int)C_LE_S);break;
            case 0x3F: emit_call((int)C_NGT_S);break;
        }
    }
    if(opcode2[i]==0x11) { // Double precision
        switch(source[i]&0x3f)
        {
            case 0x00: emit_call((int)ADD_D);break;
            case 0x01: emit_call((int)SUB_D);break;
            case 0x02: emit_call((int)MUL_D);break;
            case 0x03: emit_call((int)DIV_D);break;
            case 0x04: emit_call((int)SQRT_D);break;
            case 0x05: emit_call((int)ABS_D);break;
            case 0x06: emit_call((int)MOV_D);break;
            case 0x07: emit_call((int)NEG_D);break;
            case 0x08: emit_call((int)ROUND_L_D);break;
            case 0x09: emit_call((int)TRUNC_L_D);break;
            case 0x0A: emit_call((int)CEIL_L_D);break;
            case 0x0B: emit_call((int)FLOOR_L_D);break;
            case 0x0C: emit_call((int)ROUND_W_D);break;
            case 0x0D: emit_call((int)TRUNC_W_D);break;
            case 0x0E: emit_call((int)CEIL_W_D);break;
            case 0x0F: emit_call((int)FLOOR_W_D);break;
            case 0x20: emit_call((int)CVT_S_D);break;
            case 0x24: emit_call((int)CVT_W_D);break;
            case 0x25: emit_call((int)CVT_L_D);break;
            case 0x30: emit_call((int)C_F_D);break;
            case 0x31: emit_call((int)C_UN_D);break;
            case 0x32: emit_call((int)C_EQ_D);break;
            case 0x33: emit_call((int)C_UEQ_D);break;
            case 0x34: emit_call((int)C_OLT_D);break;
            case 0x35: emit_call((int)C_ULT_D);break;
            case 0x36: emit_call((int)C_OLE_D);break;
            case 0x37: emit_call((int)C_ULE_D);break;
            case 0x38: emit_call((int)C_SF_D);break;
            case 0x39: emit_call((int)C_NGLE_D);break;
            case 0x3A: emit_call((int)C_SEQ_D);break;
            case 0x3B: emit_call((int)C_NGL_D);break;
            case 0x3C: emit_call((int)C_LT_D);break;
            case 0x3D: emit_call((int)C_NGE_D);break;
            case 0x3E: emit_call((int)C_LE_D);break;
            case 0x3F: emit_call((int)C_NGT_D);break;
        }
    }
    emit_loadreg(CSREG,rs);
}

/*void syscall_assemble(int i,signed char regmap[])
 {
 signed char ccreg=get_reg(regmap,CCREG);
 assert(ccreg==HOST_CCREG);
 assert(!is_delayslot);
 emit_movimm(start+i*4,EAX); // Get PC
 emit_addimm(HOST_CCREG,2*ccadj[i],HOST_CCREG); // CHECK: is this right?  There should probably be an extra cycle...
 emit_jmp((int)syscall);
 }*/

void ds_assemble(int i,signed char regmap[])
{
    is_delayslot=1;
    switch(itype[i]) {
        case ALU:
            alu_assemble(i,regmap);break;
        case IMM16:
            imm16_assemble(i,regmap);break;
        case SHIFT:
            shift_assemble(i,regmap);break;
        case SHIFTIMM:
            shiftimm_assemble(i,regmap);break;
        case LOAD:
            load_assemble(i,regmap);break;
        case LOADLR:
            loadlr_assemble(i,regmap);break;
        case STORE:
            store_assemble(i,regmap);break;
        case STORELR:
            storelr_assemble(i,regmap);break;
        case COP0:
            cop0_assemble(i,regmap);break;
        case COP1:
            cop1_assemble(i,regmap);break;
        case C1LS:
            c1ls_assemble(i,regmap);break;
        case FCONV:
            fconv_assemble(i,regmap);break;
        case FLOAT:
            float_assemble(i,regmap);break;
        case MULTDIV:
            multdiv_assemble(i,regmap);break;
        case MOV:
            mov_assemble(i,regmap);break;
        case SYSCALL:
        case UJUMP:
        case RJUMP:
        case CJUMP:
        case SJUMP:
        case FJUMP:
            printf("Jump in the delay slot.  This is probably a bug.\n");
    }
    is_delayslot=0;
}

// Is the branch target a valid internal jump?
int internal_branch(uint64_t i_is32,int addr)
{
    if(addr>=start && addr<(start+slen*4))
    {
        int t=(addr-start)>>2;
        // Delay slots are not valid branch targets
        if(t>0&&(itype[t-1]==RJUMP||itype[t-1]==UJUMP||itype[t-1]==CJUMP||itype[t-1]==SJUMP||itype[t-1]==FJUMP)) return 0;
        // 64 -> 32 bit transition requires a recompile
        /*if(is32[t]&~unneeded_reg_upper[t]&~i_is32)
         {
         if(requires_32bit[t]&~i_is32) printf("optimizable: no\n");
         else printf("optimizable: yes\n");
         }*/
        //if(is32[t]&~unneeded_reg_upper[t]&~i_is32) return 0;
        if(requires_32bit[t]&~i_is32) return 0;
        else return 1;
    }
    return 0;
}

#ifndef wb_invalidate
void wb_invalidate(signed char pre[],signed char entry[],uint64_t dirty,uint64_t is32,
                   uint64_t u,uint64_t uu)
{
    int hr;
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && hr!=EXCLUDE_REG) {
            if(pre[hr]!=entry[hr]) {
                if(pre[hr]>=0) {
                    if((dirty>>hr)&1) {
                        if(get_reg(entry,pre[hr])<0) {
                            if(pre[hr]<64) {
                                if(!((u>>pre[hr])&1)) {
                                    emit_storereg(pre[hr],hr);
                                    if( ((is32>>pre[hr])&1) && !((uu>>pre[hr])&1) ) {
                                        emit_sarimm(hr,31,hr);
                                        emit_storereg(pre[hr]|64,hr);
                                    }
                                }
                            }else{
                                if(!((uu>>(pre[hr]&63))&1) && !((is32>>(pre[hr]&63))&1)) {
                                    emit_storereg(pre[hr],hr);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && hr!=EXCLUDE_REG) {
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
#endif

// Load the specified registers
// This only loads the registers given as arguments because
// we don't want to load things that will be overwritten
void load_regs(signed char entry[],signed char regmap[],int is32,int rs1,int rs2)
{
    int hr;
    // Load 32-bit regs
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && hr!=EXCLUDE_REG&&regmap[hr]>=0) {
            if(entry[hr]!=regmap[hr]) {
                if(regmap[hr]==rs1||regmap[hr]==rs2)
                {
                    if(regmap[hr]==0) {
                        emit_zeroreg(hr);
                    }
                    else
                    {
                        emit_loadreg(regmap[hr],hr);
                    }
                }
            }
        }
    }
    //Load 64-bit regs
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && hr!=EXCLUDE_REG&&regmap[hr]>=0) {
            if(entry[hr]!=regmap[hr]) {
                if(regmap[hr]-64==rs1||regmap[hr]-64==rs2)
                {
                    assert(regmap[hr]!=64);
                    if((is32>>(regmap[hr]&63))&1) {
                        int lr=get_reg(regmap,regmap[hr]-64);
                        if(lr>=0)
                            emit_sarimm(lr,31,hr);
                        else
                            emit_loadreg(regmap[hr],hr);
                    }
                    else
                    {
                        emit_loadreg(regmap[hr],hr);
                    }
                }
            }
        }
    }
}

int get_final_value(int hr, int i, int *value)
{
    int reg=regmap[i][hr];
    while(i<slen-1) {
        if(regmap[i+1][hr]!=reg) break;
        if(!((isconst[i+1]>>hr)&1)) break;
        if(bt[i+1]) break;
        i++;
    }
    if(i<slen-1&&itype[i+1]==LOAD&&rs1[i+1]==reg&&rt1[i+1]==reg&&!bt[i+1])
    {
        // Precompute load address
        *value=constmap[i][hr]+imm[i+1];
        //printf("c=%x imm=%x\n",(int)constmap[i][hr],imm[i+1]);
        return 1;
    }
    *value=constmap[i][hr];
    //printf("c=%x\n",(int)constmap[i][hr]);
    if(i==slen-1) return 0;
    if(reg<64) {
        return !((unneeded_reg[i+1]>>reg)&1);
    }else{
        return !((unneeded_reg_upper[i+1]>>reg)&1);
    }
}

// Load registers with known constants
void load_consts(signed char pre[],signed char regmap[],int is32,int i)
{
    int hr;
    // Load 32-bit regs
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && hr!=EXCLUDE_REG&&regmap[hr]>=0) {
            //if(entry[hr]!=regmap[hr]) {
            if(i==0||!((isconst[i-1]>>hr)&1)||pre[hr]!=regmap[hr]||bt[i]) {
                if(((isconst[i]>>hr)&1)&&regmap[hr]<64&&regmap[hr]>0) {
                    int value;
                    if(get_final_value(hr,i,&value)) {
                        if(value==0) {
                            emit_zeroreg(hr);
                        }
                        else {
                            emit_movimm(value,hr);
                        }
                    }
                }
            }
        }
    }
    //Load 64-bit regs
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && hr!=EXCLUDE_REG&&regmap[hr]>=0) {
            //if(entry[hr]!=regmap[hr]) {
            if(i==0||!((isconst[i-1]>>hr)&1)||pre[hr]!=regmap[hr]||bt[i]) {
                if(((isconst[i]>>hr)&1)&&regmap[hr]>64) {
                    if((is32>>(regmap[hr]&63))&1) {
                        int lr=get_reg(regmap,regmap[hr]-64);
                        assert(lr>=0);
                        emit_sarimm(lr,31,hr);
                    }
                    else
                    {
                        int value;
                        if(get_final_value(hr,i,&value)) {
                            if(value==0) {
                                emit_zeroreg(hr);
                            }
                            else {
                                emit_movimm(value,hr);
                            }
                        }
                    }
                }
            }
        }
    }
}

// Write out all dirty registers (except cycle count)
void wb_dirtys(signed char i_regmap[],uint64_t i_is32,uint64_t i_dirty)
{
    int hr;
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && hr!=EXCLUDE_REG) {
            if(i_regmap[hr]>0) {
                if(i_regmap[hr]!=CCREG) {
                    if((i_dirty>>hr)&1) {
                        if(i_regmap[hr]<64) {
                            emit_storereg(i_regmap[hr],hr);
                            if( ((i_is32>>i_regmap[hr])&1) ) {
#ifdef DESTRUCTIVE_WRITEBACK
                                emit_sarimm(hr,31,hr);
                                emit_storereg(i_regmap[hr]|64,hr);
#else
                                emit_sarimm(hr,31,HOST_TEMPREG);
                                emit_storereg(i_regmap[hr]|64,HOST_TEMPREG);
#endif
                            }
                        }else{
                            if( !((i_is32>>(i_regmap[hr]&63))&1) ) {
                                emit_storereg(i_regmap[hr],hr);
                            }
                        }
                    }
                }
            }
        }
    }
}
// Write out dirty registers that we need to reload (pair with load_needed_regs)
// This writes the registers not written by store_regs_bt
void wb_needed_dirtys(signed char i_regmap[],uint64_t i_is32,uint64_t i_dirty,int addr)
{
    int hr;
    int t=(addr-start)>>2;
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && hr!=EXCLUDE_REG) {
            if(i_regmap[hr]>0) {
                if(i_regmap[hr]!=CCREG) {
                    if(i_regmap[hr]==regmap_entry[t][hr] && ((dirty[t]>>hr)&1) && !(((i_is32&~is32[t]&~unneeded_reg_upper[t])>>(i_regmap[hr]&63))&1)) {
                        if((i_dirty>>hr)&1) {
                            if(i_regmap[hr]<64) {
                                emit_storereg(i_regmap[hr],hr);
                                if( ((i_is32>>i_regmap[hr])&1) ) {
#ifdef DESTRUCTIVE_WRITEBACK
                                    emit_sarimm(hr,31,hr);
                                    emit_storereg(i_regmap[hr]|64,hr);
#else
                                    emit_sarimm(hr,31,HOST_TEMPREG);
                                    emit_storereg(i_regmap[hr]|64,HOST_TEMPREG);
#endif
                                }
                            }else{
                                if( !((i_is32>>(i_regmap[hr]&63))&1) ) {
                                    emit_storereg(i_regmap[hr],hr);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// Load all registers (except cycle count)
void load_all_regs(signed char i_regmap[])
{
    int hr;
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && hr!=EXCLUDE_REG) {
            if(i_regmap[hr]==0) {
                emit_zeroreg(hr);
            }
            else
                if(i_regmap[hr]>0 && i_regmap[hr]!=CCREG)
                {
                    emit_loadreg(i_regmap[hr],hr);
                }
        }
    }
}

// Load all current registers also needed by next instruction
void load_needed_regs(signed char i_regmap[],signed char next_regmap[])
{
    int hr;
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && hr!=EXCLUDE_REG) {
            if(get_reg(next_regmap,i_regmap[hr])>=0) {
                if(i_regmap[hr]==0) {
                    emit_zeroreg(hr);
                }
                else
                    if(i_regmap[hr]>0 && i_regmap[hr]!=CCREG)
                    {
                        emit_loadreg(i_regmap[hr],hr);
                    }
            }
        }
    }
}

// Load all regs, storing cycle count if necessary
void load_regs_entry(int t)
{
    int hr;
    if(ccadj[t]) emit_addimm(HOST_CCREG,-ccadj[t]*2,HOST_CCREG);
    if(regmap_entry[t][HOST_CCREG]!=CCREG) {
        emit_storereg(CCREG,HOST_CCREG);
    }
    // Load 32-bit regs
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && regmap_entry[t][hr]>=0&&regmap_entry[t][hr]<64) {
            if(regmap_entry[t][hr]==0) {
                emit_zeroreg(hr);
            }
            else if(regmap_entry[t][hr]!=CCREG)
            {
                emit_loadreg(regmap_entry[t][hr],hr);
            }
        }
    }
    //Load 64-bit regs
    for(hr=0;hr<HOST_REGS;hr++) {
        if(hr!=9 && regmap_entry[t][hr]>=64) {
            assert(regmap_entry[t][hr]!=64);
            if((is32[t]>>(regmap_entry[t][hr]&63))&1) {
                int lr=get_reg(regmap_entry[t],regmap_entry[t][hr]-64);
                if(lr<0) {
                    emit_loadreg(regmap_entry[t][hr],hr);
                }
                else
                {
                    emit_sarimm(lr,31,hr);
                }
            }
            else
            {
                emit_loadreg(regmap_entry[t][hr],hr);
            }
        }
    }
}

// Store dirty registers prior to branch
void store_regs_bt(signed char i_regmap[],uint64_t i_is32,uint64_t i_dirty,int addr)
{
    if(internal_branch(i_is32,addr))
    {
        int t=(addr-start)>>2;
        int hr;
        for(hr=0;hr<HOST_REGS;hr++) {
            if(hr!=9 && hr!=EXCLUDE_REG) {
                if(i_regmap[hr]>0 && i_regmap[hr]!=CCREG) {
                    if(i_regmap[hr]!=regmap_entry[t][hr] || !((dirty[t]>>hr)&1) || (((i_is32&~is32[t]&~unneeded_reg_upper[t])>>(i_regmap[hr]&63))&1)) {
                        if((i_dirty>>hr)&1) {
                            if(i_regmap[hr]<64) {
                                if(!((unneeded_reg[t]>>i_regmap[hr])&1)) {
                                    emit_storereg(i_regmap[hr],hr);
                                    if( ((i_is32>>i_regmap[hr])&1) && !((unneeded_reg_upper[t]>>i_regmap[hr])&1) ) {
#ifdef DESTRUCTIVE_WRITEBACK
                                        emit_sarimm(hr,31,hr);
                                        emit_storereg(i_regmap[hr]|64,hr);
#else
                                        emit_sarimm(hr,31,HOST_TEMPREG);
                                        emit_storereg(i_regmap[hr]|64,HOST_TEMPREG);
#endif
                                    }
                                }
                            }else{
                                if( !((i_is32>>(i_regmap[hr]&63))&1) && !((unneeded_reg_upper[t]>>(i_regmap[hr]&63))&1) ) {
                                    emit_storereg(i_regmap[hr],hr);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        // Branch out of this block, write out all dirty regs
        wb_dirtys(i_regmap,i_is32,i_dirty);
    }
}

// Load all needed registers for branch target
void load_regs_bt(signed char i_regmap[],uint64_t i_is32,uint64_t i_dirty,int addr)
{
    //if(addr>=start && addr<(start+slen*4))
    if(internal_branch(i_is32,addr))
    {
        int t=(addr-start)>>2;
        int hr;
        // Store the cycle count before loading something else
        if(i_regmap[HOST_CCREG]!=CCREG) {
            assert(i_regmap[HOST_CCREG]==-1);
        }
        if(regmap_entry[t][HOST_CCREG]!=CCREG) {
            emit_storereg(CCREG,HOST_CCREG);
        }
        // Load 32-bit regs
        for(hr=0;hr<HOST_REGS;hr++) {
            if(hr!=9 && hr!=EXCLUDE_REG&&regmap_entry[t][hr]>=0&&regmap_entry[t][hr]<64) {
                if(i_regmap[hr]!=regmap_entry[t][hr] || ( !((dirty[t]>>hr)&1) && ((i_dirty>>hr)&1) && (((i_is32&~unneeded_reg_upper[t])>>i_regmap[hr])&1) ) || (((i_is32&~is32[t]&~unneeded_reg_upper[t])>>(i_regmap[hr]&63))&1)) {
                    if(regmap_entry[t][hr]==0) {
                        emit_zeroreg(hr);
                    }
                    else if(regmap_entry[t][hr]!=CCREG)
                    {
                        emit_loadreg(regmap_entry[t][hr],hr);
                    }
                }
            }
        }
        //Load 64-bit regs
        for(hr=0;hr<HOST_REGS;hr++) {
            if(hr!=9 && hr!=EXCLUDE_REG&&regmap_entry[t][hr]>=64) {
                if(i_regmap[hr]!=regmap_entry[t][hr]) {
                    assert(regmap_entry[t][hr]!=64);
                    if((i_is32>>(regmap_entry[t][hr]&63))&1) {
                        int lr=get_reg(regmap_entry[t],regmap_entry[t][hr]-64);
                        if(lr<0) {
                            emit_loadreg(regmap_entry[t][hr],hr);
                        }
                        else
                        {
                            emit_sarimm(lr,31,hr);
                        }
                    }
                    else
                    {
                        emit_loadreg(regmap_entry[t][hr],hr);
                    }
                }
                else if((i_is32>>(regmap_entry[t][hr]&63))&1) {
                    int lr=get_reg(regmap_entry[t],regmap_entry[t][hr]-64);
                    assert(lr>=0);
                    emit_sarimm(lr,31,hr);
                }
            }
        }
    }
}

int match_bt(signed char i_regmap[],uint64_t i_is32,uint64_t i_dirty,int addr)
{
    if(addr>=start && addr<(start+slen*4))
    {
        int t=(addr-start)>>2;
        int hr;
        if(regmap_entry[t][HOST_CCREG]!=CCREG) return 0;
        for(hr=0;hr<HOST_REGS;hr++)
        {
            if(hr!=9 && hr!=EXCLUDE_REG)
            {
                if(i_regmap[hr]!=regmap_entry[t][hr])
                {
                    if(regmap_entry[t][hr]!=-1)
                    {
                        return 0;
                    }
                    else
                        if((i_dirty>>hr)&1)
                        {
                            if(i_regmap[hr]<64)
                            {
                                if(!((unneeded_reg[t]>>i_regmap[hr])&1))
                                    return 0;
                            }
                            else
                            {
                                if(!((unneeded_reg_upper[t]>>(i_regmap[hr]&63))&1))
                                    return 0;
                            }
                        }
                }
                else // Same register but is it 32-bit or dirty?
                    if(i_regmap[hr]>=0)
                    {
                        if(!((dirty[t]>>hr)&1))
                        {
                            if((i_dirty>>hr)&1)
                            {
                                if(!((unneeded_reg[t]>>i_regmap[hr])&1))
                                {
                                    //printf("%x: dirty no match\n",addr);
                                    return 0;
                                }
                            }
                        }
                        if((((is32[t]^i_is32)&~unneeded_reg_upper[t])>>(i_regmap[hr]&63))&1)
                        {
                            //printf("%x: is32 no match\n",addr);
                            return 0;
                        }
                    }
            }
        }
        //if(is32[t]&~unneeded_reg_upper[t]&~i_is32) return 0;
        if(requires_32bit[t]&~i_is32) return 0;
        // Delay slots are not valid branch targets
        if(t>0&&(itype[t-1]==RJUMP||itype[t-1]==UJUMP||itype[t-1]==CJUMP||itype[t-1]==SJUMP||itype[t-1]==FJUMP)) return 0;
    }
    else
    {
        int hr;
        for(hr=0;hr<HOST_REGS;hr++)
        {
            if(hr!=9 && hr!=EXCLUDE_REG)
            {
                if(i_regmap[hr]>=0)
                {
                    if(hr!=HOST_CCREG||i_regmap[hr]!=CCREG)
                    {
                        if((i_dirty>>hr)&1)
                        {
                            return 0;
                        }
                    }
                }
            }
        }
    }
    return 1;
}

//FIXME: branch into delay slot is not valid
//(or at least adds an extra cycle)
void do_cc(int i,signed char i_regmap[],int *adj,int addr,int taken)
{
    int count;
    int jaddr;
    int idle=0;
    //if(ba[i]>=start && ba[i]<(start+slen*4))
    if(itype[i]==RJUMP)
    {
        *adj=0;
    }
    if(internal_branch(branch_regs[i].is32,ba[i]))
    {
        int t=(ba[i]-start)>>2;
        *adj=ccadj[t];
    }
    else
    {
        *adj=0;
    }
    count=ccadj[i];
    if(taken==TAKEN && i==(ba[i]-start)>>2 && source[i+1]==0) {
        // Idle loop
        if(count&1) emit_addimm_and_set_flags(2*(count+2),HOST_CCREG);
        idle=(int)out;
        emit_andimm(HOST_CCREG,3,HOST_CCREG);
        jaddr=(int)out;
        emit_jmp(0);
    }
    else if(*adj==0) {
        emit_addimm_and_set_flags(2*(count+2),HOST_CCREG);
        jaddr=(int)out;
        emit_jns(0);
    }
    else
    {
        emit_cmpimm(HOST_CCREG,-2*(count+2));
        jaddr=(int)out;
        emit_jns(0);
    }
    add_stub(CC_STUB,jaddr,idle?idle:(int)out,(*adj==0||idle)?0:(count+2),i,addr,taken);
}

void do_ccstub(int n)
{
    literal_pool(256);
    assem_debug("do_ccstub %x\n",start+stubs[n][4]*4);
    set_jump_target(stubs[n][1],(int)out);
    int i=stubs[n][4];
    if(stubs[n][6]==NULLDS) {
        // Delay slot instruction is nullified ("likely" branch)
        wb_dirtys(regmap[i],is32_post[i],dirty_post[i]);
    }
    else if(stubs[n][6]!=TAKEN) {
        wb_dirtys(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty);
    }
    else {
        if(internal_branch(branch_regs[i].is32,ba[i]))
            wb_needed_dirtys(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
    }
    if(stubs[n][5]!=-1)
    {
        // Save PC as return address
        emit_movimm(stubs[n][5],EAX);
        emit_writeword(EAX,(int)&pcaddr);
    }
    else
    {
        // Return address depends on which way the branch goes
        if(itype[i]==CJUMP||itype[i]==SJUMP||itype[i]==FJUMP)
        {
            int s1l=get_reg(branch_regs[i].regmap,rs1[i]);
            int s1h=get_reg(branch_regs[i].regmap,rs1[i]|64);
            int s2l=get_reg(branch_regs[i].regmap,rs2[i]);
            int s2h=get_reg(branch_regs[i].regmap,rs2[i]|64);
            if(rs1[i]==0)
            {
                s1l=s2l;s1h=s2h;
                s2l=s2h=-1;
            }
            else if(rs2[i]==0)
            {
                s2l=s2h=-1;
            }
            if((branch_regs[i].is32>>rs1[i])&(branch_regs[i].is32>>rs2[i])&1) {
                s1h=s2h=-1;
            }
            assert(s1l>=0);
#ifdef DESTRUCTIVE_WRITEBACK
            if((branch_regs[i].dirty>>s1l)&(branch_regs[i].is32>>rs1[i])&1)
                emit_loadreg(rs1[i],s1l);
            if(s2l>=0)
                if((branch_regs[i].dirty>>s2l)&(branch_regs[i].is32>>rs2[i])&1)
                    emit_loadreg(rs2[i],s2l);
#endif
            int hr=0;
            int addr,alt,ntaddr;
            while(hr<HOST_REGS)
            {
                if(hr != 9 && hr!=EXCLUDE_REG && hr!=HOST_CCREG &&
                   (branch_regs[i].regmap[hr]&63)!=rs1[i] &&
                   (branch_regs[i].regmap[hr]&63)!=rs2[i] )
                {
                    addr=hr++;break;
                }
                hr++;
            }
            while(hr<HOST_REGS)
            {
                if(hr != 9 && hr!=EXCLUDE_REG && hr!=HOST_CCREG &&
                   (branch_regs[i].regmap[hr]&63)!=rs1[i] &&
                   (branch_regs[i].regmap[hr]&63)!=rs2[i] )
                {
                    alt=hr++;break;
                }
                hr++;
            }
            if((opcode[i]&0x2E)==6) // BLEZ/BGTZ needs another register
            {
                while(hr<HOST_REGS)
                {
                    if(hr != 9 && hr!=EXCLUDE_REG && hr!=HOST_CCREG &&
                       (branch_regs[i].regmap[hr]&63)!=rs1[i] &&
                       (branch_regs[i].regmap[hr]&63)!=rs2[i] )
                    {
                        ntaddr=hr;break;
                    }
                    hr++;
                }
                assert(hr<HOST_REGS);
            }
            if((opcode[i]&0x2f)==4) // BEQ
            {
                emit_movimm(ba[i],addr);
                emit_movimm(start+i*4+8,alt);
                if(s1h>=0) {
                    if(s2h>=0) emit_cmp(s1h,s2h);
                    else emit_test(s1h,s1h);
                    emit_cmovne_reg(alt,addr);
                }
                if(s2l>=0) emit_cmp(s1l,s2l);
                else emit_test(s1l,s1l);
                emit_cmovne_reg(alt,addr);
            }
            if((opcode[i]&0x2f)==5) // BNE
            {
                emit_movimm(start+i*4+8,addr);
                emit_movimm(ba[i],alt);
                if(s1h>=0) {
                    if(s2h>=0) emit_cmp(s1h,s2h);
                    else emit_test(s1h,s1h);
                    emit_cmovne_reg(alt,addr);
                }
                if(s2l>=0) emit_cmp(s1l,s2l);
                else emit_test(s1l,s1l);
                emit_cmovne_reg(alt,addr);
            }
            if((opcode[i]&0x2f)==6) // BLEZ
            {
                emit_movimm(ba[i],alt);
                emit_movimm(start+i*4+8,addr);
                emit_cmpimm(s1l,1);
                if(s1h>=0) emit_mov(addr,ntaddr);
                emit_cmovl_reg(alt,addr);
                if(s1h>=0) {
                    emit_test(s1h,s1h);
                    emit_cmovne_reg(ntaddr,addr);
                    emit_cmovs_reg(alt,addr);
                }
            }
            if((opcode[i]&0x2f)==7) // BGTZ
            {
                emit_movimm(ba[i],addr);
                emit_movimm(start+i*4+8,ntaddr);
                emit_cmpimm(s1l,1);
                if(s1h>=0) emit_mov(addr,alt);
                emit_cmovl_reg(ntaddr,addr);
                if(s1h>=0) {
                    emit_test(s1h,s1h);
                    emit_cmovne_reg(alt,addr);
                    emit_cmovs_reg(ntaddr,addr);
                }
            }
            if((opcode[i]==1)&&(opcode2[i]&0x2D)==0) // BLTZ
            {
                emit_movimm(ba[i],alt);
                emit_movimm(start+i*4+8,addr);
                if(s1h>=0) emit_test(s1h,s1h);
                else emit_test(s1l,s1l);
                emit_cmovs_reg(alt,addr);
            }
            if((opcode[i]==1)&&(opcode2[i]&0x2D)==1) // BGEZ
            {
                emit_movimm(ba[i],addr);
                emit_movimm(start+i*4+8,alt);
                if(s1h>=0) emit_test(s1h,s1h);
                else emit_test(s1l,s1l);
                emit_cmovs_reg(alt,addr);
            }
            if(opcode[i]==0x11 && opcode2[i]==0x08 ) {
                if(source[i]&0x10000) // BC1T
                {
                    emit_movimm(ba[i],alt);
                    emit_movimm(start+i*4+8,addr);
                    emit_testimm(s1l,0x800000);
                    emit_cmovne_reg(alt,addr);
                }
                else // BC1F
                {
                    emit_movimm(ba[i],addr);
                    emit_movimm(start+i*4+8,alt);
                    emit_testimm(s1l,0x800000);
                    emit_cmovne_reg(alt,addr);
                }
            }
            emit_writeword(addr,(int)&pcaddr);
        }
        else
            if(itype[i]==RJUMP)
            {
                int r=get_reg(branch_regs[i].regmap,rs1[i]);
                emit_writeword(r,(int)&pcaddr);
            }
            else {printf("Unknown branch type in do_ccstub\n");exit(1);}
    }
    // Update cycle count
    assert(branch_regs[i].regmap[HOST_CCREG]==CCREG||branch_regs[i].regmap[HOST_CCREG]==-1);
    if(stubs[n][3]) emit_addimm(HOST_CCREG,2*stubs[n][3],HOST_CCREG);
    emit_call((int)cc_interrupt);
    if(stubs[n][3]) emit_addimm(HOST_CCREG,-2*stubs[n][3],HOST_CCREG);
    if(stubs[n][6]==TAKEN) {
        if(internal_branch(branch_regs[i].is32,ba[i]))
            load_needed_regs(branch_regs[i].regmap,regmap_entry[(ba[i]-start)>>2]);
        else if(itype[i]==RJUMP)
            emit_loadreg(rs1[i],get_reg(branch_regs[i].regmap,rs1[i]));
    }else if(stubs[n][6]==NOTTAKEN) {
        if(i<slen-2) load_needed_regs(branch_regs[i].regmap,regmap_pre[i+2]);
        else load_all_regs(branch_regs[i].regmap);
    }else if(stubs[n][6]==NULLDS) {
        // Delay slot instruction is nullified ("likely" branch)
        if(i<slen-2) load_needed_regs(regmap[i],regmap_pre[i+2]);
        else load_all_regs(regmap[i]);
    }else{
        load_all_regs(branch_regs[i].regmap);
    }
    
    emit_jmp(stubs[n][2]); // return address
    
    /* This works but uses a lot of memory...
     emit_readword((int)&last_count,ECX);
     emit_add(HOST_CCREG,ECX,EAX);
     emit_writeword(EAX,(int)&Count);
     emit_call((int)gen_interupt);
     emit_readword((int)&Count,HOST_CCREG);
     emit_readword((int)&next_interupt,EAX);
     emit_readword((int)&pending_exception,EBX);
     emit_writeword(EAX,(int)&last_count);
     emit_sub(HOST_CCREG,EAX,HOST_CCREG);
     emit_test(EBX,EBX);
     int jne_instr=(int)out;
     emit_jne(0);
     if(stubs[n][3]) emit_addimm(HOST_CCREG,-2*stubs[n][3],HOST_CCREG);
     load_all_regs(branch_regs[i].regmap);
     emit_jmp(stubs[n][2]); // return address
     set_jump_target(jne_instr,(int)out);
     emit_readword((int)&pcaddr,EAX);
     // Call get_addr_ht instead of doing the hash table here.
     // This code is executed infrequently and takes up a lot of space
     // so smaller is better.
     emit_storereg(CCREG,HOST_CCREG);
     emit_pushreg(EAX);
     emit_call((int)get_addr_ht);
     emit_loadreg(CCREG,HOST_CCREG);
     emit_addimm(ESP,4,ESP);
     emit_jmpreg(EAX);*/
}

void add_to_linker(int addr,int target,int ext)
{
    link_addr[linkcount][0]=addr;
    link_addr[linkcount][1]=target;
    link_addr[linkcount][2]=ext;
    linkcount++;
}

void ujump_assemble(int i,signed char i_regmap[])
{
    if(i==(ba[i]-start)>>2) assem_debug("idle loop\n");
#ifdef REG_PREFETCH
    int temp=get_reg(branch_regs[i].regmap,PTEMP);
    if(rt1[i]==31&&temp>=0)
    {
        int return_address=start+i*4+8;
        if(get_reg(branch_regs[i].regmap,31)>0)
            if(i_regmap[temp]==PTEMP) emit_movimm((int)hash_table[((return_address>>16)^return_address)&0xFFFF],temp);
    }
#endif
    ds_assemble(i+1,i_regmap);
    uint64_t bc_unneeded=branch_regs[i].u;
    uint64_t bc_unneeded_upper=branch_regs[i].uu;
    bc_unneeded|=1|(1LL<<rt1[i]);
    bc_unneeded_upper|=1|(1LL<<rt1[i]);
    wb_invalidate(regmap[i],branch_regs[i].regmap,dirty_post[i],is32_post[i],
                  bc_unneeded,bc_unneeded_upper);
    load_regs(regmap[i],branch_regs[i].regmap,is32[i],CCREG,CCREG);
    if(rt1[i]==31) {
        int rt,return_address;
        assert(rt1[i+1]!=31);
        assert(rt2[i+1]!=31);
        rt=get_reg(branch_regs[i].regmap,31);
        assem_debug("branch(%d): eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d\n",i,branch_regs[i].regmap[0],branch_regs[i].regmap[1],branch_regs[i].regmap[2],branch_regs[i].regmap[3],branch_regs[i].regmap[5],branch_regs[i].regmap[6],branch_regs[i].regmap[7]);
        //assert(rt>=0);
        return_address=start+i*4+8;
        if(rt>=0) {
#ifdef REG_PREFETCH
            if(temp>=0)
            {
                if(i_regmap[temp]!=PTEMP) emit_movimm((int)hash_table[((return_address>>16)^return_address)&0xFFFF],temp);
            }
#endif
            emit_movimm(return_address,rt); // PC into link register
#ifdef IMM_PREFETCH
            emit_prefetch(hash_table[((return_address>>16)^return_address)&0xFFFF]);
#endif
        }
    }
    int cc,adj;
    cc=get_reg(branch_regs[i].regmap,CCREG);
    assert(cc==HOST_CCREG);
    store_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
#ifdef REG_PREFETCH
    if(rt1[i]==31&&temp>=0) emit_prefetchreg(temp);
#endif
    do_cc(i,branch_regs[i].regmap,&adj,ba[i],TAKEN);
    if(adj) emit_addimm(cc,2*(ccadj[i]+2-adj),cc);
    load_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
    if(internal_branch(branch_regs[i].is32,ba[i]))
        assem_debug("branch: internal\n");
    else
        assem_debug("branch: external\n");
    add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
    emit_jmp(0);
}

void rjump_assemble(int i,signed char i_regmap[])
{
    int temp;
#ifdef REG_PREFETCH
    if(rt1[i]==31)
    {
        if((temp=get_reg(branch_regs[i].regmap,PTEMP))>=0) {
            int return_address=start+i*4+8;
            if(i_regmap[temp]==PTEMP) emit_movimm((int)hash_table[((return_address>>16)^return_address)&0xFFFF],temp);
        }
    }
#endif
    ds_assemble(i+1,i_regmap);
    uint64_t bc_unneeded=branch_regs[i].u;
    uint64_t bc_unneeded_upper=branch_regs[i].uu;
    bc_unneeded|=1|(1LL<<rt1[i]);
    bc_unneeded_upper|=1|(1LL<<rt1[i]);
    bc_unneeded&=~(1LL<<rs1[i]);
    wb_invalidate(regmap[i],branch_regs[i].regmap,dirty_post[i],is32_post[i],
                  bc_unneeded,bc_unneeded_upper);
    load_regs(regmap[i],branch_regs[i].regmap,is32[i],rs1[i],CCREG);
    if(rt1[i]==31) {
        int rt,return_address;
        assert(rt1[i+1]!=31);
        assert(rt2[i+1]!=31);
        rt=get_reg(branch_regs[i].regmap,31);
        assem_debug("branch(%d): eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d\n",i,branch_regs[i].regmap[0],branch_regs[i].regmap[1],branch_regs[i].regmap[2],branch_regs[i].regmap[3],branch_regs[i].regmap[5],branch_regs[i].regmap[6],branch_regs[i].regmap[7]);
        assert(rt>=0);
        return_address=start+i*4+8;
#ifdef REG_PREFETCH
        if(temp>=0)
        {
            if(i_regmap[temp]!=PTEMP) emit_movimm((int)hash_table[((return_address>>16)^return_address)&0xFFFF],temp);
        }
#endif
        emit_movimm(return_address,rt); // PC into link register
#ifdef IMM_PREFETCH
        emit_prefetch(hash_table[((return_address>>16)^return_address)&0xFFFF]);
#endif
    }
    int rs,cc,adj;
    rs=get_reg(branch_regs[i].regmap,rs1[i]);
    assert(rs>=0);
    cc=get_reg(branch_regs[i].regmap,CCREG);
    assert(cc==HOST_CCREG);
    store_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,-1);
#ifdef DESTRUCTIVE_WRITEBACK
    if((branch_regs[i].dirty>>rs)&(branch_regs[i].is32>>rs1[i])&1)
        emit_loadreg(rs1[i],rs);
#endif
#ifdef REG_PREFETCH
    if(rt1[i]==31&&temp>=0) emit_prefetchreg(temp);
#endif
    do_cc(i,branch_regs[i].regmap,&adj,-1,TAKEN);
    if(adj) emit_addimm(cc,2*(ccadj[i]+2-adj),cc); // ??? - Shouldn't happen
    assert(adj==0);
    //load_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,-1);
    if(rs!=EAX) emit_mov(rs,EAX);
    emit_jmp((int)jump_vaddr);
    /* Check hash table
     temp=!rs;
     emit_mov(rs,temp);
     emit_shrimm(rs,16,rs);
     emit_xor(temp,rs,rs);
     emit_movzwl_reg(rs,rs);
     emit_shlimm(rs,4,rs);
     emit_cmpmem_indexed((int)hash_table,rs,temp);
     emit_jne((int)out+14);
     emit_readword_indexed((int)hash_table+4,rs,rs);
     emit_jmpreg(rs);
     emit_cmpmem_indexed((int)hash_table+8,rs,temp);
     emit_addimm_no_flags(8,rs);
     emit_jeq((int)out-17);
     // No hit on hash table, call compiler
     emit_pushreg(temp);
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
     emit_storereg(CCREG,HOST_CCREG);
     emit_call((int)get_addr);
     emit_loadreg(CCREG,HOST_CCREG);
     emit_addimm(ESP,4,ESP);
     emit_jmpreg(EAX);*/
}

void cjump_assemble(int i,signed char i_regmap[])
{
    int cc;
    int match;
    match=match_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
    assem_debug("match=%d\n",match);
    int s1h,s1l,s2h,s2l;
    int prev_cop1_usable=cop1_usable;
    int unconditional=0;
    int only32=0;
    int ooo=1;
    int invert=0;
    if(i==(ba[i]-start)>>2) assem_debug("idle loop\n");
    if(likely[i]) ooo=0;
    if(!match) invert=1;
#ifdef CORTEX_A8_BRANCH_PREDICTION_HACK
    if(i>(ba[i]-start)>>2) invert=1;
#endif
    
    if(ooo)
        if((rs1[i]&&(rs1[i]==rt1[i+1]||rs1[i]==rt2[i+1]))||
           (rs2[i]&&(rs2[i]==rt1[i+1]||rs2[i]==rt2[i+1])))
        {
            // Write-after-read dependency prevents out of order execution
            // First test branch condition, then execute delay slot, then branch
            ooo=0;
        }
    
    if(ooo) {
        s1l=get_reg(branch_regs[i].regmap,rs1[i]);
        s1h=get_reg(branch_regs[i].regmap,rs1[i]|64);
        s2l=get_reg(branch_regs[i].regmap,rs2[i]);
        s2h=get_reg(branch_regs[i].regmap,rs2[i]|64);
    }
    else {
        s1l=get_reg(i_regmap,rs1[i]);
        s1h=get_reg(i_regmap,rs1[i]|64);
        s2l=get_reg(i_regmap,rs2[i]);
        s2h=get_reg(i_regmap,rs2[i]|64);
    }
    if(rs1[i]==0&&rs2[i]==0)
    {
        unconditional=1;
        assert(opcode[i]!=5);
        assert(opcode[i]!=7);
        assert(opcode[i]!=0x15);
        assert(opcode[i]!=0x17);
    }
    else if(rs1[i]==0)
    {
        s1l=s2l;s1h=s2h;
        s2l=s2h=-1;
        only32=(is32[i]>>rs2[i])&1;
    }
    else if(rs2[i]==0)
    {
        s2l=s2h=-1;
        only32=(is32[i]>>rs1[i])&1;
    }
    else {
        only32=(is32[i]>>rs1[i])&(is32[i]>>rs2[i])&1;
    }
    
    if(ooo) {
        // Out of order execution (delay slot first)
        //printf("OOOE\n");
        ds_assemble(i+1,i_regmap);
        int adj;
        uint64_t bc_unneeded=branch_regs[i].u;
        uint64_t bc_unneeded_upper=branch_regs[i].uu;
        bc_unneeded&=~((1LL<<rs1[i])|(1LL<<rs2[i]));
        bc_unneeded_upper&=~((1LL<<us1[i])|(1LL<<us2[i]));
        bc_unneeded|=1;
        bc_unneeded_upper|=1;
        // FIXME: Do we really need dirty_post and is32_post, or can we use i+1?
        wb_invalidate(regmap[i],branch_regs[i].regmap,dirty_post[i],is32_post[i],
                      bc_unneeded,bc_unneeded_upper);
        load_regs(regmap[i],branch_regs[i].regmap,is32[i],rs1[i],rs2[i]);
        load_regs(regmap[i],branch_regs[i].regmap,is32[i],CCREG,CCREG);
        cc=get_reg(branch_regs[i].regmap,CCREG);
        assert(cc==HOST_CCREG);
        if(unconditional)
            store_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
        do_cc(i,branch_regs[i].regmap,&adj,unconditional?ba[i]:-1,unconditional);
        //assem_debug("cycle count (adj)\n");
        if(unconditional) {
            if(i!=(ba[i]-start)>>2 || source[i+1]!=0) {
                if(adj) emit_addimm(cc,2*(ccadj[i]+2-adj),cc);
                load_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
                if(internal_branch(branch_regs[i].is32,ba[i]))
                    assem_debug("branch: internal\n");
                else
                    assem_debug("branch: external\n");
                add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
                emit_jmp(0);
            }
        }
        else {
            int taken=0,nottaken=0,nottaken1=0;
            if(adj&&!invert) emit_addimm(cc,2*(ccadj[i]+2-adj),cc);
            if(!only32)
            {
                assert(s1h>=0);
                if(opcode[i]==4) // BEQ
                {
                    if(s2h>=0) emit_cmp(s1h,s2h);
                    else emit_test(s1h,s1h);
                    nottaken1=(int)out;
                    emit_jne(1);
                }
                if(opcode[i]==5) // BNE
                {
                    if(s2h>=0) emit_cmp(s1h,s2h);
                    else emit_test(s1h,s1h);
                    if(invert) taken=(int)out;
                    else add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
                    emit_jne(0);
                }
                if(opcode[i]==6) // BLEZ
                {
                    emit_test(s1h,s1h);
                    if(invert) taken=(int)out;
                    else add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
                    emit_js(0);
                    nottaken1=(int)out;
                    emit_jne(1);
                }
                if(opcode[i]==7) // BGTZ
                {
                    emit_test(s1h,s1h);
                    nottaken1=(int)out;
                    emit_js(1);
                    if(invert) taken=(int)out;
                    else add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
                    emit_jne(0);
                }
            } // if(!only32)
            
            //printf("branch(%d): eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d\n",i,branch_regs[i].regmap[0],branch_regs[i].regmap[1],branch_regs[i].regmap[2],branch_regs[i].regmap[3],branch_regs[i].regmap[5],branch_regs[i].regmap[6],branch_regs[i].regmap[7]);
            assert(s1l>=0);
            if(opcode[i]==4) // BEQ
            {
                if(s2l>=0) emit_cmp(s1l,s2l);
                else emit_test(s1l,s1l);
                if(invert){
                    nottaken=(int)out;
                    emit_jne(1);
                }else{
                    add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
                    emit_jeq(0);
                }
            }
            if(opcode[i]==5) // BNE
            {
                if(s2l>=0) emit_cmp(s1l,s2l);
                else emit_test(s1l,s1l);
                if(invert){
                    nottaken=(int)out;
                    emit_jeq(1);
                }else{
                    add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
                    emit_jne(0);
                }
            }
            if(opcode[i]==6) // BLEZ
            {
                emit_cmpimm(s1l,1);
                if(invert){
                    nottaken=(int)out;
                    emit_jge(1);
                }else{
                    add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
                    emit_jl(0);
                }
            }
            if(opcode[i]==7) // BGTZ
            {
                emit_cmpimm(s1l,1);
                if(invert){
                    nottaken=(int)out;
                    emit_jl(1);
                }else{
                    add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
                    emit_jge(0);
                }
            }
            if(invert) {
                if(taken) set_jump_target(taken,(int)out);
                if(adj) emit_addimm(cc,2*(ccadj[i]+2-adj),cc);
#ifdef CORTEX_A8_BRANCH_PREDICTION_HACK
                else if(match) emit_mov(13,13);
#endif
                store_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
                load_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
                if(internal_branch(branch_regs[i].is32,ba[i]))
                    assem_debug("branch: internal\n");
                else
                    assem_debug("branch: external\n");
                add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
                emit_jmp(0);
                set_jump_target(nottaken,(int)out);
            }
            
            if(nottaken1) set_jump_target(nottaken1,(int)out);
            if(adj) {
                if(!invert) emit_addimm(cc,2*adj,cc);
                else emit_addimm(cc,2*(ccadj[i]+2),cc);
            }
        } // (!unconditional)
    } // if(ooo)
    else
    {
        // In-order execution (branch first)
        //if(likely[i]) printf("IOL\n");
        //else
        //printf("IOE\n");
        int taken=0,nottaken=0,nottaken1=0;
        if(!unconditional) {
            if(!only32)
            {
                assert(s1h>=0);
                if((opcode[i]&0x2f)==4) // BEQ
                {
                    if(s2h>=0) emit_cmp(s1h,s2h);
                    else emit_test(s1h,s1h);
                    nottaken1=(int)out;
                    emit_jne(2);
                }
                if((opcode[i]&0x2f)==5) // BNE
                {
                    if(s2h>=0) emit_cmp(s1h,s2h);
                    else emit_test(s1h,s1h);
                    taken=(int)out;
                    emit_jne(1);
                }
                if((opcode[i]&0x2f)==6) // BLEZ
                {
                    emit_test(s1h,s1h);
                    taken=(int)out;
                    emit_js(1);
                    nottaken1=(int)out;
                    emit_jne(2);
                }
                if((opcode[i]&0x2f)==7) // BGTZ
                {
                    emit_test(s1h,s1h);
                    nottaken1=(int)out;
                    emit_js(2);
                    taken=(int)out;
                    emit_jne(1);
                }
            } // if(!only32)
            
            //printf("branch(%d): eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d\n",i,branch_regs[i].regmap[0],branch_regs[i].regmap[1],branch_regs[i].regmap[2],branch_regs[i].regmap[3],branch_regs[i].regmap[5],branch_regs[i].regmap[6],branch_regs[i].regmap[7]);
            assert(s1l>=0);
            if((opcode[i]&0x2f)==4) // BEQ
            {
                if(s2l>=0) emit_cmp(s1l,s2l);
                else emit_test(s1l,s1l);
                nottaken=(int)out;
                emit_jne(2);
            }
            if((opcode[i]&0x2f)==5) // BNE
            {
                if(s2l>=0) emit_cmp(s1l,s2l);
                else emit_test(s1l,s1l);
                nottaken=(int)out;
                emit_jeq(2);
            }
            if((opcode[i]&0x2f)==6) // BLEZ
            {
                emit_cmpimm(s1l,1);
                nottaken=(int)out;
                emit_jge(2);
            }
            if((opcode[i]&0x2f)==7) // BGTZ
            {
                emit_cmpimm(s1l,1);
                nottaken=(int)out;
                emit_jl(2);
            }
        } // if(!unconditional)
        int adj;
        uint64_t ds_unneeded=branch_regs[i].u;
        uint64_t ds_unneeded_upper=branch_regs[i].uu;
        ds_unneeded&=~((1LL<<rs1[i+1])|(1LL<<rs2[i+1]));
        ds_unneeded_upper&=~((1LL<<us1[i+1])|(1LL<<us2[i+1]));
        if((~ds_unneeded_upper>>rt1[i+1])&1) ds_unneeded_upper&=~((1LL<<dep1[i+1])|(1LL<<dep2[i+1]));
        ds_unneeded|=1;
        ds_unneeded_upper|=1;
        // branch taken
        if(taken) set_jump_target(taken,(int)out);
        assem_debug("1:\n");
        wb_invalidate(regmap[i],branch_regs[i].regmap,dirty_post[i],is32_post[i],
                      ds_unneeded,ds_unneeded_upper);
        // load regs
        load_regs(regmap[i],branch_regs[i].regmap,is32[i],rs1[i+1],rs2[i+1]);
        load_regs(regmap[i],branch_regs[i].regmap,is32[i],CCREG,INVCP);
        ds_assemble(i+1,branch_regs[i].regmap);
        cc=get_reg(branch_regs[i].regmap,CCREG);
        if(cc==-1) {
            emit_loadreg(CCREG,cc=HOST_CCREG);
            // CHECK: Is the following instruction (fall thru) allocated ok?
        }
        assert(cc==HOST_CCREG);
        store_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
        do_cc(i,i_regmap,&adj,ba[i],TAKEN);
        assem_debug("cycle count (adj)\n");
        if(adj) emit_addimm(cc,2*(ccadj[i]+2-adj),cc);
        load_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
        if(internal_branch(branch_regs[i].is32,ba[i]))
            assem_debug("branch: internal\n");
        else
            assem_debug("branch: external\n");
        add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
        emit_jmp(0);
        
        // branch not taken
        cop1_usable=prev_cop1_usable;
        if(!unconditional) {
            if(nottaken1) set_jump_target(nottaken1,(int)out);
            set_jump_target(nottaken,(int)out);
            assem_debug("2:\n");
            if(!likely[i]) {
                wb_invalidate(regmap[i],branch_regs[i].regmap,dirty_post[i],is32_post[i],
                              ds_unneeded,ds_unneeded_upper);
                load_regs(regmap[i],branch_regs[i].regmap,is32[i],rs1[i+1],rs2[i+1]);
                load_regs(regmap[i],branch_regs[i].regmap,is32[i],CCREG,CCREG);
                ds_assemble(i+1,branch_regs[i].regmap);
            }
            cc=get_reg(branch_regs[i].regmap,CCREG);
            if(cc==-1&&!likely[i]) {
                // Cycle count isn't in a register, temporarily load it then write it out
                emit_loadreg(CCREG,HOST_CCREG);
                emit_addimm_and_set_flags(2*(ccadj[i]+2),HOST_CCREG);
                int jaddr=(int)out;
                emit_jns(0);
                add_stub(CC_STUB,jaddr,(int)out,0,i,start+i*4+8,NOTTAKEN);
                emit_storereg(CCREG,HOST_CCREG);
            }
            else{
                cc=get_reg(i_regmap,CCREG);
                assert(cc==HOST_CCREG);
                emit_addimm_and_set_flags(2*(ccadj[i]+2),cc);
                int jaddr=(int)out;
                emit_jns(0);
                add_stub(CC_STUB,jaddr,(int)out,0,i,start+i*4+8,likely[i]?NULLDS:NOTTAKEN);
            }
        }
    }
}

void sjump_assemble(int i,signed char i_regmap[])
{
    int cc;
    int match;
    match=match_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
    assem_debug("smatch=%d\n",match);
    int s1h,s1l;
    int prev_cop1_usable=cop1_usable;
    int unconditional=0;
    int only32=0;
    int ooo=1;
    int invert=0;
    if(i==(ba[i]-start)>>2) assem_debug("idle loop\n");
    if(likely[i]) ooo=0;
    if(!match) invert=1;
#ifdef CORTEX_A8_BRANCH_PREDICTION_HACK
    if(i>(ba[i]-start)>>2) invert=1;
#endif
    
    //if(opcode2[i]>=0x10) return; // FIXME (BxxZAL)
    assert(opcode2[i]<0x10||rs1[i]==0); // FIXME (BxxZAL)
    
    if(ooo)
        if(rs1[i]&&(rs1[i]==rt1[i+1]||rs1[i]==rt2[i+1]))
        {
            // Write-after-read dependency prevents out of order execution
            // First test branch condition, then execute delay slot, then branch
            ooo=0;
        }
    // TODO: Conditional branches w/link must execute in-order so that
    // condition test and write to r31 occur before cycle count test
    
    if(ooo) {
        s1l=get_reg(branch_regs[i].regmap,rs1[i]);
        s1h=get_reg(branch_regs[i].regmap,rs1[i]|64);
    }
    else {
        s1l=get_reg(i_regmap,rs1[i]);
        s1h=get_reg(i_regmap,rs1[i]|64);
    }
    if(rs1[i]==0)
    {
        unconditional=1;
        // TODO: These are NOPs
        assert(opcode2[i]!=0);
        assert(opcode2[i]!=2);
        assert(opcode2[i]!=0x10);
        assert(opcode2[i]!=0x12);
    }
    else {
        only32=(is32[i]>>rs1[i])&1;
    }
    
    if(ooo) {
        // Out of order execution (delay slot first)
        //printf("OOOE\n");
        ds_assemble(i+1,i_regmap);
        int adj;
        uint64_t bc_unneeded=branch_regs[i].u;
        uint64_t bc_unneeded_upper=branch_regs[i].uu;
        bc_unneeded&=~((1LL<<rs1[i])|(1LL<<rs2[i]));
        bc_unneeded_upper&=~((1LL<<us1[i])|(1LL<<us2[i]));
        bc_unneeded|=1;
        bc_unneeded_upper|=1;
        wb_invalidate(regmap[i],branch_regs[i].regmap,dirty_post[i],is32_post[i],
                      bc_unneeded,bc_unneeded_upper);
        load_regs(regmap[i],branch_regs[i].regmap,is32[i],rs1[i],rs1[i]);
        load_regs(regmap[i],branch_regs[i].regmap,is32[i],CCREG,CCREG);
        if(unconditional&&rt1[i]==31) {
            int rt,return_address;
            assert(rt1[i+1]!=31);
            assert(rt2[i+1]!=31);
            rt=get_reg(branch_regs[i].regmap,31);
            assem_debug("branch(%d): eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d\n",i,branch_regs[i].regmap[0],branch_regs[i].regmap[1],branch_regs[i].regmap[2],branch_regs[i].regmap[3],branch_regs[i].regmap[5],branch_regs[i].regmap[6],branch_regs[i].regmap[7]);
            if(rt>=0) {
                return_address=start+i*4+8;
                emit_movimm(return_address,rt); // PC into link register
#ifdef IMM_PREFETCH
                emit_prefetch(hash_table[((return_address>>16)^return_address)&0xFFFF]);
#endif
            }
        }
        cc=get_reg(branch_regs[i].regmap,CCREG);
        assert(cc==HOST_CCREG);
        if(unconditional)
            store_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
        do_cc(i,branch_regs[i].regmap,&adj,unconditional?ba[i]:-1,unconditional);
        assem_debug("cycle count (adj)\n");
        if(unconditional) {
            if(i!=(ba[i]-start)>>2 || source[i+1]!=0) {
                if(adj) emit_addimm(cc,2*(ccadj[i]+2-adj),cc);
                load_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
                if(internal_branch(branch_regs[i].is32,ba[i]))
                    assem_debug("branch: internal\n");
                else
                    assem_debug("branch: external\n");
                add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
                emit_jmp(0);
            }
        }
        else {
            int nottaken=0;
            if(adj&&!invert) emit_addimm(cc,2*(ccadj[i]+2-adj),cc);
            if(!only32)
            {
                assert(s1h>=0);
                if(opcode2[i]==0) // BLTZ
                {
                    emit_test(s1h,s1h);
                    if(invert){
                        nottaken=(int)out;
                        emit_jns(1);
                    }else{
                        add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
                        emit_js(0);
                    }
                }
                if(opcode2[i]==1) // BGEZ
                {
                    emit_test(s1h,s1h);
                    if(invert){
                        nottaken=(int)out;
                        emit_js(1);
                    }else{
                        add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
                        emit_jns(0);
                    }
                }
            } // if(!only32)
            else
            {
                assert(s1l>=0);
                if(opcode2[i]==0) // BLTZ
                {
                    emit_test(s1l,s1l);
                    if(invert){
                        nottaken=(int)out;
                        emit_jns(1);
                    }else{
                        add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
                        emit_js(0);
                    }
                }
                if(opcode2[i]==1) // BGEZ
                {
                    emit_test(s1l,s1l);
                    if(invert){
                        nottaken=(int)out;
                        emit_js(1);
                    }else{
                        add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
                        emit_jns(0);
                    }
                }
            } // if(!only32)
            
            if(invert) {
                if(adj) emit_addimm(cc,2*(ccadj[i]+2-adj),cc);
#ifdef CORTEX_A8_BRANCH_PREDICTION_HACK
                else if(match) emit_mov(13,13);
#endif
                store_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
                load_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
                if(internal_branch(branch_regs[i].is32,ba[i]))
                    assem_debug("branch: internal\n");
                else
                    assem_debug("branch: external\n");
                add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
                emit_jmp(0);
                set_jump_target(nottaken,(int)out);
            }
            
            if(adj) {
                if(!invert) emit_addimm(cc,2*adj,cc);
                else emit_addimm(cc,2*(ccadj[i]+2),cc);
            }
        } // (!unconditional)
    } // if(ooo)
    else
    {
        // In-order execution (branch first)
        //printf("IOE\n");
        int nottaken=0;
        if(!unconditional) {
            //printf("branch(%d): eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d\n",i,branch_regs[i].regmap[0],branch_regs[i].regmap[1],branch_regs[i].regmap[2],branch_regs[i].regmap[3],branch_regs[i].regmap[5],branch_regs[i].regmap[6],branch_regs[i].regmap[7]);
            if(!only32)
            {
                assert(s1h>=0);
                if((opcode2[i]&0x1d)==0) // BLTZ/BLTZL
                {
                    emit_test(s1h,s1h);
                    nottaken=(int)out;
                    emit_jns(1);
                }
                if((opcode2[i]&0x1d)==1) // BGEZ/BGEZL
                {
                    emit_test(s1h,s1h);
                    nottaken=(int)out;
                    emit_js(1);
                }
            } // if(!only32)
            else
            {
                assert(s1l>=0);
                if((opcode2[i]&0x1d)==0) // BLTZ/BLTZL
                {
                    emit_test(s1l,s1l);
                    nottaken=(int)out;
                    emit_jns(1);
                }
                if((opcode2[i]&0x1d)==1) // BGEZ/BGEZL
                {
                    emit_test(s1l,s1l);
                    nottaken=(int)out;
                    emit_js(1);
                }
            }
        } // if(!unconditional)
        int adj;
        uint64_t ds_unneeded=branch_regs[i].u;
        uint64_t ds_unneeded_upper=branch_regs[i].uu;
        ds_unneeded&=~((1LL<<rs1[i+1])|(1LL<<rs2[i+1]));
        ds_unneeded_upper&=~((1LL<<us1[i+1])|(1LL<<us2[i+1]));
        if((~ds_unneeded_upper>>rt1[i+1])&1) ds_unneeded_upper&=~((1LL<<dep1[i+1])|(1LL<<dep2[i+1]));
        ds_unneeded|=1;
        ds_unneeded_upper|=1;
        // branch taken
        //assem_debug("1:\n");
        wb_invalidate(regmap[i],branch_regs[i].regmap,dirty_post[i],is32_post[i],
                      ds_unneeded,ds_unneeded_upper);
        // load regs
        load_regs(regmap[i],branch_regs[i].regmap,is32[i],rs1[i+1],rs2[i+1]);
        load_regs(regmap[i],branch_regs[i].regmap,is32[i],CCREG,INVCP);
        ds_assemble(i+1,branch_regs[i].regmap);
        cc=get_reg(branch_regs[i].regmap,CCREG);
        if(cc==-1) {
            emit_loadreg(CCREG,cc=HOST_CCREG);
            // CHECK: Is the following instruction (fall thru) allocated ok?
        }
        assert(cc==HOST_CCREG);
        store_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
        do_cc(i,i_regmap,&adj,ba[i],TAKEN);
        assem_debug("cycle count (adj)\n");
        if(adj) emit_addimm(cc,2*(ccadj[i]+2-adj),cc);
        load_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
        if(internal_branch(branch_regs[i].is32,ba[i]))
            assem_debug("branch: internal\n");
        else
            assem_debug("branch: external\n");
        add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
        emit_jmp(0);
        
        // branch not taken
        cop1_usable=prev_cop1_usable;
        if(!unconditional) {
            set_jump_target(nottaken,(int)out);
            assem_debug("1:\n");
            if(!likely[i]) {
                wb_invalidate(regmap[i],branch_regs[i].regmap,dirty_post[i],is32_post[i],
                              ds_unneeded,ds_unneeded_upper);
                load_regs(regmap[i],branch_regs[i].regmap,is32[i],rs1[i+1],rs2[i+1]);
                load_regs(regmap[i],branch_regs[i].regmap,is32[i],CCREG,CCREG);
                ds_assemble(i+1,branch_regs[i].regmap);
            }
            cc=get_reg(branch_regs[i].regmap,CCREG);
            if(cc==-1&&!likely[i]) {
                // Cycle count isn't in a register, temporarily load it then write it out
                emit_loadreg(CCREG,HOST_CCREG);
                emit_addimm_and_set_flags(2*(ccadj[i]+2),HOST_CCREG);
                int jaddr=(int)out;
                emit_jns(0);
                add_stub(CC_STUB,jaddr,(int)out,0,i,start+i*4+8,NOTTAKEN);
                emit_storereg(CCREG,HOST_CCREG);
            }
            else{
                cc=get_reg(i_regmap,CCREG);
                assert(cc==HOST_CCREG);
                emit_addimm_and_set_flags(2*(ccadj[i]+2),cc);
                int jaddr=(int)out;
                emit_jns(0);
                add_stub(CC_STUB,jaddr,(int)out,0,i,start+i*4+8,likely[i]?NULLDS:NOTTAKEN);
            }
        }
    }
}

void fjump_assemble(int i,signed char i_regmap[])
{
    int cc;
    int match;
    match=match_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
    assem_debug("fmatch=%d\n",match);
    int fs,cs;
    int eaddr;
    int ooo=1;
    int invert=0;
    if(i==(ba[i]-start)>>2) assem_debug("idle loop\n");
    if(likely[i]) ooo=0;
    if(!match) invert=1;
#ifdef CORTEX_A8_BRANCH_PREDICTION_HACK
    if(i>(ba[i]-start)>>2) invert=1;
#endif
    
    if(ooo)
        if(itype[i+1]==FLOAT||itype[i+1]==FCOMP)
        {
            // Write-after-read dependency prevents out of order execution
            // First test branch condition, then execute delay slot, then branch
            ooo=0;
        }
    
    if(ooo) {
        cs=get_reg(branch_regs[i].regmap,CSREG); // CHECK: Is this right?
        fs=get_reg(branch_regs[i].regmap,FSREG);
    }
    else {
        cs=get_reg(i_regmap,CSREG);
        fs=get_reg(i_regmap,FSREG);
    }
    
    // Check cop1 unusable
    assert(cs>=0);
    if(!cop1_usable) {
        emit_testimm(cs,0x20000000);
        eaddr=(int)out;
        emit_jeq(0);
        add_stub(FP_STUB,eaddr,(int)out,i,cs,0,0);
        cop1_usable=1;
    }
    
    if(ooo) {
        // Out of order execution (delay slot first)
        //printf("OOOE\n");
        ds_assemble(i+1,i_regmap);
        int adj;
        uint64_t bc_unneeded=branch_regs[i].u;
        uint64_t bc_unneeded_upper=branch_regs[i].uu;
        bc_unneeded&=~((1LL<<rs1[i])|(1LL<<rs2[i]));
        bc_unneeded_upper&=~((1LL<<us1[i])|(1LL<<us2[i]));
        bc_unneeded|=1;
        bc_unneeded_upper|=1;
        wb_invalidate(regmap[i],branch_regs[i].regmap,dirty_post[i],is32_post[i],
                      bc_unneeded,bc_unneeded_upper);
        load_regs(regmap[i],branch_regs[i].regmap,is32[i],rs1[i],rs1[i]);
        load_regs(regmap[i],branch_regs[i].regmap,is32[i],CCREG,CCREG);
        cc=get_reg(branch_regs[i].regmap,CCREG);
        assert(cc==HOST_CCREG);
        do_cc(i,branch_regs[i].regmap,&adj,-1,0);
        assem_debug("cycle count (adj)\n");
        if(1) {
            int nottaken=0;
            if(adj&&!invert) emit_addimm(cc,2*(ccadj[i]+2-adj),cc);
            if(1) {
                assert(fs>=0);
                emit_testimm(fs,0x800000);
                if(source[i]&0x10000) // BC1T
                {
                    if(invert){
                        nottaken=(int)out;
                        emit_jeq(1);
                    }else{
                        add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
                        emit_jne(0);
                    }
                }
                else // BC1F
                    if(invert){
                        nottaken=(int)out;
                        emit_jne(1);
                    }else{
                        add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
                        emit_jeq(0);
                    }
                {
                }
            } // if(!only32)
            
            if(invert) {
                if(adj) emit_addimm(cc,2*(ccadj[i]+2-adj),cc);
#ifdef CORTEX_A8_BRANCH_PREDICTION_HACK
                else if(match) emit_mov(13,13);
#endif
                store_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
                load_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
                if(internal_branch(branch_regs[i].is32,ba[i]))
                    assem_debug("branch: internal\n");
                else
                    assem_debug("branch: external\n");
                add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
                emit_jmp(0);
                set_jump_target(nottaken,(int)out);
            }
            
            if(adj) {
                if(!invert) emit_addimm(cc,2*adj,cc);
                else emit_addimm(cc,2*(ccadj[i]+2),cc);
            }
        } // (!unconditional)
    } // if(ooo)
    else
    {
        // In-order execution (branch first)
        //printf("IOE\n");
        int nottaken=0;
        if(1) {
            //printf("branch(%d): eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d\n",i,branch_regs[i].regmap[0],branch_regs[i].regmap[1],branch_regs[i].regmap[2],branch_regs[i].regmap[3],branch_regs[i].regmap[5],branch_regs[i].regmap[6],branch_regs[i].regmap[7]);
            if(1) {
                assert(fs>=0);
                emit_testimm(fs,0x800000);
                if(source[i]&0x10000) // BC1T
                {
                    nottaken=(int)out;
                    emit_jeq(1);
                }
                else // BC1F
                {
                    nottaken=(int)out;
                    emit_jne(1);
                }
            }
        } // if(!unconditional)
        int adj;
        uint64_t ds_unneeded=branch_regs[i].u;
        uint64_t ds_unneeded_upper=branch_regs[i].uu;
        ds_unneeded&=~((1LL<<rs1[i+1])|(1LL<<rs2[i+1]));
        ds_unneeded_upper&=~((1LL<<us1[i+1])|(1LL<<us2[i+1]));
        if((~ds_unneeded_upper>>rt1[i+1])&1) ds_unneeded_upper&=~((1LL<<dep1[i+1])|(1LL<<dep2[i+1]));
        ds_unneeded|=1;
        ds_unneeded_upper|=1;
        // branch taken
        //assem_debug("1:\n");
        wb_invalidate(regmap[i],branch_regs[i].regmap,dirty_post[i],is32_post[i],
                      ds_unneeded,ds_unneeded_upper);
        // load regs
        load_regs(regmap[i],branch_regs[i].regmap,is32[i],rs1[i+1],rs2[i+1]);
        load_regs(regmap[i],branch_regs[i].regmap,is32[i],CCREG,INVCP);
        ds_assemble(i+1,branch_regs[i].regmap);
        cc=get_reg(branch_regs[i].regmap,CCREG);
        if(cc==-1) {
            emit_loadreg(CCREG,cc=HOST_CCREG);
            // CHECK: Is the following instruction (fall thru) allocated ok?
        }
        assert(cc==HOST_CCREG);
        store_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
        do_cc(i,i_regmap,&adj,ba[i],TAKEN);
        assem_debug("cycle count (adj)\n");
        if(adj) emit_addimm(cc,2*(ccadj[i]+2-adj),cc);
        load_regs_bt(branch_regs[i].regmap,branch_regs[i].is32,branch_regs[i].dirty,ba[i]);
        if(internal_branch(branch_regs[i].is32,ba[i]))
            assem_debug("branch: internal\n");
        else
            assem_debug("branch: external\n");
        add_to_linker((int)out,ba[i],internal_branch(branch_regs[i].is32,ba[i]));
        emit_jmp(0);
        
        // branch not taken
        if(1) { // <- FIXME (don't need this)
            set_jump_target(nottaken,(int)out);
            assem_debug("1:\n");
            if(!likely[i]) {
                wb_invalidate(regmap[i],branch_regs[i].regmap,dirty_post[i],is32_post[i],
                              ds_unneeded,ds_unneeded_upper);
                load_regs(regmap[i],branch_regs[i].regmap,is32[i],rs1[i+1],rs2[i+1]);
                load_regs(regmap[i],branch_regs[i].regmap,is32[i],CCREG,CCREG);
                ds_assemble(i+1,branch_regs[i].regmap);
            }
            cc=get_reg(branch_regs[i].regmap,CCREG);
            if(cc==-1&&!likely[i]) {
                // Cycle count isn't in a register, temporarily load it then write it out
                emit_loadreg(CCREG,HOST_CCREG);
                emit_addimm_and_set_flags(2*(ccadj[i]+2),HOST_CCREG);
                int jaddr=(int)out;
                emit_jns(0);
                add_stub(CC_STUB,jaddr,(int)out,0,i,start+i*4+8,NOTTAKEN);
                emit_storereg(CCREG,HOST_CCREG);
            }
            else{
                cc=get_reg(i_regmap,CCREG);
                assert(cc==HOST_CCREG);
                emit_addimm_and_set_flags(2*(ccadj[i]+2),cc);
                int jaddr=(int)out;
                emit_jns(0);
                add_stub(CC_STUB,jaddr,(int)out,0,i,start+i*4+8,likely[i]?NULLDS:NOTTAKEN);
            }
        }
    }
}

void unneeded_registers(int istart,int iend)
{
    int i;
    uint64_t u,uu,b,bu;
    uint64_t temp_u,temp_uu;
    uint64_t tdep;
    if(iend==slen-1) {
        u=1;uu=1;
    }else{
        u=unneeded_reg[iend+1];
        uu=unneeded_reg_upper[iend+1];
        u=1;uu=1;
    }
    for (i=iend;i>=istart;i--)
    {
        if(itype[i]==RJUMP||itype[i]==UJUMP||itype[i]==CJUMP||itype[i]==SJUMP||itype[i]==FJUMP)
        {
            // If subroutine call, flag return address as a possible branch target
            if(rt1[i]==31 && i<slen-2) bt[i+2]=1;
            
            if(ba[i]<start || ba[i]>=(start+slen*4))
            {
                // Branch out of this block, flush all regs
                u=1;
                uu=1;
                branch_unneeded_reg[i]=u;
                branch_unneeded_reg_upper[i]=uu;
                // Merge in delay slot
                tdep=(~uu>>rt1[i+1])&1;
                u|=(1LL<<rt1[i+1])|(1LL<<rt2[i+1]);
                uu|=(1LL<<rt1[i+1])|(1LL<<rt2[i+1]);
                u&=~((1LL<<rs1[i+1])|(1LL<<rs2[i+1]));
                uu&=~((1LL<<us1[i+1])|(1LL<<us2[i+1]));
                uu&=~((tdep<<dep1[i+1])|(tdep<<dep2[i+1]));
                u|=1;uu|=1;
                // If branch is "likely" (and conditional)
                // then we skip the delay slot on the fall-thru path
                if(likely[i]) {
                    if(i<slen-1) {
                        u&=unneeded_reg[i+2];
                        uu&=unneeded_reg_upper[i+2];
                    }
                    else
                    {
                        u=1;
                        uu=1;
                    }
                }
            }
            else
            {
                // Internal branch, flag target
                bt[(ba[i]-start)>>2]=1;
                if(ba[i]<=start+i*4) {
                    // Backward branch
                    if(itype[i]==RJUMP||itype[i]==UJUMP||(source[i]>>16)==0x1000)
                    {
                        // Unconditional branch
                        temp_u=1;temp_uu=1;
                    } else {
                        // Conditional branch (not taken case)
                        temp_u=unneeded_reg[i+2];
                        temp_uu=unneeded_reg_upper[i+2];
                    }
                    // Merge in delay slot
                    tdep=(~temp_uu>>rt1[i+1])&1;
                    temp_u|=(1LL<<rt1[i+1])|(1LL<<rt2[i+1]);
                    temp_uu|=(1LL<<rt1[i+1])|(1LL<<rt2[i+1]);
                    temp_u&=~((1LL<<rs1[i+1])|(1LL<<rs2[i+1]));
                    temp_uu&=~((1LL<<us1[i+1])|(1LL<<us2[i+1]));
                    temp_uu&=~((tdep<<dep1[i+1])|(tdep<<dep2[i+1]));
                    temp_u|=1;temp_uu|=1;
                    // If branch is "likely" (and conditional)
                    // then we skip the delay slot on the fall-thru path
                    if(likely[i]) {
                        if(i<slen-1) {
                            temp_u&=unneeded_reg[i+2];
                            temp_uu&=unneeded_reg_upper[i+2];
                        }
                        else
                        {
                            temp_u=1;
                            temp_uu=1;
                        }
                    }
                    tdep=(~temp_uu>>rt1[i])&1;
                    temp_u|=(1LL<<rt1[i])|(1LL<<rt2[i]);
                    temp_uu|=(1LL<<rt1[i])|(1LL<<rt2[i]);
                    temp_u&=~((1LL<<rs1[i])|(1LL<<rs2[i]));
                    temp_uu&=~((1LL<<us1[i])|(1LL<<us2[i]));
                    temp_uu&=~((tdep<<dep1[i])|(tdep<<dep2[i]));
                    temp_u|=1;temp_uu|=1;
                    unneeded_reg[i]=temp_u;
                    unneeded_reg_upper[i]=temp_uu;
                    unneeded_registers((ba[i]-start)>>2,i-1);
                } /*else*/ if(1) {
                    if(itype[i]==RJUMP||itype[i]==UJUMP||(source[i]>>16)==0x1000)
                    {
                        // Unconditional branch
                        u=unneeded_reg[(ba[i]-start)>>2];
                        uu=unneeded_reg_upper[(ba[i]-start)>>2];
                        branch_unneeded_reg[i]=u;
                        branch_unneeded_reg_upper[i]=uu;
                        //u=1;
                        //uu=1;
                        //branch_unneeded_reg[i]=u;
                        //branch_unneeded_reg_upper[i]=uu;
                        // Merge in delay slot
                        tdep=(~uu>>rt1[i+1])&1;
                        u|=(1LL<<rt1[i+1])|(1LL<<rt2[i+1]);
                        uu|=(1LL<<rt1[i+1])|(1LL<<rt2[i+1]);
                        u&=~((1LL<<rs1[i+1])|(1LL<<rs2[i+1]));
                        uu&=~((1LL<<us1[i+1])|(1LL<<us2[i+1]));
                        uu&=~((tdep<<dep1[i+1])|(tdep<<dep2[i+1]));
                        u|=1;uu|=1;
                    } else {
                        // Conditional branch
                        b=unneeded_reg[(ba[i]-start)>>2];
                        bu=unneeded_reg_upper[(ba[i]-start)>>2];
                        branch_unneeded_reg[i]=b;
                        branch_unneeded_reg_upper[i]=bu;
                        //b=1;
                        //bu=1;
                        //branch_unneeded_reg[i]=b;
                        //branch_unneeded_reg_upper[i]=bu;
                        // Branch delay slot
                        tdep=(~uu>>rt1[i+1])&1;
                        b|=(1LL<<rt1[i+1])|(1LL<<rt2[i+1]);
                        bu|=(1LL<<rt1[i+1])|(1LL<<rt2[i+1]);
                        b&=~((1LL<<rs1[i+1])|(1LL<<rs2[i+1]));
                        bu&=~((1LL<<us1[i+1])|(1LL<<us2[i+1]));
                        bu&=~((tdep<<dep1[i+1])|(tdep<<dep2[i+1]));
                        b|=1;bu|=1;
                        // If branch is "likely" then we skip the
                        // delay slot on the fall-thru path
                        if(likely[i]) {
                            u=b;
                            uu=bu;
                            if(i<slen-1) {
                                u&=unneeded_reg[i+2];
                                uu&=unneeded_reg_upper[i+2];
                                //u=1;
                                //uu=1;
                            }
                        } else {
                            u&=b;
                            uu&=bu;
                            //u=1;
                            //uu=1;
                        }
                        if(i<slen-1) {
                            branch_unneeded_reg[i]&=unneeded_reg[i+2];
                            branch_unneeded_reg_upper[i]&=unneeded_reg_upper[i+2];
                            //branch_unneeded_reg[i]=1;
                            //branch_unneeded_reg_upper[i]=1;
                        } else {
                            branch_unneeded_reg[i]=1;
                            branch_unneeded_reg_upper[i]=1;
                        }
                    }
                }
            }
        }
        else if(itype[i]==SYSCALL)
        {
            // SYSCALL instruction (software interrupt)
            u=1;
            uu=1;
        }
        else if(itype[i]==COP0 && (source[i]&0x3f)==0x18)
        {
            // ERET instruction (return from interrupt)
            u=1;
            uu=1;
        }
        tdep=(~uu>>rt1[i])&1;
        // Written registers are unneeded
        u|=1LL<<rt1[i];
        u|=1LL<<rt2[i];
        uu|=1LL<<rt1[i];
        uu|=1LL<<rt2[i];
        // Accessed registers are needed
        u&=~(1LL<<rs1[i]);
        u&=~(1LL<<rs2[i]);
        uu&=~(1LL<<us1[i]);
        uu&=~(1LL<<us2[i]);
        // Source-target dependencies
        uu&=~(tdep<<dep1[i]);
        uu&=~(tdep<<dep2[i]);
        // R0 is always unneeded
        u|=1;uu|=1;
        // Save it
        unneeded_reg[i]=u;
        unneeded_reg_upper[i]=uu;
        /*
         printf("ur (%d,%d) %x: ",istart,iend,start+i*4);
         printf("U:");
         int r;
         for(r=1;r<=CCREG;r++) {
         if((unneeded_reg[i]>>r)&1) {
         if(r==HIREG) printf(" HI");
         else if(r==LOREG) printf(" LO");
         else printf(" r%d",r);
         }
         }
         printf(" UU:");
         for(r=1;r<=CCREG;r++) {
         if(((unneeded_reg_upper[i]&~unneeded_reg[i])>>r)&1) {
         if(r==HIREG) printf(" HI");
         else if(r==LOREG) printf(" LO");
         else printf(" r%d",r);
         }
         }
         printf("\n");*/
    }
}

// Write back dirty registers as soon as we will no longer modify them,
// so that we don't end up with lots of writes at the branches.
void clean_registers(int istart,int iend,int wr)
{
    int i;
    int r;
    u_int will_dirty_i,will_dirty_next,temp_will_dirty;
    u_int wont_dirty_i,wont_dirty_next,temp_wont_dirty;
    if(iend==slen-1) {
        will_dirty_i=will_dirty_next=0;
        wont_dirty_i=wont_dirty_next=0;
    }else{
        will_dirty_i=will_dirty_next=will_dirty[iend+1];
        wont_dirty_i=wont_dirty_next=wont_dirty[iend+1];
    }
    for (i=iend;i>=istart;i--)
    {
        if(itype[i]==RJUMP||itype[i]==UJUMP||itype[i]==CJUMP||itype[i]==SJUMP||itype[i]==FJUMP)
        {
            if(ba[i]<start || ba[i]>=(start+slen*4))
            {
                // Branch out of this block, flush all regs
                if(itype[i]==RJUMP||itype[i]==UJUMP||(source[i]>>16)==0x1000)
                {
                    // Unconditional branch
                    will_dirty_i=0;
                    wont_dirty_i=0;
                    // Merge in delay slot (will dirty)
                    for(r=0;r<HOST_REGS;r++) {
                        if(r!=9 && r!=EXCLUDE_REG) {
                            if((branch_regs[i].regmap[r]&63)==rt1[i]) will_dirty_i|=1<<r;
                            if((branch_regs[i].regmap[r]&63)==rt2[i]) will_dirty_i|=1<<r;
                            if((branch_regs[i].regmap[r]&63)==rt1[i+1]) will_dirty_i|=1<<r;
                            if((branch_regs[i].regmap[r]&63)==rt2[i+1]) will_dirty_i|=1<<r;
                            if((branch_regs[i].regmap[r]&63)>33) will_dirty_i&=~(1<<r);
                            if(branch_regs[i].regmap[r]<=0) will_dirty_i&=~(1<<r);
                            if(branch_regs[i].regmap[r]==CCREG) will_dirty_i|=1<<r;
                            if((regmap[i][r]&63)==rt1[i]) will_dirty_i|=1<<r;
                            if((regmap[i][r]&63)==rt2[i]) will_dirty_i|=1<<r;
                            if((regmap[i][r]&63)==rt1[i+1]) will_dirty_i|=1<<r;
                            if((regmap[i][r]&63)==rt2[i+1]) will_dirty_i|=1<<r;
                            if((regmap[i][r]&63)>33) will_dirty_i&=~(1<<r);
                            if(regmap[i][r]<=0) will_dirty_i&=~(1<<r);
                            if(regmap[i][r]==CCREG) will_dirty_i|=1<<r;
                        }
                    }
                }
                else
                {
                    // Conditional branch
                    will_dirty_i=0;
                    wont_dirty_i=wont_dirty_next;
                    // Merge in delay slot (will dirty)
                    for(r=0;r<HOST_REGS;r++) {
                        if(r!=9 && r!=EXCLUDE_REG) {
                            if(!likely[i]) {
                                // Might not dirty if likely branch is not taken
                                if((branch_regs[i].regmap[r]&63)==rt1[i]) will_dirty_i|=1<<r;
                                if((branch_regs[i].regmap[r]&63)==rt2[i]) will_dirty_i|=1<<r;
                                if((branch_regs[i].regmap[r]&63)==rt1[i+1]) will_dirty_i|=1<<r;
                                if((branch_regs[i].regmap[r]&63)==rt2[i+1]) will_dirty_i|=1<<r;
                                if((branch_regs[i].regmap[r]&63)>33) will_dirty_i&=~(1<<r);
                                if(branch_regs[i].regmap[r]==0) will_dirty_i&=~(1<<r);
                                if(branch_regs[i].regmap[r]==CCREG) will_dirty_i|=1<<r;
                                //if((regmap[i][r]&63)==rt1[i]) will_dirty_i|=1<<r;
                                //if((regmap[i][r]&63)==rt2[i]) will_dirty_i|=1<<r;
                                if((regmap[i][r]&63)==rt1[i+1]) will_dirty_i|=1<<r;
                                if((regmap[i][r]&63)==rt2[i+1]) will_dirty_i|=1<<r;
                                if((regmap[i][r]&63)>33) will_dirty_i&=~(1<<r);
                                if(regmap[i][r]<=0) will_dirty_i&=~(1<<r);
                                if(regmap[i][r]==CCREG) will_dirty_i|=1<<r;
                            }
                        }
                    }
                }
                // Merge in delay slot (wont dirty)
                for(r=0;r<HOST_REGS;r++) {
                    if(r!=9 && r!=EXCLUDE_REG) {
                        if((regmap[i][r]&63)==rt1[i]) wont_dirty_i|=1<<r;
                        if((regmap[i][r]&63)==rt2[i]) wont_dirty_i|=1<<r;
                        if((regmap[i][r]&63)==rt1[i+1]) wont_dirty_i|=1<<r;
                        if((regmap[i][r]&63)==rt2[i+1]) wont_dirty_i|=1<<r;
                        if(regmap[i][r]==CCREG) wont_dirty_i|=1<<r;
                        if((branch_regs[i].regmap[r]&63)==rt1[i]) wont_dirty_i|=1<<r;
                        if((branch_regs[i].regmap[r]&63)==rt2[i]) wont_dirty_i|=1<<r;
                        if((branch_regs[i].regmap[r]&63)==rt1[i+1]) wont_dirty_i|=1<<r;
                        if((branch_regs[i].regmap[r]&63)==rt2[i+1]) wont_dirty_i|=1<<r;
                        if(branch_regs[i].regmap[r]==CCREG) wont_dirty_i|=1<<r;
                    }
                }
                if(wr) {
#ifndef DESTRUCTIVE_WRITEBACK
                    branch_regs[i].dirty&=wont_dirty_i;
#endif
                    branch_regs[i].dirty|=will_dirty_i;
                }
            }
            else
            {
                // Internal branch
                if(ba[i]<=start+i*4) {
                    // Backward branch
                    if(itype[i]==RJUMP||itype[i]==UJUMP||(source[i]>>16)==0x1000)
                    {
                        // Unconditional branch
                        temp_will_dirty=0;
                        temp_wont_dirty=0;
                        // Merge in delay slot (will dirty)
                        for(r=0;r<HOST_REGS;r++) {
                            if(r!=9 && r!=EXCLUDE_REG) {
                                if((branch_regs[i].regmap[r]&63)==rt1[i]) temp_will_dirty|=1<<r;
                                if((branch_regs[i].regmap[r]&63)==rt2[i]) temp_will_dirty|=1<<r;
                                if((branch_regs[i].regmap[r]&63)==rt1[i+1]) temp_will_dirty|=1<<r;
                                if((branch_regs[i].regmap[r]&63)==rt2[i+1]) temp_will_dirty|=1<<r;
                                if((branch_regs[i].regmap[r]&63)>33) temp_will_dirty&=~(1<<r);
                                if(branch_regs[i].regmap[r]<=0) temp_will_dirty&=~(1<<r);
                                if(branch_regs[i].regmap[r]==CCREG) temp_will_dirty|=1<<r;
                                if((regmap[i][r]&63)==rt1[i]) temp_will_dirty|=1<<r;
                                if((regmap[i][r]&63)==rt2[i]) temp_will_dirty|=1<<r;
                                if((regmap[i][r]&63)==rt1[i+1]) temp_will_dirty|=1<<r;
                                if((regmap[i][r]&63)==rt2[i+1]) temp_will_dirty|=1<<r;
                                if((regmap[i][r]&63)>33) temp_will_dirty&=~(1<<r);
                                if(regmap[i][r]<=0) temp_will_dirty&=~(1<<r);
                                if(regmap[i][r]==CCREG) temp_will_dirty|=1<<r;
                            }
                        }
                    } else {
                        // Conditional branch (not taken case)
                        temp_will_dirty=will_dirty_next;
                        temp_wont_dirty=wont_dirty_next;
                        // Merge in delay slot (will dirty)
                        for(r=0;r<HOST_REGS;r++) {
                            if(r!=9 && r!=EXCLUDE_REG) {
                                if(!likely[i]) {
                                    // Will not dirty if likely branch is not taken
                                    if((branch_regs[i].regmap[r]&63)==rt1[i]) temp_will_dirty|=1<<r;
                                    if((branch_regs[i].regmap[r]&63)==rt2[i]) temp_will_dirty|=1<<r;
                                    if((branch_regs[i].regmap[r]&63)==rt1[i+1]) temp_will_dirty|=1<<r;
                                    if((branch_regs[i].regmap[r]&63)==rt2[i+1]) temp_will_dirty|=1<<r;
                                    if((branch_regs[i].regmap[r]&63)>33) temp_will_dirty&=~(1<<r);
                                    if(branch_regs[i].regmap[r]==0) temp_will_dirty&=~(1<<r);
                                    if(branch_regs[i].regmap[r]==CCREG) temp_will_dirty|=1<<r;
                                    //if((regmap[i][r]&63)==rt1[i]) temp_will_dirty|=1<<r;
                                    //if((regmap[i][r]&63)==rt2[i]) temp_will_dirty|=1<<r;
                                    if((regmap[i][r]&63)==rt1[i+1]) temp_will_dirty|=1<<r;
                                    if((regmap[i][r]&63)==rt2[i+1]) temp_will_dirty|=1<<r;
                                    if((regmap[i][r]&63)>33) temp_will_dirty&=~(1<<r);
                                    if(regmap[i][r]<=0) temp_will_dirty&=~(1<<r);
                                    if(regmap[i][r]==CCREG) temp_will_dirty|=1<<r;
                                }
                            }
                        }
                    }
                    // Merge in delay slot (wont dirty)
                    for(r=0;r<HOST_REGS;r++) {
                        if(r!=9 && r!=EXCLUDE_REG) {
                            if((regmap[i][r]&63)==rt1[i]) temp_wont_dirty|=1<<r;
                            if((regmap[i][r]&63)==rt2[i]) temp_wont_dirty|=1<<r;
                            if((regmap[i][r]&63)==rt1[i+1]) temp_wont_dirty|=1<<r;
                            if((regmap[i][r]&63)==rt2[i+1]) temp_wont_dirty|=1<<r;
                            if(regmap[i][r]==CCREG) temp_wont_dirty|=1<<r;
                            if((branch_regs[i].regmap[r]&63)==rt1[i]) temp_wont_dirty|=1<<r;
                            if((branch_regs[i].regmap[r]&63)==rt2[i]) temp_wont_dirty|=1<<r;
                            if((branch_regs[i].regmap[r]&63)==rt1[i+1]) temp_wont_dirty|=1<<r;
                            if((branch_regs[i].regmap[r]&63)==rt2[i+1]) temp_wont_dirty|=1<<r;
                            if(branch_regs[i].regmap[r]==CCREG) temp_wont_dirty|=1<<r;
                        }
                    }
                    // Deal with changed mappings
                    if(i<iend) {
                        for(r=0;r<HOST_REGS;r++) {
                            if(r!=9 && r!=EXCLUDE_REG) {
                                if(regmap[i][r]!=regmap_pre[i][r]) {
                                    temp_will_dirty&=~(1<<r);
                                    temp_wont_dirty&=~(1<<r);
                                    if((regmap_pre[i][r]&63)>0 && (regmap_pre[i][r]&63)<34) {
                                        temp_will_dirty|=((unneeded_reg[i]>>(regmap_pre[i][r]&63))&1)<<r;
                                        temp_wont_dirty|=((unneeded_reg[i]>>(regmap_pre[i][r]&63))&1)<<r;
                                    } else {
                                        temp_will_dirty|=1<<r;
                                        temp_wont_dirty|=1<<r;
                                    }
                                }
                            }
                        }
                    }
                    will_dirty[i]=temp_will_dirty;
                    wont_dirty[i]=temp_wont_dirty;
                    clean_registers((ba[i]-start)>>2,i-1,0);
                }
                /*else*/ if(1)
                {
                    if(itype[i]==RJUMP||itype[i]==UJUMP||(source[i]>>16)==0x1000)
                    {
                        // Unconditional branch
                        will_dirty_i=0;
                        wont_dirty_i=0;
                        //if(ba[i]>start+i*4) { // Disable recursion (for debugging)
                        for(r=0;r<HOST_REGS;r++) {
                            if(r!=9 && r!=EXCLUDE_REG) {
                                if(branch_regs[i].regmap[r]==regmap_entry[(ba[i]-start)>>2][r]) {
                                    will_dirty_i|=will_dirty[(ba[i]-start)>>2]&(1<<r);
                                    wont_dirty_i|=wont_dirty[(ba[i]-start)>>2]&(1<<r);
                                }
                            }
                        }
                        //}
                        // Merge in delay slot
                        for(r=0;r<HOST_REGS;r++) {
                            if(r!=9 && r!=EXCLUDE_REG) {
                                if((branch_regs[i].regmap[r]&63)==rt1[i]) will_dirty_i|=1<<r;
                                if((branch_regs[i].regmap[r]&63)==rt2[i]) will_dirty_i|=1<<r;
                                if((branch_regs[i].regmap[r]&63)==rt1[i+1]) will_dirty_i|=1<<r;
                                if((branch_regs[i].regmap[r]&63)==rt2[i+1]) will_dirty_i|=1<<r;
                                if((branch_regs[i].regmap[r]&63)>33) will_dirty_i&=~(1<<r);
                                if(branch_regs[i].regmap[r]<=0) will_dirty_i&=~(1<<r);
                                if(branch_regs[i].regmap[r]==CCREG) will_dirty_i|=1<<r;
                                if((regmap[i][r]&63)==rt1[i]) will_dirty_i|=1<<r;
                                if((regmap[i][r]&63)==rt2[i]) will_dirty_i|=1<<r;
                                if((regmap[i][r]&63)==rt1[i+1]) will_dirty_i|=1<<r;
                                if((regmap[i][r]&63)==rt2[i+1]) will_dirty_i|=1<<r;
                                if((regmap[i][r]&63)>33) will_dirty_i&=~(1<<r);
                                if(regmap[i][r]<=0) will_dirty_i&=~(1<<r);
                                if(regmap[i][r]==CCREG) will_dirty_i|=1<<r;
                            }
                        }
                    } else {
                        // Conditional branch
                        will_dirty_i=will_dirty_next;
                        wont_dirty_i=wont_dirty_next;
                        //if(ba[i]>start+i*4) { // Disable recursion (for debugging)
                        for(r=0;r<HOST_REGS;r++) {
                            if(r!=9 && r!=EXCLUDE_REG) {
                                if(branch_regs[i].regmap[r]==regmap_entry[(ba[i]-start)>>2][r]) {
                                    will_dirty_i&=will_dirty[(ba[i]-start)>>2]&(1<<r);
                                    wont_dirty_i|=wont_dirty[(ba[i]-start)>>2]&(1<<r);
                                }
                                else
                                {
                                    will_dirty_i&=~(1<<r);
                                }
                                // Treat delay slot as part of branch too
                                /*if(regmap[i+1][r]==regmap_entry[(ba[i]-start)>>2][r]) {
                                 will_dirty[i+1]&=will_dirty[(ba[i]-start)>>2]&(1<<r);
                                 wont_dirty[i+1]|=wont_dirty[(ba[i]-start)>>2]&(1<<r);
                                 }
                                 else
                                 {
                                 will_dirty[i+1]&=~(1<<r);
                                 }*/
                            }
                        }
                        //}
                        // Merge in delay slot
                        for(r=0;r<HOST_REGS;r++) {
                            if(r!=9 && r!=EXCLUDE_REG) {
                                if(!likely[i]) {
                                    // Might not dirty if likely branch is not taken
                                    if((branch_regs[i].regmap[r]&63)==rt1[i]) will_dirty_i|=1<<r;
                                    if((branch_regs[i].regmap[r]&63)==rt2[i]) will_dirty_i|=1<<r;
                                    if((branch_regs[i].regmap[r]&63)==rt1[i+1]) will_dirty_i|=1<<r;
                                    if((branch_regs[i].regmap[r]&63)==rt2[i+1]) will_dirty_i|=1<<r;
                                    if((branch_regs[i].regmap[r]&63)>33) will_dirty_i&=~(1<<r);
                                    if(branch_regs[i].regmap[r]<=0) will_dirty_i&=~(1<<r);
                                    if(branch_regs[i].regmap[r]==CCREG) will_dirty_i|=1<<r;
                                    //if((regmap[i][r]&63)==rt1[i]) will_dirty_i|=1<<r;
                                    //if((regmap[i][r]&63)==rt2[i]) will_dirty_i|=1<<r;
                                    if((regmap[i][r]&63)==rt1[i+1]) will_dirty_i|=1<<r;
                                    if((regmap[i][r]&63)==rt2[i+1]) will_dirty_i|=1<<r;
                                    if((regmap[i][r]&63)>33) will_dirty_i&=~(1<<r);
                                    if(regmap[i][r]<=0) will_dirty_i&=~(1<<r);
                                    if(regmap[i][r]==CCREG) will_dirty_i|=1<<r;
                                }
                            }
                        }
                    }
                    // Merge in delay slot
                    for(r=0;r<HOST_REGS;r++) {
                        if(r!=9 && r!=EXCLUDE_REG) {
                            if((regmap[i][r]&63)==rt1[i]) wont_dirty_i|=1<<r;
                            if((regmap[i][r]&63)==rt2[i]) wont_dirty_i|=1<<r;
                            if((regmap[i][r]&63)==rt1[i+1]) wont_dirty_i|=1<<r;
                            if((regmap[i][r]&63)==rt2[i+1]) wont_dirty_i|=1<<r;
                            if(regmap[i][r]==CCREG) wont_dirty_i|=1<<r;
                            if((branch_regs[i].regmap[r]&63)==rt1[i]) wont_dirty_i|=1<<r;
                            if((branch_regs[i].regmap[r]&63)==rt2[i]) wont_dirty_i|=1<<r;
                            if((branch_regs[i].regmap[r]&63)==rt1[i+1]) wont_dirty_i|=1<<r;
                            if((branch_regs[i].regmap[r]&63)==rt2[i+1]) wont_dirty_i|=1<<r;
                            if(branch_regs[i].regmap[r]==CCREG) wont_dirty_i|=1<<r;
                        }
                    }
                    if(wr) {
#ifndef DESTRUCTIVE_WRITEBACK
                        branch_regs[i].dirty&=wont_dirty_i;
#endif
                        branch_regs[i].dirty|=will_dirty_i;
                    }
                }
            }
        }
        else if(itype[i]==SYSCALL)
        {
            // SYSCALL instruction (software interrupt)
            will_dirty_i=0;
            wont_dirty_i=0;
        }
        else if(itype[i]==COP0 && (source[i]&0x3f)==0x18)
        {
            // ERET instruction (return from interrupt)
            will_dirty_i=0;
            wont_dirty_i=0;
        }
        will_dirty_next=will_dirty_i;
        wont_dirty_next=wont_dirty_i;
        if(itype[i]==COP1) {
            // MTC1/CTC1 treats dirty registers specially,
            // so don't set will_dirty
            will_dirty_i=0;
        }
        for(r=0;r<HOST_REGS;r++) {
            if(r!=9 && r!=EXCLUDE_REG) {
                if((regmap[i][r]&63)==rt1[i]) will_dirty_i|=1<<r;
                if((regmap[i][r]&63)==rt2[i]) will_dirty_i|=1<<r;
                if((regmap[i][r]&63)>33) will_dirty_i&=~(1<<r);
                if(regmap[i][r]<=0) will_dirty_i&=~(1<<r);
                if(regmap[i][r]==CCREG) will_dirty_i|=1<<r;
                if((regmap[i][r]&63)==rt1[i]) wont_dirty_i|=1<<r;
                if((regmap[i][r]&63)==rt2[i]) wont_dirty_i|=1<<r;
                if(regmap[i][r]==CCREG) wont_dirty_i|=1<<r;
                if(i>istart) {
                    if(itype[i]!=RJUMP&&itype[i]!=UJUMP&&itype[i]!=CJUMP&&itype[i]!=SJUMP&&itype[i]!=FJUMP)
                    {
                        // Don't store a register immediately after writing it,
                        // may prevent dual-issue.
                        if((regmap[i][r]&63)==rt1[i-1]) wont_dirty_i|=1<<r;
                        if((regmap[i][r]&63)==rt2[i-1]) wont_dirty_i|=1<<r;
                    }
                }
            }
        }
        // Save it
        will_dirty[i]=will_dirty_i;
        wont_dirty[i]=wont_dirty_i;
        // Mark registers that won't be dirtied as not dirty
        if(wr) {
            /*printf("wr (%d,%d) %x will:",istart,iend,start+i*4);
             for(r=0;r<HOST_REGS;r++) {
             if((will_dirty_i>>r)&1) {
             printf(" r%d",r);
             }
             }
             printf("\n");*/
            
            if(i==istart||(itype[i-1]!=RJUMP&&itype[i-1]!=UJUMP&&itype[i-1]!=CJUMP&&itype[i-1]!=SJUMP&&itype[i-1]!=FJUMP)) {
                dirty_post[i]|=will_dirty_i;
#ifndef DESTRUCTIVE_WRITEBACK
                dirty_post[i]&=wont_dirty_i;
                if(itype[i]==RJUMP||itype[i]==UJUMP||itype[i]==CJUMP||itype[i]==SJUMP||itype[i]==FJUMP)
                {
                    if(i<iend-1&&itype[i]!=RJUMP&&itype[i]!=UJUMP&&(source[i]>>16)!=0x1000) {
                        for(r=0;r<HOST_REGS;r++) {
                            if(r!=9 && r!=EXCLUDE_REG) {
                                if(regmap[i][r]==regmap_pre[i+2][r]) {
                                    dirty[i+2]&=wont_dirty_i|~(1<<r);
                                }else {/*printf("i: %x (%d) mismatch(+2): %d\n",start+i*4,i,r);/*assert(!((wont_dirty_i>>r)&1));*/}
                            }
                        }
                    }
                }
                else
                {
                    if(i<iend) {
                        for(r=0;r<HOST_REGS;r++) {
                            if(r!=9 && r!=EXCLUDE_REG) {
                                if(regmap[i][r]==regmap_pre[i+1][r]) {
                                    dirty[i+1]&=wont_dirty_i|~(1<<r);
                                }else {/*printf("i: %x (%d) mismatch(+1): %d\n",start+i*4,i,r);/*assert(!((wont_dirty_i>>r)&1));*/}
                            }
                        }
                    }
                }
#endif
            }
        }
        // Deal with changed mappings
        if(i<iend) {
            temp_will_dirty=will_dirty_i;
            temp_wont_dirty=wont_dirty_i;
            for(r=0;r<HOST_REGS;r++) {
                if(r!=9 && r!=EXCLUDE_REG) {
                    int nr;
                    if(regmap[i][r]==regmap_pre[i][r]) {
                        if(wr) {
#ifndef DESTRUCTIVE_WRITEBACK
                            dirty[i]&=wont_dirty_i|~(1<<r);
#endif
                            dirty[i]|=will_dirty_i&(1<<r);
                        }
                    }
                    else if((nr=get_reg(regmap[i],regmap_pre[i][r]))>=0) {
                        // Register moved to a different register
                        will_dirty_i&=~(1<<r);
                        wont_dirty_i&=~(1<<r);
                        will_dirty_i|=((temp_will_dirty>>nr)&1)<<r;
                        wont_dirty_i|=((temp_wont_dirty>>nr)&1)<<r;
                        if(wr) {
#ifndef DESTRUCTIVE_WRITEBACK
                            dirty[i]&=wont_dirty_i|~(1<<r);
#endif
                            dirty[i]|=will_dirty_i&(1<<r);
                        }
                    }
                    else {
                        will_dirty_i&=~(1<<r);
                        wont_dirty_i&=~(1<<r);
                        if((regmap_pre[i][r]&63)>0 && (regmap_pre[i][r]&63)<34) {
                            will_dirty_i|=((unneeded_reg[i]>>(regmap_pre[i][r]&63))&1)<<r;
                            wont_dirty_i|=((unneeded_reg[i]>>(regmap_pre[i][r]&63))&1)<<r;
                        } else {
                            wont_dirty_i|=1<<r;
                            /*printf("i: %x (%d) mismatch: %d\n",start+i*4,i,r);/*assert(!((will_dirty>>r)&1));*/
                        }
                    }
                }
            }
        }
    }
}

/* disassembly */
void disassemble_inst(int i)
{
    if (bt[i]) printf("*"); else printf(" ");
    switch(itype[i]) {
        case UJUMP:
            printf (" %x: %s %8x\n",start+i*4,insn[i],ba[i]);break;
        case CJUMP:
            printf (" %x: %s r%d,r%d,%8x\n",start+i*4,insn[i],rs1[i],rs2[i],ba[i]);break;
        case SJUMP:
            printf (" %x: %s r%d,%8x\n",start+i*4,insn[i],rs1[i],ba[i]);break;
        case FJUMP:
            printf (" %x: %s %8x\n",start+i*4,insn[i],ba[i]);break;
        case RJUMP:
            printf (" %x: %s r%d\n",start+i*4,insn[i],rs1[i]);break;
        case IMM16:
            if(opcode[i]==0xf) //LUI
                printf (" %x: %s r%d,%4x0000\n",start+i*4,insn[i],rt1[i],imm[i]&0xffff);
            else
                printf (" %x: %s r%d,r%d,%d\n",start+i*4,insn[i],rt1[i],rs1[i],imm[i]);
            break;
        case LOAD:
        case LOADLR:
            printf (" %x: %s r%d,r%d+%x\n",start+i*4,insn[i],rt1[i],rs1[i],imm[i]);
            break;
        case STORE:
        case STORELR:
            printf (" %x: %s r%d,r%d+%x\n",start+i*4,insn[i],rs2[i],rs1[i],imm[i]);
            break;
        case ALU:
        case SHIFT:
            printf (" %x: %s r%d,r%d,r%d\n",start+i*4,insn[i],rt1[i],rs1[i],rs2[i]);
            break;
        case MULTDIV:
            printf (" %x: %s r%d,r%d\n",start+i*4,insn[i],rs1[i],rs2[i]);
            break;
        case SHIFTIMM:
            printf (" %x: %s r%d,r%d,%d\n",start+i*4,insn[i],rt1[i],rs1[i],imm[i]);
            break;
        case MOV:
            if((opcode2[i]&0x1d)==0x10)
                printf (" %x: %s r%d\n",start+i*4,insn[i],rt1[i]);
            else if((opcode2[i]&0x1d)==0x11)
                printf (" %x: %s r%d\n",start+i*4,insn[i],rs1[i]);
            else
                printf (" %x: %s\n",start+i*4,insn[i]);
            break;
        case COP0:
            if(opcode2[i]==0)
                printf (" %x: %s r%d,cpr0[%d]\n",start+i*4,insn[i],rt1[i],(source[i]>>11)&0x1f); // MFC0
            else if(opcode2[i]==4)
                printf (" %x: %s r%d,cpr0[%d]\n",start+i*4,insn[i],rs1[i],(source[i]>>11)&0x1f); // MTC0
            else printf (" %x: %s\n",start+i*4,insn[i]);
            break;
        case COP1:
            if(opcode2[i]<3)
                printf (" %x: %s r%d,cpr1[%d]\n",start+i*4,insn[i],rt1[i],(source[i]>>11)&0x1f); // MFC1
            else if(opcode2[i]>3)
                printf (" %x: %s r%d,cpr1[%d]\n",start+i*4,insn[i],rs1[i],(source[i]>>11)&0x1f); // MTC1
            else printf (" %x: %s\n",start+i*4,insn[i]);
            break;
        case C1LS:
            printf (" %x: %s cpr1[%d],r%d+%x\n",start+i*4,insn[i],(source[i]>>16)&0x1f,rs1[i],imm[i]);
            break;
        default:
            //printf (" %s %8x\n",insn[i],source[i]);
            printf (" %x: %s\n",start+i*4,insn[i]);
    }
}

void new_dynarec_init()
{
    fprintf(stderr, "Init new dynarec\n");
#if defined(__APPLE__) && defined(__arm__)
    translation_cache_iphone = (unsigned char *)(((unsigned long) translation_cache_static_iphone + (4096)) & ~(4095));
    fprintf(stderr, "translation_cache_iphone 0x%x\n", translation_cache_iphone);
#endif
    
    out=(u_char *)BASE_ADDR;
    if (munmap ((void *)BASE_ADDR, 1<<TARGET_SIZE_2) < 0) {fprintf(stderr, "base addr munmap() failed\n"); fflush(stderr); }
    
    if (mmap (out, 1<<TARGET_SIZE_2,
              PROT_READ | PROT_WRITE | PROT_EXEC,
              MAP_FIXED | MAP_PRIVATE | MAP_ANON,
              -1, 0) != out) {fprintf(stderr, "mmap() failed\n"); fflush(stderr); }
#if defined(__APPLE__) && defined(__arm__)
    //memset(translation_cache_iphone, 0, 1<<TARGET_SIZE_2);
    sys_icache_invalidate(translation_cache_iphone, 1<<TARGET_SIZE_2);
    //fflush(stderr);
#endif
    
    rdword=&readmem_dword;
    fake_pc.f.r.rs=&readmem_dword;
    fake_pc.f.r.rt=&readmem_dword;
    fake_pc.f.r.rd=&readmem_dword;
    int n;
    for(n=0x80000;n<0x80800;n++)
        invalid_code[n]=1;
    for(n=0;n<65536;n++)
        hash_table[n][0]=hash_table[n][2]=-1;
    copy=shadow;
    expirep=16384; // Expiry pointer, +2 blocks
    pending_exception=0;
    literalcount=0;
#ifdef HOST_IMM8
    // Copy this into local area so we don't have to put it in every literal pool
    invc_ptr=invalid_code;
#endif
}

void new_dynarec_cleanup()
{
    int n;
    if (munmap ((void *)BASE_ADDR, 1<<TARGET_SIZE_2) < 0) {printf("munmap() failed\n");}
    for(n=0;n<=2048;n++) ll_clear(jump_in+n);
    for(n=0;n<=2048;n++) ll_clear(jump_out+n);
    for(n=0;n<=2048;n++) ll_clear(jump_dirty+n);
}

void new_recompile_block(int addr)
{
    //fprintf(stderr, "!!!!!!!!!!!!!!!!!!!  0x%x  !!!!!!!!!!!!!!!!!!!!!\n", addr); fflush(stderr);
    //if(Count==365117028) tracedebug=1;
    assem_debug("NOTCOMPILED: addr = %x -> %x\n", (int)addr, (int)out);
    //printf("TRACE: count=%d next=%d (compile %x)\n",Count,next_interupt,addr);
    //if(debug)
    //printf("TRACE: count=%d next=%d (checksum %x)\n",Count,next_interupt,mchecksum());
    /*if(Count>=312978186) {
     rlist();
     }*/
    //rlist();
    
    start = (u_int)addr;
    if ((u_int)addr >= 0xa4000000 && (u_int)addr < 0xa4001000)
    {
        //fprintf(stderr, "SP_DMEM 0x%x\n", SP_DMEM); fflush(stderr);
        source = (u_int *)(((u_int)SP_DMEM)+((u_int)addr-0xa4000000));
    }
    else if ((u_int)addr >= 0x80000000 && (u_int)addr < 0x80800000)
    {
        //fprintf(stderr, "rdram 0x%x\n", rdram); fflush(stderr);
        source = (u_int *)(((u_int)rdram)+((u_int)addr-0x80000000));
    }
    else
    {
        printf("Compile at bogus memory address: %x \n", (int)addr);
        exit(1);
    }
    
    /* Pass 1: disassemble */
    /* Pass 2: register dependencies, branch targets */
    /* Pass 3: register allocation */
    /* Pass 4: branch dependencies */
    /* Pass 5: pre-alloc */
    /* Pass 6: optimize clean/dirty state */
    /* Pass 7: flag 32-bit registers */
    /* Pass 8: assembly */
    /* Pass 9: linker */
    /* Pass 10: garbage collection / free memory */
    
    int i,j;
    int done=0;
    unsigned int type,op,op2;
    
    //fprintf(stderr, "addr = %x source = %x %x\n", addr,source,source[0]); fflush(stderr);
    
    /* Pass 1 diassembly */
    
    for(i=0;!done;i++)
    {
        bt[i]=0;likely[i]=0;op2=0;
        opcode[i]=op=source[i]>>26;
        switch(op)
        {
            case 0x00: strcpy(insn[i],"special"); type=NI;
                op2=source[i]&0x3f;
                switch(op2)
            {
                case 0x00: strcpy(insn[i],"SLL"); type=SHIFTIMM; break;
                case 0x02: strcpy(insn[i],"SRL"); type=SHIFTIMM; break;
                case 0x03: strcpy(insn[i],"SRA"); type=SHIFTIMM; break;
                case 0x04: strcpy(insn[i],"SLLV"); type=SHIFT; break;
                case 0x06: strcpy(insn[i],"SRLV"); type=SHIFT; break;
                case 0x07: strcpy(insn[i],"SRAV"); type=SHIFT; break;
                case 0x08: strcpy(insn[i],"JR"); type=RJUMP; break;
                case 0x09: strcpy(insn[i],"JALR"); type=RJUMP; break;
                case 0x0C: strcpy(insn[i],"SYSCALL"); type=SYSCALL; break;
                case 0x0D: strcpy(insn[i],"BREAK"); type=OTHER; break;
                case 0x0F: strcpy(insn[i],"SYNC"); type=OTHER; break;
                case 0x10: strcpy(insn[i],"MFHI"); type=MOV; break;
                case 0x11: strcpy(insn[i],"MTHI"); type=MOV; break;
                case 0x12: strcpy(insn[i],"MFLO"); type=MOV; break;
                case 0x13: strcpy(insn[i],"MTLO"); type=MOV; break;
                case 0x14: strcpy(insn[i],"DSLLV"); type=SHIFT; break;
                case 0x16: strcpy(insn[i],"DSRLV"); type=SHIFT; break;
                case 0x17: strcpy(insn[i],"DSRAV"); type=SHIFT; break;
                case 0x18: strcpy(insn[i],"MULT"); type=MULTDIV; break;
                case 0x19: strcpy(insn[i],"MULTU"); type=MULTDIV; break;
                case 0x1A: strcpy(insn[i],"DIV"); type=MULTDIV; break;
                case 0x1B: strcpy(insn[i],"DIVU"); type=MULTDIV; break;
                case 0x1C: strcpy(insn[i],"DMULT"); type=MULTDIV; break;
                case 0x1D: strcpy(insn[i],"DMULTU"); type=MULTDIV; break;
                case 0x1E: strcpy(insn[i],"DDIV"); type=MULTDIV; break;
                case 0x1F: strcpy(insn[i],"DDIVU"); type=MULTDIV; break;
                case 0x20: strcpy(insn[i],"ADD"); type=ALU; break;
                case 0x21: strcpy(insn[i],"ADDU"); type=ALU; break;
                case 0x22: strcpy(insn[i],"SUB"); type=ALU; break;
                case 0x23: strcpy(insn[i],"SUBU"); type=ALU; break;
                case 0x24: strcpy(insn[i],"AND"); type=ALU; break;
                case 0x25: strcpy(insn[i],"OR"); type=ALU; break;
                case 0x26: strcpy(insn[i],"XOR"); type=ALU; break;
                case 0x27: strcpy(insn[i],"NOR"); type=ALU; break;
                case 0x2A: strcpy(insn[i],"SLT"); type=ALU; break;
                case 0x2B: strcpy(insn[i],"SLTU"); type=ALU; break;
                case 0x2C: strcpy(insn[i],"DADD"); type=ALU; break;
                case 0x2D: strcpy(insn[i],"DADDU"); type=ALU; break;
                case 0x2E: strcpy(insn[i],"DSUB"); type=ALU; break;
                case 0x2F: strcpy(insn[i],"DSUBU"); type=ALU; break;
                case 0x30: strcpy(insn[i],"TGE"); type=NI; break;
                case 0x31: strcpy(insn[i],"TGEU"); type=NI; break;
                case 0x32: strcpy(insn[i],"TLT"); type=NI; break;
                case 0x33: strcpy(insn[i],"TLTU"); type=NI; break;
                case 0x34: strcpy(insn[i],"TEQ"); type=NI; break;
                case 0x36: strcpy(insn[i],"TNE"); type=NI; break;
                case 0x38: strcpy(insn[i],"DSLL"); type=SHIFTIMM; break;
                case 0x3A: strcpy(insn[i],"DSRL"); type=SHIFTIMM; break;
                case 0x3B: strcpy(insn[i],"DSRA"); type=SHIFTIMM; break;
                case 0x3C: strcpy(insn[i],"DSLL32"); type=SHIFTIMM; break;
                case 0x3E: strcpy(insn[i],"DSRL32"); type=SHIFTIMM; break;
                case 0x3F: strcpy(insn[i],"DSRA32"); type=SHIFTIMM; break;
            }
                break;
            case 0x01: strcpy(insn[i],"regimm"); type=NI;
                op2=(source[i]>>16)&0x1f;
                switch(op2)
            {
                case 0x00: strcpy(insn[i],"BLTZ"); type=SJUMP; break;
                case 0x01: strcpy(insn[i],"BGEZ"); type=SJUMP; break;
                case 0x02: strcpy(insn[i],"BLTZL"); type=SJUMP; break;
                case 0x03: strcpy(insn[i],"BGEZL"); type=SJUMP; break;
                case 0x08: strcpy(insn[i],"TGEI"); type=NI; break;
                case 0x09: strcpy(insn[i],"TGEIU"); type=NI; break;
                case 0x0A: strcpy(insn[i],"TLTI"); type=NI; break;
                case 0x0B: strcpy(insn[i],"TLTIU"); type=NI; break;
                case 0x0C: strcpy(insn[i],"TEQI"); type=NI; break;
                case 0x0E: strcpy(insn[i],"TNEI"); type=NI; break;
                case 0x10: strcpy(insn[i],"BLTZAL"); type=SJUMP; break;
                case 0x11: strcpy(insn[i],"BGEZAL"); type=SJUMP; break;
                case 0x12: strcpy(insn[i],"BLTZALL"); type=SJUMP; break;
                case 0x13: strcpy(insn[i],"BGEZALL"); type=SJUMP; break;
            }
                break;
            case 0x02: strcpy(insn[i],"J"); type=UJUMP; break;
            case 0x03: strcpy(insn[i],"JAL"); type=UJUMP; break;
            case 0x04: strcpy(insn[i],"BEQ"); type=CJUMP; break;
            case 0x05: strcpy(insn[i],"BNE"); type=CJUMP; break;
            case 0x06: strcpy(insn[i],"BLEZ"); type=CJUMP; break;
            case 0x07: strcpy(insn[i],"BGTZ"); type=CJUMP; break;
            case 0x08: strcpy(insn[i],"ADDI"); type=IMM16; break;
            case 0x09: strcpy(insn[i],"ADDIU"); type=IMM16; break;
            case 0x0A: strcpy(insn[i],"SLTI"); type=IMM16; break;
            case 0x0B: strcpy(insn[i],"SLTIU"); type=IMM16; break;
            case 0x0C: strcpy(insn[i],"ANDI"); type=IMM16; break;
            case 0x0D: strcpy(insn[i],"ORI"); type=IMM16; break;
            case 0x0E: strcpy(insn[i],"XORI"); type=IMM16; break;
            case 0x0F: strcpy(insn[i],"LUI"); type=IMM16; break;
            case 0x10: strcpy(insn[i],"cop0"); type=NI;
                op2=(source[i]>>21)&0x1f;
                switch(op2)
            {
                case 0x00: strcpy(insn[i],"MFC0"); type=COP0; break;
                case 0x04: strcpy(insn[i],"MTC0"); type=COP0; break;
                case 0x10: strcpy(insn[i],"tlb"); type=NI;
                    switch(source[i]&0x3f)
                {
                    case 0x01: strcpy(insn[i],"TLBR"); type=COP0; break;
                    case 0x02: strcpy(insn[i],"TLBWI"); type=COP0; break;
                    case 0x06: strcpy(insn[i],"TLBWR"); type=COP0; break;
                    case 0x08: strcpy(insn[i],"TLBP"); type=COP0; break;
                    case 0x18: strcpy(insn[i],"ERET"); type=COP0; break;
                }
            }
                break;
            case 0x11: strcpy(insn[i],"cop1"); type=NI;
                op2=(source[i]>>21)&0x1f;
                switch(op2)
            {
                case 0x00: strcpy(insn[i],"MFC1"); type=COP1; break;
                case 0x01: strcpy(insn[i],"DMFC1"); type=COP1; break;
                case 0x02: strcpy(insn[i],"CFC1"); type=COP1; break;
                case 0x04: strcpy(insn[i],"MTC1"); type=COP1; break;
                case 0x05: strcpy(insn[i],"DMTC1"); type=COP1; break;
                case 0x06: strcpy(insn[i],"CTC1"); type=COP1; break;
                case 0x08: strcpy(insn[i],"BC1"); type=FJUMP;
                    switch((source[i]>>16)&0x3)
                {
                    case 0x00: strcpy(insn[i],"BC1F"); break;
                    case 0x01: strcpy(insn[i],"BC1T"); break;
                    case 0x02: strcpy(insn[i],"BC1FL"); break;
                    case 0x03: strcpy(insn[i],"BC1TL"); break;
                }
                    break;
                case 0x10: strcpy(insn[i],"C1.S"); type=NI;
                    switch(source[i]&0x3f)
                {
                    case 0x00: strcpy(insn[i],"ADD.S"); type=FLOAT; break;
                    case 0x01: strcpy(insn[i],"SUB.S"); type=FLOAT; break;
                    case 0x02: strcpy(insn[i],"MUL.S"); type=FLOAT; break;
                    case 0x03: strcpy(insn[i],"DIV.S"); type=FLOAT; break;
                    case 0x04: strcpy(insn[i],"SQRT.S"); type=FLOAT; break;
                    case 0x05: strcpy(insn[i],"ABS.S"); type=FLOAT; break;
                    case 0x06: strcpy(insn[i],"MOV.S"); type=FLOAT; break;
                    case 0x07: strcpy(insn[i],"NEG.S"); type=FLOAT; break;
                    case 0x08: strcpy(insn[i],"ROUND.L.S"); type=FLOAT; break;
                    case 0x09: strcpy(insn[i],"TRUNC.L.S"); type=FLOAT; break;
                    case 0x0A: strcpy(insn[i],"CEIL.L.S"); type=FLOAT; break;
                    case 0x0B: strcpy(insn[i],"FLOOR.L.S"); type=FLOAT; break;
                    case 0x0C: strcpy(insn[i],"ROUND.W.S"); type=FLOAT; break;
                    case 0x0D: strcpy(insn[i],"TRUNC.W.S"); type=FLOAT; break;
                    case 0x0E: strcpy(insn[i],"CEIL.W.S"); type=FLOAT; break;
                    case 0x0F: strcpy(insn[i],"FLOOR.W.S"); type=FLOAT; break;
                    case 0x21: strcpy(insn[i],"CVT.D.S"); type=FLOAT; break;
                    case 0x24: strcpy(insn[i],"CVT.W.S"); type=FLOAT; break;
                    case 0x25: strcpy(insn[i],"CVT.L.S"); type=FLOAT; break;
                    case 0x30: strcpy(insn[i],"C.F.S"); type=FLOAT; break;
                    case 0x31: strcpy(insn[i],"C.UN.S"); type=FLOAT; break;
                    case 0x32: strcpy(insn[i],"C.EQ.S"); type=FLOAT; break;
                    case 0x33: strcpy(insn[i],"C.UEQ.S"); type=FLOAT; break;
                    case 0x34: strcpy(insn[i],"C.OLT.S"); type=FLOAT; break;
                    case 0x35: strcpy(insn[i],"C.ULT.S"); type=FLOAT; break;
                    case 0x36: strcpy(insn[i],"C.OLE.S"); type=FLOAT; break;
                    case 0x37: strcpy(insn[i],"C.ULE.S"); type=FLOAT; break;
                    case 0x38: strcpy(insn[i],"C.SF.S"); type=FLOAT; break;
                    case 0x39: strcpy(insn[i],"C.NGLE.S"); type=FLOAT; break;
                    case 0x3A: strcpy(insn[i],"C.SEQ.S"); type=FLOAT; break;
                    case 0x3B: strcpy(insn[i],"C.NGL.S"); type=FLOAT; break;
                    case 0x3C: strcpy(insn[i],"C.LT.S"); type=FLOAT; break;
                    case 0x3D: strcpy(insn[i],"C.NGE.S"); type=FLOAT; break;
                    case 0x3E: strcpy(insn[i],"C.LE.S"); type=FLOAT; break;
                    case 0x3F: strcpy(insn[i],"C.NGT.S"); type=FLOAT; break;
                }
                    break;
                case 0x11: strcpy(insn[i],"C1.D"); type=NI;
                    switch(source[i]&0x3f)
                {
                    case 0x00: strcpy(insn[i],"ADD.D"); type=FLOAT; break;
                    case 0x01: strcpy(insn[i],"SUB.D"); type=FLOAT; break;
                    case 0x02: strcpy(insn[i],"MUL.D"); type=FLOAT; break;
                    case 0x03: strcpy(insn[i],"DIV.D"); type=FLOAT; break;
                    case 0x04: strcpy(insn[i],"SQRT.D"); type=FLOAT; break;
                    case 0x05: strcpy(insn[i],"ABS.D"); type=FLOAT; break;
                    case 0x06: strcpy(insn[i],"MOV.D"); type=FLOAT; break;
                    case 0x07: strcpy(insn[i],"NEG.D"); type=FLOAT; break;
                    case 0x08: strcpy(insn[i],"ROUND.L.D"); type=FLOAT; break;
                    case 0x09: strcpy(insn[i],"TRUNC.L.D"); type=FLOAT; break;
                    case 0x0A: strcpy(insn[i],"CEIL.L.D"); type=FLOAT; break;
                    case 0x0B: strcpy(insn[i],"FLOOR.L.D"); type=FLOAT; break;
                    case 0x0C: strcpy(insn[i],"ROUND.W.D"); type=FLOAT; break;
                    case 0x0D: strcpy(insn[i],"TRUNC.W.D"); type=FLOAT; break;
                    case 0x0E: strcpy(insn[i],"CEIL.W.D"); type=FLOAT; break;
                    case 0x0F: strcpy(insn[i],"FLOOR.W.D"); type=FLOAT; break;
                    case 0x20: strcpy(insn[i],"CVT.S.D"); type=FLOAT; break;
                    case 0x24: strcpy(insn[i],"CVT.W.D"); type=FLOAT; break;
                    case 0x25: strcpy(insn[i],"CVT.L.D"); type=FLOAT; break;
                    case 0x30: strcpy(insn[i],"C.F.D"); type=FLOAT; break;
                    case 0x31: strcpy(insn[i],"C.UN.D"); type=FLOAT; break;
                    case 0x32: strcpy(insn[i],"C.EQ.D"); type=FLOAT; break;
                    case 0x33: strcpy(insn[i],"C.UEQ.D"); type=FLOAT; break;
                    case 0x34: strcpy(insn[i],"C.OLT.D"); type=FLOAT; break;
                    case 0x35: strcpy(insn[i],"C.ULT.D"); type=FLOAT; break;
                    case 0x36: strcpy(insn[i],"C.OLE.D"); type=FLOAT; break;
                    case 0x37: strcpy(insn[i],"C.ULE.D"); type=FLOAT; break;
                    case 0x38: strcpy(insn[i],"C.SF.D"); type=FLOAT; break;
                    case 0x39: strcpy(insn[i],"C.NGLE.D"); type=FLOAT; break;
                    case 0x3A: strcpy(insn[i],"C.SEQ.D"); type=FLOAT; break;
                    case 0x3B: strcpy(insn[i],"C.NGL.D"); type=FLOAT; break;
                    case 0x3C: strcpy(insn[i],"C.LT.D"); type=FLOAT; break;
                    case 0x3D: strcpy(insn[i],"C.NGE.D"); type=FLOAT; break;
                    case 0x3E: strcpy(insn[i],"C.LE.D"); type=FLOAT; break;
                    case 0x3F: strcpy(insn[i],"C.NGT.D"); type=FLOAT; break;
                }
                    break;
                case 0x14: strcpy(insn[i],"C1.W"); type=NI;
                    switch(source[i]&0x3f)
                {
                    case 0x20: strcpy(insn[i],"CVT.S.W"); type=FCONV; break;
                    case 0x21: strcpy(insn[i],"CVT.D.W"); type=FCONV; break;
                }
                    break;
                case 0x15: strcpy(insn[i],"C1.L"); type=NI;
                    switch(source[i]&0x3f)
                {
                    case 0x20: strcpy(insn[i],"CVT.S.L"); type=FCONV; break;
                    case 0x21: strcpy(insn[i],"CVT.D.L"); type=FCONV; break;
                }
                    break;
            }
                break;
            case 0x14: strcpy(insn[i],"BEQL"); type=CJUMP; break;
            case 0x15: strcpy(insn[i],"BNEL"); type=CJUMP; break;
            case 0x16: strcpy(insn[i],"BLEZL"); type=CJUMP; break;
            case 0x17: strcpy(insn[i],"BGTZL"); type=CJUMP; break;
            case 0x18: strcpy(insn[i],"DADDI"); type=IMM16; break;
            case 0x19: strcpy(insn[i],"DADDIU"); type=IMM16; break;
            case 0x1A: strcpy(insn[i],"LDL"); type=LOADLR; break;
            case 0x1B: strcpy(insn[i],"LDR"); type=LOADLR; break;
            case 0x20: strcpy(insn[i],"LB"); type=LOAD; break;
            case 0x21: strcpy(insn[i],"LH"); type=LOAD; break;
            case 0x22: strcpy(insn[i],"LWL"); type=LOADLR; break;
            case 0x23: strcpy(insn[i],"LW"); type=LOAD; break;
            case 0x24: strcpy(insn[i],"LBU"); type=LOAD; break;
            case 0x25: strcpy(insn[i],"LHU"); type=LOAD; break;
            case 0x26: strcpy(insn[i],"LWR"); type=LOADLR; break;
            case 0x27: strcpy(insn[i],"LWU"); type=LOAD; break;
            case 0x28: strcpy(insn[i],"SB"); type=STORE; break;
            case 0x29: strcpy(insn[i],"SH"); type=STORE; break;
            case 0x2A: strcpy(insn[i],"SWL"); type=STORELR; break;
            case 0x2B: strcpy(insn[i],"SW"); type=STORE; break;
            case 0x2C: strcpy(insn[i],"SDL"); type=STORELR; break;
            case 0x2D: strcpy(insn[i],"SDR"); type=STORELR; break;
            case 0x2E: strcpy(insn[i],"SWR"); type=STORELR; break;
            case 0x2F: strcpy(insn[i],"CACHE"); type=NOP; break;
            case 0x30: strcpy(insn[i],"LL"); type=NI; break;
            case 0x31: strcpy(insn[i],"LWC1"); type=C1LS; break;
            case 0x34: strcpy(insn[i],"LLD"); type=NI; break;
            case 0x35: strcpy(insn[i],"LDC1"); type=C1LS; break;
            case 0x37: strcpy(insn[i],"LD"); type=LOAD; break;
            case 0x38: strcpy(insn[i],"SC"); type=NI; break;
            case 0x39: strcpy(insn[i],"SWC1"); type=C1LS; break;
            case 0x3C: strcpy(insn[i],"SCD"); type=NI; break;
            case 0x3D: strcpy(insn[i],"SDC1"); type=C1LS; break;
            case 0x3F: strcpy(insn[i],"SD"); type=STORE; break;
            default: strcpy(insn[i],"???"); type=NI; break;
        }
        itype[i]=type;
        opcode2[i]=op2;
        /* Calculate branch target addresses */
        if(type==UJUMP)
            ba[i]=(((int)addr+i*4+4)&0xF0000000)|((signed int)((unsigned int)source[i]<<6)>>4);
        else if(type==CJUMP||type==SJUMP||type==FJUMP)
            ba[i]=(int)addr+i*4+4+((signed int)((unsigned int)source[i]<<16)>>14);
        else ba[i]=0;
        /* Get registers/immediates */
        us1[i]=0;
        us2[i]=0;
        dep1[i]=0;
        dep2[i]=0;
        switch(type) {
            case LOAD:
                rs1[i]=(source[i]>>21)&0x1f;
                rs2[i]=0;
                rt1[i]=(source[i]>>16)&0x1f;
                rt2[i]=0;
                imm[i]=(short)source[i];
                break;
            case STORE:
            case STORELR:
                rs1[i]=(source[i]>>21)&0x1f;
                rs2[i]=(source[i]>>16)&0x1f;
                rt1[i]=0;
                rt2[i]=0;
                imm[i]=(short)source[i];
                if(op==0x2c||op==0x2d||op==0x3f) us1[i]=rs2[i]; // 64-bit SDL/SDR/SD
                break;
            case LOADLR:
                // LWL/LWR only load part of the register,
                // therefore the target register must be treated as a source too
                rs1[i]=(source[i]>>21)&0x1f;
                rs2[i]=(source[i]>>16)&0x1f;
                rt1[i]=(source[i]>>16)&0x1f;
                rt2[i]=0;
                imm[i]=(short)source[i];
                if(op==0x1a||op==0x1b) us1[i]=rs2[i]; // LDR/LDL
                if(op==0x26) dep1[i]=rt1[i]; // LWR
                break;
            case IMM16:
                if (op==0x0f) rs1[i]=0; // LUI instruction has no source register
                else rs1[i]=(source[i]>>21)&0x1f;
                rs2[i]=0;
                rt1[i]=(source[i]>>16)&0x1f;
                rt2[i]=0;
                if(opcode[i]>=0x0c&&opcode[i]<=0x0e) { // ANDI/ORI/XORI
                    imm[i]=(unsigned short)source[i];
                }else{
                    imm[i]=(short)source[i];
                }
                if(op==0x18||op==0x19) us1[i]=rs1[i]; // DADDI/DADDIU
                if(op==0x0a||op==0x0b) us1[i]=rs1[i]; // SLTI/SLTIU
                if(op==0x0d||op==0x0e) dep1[i]=rs1[i]; // ORI/XORI
                break;
            case UJUMP:
                rs1[i]=0;
                rs2[i]=0;
                rt1[i]=0;
                rt2[i]=0;
                // The JAL instruction writes to r31.
                if (op&1) {
                    rt1[i]=31;
                }
                rs2[i]=CCREG;
                break;
            case RJUMP:
                rs1[i]=(source[i]>>21)&0x1f;
                rs2[i]=0;
                rt1[i]=0;
                rt2[i]=0;
                // The JALR instruction writes to r31.
                if (op2&1) {
                    rt1[i]=31;
                }
                rs2[i]=CCREG;
                break;
            case CJUMP:
                rs1[i]=(source[i]>>21)&0x1f;
                rs2[i]=(source[i]>>16)&0x1f;
                rt1[i]=0;
                rt2[i]=0;
                if(op&2) { // BGTZ/BLEZ
                    rs2[i]=0;
                }
                us1[i]=rs1[i];
                us2[i]=rs2[i];
                likely[i]=op>>4;
                break;
            case SJUMP:
                rs1[i]=(source[i]>>21)&0x1f;
                rs2[i]=CCREG;
                rt1[i]=0;
                rt2[i]=0;
                us1[i]=rs1[i];
                if(op2&0x10) { // BxxAL
                    rt1[i]=31;
                    // FIXME: If the branch is not taken, r31 is not overwritten
                }
                likely[i]=(op2&2)>>1;
                break;
            case FJUMP:
                rs1[i]=FSREG;
                rs2[i]=CSREG;
                rt1[i]=0;
                rt2[i]=0;
                likely[i]=((source[i])>>17)&1;
                break;
            case ALU:
                rs1[i]=(source[i]>>21)&0x1f; // source
                rs2[i]=(source[i]>>16)&0x1f; // subtract amount
                rt1[i]=(source[i]>>11)&0x1f; // destination
                rt2[i]=0;
                if(op2==0x2a||op2==0x2b) { // SLT/SLTU
                    us1[i]=rs1[i];us2[i]=rs2[i];
                }
                else if(op2>=0x24&&op2<=0x27) { // AND/OR/XOR/NOR
                    dep1[i]=rs1[i];dep2[i]=rs2[i];
                }
                else if(op2>=0x2c&&op2<=0x2f) { // DADD/DSUB
                    dep1[i]=rs1[i];dep2[i]=rs2[i];
                }
                break;
            case MULTDIV:
                rs1[i]=(source[i]>>21)&0x1f; // source
                rs2[i]=(source[i]>>16)&0x1f; // divisor
                rt1[i]=HIREG;
                rt2[i]=LOREG;
                if (op2>=0x1c&&op2<=0x1f) { // DMULT/DMULTU/DDIV/DDIVU
                    us1[i]=rs1[i];us2[i]=rs2[i];
                }
                break;
            case MOV:
                rs1[i]=0;
                rs2[i]=0;
                rt1[i]=0;
                rt2[i]=0;
                if(op2==0x10) rs1[i]=HIREG; // MFHI
                if(op2==0x11) rt1[i]=HIREG; // MTHI
                if(op2==0x12) rs1[i]=LOREG; // MFLO
                if(op2==0x13) rt1[i]=LOREG; // MTLO
                if((op2&0x1d)==0x10) rt1[i]=(source[i]>>11)&0x1f; // MFxx
                if((op2&0x1d)==0x11) rs1[i]=(source[i]>>21)&0x1f; // MTxx
                dep1[i]=rs1[i];
                break;
            case SHIFT:
                rs1[i]=(source[i]>>16)&0x1f; // target of shift
                rs2[i]=(source[i]>>21)&0x1f; // shift amount
                rt1[i]=(source[i]>>11)&0x1f; // destination
                rt2[i]=0;
                // DSLLV/DSRLV/DSRAV are 64-bit
                if(op2>=0x14&&op2<=0x17) us1[i]=rs1[i];
                break;
            case SHIFTIMM:
                rs1[i]=(source[i]>>16)&0x1f;
                rs2[i]=0;
                rt1[i]=(source[i]>>11)&0x1f;
                rt2[i]=0;
                imm[i]=(source[i]>>6)&0x1f;
                // DSxx32 instructions
                if(op2>=0x3c) imm[i]|=0x20;
                // DSLL/DSRL/DSRA/DSRA32/DSRL32 but not DSLL32 require 64-bit source
                if(op2>=0x38&&op2!=0x3c) us1[i]=rs1[i];
                break;
            case COP0:
                rs1[i]=0;
                rs2[i]=0;
                rt1[i]=0;
                rt2[i]=0;
                if(op2==0) rt1[i]=(source[i]>>16)&0x1F; // MFC0
                if(op2==4) rs1[i]=(source[i]>>16)&0x1F; // MTC0
                if(op2==4&&((source[i]>>11)&0x1f)==12) rt2[i]=CSREG; // Status
                if(op2==16) if((source[i]&0x3f)==0x18) rs2[i]=CCREG; // ERET
                break;
            case COP1:
                rs1[i]=0;
                rs2[i]=0;
                rt1[i]=0;
                rt2[i]=0;
                if(op2<3) rt1[i]=(source[i]>>16)&0x1F; // MFC1/DMFC1/CFC1
                if(op2>3) rs1[i]=(source[i]>>16)&0x1F; // MTC1/DMTC1/CTC1
                if(op2==5) us1[i]=rs1[i]; // DMTC1
                rs2[i]=CSREG;
                break;
            case C1LS:
                rs1[i]=(source[i]>>21)&0x1F;
                rs2[i]=CSREG;
                rt1[i]=0;
                rt2[i]=0;
                imm[i]=(short)source[i];
                break;
            case FLOAT:
            case FCONV:
                rs1[i]=0;
                rs2[i]=CSREG;
                rt1[i]=0;
                rt2[i]=0;
                break;
            case FCOMP:
                rs1[i]=0;
                rs2[i]=CSREG;
                rt1[i]=FSREG;
                rt2[i]=0;
                break;
            case SYSCALL:
                rs1[i]=CCREG;
                rs2[i]=0;
                rt1[i]=0;
                rt2[i]=0;
                break;
            default:
                rs1[i]=0;
                rs2[i]=0;
                rt1[i]=0;
                rt2[i]=0;
        }
        /* Is this the end of the block? */
        if(i>0&&(itype[i-1]==UJUMP||itype[i-1]==RJUMP||(source[i-1]>>16)==0x1000)) {
            if(rt1[i-1]!=31) { // Continue past subroutine call (JAL)
                done=1;
                // Does the block continue due to a branch?
                for(j=i-1;j>=0;j--)
                {
                    if(ba[j]==(u_int)addr+i*4+4) done=j=0;
                    if(ba[j]==(u_int)addr+i*4+8) done=j=0;
                }
            }
            // Don't recompile stuff that's already compiled
            if(check_addr((u_int)addr+i*4+4)) done=1;
            // Don't get too close to the limit
            if(i>MAXBLOCK-1024) done=1;
        }
        assert(i<MAXBLOCK-1);
        if (i==MAXBLOCK-1) done=1;
    }
    slen=i; // FIXME: what if last instruction is a jump?
    
    
    /* Pass 2 - Register dependencies and branch targets */
    
    unneeded_registers(0,slen-1);
    
    /* Pass 3 - Register allocation */
    
    struct regstat current; // Current register allocations/status
    current.is32=1;
    current.dirty=0;
    current.u=unneeded_reg[0];
    current.uu=unneeded_reg_upper[0];
    clear_all_regs(current.regmap);
    alloc_reg(&current,0,CCREG);
    dirty_reg(&current,CCREG);
    current.isconst=0;
    int ds=0;
    int cc=0;
    int hr;
    
    for(i=0;i<slen;i++)
    {
        if(bt[i])
        {
            int hr;
            for(hr=0;hr<HOST_REGS;hr++)
            {
                // Is this really necessary?
                if(hr!=9 && current.regmap[hr]==0) current.regmap[hr]=-1;
            }
            current.isconst=0;
        }
        if(i>1)
        {
            if((opcode[i-2]&0x2f)==0x05) // BNE/BNEL
            {
                if(rs1[i-2]==0||rs2[i-2]==0)
                {
                    if(rs1[i-2]) {
                        current.is32|=1LL<<rs1[i-2];
                        int hr=get_reg(current.regmap,rs1[i-2]|64);
                        if(hr>=0) current.regmap[hr]=-1;
                    }
                    if(rs2[i-2]) {
                        current.is32|=1LL<<rs2[i-2];
                        int hr=get_reg(current.regmap,rs2[i-2]|64);
                        if(hr>=0) current.regmap[hr]=-1;
                    }
                }
            }
        }
        // If something jumps here with 64-bit values
        // then promote those registers to 64 bits
        if(bt[i])
        {
            uint64_t temp_is32=current.is32;
            for(j=i-1;j>=0;j--)
            {
                if(ba[j]==(u_int)addr+i*4)
                    temp_is32&=branch_regs[j].is32;
            }
            for(j=i;j<slen;j++)
            {
                if(ba[j]==(u_int)addr+i*4)
                    temp_is32=1;
            }
            if(temp_is32!=current.is32) {
                //printf("dumping 32-bit regs (%x)\n",start+i*4);
#ifdef DESTRUCTIVE_WRITEBACK
                for(hr=0;hr<HOST_REGS;hr++)
                {
                    if(hr != 9)
                    {
                        int r=current.regmap[hr];
                        if(r>0&&r<64)
                        {
                            if((current.dirty>>hr)&((current.is32&~temp_is32)>>r)&1) {
                                temp_is32|=1LL<<r;
                                //printf("restore %d\n",r);
                            }
                        }
                    }
                }
#endif
                current.is32=temp_is32;
            }
        }
        memcpy(regmap_pre[i],current.regmap,sizeof(current.regmap));
        wasconst[i]=current.isconst;
        is32[i]=current.is32;
        dirty[i]=current.dirty;
#ifdef DESTRUCTIVE_WRITEBACK
        // To change a dirty register from 32 to 64 bits, we must write
        // it out during the previous cycle (for branches, 2 cycles)
        if(i<slen-1&&bt[i+1]&&itype[i-1]!=UJUMP&&itype[i-1]!=CJUMP&&itype[i-1]!=SJUMP&&itype[i-1]!=RJUMP&&itype[i-1]!=FJUMP)
        {
            uint64_t temp_is32=current.is32;
            for(j=i-1;j>=0;j--)
            {
                if(ba[j]==(u_int)addr+i*4+4)
                    temp_is32&=branch_regs[j].is32;
            }
            for(j=i;j<slen;j++)
            {
                if(ba[j]==(u_int)addr+i*4+4)
                    temp_is32=1;
            }
            if(temp_is32!=current.is32) {
                //printf("pre-dumping 32-bit regs (%x)\n",start+i*4);
                for(hr=0;hr<HOST_REGS;hr++)
                {
                    if(hr!=9)
                    {
                        int r=current.regmap[hr];
                        if(r>0)
                        {
                            if((current.dirty>>hr)&((current.is32&~temp_is32)>>(r&63))&1) {
                                if(itype[i]!=UJUMP&&itype[i]!=CJUMP&&itype[i]!=SJUMP&&itype[i]!=RJUMP&&itype[i]!=FJUMP)
                                {
                                    if(rs1[i]!=(r&63)&&rs2[i]!=(r&63))
                                    {
                                        //printf("dump %d/r%d\n",hr,r);
                                        current.regmap[hr]=-1;
                                        if(get_reg(current.regmap,r|64)>=0)
                                            current.regmap[get_reg(current.regmap,r|64)]=-1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else if(i<slen-2&&bt[i+2]&&(source[i-1]>>16)!=0x1000&&(itype[i]==CJUMP||itype[i]==SJUMP||itype[i]==FJUMP))
        {
            uint64_t temp_is32=current.is32;
            for(j=i-1;j>=0;j--)
            {
                if(ba[j]==(u_int)addr+i*4+8)
                    temp_is32&=branch_regs[j].is32;
            }
            for(j=i;j<slen;j++)
            {
                if(ba[j]==(u_int)addr+i*4+8)
                    temp_is32=1;
            }
            if(temp_is32!=current.is32) {
                //printf("pre-dumping 32-bit regs (%x)\n",start+i*4);
                for(hr=0;hr<HOST_REGS;hr++)
                {
                    if(hr!=9)
                    {
                        int r=current.regmap[hr];
                        if(r>0)
                        {
                            if((current.dirty>>hr)&((current.is32&~temp_is32)>>(r&63))&1) {
                                if(rs1[i]!=(r&63)&&rs2[i]!=(r&63)&&rs1[i+1]!=(r&63)&&rs2[i+1]!=(r&63))
                                {
                                    //printf("dump %d/r%d\n",hr,r);
                                    current.regmap[hr]=-1;
                                    if(get_reg(current.regmap,r|64)>=0)
                                        current.regmap[get_reg(current.regmap,r|64)]=-1;
                                }
                            }
                        }
                    }
                }
            }
        }
#endif
        if(itype[i]!=UJUMP&&itype[i]!=CJUMP&&itype[i]!=SJUMP&&itype[i]!=RJUMP&&itype[i]!=FJUMP) {
            if(i+1<slen) {
                current.u=unneeded_reg[i+1]&~((1LL<<rs1[i])|(1LL<<rs2[i]));
                current.uu=unneeded_reg_upper[i+1]&~((1LL<<us1[i])|(1LL<<us2[i]));
                if((~current.uu>>rt1[i])&1) current.uu&=~((1LL<<dep1[i])|(1LL<<dep2[i]));
                current.u|=1;
                current.uu|=1;
            } else {
                current.u=1;
                current.uu=1;
            }
        } else {
            if(i+1<slen) {
                current.u=branch_unneeded_reg[i]&~((1LL<<rs1[i])|(1LL<<rs2[i]));
                current.uu=branch_unneeded_reg_upper[i]&~((1LL<<us1[i])|(1LL<<us2[i]));
                if((~current.uu>>rt1[i+1])&1) current.uu&=~((1LL<<dep1[i+1])|(1LL<<dep2[i+1]));
                current.u&=~((1LL<<rs1[i+1])|(1LL<<rs2[i+1]));
                current.uu&=~((1LL<<us1[i+1])|(1LL<<us2[i+1]));
                current.u|=1;
                current.uu|=1;
            } else { printf("oops, branch at end of block with no delay slot\n");exit(1); }
        }
        if(ds) {
            ds=0; // Skip delay slot, already allocated as part of branch
            // ...but we need to alloc it in case something jumps here
            if(i+1<slen) {
                current.u=branch_unneeded_reg[i-1]&unneeded_reg[i+1];
                current.uu=branch_unneeded_reg_upper[i-1]&unneeded_reg_upper[i+1];
            }else{
                current.u=branch_unneeded_reg[i-1];
                current.uu=branch_unneeded_reg_upper[i-1];
            }
            current.u&=~((1LL<<rs1[i])|(1LL<<rs2[i]));
            current.uu&=~((1LL<<us1[i])|(1LL<<us2[i]));
            if((~current.uu>>rt1[i])&1) current.uu&=~((1LL<<dep1[i])|(1LL<<dep2[i]));
            current.u|=1;
            current.uu|=1;
            struct regstat temp;
            memcpy(&temp,&current,sizeof(current));
            // TODO: Take into account unconditional branches, as below
            delayslot_alloc(&temp,i);
            memcpy(regmap[i],temp.regmap,sizeof(current.regmap));
            // Create entry (branch target) regmap
            //memset(regmap_entry[i],0,sizeof(current.regmap));
            for(hr=0;hr<HOST_REGS;hr++)
            {
                if(hr!=9)
                {
                    int r=current.regmap[hr];
                    if(r>=0)
                    {
                        if(r!=regmap_pre[i][hr]) {
                            regmap_entry[i][hr]=-1;
                        }
                        else
                        {
                            if(r<64){
                                if((current.u>>r)&1) {
                                    regmap_entry[i][hr]=-1;
                                    regmap[i][hr]=-1;
                                    //Don't clear regs in the delay slot as the branch might need them
                                    //current.regmap[hr]=-1;
                                }else
                                    regmap_entry[i][hr]=r;
                            }
                            else {
                                if((current.uu>>(r&63))&1) {
                                    regmap_entry[i][hr]=-1;
                                    regmap[i][hr]=-1;
                                    //Don't clear regs in the delay slot as the branch might need them
                                    //current.regmap[hr]=-1;
                                }else
                                    regmap_entry[i][hr]=r;
                            }
                        }
                        
                    } else {
                        regmap_entry[i][hr]=-1;
                    }
                }
            }
        }
        else { // Not delay slot
            switch(itype[i]) {
                case UJUMP:
                    current.isconst=0;
                    alloc_cc(&current,i);
                    dirty_reg(&current,CCREG);
                    if (rt1[i]==31) {
                        alloc_reg(&current,i,31);
                        dirty_reg(&current,31);
                        assert(rs1[i+1]!=31&&rs2[i+1]!=31);
#ifdef REG_PREFETCH
                        alloc_reg(&current,i,PTEMP);
#endif
                    }
                    delayslot_alloc(&current,i+1);
                    ds=1;
                    break;
                case RJUMP:
                    current.isconst=0;
                    alloc_cc(&current,i);
                    dirty_reg(&current,CCREG);
                    if(rs1[i]!=rt1[i+1]&&rs1[i]!=rt2[i+1]) {
                        alloc_reg(&current,i,rs1[i]);
                        if (rt1[i]==31) {
                            alloc_reg(&current,i,31);
                            dirty_reg(&current,31);
                            assert(rs1[i+1]!=31&&rs2[i+1]!=31);
#ifdef REG_PREFETCH
                            alloc_reg(&current,i,PTEMP);
#endif
                        }
                        delayslot_alloc(&current,i+1);
                    } else {
                        // The delay slot overwrites our source reg,
                        // allocate a temporary register instead
                        // Note that such a sequence of instructions should
                        // be considered a bug since the branch can not be
                        // re-executed if an exception occurs.
                        delayslot_alloc(&current,i+1);
                        alloc_reg_temp(&current,i+1,(rt1[i]==31)?31:-1);
                        assert(rs1[i]!=rt1[i+1]&&rs1[i]!=rt2[i+1]); //FIXME
                    }
                    ds=1;
                    break;
                case CJUMP:
                    current.isconst=0;
                    if((opcode[i]&0x3E)==4) // BEQ/BNE
                    {
                        alloc_cc(&current,i);
                        dirty_reg(&current,CCREG);
                        if(rs1[i]) alloc_reg(&current,i,rs1[i]);
                        if(rs2[i]) alloc_reg(&current,i,rs2[i]);
                        if(!((current.is32>>rs1[i])&(current.is32>>rs2[i])&1))
                        {
                            if(rs1[i]) alloc_reg64(&current,i,rs1[i]);
                            if(rs2[i]) alloc_reg64(&current,i,rs2[i]);
                        }
                        //printf("branch(%d)preds: eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d\n",i,current.regmap[0],current.regmap[1],current.regmap[2],current.regmap[3],current.regmap[5],current.regmap[6],current.regmap[7]);
                        delayslot_alloc(&current,i+1);
                        //printf("branch(%d)postds: eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d\n",i,current.regmap[0],current.regmap[1],current.regmap[2],current.regmap[3],current.regmap[5],current.regmap[6],current.regmap[7]);
                        if((rs1[i]&&(rs1[i]==rt1[i+1]||rs1[i]==rt2[i+1]))||
                           (rs2[i]&&(rs2[i]==rt1[i+1]||rs2[i]==rt2[i+1]))) {
                            // The delay slot overwrites one of our conditions.
                            // Allocate the branch condition registers instead.
                            // Note that such a sequence of instructions could
                            // be considered a bug since the branch can not be
                            // re-executed if an exception occurs.
                            if(rs1[i]) alloc_reg(&current,i,rs1[i]);
                            if(rs2[i]) alloc_reg(&current,i,rs2[i]);
                            if(!((current.is32>>rs1[i])&(current.is32>>rs2[i])&1))
                            {
                                if(rs1[i]) alloc_reg64(&current,i,rs1[i]);
                                if(rs2[i]) alloc_reg64(&current,i,rs2[i]);
                            }
                        }
                    }
                    else
                        if((opcode[i]&0x3E)==6) // BLEZ/BGTZ
                        {
                            alloc_cc(&current,i);
                            dirty_reg(&current,CCREG);
                            alloc_reg(&current,i,rs1[i]);
                            if(!(current.is32>>rs1[i]&1))
                            {
                                alloc_reg64(&current,i,rs1[i]);
                            }
                            delayslot_alloc(&current,i+1);
                            if(rs1[i]&&(rs1[i]==rt1[i+1]||rs1[i]==rt2[i+1])) {
                                // The delay slot overwrites one of our conditions.
                                // Allocate the branch condition registers instead.
                                // Note that such a sequence of instructions could
                                // be considered a bug since the branch can not be
                                // re-executed if an exception occurs.
                                if(rs1[i]) alloc_reg(&current,i,rs1[i]);
                                if(!((current.is32>>rs1[i])&1))
                                {
                                    if(rs1[i]) alloc_reg64(&current,i,rs1[i]);
                                }
                            }
                        }
                        else
                            // Don't alloc the delay slot yet because we might not execute it
                            if((opcode[i]&0x3E)==0x14) // BEQL/BNEL
                            {
                                alloc_cc(&current,i);
                                dirty_reg(&current,CCREG);
                                alloc_reg(&current,i,rs1[i]);
                                alloc_reg(&current,i,rs2[i]);
                                if(!((current.is32>>rs1[i])&(current.is32>>rs2[i])&1))
                                {
                                    alloc_reg64(&current,i,rs1[i]);
                                    alloc_reg64(&current,i,rs2[i]);
                                }
                            }
                            else
                                if((opcode[i]&0x3E)==0x16) // BLEZL/BGTZL
                                {
                                    alloc_cc(&current,i);
                                    dirty_reg(&current,CCREG);
                                    alloc_reg(&current,i,rs1[i]);
                                    if(!(current.is32>>rs1[i]&1))
                                    {
                                        alloc_reg64(&current,i,rs1[i]);
                                    }
                                }
                    ds=1;
                    break;
                case SJUMP:
                    current.isconst=0;
                    //if((opcode2[i]&0x1E)==0x0) // BLTZ/BGEZ
                    if((opcode2[i]&0x0E)==0x0) // BLTZ/BGEZ
                    {
                        alloc_cc(&current,i);
                        dirty_reg(&current,CCREG);
                        alloc_reg(&current,i,rs1[i]);
                        if(!(current.is32>>rs1[i]&1))
                        {
                            alloc_reg64(&current,i,rs1[i]);
                        }
                        delayslot_alloc(&current,i+1);
                        if(rs1[i]&&(rs1[i]==rt1[i+1]||rs1[i]==rt2[i+1])) {
                            // The delay slot overwrites the branch condition.
                            // Allocate the branch condition registers instead.
                            // Note that such a sequence of instructions could
                            // be considered a bug since the branch can not be
                            // re-executed if an exception occurs.
                            if(rs1[i]) alloc_reg(&current,i,rs1[i]);
                            if(!((current.is32>>rs1[i])&1))
                            {
                                if(rs1[i]) alloc_reg64(&current,i,rs1[i]);
                            }
                        }
                    }
                    else
                        // Don't alloc the delay slot yet because we might not execute it
                        if((opcode2[i]&0x1E)==0x2) // BLTZL/BGEZL
                        {
                            alloc_cc(&current,i);
                            dirty_reg(&current,CCREG);
                            alloc_reg(&current,i,rs1[i]);
                            if(!(current.is32>>rs1[i]&1))
                            {
                                alloc_reg64(&current,i,rs1[i]);
                            }
                        }
                    ds=1;
                    break;
                case FJUMP:
                    current.isconst=0;
                    if(likely[i]==0) // BC1F/BC1T
                    {
                        // TODO: Theoretically we can run out of registers here on x86.
                        // The delay slot can allocate up to six, and we need to check
                        // CSREG before executing the delay slot.  Possibly we can drop
                        // the cycle count and then reload it after checking that the
                        // FPU is in a usable state, or don't do out-of-order execution.
                        alloc_cc(&current,i);
                        dirty_reg(&current,CCREG);
                        alloc_reg(&current,i,FSREG);
                        alloc_reg(&current,i,CSREG);
                        delayslot_alloc(&current,i+1);
                        // FIXME: Should be only FCOMP
                        if(itype[i+1]==FLOAT||itype[i+1]==FCOMP) {
                            // The delay slot overwrites the branch condition.
                            // Allocate the branch condition registers instead.
                            // Note that such a sequence of instructions could
                            // be considered a bug since the branch can not be
                            // re-executed if an exception occurs.
                            alloc_cc(&current,i);
                            dirty_reg(&current,CCREG);
                            alloc_reg(&current,i,CSREG);
                            alloc_reg(&current,i,FSREG);
                        }
                    }
                    else
                        // Don't alloc the delay slot yet because we might not execute it
                        if(likely[i]) // BC1FL/BC1TL
                        {
                            alloc_cc(&current,i);
                            dirty_reg(&current,CCREG);
                            alloc_reg(&current,i,CSREG);
                            alloc_reg(&current,i,FSREG);
                        }
                    ds=1;
                    break;
                case IMM16:
                    imm16_alloc(&current,i);
                    break;
                case LOAD:
                case LOADLR:
                    load_alloc(&current,i);
                    break;
                case STORE:
                case STORELR:
                    store_alloc(&current,i);
                    break;
                case ALU:
                    alu_alloc(&current,i);
                    break;
                case SHIFT:
                    shift_alloc(&current,i);
                    break;
                case MULTDIV:
                    multdiv_alloc(&current,i);
                    break;
                case SHIFTIMM:
                    shiftimm_alloc(&current,i);
                    break;
                case MOV:
                    mov_alloc(&current,i);
                    break;
                case COP0:
                    cop0_alloc(&current,i);
                    break;
                case COP1:
                    cop1_alloc(&current,i);
                    break;
                case C1LS:
                    c1ls_alloc(&current,i);
                    break;
                case FCONV:
                    fconv_alloc(&current,i);
                    break;
                case FLOAT:
                    float_alloc(&current,i);
                    break;
                case SYSCALL:
                    syscall_alloc(&current,i);
                    break;
            }
            memcpy(regmap[i],current.regmap,sizeof(current.regmap));
            // Create entry (branch target) regmap
            for(hr=0;hr<HOST_REGS;hr++)
            {
                if(hr!=9)
                {
                    int r,or,er;
                    r=current.regmap[hr];
                    if(r>=0) {
                        if(r!=regmap_pre[i][hr]) {
                            // TODO: delay slot (?)
                            or=get_reg(regmap_pre[i],r); // Get old mapping for this register
                            if(or<0){
                                regmap_entry[i][hr]=-1;
                            }
                            else
                            {
                                // Just move it to a different register
                                regmap_entry[i][hr]=r;
                                // If it was dirty before, it's still dirty
                                if((dirty[i]>>or)&1) dirty_reg(&current,r&63);
                            }
                        }
                        else
                        {
                            // Unneeded
                            if(r==0){
                                regmap_entry[i][hr]=0;
                            }
                            else
                                if(r<64){
                                    if((current.u>>r)&1) {
                                        regmap_entry[i][hr]=-1;
                                        regmap[i][hr]=-1;
                                        current.regmap[hr]=-1; // Is this correct?
                                    }else
                                        regmap_entry[i][hr]=r;
                                }
                                else {
                                    if((current.uu>>(r&63))&1) {
                                        regmap_entry[i][hr]=-1;
                                        regmap[i][hr]=-1;
                                        current.regmap[hr]=-1; // Is this correct?
                                    }else
                                        regmap_entry[i][hr]=r;
                                }
                        }
                    } else {
                        // Branches expect CCREG to be allocated at the target
                        if(regmap_pre[i][hr]==CCREG) 
                            regmap_entry[i][hr]=CCREG;
                        else
                            regmap_entry[i][hr]=-1;
                    }
                }
            }
        }
        /* Branch post-alloc */
        if(i>0)
        {
            switch(itype[i-1]) {
                case UJUMP:
                    memcpy(&branch_regs[i-1],&current,sizeof(current));
                    branch_regs[i-1].u=branch_unneeded_reg[i-1]&~((1LL<<rs1[i-1])|(1LL<<rs2[i-1]));
                    branch_regs[i-1].uu=branch_unneeded_reg_upper[i-1]&~((1LL<<us1[i-1])|(1LL<<us2[i-1]));
                    alloc_cc(&branch_regs[i-1],i-1);
                    dirty_reg(&branch_regs[i-1],CCREG);
                    if(rt1[i-1]==31) { // JAL
                        alloc_reg(&branch_regs[i-1],i-1,31);
                        dirty_reg(&branch_regs[i-1],31);
                        branch_regs[i-1].is32|=1LL<<31;
                    }
                    break;
                case RJUMP:
                    memcpy(&branch_regs[i-1],&current,sizeof(current));
                    branch_regs[i-1].u=branch_unneeded_reg[i-1]&~((1LL<<rs1[i-1])|(1LL<<rs2[i-1]));
                    branch_regs[i-1].uu=branch_unneeded_reg_upper[i-1]&~((1LL<<us1[i-1])|(1LL<<us2[i-1]));
                    alloc_cc(&branch_regs[i-1],i-1);
                    dirty_reg(&branch_regs[i-1],CCREG);
                    alloc_reg(&branch_regs[i-1],i-1,rs1[i-1]);
                    if(rt1[i-1]==31) { // JALR
                        alloc_reg(&branch_regs[i-1],i-1,31);
                        dirty_reg(&branch_regs[i-1],31);
                        branch_regs[i-1].is32|=1LL<<31;
                    }
                    break;
                case CJUMP:
                    if((opcode[i-1]&0x3E)==4) // BEQ/BNE
                    {
                        alloc_cc(&current,i-1);
                        dirty_reg(&current,CCREG);
                        if((rs1[i-1]&&(rs1[i-1]==rt1[i]||rs1[i-1]==rt2[i]))||
                           (rs2[i-1]&&(rs2[i-1]==rt1[i]||rs2[i-1]==rt2[i]))) {
                            // The delay slot overwrote one of our conditions
                            // Delay slot goes after the test (in order)
                            delayslot_alloc(&current,i);
                        }
                        else
                        {
                            current.u=branch_unneeded_reg[i-1]&~((1LL<<rs1[i-1])|(1LL<<rs2[i-1]));
                            current.uu=branch_unneeded_reg_upper[i-1]&~((1LL<<us1[i-1])|(1LL<<us2[i-1]));
                            // Alloc the branch condition registers
                            if(rs1[i-1]) alloc_reg(&current,i-1,rs1[i-1]);
                            if(rs2[i-1]) alloc_reg(&current,i-1,rs2[i-1]);
                            if(!((current.is32>>rs1[i-1])&(current.is32>>rs2[i-1])&1))
                            {
                                if(rs1[i-1]) alloc_reg64(&current,i-1,rs1[i-1]);
                                if(rs2[i-1]) alloc_reg64(&current,i-1,rs2[i-1]);
                            }
                        }
                        memcpy(&branch_regs[i-1],&current,sizeof(current));
                    }
                    else
                        if((opcode[i-1]&0x3E)==6) // BLEZ/BGTZ
                        {
                            alloc_cc(&current,i-1);
                            dirty_reg(&current,CCREG);
                            if(rs1[i-1]==rt1[i]||rs1[i-1]==rt2[i]) {
                                // The delay slot overwrote the branch condition
                                // Delay slot goes after the test (in order)
                                delayslot_alloc(&current,i);
                            }
                            else
                            {
                                current.u=branch_unneeded_reg[i-1]&~(1LL<<rs1[i-1]);
                                current.uu=branch_unneeded_reg_upper[i-1]&~(1LL<<us1[i-1]);
                                // Alloc the branch condition register
                                alloc_reg(&current,i-1,rs1[i-1]);
                                if(!(current.is32>>rs1[i-1]&1))
                                {
                                    alloc_reg64(&current,i-1,rs1[i-1]);
                                }
                            }
                            memcpy(&branch_regs[i-1],&current,sizeof(current));
                        }
                        else
                            // Alloc the delay slot in case the branch is taken
                            if((opcode[i-1]&0x3E)==0x14) // BEQL/BNEL
                            {
                                memcpy(&branch_regs[i-1],&current,sizeof(current));
                                branch_regs[i-1].u=(branch_unneeded_reg[i-1]&~((1LL<<rs1[i])|(1LL<<rs2[i])|(1LL<<rt1[i])|(1LL<<rt2[i])))|1;
                                branch_regs[i-1].uu=(branch_unneeded_reg_upper[i-1]&~((1LL<<us1[i])|(1LL<<us2[i])|(1LL<<rt1[i])|(1LL<<rt2[i])))|1;
                                if((~branch_regs[i-1].uu>>rt1[i])&1) branch_regs[i-1].uu&=~((1LL<<dep1[i])|(1LL<<dep2[i]))|1;
                                alloc_cc(&branch_regs[i-1],i);
                                dirty_reg(&branch_regs[i-1],CCREG);
                                delayslot_alloc(&branch_regs[i-1],i);
                                alloc_reg(&current,i,CCREG); // Not taken path
                                dirty_reg(&current,CCREG);
                            }
                            else
                                if((opcode[i-1]&0x3E)==0x16) // BLEZL/BGTZL
                                {
                                    memcpy(&branch_regs[i-1],&current,sizeof(current));
                                    branch_regs[i-1].u=(branch_unneeded_reg[i-1]&~((1LL<<rs1[i])|(1LL<<rs2[i])|(1LL<<rt1[i])|(1LL<<rt2[i])))|1;
                                    branch_regs[i-1].uu=(branch_unneeded_reg_upper[i-1]&~((1LL<<us1[i])|(1LL<<us2[i])|(1LL<<rt1[i])|(1LL<<rt2[i])))|1;
                                    if((~branch_regs[i-1].uu>>rt1[i])&1) branch_regs[i-1].uu&=~((1LL<<dep1[i])|(1LL<<dep2[i]))|1;
                                    alloc_cc(&branch_regs[i-1],i);
                                    dirty_reg(&branch_regs[i-1],CCREG);
                                    delayslot_alloc(&branch_regs[i-1],i);
                                    alloc_reg(&current,i,CCREG); // Not taken path
                                    dirty_reg(&current,CCREG);
                                }
                    break;
                case SJUMP:
                    //if((opcode2[i-1]&0x1E)==0) // BLTZ/BGEZ
                    if((opcode2[i-1]&0x0E)==0) // BLTZ/BGEZ
                    {
                        alloc_cc(&current,i-1);
                        dirty_reg(&current,CCREG);
                        if(rs1[i-1]==rt1[i]||rs1[i-1]==rt2[i]) {
                            // The delay slot overwrote the branch condition
                            // Delay slot goes after the test (in order)
                            delayslot_alloc(&current,i);
                        }
                        else
                        {
                            current.u=branch_unneeded_reg[i-1]&~(1LL<<rs1[i-1]);
                            current.uu=branch_unneeded_reg_upper[i-1]&~(1LL<<us1[i-1]);
                            // Alloc the branch condition register
                            alloc_reg(&current,i-1,rs1[i-1]);
                            if(!(current.is32>>rs1[i-1]&1))
                            {
                                alloc_reg64(&current,i-1,rs1[i-1]);
                            }
                        }
                        memcpy(&branch_regs[i-1],&current,sizeof(current));
                    }
                    else
                        // Alloc the delay slot in case the branch is taken
                        if((opcode2[i-1]&0x1E)==2) // BLTZL/BGEZL
                        {
                            memcpy(&branch_regs[i-1],&current,sizeof(current));
                            branch_regs[i-1].u=(branch_unneeded_reg[i-1]&~((1LL<<rs1[i])|(1LL<<rs2[i])|(1LL<<rt1[i])|(1LL<<rt2[i])))|1;
                            branch_regs[i-1].uu=(branch_unneeded_reg_upper[i-1]&~((1LL<<us1[i])|(1LL<<us2[i])|(1LL<<rt1[i])|(1LL<<rt2[i])))|1;
                            if((~branch_regs[i-1].uu>>rt1[i])&1) branch_regs[i-1].uu&=~((1LL<<dep1[i])|(1LL<<dep2[i]))|1;
                            alloc_cc(&branch_regs[i-1],i);
                            dirty_reg(&branch_regs[i-1],CCREG);
                            delayslot_alloc(&branch_regs[i-1],i);
                            alloc_reg(&current,i,CCREG); // Not taken path
                            dirty_reg(&current,CCREG);
                        }
                    // FIXME: BLTZAL/BGEZAL
                    if(opcode2[i-1]&0x10) { // BxxZAL
                        alloc_reg(&branch_regs[i-1],i-1,31);
                        dirty_reg(&branch_regs[i-1],31);
                        branch_regs[i-1].is32|=1LL<<31;
                    }
                    break;
                case FJUMP:
                    if(likely[i-1]==0) // BC1F/BC1T
                    {
                        alloc_cc(&current,i-1);
                        dirty_reg(&current,CCREG);
                        // FIXME: Should be only FCOMP
                        if(itype[i]==FLOAT||itype[i]==FCOMP) {
                            // The delay slot overwrote the branch condition
                            // Delay slot goes after the test (in order)
                            delayslot_alloc(&current,i);
                        }
                        else
                        {
                            current.u=branch_unneeded_reg[i-1]&~(1LL<<rs1[i-1]);
                            current.uu=branch_unneeded_reg_upper[i-1]&~(1LL<<us1[i-1]);
                            // Alloc the branch condition register
                            alloc_reg(&current,i-1,FSREG);
                        }
                        memcpy(&branch_regs[i-1],&current,sizeof(current));
                    }
                    else // BC1FL/BC1TL
                    {
                        // Alloc the delay slot in case the branch is taken
                        memcpy(&branch_regs[i-1],&current,sizeof(current));
                        branch_regs[i-1].u=(branch_unneeded_reg[i-1]&~((1LL<<rs1[i])|(1LL<<rs2[i])|(1LL<<rt1[i])|(1LL<<rt2[i])))|1;
                        branch_regs[i-1].uu=(branch_unneeded_reg_upper[i-1]&~((1LL<<us1[i])|(1LL<<us2[i])|(1LL<<rt1[i])|(1LL<<rt2[i])))|1;
                        if((~branch_regs[i-1].uu>>rt1[i])&1) branch_regs[i-1].uu&=~((1LL<<dep1[i])|(1LL<<dep2[i]))|1;
                        alloc_cc(&branch_regs[i-1],i);
                        dirty_reg(&branch_regs[i-1],CCREG);
                        delayslot_alloc(&branch_regs[i-1],i);
                        alloc_reg(&current,i,CCREG); // Not taken path
                        dirty_reg(&current,CCREG);
                    }
                    break;
            }
            
            if(itype[i-1]==UJUMP||itype[i-1]==RJUMP||(source[i-1]>>16)==0x1000)
            {
                if(rt1[i-1]==31) // JAL/JALR
                {
                    // Subroutine call will return here, don't alloc any registers
                    current.is32=1;
                    current.dirty=0;
                    clear_all_regs(current.regmap);
                    alloc_reg(&current,i,CCREG);
                    dirty_reg(&current,CCREG);
                }
                else if(i+1<slen)
                {
                    // Internal branch will jump here, match registers to caller
                    for(j=i-1;j>=0;j--)
                    {
                        if(ba[j]==(u_int)addr+i*4+4) {
                            memcpy(current.regmap,branch_regs[j].regmap,sizeof(current.regmap));
                            current.is32=branch_regs[j].is32;
                            current.dirty=branch_regs[j].dirty;
                            break;
                        }
                    }
                    while(j>=0) {
                        if(ba[j]==(u_int)addr+i*4+4) {
                            for(hr=0;hr<HOST_REGS;hr++) {
                                if(hr != 9)
                                {
                                    if(current.regmap[hr]!=branch_regs[j].regmap[hr]) {
                                        current.regmap[hr]=-1;
                                    }
                                    current.is32&=branch_regs[j].is32;
                                    current.dirty&=branch_regs[j].dirty;
                                }
                            }
                        }
                        j--;
                    }
                }
            }
        }
        
        // Count cycles in between branches
        ccadj[i]=cc;
        if(i>0&&(itype[i-1]==RJUMP||itype[i-1]==UJUMP||itype[i-1]==CJUMP||itype[i-1]==SJUMP||itype[i-1]==FJUMP||itype[i]==SYSCALL))
        {
            cc=0;
        }
        else
        {
            cc++;
        }
        
        flush_dirty_uppers(&current);
        is32_post[i]=current.is32;
        dirty_post[i]=current.dirty;
        isconst[i]=current.isconst;
        memcpy(constmap[i],current.constmap,sizeof(current.constmap));
        for(hr=0;hr<HOST_REGS;hr++) {
            if(hr!=9 && hr!=EXCLUDE_REG&&regmap[i][hr]>=0) {
                if(regmap_pre[i][hr]!=regmap[i][hr]) {
                    wasconst[i]&=~(1<<hr);
                }
            }
        }
    }
    
    /* Pass 4 - Cull unused host registers */
    
    uint64_t nr=0;
    
    for (i=slen-1;i>=0;i--)
    {
        int hr;
        if(itype[i]==RJUMP||itype[i]==UJUMP||itype[i]==CJUMP||itype[i]==SJUMP||itype[i]==FJUMP)
        {
            if(ba[i]<(unsigned int)addr || ba[i]>=((unsigned int)addr+slen*4))
            {
                // Branch out of this block, don't need anything
                nr=0;
            }
            else
            {
                // Internal branch
                // Need whatever matches the target
                nr=0;
                int t=(ba[i]-start)>>2;
                for(hr=0;hr<HOST_REGS;hr++)
                {
                    if(hr != 9)
                    {
                        if(regmap_entry[i][hr]>=0) {
                            if(regmap_entry[i][hr]==regmap_entry[t][hr]) nr|=1<<hr;
                        }
                    }
                }
            }
            // Conditional branch may need registers for following instructions
            if(itype[i]!=RJUMP&&itype[i]!=UJUMP&&(source[i]>>16)!=0x1000)
            {
                if(i<slen-2) {
                    nr|=needed_reg[i+2];
                    for(hr=0;hr<HOST_REGS;hr++)
                    {
                        if(hr != 9)
                        {
                            if(regmap_pre[i+2][hr]>=0&&get_reg(regmap_entry[i+2],regmap_pre[i+2][hr])<0) nr&=~(1<<hr);
                            //if((regmap_entry[i+2][hr])>=0) if(!((nr>>hr)&1)) printf("%x-bogus(%d=%d)\n",start+i*4,hr,regmap_entry[i+2][hr]);
                        }
                    }
                }
            }
            // Don't need stuff which is overwritten
            if(regmap[i][hr]!=regmap_pre[i][hr]) nr&=~(1<<hr);
            if(regmap[i][hr]<0) nr&=~(1<<hr);
            // Merge in delay slot
            for(hr=0;hr<HOST_REGS;hr++)
            {
                if(hr!=9)
                {
                    if(!likely[i]) {
                        // These are overwritten unless the branch is "likely"
                        // and the delay slot is nullified if not taken
                        if(rt1[i+1]&&rt1[i+1]==(regmap[i][hr]&63)) nr&=~(1<<hr);
                        if(rt2[i+1]&&rt2[i+1]==(regmap[i][hr]&63)) nr&=~(1<<hr);
                    }
                    if(us1[i+1]==(regmap_pre[i][hr]&63)) nr|=1<<hr;
                    if(us2[i+1]==(regmap_pre[i][hr]&63)) nr|=1<<hr;
                    if(rs1[i+1]==regmap_pre[i][hr]) nr|=1<<hr;
                    if(rs2[i+1]==regmap_pre[i][hr]) nr|=1<<hr;
                    if(us1[i+1]==(regmap_entry[i][hr]&63)) nr|=1<<hr;
                    if(us2[i+1]==(regmap_entry[i][hr]&63)) nr|=1<<hr;
                    if(rs1[i+1]==regmap_entry[i][hr]) nr|=1<<hr;
                    if(rs2[i+1]==regmap_entry[i][hr]) nr|=1<<hr;
                    if(dep1[i+1]&&!((unneeded_reg_upper[i]>>dep1[i+1])&1)) {
                        if(dep1[i+1]==(regmap_pre[i][hr]&63)) nr|=1<<hr;
                        if(dep2[i+1]==(regmap_pre[i][hr]&63)) nr|=1<<hr;
                    }
                    if(dep2[i+1]&&!((unneeded_reg_upper[i]>>dep2[i+1])&1)) {
                        if(dep1[i+1]==(regmap_entry[i][hr]&63)) nr|=1<<hr;
                        if(dep2[i+1]==(regmap_entry[i][hr]&63)) nr|=1<<hr;
                    }
                    //if(itype[i+1]==STORE || itype[i+1]==STORELR)
                    //  if(regmap_pre[i][hr]==rs2[i+1]) nr|=1<<hr;
                    if(itype[i+1]==STORE || itype[i+1]==STORELR || (opcode[i+1]&0x3b)==0x39) {
                        if(regmap_pre[i][hr]==INVCP) nr|=1<<hr;
                        if(regmap_entry[i][hr]==INVCP) nr|=1<<hr;
                    }
                }
            }
        }
        else if(itype[i]==SYSCALL)
        {
            // SYSCALL instruction (software interrupt)
            nr=0;
        }
        else if(itype[i]==COP0 && (source[i]&0x3f)==0x18)
        {
            // ERET instruction (return from interrupt)
            nr=0;
        }
        else // Non-branch
        {
            if(i<slen-1) {
                for(hr=0;hr<HOST_REGS;hr++) {
                    if(hr != 9)
                    {
                        if(regmap_pre[i+1][hr]>=0&&get_reg(regmap_entry[i+1],regmap_pre[i+1][hr])<0) nr&=~(1<<hr);
                        if(regmap[i][hr]!=regmap_pre[i][hr]) nr&=~(1<<hr);
                        if(regmap[i][hr]<0) nr&=~(1<<hr);
                    }
                }
            }
        }
        for(hr=0;hr<HOST_REGS;hr++)
        {
            if(hr!=9)
            {
                // Overwritten registers are not needed
                if(rt1[i]&&rt1[i]==(regmap[i][hr]&63)) nr&=~(1<<hr);
                if(rt2[i]&&rt2[i]==(regmap[i][hr]&63)) nr&=~(1<<hr);
                if(FTEMP==(regmap[i][hr]&63)) nr&=~(1<<hr);
                // Source registers are needed
                if(us1[i]==(regmap_pre[i][hr]&63)) nr|=1<<hr;
                if(us2[i]==(regmap_pre[i][hr]&63)) nr|=1<<hr;
                if(rs1[i]==regmap_pre[i][hr]) nr|=1<<hr;
                if(rs2[i]==regmap_pre[i][hr]) nr|=1<<hr;
                if(us1[i]==(regmap_entry[i][hr]&63)) nr|=1<<hr;
                if(us2[i]==(regmap_entry[i][hr]&63)) nr|=1<<hr;
                if(rs1[i]==regmap_entry[i][hr]) nr|=1<<hr;
                if(rs2[i]==regmap_entry[i][hr]) nr|=1<<hr;
                if(dep1[i]&&!((unneeded_reg_upper[i]>>dep1[i])&1)) {
                    if(dep1[i]==(regmap_pre[i][hr]&63)) nr|=1<<hr;
                    if(dep1[i]==(regmap_entry[i][hr]&63)) nr|=1<<hr;
                }
                if(dep2[i]&&!((unneeded_reg_upper[i]>>dep2[i])&1)) {
                    if(dep2[i]==(regmap_pre[i][hr]&63)) nr|=1<<hr;
                    if(dep2[i]==(regmap_entry[i][hr]&63)) nr|=1<<hr;
                }
                //if(itype[i]==STORE || itype[i]==STORELR) {
                //  if(regmap_pre[i][hr]==rs2[i]) nr|=1<<hr;
                if(itype[i]==STORE || itype[i]==STORELR || (opcode[i]&0x3b)==0x39) {
                    if(regmap_pre[i][hr]==INVCP) nr|=1<<hr;
                    if(regmap_entry[i][hr]==INVCP) nr|=1<<hr;
                }
                // Don't store a register immediately after writing it,
                // may prevent dual-issue.
                // But do so if this is a branch target, otherwise we
                // might have to load the register before the branch.
                if(i>0&&!bt[i]) {
                    if(rt1[i-1]==(regmap_pre[i][hr]&63)) nr|=1<<hr;
                    if(rt2[i-1]==(regmap_pre[i][hr]&63)) nr|=1<<hr;
                    if(rt1[i-1]==(regmap_entry[i][hr]&63)) nr|=1<<hr;
                    if(rt2[i-1]==(regmap_entry[i][hr]&63)) nr|=1<<hr;
                }
            }
        }
        // Assume cycle count is needed
        if(regmap_pre[i][HOST_CCREG]==CCREG) nr|=1<<HOST_CCREG;
        if(regmap_entry[i][HOST_CCREG]==CCREG) nr|=1<<HOST_CCREG;
        // Save it
        needed_reg[i]=nr;
        
        // Deallocate unneeded registers
        for(hr=0;hr<HOST_REGS;hr++)
        {
            if(hr!=9)
            {
                if(!((nr>>hr)&1)) {
                    if(regmap_entry[i][hr]!=CCREG) regmap_entry[i][hr]=-1;
                    if((regmap[i][hr]&63)!=rs1[i] && (regmap[i][hr]&63)!=rs2[i] &&
                       (regmap[i][hr]&63)!=rt1[i] && (regmap[i][hr]&63)!=rt2[i] &&
                       (regmap[i][hr]&63)!=FTEMP && (regmap[i][hr]&63)!=CCREG &&
                       (regmap[i][hr]&63)!=PTEMP ) // FIXME: FTEMP?
                    {
                        if(itype[i]!=RJUMP&&itype[i]!=UJUMP&&(source[i]>>16)!=0x1000)
                        {
                            if(likely[i]) {
                                regmap[i][hr]=-1;
                                if(i<slen-2) regmap_pre[i+2][hr]=-1;
                            }
                        }
                    }
                    if(itype[i]==RJUMP||itype[i]==UJUMP||itype[i]==CJUMP||itype[i]==SJUMP||itype[i]==FJUMP)
                    {
                        int d1=0,d2=0,inv=0;
                        if(get_reg(regmap[i],rt1[i+1]|64)>=0||get_reg(branch_regs[i].regmap,rt1[i+1]|64)>=0)
                        {
                            d1=dep1[i+1];
                            d2=dep2[i+1];
                        }
                        if(itype[i]==STORE || itype[i]==STORELR || (opcode[i]&0x3b)==0x39) {
                            inv=INVCP;
                        }
                        if(itype[i+1]==STORE || itype[i+1]==STORELR || (opcode[i+1]&0x3b)==0x39) {
                            inv=INVCP;
                        }
                        if((regmap[i][hr]&63)!=rs1[i] && (regmap[i][hr]&63)!=rs2[i] &&
                           (regmap[i][hr]&63)!=rt1[i] && (regmap[i][hr]&63)!=rt2[i] &&
                           (regmap[i][hr]&63)!=rt1[i+1] && (regmap[i][hr]&63)!=rt2[i+1] &&
                           (regmap[i][hr]^64)!=us1[i+1] && (regmap[i][hr]^64)!=us2[i+1] &&
                           (regmap[i][hr]^64)!=d1 && (regmap[i][hr]^64)!=d2 &&
                           regmap[i][hr]!=rs1[i+1] && regmap[i][hr]!=rs2[i+1] &&
                           (regmap[i][hr]&63)!=FTEMP && regmap[i][hr]!=PTEMP &&
                           regmap[i][hr]!=CCREG && regmap[i][hr]!=inv )
                        {
                            regmap[i][hr]=-1;
                            if((branch_regs[i].regmap[hr]&63)!=rs1[i] && (branch_regs[i].regmap[hr]&63)!=rs2[i] &&
                               (branch_regs[i].regmap[hr]&63)!=rt1[i] && (branch_regs[i].regmap[hr]&63)!=rt2[i] &&
                               (branch_regs[i].regmap[hr]&63)!=rt1[i+1] && (branch_regs[i].regmap[hr]&63)!=rt2[i+1] &&
                               (branch_regs[i].regmap[hr]^64)!=us1[i+1] && (branch_regs[i].regmap[hr]^64)!=us2[i+1] &&
                               (branch_regs[i].regmap[hr]^64)!=d1 && (branch_regs[i].regmap[hr]^64)!=d2 &&
                               branch_regs[i].regmap[hr]!=rs1[i+1] && branch_regs[i].regmap[hr]!=rs2[i+1] &&
                               (branch_regs[i].regmap[hr]&63)!=FTEMP && branch_regs[i].regmap[hr]!=PTEMP &&
                               branch_regs[i].regmap[hr]!=CCREG && branch_regs[i].regmap[hr]!=inv)
                            {
                                branch_regs[i].regmap[hr]=-1;
                                if(itype[i]!=RJUMP&&itype[i]!=UJUMP&&(source[i]>>16)!=0x1000)
                                {
                                    if(!likely[i]&&i<slen-2) {
                                        regmap_pre[i+2][hr]=-1;
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        // Non-branch
                        if(i>0&&itype[i-1]!=RJUMP&&itype[i-1]!=UJUMP&&itype[i-1]!=CJUMP&&itype[i-1]!=SJUMP&&itype[i-1]!=FJUMP)
                        {
                            int d1=0,d2=0,inv=0;
                            if(get_reg(regmap[i],rt1[i]|64)>=0)
                            {
                                d1=dep1[i];
                                d2=dep2[i];
                            }
                            if(itype[i]==STORE || itype[i]==STORELR || (opcode[i]&0x3b)==0x39) {
                                inv=INVCP;
                            }
                            if((regmap[i][hr]&63)!=rt1[i] && (regmap[i][hr]&63)!=rt2[i] &&
                               (regmap[i][hr]^64)!=us1[i] && (regmap[i][hr]^64)!=us2[i] &&
                               (regmap[i][hr]^64)!=d1 && (regmap[i][hr]^64)!=d2 &&
                               regmap[i][hr]!=rs1[i] && regmap[i][hr]!=rs2[i] &&
                               (regmap[i][hr]&63)!=FTEMP && regmap[i][hr]!=inv)
                            {
                                if(i<slen-1) {
                                    if(regmap_pre[i+1][hr]!=-1 || regmap[i][hr]!=-1)
                                        if(regmap_pre[i+1][hr]!=regmap[i][hr])
                                            if(regmap[i][hr]<64||!((is32[i]>>(regmap[i][hr]&63))&1))
                                            {
                                                printf("fail: %x (%d %d!=%d)\n",start+i*4,hr,regmap_pre[i+1][hr],regmap[i][hr]);
                                                assert(regmap_pre[i+1][hr]==regmap[i][hr]);
                                            }
                                    regmap_pre[i+1][hr]=-1;
                                }
                                regmap[i][hr]=-1;
                            }
                        }
                    }
                }
            }
        }
    }
    
    /* Pass 5 - Pre-allocate registers */
    
    // This allocates registers (if possible) one instruction prior
    // to use, which can avoid a load-use penalty on certain CPUs.
    for(i=0;i<slen-1;i++)
    {
        if(!i||(itype[i-1]!=UJUMP&&itype[i-1]!=CJUMP&&itype[i-1]!=SJUMP&&itype[i-1]!=RJUMP&&itype[i-1]!=FJUMP))
        {
            if(!bt[i+1])
            {
                if(itype[i]==ALU||itype[i]==MOV||itype[i]==LOAD||itype[i]==SHIFTIMM||itype[i]==IMM16)
                {
                    if(rs1[i+1]) {
                        if((hr=get_reg(regmap[i+1],rs1[i+1]))>=0)
                        {
                            if(regmap[i][hr]<0&&regmap_entry[i+1][hr]<0)
                            {
                                regmap[i][hr]=regmap[i+1][hr];
                                regmap_pre[i+1][hr]=regmap[i+1][hr];
                                regmap_entry[i+1][hr]=regmap[i+1][hr];
                                isconst[i]&=~(1<<hr);
                                isconst[i]|=isconst[i+1]&(1<<hr);
                                constmap[i][hr]=constmap[i+1][hr];
                                dirty[i+1]&=~(1<<hr);
                                dirty_post[i]&=~(1<<hr);
                            }
                        }
                    }
                    if(rs2[i+1]) {
                        if((hr=get_reg(regmap[i+1],rs2[i+1]))>=0)
                        {
                            if(regmap[i][hr]<0&&regmap_entry[i+1][hr]<0)
                            {
                                regmap[i][hr]=regmap[i+1][hr];
                                regmap_pre[i+1][hr]=regmap[i+1][hr];
                                regmap_entry[i+1][hr]=regmap[i+1][hr];
                                isconst[i]&=~(1<<hr);
                                isconst[i]|=isconst[i+1]&(1<<hr);
                                constmap[i][hr]=constmap[i+1][hr];
                                dirty[i+1]&=~(1<<hr);
                                dirty_post[i]&=~(1<<hr);
                            }
                        }
                    }
                }
            }
        }
    }
    
    /* Pass 6 - Optimize clean/dirty state */
    clean_registers(0,slen-1,1);
    
    /* Pass 7 - Identify 32-bit registers */
    
    u_int r32=0;
    
    for (i=slen-1;i>=0;i--)
    {
        int hr;
        if(itype[i]==RJUMP||itype[i]==UJUMP||itype[i]==CJUMP||itype[i]==SJUMP||itype[i]==FJUMP)
        {
            if(ba[i]<(unsigned int)addr || ba[i]>=((unsigned int)addr+slen*4))
            {
                // Branch out of this block, don't need anything
                r32=0;
            }
            else
            {
                // Internal branch
                // Need whatever matches the target
                // (and doesn't get overwritten by the delay slot instruction)
                r32=0;
                int t=(ba[i]-start)>>2;
                if(ba[i]>(unsigned int)addr+i*4) {
                    // Forward branch
                    if(!(requires_32bit[t]&~is32[i]))
                        r32|=requires_32bit[t]&(~(1LL<<rt1[i+1]))&(~(1LL<<rt2[i+1]));
                }else{
                    // Backward branch
                    if(!(is32[t]&~unneeded_reg_upper[t]&~is32[i]))
                        r32|=is32[t]&~unneeded_reg_upper[t]&(~(1LL<<rt1[i+1]))&(~(1LL<<rt2[i+1]));
                }
            }
            // Conditional branch may need registers for following instructions
            if(itype[i]!=RJUMP&&itype[i]!=UJUMP&&(source[i]>>16)!=0x1000)
            {
                if(i<slen-2) {
                    r32|=requires_32bit[i+2];
                    r32&=is32[i];
                    // Mark this address as a branch target since it may be called
                    // upon return from interrupt
                    bt[i+2]=1;
                }
            }
            // Merge in delay slot
            if(!likely[i]) {
                // These are overwritten unless the branch is "likely"
                // and the delay slot is nullified if not taken
                r32&=~(1LL<<rt1[i+1]);
                r32&=~(1LL<<rt2[i+1]);
            }
            // Assume these are needed (delay slot)
            if(us1[i+1]>0)
            {
                if((is32[i]>>us1[i+1])&1) r32|=1LL<<us1[i+1];
            }
            if(us2[i+1]>0)
            {
                if((is32[i]>>us2[i+1])&1) r32|=1LL<<us2[i+1];
            }
            if(dep1[i+1]&&!((unneeded_reg_upper[i]>>dep1[i+1])&1))
            {
                if((is32[i]>>dep1[i+1])&1) r32|=1LL<<dep1[i+1];
            }
            if(dep2[i+1]&&!((unneeded_reg_upper[i]>>dep2[i+1])&1))
            {
                if((is32[i]>>dep2[i+1])&1) r32|=1LL<<dep2[i+1];
            }
        }
        else if(itype[i]==SYSCALL)
        {
            // SYSCALL instruction (software interrupt)
            r32=0;
        }
        else if(itype[i]==COP0 && (source[i]&0x3f)==0x18)
        {
            // ERET instruction (return from interrupt)
            r32=0;
        }
        // Check 32 bits
        r32&=~(1LL<<rt1[i]);
        r32&=~(1LL<<rt2[i]);
        if(us1[i]>0)
        {
            if((is32[i]>>us1[i])&1) r32|=1LL<<us1[i];
        }
        if(us2[i]>0)
        {
            if((is32[i]>>us2[i])&1) r32|=1LL<<us2[i];
        }
        if(dep1[i]&&!((unneeded_reg_upper[i]>>dep1[i])&1))
        {
            if((is32[i]>>dep1[i])&1) r32|=1LL<<dep1[i];
        }
        if(dep2[i]&&!((unneeded_reg_upper[i]>>dep2[i])&1))
        {
            if((is32[i]>>dep2[i])&1) r32|=1LL<<dep2[i];
        }
        requires_32bit[i]=r32;
        
        // Dirty registers which are 32-bit, require 32-bit input
        // as they will be written as 32-bit values
        for(hr=0;hr<HOST_REGS;hr++)
        {
            if(hr!=9)
            {
                if(regmap_entry[i][hr]>0&&regmap_entry[i][hr]<64) {
                    if((is32[i]>>regmap_entry[i][hr])&(dirty[i]>>hr)&1) {
                        if(!((unneeded_reg_upper[i]>>regmap_entry[i][hr])&1))
                            requires_32bit[i]|=1LL<<regmap_entry[i][hr];
                    }
                }
            }
        }
        //requires_32bit[i]=is32[i]&~unneeded_reg_upper[i]; // DEBUG
    }
    
    /* Debug/disassembly */
    if((void*)assem_debug==(void*)printf) 
        for(i=0;i<slen;i++)
        {
            printf("U:");
            int r;
            for(r=1;r<=CCREG;r++) {
                if((unneeded_reg[i]>>r)&1) {
                    if(r==HIREG) printf(" HI");
                    else if(r==LOREG) printf(" LO");
                    else printf(" r%d",r);
                }
            }
            printf(" UU:");
            for(r=1;r<=CCREG;r++) {
                if(((unneeded_reg_upper[i]&~unneeded_reg[i])>>r)&1) {
                    if(r==HIREG) printf(" HI");
                    else if(r==LOREG) printf(" LO");
                    else printf(" r%d",r);
                }
            }
            printf(" 32:");
            for(r=0;r<=CCREG;r++) {
                //if(((is32[i]>>r)&(~unneeded_reg[i]>>r))&1) {
                if((is32[i]>>r)&1) {
                    if(r==CCREG) printf(" CC");
                    else if(r==HIREG) printf(" HI");
                    else if(r==LOREG) printf(" LO");
                    else printf(" r%d",r);
                }
            }
            printf("\n");
#if defined(__i386__) || defined(__x86_64__)
            printf("pre: eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d\n",regmap_pre[i][0],regmap_pre[i][1],regmap_pre[i][2],regmap_pre[i][3],regmap_pre[i][5],regmap_pre[i][6],regmap_pre[i][7]);
#endif
#ifdef __arm__
            printf("pre: r0=%d r1=%d r2=%d r3=%d r4=%d r5=%d r6=%d r7=%d r8=%d r9=%d r10=%d r12=%d\n",regmap_pre[i][0],regmap_pre[i][1],regmap_pre[i][2],regmap_pre[i][3],regmap_pre[i][4],regmap_pre[i][5],regmap_pre[i][6],regmap_pre[i][7],regmap_pre[i][8],regmap_pre[i][9],regmap_pre[i][10],regmap_pre[i][12]);
#endif
            printf("needs: ");
            if(needed_reg[i]&1) printf("eax ");
            if((needed_reg[i]>>1)&1) printf("ecx ");
            if((needed_reg[i]>>2)&1) printf("edx ");
            if((needed_reg[i]>>3)&1) printf("ebx ");
            if((needed_reg[i]>>5)&1) printf("ebp ");
            if((needed_reg[i]>>6)&1) printf("esi ");
            if((needed_reg[i]>>7)&1) printf("edi ");
            printf("r:");
            for(r=0;r<=CCREG;r++) {
                //if(((requires_32bit[i]>>r)&(~unneeded_reg[i]>>r))&1) {
                if((requires_32bit[i]>>r)&1) {
                    if(r==CCREG) printf(" CC");
                    else if(r==HIREG) printf(" HI");
                    else if(r==LOREG) printf(" LO");
                    else printf(" r%d",r);
                }
            }
            printf("\n");
#if defined(__i386__) || defined(__x86_64__)
            printf("entry: eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d\n",regmap_entry[i][0],regmap_entry[i][1],regmap_entry[i][2],regmap_entry[i][3],regmap_entry[i][5],regmap_entry[i][6],regmap_entry[i][7]);
            printf("dirty: ");
            if(dirty[i]&1) printf("eax ");
            if((dirty[i]>>1)&1) printf("ecx ");
            if((dirty[i]>>2)&1) printf("edx ");
            if((dirty[i]>>3)&1) printf("ebx ");
            if((dirty[i]>>5)&1) printf("ebp ");
            if((dirty[i]>>6)&1) printf("esi ");
            if((dirty[i]>>7)&1) printf("edi ");
#endif
#ifdef __arm__
            printf("entry: r0=%d r1=%d r2=%d r3=%d r4=%d r5=%d r6=%d r7=%d r8=%d r9=%d r10=%d r12=%d\n",regmap_entry[i][0],regmap_entry[i][1],regmap_entry[i][2],regmap_entry[i][3],regmap_entry[i][4],regmap_entry[i][5],regmap_entry[i][6],regmap_entry[i][7],regmap_entry[i][8],regmap_entry[i][9],regmap_entry[i][10],regmap_entry[i][12]);
            printf("dirty: ");
            if(dirty[i]&1) printf("r0 ");
            if((dirty[i]>>1)&1) printf("r1 ");
            if((dirty[i]>>2)&1) printf("r2 ");
            if((dirty[i]>>3)&1) printf("r3 ");
            if((dirty[i]>>4)&1) printf("r4 ");
            if((dirty[i]>>5)&1) printf("r5 ");
            if((dirty[i]>>6)&1) printf("r6 ");
            if((dirty[i]>>7)&1) printf("r7 ");
            if((dirty[i]>>8)&1) printf("r8 ");
            if((dirty[i]>>9)&1) printf("r9 ");
            if((dirty[i]>>10)&1) printf("r10 ");
            if((dirty[i]>>12)&1) printf("r12 ");
#endif
            printf("\n");
            disassemble_inst(i);
            //printf ("ccadj[%d] = %d\n",i,ccadj[i]);
#if defined(__i386__) || defined(__x86_64__)
            printf("eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d dirty: ",regmap[i][0],regmap[i][1],regmap[i][2],regmap[i][3],regmap[i][5],regmap[i][6],regmap[i][7]);
            if(dirty_post[i]&1) printf("eax ");
            if((dirty_post[i]>>1)&1) printf("ecx ");
            if((dirty_post[i]>>2)&1) printf("edx ");
            if((dirty_post[i]>>3)&1) printf("ebx ");
            if((dirty_post[i]>>5)&1) printf("ebp ");
            if((dirty_post[i]>>6)&1) printf("esi ");
            if((dirty_post[i]>>7)&1) printf("edi ");
#endif
#ifdef __arm__
            printf("r0=%d r1=%d r2=%d r3=%d r4=%d r5=%d r6=%d r7=%d r8=%d r9=%d r10=%d r12=%d dirty: ",regmap[i][0],regmap[i][1],regmap[i][2],regmap[i][3],regmap[i][4],regmap[i][5],regmap[i][6],regmap[i][7],regmap[i][8],regmap[i][9],regmap[i][10],regmap[i][12]);
            if(dirty_post[i]&1) printf("r0 ");
            if((dirty_post[i]>>1)&1) printf("r1 ");
            if((dirty_post[i]>>2)&1) printf("r2 ");
            if((dirty_post[i]>>3)&1) printf("r3 ");
            if((dirty_post[i]>>4)&1) printf("r4 ");
            if((dirty_post[i]>>5)&1) printf("r5 ");
            if((dirty_post[i]>>6)&1) printf("r6 ");
            if((dirty_post[i]>>7)&1) printf("r7 ");
            if((dirty_post[i]>>8)&1) printf("r8 ");
            if((dirty_post[i]>>9)&1) printf("r9 ");
            if((dirty_post[i]>>10)&1) printf("r10 ");
            if((dirty_post[i]>>12)&1) printf("r12 ");
#endif
            printf("\n");
            if(isconst[i]) {
                printf("constants: ");
                if(isconst[i]&1) printf("eax=%x ",(int)constmap[i][0]);
                if((isconst[i]>>1)&1) printf("ecx=%x ",(int)constmap[i][1]);
                if((isconst[i]>>2)&1) printf("edx=%x ",(int)constmap[i][2]);
                if((isconst[i]>>3)&1) printf("ebx=%x ",(int)constmap[i][3]);
                if((isconst[i]>>5)&1) printf("ebp=%x ",(int)constmap[i][5]);
                if((isconst[i]>>6)&1) printf("esi=%x ",(int)constmap[i][6]);
                if((isconst[i]>>7)&1) printf("edi=%x ",(int)constmap[i][7]);
                printf("\n");
            }
            if(itype[i]==RJUMP||itype[i]==UJUMP||itype[i]==CJUMP||itype[i]==SJUMP||itype[i]==FJUMP) {
                printf("branch(%d): eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d\n",i,branch_regs[i].regmap[0],branch_regs[i].regmap[1],branch_regs[i].regmap[2],branch_regs[i].regmap[3],branch_regs[i].regmap[5],branch_regs[i].regmap[6],branch_regs[i].regmap[7]);
                printf(" dirty: ");
#if defined(__i386__) || defined(__x86_64__)
                if(branch_regs[i].dirty&1) printf("eax ");
                if((branch_regs[i].dirty>>1)&1) printf("ecx ");
                if((branch_regs[i].dirty>>2)&1) printf("edx ");
                if((branch_regs[i].dirty>>3)&1) printf("ebx ");
                if((branch_regs[i].dirty>>5)&1) printf("ebp ");
                if((branch_regs[i].dirty>>6)&1) printf("esi ");
                if((branch_regs[i].dirty>>7)&1) printf("edi ");
#endif
#ifdef __arm__
                if(branch_regs[i].dirty&1) printf("r0 ");
                if((branch_regs[i].dirty>>1)&1) printf("r1 ");
                if((branch_regs[i].dirty>>2)&1) printf("r2 ");
                if((branch_regs[i].dirty>>3)&1) printf("r3 ");
                if((branch_regs[i].dirty>>4)&1) printf("r4 ");
                if((branch_regs[i].dirty>>5)&1) printf("r5 ");
                if((branch_regs[i].dirty>>6)&1) printf("r6 ");
                if((branch_regs[i].dirty>>7)&1) printf("r7 ");
                if((branch_regs[i].dirty>>8)&1) printf("r8 ");
                if((branch_regs[i].dirty>>9)&1) printf("r9 ");
                if((branch_regs[i].dirty>>10)&1) printf("r10 ");
                if((branch_regs[i].dirty>>12)&1) printf("r12 ");
#endif
                printf("\n");
            }
        }
    
    /* Pass 8 - Assembly */
    linkcount=0;stubcount=0;
    ds=0;is_delayslot=0;
    cop1_usable=0;
    uint64_t is32_pre=0;
    u_int dirty_pre=0;
    for(i=0;i<slen;i++)
    {
        //if(ds) printf("ds: ");
        if((void*)assem_debug==(void*)printf) disassemble_inst(i);
        if(ds) {
            ds=0; // Skip delay slot
            if(bt[i]) assem_debug("OOPS - branch into delay slot\n");
            instr_addr[i]=0;
        } 
        else 
        {
#ifndef DESTRUCTIVE_WRITEBACK
            wb_sx(regmap_pre[i],regmap_entry[i],dirty[i],is32_pre,is32[i],
                  unneeded_reg[i],unneeded_reg_upper[i]);
            wb_valid(regmap_pre[i],regmap_entry[i],dirty_pre,dirty[i],is32_pre,
                     unneeded_reg[i],unneeded_reg_upper[i]);
            is32_pre=is32_post[i];
            dirty_pre=dirty_post[i];
#endif
            // write back
            wb_invalidate(regmap_pre[i],regmap_entry[i],dirty[i],is32[i],
                          unneeded_reg[i],unneeded_reg_upper[i]);
            // branch target entry point
            instr_addr[i]=(u_int)out;
#if 0
            save_regs_debug(0x100f);
            emit_movimm(addr, 0);
            emit_call((int)testthat);
            restore_regs_debug(0x100f);
#endif
            // TEMP !!!!!!!!!!!!
            //      assem_debug("<-teststart>\n");
            //      emit_call((int)testthis);
            //      void (*recFunc)() = (void (*)()) (unsigned long)instr_addr[i];
            //    	(recFunc)();
            //      assem_debug("<-testend>\n");
            assem_debug("<->\n");
            // load regs
            if(regmap_entry[i][HOST_CCREG]==CCREG&&regmap[i][HOST_CCREG]!=CCREG)
                wb_register(CCREG,regmap_entry[i],dirty[i],is32[i]);
            load_consts(regmap_pre[i],regmap[i],is32[i],i);
            load_regs(regmap_entry[i],regmap[i],is32[i],rs1[i],rs2[i]);
            if(itype[i]==RJUMP||itype[i]==UJUMP||itype[i]==CJUMP||itype[i]==SJUMP||itype[i]==FJUMP)
            {
                // Load the delay slot registers if necessary
                if(rs1[i+1]!=rs1[i]&&rs1[i+1]!=rs2[i])
                    load_regs(regmap_entry[i],regmap[i],is32[i],rs1[i+1],rs1[i+1]);
                if(rs2[i+1]!=rs1[i+1]&&rs2[i+1]!=rs1[i]&&rs2[i+1]!=rs2[i])
                    load_regs(regmap_entry[i],regmap[i],is32[i],rs2[i+1],rs2[i+1]);
                if(itype[i+1]==STORE||itype[i+1]==STORELR||(opcode[i+1]&0x3b)==0x39)
                    load_regs(regmap_entry[i],regmap[i],is32[i],INVCP,INVCP);
            }
            else if(i+1<slen)
            {
                // Preload registers for following instruction
                if(rs1[i+1]!=rs1[i]&&rs1[i+1]!=rs2[i])
                    if(rs1[i+1]!=rt1[i]&&rs1[i+1]!=rt2[i])
                        load_regs(regmap_entry[i],regmap[i],is32[i],rs1[i+1],rs1[i+1]);
                if(rs2[i+1]!=rs1[i+1]&&rs2[i+1]!=rs1[i]&&rs2[i+1]!=rs2[i])
                    if(rs2[i+1]!=rt1[i]&&rs2[i+1]!=rt2[i])
                        load_regs(regmap_entry[i],regmap[i],is32[i],rs2[i+1],rs2[i+1]);
            }
            if(itype[i]==CJUMP||itype[i]==FJUMP)
                load_regs(regmap_entry[i],regmap[i],is32[i],CCREG,CCREG);
            if(itype[i]==STORE||itype[i]==STORELR||(opcode[i]&0x3b)==0x39)
                load_regs(regmap_entry[i],regmap[i],is32[i],INVCP,INVCP);
            if(bt[i]) cop1_usable=0;
#if 0
            save_regs_debug(0x100f);
            emit_movimm(itype[i], 0);
            emit_call((int)testthat);
            restore_regs_debug(0x100f);
            
            save_regs_debug(0x100f);
            emit_movimm(i, 0);
            emit_call((int)testthat);
            restore_regs_debug(0x100f);
#endif      
            // assemble
            switch(itype[i]) {
                case ALU:
                    alu_assemble(i,regmap[i]);break;
                case IMM16:
                    imm16_assemble(i,regmap[i]);break;
                case SHIFT:
                    shift_assemble(i,regmap[i]);break;
                case SHIFTIMM:
                    shiftimm_assemble(i,regmap[i]);break;
                case LOAD:
                    load_assemble(i,regmap[i]);break;
                case LOADLR:
                    loadlr_assemble(i,regmap[i]);break;
                case STORE:
                    store_assemble(i,regmap[i]);break;
                case STORELR:
                    storelr_assemble(i,regmap[i]);break;
                case COP0:
                    cop0_assemble(i,regmap[i]);break;
                case COP1:
                    cop1_assemble(i,regmap[i]);break;
                case C1LS:
                    c1ls_assemble(i,regmap[i]);break;
                case FCONV:
                    fconv_assemble(i,regmap[i]);break;
                case FLOAT:
                    float_assemble(i,regmap[i]);break;
                case MULTDIV:
                    multdiv_assemble(i,regmap[i]);break;
                case MOV:
                    mov_assemble(i,regmap[i]);break;
                case SYSCALL:
                    syscall_assemble(i,regmap[i]);break;
                case UJUMP:
                    ujump_assemble(i,regmap[i]);ds=1;break;
                case RJUMP:
                    rjump_assemble(i,regmap[i]);ds=1;break;
                case CJUMP:
                    cjump_assemble(i,regmap[i]);ds=1;break;
                case SJUMP:
                    sjump_assemble(i,regmap[i]);ds=1;break;
                case FJUMP:
                    fjump_assemble(i,regmap[i]);ds=1;break;
            }
#if 0
            save_regs_debug(0x100f);
            emit_movimm(0xFFFF0000, 0);
            emit_call((int)testthat);
            restore_regs_debug(0x100f);
#endif
            
            if(itype[i]==UJUMP||itype[i]==RJUMP||(source[i]>>16)==0x1000)
                literal_pool(1024);
            else
                literal_pool_jumpover(256);
        }
    }
    // TODO: delay slot stubs?
    // Stubs
    for(i=0;i<stubcount;i++)
    { 
        switch(stubs[i][0])
        {
            case LOADB_STUB:
            case LOADH_STUB:
            case LOADW_STUB:
            case LOADD_STUB:
            case LOADBU_STUB:
            case LOADHU_STUB:
                do_readstub(i);break;
            case STOREB_STUB:
            case STOREH_STUB:
            case STOREW_STUB:
            case STORED_STUB:
                do_writestub(i);break;
            case CC_STUB:
                do_ccstub(i);break;
            case INVCODE_STUB:
                do_invstub(i);break;
            case FP_STUB:
                do_cop1stub(i);break;
            case STORELR_STUB:
                do_unalignedwritestub(i);break;
        }
    }
    
    /* Pass 9 - Linker */
    for(i=0;i<linkcount;i++)
    {
        assem_debug("%8x -> %8x\n",link_addr[i][0],link_addr[i][1]);
        literal_pool(64);
        if(!link_addr[i][2])
        {
            void *stub=out;
            void *addr=check_addr(link_addr[i][1]);
            emit_extjump(link_addr[i][0],link_addr[i][1]);
            if(addr) {
                set_jump_target(link_addr[i][0],(int)addr);
                add_link(link_addr[i][1],stub);
            }
            else set_jump_target(link_addr[i][0],(int)stub);
        }
        else
        {
            // Internal branch
            int target=(link_addr[i][1]-start)>>2;
            assert(target>=0&&target<slen);
            assert(instr_addr[target]);
            set_jump_target(link_addr[i][0],instr_addr[target]);
        }
    }
    // External Branch Targets (jump_in)
    if(copy+slen*4>(void *)shadow+sizeof(shadow)) copy=shadow;
    for(i=0;i<slen;i++)
    {
        if(bt[i]||i==0)
        {
            if(instr_addr[i]) // TODO - delay slots (=null)
            {
                u_int page=(0x80000000^start+i*4)>>12;
                if(page>2048) page=2048;
                literal_pool(256);
                //if(!(is32[i]&(~unneeded_reg_upper[i])&~(1LL<<CCREG)))
                if(!requires_32bit[i])
                {
                    assem_debug("%8x (%d) <- %8x\n",instr_addr[i],i,start+i*4);
                    assem_debug("jump_in: %x\n",start+i*4);
                    ll_add(jump_dirty+page,start+i*4,(void *)out);
                    int entry_point=do_dirty_stub(i);
                    ll_add(jump_in+page,start+i*4,(void *)entry_point);
                }
                else
                {
                    u_int r=requires_32bit[i]|!!(requires_32bit[i]>>32);
                    assem_debug("%8x (%d) <- %8x\n",instr_addr[i],i,start+i*4);
                    assem_debug("jump_in: %x (restricted - %x)\n",start+i*4,r);
                    int entry_point=(int)out;
                    //assem_debug("entry_point: %x\n",entry_point);
                    load_regs_entry(i);
                    if(entry_point==(int)out)
                        entry_point=instr_addr[i];
                    else
                        emit_jmp(instr_addr[i]);
                    ll_add_32(jump_in+page,start+i*4,r,(void *)entry_point);
                }
            }
        }
    }
    // Write out the literal pool if necessary
    literal_pool(0);
    assert((u_int)out-*instr_addr<MAX_OUTPUT_BLOCK_SIZE);
    //printf("shadow buffer: %x-%x\n",(int)copy,(int)copy+slen*4);
    memcpy(copy,source,slen*4);
    copy+=slen*4;
    
    // Trap writes to any of the pages we compiled
    for(i=start>>12;i<=(start+slen*4)>>12;i++) invalid_code[i]=0;
    
#ifdef __arm__
    //fprintf(stderr, "out 0x%x, addr 0x%x, size 0x%x\n", (unsigned long)out, (void *)*instr_addr, (unsigned long)out - (unsigned long)(*instr_addr) ); fflush(stderr);
    sys_icache_invalidate((void *)*instr_addr, (unsigned long)out - (unsigned long)(*instr_addr));
    //__clear_cache((void *)*instr_addr,out);
    //fprintf(stderr, "out 0x%x, addr 0x%x, size 0x%x\n", (unsigned long)out, (void *)*instr_addr, (unsigned long)out - (unsigned long)(*instr_addr) ); fflush(stderr);
#endif
    
    // If we're within 256K of the end of the buffer,
    // start over from the beginning. (Is 256K enough?)
    if((int)out>BASE_ADDR+(1<<TARGET_SIZE_2)-MAX_OUTPUT_BLOCK_SIZE) out=(u_char *)BASE_ADDR;
    
    /* Pass 10 - Free memory by expiring oldest blocks */
    
    int end=((((int)out-BASE_ADDR)>>(TARGET_SIZE_2-16))+16384)&65535;
    while(expirep!=end)
    {
        int shift=TARGET_SIZE_2-3; // Divide into 8 blocks
        int base=BASE_ADDR+((expirep>>13)<<shift); // Base address of this block
        inv_debug("EXP: Phase %d\n",expirep);
        switch((expirep>>11)&3)
        {
            case 0:
                                
                if (testing >= 25) {
                    printf("Going to crash");
                    
                }
                                
                // Clear jump_in and jump_dirty
                ll_remove_matching_addrs(jump_in+(expirep&2047),base,shift);
                ll_remove_matching_addrs(jump_dirty+(expirep&2047),base,shift);
                if((expirep&2047)==2047) {
                    ll_remove_matching_addrs(jump_in+2048,base,shift);
                    ll_remove_matching_addrs(jump_dirty+2048,base,shift);
                }
                
                testing++;
                
                break;
            case 1:
                // Clear pointers
                ll_kill_pointers(jump_out[expirep&2047],base,shift);
                if((expirep&2047)==2047)
                    ll_kill_pointers(jump_out[2048],base,shift);
                break;
            case 2:
                // Clear hash table
                for(i=0;i<32;i++) {
                    int *ht_bin=hash_table[((expirep&2047)<<5)+i];
                    if((ht_bin[3]>>shift)==(base>>shift) ||
                       ((ht_bin[3]-MAX_OUTPUT_BLOCK_SIZE)>>shift)==(base>>shift)) {
                        inv_debug("EXP: Remove hash %x -> %x\n",ht_bin[2],ht_bin[3]);
                        ht_bin[2]=ht_bin[3]=-1;
                    }
                    if((ht_bin[1]>>shift)==(base>>shift) ||
                       ((ht_bin[1]-MAX_OUTPUT_BLOCK_SIZE)>>shift)==(base>>shift)) {
                        inv_debug("EXP: Remove hash %x -> %x\n",ht_bin[0],ht_bin[1]);
                        ht_bin[0]=ht_bin[2];
                        ht_bin[1]=ht_bin[3];
                    }
                }
                break;
            case 3:
                // Clear jump_out
#ifdef __arm__
                if((expirep&2047)==0)
                    sys_icache_invalidate((void*)BASE_ADDR,(1<<TARGET_SIZE_2));
                //          __clear_cache((void *)BASE_ADDR,(void *)BASE_ADDR+(1<<TARGET_SIZE_2));
#endif
                ll_remove_matching_addrs(jump_out+(expirep&2047),base,shift);
                if((expirep&2047)==2047)
                    ll_remove_matching_addrs(jump_out+2048,base,shift);
                break;
        }
     
        expirep=(expirep+1)&65535;
    }
    
    printf("End of loop");
    
}