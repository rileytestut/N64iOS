/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - assem_x86.c                                             *
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

int cycle_count;
int last_count;
int pcaddr;
int pending_exception;
uint64_t readmem_dword;
precomp_instr fake_pc,fake_pc_float;

// We need these for cmovcc instructions on x86
u_int const_zero=0;
u_int const_one=1;

/* Linker */

void kill_pointer(void *stub)
{
  int *i_ptr=*((int **)(stub+6));
  *i_ptr=(int)stub-(int)i_ptr-4;
}
int get_pointer(void *stub)
{
  int *i_ptr=*((int **)(stub+6));
  return *i_ptr+(int)i_ptr+4;
}

void set_jump_target(int addr,int target)
{
  u_char *ptr=(u_char *)addr;
  if(*ptr==0x0f)
  {
    assert(ptr[1]>=0x80&&ptr[1]<=0x8f);
    u_int *ptr2=(u_int *)(ptr+2);
    *ptr2=target-(int)ptr2-4;
  }
  else
  {
    assert(*ptr==0xe8||*ptr==0xe9);
    u_int *ptr2=(u_int *)(ptr+1);
    *ptr2=target-(int)ptr2-4;
  }
}

/* Register allocation */

// Note: registers are allocated clean (unmodified state)
// if you intend to modify the register, you must call dirty_reg().
void alloc_reg(struct regstat *cur,int i,char reg)
{
  int preferred_reg = (reg&3)+(reg>28)*4-(reg==32)+2*(reg==36);
  int r,hr;
  
  // Don't allocate unused registers
  if((cur->u>>reg)&1) return;
  
  // see if it's already allocated
  for(hr=0;hr<8;hr++)
  {
    if(cur->regmap[hr]==reg) return;
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
  
  // Try to allocate EAX, EBX, ECX, or EDX
  // We prefer these because they can do byte and halfword loads
  for(hr=0;hr<4;hr++) {
    if(cur->regmap[hr]==-1) {
      cur->regmap[hr]=reg;
      cur->dirty&=~(1<<hr);
      cur->isconst&=~(1<<hr);
      return;
    }
  }
  
  // Clear any unneeded registers
  // We try to keep the mapping consistent, if possible, because it
  // makes branches easier (especially loops).  So we try to allocate
  // first (see above) before removing old mappings.  If this is not
  // possible then go ahead and clear out the registers that are no
  // longer needed.
  for(hr=0;hr<8;hr++)
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
  // Try to allocate any available register
  for(hr=0;hr<8;hr++) {
    if(hr!=ESP&&cur->regmap[hr]==-1) {
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
  //printf("hsn(%x): %d %d %d %d %d %d %d\n",start+i*4,hsn[cur->regmap[0]&63],hsn[cur->regmap[1]&63],hsn[cur->regmap[2]&63],hsn[cur->regmap[3]&63],hsn[cur->regmap[5]&63],hsn[cur->regmap[6]&63],hsn[cur->regmap[7]&63]);
  for(j=10;j>=0;j--)
  {
    for(r=1;r<=MAXREG;r++)
    {
      if(hsn[r]==j) {
        for(hr=0;hr<8;hr++) {
          if(cur->regmap[hr]==r+64) {
            cur->regmap[hr]=reg;
            cur->dirty&=~(1<<hr);
            cur->isconst&=~(1<<hr);
            return;
          }
        }
        for(hr=0;hr<8;hr++) {
          if(cur->regmap[hr]==r) {
            cur->regmap[hr]=reg;
            cur->dirty&=~(1<<hr);
            cur->isconst&=~(1<<hr);
            return;
          }
        }
      }
    }
  }
  printf("This shouldn't happen (alloc_reg)");exit(1);
}

void alloc_reg64(struct regstat *cur,int i,char reg)
{
  int preferred_reg = 5+reg%3;
  int r,hr;
  
  // allocate the lower 32 bits
  alloc_reg(cur,i,reg);
  
  // Don't allocate unused registers
  if((cur->uu>>reg)&1) return;
  
  // see if the upper half is already allocated
  for(hr=0;hr<8;hr++)
  {
    if(cur->regmap[hr]==reg+64) return;
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
  
  // Try to allocate EBP, ESI or EDI
  for(hr=5;hr<8;hr++) {
    if(cur->regmap[hr]==-1) {
      cur->regmap[hr]=reg|64;
      cur->dirty&=~(1<<hr);
      cur->isconst&=~(1<<hr);
      return;
    }
  }
  
  // Clear any unneeded registers
  // We try to keep the mapping consistent, if possible, because it
  // makes branches easier (especially loops).  So we try to allocate
  // first (see above) before removing old mappings.  If this is not
  // possible then go ahead and clear out the registers that are no
  // longer needed.
  for(hr=7;hr>=0;hr--)
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
  // Try to allocate any available register
  for(hr=0;hr<8;hr++) {
    if(hr!=ESP&&cur->regmap[hr]==-1) {
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
        for(hr=0;hr<8;hr++) {
          if(cur->regmap[hr]==r+64) {
            cur->regmap[hr]=reg|64;
            cur->dirty&=~(1<<hr);
            cur->isconst&=~(1<<hr);
            return;
          }
        }
        for(hr=0;hr<8;hr++) {
          if(cur->regmap[hr]==r) {
            cur->regmap[hr]=reg|64;
            cur->dirty&=~(1<<hr);
            cur->isconst&=~(1<<hr);
            return;
          }
        }
      }
    }
  }
  printf("This shouldn't happen");exit(1);
}

// Allocate a temporary register.  This is done without regard to
// dirty status or whether the register we request is on the unneeded list
// Note: This will only allocate one register, even if called multiple times
void alloc_reg_temp(struct regstat *cur,int i,char reg)
{
  int r,hr;
  int preferred_reg = -1;
  
  // see if it's already allocated
  for(hr=0;hr<8;hr++)
  {
    if(hr!=ESP&&cur->regmap[hr]==reg) return;
  }
  
  // Try to allocate any available register
  // We prefer ESI, EDI, EBP since the others are used for byte/halfword stores
  for(hr=7;hr>=0;hr--) {
    if(hr!=ESP&&cur->regmap[hr]==-1) {
      cur->regmap[hr]=reg;
      cur->dirty&=~(1<<hr);
      cur->isconst&=~(1<<hr);
      return;
    }
  }
  
  // Find an unneeded register
  for(hr=7;hr>=0;hr--)
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
        for(hr=0;hr<8;hr++) {
          if(cur->regmap[hr]==r+64) {
            cur->regmap[hr]=reg;
            cur->dirty&=~(1<<hr);
            cur->isconst&=~(1<<hr);
            return;
          }
        }
        for(hr=0;hr<8;hr++) {
          if(cur->regmap[hr]==r) {
            cur->regmap[hr]=reg;
            cur->dirty&=~(1<<hr);
            cur->isconst&=~(1<<hr);
            return;
          }
        }
      }
    }
  }
  printf("This shouldn't happen");exit(1);
}
// Allocate a specific x86 register.
void alloc_x86_reg(struct regstat *cur,int i,char reg,char hr)
{
  int n;
  
  // see if it's already allocated (and dealloc it)
  for(n=0;n<8;n++)
  {
    if(n!=ESP&&cur->regmap[n]==reg) {cur->regmap[n]=-1;}
  }
  
  cur->regmap[hr]=reg;
  cur->dirty&=~(1<<hr);
  cur->isconst&=~(1<<hr);
}

// Alloc cycle count into dedicated register
alloc_cc(struct regstat *cur,int i)
{
  alloc_x86_reg(cur,i,CCREG,ESI);
}

