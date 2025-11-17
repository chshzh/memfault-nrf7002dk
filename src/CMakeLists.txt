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

target_include_directories(app PRIVATE .)
