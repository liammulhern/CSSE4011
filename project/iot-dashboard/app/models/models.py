from __future__ import annotations

from dataclasses import dataclass
from typing import TypeVar, Sequence, Type, List, Tuple


T = TypeVar("T", bound="ClickhouseModel")

@dataclass
class ClickhouseModel:
    @classmethod
    def table_create(cls) -> str:
        return ""

    @classmethod
    def table_truncate(cls) -> str:
        return ""

    @classmethod
    def table_select(cls, limit: int) -> str:
        return ""

    @classmethod
    def create_from_row(cls: Type[T], row: Tuple) -> T:
        return cls()

    @classmethod
    def table_insert_rows(cls: Type[T], rows: Sequence[T]) -> str:
        raise NotImplementedError(
            f"{cls.__name__}.table_insert_rows() must be overridden"
        )


