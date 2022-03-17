/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2015, Linaro Limited
 */
#ifndef __ARM_SMCCC_H__
#define __ARM_SMCCC_H__

struct arm_smccc_res {
	unsigned long a0;
	unsigned long a1;
	unsigned long a2;
	unsigned long a3;
};

struct arm_smccc_quirk {
	int	id;
	union {
		unsigned long a6;
	} state;
};

void __arm_smccc_smc(unsigned long a0, unsigned long a1,
			unsigned long a2, unsigned long a3, unsigned long a4,
			unsigned long a5, unsigned long a6, unsigned long a7,
			struct arm_smccc_res *res, struct arm_smccc_quirk *quirk);

#define arm_smccc_smc(...) __arm_smccc_smc(__VA_ARGS__, 0)
#define arm_smccc_smc_quirk(...) __arm_smccc_smc(__VA_ARGS__)

#endif /*__ARM_SMCCC_H*/