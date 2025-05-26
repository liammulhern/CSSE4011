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
    payload = {
      "header": {
        "messageId": "d1c5261e-49ef-4cfc-a782-473549797b01",
        "gatewayId": "GW-01",
        "schemaVersion": "1.0",
        "messageType": "telemetry"
      },
      "payload": {
        "deviceId": "dev-1",
        "time": "1970-01-01T00:00:00",
        "uptime": "494",
        "location": {
          "latitude": "27.5002432",
          "ns": "S",
          "longitude": "153.0153600",
          "ew": "E",
          "altitude_m": "0.0"
        },
        "environment": {
          "temperature_c": "26.22",
          "humidity_percent": "69.00",
          "pressure_hpa": "102.3",
          "gas_ppm": "28.00"
        },
        "acceleration": {
          "x_mps2": "2.413",
          "y_mps2": "-0.459",
          "z_mps2": "-6.511"
        }
      },
      "signature": {
        "alg": "HS256",
        "keyId": "key-001",
        "value": "FBDA00C64513C32B027DA202C4C0574E86CE9FE462C42B8BB3B7734380810DA8"
      }
    }

    send_json_to_azure_iot_hub(payload)
    logger.info("Test message sent to Azure IoT Hub.")
