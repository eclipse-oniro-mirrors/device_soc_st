
#include "los_tick.h"
#include "los_task.h"
#include "los_config.h"
#include "los_interrupt.h"
#include "los_debug.h"
#include "los_compiler.h"
#include "los_swtmr.h"
#include "los_sched.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#define OS_SLEEP_S(val) ((val) * 1000)
#define OS_SAMPLE_TASK_STACK_SIZE 0x1000
#define SWTMR1_SLEEP_PERIOD 1000
#define SWTMR2_SLEEP_PERIOD 2000
#define SWTMR3_SLEEP_PERIOD 8000
#define SWTMR4_SLEEP_PERIOD 10000
STATIC UINT32 g_task1;
STATIC UINT32 g_task2;
STATIC UINT32 g_task3;
STATIC UINT32 g_task4;
STATIC UINT32 g_swtmr1;
STATIC UINT32 g_swtmr2;
STATIC UINT32 g_swtmr3;
STATIC UINT32 g_swtmr4;
STATIC UINT32 g_swtmr1TestCount;
STATIC UINT32 g_swtmr2TestCount;
STATIC UINT32 g_swtmr3TestCount;
STATIC UINT32 g_swtmr4TestCount;

STATIC VOID SwtmrSample1(UINT32 arg)
{
    if (arg == 1) {
        g_swtmr1TestCount++;
    } else if (arg == 2) {
        g_swtmr2TestCount++;
    } else if (arg == 3) {
        g_swtmr3TestCount++;
    } else if (arg == 4) {
        g_swtmr4TestCount++;
    }
}

STATIC VOID TaskSampleEntry5(VOID)
{
    UINT16 pri;
    UINT32 ret;

    pri = LOS_TaskPriGet(LOS_CurTaskIDGet());
    if (pri != 6) {
        printf("TaskSampleEntry5 LOS_TaskPriGet pri :%u failed, ERROR: 0x%x\n", pri);
    }

    ret = LOS_TaskDelete(LOS_CurTaskIDGet());
    if (ret != LOS_OK) {
        printf("TaskSampleEntry5 exit failed, ERROR: 0x%x\n", ret);
    }
}

STATIC VOID TaskSampleEntry4(VOID)
{
    UINT32 taskID;
    UINT32 ret;
    TSK_INIT_PARAM_S task = { 0 };
    task.pfnTaskEntry = (TSK_ENTRY_FUNC)TaskSampleEntry5;
    task.uwStackSize  = OS_SAMPLE_TASK_STACK_SIZE;
    task.pcName       = "taskSampleEntry5";
    task.usTaskPrio   = 30;

    while (1) {
        printf("TaskSampleEntry4 running...\n\r");

        ret = LOS_TaskCreate(&taskID, &task);
        if (ret != LOS_OK) {
            printf("TaskSampleEntry4 create task failed, ERROR : 0x%x\n", ret);
        }

        ret = LOS_TaskPriSet(taskID, 6);
        if (ret != LOS_OK) {
            printf("TaskSampleEntry4 LOS_TaskPriSet task :%u pri 6 failed, ERROR: 0x%x\n", taskID, ret);
        }

        LOS_TaskDelay(OS_SLEEP_S(2));
    }
}

STATIC VOID TaskSampleEntry3(VOID)
{
    UINT32 ret;

    while (1) {
        printf("TaskSampleEntry3 running...\n\r");

        LOS_TaskYield();

        ret = LOS_SwtmrStart(g_swtmr1);
        if (ret != LOS_OK) {
            printf("TaskSampleEntry3 LOS_SwtmrStart g_swtmr1 failed, ERROR: 0x%x\n", ret);
        }

        LOS_TaskDelay(OS_SLEEP_S(10));

        ret = LOS_SwtmrStop(g_swtmr1);
        if (ret != LOS_OK) {
            printf("TaskSampleEntry3 LOS_SwtmrStop g_swtmr1 failed, ERROR: 0x%x\n", ret);
        }

        LOS_TaskDelay(OS_SLEEP_S(1));
    }
}

