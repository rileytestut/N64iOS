#define HOST_REGS 8
#define HOST_CCREG 6
#define EXCLUDE_REG 4

#define IMM_PREFETCH 1
#define IMM_WRITE 1
#define INVERTED_CARRY 1
#define DESTRUCTIVE_WRITEBACK 1

/* x86-64 calling convention:
   func(rdi, rsi, rdx, rcx, r8, r9) {return rax;}
   callee-save: %rbp %rbx %r12-%r15 */

#define ARG1_REG 7 /* RDI */
#define ARG2_REG 6 /* RSI */