/* Special alloc */

void multdiv_alloc_x86(struct regstat *current,int i)
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
      alloc_x86_reg(current,i,HIREG,EDX);
      alloc_x86_reg(current,i,LOREG,EAX);
      alloc_reg(current,i,rs1[i]);
      alloc_reg(current,i,rs2[i]);
      current->is32|=1LL<<HIREG;
      current->is32|=1LL<<LOREG;
      dirty_reg(current,HIREG);
      dirty_reg(current,LOREG);
    }
    else // 64-bit
    {
      alloc_x86_reg(current,i,HIREG|64,EDX);
      alloc_x86_reg(current,i,HIREG,EAX);
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
#define multdiv_alloc multdiv_alloc_x86

/* Assembler */

char regname[8][4] = {
 "eax",
 "ecx",
 "edx",
 "ebx",
 "esp",
 "ebp",
 "esi",
 "edi"};

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

void emit_mov(int rs,int rt)
{
  assem_debug("mov %%%s,%%%s\n",regname[rs],regname[rt]);
  output_byte(0x89);
  output_modrm(3,rt,rs);
}

void emit_add(int rs1,int rs2,int rt)
{
  if(rs1==rt) {
    assem_debug("add %%%s,%%%s\n",regname[rs2],regname[rs1]);
    output_byte(0x01);
    output_modrm(3,rs1,rs2);
  }else if(rs2==rt) {
    assem_debug("add %%%s,%%%s\n",regname[rs1],regname[rs2]);
    output_byte(0x01);
    output_modrm(3,rs2,rs1);
  }else {
    assem_debug("lea (%%%s,%%%s),%%%s\n",regname[rs1],regname[rs2],regname[rt]);
    output_byte(0x8D);
    if(rs1!=EBP) {
      output_modrm(0,4,rt);
      output_sib(0,rs2,rs1);
    }else{
      assert(rs2!=EBP);
      output_modrm(0,4,rt);
      output_sib(0,rs1,rs2);
    }
  }
}

void emit_adds(int rs1,int rs2,int rt)
{
  emit_add(rs1,rs2,rt);
}

void emit_lea8(int rs1,int rt)
{
  assem_debug("lea 0(%%%s,8),%%%s\n",regname[rs1],regname[rt]);
  output_byte(0x8D);
  output_modrm(0,4,rt);
  output_sib(3,rs1,5);
  output_w32(0);
}

void emit_neg(int rs, int rt)
{
  if(rs!=rt) emit_mov(rs,rt);
  assem_debug("neg %%%s\n",regname[rt]);
  output_byte(0xF7);
  output_modrm(3,rt,3);
}

void emit_negs(int rs, int rt)
{
  emit_neg(rs,rt);
}

void emit_sub(int rs1,int rs2,int rt)
{
  if(rs1==rt) {
    assem_debug("sub %%%s,%%%s\n",regname[rs2],regname[rs1]);
    output_byte(0x29);
    output_modrm(3,rs1,rs2);
  } else if(rs2==rt) {
    emit_neg(rs2,rs2);
    emit_add(rs2,rs1,rs2);
  } else {
    emit_mov(rs1,rt);
    emit_sub(rt,rs2,rt);
  }
}

void emit_subs(int rs1,int rs2,int rt)
{
  emit_sub(rs1,rs2,rt);
}

void emit_zeroreg(int rt)
{
  output_byte(0x31);
  output_modrm(3,rt,rt);
  assem_debug("xor %%%s,%%%s\n",regname[rt],regname[rt]);
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
    assem_debug("mov %x+%d,%%%s\n",addr,r,regname[hr]);
    output_byte(0x8B);
    output_modrm(0,5,hr);
    output_w32(addr);
  }
}
void emit_storereg(int r, int hr)
{
  int addr=((int)reg)+((r&63)<<3)+((r&64)>>4);
  if((r&63)==HIREG) addr=(int)&hi+((r&64)>>4);
  if((r&63)==LOREG) addr=(int)&lo+((r&64)>>4);
  if(r==CCREG) addr=(int)&cycle_count;
  assem_debug("mov %%%s,%x+%d\n",regname[hr],addr,r);
  output_byte(0x89);
  output_modrm(0,5,hr);
  output_w32(addr);
}

void emit_test(int rs, int rt)
{
  assem_debug("test %%%s,%%%s\n",regname[rs],regname[rt]);
  output_byte(0x85);
  output_modrm(3,rs,rt);
}

void emit_testimm(int rs,int imm)
{
  assem_debug("test $0x%x,%%%s\n",imm,regname[rs]);
  if(imm<128&&imm>=-128&&rs<4) {
    output_byte(0xF6);
    output_modrm(3,rs,0);
    output_byte(imm);
  }
  else
  {
    output_byte(0xF7);
    output_modrm(3,rs,0);
    output_w32(imm);
  }
}

void emit_not(int rs,int rt)
{
  if(rs!=rt) emit_mov(rs,rt);
  assem_debug("not %%%s\n",regname[rt]);
  output_byte(0xF7);
  output_modrm(3,rt,2);
}

void emit_and(u_int rs1,u_int rs2,u_int rt)
{
  assert(rs1<8);
  assert(rs2<8);
  assert(rt<8);
  if(rs1==rt) {
    assem_debug("and %%%s,%%%s\n",regname[rs2],regname[rt]);
    output_byte(0x21);
    output_modrm(3,rs1,rs2);
  }
  else
  if(rs2==rt) {
    assem_debug("and %%%s,%%%s\n",regname[rs1],regname[rt]);
    output_byte(0x21);
    output_modrm(3,rs2,rs1);
  }
  else {
    emit_mov(rs1,rt);
    emit_and(rt,rs2,rt);
  }
}

void emit_or(u_int rs1,u_int rs2,u_int rt)
{
  assert(rs1<8);
  assert(rs2<8);
  assert(rt<8);
  if(rs1==rt) {
    assem_debug("or %%%s,%%%s\n",regname[rs2],regname[rt]);
    output_byte(0x09);
    output_modrm(3,rs1,rs2);
  }
  else
  if(rs2==rt) {
    assem_debug("or %%%s,%%%s\n",regname[rs1],regname[rt]);
    output_byte(0x09);
    output_modrm(3,rs2,rs1);
  }
  else {
    emit_mov(rs1,rt);
    emit_or(rt,rs2,rt);
  }
}
void emit_or_and_set_flags(int rs1,int rs2,int rt)
{
  emit_or(rs1,rs2,rt);
}

void emit_xor(u_int rs1,u_int rs2,u_int rt)
{
  assert(rs1<8);
  assert(rs2<8);
  assert(rt<8);
  if(rs1==rt) {
    assem_debug("xor %%%s,%%%s\n",regname[rs2],regname[rt]);
    output_byte(0x31);
    output_modrm(3,rs1,rs2);
  }
  else
  if(rs2==rt) {
    assem_debug("xor %%%s,%%%s\n",regname[rs1],regname[rt]);
    output_byte(0x31);
    output_modrm(3,rs2,rs1);
  }
  else {
    emit_mov(rs1,rt);
    emit_xor(rt,rs2,rt);
  }
}

void emit_movimm(int imm,u_int rt)
{
  assem_debug("mov $%d,%%%s\n",imm,regname[rt]);
  assert(rt<8);
  output_byte(0xB8+rt);
  output_w32(imm);
}

