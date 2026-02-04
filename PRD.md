# Product Requirements Document (PRD) – Memfault nRF7002DK

## Document Information

- **Product Name**: Memfault nRF7002DK Sample
- **Product ID**: memfault-nrf7002dk
- **Document Version**: 1.0
- **Date Created**: 2026-02-06
- **Status**: Draft
- **Target Release**: Refactoring (dev/refactoring branch)
- **Architecture**: SMF + zbus modular

---

## 1. Executive Summary

### 1.1 Product Overview

Memfault nRF7002DK is a comprehensive Memfault integration sample for Nordic nRF7002DK (nRF5340 + nRF7002 Wi‑Fi). It demonstrates IoT device management with Wi‑Fi STA connectivity, optional BLE provisioning, HTTPS/MQTT clients, and cloud-based monitoring via Memfault (crash reporting, metrics, OTA).

### 1.2 Target Users

- **Primary**: Embedded developers integrating Memfault with nRF70 Wi‑Fi devices
- **Secondary**: QA and support teams validating connectivity and OTA flows

### 1.3 Success Metrics

| Metric | Target | Measurement |
|--------|--------|-------------|
| Build | Clean west build for nrf7002dk/nrf5340/cpuapp | CI / local |
| Connectivity | Wi‑Fi STA connect, Memfault upload | Manual/automated |
| OTA | FOTA via Memfault | Manual test |

---

## 2. Product Requirements

### 2.1 Feature Selection

#### Core Wi‑Fi & Network

| Feature | Selected | Config | Description |
|---------|----------|--------|-------------|
| Wi‑Fi STA | ☑ | `CONFIG_WIFI=y`, `CONFIG_WIFI_NM_WPA_SUPPLICANT=y` | Station mode, connect to AP |
| Wi‑Fi Shell | ☐ (board overlay can enable) | `CONFIG_NET_L2_WIFI_SHELL=y` | Shell commands |
| IPv4 | ☑ | `CONFIG_NET_IPV4=y` | IPv4 only |
| TCP/UDP | ☑ | `CONFIG_NET_TCP=y`, `CONFIG_NET_UDP=y` | Required for HTTP/MQTT |
| DNS | ☑ | `CONFIG_DNS_RESOLVER=y` | Memfault/HTTPS/MQTT |
| DHCP Client | ☑ | `CONFIG_NET_DHCPV4=y` | IP assignment |

#### Optional Features (overlays / Kconfig)

| Feature | Selected | Config | Description |
|---------|----------|--------|-------------|
| BLE Provisioning | ☑ (default in board conf) | `CONFIG_BLE_PROV_ENABLED=y` | Wi‑Fi credentials via nRF Wi‑Fi Provisioner |
| HTTPS Client | ☐ | `CONFIG_HTTPS_CLIENT_ENABLED=y` | Periodic HEAD requests |
| MQTT Client | ☐ | `CONFIG_MQTT_CLIENT_ENABLED=y` | MQTT echo test with TLS |
| nRF70 FW Stats CDR | ☑ (default) | `CONFIG_NRF70_FW_STATS_CDR_ENABLED=y` | PHY/LMAC/UMAC stats to Memfault |

#### Cloud & OTA

| Feature | Selected | Config | Description |
|---------|----------|--------|-------------|
| Memfault | ☑ | `CONFIG_MEMFAULT=y` | Crash, metrics, logs, OTA |
| Memfault FOTA | ☑ | `CONFIG_MEMFAULT_FOTA=y` | OTA via Memfault |
| MCUBoot | ☑ | sysbuild | Dual-bank, rollback |

#### State Management & Architecture

| Feature | Selected | Config | Description |
|---------|----------|--------|-------------|
| SMF | ☑ | `CONFIG_SMF=y` | State Machine Framework |
| Zbus | ☑ | `CONFIG_ZBUS=y` | Message bus between modules |

### 2.2 Functional Requirements (Summary)

- **FR-001**: Device connects to Wi‑Fi (STA) using stored or BLE-provisioned credentials.
- **FR-002**: On L4 connected, Memfault data (heartbeat, coredump, CDR) uploads after DNS ready.
- **FR-003**: Button 1 short press: trigger heartbeat + optional nRF70 CDR; long press: stack overflow (demo).
- **FR-004**: Button 2 short press: OTA check; long press: division-by-zero (demo).
- **FR-005**: Optional BLE provisioning for Wi‑Fi credentials; optional HTTPS/MQTT clients react to Wi‑Fi state via zbus.

### 2.3 Architecture

- **Pattern**: SMF + zbus modular (all modules use SMF where applicable, zbus for events).
- **Modules**: button, wifi (STA), memfault (core, metrics, ota, cdr), ble_prov, https_client, mqtt_client.
- **Channels**: `BUTTON_CHAN`, `WIFI_CHAN`, `MEMFAULT_CHAN`; subscribers react to connection and button events.

### 2.4 Hardware

- **Target**: nRF7002DK (nRF5340 + nRF7002).
- **Memory**: See boards/nrf7002dk_nrf5340_cpuapp.conf (heap, TLS, etc.).

### 2.5 Build & Workspace

- **Workspace**: `west.yml` pins `sdk-nrf` to v3.2.1; app path `memfault-nrf7002dk`.
- **Build**: `west build -p -b nrf7002dk/nrf5340/cpuapp` with optional `-DEXTRA_CONF_FILE="overlay-project-key.conf;overlay-app-https-req.conf;overlay-app-mqtt-echo.conf"`.

---

## 3. Quality Assurance

- **Definition of Done**: All P0 features implemented, build passes, no hardcoded credentials, README and PRD updated.
- **Test**: Wi‑Fi connect, Memfault upload, button actions, optional BLE/HTTPS/MQTT per overlay.

---

## 4. References

- [NCS Documentation](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/index.html)
- [Memfault Documentation](https://docs.memfault.com)
- Project README.md and plan (modular refactoring).
