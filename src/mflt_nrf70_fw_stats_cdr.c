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
 * The blob can be parsed using the nrf70_fw_stats_parser.py script
 * located at: modules/lib/nrf_wifi/scripts/nrf70_fw_stats_parser.py
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_stats.h>
#include <string.h>

#include "memfault/components.h"
#include "memfault/core/data_packetizer.h"

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
 * @return 0 on success, negative error code on failure
 */
static int collect_nrf70_fw_stats(void)
{
	struct net_if *iface;
	const struct ethernet_api *eth_api;
	struct net_stats_eth *stats;
	size_t blob_offset = 0;

	/* Find the WiFi interface */
	iface = net_if_get_default();
	if (!iface) {
		LOG_ERR("No default network interface");
		return -ENODEV;
	}

	/* Verify it's an Ethernet/WiFi interface */
	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		LOG_ERR("Default interface is not Ethernet/WiFi");
		return -ENOTSUP;
	}

	/* Get the Ethernet API */
	eth_api = (const struct ethernet_api *)net_if_get_device(iface)->api;
	if (!eth_api || !eth_api->get_stats) {
		LOG_ERR("Ethernet API or get_stats not available");
		return -ENOTSUP;
	}

	/* Get the statistics */
	stats = eth_api->get_stats(net_if_get_device(iface));
	if (!stats) {
		LOG_ERR("Failed to get Ethernet stats");
		return -EIO;
	}

#if defined(CONFIG_NET_STATISTICS_ETHERNET_VENDOR)
	/* Check if vendor stats are available */
	if (!stats->vendor) {
		LOG_WRN("No vendor statistics available");
		return -ENODATA;
	}

	/* Collect vendor stats as binary blob */
	LOG_INF("Collecting nRF70 firmware statistics...");

	size_t i = 0;
	while (stats->vendor[i].key != NULL && blob_offset < NRF70_FW_STATS_BLOB_MAX_SIZE - 4) {
		uint32_t value = stats->vendor[i].value;

		/* Store as little-endian (matching shell output format) */
		s_nrf70_fw_stats_blob[blob_offset++] = (uint8_t)(value & 0xFF);
		s_nrf70_fw_stats_blob[blob_offset++] = (uint8_t)((value >> 8) & 0xFF);
		s_nrf70_fw_stats_blob[blob_offset++] = (uint8_t)((value >> 16) & 0xFF);
		s_nrf70_fw_stats_blob[blob_offset++] = (uint8_t)((value >> 24) & 0xFF);

		i++;
	}

	s_nrf70_fw_stats_blob_size = blob_offset;
	LOG_INF("Collected %zu bytes of nRF70 FW stats (%zu fields)", s_nrf70_fw_stats_blob_size,
		i);

	return 0;
#else
	LOG_ERR("CONFIG_NET_STATISTICS_ETHERNET_VENDOR not enabled");
	return -ENOTSUP;
#endif
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
 * @brief Print the collected blob as hex string (for debugging/verification)
 */
static void print_blob_hex(void)
{
	if (s_nrf70_fw_stats_blob_size == 0) {
		return;
	}

	/* Print in chunks to avoid log buffer overflow */
	printk("nRF70 FW stats hex blob: ");
	for (size_t i = 0; i < s_nrf70_fw_stats_blob_size; i++) {
		printk("%02x", s_nrf70_fw_stats_blob[i]);
	}
	printk("\n");
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

	/* Print blob hex for debugging/verification */
	// print_blob_hex();

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