void emit_addimm(int rs,int imm,int rt)
{
  if(rs==rt) {
    if(imm!=0) {
      assem_debug("add $%d,%%%s\n",imm,regname[rt]);
      if(imm<128&&imm>=-128) {
        output_byte(0x83);
        output_modrm(3,rt,0);
        output_byte(imm);
      }
      else
      {
        output_byte(0x81);
        output_modrm(3,rt,0);
        output_w32(imm);
      }
    }
  }
  else {
    if(imm!=0) {
      assem_debug("lea %d(%%%s),%%%s\n",imm,regname[rs],regname[rt]);
      output_byte(0x8D);
      if(imm<128&&imm>=-128) {
        output_modrm(1,rs,rt);
        output_byte(imm);
      }else{
        output_modrm(2,rs,rt);
        output_w32(imm);
      }
    }else{
      emit_mov(rs,rt);
    }
  }
}

void emit_addimm_and_set_flags(int imm,int rt)
{
  assem_debug("add $%d,%%%s\n",imm,regname[rt]);
  if(imm<128&&imm>=-128) {
    output_byte(0x83);
    output_modrm(3,rt,0);
    output_byte(imm);
  }
  else
  {
    output_byte(0x81);
    output_modrm(3,rt,0);
    output_w32(imm);
  }
}
void emit_addimm_no_flags(int imm,int rt)
{
  if(imm!=0) {
    assem_debug("lea %d(%%%s),%%%s\n",imm,regname[rt],regname[rt]);
    output_byte(0x8D);
    if(imm<128&&imm>=-128) {
      output_modrm(1,rt,rt);
      output_byte(imm);
    }else{
      output_modrm(2,rt,rt);
      output_w32(imm);
    }
  }
}

void emit_adcimm(int imm,u_int rt)
{
  assem_debug("adc $%d,%%%s\n",imm,regname[rt]);
  assert(rt<8);
  if(imm<128&&imm>=-128) {
    output_byte(0x83);
    output_modrm(3,rt,2);
    output_byte(imm);
  }
  else
  {
    output_byte(0x81);
    output_modrm(3,rt,2);
    output_w32(imm);
  }
}
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
  if(rs==rt) {
    assem_debug("and $%d,%%%s\n",imm,regname[rt]);
    if(imm<128&&imm>=-128) {
      output_byte(0x83);
      output_modrm(3,rt,4);
      output_byte(imm);
    }
    else
    {
      output_byte(0x81);
      output_modrm(3,rt,4);
      output_w32(imm);
    }
  }
  else {
    emit_mov(rs,rt);
    emit_andimm(rt,imm,rt);
  }
}

void emit_orimm(int rs,int imm,int rt)
{
  if(rs==rt) {
    assem_debug("or $%d,%%%s\n",imm,regname[rt]);
    if(imm<128&&imm>=-128) {
      output_byte(0x83);
      output_modrm(3,rt,1);
      output_byte(imm);
    }
    else
    {
      output_byte(0x81);
      output_modrm(3,rt,1);
      output_w32(imm);
    }
  }
  else {
    emit_mov(rs,rt);
    emit_orimm(rt,imm,rt);
  }
}

void emit_xorimm(int rs,int imm,int rt)
{
  if(rs==rt) {
    assem_debug("xor $%d,%%%s\n",imm,regname[rt]);
    if(imm<128&&imm>=-128) {
      output_byte(0x83);
      output_modrm(3,rt,6);
      output_byte(imm);
    }
    else
    {
      output_byte(0x81);
      output_modrm(3,rt,6);
      output_w32(imm);
    }
  }
  else {
    emit_mov(rs,rt);
    emit_xorimm(rt,imm,rt);
  }
}

void emit_shlimm(int rs,u_int imm,int rt)
{
  if(rs==rt) {
    assem_debug("shl %%%s,%d\n",regname[rt],imm);
    assert(imm>0);
    if(imm==1) output_byte(0xD1);
    else output_byte(0xC1);
    output_modrm(3,rt,4);
    if(imm>1) output_byte(imm);
  }
  else {
    emit_mov(rs,rt);
    emit_shlimm(rt,imm,rt);
  }
}

void emit_shrimm(int rs,u_int imm,int rt)
{
  if(rs==rt) {
    assem_debug("shr %%%s,%d\n",regname[rt],imm);
    assert(imm>0);
    if(imm==1) output_byte(0xD1);
    else output_byte(0xC1);
    output_modrm(3,rt,5);
    if(imm>1) output_byte(imm);
  }
  else {
    emit_mov(rs,rt);
    emit_shrimm(rt,imm,rt);
  }
}

void emit_sarimm(int rs,u_int imm,int rt)
{
  if(rs==rt) {
    assem_debug("sar %%%s,%d\n",regname[rt],imm);
    assert(imm>0);
    if(imm==1) output_byte(0xD1);
    else output_byte(0xC1);
    output_modrm(3,rt,7);
    if(imm>1) output_byte(imm);
  }
  else {
    emit_mov(rs,rt);
    emit_sarimm(rt,imm,rt);
  }
}

void emit_rorimm(int rs,u_int imm,int rt)
{
  if(rs==rt) {
    assem_debug("ror %%%s,%d\n",regname[rt],imm);
    assert(imm>0);
    if(imm==1) output_byte(0xD1);
    else output_byte(0xC1);
    output_modrm(3,rt,1);
    if(imm>1) output_byte(imm);
  }
  else {
    emit_mov(rs,rt);
    emit_sarimm(rt,imm,rt);
  }
}

void emit_shldimm(int rs,int rs2,u_int imm,int rt)
{
  if(rs==rt) {
    assem_debug("shld %%%s,%%%s,%d\n",regname[rt],regname[rs2],imm);
    assert(imm>0);
    output_byte(0x0F);
    output_byte(0xA4);
    output_modrm(3,rt,rs2);
    output_byte(imm);
  }
  else {
    emit_mov(rs,rt);
    emit_shldimm(rt,rs2,imm,rt);
  }
}

void emit_shrdimm(int rs,int rs2,u_int imm,int rt)
{
  if(rs==rt) {
    assem_debug("shrd %%%s,%%%s,%d\n",regname[rt],regname[rs2],imm);
    assert(imm>0);
    output_byte(0x0F);
    output_byte(0xAC);
    output_modrm(3,rt,rs2);
    output_byte(imm);
  }
  else {
    emit_mov(rs,rt);
    emit_shrdimm(rt,rs2,imm,rt);
  }
}

void emit_shlcl(int r)
{
  assem_debug("shl %%%s,%%cl\n",regname[r]);
  output_byte(0xD3);
  output_modrm(3,r,4);
}
void emit_shrcl(int r)
{
  assem_debug("shr %%%s,%%cl\n",regname[r]);
  output_byte(0xD3);
  output_modrm(3,r,5);
}
void emit_sarcl(int r)
{
  assem_debug("sar %%%s,%%cl\n",regname[r]);
  output_byte(0xD3);
  output_modrm(3,r,7);
}

void emit_shldcl(int r1,int r2)
{
  assem_debug("shld %%%s,%%%s,%%cl\n",regname[r1],regname[r2]);
  output_byte(0x0F);
  output_byte(0xA5);
  output_modrm(3,r1,r2);
}
void emit_shrdcl(int r1,int r2)
{
  assem_debug("shrd %%%s,%%%s,%%cl\n",regname[r1],regname[r2]);
  output_byte(0x0F);
  output_byte(0xAD);
  output_modrm(3,r1,r2);
}

void emit_cmpimm(int rs,int imm)
{
  assem_debug("cmp $%d,%%%s\n",imm,regname[rs]);
  if(imm<128&&imm>=-128) {
    output_byte(0x83);
    output_modrm(3,rs,7);
    output_byte(imm);
  }
  else
  {
    output_byte(0x81);
    output_modrm(3,rs,7);
    output_w32(imm);
  }
}

