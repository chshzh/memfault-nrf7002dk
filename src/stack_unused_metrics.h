/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef STACK_UNUSED_METRICS_H_
#define STACK_UNUSED_METRICS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize stack metrics monitoring for all system threads
 *
 * Registers various system threads for stack usage monitoring with Memfault.
 * Only available when CONFIG_MEMFAULT_NCS_STACK_METRICS is enabled.
 */
void stack_unused_metrics_init(void);

#ifdef __cplusplus
}
#endif

#endif /* STACK_UNUSED_METRICS_H_ */
