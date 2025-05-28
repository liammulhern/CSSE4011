import os
import json
import logging
import threading

from azure.iot.hub import IoTHubRegistryManager
from azure.iot.device import IoTHubDeviceClient, Message
from typing import Callable, Dict, Any, Optional
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

    def __init__(self, message_callback: Optional[Callable] = None, connection_string: Optional[str] = None) -> None:
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

        if message_callback:
            self.message_callback = message_callback

        # create client (under the hood, connects via MQTT on port 8883)
        self.client = IoTHubDeviceClient.create_from_connection_string(conn_str)
        self.client.on_message_received = self.receive_telemetry

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

    def receive_telemetry(self, message: bytes) -> None:
        """
            Receive data from Azure MQTT notifications
        """

        # Log the SDK Message object itself
        logger.info("IoT received: %r", message)

        # Step 1: pull the payload bytes out
        try:
            # `message.data` is usually bytes or bytearray
            raw_bytes = message.data if hasattr(message, "data") else message
            if isinstance(raw_bytes, (bytes, bytearray)):
                raw_str = raw_bytes.decode("utf-8")
            else:
                raw_str = str(raw_bytes)
        except Exception as e:
            logger.error("Failed to extract payload from Message: %s", e, exc_info=True)
            return

        # Step 2: JSON-decode
        try:
            message_parsed = json.loads(raw_str)
            logger.info("Parsed JSON: %s", message_parsed)
        except json.JSONDecodeError as e:
            logger.error("JSON decode error: %s -- payload was: %s", e, raw_str)
            return
        except Exception as e:
            logger.error("Unexpected error during JSON parsing: %s", e, exc_info=True)
            return

        # Step 3: hand off to your callback, catching _its_ errors too
        try:
            self.message_callback(message_parsed)
        except Exception as e:
            logger.error("message_callback raised an exception: %s", e, exc_info=True)
            # swallow it so the SDK handler doesn’t wrap it again

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

def send_iot_hub_test_message(connection_string: Optional[str] = None) -> None:
    """
        Send MQTT message to iothub
    """
    conn_str = connection_string or os.getenv("IOTHUB_CONNECTION_STRING")

    registry_manager = IoTHubRegistryManager.from_connection_string(conn_str)

    # define the device ID
    deviceID = "pathledger-gateway-uart-0"

    message = {
        "messageType": "deviceIDUpdate",
        "message": 10
    }

    message_json = json.dumps(message)

    # send the message through the cloud (IoT Hub) to the device
    registry_manager.send_c2d_message(deviceID, message_json)
