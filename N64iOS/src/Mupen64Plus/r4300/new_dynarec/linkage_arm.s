/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - linkage_arm.s                                           *
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
/*	.cpu arm9tdmi
	.fpu softvfp
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 2
	.eabi_attribute 30, 6
	.eabi_attribute 18, 4
*/
	.file	"dynarec_arm.s"
//	.globl	_rdram
//_rdram = 0x80000000
	.globl	_dynarec_local
	.globl	_reg
	.globl	_hi
	.globl	_lo
	.globl	_reg_cop1_simple
	.globl	_reg_cop1_double
	.globl  _reg_cop0
	.globl	_FCR0
	.globl	_FCR31
	.globl	_next_interupt
	.globl	_cycle_count
	.globl	_last_count
	.globl	_pending_exception
	.globl	_pcaddr
	.globl	_stop
	.globl	_invc_ptr
	.globl	_address
	.globl	_readmem_dword
	.globl	_dword
	.globl	_word
	.globl	_hword
	.globl	_byte
	.globl	_PC
	.globl	_fake_pc
	.globl	_fake_pc_float
	.data
	.align	4
////	.type	_dynarec_local, %object
////	.size	_dynarec_local, 64
_dynarec_local:
	.space	64+16+16+8+8+8+8+256+8+8+128+128+128+8+132+132
//	.fill   64+16+16+8+8+8+8+256+8+8+128+128+128+8+132+132, 1, 0
_next_interupt = _dynarec_local + 64
////	.type	_next_interupt, %object
////	.size	_next_interupt, 4
//	.fill   4, 1, 0
_cycle_count = _next_interupt + 4
////	.type	_cycle_count, %object
////	.size	_cycle_count, 4
//	.fill   4, 1, 0
_last_count = _cycle_count + 4
////	.type	_last_count, %object
////	.size	_last_count, 4
//	.fill   4, 1, 0
_pending_exception = _last_count + 4
////	.type	_pending_exception, %object
////	.size	_pending_exception, 4
//	.fill   4, 1, 0
_pcaddr = _pending_exception + 4
////	.type	_pcaddr, %object
////	.size	_pcaddr, 4
//	.fill   4, 1, 0
_stop = _pcaddr + 4
////	.type	_stop, %object
////	.size	_stop, 4
//	.fill   4, 1, 0
_invc_ptr = _stop + 4
////	.type	_invc_ptr, %object
////	.size	_invc_ptr, 4
//	.fill   4, 1, 0
_address = _invc_ptr + 4
////	.type	_address, %object
////	.size	_address, 4
//	.fill   4, 1, 0
_readmem_dword = _address + 4
////	.type	_readmem_dword, %object
////	.size	_readmem_dword, 8
//	.fill   8, 1, 0
_dword = _readmem_dword + 8
////	.type	_dword, %object
////	.size	_dword, 8
//	.fill   8, 1, 0
_word = _dword + 8
////	.type	_word, %object
////	.size	_word, 4
//	.fill   4, 1, 0
_hword = _word + 4
////	.type	_hword, %object
////	.size	_hword, 2
//	.fill   2, 1, 0
_byte = _hword + 2
//	.type	_byte, %object
//	.size	_byte, 1 /* 1 byte free */
//	.fill   2, 1, 0
_FCR0 = _hword + 4
//	.type	_FCR0, %object
//	.size	_FCR0, 4
//	.fill   4, 1, 0
_FCR31 = _FCR0 + 4
//	.type	_FCR31, %object
//	.size	_FCR31, 4
//	.fill   4, 1, 0
_reg = _FCR31 + 4
//	.type	_reg, %object
//	.size	_reg, 256
//	.fill   256, 1, 0
_hi = _reg + 256
//	.type	_hi, %object
//	.size	_hi, 8
//	.fill   8, 1, 0
_lo = _hi + 8
//	.type	_lo, %object
//	.size	_lo, 8
//	.fill   8, 1, 0
_reg_cop0 = _lo + 8
//	.type	_reg_cop0, %object
//	.size	_reg_cop0, 128
//	.fill   128, 1, 0
_reg_cop1_simple = _reg_cop0 + 128
//	.type	_reg_cop1_simple, %object
//	.size	_reg_cop1_simple, 128
//	.fill   128, 1, 0
_reg_cop1_double = _reg_cop1_simple + 128
//	.type	_reg_cop1_double, %object
//	.size	_reg_cop1_double, 128
//	.fill   128, 1, 0
_PC = _reg_cop1_double + 128
//	.type	_PC, %object
//	.size	_PC, 4
//	.fill   8, 1, 0
	/* 4 bytes free */
