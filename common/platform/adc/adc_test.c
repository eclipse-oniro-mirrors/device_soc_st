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

#include "los_event.h"
#include "hdf_device_desc.h"
#include "hdf_base.h"
#include "hdf_log.h"
#include "adc_if.h"

#define HDF_LOG_TAG adc_test

static void MainTest(void)
{
    int32_t ret;
    uint32_t devNum = 1;
    uint32_t i;
    uint32_t value;
    DevHandle handle = NULL;

    handle = AdcOpen(devNum);
    if (handle == NULL) {
        HDF_LOGE("%s: handle is null", __func__);
        goto ERR;
    }

    for (i = 0; i < 200; i++) {
        ret = AdcRead(handle, 1, &value);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: read fail", __func__);
            goto ERR;
        }
        HDF_LOGE("%s:------------------------value--is--%u------------------------", __func__, value);
    }

ERR:
    /* 销毁ADC设备句柄 */
    AdcClose(handle); 
}

static int32_t Stm32AdcTestInit(struct HdfDeviceObject *device)
{
    HDF_LOGI("%s: enter", __func__);
    if (device == NULL) {
        HDF_LOGE("%s: device is null", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    MainTest();

    return HDF_SUCCESS;
}

static void Stm32AdcTestRelease(struct HdfDeviceObject *device)
{
    HDF_LOGI("%s: enter", __func__);
    if (device == NULL) {
        HDF_LOGE("%s: device is null", __func__);
        return;
    }
}

struct HdfDriverEntry g_hdfUart32ByteTest = {
    .moduleVersion = 1,
    .moduleName = "stm32mp157_adc_test",
    .Init = Stm32AdcTestInit,
    .Release = Stm32AdcTestRelease,
};

HDF_INIT(g_hdfUart32ByteTest);
