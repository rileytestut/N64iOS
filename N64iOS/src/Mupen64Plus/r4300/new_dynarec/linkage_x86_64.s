/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - linkage_x86_64.s                                        *
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
	.file	"linkage_x86_64.s"
	.bss
	.align 4
//.globl rdram
//rdram = 0x80000000
	.section	.rodata
	.text
.globl dyna_linker
	.type	dyna_linker, @function
dyna_linker:
	leal	0x80000000(%eax), %ecx
	movl	$2048, %edx
	shrl	$12, %ecx
	cmpl	%edx, %ecx
	cmova	%edx, %ecx
	movq	jump_in(,%ecx,8), %r12
.L1:
	test	%r12, %r12
	je	.L3
	movl	(%r12), %edi
	cmpl	%edi, %eax
	je	.L2
	movq	16(%r12), %r12
	jmp	.L1
.L2:
	mov	(%ebx), %edi
	mov	%esi, %ebp
	lea	4(%ebx,%edi,1), %esi
	mov	%eax, %edi
	call	add_link
	mov	8(%r12), %edi
	mov	%ebp, %esi
	lea	-4(%edi), %ebp
	subl	%ebx, %ebp
	movl	%ebp, (%ebx)
	jmp	*%rdi
.L3:
	movq	jump_dirty(,%ecx,8), %r12
.L4:
	test	%r12, %r12
	je	.L6
	movl	(%r12), %edi
	cmpl	%edi, %eax
	je	.L5
	movq	16(%r12), %r12
	jmp	.L4
.L5:
	movl	8(%r12), %edi
	jmp	*%rdi
.L6:
	mov	%eax, %edi
	mov	%eax, %ebp /* Note: assumes %rbx and %rbp are callee-saved */
	mov	%esi, %r12d
	call	new_recompile_block
	mov	%ebp, %eax
	mov	%r12d, %esi
	jmp	dyna_linker
	.size	dyna_linker, .-dyna_linker

.globl verify_code
	.type	verify_code, @function
verify_code:
	mov	%esi, cycle_count
	test	$7, %ecx
	je	.L7
	mov	-4(%eax,%ecx,1), %esi
	mov	-4(%ebx,%ecx,1), %edi
	add	$-4, %ecx
	xor	%esi, %edi
	jne	.L8
.L7:
	mov	-4(%eax,%ecx,1), %edx
	mov	-4(%ebx,%ecx,1), %ebp
	mov	-8(%eax,%ecx,1), %esi
	xor	%edx, %ebp
	mov	-8(%ebx,%ecx,1), %edi
	jne	.L8
	xor	%esi, %edi
	jne	.L8
	add	$-8, %ecx
	jne	.L7
	mov	cycle_count, %esi /* CCREG */
	ret
.L8:
	add	$8, %rsp /* pop return address, we're not returning */
	mov	%r12d, %edi
	call	remove_hash
	mov	%r12d, %edi
	call	new_recompile_block
	mov	%r12d, %edi
	call	get_addr
	mov	cycle_count, %esi
	jmp	*%rax
	.size	verify_code, .-verify_code

.globl fp_exception
	.type	fp_exception, @function
fp_exception:
	mov	$0x1000002c, %edx
.fpe:
	mov	reg_cop0+48, %ebx
	or	$2, %ebx
	mov	%ebx, reg_cop0+48 /* Status */
	mov	%edx, reg_cop0+52 /* Cause */
	mov	%eax, reg_cop0+56 /* EPC */
	mov	%esi, %ebx
	mov	$0x80000180, %edi
	call	get_addr_ht
	mov	%ebx, %esi
	jmp	*%rax
	.size	fp_exception, .-fp_exception

.globl fp_exception_ds
	.type	fp_exception_ds, @function
fp_exception_ds:
	mov	$0x9000002c, %edx /* Set high bit if delay slot */
	jmp	.fpe
	.size	fp_exception_ds, .-fp_exception_ds

.globl jump_syscall
	.type	jump_syscall, @function
jump_syscall:
	mov	$0x20, %edx
	mov	reg_cop0+48, %ebx
	or	$2, %ebx
	mov	%ebx, reg_cop0+48 /* Status */
	mov	%edx, reg_cop0+52 /* Cause */
	mov	%eax, reg_cop0+56 /* EPC */
	mov	%esi, %ebx
	mov	$0x80000180, %edi
	call	get_addr_ht
	mov	%ebx, %esi
	jmp	*%rax
	.size	jump_syscall, .-jump_syscall

.globl cc_interrupt
	.type	cc_interrupt, @function
cc_interrupt:
	add	last_count, %esi
	mov	%esi, reg_cop0+36 /* Count */
	call	gen_interupt
	mov	reg_cop0+36, %esi
	mov	next_interupt, %eax
	mov	pending_exception, %ebx
	mov	stop, %ecx
	mov	%eax, last_count
	sub	%eax, %esi
	test	%ecx, %ecx
	jne	.L10
	test	%ebx, %ebx
	jne	.L9
	ret
.L9:
	mov	pcaddr, %edi
	mov	%esi, cycle_count /* CCREG */
	call	get_addr_ht
	mov	cycle_count, %esi
	add	$8, %rsp /* pop return address */
	jmp	*%rax
.L10:
	pop	%rbp /* pop return address and discard it */
	pop	%r15 /* restore callee-save registers */
	pop	%r14
	pop	%r13
	pop	%r12
	pop	%rbx
	pop	%rbp
	ret	     /* exit dynarec */
	.size	cc_interrupt, .-cc_interrupt

.globl new_dyna_start
	.type	new_dyna_start, @function
new_dyna_start:
	push	%rbp
	push	%rbx
	push	%r12
	push	%r13
	push	%r14
	push	%r15
	mov	$0xa4000040, %edi
	call	new_recompile_block
	movl	next_interupt, %edi
	movl	reg_cop0+36, %esi
	movl	%edi, last_count
	subl	%edi, %esi
	jmp	0x7000000
	.size	new_dyna_start, .-new_dyna_start

.globl jump_vaddr
	.type	jump_vaddr, @function
jump_vaddr:
  /* Check hash table */
	mov	%eax, %edi
	shr	$16, %eax
	xor	%edi, %eax
	movzwl	%ax, %eax
	shl	$4, %eax
	cmp	hash_table(%eax), %edi
	jne	.L12
.L11:
	mov	hash_table+4(%eax), %edi
	jmp	*%rdi
.L12:
	cmp	hash_table+8(%eax), %edi
	lea	8(%eax), %eax
	je	.L11
  /* No hit on hash table, call compiler */
	mov	%esi, cycle_count /* CCREG */
	call	get_addr
	mov	cycle_count, %esi
	jmp	*%rax
	.size	jump_vaddr, .-jump_vaddr

