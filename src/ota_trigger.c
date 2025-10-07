/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ota_trigger.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <memfault/nrfconnect_port/fota.h>

LOG_MODULE_REGISTER(ota_trigger, CONFIG_MEMFAULT_SAMPLE_LOG_LEVEL);

#ifndef OTA_CHECK_INTERVAL
#define OTA_CHECK_INTERVAL K_MINUTES(60)
#endif

#define OTA_TRIGGER_THREAD_STACK_SIZE 4096
#define OTA_TRIGGER_THREAD_PRIORITY   K_LOWEST_APPLICATION_THREAD_PRIO

#define OTA_TRIGGER_BUTTON_FLAG  BIT(0)
#define OTA_TRIGGER_CONNECT_FLAG BIT(1)

static K_SEM_DEFINE(ota_trigger_sem, 0, 1);
static atomic_t ota_trigger_flags = ATOMIC_INIT(0);

static const char *consume_trigger_context(void)
{
	atomic_val_t flags = atomic_set(&ota_trigger_flags, 0);

	if (flags == 0) {
		return "manual";
	}

	if ((flags & OTA_TRIGGER_BUTTON_FLAG) && (flags & OTA_TRIGGER_CONNECT_FLAG)) {
		return "button+connect";
	}

	if (flags & OTA_TRIGGER_BUTTON_FLAG) {
		return "button";
	}

	if (flags & OTA_TRIGGER_CONNECT_FLAG) {
		return "connect";
	}

	return "manual";
}

static void schedule_ota_check(const char *context)
{
#if IS_ENABLED(CONFIG_MEMFAULT_FOTA)
	LOG_INF("Starting Memfault OTA check (%s)", context);
	int rv = memfault_fota_start();

	if (rv < 0) {
		LOG_ERR("Memfault OTA check failed (%s), err %d", context, rv);
	} else if (rv == 0) {
		LOG_INF("No new Memfault OTA update available (%s)", context);
	} else {
		LOG_INF("Memfault OTA download started (%s)", context);
	}
#else
	static bool warned;

	if (!warned) {
		LOG_WRN("Memfault OTA support is disabled. Enable CONFIG_MEMFAULT_FOTA to use OTA "
			"checks");
		warned = true;
	}

	ARG_UNUSED(context);
#endif
}

static void ota_trigger_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_INF("Memfault OTA trigger thread started");

	while (true) {
		int ret = k_sem_take(&ota_trigger_sem, OTA_CHECK_INTERVAL);

		if (ret == 0) {
			const char *context = consume_trigger_context();
			schedule_ota_check(context);
		} else if (ret == -EAGAIN) {
			schedule_ota_check("periodic");
		} else {
			LOG_ERR("k_sem_take returned unexpected value %d", ret);
		}
	}
}

K_THREAD_DEFINE(ota_trigger_tid, OTA_TRIGGER_THREAD_STACK_SIZE, ota_trigger_thread, NULL, NULL,
		NULL, OTA_TRIGGER_THREAD_PRIORITY, 0, 0);

void ota_trigger_notify_button(void)
{
	atomic_or(&ota_trigger_flags, OTA_TRIGGER_BUTTON_FLAG);

	if (k_sem_count_get(&ota_trigger_sem) == 0) {
		k_sem_give(&ota_trigger_sem);
		LOG_INF("Memfault OTA check requested by button press");
	} else {
		LOG_DBG("Memfault OTA check already pending");
	}
}

void ota_trigger_notify_connected(void)
{
	atomic_or(&ota_trigger_flags, OTA_TRIGGER_CONNECT_FLAG);

	if (k_sem_count_get(&ota_trigger_sem) == 0) {
		k_sleep(K_SECONDS(10));
		k_sem_give(&ota_trigger_sem);
		LOG_INF("Memfault OTA check scheduled for network connect");
	} else {
		LOG_DBG("Memfault OTA check already pending");
	}
}
