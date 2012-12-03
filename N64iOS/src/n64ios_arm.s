//
//  low.s
//  N64iOS
//
//  Created by Riley Testut on 11/27/12.
//  Copyright (c) 2012 Testut Tech. All rights reserved.
//

.globl _add_in_asm
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

_dynarec_local:
.space	64+16+16+8+8+8+8+256+8+8+128+128+128+8+132+132
_next_interupt = _dynarec_local + 64

_cycle_count = _next_interupt + 4

_last_count = _cycle_count + 4

_pending_exception = _last_count + 4

_pcaddr = _pending_exception + 4

_stop = _pcaddr + 4

_invc_ptr = _stop + 4

_address = _invc_ptr + 4

_readmem_dword = _address + 4

_dword = _readmem_dword + 8

_word = _dword + 8

_hword = _word + 4

_byte = _hword + 2

_FCR0 = _hword + 4

_FCR31 = _FCR0 + 4

_reg = _FCR31 + 4

_hi = _reg + 256

_lo = _hi + 8

_reg_cop0 = _lo + 8

_reg_cop1_simple = _reg_cop0 + 128

_reg_cop1_double = _reg_cop1_simple + 128

_PC = _reg_cop1_double + 128

_fake_pc = _PC + 8

_fake_pc_float = _fake_pc + 132

_add_in_asm:
add r0,r0,#1
bx lr

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

.align	2
.globl	_cc_interrupt

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

.align	2
.globl	_fp_exception_ds
//	.type	fp_exception_ds, %function
_fp_exception_ds:
mov	r2, #0x90000000 /* Set high bit if delay slot */
b	.fpe

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

.align	2
.globl	_indirect_jump
//	.type	indirect_jump, %function
_indirect_jump:
ldr	r3, [fp, #72]
add	r0, r0, r1, lsl #2
add	r2, r2, r3
str	r2, [fp, #400+36] /* Count */
ldr	pc, [r0]

.align	2
.globl	_testthis
_testthis:
bl _testthat
mov	pc, lr

.align	4
TranslationCache:
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
ldr r0, TranslationCache
ldr	pc, [r0]

.dlptr:
.word	_dynarec_local+28

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
