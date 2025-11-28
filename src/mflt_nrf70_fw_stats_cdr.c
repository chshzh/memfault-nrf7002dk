/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * nRF70 Firmware Statistics CDR (Custom Data Recording) for Memfault
 *
 * This module collects nRF70 WiFi firmware statistics (PHY, LMAC, UMAC)
 * as a binary blob and uploads them to Memfault using the CDR feature.
 *
 * IMPLEMENTATION: Uses direct FMAC API (nrf_wifi_sys_fmac_stats_get) like
 * wifi_util.c does. This provides ON-DEMAND stats collection without
 * per-packet polling overhead.
 *
 * The blob can be parsed using the nrf70_fw_stats_parser.py script
 * located at: modules/lib/nrf_wifi/scripts/nrf70_fw_stats_parser.py
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

/* Direct FMAC API access - same includes as wifi_util.c */
#include "host_rpu_umac_if.h"
#include "system/fmac_api.h"
#include "fmac_main.h"

/* Stats structure definitions */
#include "rpu_lmac_phy_stats.h"
#include "rpu_umac_stats.h"

#include "memfault/components.h"
#include "memfault/core/data_packetizer.h"

/* External reference to the global nRF70 driver context (same as wifi_util.c) */
extern struct nrf_wifi_drv_priv_zep rpu_drv_priv_zep;

LOG_MODULE_REGISTER(mflt_nrf70_fw_stats_cdr, CONFIG_MEMFAULT_SAMPLE_LOG_LEVEL);

/* Maximum expected size of nRF70 FW stats blob (161 uint32_t values = 644 bytes) */
#define NRF70_FW_STATS_BLOB_MAX_SIZE 1024

/* Forward declarations for CDR callbacks */
static bool has_cdr_cb(sMemfaultCdrMetadata *metadata);
static bool read_data_cb(uint32_t offset, void *buf, size_t buf_len);
static void mark_cdr_read_cb(void);

/* MIME types for the CDR payload */
static const char *const mimetypes[] = {MEMFAULT_CDR_BINARY};

/* Buffer to hold the collected nRF70 FW stats blob */
static uint8_t s_nrf70_fw_stats_blob[NRF70_FW_STATS_BLOB_MAX_SIZE];
static size_t s_nrf70_fw_stats_blob_size = 0;
static bool s_cdr_data_ready = false;
static size_t s_read_offset = 0;

/* CDR metadata */
static sMemfaultCdrMetadata s_nrf70_fw_stats_metadata = {
	.start_time.type = kMemfaultCurrentTimeType_Unknown,
	.mimetypes = (const char **)mimetypes,
	.num_mimetypes = 1,
	.data_size_bytes = 0,
	.duration_ms = 0,
	.collection_reason = "nrf70_fw_stats",
};

/* CDR source implementation */
static const sMemfaultCdrSourceImpl s_nrf70_fw_stats_cdr_source = {
	.has_cdr_cb = has_cdr_cb,
	.read_data_cb = read_data_cb,
	.mark_cdr_read_cb = mark_cdr_read_cb,
};

/**
 * @brief Check if CDR data is available
 */
static bool has_cdr_cb(sMemfaultCdrMetadata *metadata)
{
	if (!s_cdr_data_ready || s_nrf70_fw_stats_blob_size == 0) {
		return false;
	}

	s_nrf70_fw_stats_metadata.data_size_bytes = s_nrf70_fw_stats_blob_size;
	*metadata = s_nrf70_fw_stats_metadata;

	LOG_DBG("CDR data available: %zu bytes", s_nrf70_fw_stats_blob_size);
	return true;
}

/**
 * @brief Read CDR data at specified offset
 */
static bool read_data_cb(uint32_t offset, void *buf, size_t buf_len)
{
	if (offset != s_read_offset) {
		LOG_WRN("Unexpected read offset: %u vs %zu", offset, s_read_offset);
		/* Reset and try to continue */
		s_read_offset = offset;
	}

	if (offset >= s_nrf70_fw_stats_blob_size) {
		LOG_DBG("Read complete");
		return false;
	}

	size_t remaining = s_nrf70_fw_stats_blob_size - offset;
	size_t copy_len = (buf_len < remaining) ? buf_len : remaining;

	memcpy(buf, &s_nrf70_fw_stats_blob[offset], copy_len);
	s_read_offset += copy_len;

	LOG_DBG("Read %zu bytes at offset %u", copy_len, offset);
	return true;
}

/**
 * @brief Called when CDR data has been fully read/uploaded
 */
static void mark_cdr_read_cb(void)
{
	LOG_INF("nRF70 FW stats CDR data uploaded successfully");

	/* Reset state for next collection */
	s_cdr_data_ready = false;
	s_nrf70_fw_stats_blob_size = 0;
	s_read_offset = 0;
	s_nrf70_fw_stats_metadata.data_size_bytes = 0;
}

