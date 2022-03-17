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

#include "stm32mp1_adc.h"
#include "device_resource_if.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "osal_io.h"
#include "osal_mem.h"
#include "osal_time.h"
#include "stm32mp1xx.h"
#include "stm32mp1xx_hal_conf.h"

#define HDF_LOG_TAG stm32mp1_adc

static void Mp15xAdcPinInit(struct Mp15xAdcDevice *stm32mp1)
{
    uint32_t i;
    uint32_t value;
    volatile unsigned char  *gpioBase;
    volatile unsigned char  *comBase;

    gpioBase = OsalIoRemap(MP15X_GPIO_BASE + MP15X_GPIO_MODE_REG_OFFSET,
        MP15X_GPIO_GROUP_NUMBER * MP15X_GPIO_GROUP_SIZE);
    for (i = 0; i < MP15X_ADC_CHANNEL_COUNT_MAX; i++) {
        if (stm32mp1->validChannel[i] == 0 || stm32mp1->pins[i * MP1_ADC_PIN_DATA_WIDTH] >= MP15X_GPIO_GROUP_NUMBER) {
            continue;
        }
        
        value = OSAL_READL(gpioBase);
        value |= (MP15X_GPIO_ANALOG_MODE_MASK << MP15X_GPIO_REG_PIN_SHIFT);
        OSAL_WRITEL(value, MP15X_GPIO_BASE + MP15X_GPIO_MODE_REG_OFFSET);

        RCC->MC_AHB4ENSETR |= 0x1U << stm32mp1->pins[i * MP1_ADC_PIN_DATA_WIDTH];
    }
    RCC->PLL4CR |= 0x1U;
    comBase = OsalIoRemap(MP15X_ADC_COMMON_REG_BASE, MP15X_ADC_COMMON_REG_SIZE);
    OSAL_WRITEL(MP15X_ADC_CKMODE_SEL, comBase + MP15X_ADC_CCR_OFFSET);
    if (stm32mp1->devNum == MP15X_ADC_DEVICE_2) {
        value = stm32mp1->validChannel[MP15X_ADC_VDDCORE_CHANNEL] & 0x1U;     // VddCore
        OSAL_WRITEL(value, stm32mp1->regBase + MP15X_ADC_OR_OFFSET);
        value = stm32mp1->validChannel[MP15X_ADC_VREF_CHANNEL] & 0x1U;        // Vref
        value |= (stm32mp1->validChannel[MP15X_ADC_TSEN_CHANNEL] & 0x1U) << MP15X_ADC_VREF_SHIFT; // Tsen
        value |= (stm32mp1->validChannel[MP15X_ADC_VBAT_CHANNEL] & 0x1U) << MP15X_ADC_VBAT_SHIFT; // Vbat
        OSAL_WRITEL(value, comBase + MP15X_ADC_CCR_OFFSET);
    }
}

static inline void Mp15xAdcReset(struct Mp15xAdcDevice *stm32mp1)
{
    (void)stm32mp1;
}

static inline void Mp15xAdcSetConfig(struct Mp15xAdcDevice *stm32mp1)
{
    uint32_t value;

    value = 0x1U << MP15X_ADC_JQDIS_SHIFT;
    OSAL_WRITEL(value, stm32mp1->regBase + MP15X_ADC_CFGR_OFFSET);
}

static inline void Mp15xAdcSetSampleTime(struct Mp15xAdcDevice *stm32mp1)
{
    uint32_t sampleTime;
    uint32_t i;

    sampleTime = 0;
    for (i = 0; i <MP15X_CHANNLE_NUM_PER_REG; i++) {
            sampleTime |= (stm32mp1->sampleTime & MP15X_SAMPLE_TIME_MASK) << (i * MP15X_SAMPLE_TIME_BITS);
    }
    OSAL_WRITEL(sampleTime, stm32mp1->regBase + MP15X_ADC_SMPR1_OFFSET);
    OSAL_WRITEL(sampleTime, stm32mp1->regBase + MP15X_ADC_SMPR2_OFFSET);
}

