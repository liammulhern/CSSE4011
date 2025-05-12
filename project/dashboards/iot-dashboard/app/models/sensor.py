from __future__ import annotations

from models.models import ClickhouseModel

from dataclasses import dataclass
from typing import  Sequence, List, Tuple

from datetime import datetime

import json

@dataclass 
class Sensor(ClickhouseModel):
    """
        Model for Sensor Messages
    """
    message_type: int
    timestamp: datetime 
    message: dict

    MESSAGE_TYPE_POSITION = 0
    MESSAGE_TYPE_RSSI = 1
    MESSAGE_TYPE_ULTRA = 2
    MESSAGE_TYPE_KALMAN_POSITION = 3

    @classmethod
    def table_create(cls) -> str:
        return """
        CREATE TABLE IF NOT EXISTS uart_messages (
            message_type UInt32,
            timestamp DateTime,
            message JSON
        ) ENGINE = MergeTree()
        ORDER BY timestamp
        """

    @classmethod
    def table_truncate(cls) -> str:
        return """
        TRUNCATE TABLE uart_messages
        """
    @classmethod
    def table_select(cls, limit: int = 100) -> str:
        return f"""
        SELECT message_type, toUnixTimestamp(timestamp) AS timestamp, message FROM uart_messages ORDER BY timestamp LIMIT {limit}
        """

    @classmethod
    def _render_values_for_ch(cls, rows: Sequence[Sensor]) -> List[str]:
        """
        Helper to turn each Sensor into a SQL value tuple.
        """
        rendered: List[str] = []
        for r in rows:
            timestamp_str = r.timestamp.strftime("%Y-%m-%d %H:%M:%S")
            message_json = json.dumps(r.message).replace("'", "\\'")

            rendered.append(
                f"({r.message_type}, toDateTime('{timestamp_str}'), '{message_json}')"
            )
        return rendered

    @classmethod
    def table_insert_rows(cls, rows: Sequence[Sensor]) -> str:
        if not rows:
            raise ValueError("At least one Sensor instance required for insertion")

        values_list = cls._render_values_for_ch(rows)
        values_clause = ",\n    ".join(values_list)

        return f"""
            INSERT INTO uart_messages (message_type, timestamp, message) VALUES {values_clause}
        """

    @classmethod
    def create_from_row(cls, row: Tuple[int, datetime, dict]) -> Sensor:
        return cls(
            message_type=row[0],
            timestamp=row[1],
            message=row[2],
        )

    def render_value(self) -> dict:
        d = {
            'message_type': self.message_type,
            'timestamp': self.timestamp,
            'message': self.message
        }

        return d
