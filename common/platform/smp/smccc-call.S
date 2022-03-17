/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2015, Linaro Limited
 */
 
#define __ALIGN .align 0
#define __ALIGN_STR ".align 0"

#define ALIGN __ALIGN
#define ALIGN_STR __ALIGN_STR

#ifndef ASM_NL
#define ASM_NL		 ;
#endif

#ifndef ENTRY
#define ENTRY(name) \
	.globl name ASM_NL \
	ALIGN ASM_NL \
	name:
#endif

#ifndef END
#define END(name) \
	.size name, .-name
#endif

#define ENDPROC(name) \
  .type name, %function; \
  END(name)

	.macro SMCCC_SMC
	smc #0
	.endm

	.macro SMCCC instr
	mov	r12, sp
	push	{r4-r7}
	ldm	r12, {r4-r7}
	\instr
	pop	{r4-r7}
	ldr	r12, [sp, #(4 * 4)]
	stm	r12, {r0-r3}
	bx	lr
	.endm

ENTRY(__arm_smccc_smc)
	SMCCC SMCCC_SMC
ENDPROC(__arm_smccc_smc)
