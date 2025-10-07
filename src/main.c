/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <memfault/metrics/metrics.h>
#include <memfault/ports/zephyr/http.h>
#include <memfault/core/data_packetizer.h>
#include <memfault/core/trace_event.h>
#include <memfault/panics/coredump.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/sys/util.h>

#include "ota_trigger.h"

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

#if CONFIG_MEMFAULT_NCS_STACK_METRICS
#include "stack_unused_metrics.h"
#endif

LOG_MODULE_REGISTER(memfault_sample, CONFIG_MEMFAULT_SAMPLE_LOG_LEVEL);

/* Macros used to subscribe to specific Zephyr NET management events. */
#define L4_EVENT_MASK         (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

static K_SEM_DEFINE(nw_connected_sem, 0, 1);
static bool wifi_connected = false;

/* Zephyr NET management event callback structures. */
static struct net_mgmt_event_callback l4_cb;
static struct net_mgmt_event_callback conn_cb;

/* Recursive Fibonacci calculation used to trigger stack overflow. */
// static int fib(int n)
// {
// 	if (n <= 1) {
// 		return n;
// 	}

// 	return fib(n - 1) + fib(n - 2);
// }

/* WiFi metrics collection timer */
static void wifi_metrics_timer_handler(struct k_timer *timer);
static void wifi_metrics_work_handler(struct k_work *work);

K_TIMER_DEFINE(wifi_metrics_timer, wifi_metrics_timer_handler, NULL);
K_WORK_DEFINE(wifi_metrics_work, wifi_metrics_work_handler);

/* WiFi metrics collection function */
static void collect_post_wifi_connection_metrics(void)
{
	struct net_if *iface = net_if_get_default();
	struct wifi_iface_status status = {0};

	if (!iface) {
		LOG_WRN("No network interface found");
		return;
	}

	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
		     sizeof(struct wifi_iface_status))) {
		LOG_WRN("Failed to get WiFi interface status");
		return;
	}

	/* Only collect metrics if we're connected in station mode */
	if (status.state != WIFI_STATE_COMPLETED || status.iface_mode != WIFI_MODE_INFRA) {
		LOG_DBG("WiFi not connected in station mode, skipping metrics");
		return;
	}

	/* Set custom WiFi metrics */
	MEMFAULT_METRIC_SET_SIGNED(my_wifi_rssi, status.rssi);
	MEMFAULT_METRIC_SET_UNSIGNED(my_wifi_channel, status.channel);
	MEMFAULT_METRIC_SET_UNSIGNED(my_wifi_link_mode,
				     status.link_mode); /*show which wifi is used, ie. wifi5 or 6*/

	/* Set TX rate if available (some devices may not have this value set) */
	if (status.current_phy_tx_rate > 0.0f) {
		MEMFAULT_METRIC_SET_UNSIGNED(my_wifi_tx_rate_mbps,
					     (uint32_t)status.current_phy_tx_rate);
		LOG_INF("TX Rate: %.1f Mbps", (double)status.current_phy_tx_rate);
	} else {
		LOG_INF("TX Rate not available (driver may not support or no data transmitted "
			"yet)");
	}

	/* Also trigger a heartbeat to capture the current metrics */
	memfault_metrics_heartbeat_debug_trigger();
}

/* Timer handler runs in ISR context, so dispatch to work queue */
static void wifi_metrics_timer_handler(struct k_timer *timer)
{
	if (wifi_connected) {
		k_work_submit(&wifi_metrics_work);
	} else {
		LOG_DBG("WiFi not connected, skipping metrics collection");
	}
}

/* Work handler runs in thread context */
static void wifi_metrics_work_handler(struct k_work *work)
{
	collect_post_wifi_connection_metrics();
}

/* Handle button presses and trigger faults that can be captured and sent to
 * the Memfault cloud for inspection after rebooting:
 * Only button 1 is available on Thingy:91, the rest are available on nRF9160 DK.
 *	Button 1: Trigger stack overflow.
 *	Button 2: Trigger NULL-pointer dereference.
 *	Switch 1: Increment switch_1_toggle_count metric by one.
 *	Switch 2: Trace switch_2_toggled event, along with switch state.
 */
static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	uint32_t buttons_pressed = has_changed & button_states;

	if (buttons_pressed & DK_BTN1_MSK) {
		// LOG_WRN("Stack overflow will now be triggered");
		// fib(10000);
		/* Manually trigger WiFi metrics collection */
		LOG_INF("Manually triggering WiFi metrics collection");
		if (wifi_connected) {
			k_work_submit(&wifi_metrics_work);
		} else {
			LOG_WRN("WiFi not connected, cannot collect metrics");
		}
	} else if (buttons_pressed & DK_BTN2_MSK) {
		LOG_INF("Button 2 pressed, scheduling Memfault OTA check");
		ota_trigger_notify_button();
	} else if (has_changed & DK_BTN3_MSK) {
		/* DK_BTN3_MSK is Switch 1 on nRF9160 DK. */
		int err = MEMFAULT_METRIC_ADD(switch_1_toggle_count, 1);
		if (err) {
			LOG_ERR("Failed to increment switch_1_toggle_count");
		} else {
			LOG_INF("switch_1_toggle_count incremented");
		}
	} else if (has_changed & DK_BTN4_MSK) {
		/* DK_BTN4_MSK is Switch 2 on nRF9160 DK. */
		MEMFAULT_TRACE_EVENT_WITH_LOG(switch_2_toggled, "Switch state: %d",
					      buttons_pressed & DK_BTN4_MSK ? 1 : 0);
		LOG_INF("switch_2_toggled event has been traced, button state: %d",
			buttons_pressed & DK_BTN4_MSK ? 1 : 0);
	}
}

