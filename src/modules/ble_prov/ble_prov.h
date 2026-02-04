/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BLE_PROV_H
#define BLE_PROV_H

#include <stdbool.h>

int ble_prov_init(void);
void ble_prov_update_wifi_status(bool connected);

#endif /* BLE_PROV_H */
