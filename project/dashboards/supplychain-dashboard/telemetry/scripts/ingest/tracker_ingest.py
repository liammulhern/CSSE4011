from supplychain.models import Gateway, TrackerEvent, Tracker

from telemetry.models import GatewayEventRaw
from telemetry.types import TelemetryPayload, BatchTelemetryPayload, EventPayload, HeartbeatPayload, matches_typed_dict

from typing import List
from datetime import datetime

import logging

logger = logging.getLogger(__name__)

def tracker_raw_data_ingest_from_gateway(gatewayeventraw: GatewayEventRaw) -> List[TrackerEvent]:
    """ Ingests raw gateway data into the TrackerEvent model.

    Args:
        gatewayeventraw (GatewayEventRaw): The raw gateway event data to be ingested.

    Returns:
        TrackerEvent: The created TrackerEvent instance.
    """
    if gatewayeventraw.message_type == GatewayEventRaw.MESSAGE_TYPE_TELEMETRY:
        return tracker_raw_data_ingest_batch(gatewayeventraw.gateway, gatewayeventraw.payload)
    elif gatewayeventraw.message_type == GatewayEventRaw.MESSAGE_TYPE_BATCH_TELEMETRY:
        return [tracker_raw_data_ingest(gatewayeventraw.gateway, gatewayeventraw.payload)]

    return []

def tracker_raw_data_ingest_batch(gateway: Gateway, payload: BatchTelemetryPayload) -> List[TrackerEvent]:
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
            trackerevent = tracker_raw_data_ingest(gateway, reading)
        except ValueError as e:
            logger.error(f"Failed to ingest reading {reading}: {e}")
            continue

        trackerevents.append(trackerevent)

    trackerevents_created = TrackerEvent.objects.bulk_create(trackerevents)

    return trackerevents_created

def tracker_raw_data_ingest(gateway: Gateway, payload: TelemetryPayload) -> TrackerEvent:
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

    # 3. Validate timestamp
    try:
        timestamp = datetime.fromisoformat(payload['timestamp'])
    except ValueError:
        raise ValueError("Invalid timestamp format. Must be an ISO 8601 string.")

    # 4. Create TrackerEvent instance
    tracker_event = TrackerEvent(
        message_id=payload['messageId'],
        gateway=gateway,
        event_type=TrackerEvent.EVENT_TYPE_TELEMETRY,
        payload=payload,
        timestamp=timestamp,
        data_hash=payload['hash'],
        block_id=payload['blockId'],
    )

    return tracker_event