STATIC VOID TaskSampleEntry2(VOID)
{
    UINT32 ret;

    while (1) {
        printf("TaskSampleEntry2 running... swtmr1: %u swtmr2: %u swtmr3: %u swtmr4: %u\n\r",
               g_swtmr1TestCount, g_swtmr2TestCount, g_swtmr3TestCount, g_swtmr4TestCount);

        ret = LOS_TaskSuspend(g_task1);
        if (ret != LOS_OK) {
            printf("TaskSampleEntry2 LOS_TaskSuspend g_task1 failed, ERROR: 0x%x\n", ret);
        }

        LOS_TaskDelay(OS_SLEEP_S(2));

        ret = LOS_TaskResume(g_task1);
        if (ret != LOS_OK) {
            printf("TaskSampleEntry2 LOS_TaskResume g_task1 failed, ERROR: 0x%x\n", ret);
        }
    }
}

#define BACKTRACE_LR_SIZE 10
STATIC VOID TaskSampleEntry1(VOID)
{
    UINT32 LR[BACKTRACE_LR_SIZE] = { 0 };
    UINT32 ret;
    ret = LOS_SwtmrStart(g_swtmr2);
    if (ret != LOS_OK) {
        printf("TaskSampleEntry1 LOS_SwtmrStart g_swtmr2 failed, ERROR: 0x%x\n", ret);
    }

    ret = LOS_SwtmrStart(g_swtmr3);
    if (ret != LOS_OK) {
        printf("TaskSampleEntry1 LOS_SwtmrStart g_swtmr3 failed, ERROR: 0x%x\n", ret);
    }

    ret = LOS_SwtmrStart(g_swtmr4);
    if (ret != LOS_OK) {
        printf("TaskSampleEntry1 LOS_SwtmrStart g_swtmr4 failed, ERROR: 0x%x\n", ret);
    }

    LOS_RecordLR(LR, BACKTRACE_LR_SIZE, 0, 0);

    while (1) {
        printf("TaskSampleEntry1 running... time : %u s\n\r", (UINT32)(OsGetCurrSchedTimeCycle() / OS_SYS_CLOCK));

        LOS_TaskDelay(OS_SLEEP_S(1));
    }
}


STATIC UINT32 SwtmrSample(VOID)
{
    UINT32 ret;
    g_swtmr1TestCount = 0;
    g_swtmr2TestCount = 0;
    g_swtmr3TestCount = 0;
    g_swtmr4TestCount = 0;

#if (LOSCFG_BASE_CORE_SWTMR_ALIGN == 1)
    ret = LOS_SwtmrCreate(SWTMR1_SLEEP_PERIOD, LOS_SWTMR_MODE_PERIOD, SwtmrSample1, &g_swtmr1, 1,
                          OS_SWTMR_ROUSES_ALLOW, OS_SWTMR_ALIGN_SENSITIVE);
    if (ret != LOS_OK) {
        return ret;
    }

    ret = LOS_SwtmrCreate(SWTMR2_SLEEP_PERIOD, LOS_SWTMR_MODE_PERIOD, SwtmrSample1, &g_swtmr2, 2,
                          OS_SWTMR_ROUSES_ALLOW, OS_SWTMR_ALIGN_SENSITIVE);
    if (ret != LOS_OK) {
        return ret;
    }

    ret = LOS_SwtmrCreate(SWTMR3_SLEEP_PERIOD, LOS_SWTMR_MODE_PERIOD, SwtmrSample1, &g_swtmr3, 3,
                          OS_SWTMR_ROUSES_ALLOW, OS_SWTMR_ALIGN_SENSITIVE);
    if (ret != LOS_OK) {
        return ret;
    }

    ret = LOS_SwtmrCreate(SWTMR4_SLEEP_PERIOD, LOS_SWTMR_MODE_PERIOD, SwtmrSample1, &g_swtmr4, 4,
                          OS_SWTMR_ROUSES_ALLOW, OS_SWTMR_ALIGN_SENSITIVE);
    if (ret != LOS_OK) {
        return ret;
    }
#else
    ret = LOS_SwtmrCreate(SWTMR1_SLEEP_PERIOD, LOS_SWTMR_MODE_PERIOD, SwtmrSample1, &g_swtmr1, 1);
    if (ret != LOS_OK) {
        return ret;
    }

    ret = LOS_SwtmrCreate(SWTMR2_SLEEP_PERIOD, LOS_SWTMR_MODE_PERIOD, SwtmrSample1, &g_swtmr2, 2);
    if (ret != LOS_OK) {
        return ret;
    }

    ret = LOS_SwtmrCreate(SWTMR3_SLEEP_PERIOD, LOS_SWTMR_MODE_PERIOD, SwtmrSample1, &g_swtmr3, 3);
    if (ret != LOS_OK) {
        return ret;
    }

    ret = LOS_SwtmrCreate(SWTMR4_SLEEP_PERIOD, LOS_SWTMR_MODE_PERIOD, SwtmrSample1, &g_swtmr4, 4);
    if (ret != LOS_OK) {
        return ret;
    }
#endif

    return LOS_OK;
}

