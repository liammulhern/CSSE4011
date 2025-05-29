from django.conf import settings
from azure.iot.hub import IoTHubRegistryManager

import os
import json

def send_iot_hub_test_message(deviceID: str, message: dict) -> None:
    """
        Send MQTT message to iothub
    """
    registry_manager = IoTHubRegistryManager.from_connection_string(settings.IOTHUB_CONNECTION_STRING)


    message_json = json.dumps(message)

    # send the message through the cloud (IoT Hub) to the device
    registry_manager.send_c2d_message(deviceID, message_json)
