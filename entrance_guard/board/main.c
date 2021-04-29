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

#include "los_tick.h"
#include "los_task.h"
#include "los_config.h"
#include "los_interrupt.h"
#include "los_debug.h"
#include "los_compiler.h"
#include "los_sched.h"

VOID TaskSample(VOID);

#if (LOSCFG_BASE_CORE_SCHED_SLEEP == 1)
#define TIMER_DEMO_IRQ                2
#define SLEEP_TIME_CYCLE              90000000UL
#define SLEEP_TIME_MAX_RESPONSE_TIME  ((UINT32)-1)
#define TIM2_INI_PRIORITY             3
unsigned long long GetSleepTimeNs(VOID)
{
    return 0;
}

void SleepTimerStop(void)
{
    return;
}

void SleepTimerStart(unsigned long long sleepTimeNs)
{
    return;
}

void TIM2_IRQHandler(void)
{
    return;
}

unsigned int SleepTimerInit(void)
{
    /* Timer init */

#if (LOSCFG_USE_SYSTEM_DEFINED_INTERRUPT == 1)
    UINT32 ret = HalHwiCreate(TIMER_DEMO_IRQ, TIM2_INI_PRIORITY, 0, TIM2_IRQHandler, 0);
    if (ret != LOS_OK) {
        printf("SleepTimerInit create time2 irq failed! ERROR: 0x%x\n", ret);
        return ret;
    }
#endif

    return LOS_OK;
}
#endif

/*****************************************************************************
 Function    : main
 Description : Main function entry
 Input       : None
 Output      : None
 Return      : None
 *****************************************************************************/
LITE_OS_SEC_TEXT_INIT int main(void)
{
    unsigned int ret;

    /* uart init */

    printf("\n\rhello world!!\n\r");

    ret = LOS_KernelInit();
    if (ret == LOS_OK) {
#if (LOSCFG_BASE_CORE_SCHED_SLEEP == 1)
    ret = LOS_SchedSleepInit(SleepTimerInit, SleepTimerStart, SleepTimerStop, GetSleepTimeNs);
    if (ret != LOS_OK) {
        goto EXIT;
    }
#endif
        TaskSample();
        LOS_Start();
    }

#if (LOSCFG_BASE_CORE_SCHED_SLEEP == 1)
EXIT:
#endif
    while (1) {
        __asm volatile("wfi");
    }
}

