/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HTTPS_CLIENT_H_
#define HTTPS_CLIENT_H_

/**
 * @brief Initialize the HTTPS client
 *
 * @return 0 on success, negative error code on failure
 */
int https_client_init(void);

/**
 * @brief Notify HTTPS client that network connection is established
 *
 * This function should be called when the device successfully connects
 * to the network. It will trigger the HTTPS client to start sending
 * periodic requests.
 */
void https_client_notify_connected(void);

/**
 * @brief Notify HTTPS client that network connection is lost
 *
 * This function should be called when the device loses network connectivity.
 * It will pause the HTTPS client from sending requests.
 */
void https_client_notify_disconnected(void);

#endif /* HTTPS_CLIENT_H_ */
