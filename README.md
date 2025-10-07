# Memfault nRF7002DK Sample

A Memfault integration sample for Nordic nRF7002DK, adapted from the original Nordic Semiconductor Memfault sample. This project demonstrates how to integrate Memfault's crash reporting and monitoring capabilities with Wi-Fi connectivity on the nRF7002DK platform.

## Overview

This sample application is built with:
- nRF Connect SDK v3.1.1
- Memfault Firmware SDK v1.30.0


It showcases:
- Wi-Fi connectivity using nRF7002DK
- Crash reporting and coredump collection
- Custom Metrics collection and heartbeat reporting
- Device OTA based on Memfault cloud

## Hardware Requirements

- nRF7002DK
- Wi-Fi network access for data upload

## Memory Layout

This project uses a custom partition layout optimized for Memfault operation and Wi-Fi connectivity on the nRF7002DK.

The values below are generated from `build/partitions.yml` in the latest build.

### Internal Flash Layout (1MB)

```
┌────────────────────────────────────────────────────────────────┐
│                    nRF5340 Internal Flash (1MB)                │
├────────────────────────────────────────────────────────────────┤
│ 0x00000 ┌─────────────────────────────────────┐                │
│         │            mcuboot                  │ 32KB           │
│         │          (Bootloader)               │ (0x8000)       │
│ 0x08000 ├─────────────────────────────────────┤                │
│         │        mcuboot_pad                  │ 512B           │
│ 0x08200 ├─────────────────────────────────────┤ (0x200)        │
│         │                                     │                │
│         │                                     │                │
│         │                                     │                │
│         │                                     │                │
│         │              app                    │ 918KB          │
│         │        (Main Application)           │ (0xE5E00)      │
│         │                                     │                │
│         │                                     │                │
│         │                                     │                │
│         │                                     │                │
│ 0xEE000 ├─────────────────────────────────────┤                │
│         │       settings_storage              │ 8KB            │
│         │    (Wi-Fi Credentials & Settings)   │ (0x2000)       │
│ 0xF0000 ├─────────────────────────────────────┤                │
│         │                                     │                │
│         │       memfault_storage              │ 64KB           │
│         │     (Crash Dumps & Metrics)         │ (0x10000)      │
│         │                                     │                │
│ 0x100000└─────────────────────────────────────┘                │
└────────────────────────────────────────────────────────────────┘
```

### External Flash Layout (8MB - MX25R64)

```
┌────────────────────────────────────────────────────────────────┐
│                  MX25R64 External Flash (8MB)                  │
├────────────────────────────────────────────────────────────────┤
│ 0x00000 ┌─────────────────────────────────────┐                │
│         │                                     │                │
│         │                                     │                │
│         │                                     │                │
│         │        mcuboot_secondary            │ 918KB          │
│         │      (App Update Slot)              │ (0xE6000)      │
│         │                                     │                │
│         │                                     │                │
│ 0xE6000 ├─────────────────────────────────────┤                │
│         │                                     │                │
│         │                                     │                │
│         │                                     │                │
│         │                                     │                │
│         │         external_flash              │ 7.1MB+         │
│         │      (Available Storage)            │ (0x71A000)     │
│         │                                     │                │
│         │         • Additional Memfault Data  │                │
│         │         • Log Files                 │                │
│         │         • User Data                 │                │
│         │         • Future Expansion          │                │
│         │                                     │                │
│         │                                     │                │
│ 0x800000└─────────────────────────────────────┘                │
└────────────────────────────────────────────────────────────────┘
```

### SRAM Layout (512KB)

```
┌──────────────────────────────────────────────────────────────────┐
│                    nRF5340 SRAM (512KB)                          │
├──────────────────────────────────────────────────────────────────┤
│ 0x20000000 ┌───────────────────────────────────────────────-───┐ │
│            │                                                   │ │
│            │              Application SRAM                     │ │
│            │           (Stack, Heap, Variables)                │ │
│            │                   512KB                           │ │
│            │               (0x80000)                           │ │
│ 0x20080000 └───────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────┘
```

## Building and Usage

### Building Firmware

1. Get your Memfault project key from the Memfault platform and set it in `CONFIG_MEMFAULT_NCS_PROJECT_KEY="your_project_key"` inside `prj.conf`.
2. Build and flash the firmware:
   ```sh
   west build -b nrf7002dk_nrf5340_cpuapp
   west flash --erase
   ```
