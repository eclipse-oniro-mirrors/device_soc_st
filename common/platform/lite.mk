# Copyright (c) 2021 HiSilicon (Shanghai) Technologies CO., LIMITED.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

SOC_COMPANY := $(subst $\",,$(LOSCFG_DEVICE_COMPANY))

ST_DRIVERS_ROOT := $(LITEOSTOPDIR)/../../device/soc/$(SOC_COMPANY)/common/platform/

ifeq ($(LOSCFG_DRIVERS_HDF_PLATFORM_I2C), y)
    LITEOS_BASELIB += -lhdf_i2c
    LIB_SUBDIRS    += $(ST_DRIVERS_ROOT)/i2c
endif

ifeq ($(LOSCFG_DRIVERS_HDF_PLATFORM_ADC), y)
    LITEOS_BASELIB += -lhdf_adc
    LIB_SUBDIRS    += $(ST_DRIVERS_ROOT)/adc
endif


ifeq ($(LOSCFG_DRIVERS_HDF_PLATFORM_GPIO), y)
    LITEOS_BASELIB += -lhdf_gpio
    LIB_SUBDIRS    += $(ST_DRIVERS_ROOT)/gpio
endif

ifeq ($(LOSCFG_DRIVERS_HDF_PLATFORM_WATCHDOG), y)
    LITEOS_BASELIB += -lhdf_iwdg
    LIB_SUBDIRS    += $(ST_DRIVERS_ROOT)/iwdg
endif

ifeq ($(LOSCFG_DRIVERS_HDF_PLATFORM_UART), y)
    LITEOS_BASELIB += -lhdf_uart
    LIB_SUBDIRS    += $(ST_DRIVERS_ROOT)/uart
endif

ifeq ($(LOSCFG_KERNEL_SMP), y)
	LIB_SUBDIRS     += $(ST_DRIVERS_ROOT)/smp
	LITEOS_BASELIB += -lsmp
endif

LITEOS_BASELIB += -lstm32mp1x_hal
LIB_SUBDIRS    += $(ST_DRIVERS_ROOT)/stm32mp1xx_hal

LITEOS_BASELIB += -lrng
LIB_SUBDIRS    += $(ST_DRIVERS_ROOT)/rng

# ltdc dirvers
LITEOS_BASELIB += -lltdc

# mmc dirvers
ifeq ($(LOSCFG_DRIVERS_HDF_PLATFORM_MMC), y)
    LITEOS_BASELIB += -lmmc
endif

# wifi dirvers
ifeq ($(LOSCFG_DRIVERS_HDF_WIFI), y)
    LITEOS_BASELIB += -lhdf_vendor_wifi

ifeq ($(LOSCFG_DRIVERS_HI3881), y)
    LITEOS_BASELIB += -lhi3881
endif
endif

LITEOS_LD_PATH += -L$(ST_DRIVERS_ROOT)/libs/ohos/llvm/stm32mp157


