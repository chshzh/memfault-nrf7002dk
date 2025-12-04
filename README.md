# Memfault nRF7002DK Sample

A comprehensive Memfault integration sample for Nordic nRF7002DK, demonstrating IoT device management with Wi-Fi connectivity, BLE provisioning, HTTPS communication, and cloud-based monitoring.

## Overview

This sample application showcases:
- **Platform**: nRF7002DK (nRF5340 + nRF7002 WiFi companion chip)
- **SDK**: nRF Connect SDK v3.1.1v3.1.1 (requires a special Zephyr branch - see Prerequisites)
- **Memfault SDK**: v1.31.0 (upgraded from default v1.26.0)

### Key Features

- ✅ **Wi-Fi Connectivity** - WPA2/WPA3 with automatic reconnection
- ✅ **BLE Provisioning** - Wireless WiFi credential configuration via mobile app
- ✅ **Crash Reporting** - Automatic coredump collection and upload to cloud
- ✅ **Metrics Collection** - WiFi stats, stack usage, heap, CPU temperature
- ✅ **nRF70 WiFi Diagnostics** - Firmware statistics (PHY/LMAC/UMAC) via CDR
- ✅ **OTA Updates** - Secure firmware updates via Memfault cloud
- ✅ **MCUBoot Bootloader** - Dual-bank updates with rollback protection

### Optional Features (via overlays)

- 📡 **HTTPS Client** - Periodic connectivity testing (`overlay-https-req.conf`)

## Hardware Requirements

