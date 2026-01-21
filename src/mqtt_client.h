/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MQTT_CLIENT_H_
#define MQTT_CLIENT_H_

/**
 * @brief Initialize the MQTT client
 *
 * Initializes the MQTT helper library and sets up callbacks.
 *
 * @return 0 on success, negative error code on failure
 */
int app_mqtt_client_init(void);

/**
 * @brief Notify MQTT client that network connection is established
 *
 * This function should be called when the device successfully connects
 * to the network. It will trigger the MQTT client to connect to the broker.
 */
void app_mqtt_client_notify_connected(void);

/**
 * @brief Notify MQTT client that network connection is lost
 *
 * This function should be called when the device loses network connectivity.
 * It will disconnect the MQTT client from the broker.
 */
void app_mqtt_client_notify_disconnected(void);

/**
 * @brief Publish a message to the configured topic
 *
 * @param payload Null-terminated string to publish
 * @return 0 on success, negative error code on failure
 */
int app_mqtt_client_publish(const char *payload);

#endif /* MQTT_CLIENT_H_ */