3. After flashing, connect to the UART shell (115200 baud) and provide Wi-Fi credentials so the device can reconnect automatically on subsequent boots:
   ```
   uart:~$ wifi cred add -s YOURSSID -k 1 -p YOURPASSWORD
   uart:~$ wifi cred auto_connect
   [00:03:45.745,330] <inf> wpa_supp: wlan0: SME: Trying to authenticate with 82:cf:84:3c:f6:41 (SSID='YOURSSID' freq=2437 MHz)
   [00:03:46.116,058] <inf> wpa_supp: wlan0: Trying to associate with 82:cf:84:3c:f6:41 (SSID='YOURSSID' freq=2437 MHz)
   [00:03:46.174,163] <inf> wpa_supp: wlan0: Associated with 82:cf:84:3c:f6:41
   [00:03:46.175,445] <inf> wpa_supp: wlan0: CTRL-EVENT-SUBNET-STATUS-UPDATE status=0
   [00:03:46.216,308] <inf> wpa_supp: wlan0: WPA: Key negotiation completed with 82:cf:84:3c:f6:41 [PTK=CCMP GTK=CCMP]
   [00:03:46.216,796] <inf> wpa_supp: wlan0: CTRL-EVENT-CONNECTED - Connection to 82:cf:84:3c:f6:41 completed [id=0 id_str=]
   Connected
   ```
4. Upload `build/memfault-nrf7002dk/zephyr/zephyr.elf` to **Symbol Files** on the Memfault platform. The device should now appear on the **Devices** list.
5. Review **Timeline** or **Reports** to see the custom metrics collected every 60s or when `BUTTON1` is pressed. Periodic uploads occur according to `CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_INTERVAL_SECS`(60s in this sample). Enable Developer Mode in the Memfault dashboard if you need more frequent uploads during development.

### OTA Process

1. Update `CONFIG_MEMFAULT_NCS_FW_VERSION="2.0.0"` (or your new version number) after modifying the firmware. See [Versioning Schemes](https://docs.memfault.com/docs/platform/software-version-hardware-version#version-schemes) for guidance.
2. Build the new firmware and upload build/memfault-nrf7002dk/zephyr/zephyr.elf to **Symbol Files**, retrieve the OTA payload at `build/memfault-nrf7002dk/zephyr/zephyr.signed.bin`.
3. In the Memfault platform, open the **OTA releases** page. Create a **Full Release** (e.g., version 2.0.0), add the OTA payload to the release, and activate it for the target cohort.
4. Run `mflt_nrf fota` in the device shell to start downloading the new firmware. After the download completes, the device may take several minutes to apply the update and reboot. Check and confirm the new firmware version from device output:
```
[00:00:00.261,535] <inf> memfault_sample: Memfault sample has started! Version: 2.0.0
```

### Automatic OTA Triggers

The sample also provides automated OTA checks so you don't need to issue the shell command manually every time:

- **Button 2 (``DK_BTN2``)**: pressing the button notifies the firmware to run the same flow as `mflt_nrf fota`. A semaphore wakes the OTA thread immediately and the download starts if an update is available.
- **Network connectivity**: each time L4 connectivity is established the OTA thread is nudged so newly connected devices immediately check for pending updates.
- **Periodic background check**: the `mflt_ota_triggers` thread wakes every `OTA_CHECK_INTERVAL` (defaults to `K_MINUTES(60)` in `mflt_ota_triggers.c`) to poll Memfault for a new release. You can override this interval at build time by defining `OTA_CHECK_INTERVAL` (for example, via a compiler flag or by editing the source).

All triggers use the shared Memfault FOTA client, so ensure your project is built with `CONFIG_MEMFAULT_FOTA=y`. If an OTA is already in flight, additional requests (button, connectivity, or periodic) are coalesced until the current attempt finishes.

## License

This project is based on Nordic Semiconductor's Memfault sample and follows the same licensing terms (LicenseRef-Nordic-5-Clause).

## Resources

- [Memfault Documentation](https://docs.memfault.com)
- [nRF Connect SDK Documentation](https://docs.nordicsemi.com/category/software-nrf-connect-sdk)
- [nRF7002DK User Guide](https://docs.nordicsemi.com/category/hardware-development-kits)

## Support

For issues specific to this adaptation:
- Create an issue in this repository

For general Memfault support:
- Visit [Memfault Support](https://docs.memfault.com)

For nRF Connect SDK support:
- Visit [Nordic DevZone](https://devzone.nordicsemi.com)