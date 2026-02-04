/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "wifi.h"
#include "../messages.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wifi_module, CONFIG_WIFI_MODULE_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_credentials.h>
#include <zephyr/zbus/zbus.h>

#ifdef CONFIG_BLE_PROV_ENABLED
#include "../ble_prov/ble_prov.h"
#endif

#define WIFI_INIT_PRIORITY 1

#define L4_EVENT_MASK         (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR | NET_EVENT_CONN_IF_TIMEOUT)

/* ============================================================================
 * ZBUS CHANNEL DEFINITION
 * ============================================================================
 */

ZBUS_CHAN_DEFINE(WIFI_CHAN, struct wifi_msg, NULL, NULL,
		 ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));

/* ============================================================================
 * RECONNECT WORK
 * ============================================================================
 */

static void reconnect_work_handler(struct k_work *work)
{
	int err;

	LOG_INF("Retrying network bring-up after connectivity fault");
	err = conn_mgr_all_if_up(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_up during retry failed: %d", err);
		return;
	}
	err = conn_mgr_all_if_connect(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_connect during retry failed: %d", err);
	}
}

static K_WORK_DELAYABLE_DEFINE(reconnect_work, reconnect_work_handler);

/* ============================================================================
 * NET MANAGEMENT CALLBACKS
 * ============================================================================
 */

static struct net_mgmt_event_callback l4_cb;
static struct net_mgmt_event_callback conn_cb;

static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint64_t mgmt_event, struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	struct wifi_msg msg = {
		.rssi = 0,
		.error_code = 0,
	};

	switch (mgmt_event) {
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("Network connectivity established");
		msg.type = WIFI_STA_CONNECTED;
		zbus_chan_pub(&WIFI_CHAN, &msg, K_MSEC(100));
		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_INF("Network connectivity lost");
		msg.type = WIFI_STA_DISCONNECTED;
		zbus_chan_pub(&WIFI_CHAN, &msg, K_MSEC(100));
		break;
	default:
		LOG_DBG("Unknown L4 event: 0x%016llX", mgmt_event);
		return;
	}
}

static void connectivity_event_handler(struct net_mgmt_event_callback *cb,
				       uint64_t mgmt_event,
				       struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	struct wifi_msg msg = {
		.type = WIFI_ERROR,
		.rssi = 0,
		.error_code = 0,
	};

	switch (mgmt_event) {
	case NET_EVENT_CONN_IF_FATAL_ERROR:
		LOG_ERR("Connectivity fatal error, scheduling reconnect");
		zbus_chan_pub(&WIFI_CHAN, &msg, K_MSEC(100));
		k_work_reschedule(&reconnect_work, K_SECONDS(2));
		break;
	case NET_EVENT_CONN_IF_TIMEOUT:
		LOG_WRN("Connectivity timeout, scheduling reconnect");
		zbus_chan_pub(&WIFI_CHAN, &msg, K_MSEC(100));
		k_work_reschedule(&reconnect_work, K_SECONDS(1));
		break;
	default:
		break;
	}
}

/* ============================================================================
 * MODULE INITIALIZATION
 * ============================================================================
 */

static int wifi_module_init(void)
{
	int err;

	LOG_INF("Initializing WiFi STA module");

	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);

	net_mgmt_init_event_callback(&conn_cb, connectivity_event_handler,
				     CONN_LAYER_EVENT_MASK);
	net_mgmt_add_event_callback(&conn_cb);

#ifdef CONFIG_BLE_PROV_ENABLED
	k_sleep(K_SECONDS(1));
	err = conn_mgr_all_if_up(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_up: %d", err);
	}
	/* BLE prov already initialized by ble_prov module (init priority 0) */
	if (wifi_credentials_is_empty()) {
		LOG_INF("No stored WiFi credentials; provision via BLE");
	} else {
		LOG_INF("Connecting using stored WiFi credentials");
		err = conn_mgr_all_if_connect(true);
		if (err) {
			LOG_ERR("WiFi connection request failed: %d", err);
		}
	}
#else
	LOG_INF("Bringing network up and connecting");
	err = conn_mgr_all_if_connect(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_connect: %d", err);
	}
#endif

	LOG_INF("WiFi module initialized");
	return 0;
}

SYS_INIT(wifi_module_init, APPLICATION, WIFI_INIT_PRIORITY);
