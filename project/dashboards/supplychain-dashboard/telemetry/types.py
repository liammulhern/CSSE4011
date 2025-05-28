from typing import TypedDict, List, Type, Dict
from typing_extensions import TypedDict as TypedDictClass

class Header(TypedDict):
    messageId: str
    gatewayId: str
    schemaVersion: str
    messageType: str

class Signature(TypedDict):
    alg: str
    keyId: str
    value: str

class LocationPayload(TypedDict):
    latitude: float
    ns: str  # 'N' or 'S'
    longitude: float
    ew: str  # 'E' or 'W'
    altitude_m: float

class EnvironmentPayload(TypedDict):
    temperature_c: float
    humidity_percent: float
    pressure_hpa: float
    gas_ppm: float

class AccelerationPayload(TypedDict):
    x_mps2: float
    y_mps2: float
    z_mps2: float

#########
# Telemetry
#########

class TelemetryPayload(TypedDict):
    deviceId: str
    timestamp: str
    uptime: str
    location: LocationPayload
    environment: EnvironmentPayload
    acceleration: AccelerationPayload

class GatewayPayloadTelemetry(TypedDict):
    header: Header
    payload: TelemetryPayload
    signature: Signature


#########
# Event
#########

class EventPayload(TypedDict):
    deviceId: str
    eventType: str

class GatewayPayloadEvent(TypedDict):
    header: Header
    payload: EventPayload
    signature: Signature


#########
# Heartbeat
#########

class HeartbeatPayload(TypedDict):
    deviceId: str
    uptime: int
    firmwareVersion: str

class GatewayPayloadHeartbeat(TypedDict):
    header: Header
    payload: HeartbeatPayload
    signature: Signature


#########
# Batch Telemetry
#########

class BatchTelemetryPayload(TypedDict):
    readings: List[TelemetryPayload]

class GatewayPayloadBatchTelemetry(TypedDict):
    header: Header
    payload: BatchTelemetryPayload
    signature: Signature

#########
# Gateway Data Type
#########

class GatewayData(TypedDict):
    header: Header
    payload: TelemetryPayload | EventPayload | HeartbeatPayload | BatchTelemetryPayload
    signature: Signature


def matches_typed_dict(data: Dict, td_cls: Type[TypedDictClass], *, strict: bool = False) -> bool:
    """
    Returns True if `data` has at least all required keys of td_cls.
    If strict=True, then it must have exactly required+optional keys.
    """
    # grab the required/optional key sets
    req = td_cls.__required_keys__
    opt = td_cls.__optional_keys__
    keys = set(data)

    # first, check required keys are all present
    if not req.issubset(keys):
        return False

    # if strict, ensure there are no extra keys
    if strict and not keys.issubset(req | opt):
        return False

    return True
