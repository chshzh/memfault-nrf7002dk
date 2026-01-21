/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "mqtt_client.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <net/mqtt_helper.h>
#include <hw_id.h>
#include <memfault/metrics/metrics.h>

LOG_MODULE_REGISTER(mqtt_client, CONFIG_MQTT_CLIENT_LOG_LEVEL);

/* MQTT client states - prefixed to avoid conflict with mqtt_helper.h */
enum app_mqtt_client_state {
	APP_MQTT_STATE_DISCONNECTED,
	APP_MQTT_STATE_CONNECTING,
	APP_MQTT_STATE_CONNECTED,
};

/* Forward declarations */
static void connect_work_fn(struct k_work *work);
static void publish_work_fn(struct k_work *work);

/* Define delayed work items */
static K_WORK_DELAYABLE_DEFINE(connect_work, connect_work_fn);
static K_WORK_DELAYABLE_DEFINE(publish_work, publish_work_fn);

/* Define work queue stack */
K_THREAD_STACK_DEFINE(mqtt_workq_stack, CONFIG_MQTT_CLIENT_WORKQUEUE_STACK_SIZE);
static struct k_work_q mqtt_workq;

/* State variables */
static enum app_mqtt_client_state current_state = APP_MQTT_STATE_DISCONNECTED;
static bool network_ready;
static uint32_t message_count;
static char last_published_msg[32]; /* Track last published message for validation */
static uint32_t mqtt_loop_total;    /* Local counter for total loopback tests */
static uint32_t mqtt_loop_failures; /* Local counter for failed loopback tests */

/* Client ID and topic buffers */
static char client_id[CONFIG_MQTT_CLIENT_ID_BUFFER_SIZE];
static char pub_topic[CONFIG_MQTT_CLIENT_ID_BUFFER_SIZE + sizeof(CONFIG_MQTT_CLIENT_PUBLISH_TOPIC) +
		      16];
static char sub_topic[CONFIG_MQTT_CLIENT_ID_BUFFER_SIZE + sizeof(CONFIG_MQTT_CLIENT_PUBLISH_TOPIC) +
		      16];

/* Forward declaration for subscribe */
static int subscribe_to_topic(void);

/* MQTT helper callbacks */
static void on_mqtt_connack(enum mqtt_conn_return_code return_code, bool session_present)
{
	ARG_UNUSED(session_present);

	if (return_code != MQTT_CONNECTION_ACCEPTED) {
		LOG_ERR("MQTT broker rejected connection, return code: %d", return_code);
		current_state = APP_MQTT_STATE_DISCONNECTED;
		return;
	}

	LOG_INF("Connected to MQTT broker");
	LOG_INF("Hostname: %s", CONFIG_MQTT_CLIENT_BROKER_HOSTNAME);
	LOG_INF("Client ID: %s", client_id);
	LOG_INF("Port: %d", CONFIG_MQTT_HELPER_PORT);
	LOG_INF("TLS: Yes");

	current_state = APP_MQTT_STATE_CONNECTED;

	/* Cancel reconnection attempts */
	k_work_cancel_delayable(&connect_work);

	/* Subscribe to loopback topic */
	int err = subscribe_to_topic();
	if (err) {
		LOG_WRN("Failed to subscribe: %d", err);
	}

	/* Start periodic publishing */
	k_work_reschedule_for_queue(&mqtt_workq, &publish_work,
				    K_SECONDS(CONFIG_MQTT_CLIENT_PUBLISH_INTERVAL_SEC));
}

static void on_mqtt_disconnect(int result)
{
	LOG_INF("Disconnected from MQTT broker, result: %d", result);
	current_state = APP_MQTT_STATE_DISCONNECTED;

	/* Stop publishing */
	k_work_cancel_delayable(&publish_work);

	/* Schedule reconnection if network is still available */
	if (network_ready) {
		LOG_INF("Scheduling reconnection in %d seconds",
			CONFIG_MQTT_CLIENT_RECONNECT_TIMEOUT_SEC);
		k_work_reschedule_for_queue(&mqtt_workq, &connect_work,
					    K_SECONDS(CONFIG_MQTT_CLIENT_RECONNECT_TIMEOUT_SEC));
	}
}

