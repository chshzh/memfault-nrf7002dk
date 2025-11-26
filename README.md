# Memfault nRF7002DK Sample

A comprehensive Memfault integration sample for Nordic nRF7002DK, demonstrating production-ready IoT device management with Wi-Fi connectivity, BLE provisioning, HTTPS communication, and cloud-based monitoring.

## Overview

This sample application is built with:
- **nRF Connect SDK v3.1.1**
- **Memfault Firmware SDK v1.30.3**
- **Board**: nRF7002DK (nRF5340 + nRF7002 WiFi companion chip)

> **Note:** The default Memfault Firmware SDK that ships with NCS v3.1.1 is v1.26.0. This project has been updated to use v1.30.3 for improved features and bug fixes. See the "Updating the memfault-firmware-sdk version" section below for upgrade instructions.

## Features

### Core Capabilities
- ✅ **Wi-Fi Connectivity** - WPA2/WPA3 support with automatic reconnection
- ✅ **BLE Provisioning** - Configure WiFi credentials wirelessly via mobile app
- ✅ **HTTPS Client** - Periodic connectivity testing with TLS 1.2/1.3
- ✅ **Crash Reporting** - Automatic coredump collection and upload
- ✅ **Metrics Collection** - WiFi signal strength, stack usage, heap statistics
- ✅ **OTA Updates** - Secure firmware updates via Memfault cloud
- ✅ **MCUBoot Bootloader** - Dual-bank firmware updates with rollback protection
- ✅ **nRF70 FW Stats CDR** - WiFi firmware diagnostics via Custom Data Recording

### Custom Implementations
- **WiFi Metrics** (`mflt_wifi_metrics.c`) - RSSI, connection quality tracking
- **Stack Metrics** (`mflt_stack_metrics.c`) - Per-thread stack usage monitoring
- **OTA Triggers** (`mflt_ota_triggers.c`) - Automatic OTA checks on network events
- **HTTPS Client** (`https_client.c`) - Periodic HEAD requests with certificate validation
- **BLE Provisioning** (`ble_provisioning.c`) - Nordic WiFi Provisioning Service integration
- **nRF70 FW Stats CDR** (`mflt_nrf70_fw_stats_cdr.c`) - WiFi firmware statistics collection for cloud diagnostics

## Hardware Requirements

