/* 
 * Copyright (c) 2022 Nanjing Xiaoxiongpai Intelligent Technology CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

ENTRY(__smp_smc)
	SMCCC SMCCC_SMC
ENDPROC(__smp_smc)
