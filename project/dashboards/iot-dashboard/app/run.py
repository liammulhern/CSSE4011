from flask import Flask

from clickhouse import ClickhouseExtension
from models.sensor import Sensor

import routes
import routes_sensor
import logging
import os

ch_ext = ClickhouseExtension()

logger = logging.getLogger(__name__)

def create_app() -> Flask:
    flask_app: Flask = Flask(__name__)

    config_name = os.environ.get('FLASK_ENV', 'development').capitalize() + 'Config'

    flask_app.config.from_object(f'config.{config_name}')

    gunicorn_logger = logging.getLogger('gunicorn.error')

    flask_app.logger.handlers = gunicorn_logger.handlers
    flask_app.logger.setLevel(gunicorn_logger.level)

    flask_app.register_blueprint(routes.bp)
    flask_app.register_blueprint(routes_sensor.bp)

    ch_ext.init_app(flask_app, create_models)

    logger.info("Initialised flask app")

    return flask_app

def create_models(ch_manager) -> None:
    logger.info(f"Initialising models with {ch_manager}")

    if ch_manager:
        logger.info("Initialised Sensor model")
        ch_manager.model_create(Sensor)

app = create_app()
