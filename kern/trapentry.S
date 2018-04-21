/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
 TRAPHANDLER_NOEC(handler0, T_DIVIDE)
 TRAPHANDLER_NOEC(handler1, T_DEBUG)
 TRAPHANDLER_NOEC(handler2, T_NMI)
 TRAPHANDLER_NOEC(handler3, T_BRKPT)
 TRAPHANDLER_NOEC(handler4, T_OFLOW)
 TRAPHANDLER_NOEC(handler5, T_BOUND)
 TRAPHANDLER_NOEC(handler6, T_ILLOP)
 TRAPHANDLER_NOEC(handler7, T_DEVICE)

 TRAPHANDLER(handler8, T_DBLFLT)
 TRAPHANDLER(handler9, 9) // not used
 TRAPHANDLER(handler10, T_TSS)
 TRAPHANDLER(handler11, T_SEGNP)
 TRAPHANDLER(handler12, T_STACK)
 TRAPHANDLER(handler13, T_GPFLT)
 TRAPHANDLER(handler14, T_PGFLT)
 TRAPHANDLER(handler15, 15) // not used

 TRAPHANDLER_NOEC(handler16, T_FPERR)
 TRAPHANDLER_NOEC(handler17, T_ALIGN)
 TRAPHANDLER_NOEC(handler18, T_MCHK)
 TRAPHANDLER_NOEC(handler19, T_SIMDERR)


.globl sysenter_handler;
.type sysenter_handler, @function;
.align 2;
sysenter_handler:
/*
 * Lab 3: Your code here for system call handling
 */
 pushl $0
 pushl %edi
 pushl %ebx
 pushl %ecx
 pushl %edx
 pushl %eax
 call syscall
 movl %ebp, %ecx
 movl %esi, %edx
 sysexit


/*
 * Lab 3: Your code here for _alltraps
 */
 .globl _alltraps
 _alltraps:
 	# trap frame
 	pushl %ds
	pushl %es

	movw $(GD_KD), %ax
	movw %ax, %ds
	movw %ax, %es

	pushal
	pushl %esp
	call trap
	addl $4, %esp

.global _trapret
_trapret:
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	addl $0x8, %esp # trap number and error code
	iret


 .data
 .global vectors
 vectors:
 	.long handler0
	.long handler1
	.long handler2
	.long handler3
	.long handler4
	.long handler5
	.long handler6
	.long handler7
	.long handler5
	.long handler8
	.long handler9
	.long handler10
	.long handler11
	.long handler12
	.long handler13
	.long handler14
	.long handler15
	.long handler16
	.long handler17
	.long handler18
	.long handler19