- **nRF7002DK** development kit
- **USB cable** for power and debugging
- **WiFi Access Point** (2.4 GHz, WPA2/WPA3)
- **Memfault Account** ([sign up free](https://memfault.com))

## Quick Start

1. **Clone the repository** (or copy to your NCS workspace)
2. **Set your Memfault project key** in `prj.conf`:
   ```properties
   CONFIG_MEMFAULT_NCS_PROJECT_KEY="your_project_key_here"
   ```
3. **Build and flash** (recommended: Option 4 with all features):
   ```bash
   west build -b nrf7002dk/nrf5340/cpuapp -p -- -DSB_CONFIG_NETCORE_HCI_IPC=y -DEXTRA_CONF_FILE="overlay-ble-prov.conf;overlay-https-req.conf"
   west flash --erase
   ```
4. **Provision WiFi** using the nRF Wi-Fi Provisioner mobile app
5. **Monitor device** on Memfault dashboard

## Project Structure

```
memfault-nrf7002dk/
├── src/
│   ├── main.c                       # Application entry point
│   ├── https_client.c/h             # HTTPS client implementation
│   ├── ble_provisioning.c/h         # BLE WiFi provisioning
│   ├── mflt_ota_triggers.c/h        # OTA automation logic
│   ├── mflt_wifi_metrics.c/h        # WiFi metrics collection
│   ├── mflt_stack_metrics.c/h       # Stack usage tracking
│   └── mflt_nrf70_fw_stats_cdr.c/h  # nRF70 FW stats CDR collection
├── boards/                           # Board-specific configs
├── cert/                             # TLS certificates
│   └── DigiCertGlobalG3.pem         # Root CA for HTTPS
├── config/                           # Memfault config templates
├── sysbuild/                         # Multi-image build configs
├── prj.conf                          # Main project configuration
├── overlay-ble-prov.conf            # BLE provisioning overlay
├── overlay-https-req.conf           # HTTPS client overlay
├── overlay-nrf70-fw-stats-cdr.conf  # nRF70 FW stats CDR overlay
├── pm_static_*.yml                  # Flash partition layout
└── README.md                         # This file
```

## Advanced Topics

### Updating Memfault Firmware SDK Version

The project uses Memfault SDK v1.30.3 (newer than NCS v3.1.1 default v1.26.0).

To change SDK version:

1. **Edit NCS manifest** at `<ncs_root>/nrf/west.yml`:
   ```yaml
   - name: memfault-firmware-sdk
     path: modules/lib/memfault-firmware-sdk
     revision: 1.30.3  # Change this
     remote: memfault
   ```

2. **Update modules:**
   ```bash
   cd /opt/nordic/ncs/v3.1.1
   west update
   ```

3. **Verify update:**
   ```bash
   cd modules/lib/memfault-firmware-sdk
   git log --oneline -1
   # Should show: 67244e0 Memfault Firmware SDK 1.30.3 (Build 15552)
   ```

### Custom Partition Layout

The project uses static partition configuration in `pm_static_nrf7002dk_nrf5340_cpuapp.yml`.

Key partitions:
- **mcuboot**: 32 KB bootloader
- **app**: 919 KB application (internal flash)
- **settings_storage**: 8 KB WiFi credentials (internal flash)
- **memfault_storage**: 64 KB coredump storage (internal flash)
- **mcuboot_secondary**: 919 KB OTA slot (external flash)

To modify:
1. Edit `pm_static_nrf7002dk_nrf5340_cpuapp.yml`
2. Ensure `app` and `mcuboot_secondary` sizes match
3. Keep `settings_storage` for WiFi credential persistence
4. Rebuild with `-p` to regenerate partitions

### Adding Custom Metrics

Example: Track custom application metrics:

1. **Define metric** in your code:
   ```c
   #include "memfault/metrics/metrics.h"
   
   void record_custom_metric(void) {
       MEMFAULT_METRIC_SET_UNSIGNED(custom_counter, value);
   }
   ```

2. **Add to heartbeat definition:**
   - Edit generated `config/memfault_metrics_heartbeat_config.def`
   - Or use runtime API

See [Memfault Metrics Documentation](https://docs.memfault.com/docs/mcu/metrics-api)

### Non-Volatile Metrics Storage

Currently, metrics use RAM-only storage. To implement persistent metrics:

1. Enable in configuration
2. Implement `memfault_platform_metrics_storage_*` APIs
3. Use `memfault_storage` partition or external flash

See `components/include/memfault/core/platform/nonvolatile_event_storage.h`

### nRF70 Firmware Statistics CDR (Custom Data Recording)

This feature enables collection and upload of nRF70 WiFi firmware statistics (PHY, LMAC, UMAC) to Memfault cloud using the [Custom Data Recording (CDR)](https://docs.memfault.com/docs/mcu/custom-data-recording) feature. This is valuable for diagnosing WiFi connectivity issues in production deployments.

#### Prerequisites

This feature requires a modified Zephyr with vendor statistics support. You need to use the `mf_stats_311` branch from the Nordic fork:

1. **Modify** `/opt/nordic/ncs/v3.1.1/nrf/west.yml`:
   ```yaml
   remotes:
     # Add this remote
     - name: krish2718
       url-base: https://github.com/krish2718
   
   projects:
     - name: zephyr
       repo-path: sdk-zephyr
       remote: krish2718           # Change from ncsinternal
       revision: mf_stats_311      # Use the stats branch
   ```

2. **Update Zephyr:**
   ```bash
   cd /opt/nordic/ncs/v3.1.1/zephyr
   git fetch krish2718 mf_stats_311
   git reset --hard krish2718/mf_stats_311
   ```

#### Building with CDR Support

```bash
west build -b nrf7002dk/nrf5340/cpuapp -p -- \
  -DSB_CONFIG_NETCORE_HCI_IPC=y \
  -DEXTRA_CONF_FILE="overlay-ble-prov.conf;overlay-nrf70-fw-stats-cdr.conf"
west flash --erase
```

#### Usage

**Manual Collection (Button 1 short press):**
- Press Button 1 to collect nRF70 FW stats and upload to Memfault
- The hex blob is printed to console for verification
- Data uploads during the next `memfault_zephyr_port_post_data()` call

**Programmatic Collection:**
```c
#include "mflt_nrf70_fw_stats_cdr.h"

// Collect stats on WiFi events
void on_wifi_event(enum wifi_event event) {
    if (event == WIFI_EVENT_DISCONNECTED || 
        event == WIFI_EVENT_CONNECTION_FAILED) {
        
        int err = mflt_nrf70_fw_stats_cdr_collect();
        if (err == 0) {
            memfault_zephyr_port_post_data();
        }
    }
}
```

#### Recommended Collection Events

For production deployments, collect nRF70 FW stats on these application-level events:

| Event | Description | When to Collect |
|-------|-------------|-----------------|
| **WiFi Connection Lost** | Unexpected disconnection | Before reconnection attempt |
| **DHCP Failure** | Failed to obtain IP address | After DHCP timeout |
| **DNS Failure** | Unable to resolve hostname | After DNS timeout |
| **Low RSSI** | Signal strength below threshold | When RSSI < -80 dBm |
| **Scan/Connect Failure** | Unable to find or connect to AP | After max retries |
| **Cloud Unreachable** | Cannot reach backend server | After connection timeout |
| **Driver Recovery** | WiFi driver initiated recovery | On recovery event |

**Best Practice:** Collect stats **5 consecutive times** for each event to capture temporal patterns:

```c
#define CDR_COLLECTION_COUNT 5
#define CDR_COLLECTION_INTERVAL_MS 1000

void collect_stats_for_event(const char *event_reason) {
    LOG_INF("Collecting nRF70 stats for event: %s", event_reason);
    
    for (int i = 0; i < CDR_COLLECTION_COUNT; i++) {
        int err = mflt_nrf70_fw_stats_cdr_collect();
        if (err) {
            LOG_WRN("Collection %d failed: %d", i + 1, err);
        }
        
        // Upload immediately (CDR has 1/24h limit per device)
        memfault_zephyr_port_post_data();
        
        k_sleep(K_MSEC(CDR_COLLECTION_INTERVAL_MS));
    }
}
```

#### Parsing the Statistics Blob

The uploaded CDR blob contains raw nRF70 firmware statistics (PHY, LMAC, UMAC counters). Parse using the provided Python script:

```bash
# Script location after using mf_stats_311 branch
/opt/nordic/ncs/v3.1.1/modules/lib/nrf_wifi/scripts/nrf70_fw_stats_parser.py

# Parse a downloaded CDR blob
python3 nrf70_fw_stats_parser.py \
  --header /opt/nordic/ncs/v3.1.1/modules/lib/nrf_wifi/fw_if/umac_if/inc/fw/host_rpu_sys_if.h \
  --blob <downloaded_cdr_file.bin>
```

**Cloud-side parsing:** The Python script (or equivalent) can be integrated into Memfault cloud workflows for automatic parsing and analytics.

#### CDR Limitations

> ⚠️ **Important:** Memfault CDR is limited to **1 upload per device per 24 hours** by default.

- Enable **Developer Mode** in Memfault dashboard for higher limits during development
- Plan collection strategy carefully for production deployments
- Consider aggregating multiple events before upload
- Use event annotation (collection_reason) to correlate stats with specific issues

#### Data Flow

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   WiFi Event    │────▶│  CDR Collector  │────▶│    Memfault     │
│  (App-level)    │     │  (Binary Blob)  │     │     Cloud       │
└─────────────────┘     └─────────────────┘     └─────────────────┘
                                                        │
                                                        ▼
                                               ┌─────────────────┐
                                               │  Python Parser  │
                                               │  (Analytics)    │
                                               └─────────────────┘
```

#### Known Issues

When `CONFIG_NET_STATISTICS_ETHERNET_VENDOR=y` is enabled, you may see `<err>` logs during heavy network traffic:

```
<err> wifi_nrf: nrf_wifi_sys_fmac_stats_get: Stats request already pending
```

**These errors are harmless** - they occur because the network stack queries stats on every packet, and concurrent requests are rejected. The CDR button press collection works correctly as an isolated request.

## Contributing

Contributions are welcome! Areas for improvement:
- [ ] 5 GHz WiFi support (when hardware available)
- [ ] Low-power mode optimization
- [ ] Additional metric types
- [ ] More comprehensive tests
- [ ] Documentation improvements

## License

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

### External Flash Layout (8MB - MX25R64)

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
Note about Memfault storage usage
- The `memfault_storage` fixed partition (shown above) is allocated in internal flash and used by the Memfault coredump implementation to persist crash dumps across power cycles.
- In this sample, metrics and event data are collected into an in-RAM event buffer by default. Non-volatile (flash-backed) event storage for metrics is optional and is only used if you enable and provide a platform implementation of the Memfault non-volatile event storage API (see `components/include/memfault/core/platform/nonvolatile_event_storage.h`).
- The partition is available for use by such an implementation (or other user data) but metrics will not be written there unless you implement and enable the NV event storage glue that writes into `memfault_storage`.

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

## Building Firmware

### Prerequisites

Before building, ensure you have:
1. **NCS v3.1.1 toolchain** properly installed
2. **Memfault project key** from your Memfault dashboard
3. **Updated** `CONFIG_MEMFAULT_NCS_PROJECT_KEY` in `prj.conf`

### Build Options Comparison

| Option | WiFi Provisioning | HTTPS Client | nRF70 CDR | BLE Required | Use Case |
|--------|------------------|--------------|-----------|--------------|----------|
| **Option 1** | Shell commands | ❌ | ❌ | No | Development/Testing |
| **Option 2** | BLE mobile app | ❌ | ❌ | Yes | Production WiFi setup |
| **Option 3** | Shell commands | ✅ | ❌ | No | Connectivity testing |
| **Option 4** ⭐ | BLE mobile app | ✅ | ❌ | Yes | **Production (Recommended)** |
| **Option 5** | BLE mobile app | ✅ | ✅ | Yes | WiFi diagnostics (requires mf_stats_311) |

### Option 1: Basic Build (Shell Provisioning Only)

Follow these steps for the standard build that uses shell commands to configure WiFi:

1. Retrieve the project key from your Memfault **Project Settings** page and set it in `CONFIG_MEMFAULT_NCS_PROJECT_KEY="your_project_key"` inside `prj.conf`.
2. Build and flash the firmware:
   ```sh
   west build -b nrf7002dk/nrf5340/cpuapp
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

### Option 2: Building with BLE Provisioning

The BLE provisioning feature allows you to configure WiFi credentials wirelessly via Bluetooth LE using the nRF Wi-Fi Provisioner mobile app:

1. Build with the BLE provisioning overlay:
   ```sh
   west build -b nrf7002dk/nrf5340/cpuapp -p -- -DSB_CONFIG_NETCORE_HCI_IPC=y -DEXTRA_CONF_FILE=overlay-ble-prov.conf
   west flash --erase
   ```

2. Install the **nRF Wi-Fi Provisioner** mobile app:
   - [Android](https://play.google.com/store/apps/details?id=no.nordicsemi.android.wifi.provisioning)
   - [iOS](https://apps.apple.com/app/nrf-wi-fi-provisioner/id1638948698)

3. Provision WiFi credentials:
   - Open the app and scan for BLE devices
   - Connect to device named `PV<MAC>` (e.g., `PV006EB1`)
   - Scan for available WiFi networks
   - Select your network and enter the password
   - Provision the credentials

4. The device will automatically:
   - Connect to the configured WiFi network
   - Reconnect automatically after power cycle using stored credentials
   - Update BLE advertisement with connection status (provisioned, connected, RSSI)

5. **Re-provisioning to a different WiFi network:**
   - The device supports changing to a different WiFi network without erasing flash
   - Simply provision new credentials via the BLE app
   - The device will disconnect from the current network and connect to the new one
   - **Note:** After re-provisioning, you may need to wait ~10 seconds for the connection to establish

### Option 3: Building with HTTPS Client Requests

The HTTPS client feature enables periodic HTTPS HEAD requests to test network connectivity and demonstrate HTTPS functionality:

1. Build with the HTTPS client overlay:
   ```sh
   west build -b nrf7002dk/nrf5340/cpuapp -p -- -DEXTRA_CONF_FILE="overlay-https-req.conf"
   west flash --erase
   ```

2. Configure WiFi credentials using shell commands (or combine with BLE provisioning overlay):
   ```
   uart:~$ wifi cred add -s YOURSSID -k 1 -p YOURPASSWORD
   uart:~$ wifi cred auto_connect
   ```

3. The device will automatically:
   - Send periodic HTTPS HEAD requests to `example.com` (configurable via `CONFIG_HTTPS_HOSTNAME`)
   - Display request/response information in logs
   - Operate on a 10-second interval (configurable via `CONFIG_HTTPS_REQUEST_INTERVAL_SEC`)

4. **Customization options:**
   - Edit `overlay-https-req.conf` to change the target hostname
   - Adjust request interval by modifying `CONFIG_HTTPS_REQUEST_INTERVAL_SEC`
   - Combine with ble overlays: `-DEXTRA_CONF_FILE="overlay-ble-prov.conf;overlay-https-req.conf"`

### Option 4: (BLE + HTTPS - Recommended)

**Best for production deployments** - combines wireless provisioning with connectivity monitoring:

1. **Build with both overlays:**
   ```bash
   west build -b nrf7002dk/nrf5340/cpuapp -p -- \
     -DSB_CONFIG_NETCORE_HCI_IPC=y \
     -DEXTRA_CONF_FILE="overlay-ble-prov.conf;overlay-https-req.conf"
   west flash --erase
   ```

2. **Provision WiFi** via the nRF Wi-Fi Provisioner mobile app:
   - [Android](https://play.google.com/store/apps/details?id=no.nordicsemi.android.wifi.provisioning) | [iOS](https://apps.apple.com/app/nrf-wi-fi-provisioner/id1638948698)
   - Scan and connect to device `PV<MAC>` (e.g., `PV006EB1`)
   - Select WiFi network and enter password
   - Device connects automatically

3. **Features enabled:**
   - ✅ Wireless WiFi provisioning (no USB/shell needed)
   - ✅ Periodic HTTPS requests to `example.com` (every 60 seconds)
   - ✅ Automatic reconnection after power cycles
   - ✅ Re-provisioning support for network changes
   - ✅ Connection status in BLE advertisements

4. **Expected behavior:**
   ```
   [00:00:05.123] <inf> ble_prov: BLE advertising started as 'PV006EB1'
   [00:00:15.456] <inf> ble_prov: WiFi provisioned: SSID=MyNetwork
   [00:00:18.789] <inf> wpa_supp: Connected to MyNetwork
   [00:00:19.012] <inf> https_client: Sending HTTPS request to example.com
   [00:00:19.234] <inf> https_client: HTTP/1.1 200 OK
   ```

### Option 5: Full Features + nRF70 FW Stats CDR (WiFi Diagnostics)

**For advanced WiFi diagnostics** - adds nRF70 firmware statistics collection for debugging connectivity issues:

> ⚠️ **Prerequisite:** Requires the `mf_stats_311` Zephyr branch. See the [nRF70 FW Stats CDR section](#nrf70-firmware-statistics-cdr-custom-data-recording) for setup instructions.

1. **Build with all overlays:**
   ```bash
   west build -b nrf7002dk/nrf5340/cpuapp -p -- \
     -DSB_CONFIG_NETCORE_HCI_IPC=y \
     -DEXTRA_CONF_FILE="overlay-ble-prov.conf;overlay-https-req.conf;overlay-nrf70-fw-stats-cdr.conf"
   west flash --erase
   ```

2. **Features enabled:**
   - ✅ All Option 4 features (BLE provisioning, HTTPS client)
   - ✅ nRF70 firmware statistics collection (PHY, LMAC, UMAC)
   - ✅ CDR upload to Memfault cloud
   - ✅ Console hex blob output for verification

3. **Collect WiFi diagnostics:**
   - **Button 1 short press**: Collects nRF70 stats + uploads to Memfault
   - **Programmatic**: Call `mflt_nrf70_fw_stats_cdr_collect()` on WiFi events

4. **Expected behavior:**
   ```
   [00:01:23.456] <inf> memfault_sample: Button 1 short press detected
   [00:01:23.458] <wrn> mflt_nrf70_fw_stats_cdr: Collecting nRF70 FW stats CDR (limited to 1/24h)...
   nRF70 FW stats hex blob: e2000000000000000000010000000300...
   [00:01:23.512] <inf> mflt_nrf70_fw_stats_cdr: nRF70 FW stats CDR collected (644 bytes), uploading...
   ```

### Configuration Options

#### HTTPS Client Customization

Edit `overlay-https-req.conf` to customize HTTPS behavior:

```properties
# Change target hostname
CONFIG_HTTPS_HOSTNAME="api.myserver.com"

# Adjust request interval (seconds)
CONFIG_HTTPS_REQUEST_INTERVAL_SEC=300  # 5 minutes

# Increase stack size if needed
CONFIG_HTTPS_CLIENT_STACK_SIZE=8192
```

## Memfault Integration

### Onboarding a New Device

1. **Upload symbol file** after building:
   ```bash
   # Symbol file location
   build/memfault-nrf7002dk/zephyr/zephyr.elf
   ```
   - Navigate to **Fleet** → **Symbol Files** in Memfault dashboard
   - Upload `zephyr.elf`
   - Device will appear in **Devices** list after first connection

2. **Monitor device activity:**
   - **Timeline** - Real-time event stream
   - **Metrics** - Heartbeat data (every 15 minutes)
   - **Issues** - Crashes and errors
   - **Traces** - Debug logs (if enabled)

3. **Enable Developer Mode** for faster uploads during development:
   - Default: 900 seconds (15 minutes)
   - Dev Mode: ~60 seconds
   - Configure via `CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_INTERVAL_SECS`

### Performing OTA Updates

Complete workflow for pushing firmware updates:

1. **Update firmware version** in `prj.conf`:
   ```properties
   CONFIG_MEMFAULT_NCS_FW_VERSION="4.1.0"  # Increment version
   ```
   See [Memfault Versioning Schemes](https://docs.memfault.com/docs/platform/software-version-hardware-version#version-schemes)

2. **Build new firmware:**
   ```bash
   west build -b nrf7002dk/nrf5340/cpuapp -p -- \
     -DSB_CONFIG_NETCORE_HCI_IPC=y \
     -DEXTRA_CONF_FILE="overlay-ble-prov.conf;overlay-https-req.conf"
   ```

3. **Upload artifacts to Memfault:**
   - **Symbol file**: `build/memfault-nrf7002dk/zephyr/zephyr.elf`
   - **OTA payload**: `build/memfault-nrf7002dk/zephyr/zephyr.signed.bin`

4. **Create release in Memfault:**
   - Navigate to **Fleet** → **OTA Releases**
   - Click **Create Release**
   - Set version (e.g., `4.1.0`)
   - Add `zephyr.signed.bin` as artifact
   - Activate release for target cohort (e.g., `default`)

5. **Trigger update on device:**
   - **Automatic**: Device checks on network connection + every 60 min
   - **Manual**: Press Button 2 (short press) or run `mflt_nrf fota` in shell

6. **Monitor update progress:**
   ```
   [00:05:12.345] <inf> mflt_ota: Checking for updates...
   [00:05:13.678] <inf> mflt_ota: Update available: 4.1.0
   [00:05:14.012] <inf> fota_download: Downloading...
   [00:07:45.234] <inf> fota_download: Download complete
   [00:07:46.567] <inf> mcuboot: Image confirmed
   [00:07:47.890] <inf> sys: Rebooting...
   [00:00:00.261] <inf> memfault_sample: Memfault sample has started! Version: 4.1.0
   ```

### Collected Metrics

The sample automatically tracks:

| Metric | Type | Description | Collection Interval |
|--------|------|-------------|---------------------|
| `wifi_rssi` | Gauge | WiFi signal strength (dBm) | Every heartbeat (15 min) |
| `wifi_connected` | Counter | Connection state changes | On change |
| `heap_free` | Gauge | Free heap memory (bytes) | Every heartbeat |
| `stack_free_*` | Gauge | Per-thread stack usage | Every heartbeat |
| `http_requests` | Counter | HTTPS request count | On request |
| `ota_checks` | Counter | OTA check attempts | On check |

View metrics in **Fleet** → **Metrics** → Select device

## Device Operation

### Button Functions

Interactive testing and crash generation:

| Button | Press Type | Action | Purpose |
|--------|-----------|---------|---------|
| **Button 1** | Short (< 3s) | Trigger Memfault heartbeat + CDR* | Manual metrics/stats upload |
| **Button 1** | Long (≥ 3s) | Stack overflow crash | Test crash reporting |
| **Button 2** | Short (< 3s) | Check for OTA update | Manual OTA trigger |
| **Button 2** | Long (≥ 3s) | Division by zero crash | Test fault handler |

> *CDR collection only when built with `overlay-nrf70-fw-stats-cdr.conf`

### Automatic OTA Triggers

The firmware automatically checks for updates in these scenarios:

1. **Network Connected** - OTA check immediately after WiFi connection
2. **Button Press** - Button 2 short press triggers manual check
3. **Periodic Check** - Every 60 minutes (configurable in `mflt_ota_triggers.c`)

> **Note:** All triggers use the shared Memfault FOTA client. Concurrent requests are coalesced.


4. **Re-provision via BLE** if using BLE provisioning overlay



This project is based on Nordic Semiconductor's Memfault sample and follows the same licensing terms (LicenseRef-Nordic-5-Clause).

## Resources

### Documentation
- [Memfault Documentation](https://docs.memfault.com) - Complete platform guide
- [Memfault Metrics API](https://docs.memfault.com/docs/mcu/metrics-api) - Custom metrics
- [Memfault OTA Updates](https://docs.memfault.com/docs/mcu/releases-integration-guide) - OTA guide
- [nRF Connect SDK](https://docs.nordicsemi.com/category/software-nrf-connect-sdk) - NCS documentation
- [nRF7002DK User Guide](https://docs.nordicsemi.com/category/hardware-development-kits) - Hardware guide

### Related Projects
- [Nordic Memfault Sample](https://github.com/nrfconnect/sdk-nrf/tree/main/samples/debug/memfault) - Original base
- [Memfault Firmware SDK](https://github.com/memfault/memfault-firmware-sdk) - SDK repository

### Tools
- [nRF Wi-Fi Provisioner App](https://www.nordicsemi.com/Products/Development-tools/nRF-Wi-Fi-Provisioner) - Mobile provisioning
- [nRF Connect for Desktop](https://www.nordicsemi.com/Products/Development-tools/nrf-connect-for-desktop) - Development tools
- [Memfault CLI](https://docs.memfault.com/docs/ci/install-memfault-cli) - Command-line tools

## Support

### For This Project
- **Issues**: [GitHub Issues](https://github.com/chshzh/memfault-nrf7002dk/issues)
- **Discussions**: [GitHub Discussions](https://github.com/chshzh/memfault-nrf7002dk/discussions)

### For Memfault Platform
- **Documentation**: [Memfault Docs](https://docs.memfault.com)
- **Support**: [Memfault Support](https://memfault.com/support)
- **Community**: [Memfault Community Slack](https://memfault.com/slack)

### For nRF Connect SDK
- **DevZone**: [Nordic DevZone](https://devzone.nordicsemi.com)
- **Documentation**: [NCS Docs](https://docs.nordicsemi.com)

## Acknowledgments

This project is based on Nordic Semiconductor's Memfault sample and incorporates enhancements for production IoT deployments.