UINT32 SemSample(VOID)
{
    UINT32 semID = 0;
    UINT32 ret = LOS_SemCreate(1, &semID);
    if (ret != LOS_OK) {
        return ret;
    }

    ret = LOS_SemPend(semID, 10);
    if (ret != LOS_OK) {
        return ret;
    }

    ret = LOS_SemPost(semID);
    if (ret != LOS_OK) {
        return ret;
    }

    ret = LOS_SemDelete(semID);
    if (ret != LOS_OK) {
        return ret;
    }

    return 0;
}

UINT32 MuxSample(VOID)
{
    UINT32 muxID = 0;
    UINT32 ret = LOS_MuxCreate(&muxID);
    if (ret != LOS_OK) {
        return ret;
    }

    ret = LOS_MuxPend(muxID, 10);
    if (ret != LOS_OK) {
        return ret;
    }

    ret = LOS_MuxPost(muxID);
    if (ret != LOS_OK) {
        return ret;
    }

    ret = LOS_MuxDelete(muxID);
    if (ret != LOS_OK) {
        return ret;
    }

    return 0;
}

UINT32 EventSample(VOID)
{
    EVENT_CB_S event = { 0 };

    UINT32 ret = LOS_EventInit(&event);
    if (ret != LOS_OK) {
        return ret;
    }

    ret = LOS_EventRead(&event, 0xff, LOS_WAITMODE_OR, LOS_WAIT_FOREVER);
    if (ret != LOS_OK) {
        return ret;
    }

    ret = LOS_EventWrite(&event, 0x1);
    if (ret != LOS_OK) {
        return ret;
    }

    ret = LOS_EventClear(&event, 0);
    if (ret != LOS_OK) {
        return ret;
    }

    ret = LOS_EventDestroy(&event);
    if (ret != LOS_OK) {
        return ret;
    }

    return 0;
}

UINT32 TaskSample(VOID)
{
    UINT32  ret;
    TSK_INIT_PARAM_S task = { 0 };
    task.pfnTaskEntry = (TSK_ENTRY_FUNC)TaskSampleEntry1;
    task.uwStackSize  = OS_SAMPLE_TASK_STACK_SIZE;
    task.pcName       = "taskSampleEntry1";
    task.usTaskPrio   = 6;
    ret = LOS_TaskCreate(&g_task1, &task);
    if (ret != LOS_OK) {
        printf("task1 create failed\n");
    }

    task.pfnTaskEntry = (TSK_ENTRY_FUNC)TaskSampleEntry2;
    task.uwStackSize  = OS_SAMPLE_TASK_STACK_SIZE;
    task.pcName       = "taskSampleEntry2";
    task.usTaskPrio   = 7;
    ret = LOS_TaskCreate(&g_task2, &task);
    if (ret != LOS_OK) {
        printf("task2 create failed\n");
    }

    task.pfnTaskEntry = (TSK_ENTRY_FUNC)TaskSampleEntry3;
    task.uwStackSize  = OS_SAMPLE_TASK_STACK_SIZE;
    task.pcName       = "taskSampleEntry3";
    task.usTaskPrio   = 8;
    ret = LOS_TaskCreate(&g_task3, &task);
    if (ret != LOS_OK) {
        printf("task2 create failed\n");
    }

    ret = SwtmrSample();
    if (ret != LOS_OK) {
        return ret;
    }

    task.pfnTaskEntry = (TSK_ENTRY_FUNC)TaskSampleEntry4;
    task.uwStackSize  = OS_SAMPLE_TASK_STACK_SIZE;
    task.pcName       = "taskSampleEntry4";
    task.usTaskPrio   = 8;
    ret = LOS_TaskCreate(&g_task4, &task);
    if (ret != LOS_OK) {
        return ret;
    }

    SemSample();
    MuxSample();
    EventSample();

    return LOS_OK;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */
