/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * nRF70 Firmware Statistics CDR (Custom Data Recording) for Memfault
 * 
 * WARNING: Memfault CDR is limited to 1 upload per device per 24 hours!
 *          Enable Developer Mode in Memfault dashboard for higher limits.
 * 
 * TRIGGER: Button 1 short press ONLY - no automatic/periodic collection.
 */

#ifndef MFLT_NRF70_FW_STATS_CDR_H__
#define MFLT_NRF70_FW_STATS_CDR_H__

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Initialize the nRF70 FW stats CDR module
 * 
 * Registers the CDR source with Memfault. Should be called once
 * during application startup, after Memfault is initialized.
 * 
 * @return 0 on success
 * @return -EALREADY if already initialized
 * @return -EIO if registration failed
 */
int mflt_nrf70_fw_stats_cdr_init(void);

/**
 * @brief Trigger collection of nRF70 firmware stats for CDR upload
 * 
 * Collects current nRF70 WiFi firmware statistics (PHY, LMAC, UMAC)
 * as a binary blob. The data will be uploaded to Memfault during
 * the next data post operation (memfault_zephyr_port_post_data()).
 * 
 * The blob format matches the output of "net stats all hex-blob" and
 * can be parsed using: modules/lib/nrf_wifi/scripts/nrf70_fw_stats_parser.py
 * 
 * Note: Memfault limits CDR uploads to 1 per device per 24 hours.
 * Enable Developer Mode in Memfault dashboard for higher limits
 * during development.
 * 
 * @return 0 on success
 * @return -ENODEV if no network interface found
 * @return -ENOTSUP if vendor stats not supported
 * @return -ENODATA if no stats available
 * @return -EIO on other errors
 */
int mflt_nrf70_fw_stats_cdr_collect(void);

/**
 * @brief Check if nRF70 FW stats CDR data is pending upload
 * 
 * @return true if data is waiting to be uploaded
 * @return false if no pending data
 */
bool mflt_nrf70_fw_stats_cdr_is_pending(void);

/**
 * @brief Get the size of collected nRF70 FW stats
 * 
 * @return Size in bytes of the collected stats blob
 * @return 0 if no data collected
 */
size_t mflt_nrf70_fw_stats_cdr_get_size(void);

#endif /* MFLT_NRF70_FW_STATS_CDR_H__ */

