from iota_sdk import HexStr
from supplychain.models import Gateway, TrackerEvent, Tracker
from supplychain.scripts import compute_event_hash

from notifications.models import TrackerNotification

from telemetry.models import GatewayEventRaw
from telemetry.types import TelemetryPayload, BatchTelemetryPayload, EventPayload, HeartbeatPayload, matches_typed_dict

from typing import List
from datetime import datetime

import logging

logger = logging.getLogger(__name__)

def tracker_raw_data_ingest_from_gatewayevent(gatewayeventraw: GatewayEventRaw) -> List[TrackerEvent]:
    """ Ingests raw gateway data into the TrackerEvent model.

    Args:
        gatewayeventraw (GatewayEventRaw): The raw gateway event data to be ingested.

    Returns:
        TrackerEvent: The created TrackerEvent instance.
    """
    if gatewayeventraw.message_type == GatewayEventRaw.MESSAGE_TYPE_BATCH_TELEMETRY:
            return tracker_raw_data_ingest_batch(gatewayeventraw, gatewayeventraw.pay)
    elif gatewayeventraw.message_type == GatewayEventRaw.MESSAGE_TYPE_TELEMETRY:
        return [tracker_raw_data_ingest(gatewayeventraw, gatewayeventraw.payload)]

    return []

def tracker_raw_data_ingest_batch(gatewayeventraw: GatewayEventRaw, payload: BatchTelemetryPayload) -> List[TrackerEvent]:
    """
    Ingests batch telemetry data into the TrackerEvent model.

    Args:
        payload (BatchTelemetryPayload): The batch telemetry payload to be ingested.

    Returns:
        List[TrackerEvent]: A list of created TrackerEvent instances.
    """
    trackerevents = []

    for reading in payload['readings']:
        try:
            trackerevent = tracker_raw_data_ingest(gatewayeventraw, reading)
        except ValueError as e:
            logger.error(f"Failed to ingest reading {reading}: {e}")
            continue

        trackerevents.append(trackerevent)

    return trackerevents

def tracker_raw_data_ingest(gatewayeventraw: GatewayEventRaw, payload: TelemetryPayload) -> TrackerEvent:
    """
    Ingests telemetry data into the TrackerEvent model.

    Args:
        gateway (Gateway): The gateway associated with the telemetry data.
        payload (TelemetryPayload): The telemetry payload to be ingested.

    Returns:
        TrackerEvent: The created TrackerEvent instance.
    """
    # 1. Validate that tracker exists
    tracker = Tracker.objects.filter(tracker_key=payload['deviceId']).first()

    if not tracker:
        raise ValueError(f"Tracker with ID {payload['deviceId']} does not exist.")

    # 2. Validate payload structure
    if not matches_typed_dict(payload, TelemetryPayload):
        raise ValueError("Invalid payload format. Must match TelemetryPayload TypedDict.")


    time = ""

    # 6. Validate timestamp
    if 'timestamp' not in payload or not isinstance(payload['timestamp'], str):
        if 'time' not in payload or not isinstance(payload['time'], str):
            raise ValueError(f"Invalid or missing timestamp in payload. Must be a string. {payload}")
        else:
            time = payload['time']
    else:
        time = payload['timestamp']


    # 3. Validate timestamp
    try:
        timestamp = datetime.fromisoformat(time)
    except ValueError:
        raise ValueError("Invalid timestamp format. Must be an ISO 8601 string.")

    print(gatewayeventraw.signature.get('value', ''))

    hash = compute_event_hash.compute_tracker_hash(payload)

    # 5. Create TrackerEvent instance
    tracker_event, created = TrackerEvent.objects.get_or_create(
        message_id=gatewayeventraw.message_id,
        tracker=tracker,
        gateway=gatewayeventraw.gateway,
        event_type=TrackerEvent.EVENT_TYPE_TELEMETRY,
        payload=payload,
        timestamp=timestamp,
        data_hash=HexStr(hash),
    )

    if not created:
        logger.error(f"Payload already ingested for {tracker_event.message_id}")
        return tracker_event

    try:
        if HexStr(hash) != tracker_event.compute_hash():
            TrackerNotification.objects.create(
                tracker=tracker,
                message=f"Payload hash mismatch for tracker {tracker.tracker_key}. Expected {tracker_event.compute_hash()}, got {hash}.",
                timestamp=timestamp,
                notication_type=TrackerNotification.NOTICATION_TYPE_ALERT,
            )

            logger.error(f"Payload hash mismatch for tracker {tracker.tracker_key}. Expected {tracker_event.compute_hash()}, got {hash}.")
        else:
            # Upload the tracker event to the blockchain
            block_id = tracker_event.anchor_on_iota()
            logger.info(f"Anchored tracker event {tracker_event.message_id} as IOTA block {block_id}")
    except Exception as e:
        logger.error(f"{e}")
    return tracker_event
