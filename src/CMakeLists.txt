#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE main.c mflt_ota_triggers.c mflt_wifi_metrics.c)

# Add stack metrics monitoring when Memfault stack metrics are enabled
if(CONFIG_MEMFAULT_NCS_STACK_METRICS)
    target_sources(app PRIVATE mflt_stack_metrics.c)
    # Add the NCS Memfault include directory
    target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/modules/memfault-firmware-sdk/include)
endif()

# Add BLE provisioning when enabled
if(CONFIG_BLE_PROV_ENABLED)
    target_sources(app PRIVATE ble_provisioning.c)
endif()

# Add HTTPS client when enabled
if(CONFIG_HTTPS_CLIENT_ENABLED)
    target_sources(app PRIVATE https_client.c)
endif()

# Add MQTT client when enabled
if(CONFIG_MQTT_CLIENT_ENABLED)
    target_sources(app PRIVATE mqtt_client.c)
endif()

# Add nRF70 FW stats CDR when enabled
if(CONFIG_NRF70_FW_STATS_CDR_ENABLED)
    target_sources(app PRIVATE mflt_nrf70_fw_stats_cdr.c)
    
    # Include internal nrf_wifi headers for direct FMAC API access
    # This bypasses the Ethernet API to avoid per-packet stats polling
    # Mirroring the includes used by wifi_util.c in the nRF70 driver
    set(NRF_WIFI_DIR ${ZEPHYR_BASE}/../modules/lib/nrf_wifi)
    target_include_directories(app PRIVATE
        # Zephyr nRF70 driver headers
        ${ZEPHYR_BASE}/drivers/wifi/nrf_wifi/inc
        ${ZEPHYR_BASE}/drivers/wifi/nrf_wifi/src
        # nrf_wifi FMAC layer headers
        ${NRF_WIFI_DIR}/fw_if/umac_if/inc
        ${NRF_WIFI_DIR}/fw_if/umac_if/inc/fw
        ${NRF_WIFI_DIR}/fw_if/umac_if/inc/fw/stats
        ${NRF_WIFI_DIR}/fw_if/umac_if/inc/fw/stats/system
        # nrf_wifi HAL layer headers (for rpu_if.h, hal_structs_common.h)
        ${NRF_WIFI_DIR}/hw_if/hal/inc
        ${NRF_WIFI_DIR}/hw_if/hal/inc/common
        # nrf_wifi OS abstraction headers (for osal_api.h)
        ${NRF_WIFI_DIR}/os_if/inc
        # nrf_wifi utility headers (for queue.h, list.h)
        ${NRF_WIFI_DIR}/utils/inc
        # nrf_wifi bus abstraction headers
        ${NRF_WIFI_DIR}/bus_if/bal/inc
    )
endif()

target_include_directories(app PRIVATE .)
