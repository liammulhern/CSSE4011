import os
import json
import logging

from azure.iot.device import IoTHubDeviceClient, Message
from typing import Dict, Any
from dotenv import load_dotenv
from datetime import datetime

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

load_dotenv()

class AzureIoTHubMqttClient:
    """
    Simple wrapper around Azure IoT Hub DeviceClient (uses MQTT by default)
    to send JSON-encoded telemetry.
    """

    def __init__(self, connection_string: str = None) -> None:
        """
        If connection_string is None, reads from environment variable
        IOTHUB_DEVICE_CONNECTION_STRING.
        """
        conn_str = connection_string or os.getenv("IOTHUB_DEVICE_CONNECTION_STRING")
        if not conn_str:
            raise ValueError(
                "IoT Hub device connection string must be provided either "
                "via constructor or IOTHUB_DEVICE_CONNECTION_STRING env var"
            )

        # create client (under the hood, connects via MQTT on port 8883)
        self.client = IoTHubDeviceClient.create_from_connection_string(conn_str)
        logger.info("Azure IoT Hub client initialized.")

    def send_telemetry(self, data: Dict[str, Any]) -> None:
        """
        Send a single telemetry message (dict→JSON) to the IoT Hub.
        """
        # Serialize dictionary to JSON string
        payload = json.dumps(data)
        msg = Message(payload)
        msg.content_encoding = "utf-8"
        msg.content_type = "application/json"

        # Send synchronously
        logger.debug("Sending telemetry: %s", payload)
        self.client.send_message(msg)
        logger.info("Telemetry successfully sent.")


    def shutdown(self) -> None:
        """
        Gracefully tear down the MQTT connection.
        """
        self.client.shutdown()
        logger.info("IoT Hub client shut down.")


def send_json_to_azure_iot_hub(data: Dict[str, Any]) -> None:
    """
    Convenience function to send JSON data to Azure IoT Hub.

    Args:
        data: Dictionary containing telemetry data.
        connection_string: Optional connection string for the IoT Hub.
    """
    client = AzureIoTHubMqttClient()

    try:
        client.send_telemetry(data)
    finally:
        client.shutdown()

def send_test_message_to_azure_iot_hub() -> None:
    """
    Send a test message to Azure IoT Hub to verify connectivity.
    """
    test_data = {
        "message": "Hello from Azure IoT Hub client!",
        "timestamp": f"{datetime.now().isoformat()}",
    }
    send_json_to_azure_iot_hub(test_data)
    logger.info("Test message sent to Azure IoT Hub.")
