/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MFLT_STACK_METRICS_H_
#define MFLT_STACK_METRICS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize stack metrics monitoring for all system threads
 *
 * Registers various system threads for stack usage monitoring with Memfault.
 * Only available when CONFIG_MEMFAULT_NCS_STACK_METRICS is enabled.
 */
void mflt_stack_metrics_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MFLT_STACK_METRICS_H_ */