static void on_connect(void)
{
#if IS_ENABLED(MEMFAULT_NCS_LTE_METRICS)
	uint32_t time_to_lte_connection;

	/* Retrieve the LTE time to connect metric. */
	memfault_metrics_heartbeat_timer_read(MEMFAULT_METRICS_KEY(ncs_lte_time_to_connect_ms),
					      &time_to_lte_connection);

	LOG_INF("Time to connect: %d ms", time_to_lte_connection);
#endif /* IS_ENABLED(MEMFAULT_NCS_LTE_METRICS) */

	if (IS_ENABLED(CONFIG_MEMFAULT_NCS_POST_COREDUMP_ON_NETWORK_CONNECTED) &&
	    memfault_coredump_has_valid_coredump(NULL)) {
		/* Coredump sending handled internally */
		return;
	}

	LOG_INF("Sending already captured data to Memfault");

	/* Trigger collection of heartbeat data. */
	memfault_metrics_heartbeat_debug_trigger();

	/* Check if there is any data available to be sent. */
	if (!memfault_packetizer_data_available()) {
		LOG_DBG("There was no data to be sent");
		return;
	}

	LOG_DBG("Sending stored data...");

	/* Send the data that has been captured to the memfault cloud.
	 * This will also happen periodically, with an interval that can be configured using
	 * CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_INTERVAL_SECS.
	 */
	memfault_zephyr_port_post_data();
}

static void l4_event_handler(struct net_mgmt_event_callback *cb, uint32_t event,
			     struct net_if *iface)
{
	switch (event) {
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("Network connectivity established");
		wifi_connected = true;

		/* Initialize stack metrics monitoring */
#if CONFIG_MEMFAULT_NCS_STACK_METRICS
		stack_unused_metrics_init();
		LOG_INF("Stack metrics monitoring initialized");
#endif

		/* Start WiFi metrics timer - collect metrics every 60 seconds */
		k_timer_start(&wifi_metrics_timer, K_SECONDS(60), K_SECONDS(60));
		LOG_INF("WiFi metrics timer started (60 second interval)");
		k_sem_give(&nw_connected_sem);
		ota_trigger_notify_connected();
		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_INF("Network connectivity lost");
		wifi_connected = false;
		/* Stop WiFi metrics timer */
		k_timer_stop(&wifi_metrics_timer);
		LOG_INF("WiFi metrics timer stopped");
		break;
	default:
		LOG_DBG("Unknown event: 0x%08X", event);
		return;
	}
}

static void connectivity_event_handler(struct net_mgmt_event_callback *cb, uint32_t event,
				       struct net_if *iface)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		__ASSERT(false, "Failed to connect to a network");
		return;
	}
}

int main(void)
{
	int err;

	LOG_INF("Memfault sample has started! Version: %s", CONFIG_MEMFAULT_NCS_FW_VERSION);

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("dk_buttons_init, error: %d", err);
	}
	/* Setup handler for Zephyr NET Connection Manager events. */
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);

	/* Setup handler for Zephyr NET Connection Manager Connectivity layer. */
	net_mgmt_init_event_callback(&conn_cb, connectivity_event_handler, CONN_LAYER_EVENT_MASK);
	net_mgmt_add_event_callback(&conn_cb);

	/* Connecting to the configured connectivity layer.
	 * Wi-Fi or LTE depending on the board that the sample was built for.
	 */
	LOG_INF("Bringing network interface up and connecting to the network");

	err = conn_mgr_all_if_up(true);
	if (err) {
		__ASSERT(false, "conn_mgr_all_if_up, error: %d", err);
		return err;
	}

	err = conn_mgr_all_if_connect(true);
	if (err) {
		__ASSERT(false, "conn_mgr_all_if_connect, error: %d", err);
		return err;
	}

	/* Performing in an infinite loop to be resilient against
	 * re-connect bursts directly after boot, e.g. when connected
	 * to a roaming network or via weak signal. Note that
	 * Memfault data will be uploaded periodically every
	 * CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_INTERVAL_SECS.
	 * We post data here so as soon as a connection is available
	 * the latest data will be pushed to Memfault.
	 */

	while (1) {
		k_sem_take(&nw_connected_sem, K_FOREVER);
		LOG_INF("Connected to network");
		on_connect();
	}
}
