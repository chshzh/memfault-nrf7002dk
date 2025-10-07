/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Notify the OTA trigger thread that a manual check should be performed.
 *
 * Typically invoked from the button handler when the user presses button 2.
 */
void mflt_ota_triggers_notify_button(void);

/**
 * @brief Notify the OTA trigger thread that network connectivity is available.
 */
void mflt_ota_triggers_notify_connected(void);

#ifdef __cplusplus
}
#endif
