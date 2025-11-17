#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(memfault-nrf7002dk)

# NORDIC SDK APP START
add_subdirectory(src)
# NORDIC SDK APP END

zephyr_include_directories(src)
zephyr_include_directories(config)

# Add BLE provisioning library when enabled
if(CONFIG_BLE_PROV_ENABLED)
    zephyr_library_include_directories($<TARGET_PROPERTY:wifi_prov_core,BINARY_DIR>)
    target_link_libraries(app PRIVATE wifi_prov_ble)
endif()