static void Mp15xAdcCalibration(struct Mp15xAdcDevice *stm32mp1)
{
    uint32_t value;
    uint32_t delay  = 0;

    OSAL_WRITEL(0, stm32mp1->regBase + MP15X_ADC_CR_OFFSET);
    OSAL_WRITEL(MP15X_ADC_REGULATOR_EN, stm32mp1->regBase + MP15X_ADC_CR_OFFSET);
    while (1) {
        value = OSAL_READL(stm32mp1->regBase + MP15X_ADC_ISR_OFFSET);
        if (((value >> MP15X_ADC_REGULATOR_RDY_SHIFT) & 0x1U) == 1 || delay > MP15X_ADC_CAL_TIME_OUT) {
            break;
        }
        OsalMDelay(1);
        delay++;
    }
    delay = 0;
    value = OSAL_READL(stm32mp1->regBase + MP15X_ADC_CR_OFFSET);
    value |= 0x1U << MP15X_ADC_ADCAL_SHIFT;
    OSAL_WRITEL(value, stm32mp1->regBase + MP15X_ADC_CR_OFFSET);
    while (1) {
        value = OSAL_READL(stm32mp1->regBase + MP15X_ADC_CR_OFFSET);
        if (((value >> MP15X_ADC_ADCAL_SHIFT) & 0x1U) == 0 || delay > MP15X_ADC_CAL_TIME_OUT) {
            break;
        }
        OsalMDelay(1);
        delay++;
    }
}

static inline void Mp15xAdcClkEnable(struct Mp15xAdcDevice *stm32mp1)
{
    (void)stm32mp1;
    static bool hasInit = false;
    if (hasInit == true) {
        return;
    }

    __HAL_RCC_ADC12_CLK_ENABLE();
    hasInit = true;
}

static inline void Mp15xAdcEnable(struct Mp15xAdcDevice *stm32mp1)
{
    uint32_t value = 0;

    value = OSAL_READL(stm32mp1->regBase + MP15X_ADC_CR_OFFSET);
    OSAL_WRITEL((value | 1), stm32mp1->regBase + MP15X_ADC_CR_OFFSET);
}

static inline void Mp15xAdcDeviceInit(struct Mp15xAdcDevice *stm32mp1)
{
    if (!stm32mp1->adcEnable) {
        return;
    }
    Mp15xAdcClkEnable(stm32mp1);
    Mp15xAdcPinInit(stm32mp1);
    Mp15xAdcCalibration(stm32mp1);
    Mp15xAdcReset(stm32mp1);
    Mp15xAdcSetConfig(stm32mp1);
    Mp15xAdcSetSampleTime(stm32mp1);
    Mp15xAdcEnable(stm32mp1);
}

static inline void Mp15xAdcStart(struct Mp15xAdcDevice *stm32mp1)
{
    uint32_t value;

    value = OSAL_READL(stm32mp1->regBase + MP15X_ADC_CR_OFFSET);

    value |= 0x1U << MP15X_ADC_ADSTART_SHIFT;
    value |= MP15X_ADC_ENABLE;
    OSAL_WRITEL(value, stm32mp1->regBase + MP15X_ADC_CR_OFFSET);
}