void emit_cmovne(u_int *addr,int rt)
{
  assem_debug("cmovne %x,%%%s",(int)addr,regname[rt]);
  if(addr==&const_zero) assem_debug(" [zero]\n");
  else if(addr==&const_one) assem_debug(" [one]\n");
  else assem_debug("\n");
  output_byte(0x0F);
  output_byte(0x45);
  output_modrm(0,5,rt);
  output_w32((int)addr);
}
void emit_cmovl(u_int *addr,int rt)
{
  assem_debug("cmovl %x,%%%s",(int)addr,regname[rt]);
  if(addr==&const_zero) assem_debug(" [zero]\n");
  else if(addr==&const_one) assem_debug(" [one]\n");
  else assem_debug("\n");
  output_byte(0x0F);
  output_byte(0x4C);
  output_modrm(0,5,rt);
  output_w32((int)addr);
}
void emit_cmovs(u_int *addr,int rt)
{
  assem_debug("cmovs %x,%%%s",(int)addr,regname[rt]);
  if(addr==&const_zero) assem_debug(" [zero]\n");
  else if(addr==&const_one) assem_debug(" [one]\n");
  else assem_debug("\n");
  output_byte(0x0F);
  output_byte(0x48);
  output_modrm(0,5,rt);
  output_w32((int)addr);
}
void emit_cmovne_reg(int rs,int rt)
{
  assem_debug("cmovne %%%s,%%%s\n",regname[rs],regname[rt]);
  output_byte(0x0F);
  output_byte(0x45);
  output_modrm(3,rs,rt);
}
void emit_cmovl_reg(int rs,int rt)
{
  assem_debug("cmovl %%%s,%%%s\n",regname[rs],regname[rt]);
  output_byte(0x0F);
  output_byte(0x4C);
  output_modrm(3,rs,rt);
}
void emit_cmovs_reg(int rs,int rt)
{
  assem_debug("cmovs %%%s,%%%s\n",regname[rs],regname[rt]);
  output_byte(0x0F);
  output_byte(0x48);
  output_modrm(3,rs,rt);
}
void emit_setl(int rt)
{
  assem_debug("setl %%%s\n",regname[rt]);
  output_byte(0x0F);
  output_byte(0x9C);
  output_modrm(3,rt,2);
}
void emit_movzbl_reg(int rs, int rt)
{
  assem_debug("movzbl %%%s,%%%s\n",regname[rs]+1,regname[rt]);
  output_byte(0x0F);
  output_byte(0xB6);
  output_modrm(3,rs,rt);
}

void emit_slti32(int rs,int imm,int rt)
{
  if(rs!=rt) emit_zeroreg(rt);
  emit_cmpimm(rs,imm);
  if(rt<4) {
    emit_setl(rt);
    if(rs==rt) emit_movzbl_reg(rt,rt);
  }
  else
  {
    if(rs==rt) emit_movimm(0,rt);
    emit_cmovl(&const_one,rt);
  }
}
void emit_sltiu32(int rs,int imm,int rt)
{
  if(rs!=rt) emit_zeroreg(rt);
  emit_cmpimm(rs,imm);
  if(rs==rt) emit_movimm(0,rt);
  emit_adcimm(0,rt);
}
void emit_slti64_32(int rsh,int rsl,int imm,int rt)
{
  assert(rsh!=rt);
  emit_slti32(rsl,imm,rt);
  if(imm>=0)
  {
    emit_test(rsh,rsh);
    emit_cmovne(&const_zero,rt);
    emit_cmovs(&const_one,rt);
  }
  else
  {
    emit_cmpimm(rsh,-1);
    emit_cmovne(&const_zero,rt);
    emit_cmovl(&const_one,rt);
  }
}
void emit_sltiu64_32(int rsh,int rsl,int imm,int rt)
{
  assert(rsh!=rt);
  emit_sltiu32(rsl,imm,rt);
  if(imm>=0)
  {
    emit_test(rsh,rsh);
    emit_cmovne(&const_zero,rt);
  }
  else
  {
    emit_cmpimm(rsh,-1);
    emit_cmovne(&const_one,rt);
  }
}

void emit_cmp(int rs,int rt)
{
  assem_debug("cmp %%%s,%%%s\n",regname[rt],regname[rs]);
  output_byte(0x39);
  output_modrm(3,rs,rt);
}
void emit_set_gz32(int rs, int rt)
{
  //assem_debug("set_gz32\n");
  emit_cmpimm(rs,1);
  emit_movimm(1,rt);
  emit_cmovl(&const_zero,rt);
}
void emit_set_nz32(int rs, int rt)
{
  //assem_debug("set_nz32\n");
  emit_cmpimm(rs,1);
  emit_movimm(1,rt);
  emit_sbbimm(0,rt);
}
void emit_set_gz64_32(int rsh, int rsl, int rt)
{
  //assem_debug("set_gz64\n");
  assert(rsl!=rt);
  emit_set_gz32(rsl,rt);
  emit_test(rsh,rsh);
  emit_cmovne(&const_one,rt);
  emit_cmovs(&const_zero,rt);
}
void emit_set_nz64_32(int rsh, int rsl, int rt)
{
  //assem_debug("set_nz64\n");
  emit_or_and_set_flags(rsh,rsl,rt);
  emit_cmovne(&const_one,rt);
}
void emit_set_if_less32(int rs1, int rs2, int rt)
{
  //assem_debug("set if less (%%%s,%%%s),%%%s\n",regname[rs1],regname[rs2],regname[rt]);
  if(rs1!=rt&&rs2!=rt) emit_zeroreg(rt);
  emit_cmp(rs1,rs2);
  if(rs1==rt||rs2==rt) emit_movimm(0,rt);
  emit_cmovl(&const_one,rt);
}
void emit_set_if_carry32(int rs1, int rs2, int rt)
{
  //assem_debug("set if carry (%%%s,%%%s),%%%s\n",regname[rs1],regname[rs2],regname[rt]);
  if(rs1!=rt&&rs2!=rt) emit_zeroreg(rt);
  emit_cmp(rs1,rs2);
  if(rs1==rt||rs2==rt) emit_movimm(0,rt);
  emit_adcimm(0,rt);
}
void emit_set_if_less64_32(int u1, int l1, int u2, int l2, int rt)
{
  //assem_debug("set if less64 (%%%s,%%%s,%%%s,%%%s),%%%s\n",regname[u1],regname[l1],regname[u2],regname[l2],regname[rt]);
  assert(u1!=rt);
  assert(u2!=rt);
  emit_cmp(l1,l2);
  emit_mov(u1,rt);
  emit_sbb(rt,u2);
  emit_movimm(0,rt);
  emit_cmovl(&const_one,rt);
}
void emit_set_if_carry64_32(int u1, int l1, int u2, int l2, int rt)
{
  //assem_debug("set if carry64 (%%%s,%%%s,%%%s,%%%s),%%%s\n",regname[u1],regname[l1],regname[u2],regname[l2],regname[rt]);
  assert(u1!=rt);
  assert(u2!=rt);
  emit_cmp(l1,l2);
  emit_mov(u1,rt);
  emit_sbb(rt,u2);
  emit_movimm(0,rt);
  emit_adcimm(0,rt);
}

