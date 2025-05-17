from iota_sdk import Client, HexStr, utf8_to_hex
import azure.functions as func

import logging
import os
import json
import hashlib
import psycopg2

DATABASE_URL = os.environ["DATABASE_URL"]
AZURE_EVENT_HUB_NAME = os.environ["AZURE_EVENT_HUB_NAME"]
IOTA_NODE_URL = os.environ["IOTA_NODE_URL"]

iota_client = Client(nodes=[IOTA_NODE_URL])

app = func.FunctionApp()

@app.function_name(name="IoTHub-Trigger-Telemetry")
@app.event_hub_message_trigger(arg_name="event", 
                               event_hub_name=AZURE_EVENT_HUB_NAME,
                               connection="IOTHUB_EVENTS_CS") 
def process_sensor_data_function(event: func.EventHubEvent):
    # Parse incoming IoT Hub message
    body  = event.get_body().decode("utf-8")
    record = json.loads(body)

    # Compute SHA‐256 digest
    hash_computed = _compute_hash(record["messageId"], record["payload"])

    logging.info(f"Hashed {record['messageId']} with {hash_computed}")

    if hash_computed != record["dataHash"]:
        logging.error(f"Hash {record['dataHash']} != {hash_computed}")

        # TODO: Send alert to dashboard
        return

    # Anchor minimal proof on IOTA
    block_id = _anchor_on_iota(record["messageId"], record["dataHash"])

    # Store complete record off‐chain
    message_id = _store_offchain(record, block_id)

    logging.info(f"Anchored msg {message_id} as IOTA block {block_id}")

def _store_offchain(record: dict, block_id: str) -> str:
    """
        Insert full record into Postgres; 

        return 
            MessageId.
    """
    conn = psycopg2.connect(
        user=os.environ["DATABASE_USER"],
        password=os.environ["DATABASE_PASSWORD"],
        host=os.environ["DATABASE_HOST"],
        port=os.environ["DATABASE_PORT"], 
        database=os.environ["DATABASE_DB"]
    )

    cur = conn.cursor()

    cur.execute(
        """
        INSERT INTO supplychain_trackerevent(
            message_id, 
            event_type, 
            payload, 
            timestamp, 
            data_hash,
            block_id
        )
        VALUES (
            %s, 
            %s, 
            %s, 
            %s,
            %s,
            %s
        )
        """,
        (
            record["messageId"],
            record["eventType"],
            json.dumps(record["payload"]),
            record["timestamp"],
            record["dataHash"],
            block_id,
        )
    )

    conn.commit()
    cur.close()
    conn.close()

    return record["messageId"]

def _compute_hash(message_id: str, payload: dict) -> str:
    """
        SHA-256 over {"id":…, "payload":…} with sorted keys.
    """

    d = {
        "message_id": message_id,
        "payload": payload
    }

    serialized = json.dumps(d, sort_keys=True)

    return hashlib.sha256(serialized.encode("utf-8")).hexdigest()

def _anchor_on_iota(message_id: str, digest_hex: str) -> str:
    """
        Publish zero‐value tagged data to IOTA blockchain

        Args:
            message_id: The message ID to store on-chain
            digest_hex: The hash ID to store on-chain

        Returns:
            BlockId of stored block
    """

    idx_hex = utf8_to_hex(message_id)
    data_hex = HexStr(digest_hex)

    block_id, _ = iota_client.build_and_post_block(
        secret_manager=None,
        tag=idx_hex,
        data=data_hex,
    )

    return block_id
