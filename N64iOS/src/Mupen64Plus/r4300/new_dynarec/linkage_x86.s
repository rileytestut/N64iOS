/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - linkage_x86.s                                           *
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
	.file	"linkage_x86.s"
	.bss
	.align 4
.globl rdram
rdram = 0x80000000
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
	movl	jump_in(,%ecx,4), %edx
.L1:
	testl	%edx, %edx
	je	.L3
	movl	(%edx), %edi
	cmpl	%edi, %eax
	je	.L2
	movl	8(%edx), %edx
	jmp	.L1
.L2:
	mov	(%ebx), %edi
	mov	%esi, %ebp
	lea	4(%ebx,%edi,1), %esi
	mov	%eax, %edi
	pusha
	call	add_link
	popa
	mov	4(%edx), %edi
	mov	%ebp, %esi
	lea	-4(%edi), %ebp
	subl	%ebx, %ebp
	movl	%ebp, (%ebx)
	jmp	*%edi
.L3:
	movl	jump_dirty(,%ecx,4), %edx
.L4:
	testl	%edx, %edx
	je	.L6
	movl	(%edx), %edi
	cmpl	%edi, %eax
	je	.L5
	movl	8(%edx), %edx
	jmp	.L4
.L5:
	mov	4(%edx), %edi
	jmp	*%edi
.L6:
	mov	%eax, %edi
	pusha
	call	new_recompile_block
	popa
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
	add	$4, %esp /* pop return address, we're not returning */
	call	remove_hash
	call	new_recompile_block
	call	get_addr
	mov	cycle_count, %esi
	add	$4, %esp /* pop virtual address */
	jmp	*%eax
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
	push	%esi
	push    $0x80000180
	call	get_addr_ht
	pop	%esi
	pop	%esi
	jmp	*%eax
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
	push	%esi
	push    $0x80000180
	call	get_addr_ht
	pop	%esi
	pop	%esi
	jmp	*%eax
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
	push	%edi
	call	get_addr_ht
	mov	cycle_count, %esi
	add	$8, %esp
	jmp	*%eax
.L10:
	pop	%ebp /* pop return address */
	/* dynarec cleanup? */
	pop	%ebp /* restore ebp */
	ret	     /* exit dynarec */
	.size	cc_interrupt, .-cc_interrupt

.globl new_dyna_start
	.type	new_dyna_start, @function
new_dyna_start:
	push	%ebp
	push	$0xa4000040
	call	new_recompile_block
	add	$4, %esp
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
	jmp	*%edi
.L12:
	cmp	hash_table+8(%eax), %edi
	lea	8(%eax), %eax
	je	.L11
  /* No hit on hash table, call compiler */
	push	%edi
	mov	%esi, cycle_count /* CCREG */
	call	get_addr
	mov	cycle_count, %esi
	add	$4, %esp
	jmp	*%eax
	.size	jump_vaddr, .-jump_vaddr

