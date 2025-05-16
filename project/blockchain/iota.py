import hashlib
import os
import json

from iota_sdk import Client, utf8_to_hex, hex_to_utf8, HexStr, Block
from uuid import uuid4


node_url = os.environ.get('NODE_URL', 'https://api.testnet.iotaledger.net')

# Create a Client instance
iota_client = Client(nodes=[node_url])

def anchor_on_iota(message_id: str, digest_hex: str):
    """
        Publish zero‐value tagged data: index=messageId, data=hash.
    """

    idx_hex = utf8_to_hex(message_id)
    data_hex: HexStr = HexStr("0x" + digest_hex)


    block_id, block = iota_client.build_and_post_block(
        secret_manager=None,
        tag=idx_hex,
        data=data_hex,
    )

    return block_id, block

def compute_hash(message_id: str, payload: dict) -> str:
    """
        SHA-256 over {"id":…, "payload":…} with sorted keys.
    """

    serialized = json.dumps(
        {"id": message_id, "payload": payload},
        sort_keys=True
    )

    return hashlib.sha256(serialized.encode("utf-8")).hexdigest()

def read_from_iota(block_id: HexStr):
    # Retrieve the full block (payload + metadata)
    block: Block = iota_client.get_block_data(block_id)

    # Extract and decode tag and data
    tag_hex  = block.payload.tag
    data_hex = block.payload.data

    message_id: str = hex_to_utf8(tag_hex)
    # digest_hex: str = data_hex

    return message_id, data_hex

payload = {
    "sensor1": 1000
}

# message_id: str = str(uuid4())
# digest_hex = compute_hash(message_id, payload)
#
# print(f"ID: {message_id}")
# print(f"HEX: {digest_hex}")
#
# block_id, block = anchor_on_iota(message_id, digest_hex)
#
# print(f"BLOCK ID: {block_id}")
# print(f"BLOCK: {block}")

message_id, digest_hex = read_from_iota(HexStr("0xf9b40b594669f2b1e4a924387f066e25e266b0562ac85b49f9adace6c53cabdc"))

print(f"ID: {message_id}")
print(f"HEX: {digest_hex}")

digest_hex_comp = compute_hash("0610c255-f315-4cc4-8b7b-643e39ce4732", payload)
digest_hex_comp = HexStr("0x" + digest_hex_comp)

print(digest_hex == digest_hex_comp, digest_hex, digest_hex_comp)



