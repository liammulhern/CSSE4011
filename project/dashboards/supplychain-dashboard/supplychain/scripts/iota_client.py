from django.conf import settings

from iota_sdk import Client, utf8_to_hex, hex_to_utf8, HexStr, Block

from typing import Tuple


def iota_build_and_post_block(message_id: str, data_hex: HexStr) -> Tuple[HexStr, Block]:
    """
        Publish zero‐value tagged data: index=messageId, data=hash to 
        IOTA blockchain.

        Args:
            productevent: The product event model to store in the IOTA tangle.

        Returns:
            Tuple of block ID and block meta data 
            after it is stored in the tangle.

    """
    iota_client = Client(nodes=[settings.IOTA_NODE_URL])

    idx_hex = utf8_to_hex(message_id)

    block_id, block = iota_client.build_and_post_block(
        secret_manager=None,
        tag=idx_hex,
        data=data_hex,
    )

    return (block_id, block)

def iota_get_block_data(message_id: str) -> Tuple[str, HexStr]:
    """
         Get the block data for a product event from the IOTA tangle.

         Args:
            product_event: The off-chain model event to lookup in the tangle

        Returns:
            The iota block model
    """
    iota_client = Client(nodes=[settings.IOTA_NODE_URL])

    # Get the block data from the IOTA tangle
    block: Block = iota_client.get_block_data(message_id)

    # Extract and decode tag and data
    tag_hex  = block.payload.tag
    data_hash = block.payload.data

    block_message_id: str = hex_to_utf8(tag_hex)

    return (block_message_id, data_hash)
