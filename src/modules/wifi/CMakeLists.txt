target_sources(app PRIVATE wifi.c)
target_include_directories(app PUBLIC ${ZEPHYR_BASE}/subsys/net/ip)
