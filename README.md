[<img src="https://img.shields.io/badge/Anedya-Documentation-blue?style=for-the-badge">](https://docs.anedya.io?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp-idf)

# Anedya-Example-ESP_IDF

## Menu-Config Setup
    1. Uncheck
        - Store phy calibration data in NVS
        - WiFi NVS flash
    2. Select Flash Size 4MB.
    3. Select `Partition Table` -> Custom partition table csv
    4. Add anedya root certificate
        - Tick "Add custom certificates to the default bundle" and provide path "./certs
        - Select "use only the most common certificates from the default bundle" in "Default certificate bundle options".
    5. Fill Anedya Credentials
        - Connection Key
        - Physical Device ID
    6. Fill Wifi Credentials
        - SSID
        - Password
    7. Allocate "Main stack Size"-"10240".


> [!TIP]
> Looking for Python SDK? Visit [PyPi](https://pypi.org/project/anedya-dev-sdk/) or [Github Repository](https://github.com/anedyaio/anedya-dev-sdk-pyhton)

>[!TIP]
> For more information, visit [anedya.io](https://anedya.io/?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp-idf)