/**
 * @brief Collect nRF70 firmware statistics into the blob buffer
 *
 * Uses direct FMAC API (nrf_wifi_sys_fmac_stats_get) like wifi_util.c does.
 * This provides ON-DEMAND stats collection without per-packet polling.
 *
 * @return 0 on success, negative error code on failure
 */
static int collect_nrf70_fw_stats(void)
{
	struct nrf_wifi_ctx_zep *ctx = &rpu_drv_priv_zep.rpu_ctx_zep;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct rpu_sys_op_stats stats;
	enum nrf_wifi_status status;
	int ret = 0;

	/* Lock the RPU context (same pattern as wifi_util.c) */
	k_mutex_lock(&ctx->rpu_lock, K_FOREVER);

	if (!ctx->rpu_ctx) {
		LOG_ERR("RPU context not initialized - WiFi not started?");
		ret = -ENODEV;
		goto unlock;
	}

	fmac_dev_ctx = ctx->rpu_ctx;

	LOG_DBG("Collecting nRF70 firmware statistics (direct FMAC API)...");

	/* Query RPU stats directly - same call as wifi_util.c:nrf_wifi_util_dump_rpu_stats() */
	memset(&stats, 0, sizeof(struct rpu_sys_op_stats));
	status = nrf_wifi_sys_fmac_stats_get(fmac_dev_ctx, 0, &stats);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("Failed to get RPU stats: %d", status);
		ret = -EIO;
		goto unlock;
	}

	/* Copy the firmware stats structure directly as the blob */
	size_t fw_stats_size = sizeof(stats.fw);
	if (fw_stats_size > NRF70_FW_STATS_BLOB_MAX_SIZE) {
		LOG_WRN("FW stats truncated: %zu > %d bytes", fw_stats_size, NRF70_FW_STATS_BLOB_MAX_SIZE);
		fw_stats_size = NRF70_FW_STATS_BLOB_MAX_SIZE;
	}

	memcpy(s_nrf70_fw_stats_blob, &stats.fw, fw_stats_size);
	s_nrf70_fw_stats_blob_size = fw_stats_size;

	LOG_DBG("Collected %zu bytes of nRF70 FW stats (UMAC+LMAC+PHY)", s_nrf70_fw_stats_blob_size);

unlock:
	k_mutex_unlock(&ctx->rpu_lock);
	return ret;
}

/**
 * @brief Initialize the nRF70 FW stats CDR module
 *
 * @return 0 on success, negative error code on failure
 */
int mflt_nrf70_fw_stats_cdr_init(void)
{
	static bool initialized = false;

	if (initialized) {
		LOG_WRN("nRF70 FW stats CDR already initialized");
		return -EALREADY;
	}

	/* Register CDR source with Memfault */
	if (!memfault_cdr_register_source(&s_nrf70_fw_stats_cdr_source)) {
		LOG_ERR("Failed to register nRF70 FW stats CDR source");
		return -EIO;
	}

	initialized = true;
	LOG_INF("nRF70 FW stats CDR module initialized");

	return 0;
}



/**
 * @brief Trigger collection of nRF70 FW stats for CDR upload
 *
 * Call this function to collect current nRF70 firmware statistics
 * and prepare them for upload to Memfault. The data will be uploaded
 * during the next Memfault data post operation.
 *
 * @return 0 on success, negative error code on failure
 */
int mflt_nrf70_fw_stats_cdr_collect(void)
{
	int err;

	if (s_cdr_data_ready) {
		LOG_WRN("Previous CDR data not yet uploaded, overwriting");
	}

	/* Reset state */
	s_cdr_data_ready = false;
	s_nrf70_fw_stats_blob_size = 0;
	s_read_offset = 0;

	/* Collect the stats */
	err = collect_nrf70_fw_stats();
	if (err) {
		LOG_ERR("Failed to collect nRF70 FW stats: %d", err);
		return err;
	}

	if (s_nrf70_fw_stats_blob_size == 0) {
		LOG_WRN("No nRF70 FW stats collected");
		return -ENODATA;
	}


	/* Mark data as ready for upload */
	s_cdr_data_ready = true;

	LOG_INF("nRF70 FW stats CDR ready for upload (%zu bytes)", s_nrf70_fw_stats_blob_size);

	return 0;
}

/**
 * @brief Check if nRF70 FW stats CDR data is pending upload
 *
 * @return true if data is waiting to be uploaded
 */
bool mflt_nrf70_fw_stats_cdr_is_pending(void)
{
	return s_cdr_data_ready;
}

/**
 * @brief Get the size of collected nRF70 FW stats
 *
 * @return Size in bytes, or 0 if no data collected
 */
size_t mflt_nrf70_fw_stats_cdr_get_size(void)
{
	return s_nrf70_fw_stats_blob_size;
}