void emit_call(int a)
{
  assem_debug("call %x (%x+%x)\n",a,(int)out+5,a-(int)out-5);
  output_byte(0xe8);
  output_w32(a-(int)out-4);
}
void emit_jmp(int a)
{
  assem_debug("jmp %x (%x+%x)\n",a,(int)out+5,a-(int)out-5);
  output_byte(0xe9);
  output_w32(a-(int)out-4);
}
void emit_jne(int a)
{
  assem_debug("jne %x\n",a);
  output_byte(0x0f);
  output_byte(0x85);
  output_w32(a-(int)out-4);
}
void emit_jeq(int a)
{
  assem_debug("jeq %x\n",a);
  output_byte(0x0f);
  output_byte(0x84);
  output_w32(a-(int)out-4);
}
void emit_js(int a)
{
  assem_debug("js %x\n",a);
  output_byte(0x0f);
  output_byte(0x88);
  output_w32(a-(int)out-4);
}
void emit_jns(int a)
{
  assem_debug("jns %x\n",a);
  output_byte(0x0f);
  output_byte(0x89);
  output_w32(a-(int)out-4);
}
void emit_jl(int a)
{
  assem_debug("jl %x\n",a);
  output_byte(0x0f);
  output_byte(0x8c);
  output_w32(a-(int)out-4);
}
void emit_jge(int a)
{
  assem_debug("jge %x\n",a);
  output_byte(0x0f);
  output_byte(0x8d);
  output_w32(a-(int)out-4);
}
void emit_jno(int a)
{
  assem_debug("jno %x\n",a);
  output_byte(0x0f);
  output_byte(0x81);
  output_w32(a-(int)out-4);
}

void emit_pushimm(int imm)
{
  assem_debug("push $%x\n",imm);
  output_byte(0x68);
  output_w32(imm);
}
void emit_pusha()
{
  assem_debug("pusha\n");
  output_byte(0x60);
}
void emit_popa()
{
  assem_debug("popa\n");
  output_byte(0x61);
}
void emit_pushreg(u_int r)
{
  assem_debug("push %%%s\n",regname[r]);
  assert(r<8);
  output_byte(0x50+r);
}
void emit_popreg(u_int r)
{
  assem_debug("pop %%%s\n",regname[r]);
  assert(r<8);
  output_byte(0x58+r);
}
void emit_callreg(u_int r)
{
  assem_debug("call *%%%s\n",regname[r]);
  assert(r<8);
  output_byte(0xFF);
  output_modrm(3,r,2);
}
void emit_jmpreg(u_int r)
{
  assem_debug("jmp *%%%s\n",regname[r]);
  assert(r<8);
  output_byte(0xFF);
  output_modrm(3,r,4);
}

void emit_readword_indexed(int addr, int rs, int rt)
{
  assem_debug("mov %x+%%%s,%%%s\n",addr,regname[rs],regname[rt]);
  output_byte(0x8B);
  if(addr<128&&addr>=-128) {
    output_modrm(1,rs,rt);
    if(rs==ESP) output_sib(0,4,4);
    output_byte(addr);
  }
  else
  {
    output_modrm(2,rs,rt);
    if(rs==ESP) output_sib(0,4,4);
    output_w32(addr);
  }
}
void emit_movmem_indexedx4(int addr, int rs, int rt)
{
  assem_debug("mov (%x,%%%s,4),%%%s\n",addr,regname[rs],regname[rt]);
  output_byte(0x8B);
  output_modrm(0,4,rt);
  output_sib(2,rs,5);
  output_w32(addr);
}
void emit_movsbl_indexed(int addr, int rs, int rt)
{
  assem_debug("movsbl %x+%%%s,%%%s\n",addr,regname[rs],regname[rt]);
  output_byte(0x0F);
  output_byte(0xBE);
  output_modrm(2,rs,rt);
  output_w32(addr);
}
void emit_movswl_indexed(int addr, int rs, int rt)
{
  assem_debug("movswl %x+%%%s,%%%s\n",addr,regname[rs],regname[rt]);
  output_byte(0x0F);
  output_byte(0xBF);
  output_modrm(2,rs,rt);
  output_w32(addr);
}
void emit_movzbl_indexed(int addr, int rs, int rt)
{
  assem_debug("movzbl %x+%%%s,%%%s\n",addr,regname[rs],regname[rt]);
  output_byte(0x0F);
  output_byte(0xB6);
  output_modrm(2,rs,rt);
  output_w32(addr);
}
void emit_movzwl_indexed(int addr, int rs, int rt)
{
  assem_debug("movzwl %x+%%%s,%%%s\n",addr,regname[rs],regname[rt]);
  output_byte(0x0F);
  output_byte(0xB7);
  output_modrm(2,rs,rt);
  output_w32(addr);
}
void emit_readword(int addr, int rt)
{
  assem_debug("mov %x,%%%s\n",addr,regname[rt]);
  output_byte(0x8B);
  output_modrm(0,5,rt);
  output_w32(addr);
}
void emit_movsbl(int addr, int rt)
{
  assem_debug("movsbl %x,%%%s\n",addr,regname[rt]);
  output_byte(0x0F);
  output_byte(0xBE);
  output_modrm(0,5,rt);
  output_w32(addr);
}
void emit_movswl(int addr, int rt)
{
  assem_debug("movswl %x,%%%s\n",addr,regname[rt]);
  output_byte(0x0F);
  output_byte(0xBF);
  output_modrm(0,5,rt);
  output_w32(addr);
}
void emit_movzbl(int addr, int rt)
{
  assem_debug("movzbl %x,%%%s\n",addr,regname[rt]);
  output_byte(0x0F);
  output_byte(0xB6);
  output_modrm(0,5,rt);
  output_w32(addr);
}
void emit_movzwl(int addr, int rt)
{
  assem_debug("movzwl %x,%%%s\n",addr,regname[rt]);
  output_byte(0x0F);
  output_byte(0xB7);
  output_modrm(0,5,rt);
  output_w32(addr);
}
void emit_movzwl_reg(int rs, int rt)
{
  assem_debug("movzwl %%%s,%%%s\n",regname[rs]+1,regname[rt]);
  output_byte(0x0F);
  output_byte(0xB7);
  output_modrm(3,rs,rt);
}

void emit_xchg(int rs, int rt)
{
  assem_debug("xchg %%%s,%%%s\n",regname[rs],regname[rt]);
  if(rs==EAX) {
    output_byte(0x90+rt);
  }
  else
  {
    output_byte(0x87);
    output_modrm(3,rs,rt);
  }
}
void emit_writeword_indexed(int rt, int addr, int rs)
{
  assem_debug("mov %%%s,%x+%%%s\n",regname[rt],addr,regname[rs]);
  output_byte(0x89);
  if(addr<128&&addr>=-128) {
    output_modrm(1,rs,rt);
    if(rs==ESP) output_sib(0,4,4);
    output_byte(addr);
  }
  else
  {
    output_modrm(2,rs,rt);
    if(rs==ESP) output_sib(0,4,4);
    output_w32(addr);
  }
}
void emit_writehword_indexed(int rt, int addr, int rs)
{
  assem_debug("movw %%%s,%x+%%%s\n",regname[rt]+1,addr,regname[rs]);
  output_byte(0x66);
  output_byte(0x89);
  if(addr<128&&addr>=-128) {
    output_modrm(1,rs,rt);
    output_byte(addr);
  }
  else
  {
    output_modrm(2,rs,rt);
    output_w32(addr);
  }
}
void emit_writebyte_indexed(int rt, int addr, int rs)
{
  if(rt<4) {
    assem_debug("movb %%%cl,%x+%%%s\n",regname[rt][1],addr,regname[rs]);
    output_byte(0x88);
    if(addr<128&&addr>=-128) {
      output_modrm(1,rs,rt);
      output_byte(addr);
    }
    else
    {
      output_modrm(2,rs,rt);
      output_w32(addr);
    }
  }
  else
  {
    emit_xchg(EAX,rt);
    emit_writebyte_indexed(EAX,addr,rs==EAX?rt:rs);
    emit_xchg(EAX,rt);
  }
}
void emit_writeword(int rt, int addr)
{
  assem_debug("movl %%%s,%x\n",regname[rt],addr);
  output_byte(0x89);
  output_modrm(0,5,rt);
  output_w32(addr);
}
void emit_writehword(int rt, int addr)
{
  assem_debug("movw %%%s,%x\n",regname[rt]+1,addr);
  output_byte(0x66);
  output_byte(0x89);
  output_modrm(0,5,rt);
  output_w32(addr);
}
void emit_writebyte(int rt, int addr)
{
  if(rt<4) {
    assem_debug("movb %%%cl,%x\n",regname[rt][1],addr);
    output_byte(0x88);
    output_modrm(0,5,rt);
    output_w32(addr);
  }
  else
  {
    emit_xchg(EAX,rt);
    emit_writebyte(EAX,addr);
    emit_xchg(EAX,rt);
  }
}
void emit_writeword_imm(int imm, int addr)
{
  assem_debug("movl $%x,%x\n",imm,addr);
  output_byte(0xC7);
  output_modrm(0,5,0);
  output_w32(addr);
  output_w32(imm);
}
void emit_writebyte_imm(int imm, int addr)
{
  assem_debug("movb $%x,%x\n",imm,addr);
  output_byte(0xC6);
  output_modrm(0,5,0);
  output_w32(addr);
  output_byte(imm);
}

