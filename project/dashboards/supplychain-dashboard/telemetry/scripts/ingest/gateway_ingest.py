from telemetry.models import GatewayEventRaw
from telemetry.types import GatewayData, Header, Signature, TelemetryPayload, BatchTelemetryPayload, EventPayload, HeartbeatPayload, matches_typed_dict

from supplychain.models import Gateway

from datetime import datetime

def gateway_raw_data_ingest(data: GatewayData) -> GatewayEventRaw:
    """
    Ingests raw gateway data into the GatewayEventRaw model.

    Args:
        data (dict): The raw data to be ingested, expected to contain
                     'message_id', 'gateway_id', 'message_type', 'payload', and 'timestamp'.

    Returns:
        GatewayEventRaw: The created GatewayEventRaw instance.
    """
    # 1. Validate header fields
    if not matches_typed_dict(data['header'], Header):
        raise ValueError("Invalid header format. Must match Header TypedDict.")

    header = data['header']

    # 2. Validate signature fields
    if not matches_typed_dict(data['signature'], Signature):
        raise ValueError("Invalid signature format. Must match Signature TypedDict.")

    signature = data['signature']

    # 3. Validate gateway_id is in database
    gateway = Gateway.objects.filter(gateway_key=header['gatewayId']).first()

    if not gateway:
        raise ValueError(f"Gateway with ID {header['gatewayId']} does not exist.")

    # 4. Validate message type
    valid_message_types = [
        GatewayEventRaw.MESSAGE_TYPE_TELEMETRY,
        GatewayEventRaw.MESSAGE_TYPE_BATCH_TELEMETRY,
        GatewayEventRaw.MESSAGE_TYPE_EVENT,
        GatewayEventRaw.MESSAGE_TYPE_HEARTBEAT
    ]

    if header['messageType'] not in valid_message_types:
        raise ValueError(f"Invalid message type: {header['messageType']}. ")

    # 5. Validate payload structure based on message type
    # Telemetry and Batch Telemetry are not handled here, as they are not part of the GatewayEventRaw model.
    if header['messageType'] == GatewayEventRaw.MESSAGE_TYPE_EVENT:
        if not matches_typed_dict(data['payload'], EventPayload):
            raise ValueError("Invalid payload format for event message type.")
    elif header['messageType'] == GatewayEventRaw.MESSAGE_TYPE_HEARTBEAT:
        if not matches_typed_dict(data['payload'], HeartbeatPayload):
            raise ValueError("Invalid payload format for heartbeat message type.")

    payload = data['payload']

    # 6. Validate timestamp
    if 'timestamp' not in header or not isinstance(header['timestamp'], str):
        raise ValueError("Invalid or missing timestamp in header. Must be a string.")

    try:
        timestamp = datetime.fromisoformat(header['timestamp'])
    except ValueError:
        raise ValueError("Invalid timestamp format. Must be an ISO 8601 string.")

    # 7. Create a new GatewayEventRaw instance
    event = GatewayEventRaw.objects.create(
        message_id=header['messageId'],
        gateway_key=header['gatewayId'],
        message_type=header['messageType'],
        gateway=gateway,
        payload=payload,
        timestamp=timestamp,
        signature=signature,
    )

    return event
