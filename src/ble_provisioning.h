/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BLE_PROVISIONING_H_
#define BLE_PROVISIONING_H_

#include <zephyr/kernel.h>

/**
 * @brief Initialize BLE provisioning
 *
 * This function sets up Bluetooth LE and starts advertising for Wi-Fi provisioning.
 * It should be called early in the application lifecycle.
 *
 * @return 0 on success, negative error code on failure
 */
int ble_prov_init(void);

/**
 * @brief Update WiFi connection status in BLE advertisement
 *
 * This function should be called when WiFi connection status changes
 * to reflect the current state in BLE advertisements.
 *
 * @param connected true if WiFi is connected, false otherwise
 */
void ble_prov_update_wifi_status(bool connected);

#endif /* BLE_PROVISIONING_H_ */