void emit_mul(int rs)
{
  assem_debug("mul %%%s\n",regname[rs]);
  output_byte(0xF7);
  output_modrm(3,rs,4);
}
void emit_imul(int rs)
{
  assem_debug("imul %%%s\n",regname[rs]);
  output_byte(0xF7);
  output_modrm(3,rs,5);
}
void emit_div(int rs)
{
  assem_debug("div %%%s\n",regname[rs]);
  output_byte(0xF7);
  output_modrm(3,rs,6);
}
void emit_idiv(int rs)
{
  assem_debug("idiv %%%s\n",regname[rs]);
  output_byte(0xF7);
  output_modrm(3,rs,7);
}
void emit_cdq()
{
  assem_debug("cdq\n");
  output_byte(0x99);
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
  output_w32((int)addr);
}

/*void emit_submem(int r,int addr)
{
  assert(r>=0&&r<8);
  assem_debug("sub %x,%%%s\n",addr,regname[r]);
  output_byte(0x2B);
  output_modrm(0,5,r);
  output_w32((int)addr);
}*/

/* Special assem */

void shift_assemble_x86(int i,char regmap[])
{
  if(rt1[i]) {
    if(opcode2[i]<=0x07) // SLLV/SRLV/SRAV
    {
      char s,t,shift;
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
          char temp=get_reg(regmap,-1);
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
      char sh,sl,th,tl,shift;
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
#define shift_assemble shift_assemble_x86

void loadlr_assemble_x86(int i,char regmap[])
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
    if(regmap[hr]>=0) reglist|=1<<hr;
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
      emit_readword_indexed((int)rdram-0x80000000,temp2,temp2);
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
      if(th>=0) emit_readword_indexed((int)rdram-0x80000000,temp2,temp2h);
      emit_readword_indexed((int)rdram-0x7FFFFFFC,temp2,temp2);
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
#define loadlr_assemble loadlr_assemble_x86

void cop0_assemble(int i,char regmap[])
{
  if(opcode2[i]==0) // MFC0
  {
    char t=get_reg(regmap,rt1[i]);
    char copr=(source[i]>>11)&0x1f;
    //assert(t>=0); // Why does this happen?  OOT is weird
    if(t>=0) {
      emit_writeword_imm((int)&fake_pc,(int)&PC);
      emit_writebyte_imm((source[i]>>11)&0x1f,(int)&(fake_pc.f.r.nrd));
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
    char s=get_reg(regmap,rs1[i]);
    char copr=(source[i]>>11)&0x1f;
    assert(s>=0);
    emit_writeword(s,(int)&readmem_dword);
    wb_register(rs1[i],regmap,dirty_post[i],is32[i]);
    emit_writeword_imm((int)&fake_pc,(int)&PC);
    emit_writebyte_imm((source[i]>>11)&0x1f,(int)&(fake_pc.f.r.nrd));
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
      emit_addimm_and_set_flags(2*count,HOST_CCREG); // TODO: Should there be an extra cycle here?
      // FIXME: Check for interrupt
      emit_readword((int)&Status,EAX);
      // FIXME: Check bit 2 before clearing bit 1
      emit_andimm(EAX,~2,EAX);
      emit_writeword(EAX,(int)&Status);
      emit_readword((int)&EPC,EAX);
      emit_pushreg(EAX);
      emit_storereg(CCREG,HOST_CCREG);
      emit_call((int)get_addr_ht);
      emit_loadreg(CCREG,HOST_CCREG);
      emit_addimm(ESP,4,ESP);
      emit_jmpreg(EAX);
    }
  }
}

void cop1_assemble(int i,char regmap[])
{
  char rs=get_reg(regmap,CSREG);
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
    char tl=get_reg(regmap,rt1[i]);
    char th=get_reg(regmap,rt1[i]|64);
    //assert(tl>=0);
    if(tl>=0) {
      emit_writeword_imm((int)&fake_pc,(int)&PC);
      emit_writebyte_imm((source[i]>>11)&0x1f,(int)&(fake_pc.f.r.nrd));
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
    char sl=get_reg(regmap,rs1[i]);
    char sh=get_reg(regmap,rs1[i]|64);
    char copr=(source[i]>>11)&0x1f;
    assert(sl>=0);
    emit_writeword(sl,(int)&readmem_dword);
    if(opcode2[i]==5) {
      assert(sh>=0);
      emit_writeword(sh,(int)&readmem_dword+4);
    }
    wb_register(rs1[i],regmap,dirty_post[i],is32[i]);
    emit_writeword_imm((int)&fake_pc,(int)&PC);
    emit_writebyte_imm((source[i]>>11)&0x1f,(int)&(fake_pc.f.r.nrd));
    if(opcode2[i]==4) emit_call((int)MTC1);
    if(opcode2[i]==5) emit_call((int)DMTC1);
    if(opcode2[i]==6) emit_call((int)CTC1);
    emit_loadreg(rs1[i],sl);
    if(sh>=0) emit_loadreg(rs1[i]|64,sh);
    emit_loadreg(CSREG,rs);
  }
  else if (opcode2[i]==0) { // MFC1
    char tl=get_reg(regmap,rt1[i]);
    emit_readword((int)&reg_cop1_simple[(source[i]>>11)&0x1f],tl);
    emit_readword_indexed(0,tl,tl);
  }
  else if (opcode2[i]==1) { // DMFC1
    char tl=get_reg(regmap,rt1[i]);
    char th=get_reg(regmap,rt1[i]|64);
    emit_readword((int)&reg_cop1_double[(source[i]>>11)&0x1f],tl);
    emit_readword_indexed(4,tl,th);
    emit_readword_indexed(0,tl,tl);
  }
  else if (opcode2[i]==4) { // MTC1
    char sl=get_reg(regmap,rs1[i]);
    char temp=get_reg(regmap,-1);
    emit_readword((int)&reg_cop1_simple[(source[i]>>11)&0x1f],temp);
    emit_writeword_indexed(sl,0,temp);
  }
  else if (opcode2[i]==5) { // DMTC1
    char sl=get_reg(regmap,rs1[i]);
    char sh=get_reg(regmap,rs1[i]|64);
    char temp=get_reg(regmap,-1);
    emit_readword((int)&reg_cop1_double[(source[i]>>11)&0x1f],temp);
    emit_writeword_indexed(sh,4,temp);
    emit_writeword_indexed(sl,0,temp);
  }
}

void multdiv_assemble_x86(int i,char regmap[])
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
        char m1=get_reg(regmap,rs1[i]);
        char m2=get_reg(regmap,rs2[i]);
        assert(m1>=0);
        assert(m2>=0);
        emit_mov(m1,EAX);
        emit_imul(m2);
      }
      if(opcode2[i]==0x19) // MULTU
      {
        char m1=get_reg(regmap,rs1[i]);
        char m2=get_reg(regmap,rs2[i]);
        assert(m1>=0);
        assert(m2>=0);
        emit_mov(m1,EAX);
        emit_mul(m2);
      }
      if(opcode2[i]==0x1A) // DIV
      {
        char d1=get_reg(regmap,rs1[i]);
        char d2=get_reg(regmap,rs2[i]);
        assert(d1>=0);
        assert(d2>=0);
        emit_mov(d1,EAX);
        emit_cdq();
        emit_idiv(d2);
      }
      if(opcode2[i]==0x1B) // DIVU
      {
        char d1=get_reg(regmap,rs1[i]);
        char d2=get_reg(regmap,rs2[i]);
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
        char m1h=get_reg(regmap,rs1[i]|64);
        char m1l=get_reg(regmap,rs1[i]);
        char m2h=get_reg(regmap,rs2[i]|64);
        char m2l=get_reg(regmap,rs2[i]);
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
        char hih=get_reg(regmap,HIREG|64);
        char hil=get_reg(regmap,HIREG);
        if(hih>=0) emit_loadreg(HIREG|64,hih);
        if(hil>=0) emit_loadreg(HIREG,hil);
        char loh=get_reg(regmap,LOREG|64);
        char lol=get_reg(regmap,LOREG);
        if(loh>=0) emit_loadreg(LOREG|64,loh);
        if(lol>=0) emit_loadreg(LOREG,lol);
      }
      if(opcode2[i]==0x1D) // DMULTU
      {
        char m1h=get_reg(regmap,rs1[i]|64);
        char m1l=get_reg(regmap,rs1[i]);
        char m2h=get_reg(regmap,rs2[i]|64);
        char m2l=get_reg(regmap,rs2[i]);
        char temp=get_reg(regmap,-1);
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
        emit_loadreg(HIREG,temp);
        emit_adcimm(0,EDX);
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
        char hih=get_reg(regmap,HIREG|64);
        char hil=get_reg(regmap,HIREG);
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
        char d1h=get_reg(regmap,rs1[i]|64);
        char d1l=get_reg(regmap,rs1[i]);
        char d2h=get_reg(regmap,rs2[i]|64);
        char d2l=get_reg(regmap,rs2[i]);
        assert(d1h>=0);
        assert(d2h>=0);
        assert(d1l>=0);
        assert(d2l>=0);
        //emit_pushreg(d2h);
        //emit_pushreg(d2l);
        //emit_pushreg(d1h);
        //emit_pushreg(d1l);
        emit_addimm(ESP,-16,ESP);
        emit_writeword_indexed(d2h,12,ESP);
        emit_writeword_indexed(d2l,8,ESP);
        emit_writeword_indexed(d1h,4,ESP);
        emit_writeword_indexed(d1l,0,ESP);
        emit_call((int)&div64);
        //emit_popreg(d1l);
        //emit_popreg(d1h);
        //emit_popreg(d2l);
        //emit_popreg(d2h);
        emit_readword_indexed(0,ESP,d1l);
        emit_readword_indexed(4,ESP,d1h);
        emit_readword_indexed(8,ESP,d2l);
        emit_readword_indexed(12,ESP,d2h);
        emit_addimm(ESP,16,ESP);
        char hih=get_reg(regmap,HIREG|64);
        char hil=get_reg(regmap,HIREG);
        char loh=get_reg(regmap,LOREG|64);
        char lol=get_reg(regmap,LOREG);
        if(hih>=0) emit_loadreg(HIREG|64,hih);
        if(hil>=0) emit_loadreg(HIREG,hil);
        if(loh>=0) emit_loadreg(LOREG|64,loh);
        if(lol>=0) emit_loadreg(LOREG,lol);
      }
      if(opcode2[i]==0x1F) // DDIVU
      {
        char d1h=get_reg(regmap,rs1[i]|64);
        char d1l=get_reg(regmap,rs1[i]);
        char d2h=get_reg(regmap,rs2[i]|64);
        char d2l=get_reg(regmap,rs2[i]);
        assert(d1h>=0);
        assert(d2h>=0);
        assert(d1l>=0);
        assert(d2l>=0);
        //emit_pushreg(d2h);
        //emit_pushreg(d2l);
        //emit_pushreg(d1h);
        //emit_pushreg(d1l);
        emit_addimm(ESP,-16,ESP);
        emit_writeword_indexed(d2h,12,ESP);
        emit_writeword_indexed(d2l,8,ESP);
        emit_writeword_indexed(d1h,4,ESP);
        emit_writeword_indexed(d1l,0,ESP);
        emit_call((int)&divu64);
        //emit_popreg(d1l);
        //emit_popreg(d1h);
        //emit_popreg(d2l);
        //emit_popreg(d2h);
        emit_readword_indexed(0,ESP,d1l);
        emit_readword_indexed(4,ESP,d1h);
        emit_readword_indexed(8,ESP,d2l);
        emit_readword_indexed(12,ESP,d2h);
        emit_addimm(ESP,16,ESP);
        char hih=get_reg(regmap,HIREG|64);
        char hil=get_reg(regmap,HIREG);
        char loh=get_reg(regmap,LOREG|64);
        char lol=get_reg(regmap,LOREG);
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
    char hr=get_reg(regmap,HIREG);
    char lr=get_reg(regmap,LOREG);
    if(hr>=0) emit_zeroreg(hr);
    if(lr>=0) emit_zeroreg(lr);
  }
}
#define multdiv_assemble multdiv_assemble_x86

void syscall_assemble(int i,char regmap[])
{
  char ccreg=get_reg(regmap,CCREG);
  assert(ccreg==HOST_CCREG);
  assert(!is_delayslot);
  emit_movimm(start+i*4,EAX); // Get PC
  emit_addimm(HOST_CCREG,2*ccadj[i],HOST_CCREG); // CHECK: is this right?  There should probably be an extra cycle...
  emit_jmp((int)jump_syscall);
}

emit_extjump(int addr, int target)
{
  u_char *ptr=(u_char *)addr;
  if(*ptr==0x0f)
  {
    assert(ptr[1]>=0x80&&ptr[1]<=0x8f);
    addr+=2;
  }
  else
  {
    assert(*ptr==0xe8||*ptr==0xe9);
    addr++;
  }
  emit_movimm(target,EAX);
  emit_movimm(addr,EBX);
  assert(addr>=0x7000000&&addr<0x7FFFFFF);
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

do_readstub(int n)
{
  set_jump_target(stubs[n][1],(int)out);
  int type=stubs[n][0];
  char *regmap=(char *)stubs[n][3];
  int rs=get_reg(regmap,stubs[n][4]);
  int rth=get_reg(regmap,stubs[n][4]|64);
  int rt=get_reg(regmap,stubs[n][4]);
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
  emit_shrimm(rs,16,rs);
  emit_movmem_indexedx4(ftable,rs,rs);
  emit_pusha();
  int cc=get_reg(regmap,CCREG);
  int temp;
  if(cc<0) {
    if(rs==HOST_CCREG)
    {
      cc=0;temp=1;
      assert(cc!=HOST_CCREG);
      assert(temp!=HOST_CCREG);
      emit_loadreg(CCREG,cc);
    }
    else
    {
      cc=HOST_CCREG;
      emit_loadreg(CCREG,cc);
      temp=!rs;
    }
  }
  else
  {
    temp=!rs;
  }
  emit_readword((int)&last_count,temp);
  emit_addimm(cc,2*stubs[n][5]+2,cc);
  emit_add(cc,temp,cc);
  emit_writeword(cc,(int)&Count);
  emit_callreg(rs);
  // We really shouldn't need to update the count here,
  // but not doing so causes random crashes...
  emit_readword((int)&Count,HOST_CCREG);
  emit_readword((int)&next_interupt,ECX);
  emit_addimm(HOST_CCREG,-2*stubs[n][5]-2,HOST_CCREG);
  emit_sub(HOST_CCREG,ECX,HOST_CCREG);
  emit_writeword(ECX,(int)&last_count);
  emit_storereg(CCREG,HOST_CCREG);
  emit_popa();
  if((cc=get_reg(regmap,CCREG))>=0) {
    emit_loadreg(CCREG,cc);
  }
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

inline_readstub(int type, u_int addr, char regmap[], int target, int adj, u_int reglist)
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
  //emit_shrimm(rs,16,rs);
  //emit_movmem_indexedx4(ftable,rs,rs);
  emit_pusha();
  int cc=get_reg(regmap,CCREG);
  int temp;
  if(cc<0) {
    if(rs==HOST_CCREG)
    {
      cc=0;temp=1;
      assert(cc!=HOST_CCREG);
      assert(temp!=HOST_CCREG);
      emit_loadreg(CCREG,cc);
    }
    else
    {
      cc=HOST_CCREG;
      emit_loadreg(CCREG,cc);
      temp=!rs;
    }
  }
  else
  {
    temp=!rs;
  }
  emit_readword((int)&last_count,temp);
  emit_addimm(cc,2*adj+2,cc);
  emit_add(cc,temp,cc);
  emit_writeword(cc,(int)&Count);
  emit_call(((u_int *)ftable)[addr>>16]);
  // We really shouldn't need to update the count here,
  // but not doing so causes random crashes...
  emit_readword((int)&Count,HOST_CCREG);
  emit_readword((int)&next_interupt,ECX);
  emit_addimm(HOST_CCREG,-2*adj-2,HOST_CCREG);
  emit_sub(HOST_CCREG,ECX,HOST_CCREG);
  emit_writeword(ECX,(int)&last_count);
  emit_storereg(CCREG,HOST_CCREG);
  emit_popa();
  if((cc=get_reg(regmap,CCREG))>=0) {
    emit_loadreg(CCREG,cc);
  }
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

do_writestub(int n)
{
  set_jump_target(stubs[n][1],(int)out);
  int type=stubs[n][0];
  char *regmap=(char *)stubs[n][3];
  int rs=get_reg(regmap,-1);
  int rth=get_reg(regmap,stubs[n][4]|64);
  int rt=get_reg(regmap,stubs[n][4]);
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
  emit_shrimm(rs,16,rs);
  emit_movmem_indexedx4(ftable,rs,rs);
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
  emit_pusha();
  int cc=get_reg(regmap,CCREG);
  int temp;
  if(cc<0) {
    if(rs==HOST_CCREG)
    {
      cc=0;temp=1;
      assert(cc!=HOST_CCREG);
      assert(temp!=HOST_CCREG);
      emit_loadreg(CCREG,cc);
    }
    else
    {
      cc=HOST_CCREG;
      emit_loadreg(CCREG,cc);
      temp=!rs;
    }
  }
  else
  {
    temp=!rs;
  }
  emit_readword((int)&last_count,temp);
  emit_addimm(cc,2*stubs[n][5]+2,cc);
  emit_add(cc,temp,cc);
  emit_writeword(cc,(int)&Count);
  emit_callreg(rs);
  emit_readword((int)&Count,HOST_CCREG);
  emit_readword((int)&next_interupt,ECX);
  emit_addimm(HOST_CCREG,-2*stubs[n][5]-2,HOST_CCREG);
  emit_sub(HOST_CCREG,ECX,HOST_CCREG);
  emit_writeword(ECX,(int)&last_count);
  emit_storereg(CCREG,HOST_CCREG);
  emit_popa();
  if((cc=get_reg(regmap,CCREG))>=0) {
    emit_loadreg(CCREG,cc);
  }
  emit_jmp(stubs[n][2]); // return address
}

inline_writestub(int type, u_int addr, char regmap[], int target, int adj, u_int reglist)
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
  emit_pusha();
  int cc=get_reg(regmap,CCREG);
  int temp;
  if(cc<0) {
    if(rs==HOST_CCREG)
    {
      cc=0;temp=1;
      assert(cc!=HOST_CCREG);
      assert(temp!=HOST_CCREG);
      emit_loadreg(CCREG,cc);
    }
    else
    {
      cc=HOST_CCREG;
      emit_loadreg(CCREG,cc);
      temp=!rs;
    }
  }
  else
  {
    temp=!rs;
  }
  emit_readword((int)&last_count,temp);
  emit_addimm(cc,2*adj+2,cc);
  emit_add(cc,temp,cc);
  emit_writeword(cc,(int)&Count);
  emit_call(((u_int *)ftable)[addr>>16]);
  emit_readword((int)&Count,HOST_CCREG);
  emit_readword((int)&next_interupt,ECX);
  emit_addimm(HOST_CCREG,-2*adj-2,HOST_CCREG);
  emit_sub(HOST_CCREG,ECX,HOST_CCREG);
  emit_writeword(ECX,(int)&last_count);
  emit_storereg(CCREG,HOST_CCREG);
  emit_popa();
  if((cc=get_reg(regmap,CCREG))>=0) {
    emit_loadreg(CCREG,cc);
  }
}

do_unalignedwritestub(int n)
{
  set_jump_target(stubs[n][1],(int)out);
  output_byte(0xCC);
  emit_jmp(stubs[n][2]); // return address
}

void printregs(int edi,int esi,int ebp,int esp,int b,int d,int c,int a)
{
  printf("regs: %x %x %x %x %x %x %x (%x)\n",a,b,c,d,ebp,esi,edi,(&edi)[-1]);
}

do_invstub(int n)
{
  set_jump_target(stubs[n][1],(int)out);
  if(stubs[n][4]!=EDI) emit_xchg(stubs[n][4],EDI);
  emit_pusha();
  emit_call((int)&invalidate_block);
  emit_popa();
  if(stubs[n][4]!=EDI) emit_xchg(stubs[n][4],EDI);
  emit_jmp(stubs[n][2]); // return address
}

int do_dirty_stub(int i)
{
  assem_debug("do_dirty_stub %x\n",start+i*4);
  emit_pushimm(start+i*4);
  emit_movimm((int)source,EAX);
  emit_movimm((int)copy,EBX);
  emit_movimm(slen*4,ECX);
  emit_call((int)&verify_code);
  emit_addimm(ESP,4,ESP);
  int entry=(int)out;
  load_regs_entry(i);
  if(entry==(int)out) entry=instr_addr[i];
  emit_jmp(instr_addr[i]);
  return entry;
}

do_cop1stub(int n)
{
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

// We don't need this for x86
void literal_pool(int n) {}
void literal_pool_jumpover(int n) {}
