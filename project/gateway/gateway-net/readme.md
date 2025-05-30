# Overview
Receives batched tracker device data via bluetooth and uploads to the gateway via network connection.

# Connection
Gateway connection to Azure IoT Hub using on device MQTT service

> NOTE: Refer to the [Azure IoT Hub MQTT](https://docs.zephyrproject.org/latest/samples/net/cloud/mqtt_azure/README.html) sample for help

west build -b disco_l475_iot1 ~/csse4011/repo-project/project/gateway/gateway-net --pristine


west build -b thingy52/nrf52832  ~/csse4011/repo-project/project/gateway/gateway-net --pristine


west build -b nrf52840dk/nrf52840 ~/csse4011/repo-project/project/gateway/gateway-net --pristine


west build -b esp32c3_devkitm ~/csse4011/repo-project/project/gateway/gateway-net --pristine


west build -b esp32_devkitc_wroom/esp32/procpu --sysbuild ~/csse4011/repo-project/project/gateway/gateway-net --pristine


west build -b m5stack_core2/esp32/procpu --sysbuild ~/csse4011/repo-project/project/gateway/gateway-net --pristine