- nRF7002DK development kit
- USB cable for power and debugging
- WiFi Access Point (2.4 GHz, WPA2/WPA3)
- Memfault Account ([sign up free](https://memfault.com))

## Prerequisites

### 1. Memfault SDK v1.31.0

This project uses Memfault SDK v1.31.0 (default NCS v3.1.1 is v1.26.0):

```bash
cd /opt/nordic/ncs/v3.1.1/modules/lib/memfault-firmware-sdk
git pull
git checkout 1.31.0
```

### 2. Zephyr with Vendor Statistics Support

The nRF70 WiFi firmware statistics feature requires a modified Zephyr (will be merged into Zephyr main branch soon):

```bash
cd /opt/nordic/ncs/v3.1.1/zephyr

# Add remote (only needed once)
git remote add krish2718 https://github.com/krish2718/sdk-zephyr.git

# Fetch and checkout stats branch
git fetch krish2718 mf_stats_311
git checkout FETCH_HEAD
```

After updating, rebuild and upload new symbol file.

## Quick Start

1. **Set your Memfault project key** in `prj.conf`:
   ```properties
   CONFIG_MEMFAULT_NCS_PROJECT_KEY="your_project_key_here"
   ```

2. **Build and flash**:
   ```bash
   west build -b nrf7002dk/nrf5340/cpuapp -p
   west flash --erase
   ```

3. **Provision WiFi** using the nRF Wi-Fi Provisioner mobile app:
   - Download: [Android](https://play.google.com/store/apps/details?id=no.nordicsemi.android.wifi.provisioning) | [iOS](https://apps.apple.com/app/nrf-wi-fi-provisioner/id1638948698)
   - Connect to device `PV<MAC>` (e.g., `PV006EB1`)
   - Select your WiFi network and enter password

4. **Monitor device** on Memfault dashboard

## Button Controls

| Button | Press | Action |
|--------|-------|--------|
| **Button 1** | Short (< 3s) | Trigger Memfault heartbeat + nRF70 stats CDR upload |
| **Button 1** | Long (≥ 3s) | Stack overflow crash (test crash reporting) |
| **Button 2** | Short (< 3s) | Check for OTA update |
| **Button 2** | Long (≥ 3s) | Division by zero crash (test fault handler) |

## Project Structure

```
memfault-nrf7002dk/
├── src/
│   ├── main.c                       # Application entry point
│   ├── https_client.c/h             # HTTPS client (optional)
│   ├── ble_provisioning.c/h         # BLE WiFi provisioning
│   ├── mflt_ota_triggers.c/h        # OTA automation logic
│   ├── mflt_wifi_metrics.c/h        # WiFi metrics collection
│   ├── mflt_stack_metrics.c/h       # Stack usage tracking
│   └── mflt_nrf70_fw_stats_cdr.c/h  # nRF70 FW stats CDR
├── boards/
│   └── nrf7002dk_nrf5340_cpuapp.conf # Board-specific config
├── cert/
│   └── DigiCertGlobalG3.pem         # Root CA for HTTPS
├── config/
│   └── memfault_metrics_heartbeat_config.def  # Metric definitions
├── sysbuild/                         # Multi-image build configs
├── prj.conf                          # Main configuration
├── overlay-https-req.conf           # HTTPS client overlay (optional)
├── pm_static_*.yml                  # Flash partition layout
└── README.md
```

---

## Building Firmware

### Default Build (Recommended)

**Includes**: BLE provisioning + WiFi metrics + nRF70 diagnostics CDR

```bash
west build -b nrf7002dk/nrf5340/cpuapp -p
west flash --erase
```

**Features enabled**:
- ✅ BLE WiFi provisioning
- ✅ Memfault crash reporting, metrics, OTA
- ✅ nRF70 firmware statistics CDR (Button 1)
- ✅ WiFi vendor detection (AP OUI lookup)

### With HTTPS Client (Optional)

Adds periodic HTTPS connectivity testing:

```bash
west build -b nrf7002dk/nrf5340/cpuapp -p -- \
  -DEXTRA_CONF_FILE="overlay-https-req.conf"
west flash --erase
```

**Additional features**:
- ✅ Periodic HTTPS HEAD requests to `example.com` (every 60s)
- ✅ Network connectivity monitoring

---

## Flash Memory Layout

This project uses a custom partition layout optimized for Memfault operation and Wi-Fi connectivity.

## App Core Memory

| Memory Region | Used Size | Region Size | Usage |
|---------------|-----------|-------------|-------|
| FLASH | 889460 B | 941568 B | 94.47% |
| RAM | 421912 B | 512 KB | 80.47% |

### Internal Flash (1MB)

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
│         │              app                    │ 919KB          │
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
│         │     (Crash Dumps — coredumps)       │ (0x10000)      │
│         │                                     │                │
│ 0x100000└─────────────────────────────────────┘                │
└────────────────────────────────────────────────────────────────┘
```

### External Flash (8MB - MX25R64)

```
┌────────────────────────────────────────────────────────────────┐
│                  MX25R64 External Flash (8MB)                  │
├────────────────────────────────────────────────────────────────┤
│ 0x00000 ┌─────────────────────────────────────┐                │
│         │                                     │                │
│         │                                     │                │
│         │                                     │                │
│         │        mcuboot_secondary            │ 919KB          │
│         │      (App Update Slot)              │ (0xE6000)      │
│         │                                     │                │
│         │                                     │                │
│ 0xE6000 ├─────────────────────────────────────┤                │
│         │                                     │                │
│         │                                     │                │
│         │                                     │                │
│         │                                     │                │
│         │         external_flash              │ 7.1MB+         │
│         │         (Reserved, Unused)          │ (0x71A000)     │
│         │                                     │                │
│         │    ⚠️  Currently not used by the    │                │
│         │       sample application.           │                │
│         │                                     │                │
│         │    See "External Flash Usage" note  │                │
│         │    below for implementation guide.  │                │
│         │                                     │                │
│ 0x800000└─────────────────────────────────────┘                │
└────────────────────────────────────────────────────────────────┘
```

### Flash Partition Usage

#### `memfault_storage` (Internal Flash)
The 64KB partition stores crash coredumps. **Must** be in internal flash for:
- Power-fail safety (survives brownouts)
- Boot-time access (before external flash init)
- Minimal dependencies (no SPI/QSPI driver needed)

#### `external_flash` (External Flash - Unused)
> ⚠️ The 7.1MB partition is **reserved but currently unused**.

### SRAM (512KB)

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

### Network Core Memory

The nRF5340 network core runs the BLE controller (`hci_ipc`):

| Memory | Used | Total | Usage |
|--------|------|-------|-------|
| FLASH | 151.9 KB | 256 KB | 57.95% |
| RAM | 38.9 KB | 64 KB | 59.35% |

> **Note:** Largest RAM consumer is BLE Controller Memory Pool (15.9 KB). Reduce via `CONFIG_BT_MAX_CONN`.

---

## Basic Memfault Cloud Usage

### Device Onboarding

1. **Upload symbol file** after building:
   - File: `build/memfault-nrf7002dk/zephyr/zephyr.elf`
   - Dashboard: **Fleet** → **Symbol Files** → Upload

2. **Monitor device**:
   - **Timeline**: Real-time event stream
   - **Metrics**: Heartbeat data (every 15 min)
   - **Issues**: Crashes and errors

3. **Enable Developer Mode** for faster uploads:
   - Default: 900s (15 min)
   - Dev Mode: ~60s

### Collected Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `wifi_rssi` | Gauge | Signal strength (dBm) |
| `wifi_sta_*` | Gauge | Channel, beacon interval, DTIM, TWT |
| `wifi_ap_oui_vendor` | String | AP vendor (Cisco, Apple, ASUS, etc.) |
| `heap_free` | Gauge | Free heap memory |
| `stack_free_*` | Gauge | Per-thread stack usage |

### OTA Updates

1. **Update version** in `prj.conf`:
   ```properties
   CONFIG_MEMFAULT_NCS_FW_VERSION="4.1.0"
   ```

2. **Build and upload**:
   ```bash
   west build -b nrf7002dk/nrf5340/cpuapp -p
   ```
   - Upload `zephyr.elf` (symbols)
   - Upload `zephyr.signed.bin` (OTA payload)

3. **Create release**:
   - Dashboard: **Fleet** → **OTA Releases**
   - Activate for device cohort

4. **Trigger update**:
   - **Auto**: On WiFi connect + every 60 min
   - **Manual**: Button 2 short press

---

## Advanced Topics

### nRF70 WiFi Firmware Statistics (CDR)

The nRF70 firmware statistics feature is **enabled by default** and collects WiFi diagnostics via Memfault's Custom Data Recording.

#### Usage

**Manual Collection**:
- Press **Button 1** (short press) to collect and upload nRF70 stats

**Programmatic Collection**:
```c
#include "mflt_nrf70_fw_stats_cdr.h"

void on_wifi_failure(void) {
    int err = mflt_nrf70_fw_stats_cdr_collect();
    if (err == 0) {
        memfault_zephyr_port_post_data();
    }
}
```

#### Recommended Collection Events

| Event | When to Collect |
|-------|------------------|
| WiFi Connection Lost | Before reconnection attempt |
| DHCP/DNS Failure | After timeout |
| Low RSSI | When < -80 dBm |
| Cloud Unreachable | After connection timeout |

#### Parsing Statistics

Download CDR blob from Memfault and parse:

```bash
python3 script/nrf70_fw_stats_parser.py \
  /opt/nordic/ncs/v3.1.1/modules/lib/nrf_wifi/fw_if/umac_if/inc/fw/host_rpu_sys_if.h \
  ~/Downloads/F4CE36006EB1_nrf70-fw-stats_20251128-111955.bin
```

**Output includes**: PHY stats (RSSI, CRC), LMAC stats (TX/RX counters), UMAC stats (events, packets)

#### CDR Limitations

> ⚠️ **1 upload per device per 24 hours** by default. Contact Memfault support to increase limits for debugging.

### Custom Metrics

Add to `config/memfault_metrics_heartbeat_config.def`:

```c
MEMFAULT_METRICS_KEY_DEFINE(custom_counter, kMemfaultMetricType_Unsigned)
```

Record in code:
```c
MEMFAULT_METRIC_SET_UNSIGNED(custom_counter, value);
```

### Partition Layout Customization

Edit `pm_static_nrf7002dk_nrf5340_cpuapp.yml`:
- Ensure `app` and `mcuboot_secondary` match sizes
- Keep `settings_storage` for WiFi credentials
- Rebuild with `-p` flag

---

## Resources

- **Memfault**: [Documentation](https://docs.memfault.com) | [Metrics API](https://docs.memfault.com/docs/mcu/metrics-api) | [OTA Guide](https://docs.memfault.com/docs/mcu/releases-integration-guide)
- **Nordic**: [NCS Docs](https://docs.nordicsemi.com) | [nRF7002DK Guide](https://docs.nordicsemi.com/category/hardware-development-kits) | [DevZone](https://devzone.nordicsemi.com)
- **Tools**: [nRF Wi-Fi Provisioner](https://www.nordicsemi.com/Products/Development-tools/nRF-Wi-Fi-Provisioner) | [nRF Connect Desktop](https://www.nordicsemi.com/Products/Development-tools/nrf-connect-for-desktop)

## License

Based on Nordic Semiconductor's Memfault sample (LicenseRef-Nordic-5-Clause).