_fake_pc = _PC + 8
//	.type	_fake_pc, %object
//	.size	_fake_pc, 132
//	.fill   132, 1, 0
_fake_pc_float = _fake_pc + 132
//	.type	_fake_pc_float, %object
//	.size	_fake_pc_float, 132
//	.fill   132, 1, 0

	.text
	.align	2
	.globl	_dyna_linker
//	.type	dyna_linker, %function
_dyna_linker:
	mov	r2, #0x200000
	ldr	r3, .jiptr
	eor	r2, r2, r0, lsr #10
	ldr	r7, [r1]
	cmp	r2, #8192
	bic	r2, r2, #3
	movcs	r2, #8192
	add	r12, r7, #2
	ldr	r5, [r3, r2]
	lsl	r12, r12, #8
.L1:
	movs	r4, r5
	beq	.L3
	ldr	r3, [r5]
	ldr	r5, [r4, #12]
	teq	r3, r0
	bne	.L1
.L2:
	ldr	r3, [r4, #4]
	ldr	r4, [r4, #8]
	tst	r3, r3
	bne	.L1
	mov	r5, r1
	add	r1, r1, r12, asr #6
	teq	r1, r4
	moveq	pc, r4 /* Stale i-cache */
	bl	_add_link
	sub	r2, r4, r5
	and	r1, r7, #0xff000000
	lsl	r2, r2, #6
	sub	r1, r1, #2
	add	r1, r1, r2, lsr #8
	str	r1, [r5]
	mov	pc, r4
.L3:
	ldr	r3, .jdptr
	ldr	r5, [r3, r2]
.L4:
	movs	r4, r5
	beq	.L6
	ldr	r3, [r5]
	ldr	r5, [r4, #12]
	teq	r3, r0
	bne	.L4
.L5:
	ldr	pc, [r4, #8]
.L6:
	mov	r4, r0
	mov	r5, r1
	bl	_new_recompile_block
	mov	r0, r4
	mov	r1, r5
	b	_dyna_linker
.jiptr:
	.word	_jump_in
.jdptr:
	.word	_jump_dirty
//	.size	dyna_linker, .-dyna_linker
	.align	2
	.globl	_verify_code
//	.type	verify_code, %function
_verify_code:
	tst	r3, #4
	mov	r4, #0
	add	r3, r1, r3
	mov	r5, #0
	ldrne	r4, [r1], #4
	mov	r12, #0
	ldrne	r5, [r2], #4
.L7:
	ldr	r7, [r1], #4
	eor	r6, r4, r5 /* r9 to r6 */
	ldr	r8, [r2], #4
	orrs	r6, r6, r12 /* r9 to r6 */
	bne	.L8
	ldr	r4, [r1], #4
	eor	r12, r7, r8
	ldr	r5, [r2], #4
	cmp	r1, r3
	bcc	.L7
	teq	r4, r5
	bne	.L8
	mov	pc, lr
.L8:
	mov	r4, r0
	bl	_remove_hash
	mov	r0, r4
	bl	_new_recompile_block
	mov	r0, r4
	bl	_get_addr
	mov	pc, r0
//	.size	verify_code, .-verify_code
42421:
  .long _last_count
42422:
  .long _reg_cop0 + 36
42426:
  .long _next_interupt
42427:
  .long _pending_exception
42428:
  .long _stop
42429:
  .long _pcaddr

	.align	2
	.globl	_cc_interrupt
//	.type	cc_interrupt, %function
_cc_interrupt:
	ldr	r0, [fp, #72]
	add	r10, r0, r10
	str	r10, [fp, #400+36] /* Count */
	mov	r10, lr
	bl	_gen_interupt
	mov	lr, r10
	ldr	r10, [fp, #400+36] /* Count */
	ldr	r0, [fp, #64]
	ldr	r1, [fp, #76]
	ldr	r2, [fp, #84]
	str	r0, [fp, #72]
	sub	r10, r10, r0
	tst	r2, r2
	bne	.L10
	tst	r1, r1
	bne	.L9
	mov	pc, lr
.L9:
	ldr	r0, [fp, #80]
	bl	_get_addr_ht
	mov	pc, r0
.L10:
	add	r12, fp, #28
	ldmia	r12, {r4, r5, r6, r7, r8, r9, sl, fp, pc}
	
//	.size	cc_interrupt, .-cc_interrupt
	.align	2
	.globl	_jump_vaddr
//	.type	jump_vaddr, %function
_jump_vaddr:
	eor	r2, r0, r0, lsl #16
	ldr	r1, .htptr
	lsr	r2, r2, #12
	bic	r2, r2, #15
	ldr	r2, [r1, r2]!
	teq	r2, r0
	bne	.L12
.L11:
	ldr	pc, [r1, #4]
.L12:
	ldr	r2, [r1, #8]!
	teq	r2, r0
	beq	.L11
	bl	_get_addr
	mov	pc, r0
.htptr:
	.word	_hash_table
//	.size	jump_vaddr, .-jump_vaddr
42423:
  .long _reg_cop0 + 48
42424:
  .long _reg_cop0 + 52
42425:
  .long _reg_cop0 + 56

	.align	2
	.globl	_fp_exception
//	.type	fp_exception, %function
_fp_exception:
	mov	r2, #0x10000000
.fpe:
ldr	r1, [fp, #400+48] /* Status */
mov	r3, #0x80000000
str	r0, [fp, #400+56] /* EPC */
orr	r1, #2
add	r2, r2, #0x2c
str	r1, [fp, #400+48] /* Status */
str	r2, [fp, #400+52] /* Cause */
add	r0, r3, #0x180
bl	_get_addr_ht
mov	pc, r0

//	.size	fp_exception, .-fp_exception
	.align	2
	.globl	_fp_exception_ds
//	.type	fp_exception_ds, %function
_fp_exception_ds:
	mov	r2, #0x90000000 /* Set high bit if delay slot */
	b	.fpe
//	.size	fp_exception_ds, .-fp_exception_ds

42423:
  .long _reg_cop0 + 48
42424:
  .long _reg_cop0 + 52
42425:
  .long _reg_cop0 + 56

	.align	2
	.globl	_jump_syscall
//	.type	jump_syscall, %function
_jump_syscall:
ldr	r1, [fp, #400+48] /* Status */
mov	r3, #0x80000000
str	r0, [fp, #400+56] /* EPC */
orr	r1, #2
mov	r2, #0x20
str	r1, [fp, #400+48] /* Status */
str	r2, [fp, #400+52] /* Cause */
add	r0, r3, #0x180
bl	_get_addr_ht
mov	pc, r0

//	.size	jump_syscall, .-jump_syscall
42421:
  .long _last_count
42422:
  .long _reg_cop0 + 36

	.align	2
	.globl	_indirect_jump
//	.type	indirect_jump, %function
_indirect_jump:
ldr	r3, [fp, #72]
add	r0, r0, r1, lsl #2
add	r2, r2, r3 
str	r2, [fp, #400+36] /* Count */
ldr	pc, [r0]

//	.size	indirect_jump, .-indirect_jump

.align	2
.globl	_testthis
_testthis:
  bl _testthat
  mov	pc, lr

.align	4
42421:
  .long _last_count
42422:
  .long _reg_cop0 + 36
42426:
  .long _next_interupt
42430:
  .long _translation_cache_iphone
  
	.align	2
	.globl	_new_dyna_start
//	.type	new_dyna_start, %function
_new_dyna_start:
	ldr	r12, .dlptr
	mov	r0, #0xa4000000
	stmia	r12, {r4, r5, r6, r7, r8, r9, sl, fp, lr}
	sub	fp, r12, #28
	add	r0, r0, #0x40
	bl	_new_recompile_block
ldr	r0, [fp, #64]
ldr	r10, [fp, #400+36] /* Count */
str	r0, [fp, #72]
	sub	r10, r10, r0
//bl _testthis
  ldr r0, 42430b
	ldr	pc, [r0]
.dlptr:
	.word	_dynarec_local+28
//	.size	new_dyna_start, .-new_dyna_start
	.align	2
	.globl	_jump_eret
//	.type	jump_eret, %function
_jump_eret:
	ldr	r1, [fp, #448] /* Status */
	ldr	r0, [fp, #456] /* EPC */
	bic	r1, r1, #2
	str	r1, [fp, #448] /* Status */
	mov	r5, #248
	add	r6, fp, #128+256
	mov	r1, #0
.L13:
	ldr	r2, [r6, #-8]!
	ldr	r3, [r6, #4]
	eor	r3, r3, r2, asr #31
	subs	r3, r3, #1
	adc	r1, r1, r1
	subs	r5, r5, #8
	bne	.L13
	ldr	r2, [fp, #384]
	ldr	r3, [fp, #384+4]
	eors	r3, r3, r2, asr #31
	ldr	r2, [fp, #392]
	ldreq	r3, [fp, #396]
	eoreq	r3, r3, r2, asr #31
	subs	r3, r3, #1
	adc	r1, r1, r1
	bl	_get_addr_32
	mov	pc, r0
	.section	.note.GNUstack,""
