/*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ST_CONFIG_H
#define _ST_CONFIG_H

typedef struct {
    unsigned int VTOR;
    unsigned int SHCSR;
} SCBContext;

#define SCB ((SCBContext *)0x10000)

#define SCB_SHCSR_MEMFAULTENA_Msk 1

typedef struct {
    unsigned int CTRL;
    unsigned int RNR;
    unsigned int RBAR;
    unsigned int RASR;
    unsigned int TYPE;
} MPUContext;

#define MPU ((MPUContext *)0x10000)
#define MPU_CTRL_ENABLE_Msk         1
#define MPU_CTRL_PRIVDEFENA_Pos     1
#define MPU_CTRL_PRIVDEFENA_Msk     1
#define MPU_TYPE_DREGION_Msk        1
#define MPU_RASR_S_Pos              1
#define MPU_RASR_S_Msk              1
#define MPU_RASR_AP_Msk             1
#define MPU_RASR_AP_Pos             1
#define MPU_RASR_C_Pos              1
#define MPU_RASR_B_Pos              1
#define MPU_RASR_ENABLE_Msk         1
#define MPU_RASR_SIZE_Pos           1
#define MPU_RASR_SIZE_Msk           1
#define MPU_RASR_XN_Pos             1
#define MPU_RASR_XN_Msk             1
#define MPU_RBAR_ADDR_Msk           1

#define __get_IPSR()                0
#define __DSB()
#define __WFI()
#define __ISB()

#define NVIC_EnableIRQ(vector)
#define NVIC_SetPriority(vector, pri)
#define NVIC_DisableIRQ(vector)

typedef struct {
    unsigned int CTRL;
    unsigned int LOAD;
    unsigned int VAL;
} SysTickCB;
#define SysTick ((SysTickCB *)0x100000)

#define SysTick_Config(vector)        0
#define NVIC_ClearPendingIRQ(vector)
#define SysTick_CTRL_ENABLE_Msk       1

#define SysTick_IRQn                  0
#define NonMaskableInt_IRQn           1
#define MemoryManagement_IRQn         2
#define BusFault_IRQn                 3
#define UsageFault_IRQn               4
#define SVCall_IRQn                   5
#define PendSV_IRQn                   6

#endif
