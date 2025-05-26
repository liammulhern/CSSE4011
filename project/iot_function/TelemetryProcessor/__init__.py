import logging
import json
import azure.functions as func
import hashlib

from typing import Dict, Any, TypedDict

def main(event: func.EventHubEvent):
    # 1) Pull out the raw JSON string
    body = event.get_body().decode("utf-8")
    logging.info(f"Recieved: {body}")

    # 2) Parse it
    try:
        data = json.loads(body)
        logging.info("Parsed telemetry: %s", data)
        # …do your processing here…
    except json.JSONDecodeError:
        logging.warning("Received invalid JSON payload")

def process_sensor_data_function(record: Dict[str, Any]) -> None:
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