static void on_mqtt_publish(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload)
{
	char received_msg[32];
	size_t copy_len = MIN(payload.size, sizeof(received_msg) - 1);
	memcpy(received_msg, payload.ptr, copy_len);
	received_msg[copy_len] = '\0';

	LOG_INF("Received payload: %.*s on topic: %.*s", payload.size, payload.ptr, topic.size,
		topic.ptr);

	/* Validate loopback: check if received matches published */
	if (last_published_msg[0] != '\0') {
		mqtt_loop_total++;
		if (strcmp(received_msg, last_published_msg) == 0) {
			/* Successful loopback */
			MEMFAULT_METRIC_ADD(mqtt_loop_total_count, 1);
			LOG_INF("MQTT loopback success: '%s' matched", received_msg);
		} else {
			/* Loopback validation failed - mismatch */
			mqtt_loop_failures++;
			MEMFAULT_METRIC_ADD(mqtt_loop_fail_count, 1);
			LOG_ERR("MQTT loopback FAILED: expected '%s', got '%s'", last_published_msg,
				received_msg);
		}
	}

	/* Log local metrics (these persist across heartbeat intervals) */
	LOG_INF("MQTT Loop Test Metrics - Total: %u, Failures: %u", mqtt_loop_total,
		mqtt_loop_failures);
}

static void on_mqtt_suback(uint16_t message_id, int result)
{
	if (result == 0) {
		LOG_INF("Subscription successful, message_id: %d", message_id);
	} else {
		LOG_ERR("Subscription failed, error: %d", result);
	}
}

static int setup_topics(void)
{
	int len;

	len = snprintk(pub_topic, sizeof(pub_topic), "Memfault/%s/%s", client_id,
		       CONFIG_MQTT_CLIENT_PUBLISH_TOPIC);
	if ((len < 0) || (len >= sizeof(pub_topic))) {
		LOG_ERR("Publish topic buffer too small");
		return -EMSGSIZE;
	}

	/* Subscribe to the same topic for loopback */
	len = snprintk(sub_topic, sizeof(sub_topic), "Memfault/%s/%s", client_id,
		       CONFIG_MQTT_CLIENT_PUBLISH_TOPIC);
	if ((len < 0) || (len >= sizeof(sub_topic))) {
		LOG_ERR("Subscribe topic buffer too small");
		return -EMSGSIZE;
	}

	LOG_INF("Publish topic: %s", pub_topic);
	LOG_INF("Subscribe topic: %s", sub_topic);
	return 0;
}

static int subscribe_to_topic(void)
{
	int err;
	struct mqtt_topic topics[] = {
		{
			.topic.utf8 = sub_topic,
			.topic.size = strlen(sub_topic),
			.qos = MQTT_QOS_1_AT_LEAST_ONCE,
		},
	};
	struct mqtt_subscription_list sub_list = {
		.list = topics,
		.list_count = ARRAY_SIZE(topics),
		.message_id = mqtt_helper_msg_id_get(),
	};

	LOG_INF("Subscribing to topic: %s", sub_topic);

	err = mqtt_helper_subscribe(&sub_list);
	if (err) {
		LOG_ERR("Failed to subscribe: %d", err);
		return err;
	}

	return 0;
}

static void connect_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);
	int err;

	if (!network_ready) {
		LOG_WRN("Network not ready, skipping MQTT connection");
		return;
	}

	if (current_state == APP_MQTT_STATE_CONNECTED) {
		LOG_DBG("Already connected to MQTT broker");
		return;
	}

	current_state = APP_MQTT_STATE_CONNECTING;

	/* Get client ID based on hardware ID (MAC address) */
	err = hw_id_get(client_id, sizeof(client_id));
	if (err) {
		LOG_ERR("Failed to get hardware ID: %d", err);
		current_state = APP_MQTT_STATE_DISCONNECTED;
		return;
	}

	err = setup_topics();
	if (err) {
		LOG_ERR("Failed to setup topics: %d", err);
		current_state = APP_MQTT_STATE_DISCONNECTED;
		return;
	}

	struct mqtt_helper_conn_params conn_params = {
		.hostname.ptr = CONFIG_MQTT_CLIENT_BROKER_HOSTNAME,
		.hostname.size = strlen(CONFIG_MQTT_CLIENT_BROKER_HOSTNAME),
		.device_id.ptr = client_id,
		.device_id.size = strlen(client_id),
	};

	LOG_INF("Connecting to MQTT broker: %s", CONFIG_MQTT_CLIENT_BROKER_HOSTNAME);

	err = mqtt_helper_connect(&conn_params);
	if (err) {
		LOG_ERR("Failed to connect to MQTT broker: %d", err);
		current_state = APP_MQTT_STATE_DISCONNECTED;
		/* Schedule retry */
		k_work_reschedule_for_queue(&mqtt_workq, &connect_work,
					    K_SECONDS(CONFIG_MQTT_CLIENT_RECONNECT_TIMEOUT_SEC));
	}
}

