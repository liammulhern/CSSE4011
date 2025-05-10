"""
Simple HTTP client for posting JSON payloads.
"""

import logging
import requests

logger = logging.getLogger(__name__)


class HttpClient:
    """
    Wraps requests.post to send JSON payloads.

    Attributes:
        server_url: The URL to POST to.
    """

    def __init__(self, server_url: str):
        """
        Initialize with target server URL.

        Args:
            server_url: Endpoint to which JSON messages are sent.
        """
        self.server_url = server_url

    def post_json(self, payload: dict):
        """
        POST a JSON payload to the configured server.

        Args:
            payload: The dict to serialize as JSON.
        """
        try:
            resp = requests.put(self.server_url, json=payload, timeout=5)
            resp.raise_for_status()

            logger.info("Posted JSON → %s [%d]", self.server_url, resp.status_code)

        except Exception as exc:
            logger.error("Failed to POST JSON: %s", exc)

