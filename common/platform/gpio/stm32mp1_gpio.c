/* 
 * Copyright (c) 2021 Nanjing Xiaoxiongpai Intelligent Technology CO., LIMITED.
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

#include "stm32mp1_gpio.h"


static inline struct Mp15xGpioCntlr *ToMp15xGpioCntlr(struct GpioCntlr *cntlr)
{
    return (struct Mp15xGpioCntlr *)cntlr;
}

static inline uint16_t Mp15xToGroupNum(uint16_t gpio)
{
    return (uint16_t)(gpio / g_Mp15xGpioCntlr.bitNum);
}

static inline uint16_t Mp15xToBitNum(uint16_t gpio)
{
    return (uint16_t)(gpio % g_Mp15xGpioCntlr.bitNum);
}
static inline uint16_t Mp15xToGpioNum(uint16_t group, uint16_t bit)
{
    return (uint16_t)(group * g_Mp15xGpioCntlr.bitNum + bit);
}

static int32_t Mp15xGetGroupByGpioNum(struct GpioCntlr *cntlr, uint16_t gpio, struct GpioGroup **group)
{
    struct Mp15xGpioCntlr *stm32gpio = NULL;
    uint16_t groupIndex = Mp15xToGroupNum(gpio);

    if (cntlr == NULL || cntlr->priv == NULL) {
        HDF_LOGE("%s: cntlr or priv is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    stm32gpio = ToMp15xGpioCntlr(cntlr);
    if (groupIndex >= stm32gpio->groupNum) {
        HDF_LOGE("%s: err group index:%u", __func__, groupIndex);
        return HDF_ERR_INVALID_PARAM;
    }
    *group = &stm32gpio->groups[groupIndex];
    return HDF_SUCCESS;
}


static int32_t Mp15xGpioSetDir(struct GpioCntlr *cntlr, uint16_t gpio, uint16_t dir)
{
    int32_t ret;
    
    unsigned int val;
    volatile unsigned char *addr = NULL;

    unsigned int bitNum = Mp15xToBitNum(gpio);
    struct GpioGroup *group = NULL;

    ret = Mp15xGetGroupByGpioNum(cntlr, gpio, &group);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("Mp15xGetGroupByGpioNum failed\n");
        return ret;
    }

    if (OsalSpinLockIrqSave(&group->lock, &group->irqSave) != HDF_SUCCESS) {
        HDF_LOGE("OsalSpinLockIrqSave failed\n");
        return HDF_ERR_DEVICE_BUSY;
    }
    addr = STM32MP15X_GPIO_MODER(group->regBase);
    val = OSAL_READL(addr);
    if (dir == GPIO_DIR_IN) {
        val &= ~(0X3 << (bitNum*2)); /* bit0:1 清零 */
    } else if (dir == GPIO_DIR_OUT) {
        val &= ~(0X3 << (bitNum*2)); /* bit0:1 清零 */
        val |= (0X1 << (bitNum*2)); /* bit0:1 设置 01 */
    }
    OSAL_WRITEL(val, addr);
    (void)OsalSpinUnlockIrqRestore(&group->lock, &group->irqSave);
    return HDF_SUCCESS;
}
static int32_t Mp15xGpioGetDir(struct GpioCntlr *cntlr, uint16_t gpio, uint16_t *dir)
{
    int32_t ret;
    unsigned int val;
    volatile unsigned char *addr = NULL;
    unsigned int bitNum = Mp15xToBitNum(gpio);
    struct GpioGroup *group = NULL;

    ret = Mp15xGetGroupByGpioNum(cntlr, gpio, &group);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    addr = STM32MP15X_GPIO_MODER(group->regBase);
    val = OSAL_READL(addr);
    if (val & (1 << (bitNum*2))) {
        *dir = GPIO_DIR_OUT;
    } else {
        *dir = GPIO_DIR_IN;
    }
    return HDF_SUCCESS;
}
static int32_t Mp15xGpioWrite(struct GpioCntlr *cntlr, uint16_t gpio, uint16_t val)
{
    int32_t ret;
    
    unsigned int valCur;
    unsigned int bitNum = Mp15xToBitNum(gpio);
    volatile unsigned char *addr = NULL;
    struct GpioGroup *group = NULL;

    ret = Mp15xGetGroupByGpioNum(cntlr, gpio, &group);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    if (OsalSpinLockIrqSave(&group->lock, &group->irqSave) != HDF_SUCCESS) {
        return HDF_ERR_DEVICE_BUSY;
    }
    addr = STM32MP15X_GPIO_BSRR(group->regBase);
    valCur = OSAL_READL(addr);
    if (val == GPIO_VAL_LOW) {
        valCur &= ~(0x1 << bitNum);
        valCur |= (0x1 << (bitNum+16));
    } else {
        valCur |= (0x1 << bitNum);
    }
    OSAL_WRITEL(valCur, addr);
    (void)OsalSpinUnlockIrqRestore(&group->lock, &group->irqSave);

    return HDF_SUCCESS;
}

