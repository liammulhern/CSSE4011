"""
Parse incoming byte buffers for JSON payloads embedded in log lines,
and forward valid ones via HTTP.
"""

import json
import logging

from http_client import HttpClient
from settings import SERVER_URL

from prompt_toolkit import print_formatted_text
from prompt_toolkit.formatted_text import ANSI

logger = logging.getLogger(__name__)
_http = HttpClient(SERVER_URL)


def parse_buffer(buf: bytearray) -> bytearray:
    """
    Extract newline-terminated lines from buf, strip off any non-JSON prefix,
    attempt to parse the remainder as JSON, validate schema, and POST to server.

    Args:
        buf: Byte buffer that may contain partial or multiple lines.

    Returns:
        Leftover bytes after processing complete lines.
    """
    while b"\n" in buf:
        line, _, rest = buf.partition(b"\n")
        buf = rest

        # Decode the line and trim whitespace/newlines
        text = line.decode("utf-8", errors="ignore").strip()
        logger.debug("Raw line: %s", text)

        print_formatted_text(ANSI(text))

        # Find the JSON payload start
        json_idx = text.find("{")
        if json_idx == -1:
            logger.debug("No JSON payload found; skipping line.")
            continue

        json_text = text[json_idx:]
        logger.info("JSON candidate: %s", json_text)

        # Try to parse JSON
        try:
            msg = json.loads(json_text)
        except json.JSONDecodeError:
            logger.debug("Malformed JSON; ignoring.")
            continue

        # Minimal schema validation
        if (
            isinstance(msg.get("message_type"), int)
            and isinstance(msg.get("timestamp"), int)
            and isinstance(msg.get("message"), dict)
        ):
            _http.post_json(msg)
        else:
            logger.debug("JSON does not match schema; ignoring.")

    return buf

