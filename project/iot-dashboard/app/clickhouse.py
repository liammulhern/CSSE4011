from clickhouse_connect import get_client

from typing import Callable, Type, List, Sequence, Optional
from flask import Flask, current_app

from models.models import T

import time
import threading
import logging

logger = logging.getLogger(__name__)

class ClickhouseManager:
    def __init__(self, app: Flask, on_connect: Callable) -> None:
        self._lock = threading.Lock()
        self._app = app
        self._on_connect = on_connect
        self._wait_for_clickhouse()

    def _wait_for_clickhouse(self) -> None:
        """
        Wait until ClickHouse is available and initialize the client.

        Tries to connect repeatedly until a successful response or the timeout is reached.
        On success, assigns a persistent client and threading lock.

        :param timeout: Maximum seconds to wait before raising an exception.
        :raises Exception: If ClickHouse is not available within the timeout.
        """
        start_time = time.time()

        while True:
            try:
                print(f'Attempting to connect to {self._app.config["CLICKHOUSE_HOST"]}:{self._app.config["CLICKHOUSE_PORT"]} @ {self._app.config["CLICKHOUSE_USER"]} {self._app.config["CLICKHOUSE_PASSWORD"]}')

                temp_client = get_client(
                    host=self._app.config["CLICKHOUSE_HOST"],
                    port=self._app.config["CLICKHOUSE_PORT"],
                    username=self._app.config["CLICKHOUSE_USER"],
                    password=self._app.config["CLICKHOUSE_PASSWORD"]
                )

                temp_client.command("SELECT 1")

                logger.info("Connected to ClickHouse.")

                break

            except Exception as e:
                elapsed = time.time() - start_time

                if elapsed > self._app.config["CLICKHOUSE_TIMEOUT_S"]:
                    message = "Timeout waiting for ClickHouse to be available."
                    logger.error(message)
                    raise Exception(message)

                logger.warning(
                    "Waiting for ClickHouse to be available... (%ds elapsed) with %s", int(elapsed), e
                )

                time.sleep(5)

        self._client = get_client(
            host=self._app.config["CLICKHOUSE_HOST"],
            port=self._app.config["CLICKHOUSE_PORT"],
            username=self._app.config["CLICKHOUSE_USER"],
            password=self._app.config["CLICKHOUSE_PASSWORD"]
        )

        logger.info(f'Connected to {self._app.config["CLICKHOUSE_HOST"]}:{self._app.config["CLICKHOUSE_PORT"]} @ {self._app.config["CLICKHOUSE_USER"]} {self._app.config["CLICKHOUSE_PASSWORD"]}')

        sql = "SET allow_experimental_object_type = 1;"

        self._run_safe_command(sql, 'init_json')

        self._on_connect(self)

    def model_create(self, model: Type[T]) -> None:
        """
        Create the ClickHouse table for the given model.

        Executes the model's table_create command within a thread-safe lock.

        :param model: Class of ClickhouseModel defining the table schema.
        """
        self._run_safe_command(model.table_create(), f"create table {model.__name__}")

    def model_insert_rows(self, model: Type[T], models: Sequence[T]) -> None:
        """
        Insert records into ClickHouse using the model's insert command.

        :param model: Instance of ClickhouseModel containing data to insert. """
        self._run_safe_command(model.table_insert_rows(models), f"insert rows for {model.__name__}")

    def model_truncate(self, model: Type[T]) -> None:
        """
        Truncate the ClickHouse table for the given model.

        Executes the model's table_truncate command within a thread-safe lock.

        :param model: Class of ClickhouseModel defining the table to truncate.
        """
        self._run_safe_command(model.table_truncate(), f"truncate {model.__name__}")

    def model_select(self, model: Type[T], limit: int = 10_000) -> List[T]:
        """
        Select the ClickHouse table for the given model.

        Executes the model's table_select command within a thread-safe lock.

        :param model: Class of ClickhouseModel defining the table schema.
        """
        result = self._run_safe_query(model.table_select(limit), f"select {model.__name__}", True)

        if result is None:
            return []

        return [model.create_from_row(row) for row in result.result_rows]

    def _run_safe_command(self, sql: str, action: str, return_result: bool = False):
        with self._lock:
            try:
                result = self._client.command(sql)
                logger.info("%s succeeded: %s", action, sql.splitlines()[0])

                return result if return_result else None
            except Exception as e:
                logger.exception("%s failed: %s", action, e)
                raise

    def _run_safe_query(self, sql: str, action: str, return_result: bool = False):
        with self._lock:
            try:
                result = self._client.query(sql)
                logger.info("%s succeeded: %s", action, sql.splitlines()[0])

                return result if return_result else None
            except Exception as e:
                logger.exception("%s failed: %s", action, e)
                raise


class ClickhouseExtension:
    def __init__(self, app: Optional[Flask] = None, on_connect: Optional[Callable] = None) -> None:
        self.manager = None

        if app and on_connect:
            self.init_app(app, on_connect)

    def init_app(self, app: Flask, on_connect: Callable) -> None:
        self.manager = ClickhouseManager(app, on_connect)
        app.extensions['clickhouse'] = self.manager
