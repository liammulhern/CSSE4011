import json
import socket
import logging
from typing import Any

from flask import current_app

logger = logging.getLogger(__name__)

position_x: float = 0
position_y: float = 0
kalman_x: float = 0
kalman_y: float = 0

def update_kalman_position(
    x_pos: float,
    y_pos: float
) -> None:
    global position_y, position_x

    position_x = x_pos
    position_y = y_pos

def update_position(
    x_pos: float,
    y_pos: float
) -> None:
    global kalman_x, kalman_y

    kalman_x = x_pos
    kalman_y = y_pos

def send_client_position(
    message_type: int,
) -> None:
    """
    Send a JSON‐encoded position update to the viewer client over TCP.

    Expects VIEWER_IPADDR and VIEWER_PORT in Flask's app.config.
    """
    host: str = current_app.config["VIEWER_IPADDR"]
    port: int = current_app.config["VIEWER_PORT"]
    addr = (host, port)

    payload: dict[str, Any] = {
        "type": message_type,
        "message": {
            "x1_pos": position_x,
            "y1_pos": position_y,
            "x2_pos": kalman_x,
            "y2_pos": kalman_y,
        }
    }

    try:
        with socket.create_connection(addr) as sock:
            logger.info("Connected to %s:%d", host, port)
            data = json.dumps(payload).encode("utf-8")
            sock.sendall(data)
            logger.debug("Sent payload: %s", payload)

    except socket.error as err:
        logger.error("Socket error sending to %s:%d — %s", host, port, err)

    except Exception:
        logger.exception("Unexpected error sending position to %s:%d", host, port)

