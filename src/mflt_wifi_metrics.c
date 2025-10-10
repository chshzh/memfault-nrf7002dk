/*
 * Wi-Fi metrics helpers for Memfault heartbeat collection.
 */

#include "mflt_wifi_metrics.h"

#include <memfault/metrics/metrics.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>

LOG_MODULE_DECLARE(memfault_sample, CONFIG_MEMFAULT_SAMPLE_LOG_LEVEL);

void mflt_wifi_metrics_collect(void)
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
	MEMFAULT_METRIC_SET_SIGNED(ncs_wifi_rssi, status.rssi);
	MEMFAULT_METRIC_SET_UNSIGNED(ncs_wifi_channel, status.channel);
	MEMFAULT_METRIC_SET_UNSIGNED(ncs_wifi_link_mode, status.link_mode);

	/* Set TX rate if available (some devices may not have this value set) */
	if (status.current_phy_tx_rate > 0.0f) {
		MEMFAULT_METRIC_SET_UNSIGNED(ncs_wifi_tx_rate_mbps,
					     (uint32_t)status.current_phy_tx_rate);
		LOG_INF("TX Rate: %.1f Mbps", (double)status.current_phy_tx_rate);
	} else {
		LOG_INF("TX Rate not available (driver may not support or no data transmitted "
			"yet)");
	}
}
