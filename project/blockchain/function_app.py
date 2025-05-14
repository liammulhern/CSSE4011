from iota_sdk import Client, utf8_to_hex
import azure.functions as func

import logging
import os
import json
import hashlib

import psycopg2

# Init once at cold start
POSTGRES_CONN = os.environ["POSTGRES_CONN"]
IOTA_NODE_URL = os.environ["IOTA_NODE_URL"]

iota_client = Client(nodes=[IOTA_NODE_URL])

app = func.FunctionApp()

@app.function_name(name="ProcessSensorData")
@app.event_hub_message_trigger(arg_name="PathLedger-CSSE4011", 
                               event_hub_name="<EVENT_HUB_NAME>",
                               connection="<CONNECTION_SETTING>") 
def process_sensor_data_function(event: func.EventHubEvent):
    # Parse incoming IoT Hub message
    body  = event.get_body().decode("utf-8")
    record = json.loads(body)

    # TODO: Get each individual tracker message if batch_telemetry

    # Compute SHA‐256 digest
    digest = compute_hash(record["messageId"], record["payload"])

    # Anchor minimal proof on IOTA
    block_id = anchor_on_iota(record["messageId"], digest)

    # Store complete record off‐chain
    msg_id = store_offchain(record, blockId)

    logging.info(f"Anchored msg {msg_id} as IOTA block {block_id}")

def store_offchain(record: dict, block_id: str) -> str:
    """
        Insert full record into Postgres; return its messageId.
    """
    conn = psycopg2.connect(POSTGRES_CONN)
    cur = conn.cursor()

    cur.execute(
        """
        INSERT INTO supplychain_productevent(id, device_id, ts, payload)
        VALUES (%s, %s, %s, %s)
        """,
        (
            record["messageId"],
            record["deviceId"],
            record["timestamp"],
            json.dumps(record["payload"])
        )
    )

    conn.commit()
    cur.close()
    conn.close()

    return record["messageId"]

def compute_hash(message_id: str, payload: dict) -> str:
    """
        SHA-256 over {"id":…, "payload":…} with sorted keys.
    """

    serialized = json.dumps(
        {"id": message_id, "payload": payload},
        sort_keys=True
    )

    return hashlib.sha256(serialized.encode("utf-8")).hexdigest()

def anchor_on_iota(message_id: str, digest_hex: str) -> str:
    """
        Publish zero‐value tagged data: index=messageId, data=hash.
    """

    idx_hex = utf8_to_hex(message_id)
    data_hex = "0x" + digest_hex
    block = iota_client.message(index=idx_hex, data=data_hex).submit()

    return block["blockId"]