static int32_t Mp15xAdcOpen(struct AdcDevice *device)
{
    struct Mp15xAdcDevice *stm32mp1 = NULL;

    if (device == NULL) {
        HDF_LOGE("%s: device is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    stm32mp1 = (struct Mp15xAdcDevice *)device;
    if (!stm32mp1->adcEnable) {
        HDF_LOGE("%s: ADC %u is disabled!", __func__, stm32mp1->devNum);
        return HDF_ERR_NOT_SUPPORT;
    }

    Mp15xAdcStart(stm32mp1);
    return HDF_SUCCESS;
}

static int32_t Mp15xAdcClose(struct AdcDevice *device)
{
    (void)device;
    return HDF_SUCCESS;
}

static inline void Mp15xAdcSetSequence(struct Mp15xAdcDevice *stm32mp1, uint32_t channel)
{
    uint32_t value;

    if (channel >= MP15X_ADC_CHANNEL_COUNT_MAX) {
        return;
    }
    value = channel << MP15X_ADC_SQ1_SHIFT;
    OSAL_WRITEL(value, stm32mp1->regBase + MP15X_ADC_SQR1_OFFSET);

    value = 0x1U << channel;
    OSAL_WRITEL(value, stm32mp1->regBase + MP15X_ADC_PCSEL_OFFSET);
}



static int32_t Mp15xAdcRead(struct AdcDevice *device, uint32_t channel, uint32_t *val)
{
    uint32_t value;
    uint32_t delay = 0;
    struct Mp15xAdcDevice *stm32mp1 = NULL;

    if (device == NULL || val == NULL) {
        HDF_LOGE("%s: device or val is NULL!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    if (channel >= MP15X_ADC_CHANNEL_COUNT_MAX) {
        HDF_LOGE("%s: invalid channel: %u!", __func__, channel);
        return HDF_ERR_INVALID_PARAM;
    }

    stm32mp1 = (struct Mp15xAdcDevice *)device;
    if (!stm32mp1->adcEnable) {
        HDF_LOGE("%s: ADC %u is disabled!", __func__, stm32mp1->devNum);
        return HDF_ERR_NOT_SUPPORT;
    }

    Mp15xAdcSetSequence(stm32mp1, channel);
    Mp15xAdcStart(stm32mp1);
    while (true) {
        value = OSAL_READL(stm32mp1->regBase + MP15X_ADC_ISR_OFFSET);
        if ((value & MP15X_ADC_EOC_MASK) != 0 || delay > MP15X_ADC_CONV_TIME_OUT) {
            OSAL_WRITEL(value, stm32mp1->regBase + MP15X_ADC_ISR_OFFSET);
            break;
        }
        OsalUDelay(1);
        delay++;
    }

    value = OSAL_READL(stm32mp1->regBase + MP15X_ADC_DR_OFFSET);
    *val = value >> (MP15X_ADC_DATA_WIDTH_MAX - stm32mp1->dataWidth);

    return HDF_SUCCESS;
}

static const struct AdcMethod g_method = {
    .start = Mp15xAdcOpen,
    .stop = Mp15xAdcClose,
    .read = Mp15xAdcRead,
};

static int32_t Mp15xAdcReadDrs(struct Mp15xAdcDevice *stm32mp1, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL || drsOps->GetUint8Array == NULL || drsOps->GetBool == NULL) {
        HDF_LOGE("%s: invalid drs ops", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }

    ret = drsOps->GetUint32(node, "reg_pbase", &stm32mp1->regBasePhy, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read regBasePhy failed", __func__);
        return ret;
    }

    ret = drsOps->GetUint32(node, "reg_size", &stm32mp1->regSize, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read regSize failed", __func__);
        return ret;
    }

    ret = drsOps->GetUint32(node, "dev_num", &stm32mp1->devNum, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read devNum failed", __func__);
        return ret;
    }

    ret = drsOps->GetUint8Array(node, "channel_enable", stm32mp1->validChannel, MP15X_ADC_CHANNEL_COUNT_MAX, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read validChannel failed", __func__);
        return ret;
    }

    ret = drsOps->GetUint32(node, "sample_time", &stm32mp1->sampleTime, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read sampleTime failed", __func__);
        return ret;
    }

    ret = drsOps->GetUint32(node, "data_width", &stm32mp1->dataWidth, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read dataWidth failed", __func__);
        return ret;
    }

    stm32mp1->adcEnable = drsOps->GetBool(node, "adc_enable");

    ret = drsOps->GetUint8Array(node, "pins", stm32mp1->pins, MP15X_ADC_CHANNEL_COUNT_MAX, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read pin failed", __func__);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t Mp15xAdcParseInit(struct HdfDeviceObject *device, struct DeviceResourceNode *node)
{
    int32_t ret;
    struct Mp15xAdcDevice *stm32mp1 = NULL;
    (void)device;

    stm32mp1 = (struct Mp15xAdcDevice *)OsalMemCalloc(sizeof(*stm32mp1));
    if (stm32mp1 == NULL) {
        HDF_LOGE("%s: alloc stm32mp1 failed", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = Mp15xAdcReadDrs(stm32mp1, node);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read drs failed:%d", __func__, ret);
        goto ERR;
    }

    stm32mp1->regBase = OsalIoRemap(stm32mp1->regBasePhy, stm32mp1->regSize);
    if (stm32mp1->regBase == NULL) {
        HDF_LOGE("%s: remap regbase failed", __func__);
        ret = HDF_ERR_IO;
        goto ERR;
    }

    Mp15xAdcDeviceInit(stm32mp1);
    stm32mp1->device.priv = (void *)node;
    stm32mp1->device.devNum = stm32mp1->devNum;
    stm32mp1->device.ops = &g_method;
    ret = AdcDeviceAdd(&stm32mp1->device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: add adc device:%u failed", __func__, stm32mp1->devNum);
        goto ERR;
    }
    return HDF_SUCCESS;

ERR:
    if (stm32mp1 != NULL) {
        if (stm32mp1->regBase != NULL) {
            OsalIoUnmap((void *)stm32mp1->regBase);
            stm32mp1->regBase = NULL;
        }
        AdcDeviceRemove(&stm32mp1->device);
        OsalMemFree(stm32mp1);
    }
    return ret;
}

static int32_t Mp15xAdcInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct DeviceResourceNode *childNode = NULL;

    HDF_LOGI("%s: Enter", __func__);
    if (device == NULL || device->property == NULL) {
        HDF_LOGE("%s: device or property is null", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = HDF_SUCCESS;
    DEV_RES_NODE_FOR_EACH_CHILD_NODE(device->property, childNode) {
        ret = Mp15xAdcParseInit(device, childNode);
        if (ret != HDF_SUCCESS) {
            break;
        }
    }
    return ret;
}

static void Mp15xAdcRemoveByNode(const struct DeviceResourceNode *node)
{
    int32_t ret;
    int32_t devNum;
    struct AdcDevice *device = NULL;
    struct Mp15xAdcDevice *stm32mp1 = NULL;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("%s: invalid drs ops", __func__);
        return;
    }

    ret = drsOps->GetUint32(node, "devNum", (uint32_t *)&devNum, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read devNum failed", __func__);
        return;
    }

    device = AdcDeviceGet(devNum);
    if (device != NULL && device->priv == node) {
        AdcDevicePut(device);
        AdcDeviceRemove(device);
        stm32mp1 = (struct Mp15xAdcDevice *)device;
        OsalIoUnmap((void *)stm32mp1->regBase);
        OsalMemFree(stm32mp1);
    }
    return;
}

static void Mp15xAdcRelease(struct HdfDeviceObject *device)
{
    const struct DeviceResourceNode *childNode = NULL;

    HDF_LOGI("%s: enter", __func__);
    if (device == NULL || device->property == NULL) {
        HDF_LOGE("%s: device or property is null", __func__);
        return;
    }
    DEV_RES_NODE_FOR_EACH_CHILD_NODE(device->property, childNode) {
        Mp15xAdcRemoveByNode(childNode);
    }
}

static struct HdfDriverEntry g_stm32mp1AdcDriverEntry = {
    .moduleVersion = 1,
    .Init = Mp15xAdcInit,
    .Release = Mp15xAdcRelease,
    .moduleName = "stm32mp157_adc_driver",
};
HDF_INIT(g_stm32mp1AdcDriverEntry);
