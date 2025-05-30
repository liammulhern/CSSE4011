# Overview
Receives batched tracker device data via bluetooth and uploads to the gateway via uart connection.

# Connection
Gateway connection to Azure IoT Hub uses off device transmission via UART protocol (See gateway/gateway-server)


west build -b disco_l475_iot1 ~/csse4011/repo-project/project/gateway/gateway-uart --pristine


west build -b thingy52/nrf52832  ~/csse4011/repo-project/project/gateway/gateway-uart --pristine


west build -b nrf52840dk/nrf52840 ~/csse4011/repo-project/project/gateway/gateway-uart --pristine


west build -b esp32c3_devkitm ~/csse4011/repo-project/project/gateway/gateway-uart --pristine


west build -b esp32_devkitc_wroom/esp32/procpu --sysbuild ~/csse4011/repo-project/project/gateway/gateway-uart --pristine