static int32_t Mp15xGpioRead(struct GpioCntlr *cntlr, uint16_t gpio, uint16_t *val)
{
    int32_t ret;
    unsigned int valCur;
    volatile unsigned char *addr = NULL;
    unsigned int bitNum = Mp15xToBitNum(gpio);
    struct GpioGroup *group = NULL;

    ret = Mp15xGetGroupByGpioNum(cntlr, gpio, &group);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    addr = STM32MP15X_GPIO_IDR(group->regBase);
    valCur = OSAL_READL(addr);
    if (valCur & (1 << bitNum)) {
        *val = GPIO_VAL_HIGH;   
    } else{
        *val = GPIO_VAL_LOW;
    }
    return HDF_SUCCESS;
}

static uint32_t IrqHandleNoShare(uint32_t irq, void *data)
{
    unsigned int i;
    (void)irq;
    struct GpioGroup *group = (struct GpioGroup *)data;

    if (data == NULL) {
        HDF_LOGW("%s: data is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    for (i = 0; i < g_Mp15xGpioCntlr.bitNum; i++) {
        if(__HAL_GPIO_EXTI_GET_IT(1<<i,group->exitBase) != 0)
        {
            __HAL_GPIO_EXTI_CLEAR_IT(1<<i,group->exitBase);
            GpioCntlrIrqCallback(&g_Mp15xGpioCntlr.cntlr, Mp15xToGpioNum(group->index, i));
        }
    }
    return HDF_SUCCESS;
}

static uint32_t  GetGpioIrqNum(uint16_t pinNum)
{
    if(pinNum > 15)
    {
        HDF_LOGE("%s: get gpio irq num fail!", __func__);
        return 0;
    }
    switch (pinNum)
    {
    case 0:
        return EXTI0_IRQn;
        break;
    case 1:
        return EXTI1_IRQn;
        break;
    case 2:
        return EXTI2_IRQn;
        break;
    case 3:
        return EXTI3_IRQn;
        break;
    case 4:
        return EXTI4_IRQn;
        break;
    case 5:
        return EXTI5_IRQn;
        break;
    case 6:
        return EXTI6_IRQn;
        break;
    case 7:
        return EXTI7_IRQn;
        break;
    case 8:
        return EXTI8_IRQn;
        break;
    case 9:
        return EXTI9_IRQn;
        break;
    case 10:
        return EXTI10_IRQn;
        break;
    case 11:
        return EXTI11_IRQn;
        break;
    case 12:
        return EXTI12_IRQn;
        break;
    case 13:
        return EXTI13_IRQn;
        break;
    case 14:
        return EXTI14_IRQn;
        break;
    case 15:
        return EXTI15_IRQn;
        break;
    default:
        break;
    }
    return 0;
}

static int32_t GpioRegisterGroupIrqUnsafe(uint16_t pinNum, struct GpioGroup *group)
{
    int ret;
    
    ret = OsalRegisterIrq(GetGpioIrqNum(pinNum), 0, IrqHandleNoShare, "GPIO", group);
    if (ret != 0) {
        (void)OsalUnregisterIrq(GetGpioIrqNum(pinNum), group);
        ret = OsalRegisterIrq(GetGpioIrqNum(pinNum), 0, IrqHandleNoShare, "GPIO", group);
    }
    
    if (ret != 0) {
        HDF_LOGE("%s: irq reg fail:%d!", __func__, ret);
        return HDF_FAILURE;
    }
        
    ret = OsalEnableIrq(GetGpioIrqNum(pinNum));
    if (ret != 0) {
        HDF_LOGE("%s: irq enable fail:%d!", __func__, ret);     
        (void)OsalUnregisterIrq(GetGpioIrqNum(pinNum), group);
        return HDF_FAILURE;
    }
        
    group->irqFunc = IrqHandleNoShare;
    
    return HDF_SUCCESS;
}
static void GpioClearIrqUnsafe(struct GpioGroup *group, uint16_t bitNum)
{
    __HAL_GPIO_EXTI_CLEAR_IT(bitNum,group->exitBase);
}
static int32_t Mp15xGpioSetIrq(struct GpioCntlr *cntlr, uint16_t gpio, uint16_t mode)
{
    int32_t ret = HDF_SUCCESS;
    struct GpioGroup *group = NULL;
    (void)mode;
    unsigned int bitNum = Mp15xToBitNum(gpio);
    
    ret = Mp15xGetGroupByGpioNum(cntlr, gpio, &group);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    Mp15xGpioSetDir(cntlr, gpio, GPIO_DIR_IN);
    
    if (OsalSpinLockIrqSave(&group->lock, &group->irqSave) != HDF_SUCCESS) {
        return HDF_ERR_DEVICE_BUSY;
    }

    EXTI_ConfigTypeDef EXTI_ConfigStructure;
    EXTI_HandleTypeDef hexti;

    EXTI_ConfigStructure.Line = EXTI_GPIO | EXTI_EVENT | EXTI_REG1 |bitNum;
    EXTI_ConfigStructure.Trigger = EXTI_TRIGGER_FALLING;
    EXTI_ConfigStructure.GPIOSel = Mp15xToGroupNum(gpio);
    EXTI_ConfigStructure.Mode = EXTI_MODE_C1_INTERRUPT;

    HAL_EXTI_SetConfigLine(&hexti, &EXTI_ConfigStructure);
    GpioClearIrqUnsafe(group, bitNum);        // clear irq on set
    if (group->irqFunc != NULL) {
        (void)OsalSpinUnlockIrqRestore(&group->lock, &group->irqSave);
        HDF_LOGI("%s: group irq(%p) already registered!", __func__, group->irqFunc);
        return HDF_SUCCESS;
    }
    ret = GpioRegisterGroupIrqUnsafe(bitNum, group);
    (void)OsalSpinUnlockIrqRestore(&group->lock, &group->irqSave);
    HDF_LOGI("%s: group irq(%p) registered!", __func__, group->irqFunc);
    return ret;
}

static int32_t Mp15xGpioUnsetIrq(struct GpioCntlr *cntlr, uint16_t gpio)
{
    int32_t ret = HDF_SUCCESS;
    (void)gpio;
    struct Mp15xGpioCntlr *gCntlr = NULL;

    if (cntlr == NULL || cntlr->priv == NULL) {
        HDF_LOGE("%s: GpioCntlr or cntlr.priv null!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    gCntlr = (struct Mp15xGpioCntlr *)cntlr->priv;


    return ret;
}

static int32_t Mp15xGpioEnableIrq(struct GpioCntlr *cntlr, uint16_t gpio)
{
    int32_t ret = HDF_SUCCESS;
    struct Mp15xGpioCntlr *gCntlr = NULL;
    struct GpioGroup *group = NULL;
    unsigned int bitNum = Mp15xToBitNum(gpio);

    if (cntlr == NULL || cntlr->priv == NULL) {
        HDF_LOGE("%s: GpioCntlr or cntlr.priv null!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }


    ret = Mp15xGetGroupByGpioNum(cntlr, gpio, &group);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("Mp15xGetGroupByGpioNum failed\n");
        return ret;
    }

    EXTI_ConfigTypeDef EXTI_ConfigStructure;
    EXTI_HandleTypeDef hexti;

    Mp15xGpioSetDir(cntlr,gpio,GPIO_DIR_IN);
    
    EXTI_ConfigStructure.Line = EXTI_GPIO | EXTI_EVENT | EXTI_REG1 |bitNum;
    EXTI_ConfigStructure.Trigger = EXTI_TRIGGER_FALLING;
    EXTI_ConfigStructure.GPIOSel = Mp15xToGroupNum(gpio);
    EXTI_ConfigStructure.Mode = EXTI_MODE_C1_INTERRUPT;
    
    HAL_EXTI_SetConfigLine(&hexti, &EXTI_ConfigStructure);
    gCntlr = (struct Mp15xGpioCntlr *)cntlr->priv;

    return ret;
}

static int32_t Mp15xGpioDisableIrq(struct GpioCntlr *cntlr, uint16_t gpio)
{
    int32_t ret = HDF_SUCCESS;


    if (cntlr == NULL || cntlr->priv == NULL) {
        HDF_LOGE("%s: GpioCntlr or cntlr.priv null!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    struct GpioGroup *group = NULL;

    ret = Mp15xGetGroupByGpioNum(cntlr, gpio, &group);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("Mp15xGetGroupByGpioNum failed\n");
        return ret;
    }

    EXTI_HandleTypeDef hexti;
    HAL_EXTI_ClearConfigLine(&hexti);

    return ret;
}
/* GpioMethod definition */
struct GpioMethod g_GpioMethod = {
    .request = NULL,
    .release = NULL,
    .write = Mp15xGpioWrite,
    .read = Mp15xGpioRead,
    .setDir = Mp15xGpioSetDir,
    .getDir = Mp15xGpioGetDir,
    .toIrq = NULL,
    .setIrq = Mp15xGpioSetIrq,
    .unsetIrq = Mp15xGpioUnsetIrq,
    .enableIrq = Mp15xGpioEnableIrq,
    .disableIrq = Mp15xGpioDisableIrq,
};

static int32_t Mp15xGpioReadDrs(struct Mp15xGpioCntlr *stm32gpio, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("%s: invalid drs ops fail!", __func__);
        return HDF_FAILURE;
    }

    ret = drsOps->GetUint32(node, "gpioRegBase", &stm32gpio->gpioPhyBase, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read regBase fail!", __func__);
        return ret;
    }

    ret = drsOps->GetUint32(node, "gpioRegStep", &stm32gpio->gpioRegStep, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read gpioRegStep fail!", __func__);
        return ret;
    }

    ret = drsOps->GetUint16(node, "groupNum", &stm32gpio->groupNum, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read groupNum fail!", __func__);
        return ret;
    }

    ret = drsOps->GetUint16(node, "bitNum", &stm32gpio->bitNum, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read bitNum fail!", __func__);
        return ret;
    }

    ret = drsOps->GetUint32(node, "irqRegBase", &stm32gpio->irqPhyBase, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read regBase fail!", __func__);
        return ret;
    }

    ret = drsOps->GetUint32(node, "irqRegStep", &stm32gpio->iqrRegStep, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read gpioRegStep fail!", __func__);
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t InitGpioCntlrMem(struct Mp15xGpioCntlr *cntlr)
{
    size_t groupMemSize;
    struct GpioGroup *groups = NULL;

    if (cntlr == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    groupMemSize = sizeof(struct GpioGroup) * cntlr->groupNum;
    groups = (struct GpioGroup *)OsalMemCalloc(groupMemSize);
    if (groups == NULL) {
        return HDF_ERR_MALLOC_FAIL;
    }
    cntlr->groups = groups;

    for (uint16_t i = 0; i < cntlr->groupNum; i++) {
        groups[i].index = i;
        groups[i].regBase = cntlr->regBase + (i * cntlr->gpioRegStep);
        groups[i].exitBase = cntlr->exitBase;
        if (OsalSpinInit(&groups[i].lock) != HDF_SUCCESS) {
            for (; i > 0; i--) {
                (void)OsalSpinDestroy(&groups[i - 1].lock);
            }
            OsalMemFree(groups);
            return HDF_FAILURE;
        }
    }
    return HDF_SUCCESS;
}

static void ReleaseGpioCntlrMem(struct Mp15xGpioCntlr *cntlr)
{
    if (cntlr == NULL) {
        return;
    }
    if (cntlr->groups != NULL) {
        for (uint16_t i = 0; i < cntlr->groupNum; i++) {
            (void)OsalSpinDestroy(&cntlr->groups[i].lock);
        }
        OsalMemFree(cntlr->groups);
        cntlr->groups = NULL;
    }
}

/* HdfDriverEntry hook function implementations */
static int32_t GpioDriverBind(struct HdfDeviceObject *device)
{   
    (void)device;
    return HDF_SUCCESS;
    
}

static int32_t GpioDriverInit(struct HdfDeviceObject *device)
{
   
    int32_t ret;
    struct Mp15xGpioCntlr *stm32gpio = &g_Mp15xGpioCntlr;

    dprintf("%s: Enter", __func__);
    if (device == NULL || device->property == NULL) {
        HDF_LOGE("%s: device or property NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    //获取属性数据
    ret = Mp15xGpioReadDrs(stm32gpio, device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: get gpio device resource fail:%d", __func__, ret);
        return ret;
    }

    if (stm32gpio->groupNum > GROUP_MAX || stm32gpio->groupNum <= 0 || stm32gpio->bitNum > BIT_MAX ||
        stm32gpio->bitNum <= 0) {
        HDF_LOGE("%s: invalid groupNum:%u or bitNum:%u", __func__, stm32gpio->groupNum,
                 stm32gpio->bitNum);
        return HDF_ERR_INVALID_PARAM;
    }
    //寄存器地址映射
    stm32gpio->regBase = OsalIoRemap(stm32gpio->gpioPhyBase, stm32gpio->groupNum * stm32gpio->gpioRegStep);
    if (stm32gpio->regBase == NULL) {
        HDF_LOGE("%s: err remap phy:0x%x", __func__, stm32gpio->gpioPhyBase);
        return HDF_ERR_IO;
    }
    /* OsalIoRemap: remap registers */
    stm32gpio->exitBase = OsalIoRemap(stm32gpio->irqPhyBase, stm32gpio->iqrRegStep);
    if (stm32gpio->exitBase == NULL) {
        dprintf("%s: OsalIoRemap fail!", __func__);
        return -1;
    }

    ret = InitGpioCntlrMem(stm32gpio);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: err init cntlr mem:%d", __func__, ret);
        OsalIoUnmap((void *)stm32gpio->regBase);
        stm32gpio->regBase = NULL;
        return ret;
    }
    stm32gpio->cntlr.count = stm32gpio->groupNum * stm32gpio->bitNum;
    stm32gpio->cntlr.priv = (void *)device->property;
    stm32gpio->cntlr.ops = &g_GpioMethod;
    ret = GpioCntlrAdd(&stm32gpio->cntlr);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: err add controller: %d", __func__, ret);
        return ret;
    }
    HDF_LOGE("%s: dev service:%s init success!", __func__, HdfDeviceGetServiceName(device));
    return ret;
}

static void GpioDriverRelease(struct HdfDeviceObject *device)
{
    struct GpioCntlr *gpioCntlr = NULL;
    struct Mp15xGpioCntlr *stm32gpioGpioCntlr = NULL;

    HDF_LOGD("%s: Enter", __func__);
    if (device == NULL) {
        HDF_LOGE("%s: device is null!", __func__);
        return;
    }

    GpioCntlrRemove(gpioCntlr);

    stm32gpioGpioCntlr = (struct Mp15xGpioCntlr *)gpioCntlr;
    ReleaseGpioCntlrMem(stm32gpioGpioCntlr);
    OsalIoUnmap((void *)stm32gpioGpioCntlr->regBase);
    stm32gpioGpioCntlr->regBase = NULL;
}

/* HdfDriverEntry definition */
struct HdfDriverEntry g_GpioDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_PLATFORM_GPIO",
    .Bind = GpioDriverBind,
    .Init = GpioDriverInit,
    .Release = GpioDriverRelease,
};

/* Init HdfDriverEntry */
HDF_INIT(g_GpioDriverEntry);