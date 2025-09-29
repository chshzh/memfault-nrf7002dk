# Memfault nRF7002DKSample

A Memfault integration sample for Nordic nRF7002DK, adapted from the original Nordic Semiconductor Memfault sample. This project demonstrates how to integrate Memfault's crash reporting and monitoring capabilities with Wi-Fi connectivity on the nRF7002DK platform.

## Overview

This sample application is built with:
- nRF Connect SDK v3.1.1
- Memfault Firmware SDK v1.30.0


It showcases:
- Wi-Fi connectivity using nRF7002dk
- Crash reporting and coredump collection
- Metrics collection and heartbeat reporting
- HTTPS data upload to Memfault cloud

## Hardware Requirements

- nRF7002DK
- Wi-Fi network access for data upload

## Features

- **Crash Reporting**: Automatic coredump collection and upload
- **Metrics Collection**: System health monitoring and custom metrics
- **Wi-Fi Connectivity**: Robust Wi-Fi connection management
- **Shell Interface**: Interactive debugging and testing commands
- **Flash Storage**: Persistent storage for coredumps and metrics
- **HTTP Upload**: Secure data transmission to Memfault cloud

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

## Prerequisites

- nRF Connect SDK v3.1.1 (matching this sample)
- nRF Connect for Desktop (recommended)
- Memfault account and project setup
- Wi-Fi network credentials

## Setup

### 1. Memfault Account Setup

1. Create a Memfault account at [memfault.com](https://memfault.com)
2. Create a new project in your Memfault dashboard
3. Obtain your project key from the project settings

### 2. Configuration

1. Update `prj.conf` with your Memfault project key:
   ```
   CONFIG_MEMFAULT_NCS_PROJECT_KEY="your_project_key_here"
   ```

2. Configure your device ID (optional):
   ```
   CONFIG_MEMFAULT_NCS_DEVICE_ID="your_device_id"
   ```

### 3. Wi-Fi Configuration

Configure your Wi-Fi credentials either through:
- Shell commands at runtime
- Compile-time configuration in overlay files

## Building and Flashing

### Using west (recommended)

```bash
# Navigate to the project directory
cd /path/to/memfault-nrf7002dk

# Build for nRF7002dk
west build -b nrf7002dk_nrf5340_cpuapp

# Flash the application
west flash
```

### Using nRF Connect for Desktop

1. Open Programmer in nRF Connect for Desktop
2. Connect your nRF7002dk
3. Add the generated hex file from `build/zephyr/merged.hex`
4. Write the firmware to the device

## Usage

### Shell Commands

After flashing and connecting via serial terminal (115200 baud), you can use these commands:

```
# Check Memfault status
memfault get_core

# Export collected data
memfault export

# Trigger a test crash (for testing purposes)
memfault crash

# Show device info
memfault get_device_info

# Connect to Wi-Fi (if not configured at compile time)
net wifi connect "SSID" "password"
```

### Data Upload

The application automatically uploads data to Memfault when:
- A Wi-Fi connection is established
- Periodic upload interval is reached
- Manual export is triggered via shell

## Project Structure

```
memfault-nrf7002dk/
├── CMakeLists.txt              # Main CMake configuration
├── prj.conf                    # Project configuration
├── Kconfig                     # Custom Kconfig options
├── README.md                   # This file
├── sample.yaml                 # Sample metadata
├── boards/                     # Board-specific configurations
│   └── nrf7002dk_nrf5340_cpuapp.*
├── config/                     # Memfault configuration files
├── src/                        # Application source code
└── overlay-*.conf              # Feature-specific overlays
```

## Customization

### Adding Custom Metrics

Edit `config/memfault_metrics_heartbeat_config.def` to define custom metrics:

```c
MEMFAULT_METRICS_KEY_DEFINE(your_custom_metric, kMemfaultMetricType_Unsigned)
```

### Board-Specific Configuration

Modify files in the `boards/` directory to customize:
- GPIO configurations
- Peripheral settings
- Memory layout
- Power management

## Troubleshooting

### Common Issues

1. **Wi-Fi Connection Issues**
   - Verify network credentials
   - Check signal strength
   - Ensure network supports the device

2. **Memfault Upload Failures**
   - Verify project key configuration
   - Check internet connectivity
   - Review Memfault dashboard for quota limits

3. **Build Issues**
   - Ensure nRF Connect SDK v3.1.1 is installed
   - Verify west tool is properly configured
   - Check for missing dependencies

### Debug Output

Enable verbose logging by setting in `prj.conf`:
```
CONFIG_MEMFAULT_SAMPLE_LOG_LEVEL_DBG=y
CONFIG_MEMFAULT_LOG_LEVEL_DBG=y
```

## Contributing

This project is adapted for specific nRF7002DKusage. Contributions are welcome:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## License

This project is based on Nordic Semiconductor's Memfault sample and follows the same licensing terms (LicenseRef-Nordic-5-Clause).

## Resources

- [Memfault Documentation](https://docs.memfault.com)
- [nRF Connect SDK Documentation](https://docs.nordicsemi.com/category/software-nrf-connect-sdk)
- [nRF7002DKUser Guide](https://docs.nordicsemi.com/category/hardware-development-kits)

## Support

For issues specific to this adaptation:
- Create an issue in this repository

For general Memfault support:
- Visit [Memfault Support](https://docs.memfault.com)

For nRF Connect SDK support:
- Visit [Nordic DevZone](https://devzone.nordicsemi.com)