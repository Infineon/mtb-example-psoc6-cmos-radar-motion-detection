# Changelog for Code Example for the on-chip feature motion detection of XENSIV&trade; BGT60CUTRxx Radar Sensor

## Overview

This code example demonstrates Infineon's radar capabilities by implementing the on-chip feature for motion detection on the CSK hardware platform

### 1.1.0 (public release)

#### Add
- JSON protocol module (`protocol.h`) for structured UART output; all startup messages and error prints now use `protocol_print_info` / `protocol_print_error`
- Host command handling: `get_info` command returns firmware version and sensor information
- `version.h` included for automatic firmware version extraction
- `protocol_set_app_info` and `protocol_set_sensor_info` calls after initialization
- Protocol handler called on every loop iteration to process incoming host commands

#### Change
- Default power mode changed to `DEEP_SLEEP_MODE`; `deep_sleep_after_calibration` set to `false`
- `USE_CASE_NAME` macro used instead of hardcoded application name string
- `print_detections` now takes an additional boolean parameter

### 1.0.0 (public release)

This is the initial release of the code example to demonstrate Infineon's radar capabilities by implementing the on-chip feature for motion detection on the CSK hardware platform

#### Add
- Initializes the board, peripherals, and debug UART.
- Configures GPIOs and the SPI interface for radar sensor communication.
- Initializes the radar sensor and sets up the interrupt handler for motion detection events.
- Sets up the motion detection configuration.
- Streams motion detection events over UART for monitoring.


#### Change

#### Fixed

#### Improved

## Supported software and tools

This version of the XENSIV&trade; BGT60CUTRxx 60GHz radar sensor code example was validated for compatibility with the following software and tools:

| Software and tools                        | Version |
| :---                                      | :----:  |
| ModusToolbox&trade; software environment        | 3.4.0   |
| GCC compiler                              | 10.3.1  |

Minimum required ModusToolbox&trade; software environment: v3.4


© Infineon Technologies AG, 2026.