static void publish_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);
	int err;
	char payload[128];

	if (current_state != APP_MQTT_STATE_CONNECTED) {
		LOG_WRN("Not connected to MQTT broker, skipping publish");
		return;
	}

	message_count++;
	snprintk(payload, sizeof(payload), "%u", message_count);

	/* Save for loopback validation */
	strncpy(last_published_msg, payload, sizeof(last_published_msg) - 1);
	last_published_msg[sizeof(last_published_msg) - 1] = '\0';

	struct mqtt_publish_param param = {
		.message.payload.data = payload,
		.message.payload.len = strlen(payload),
		.message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE,
		.message_id = mqtt_helper_msg_id_get(),
		.message.topic.topic.utf8 = pub_topic,
		.message.topic.topic.size = strlen(pub_topic),
	};

	err = mqtt_helper_publish(&param);
	if (err) {
		LOG_WRN("Failed to publish message: %d", err);
		/* Increment failure count if publish fails */
		MEMFAULT_METRIC_ADD(mqtt_loop_fail_count, 1);
	} else {
		LOG_INF("Published message: \"%s\" on topic: \"%s\"", payload, pub_topic);
	}

	/* Schedule next publish */
	k_work_reschedule_for_queue(&mqtt_workq, &publish_work,
				    K_SECONDS(CONFIG_MQTT_CLIENT_PUBLISH_INTERVAL_SEC));
}

int app_mqtt_client_init(void)
{
	int err;

	LOG_INF("Initializing MQTT client");

	/* Initialize work queue */
	k_work_queue_init(&mqtt_workq);
	k_work_queue_start(&mqtt_workq, mqtt_workq_stack, K_THREAD_STACK_SIZEOF(mqtt_workq_stack),
			   CONFIG_MQTT_CLIENT_THREAD_PRIORITY, NULL);

	/* Configure MQTT helper callbacks */
	struct mqtt_helper_cfg cfg = {
		.cb =
			{
				.on_connack = on_mqtt_connack,
				.on_disconnect = on_mqtt_disconnect,
				.on_publish = on_mqtt_publish,
				.on_suback = on_mqtt_suback,
			},
	};

	err = mqtt_helper_init(&cfg);
	if (err) {
		LOG_ERR("Failed to initialize MQTT helper: %d", err);
		return err;
	}

	LOG_INF("MQTT client initialized");
	return 0;
}

void app_mqtt_client_notify_connected(void)
{
	LOG_INF("Network connected, scheduling MQTT connection");
	network_ready = true;

	/* Wait a few seconds for network stack to stabilize */
	k_work_reschedule_for_queue(&mqtt_workq, &connect_work, K_SECONDS(5));
}

void app_mqtt_client_notify_disconnected(void)
{
	LOG_INF("Network disconnected, stopping MQTT client");
	network_ready = false;

	/* Cancel pending work */
	k_work_cancel_delayable(&connect_work);
	k_work_cancel_delayable(&publish_work);

	/* Disconnect from broker if connected */
	if (current_state == APP_MQTT_STATE_CONNECTED) {
		mqtt_helper_disconnect();
		current_state = APP_MQTT_STATE_DISCONNECTED;
	}
}

int app_mqtt_client_publish(const char *payload)
{
	int err;

	if (current_state != APP_MQTT_STATE_CONNECTED) {
		LOG_WRN("Not connected to MQTT broker");
		return -ENOTCONN;
	}

	if (!payload) {
		return -EINVAL;
	}

	struct mqtt_publish_param param = {
		.message.payload.data = (uint8_t *)payload,
		.message.payload.len = strlen(payload),
		.message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE,
		.message_id = mqtt_helper_msg_id_get(),
		.message.topic.topic.utf8 = pub_topic,
		.message.topic.topic.size = strlen(pub_topic),
	};

	err = mqtt_helper_publish(&param);
	if (err) {
		LOG_WRN("Failed to publish message: %d", err);
		return err;
	}

	LOG_INF("Published message: \"%s\" on topic: \"%s\"", payload, pub_topic);
	return 0;
}
