# Anedya ESP-IDF Example

[![Anedya Documentation](https://img.shields.io/badge/Anedya-Documentation-blue?style=for-the-badge)](https://docs.anedya.io?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp32-idf)

## Overview

This repository provides an example of how to integrate Anedya with an ESP32 project using the ESP-IDF framework. Anedya is a platform for managing IoT devices, enabling seamless device management, data storage, and analytics.

## Getting Started with Anedya

To set up your project with Anedya, follow these steps:

1. **Create an Anedya Account**:
   - Sign up for an account at [Anedya](https://anedya.io) and log in.

2. **Create a New Project**:
   - In the Anedya Dashboard, create a new project to organize your devices and data.

3. **Define Variables**:
   - Define variables like `temperature` and `humidity` for your project to track device data.

4. **Add a Node**:
   - Create a node to represent a physical device (e.g., "Room1" or "Study Room"). This will allow you to interact with the actual IoT hardware.

> **Note**: Make sure to correctly fill in the variable identifiers as they are crucial for data mapping between the physical device and the Anedya platform.

---

## Menu-Config Setup

To configure your project with the correct settings in ESP-IDF, follow these steps:

1. **Disable certain NVS settings**:
   - Uncheck the following options:
     - Store PHY calibration data in NVS
     - WiFi NVS flash

2. **Select Flash Size**:
   - Set the flash size to `4MB`.

3. **Partition Table**:
   - Choose the `Custom partition table CSV` option.

4. **Add Anedya Root Certificate**:
   - Tick the "Add custom certificates to the default bundle" option.
   - Provide the certificate path, e.g., `./certs`.
   - In the "Default certificate bundle options", select "Use only the most common certificates from the default bundle".

5. **Configure Anedya Credentials**:
   - Provide the following credentials:
     - **Connection Key**
     - **Physical Device ID**

6. **Configure Wi-Fi Credentials**:
   - Provide the Wi-Fi details:
     - **SSID**
     - **Password**

7. **Adjust Stack Size**:
   - Set the "Main stack size" to `10240`.

---

## Example Usage

Depending on your use case, you can enable or disable the tasks in main.c.

---

## Additional Resources

- **Anedya Python SDK**:
   - [PyPI](https://pypi.org/project/anedya-dev-sdk/)
   - [GitHub Repository](https://github.com/anedyaio/anedya-dev-sdk-python)

- **Official Anedya Documentation**:  
   - Visit the [Anedya Documentation](https://docs.anedya.io) for detailed guides and API references.

- **Anedya Website**:
   - Explore more at [Anedya.io](https://anedya.io/?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp32_quectel).
