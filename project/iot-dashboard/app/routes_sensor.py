from flask import Blueprint, request, jsonify, abort, current_app
from typing import List
from clickhouse import ClickhouseManager
from models.sensor import Sensor
from scripts.viewer_client import send_client_position, update_kalman_position, update_position
from datetime import datetime

bp = Blueprint('api', __name__, url_prefix='/api')

@bp.route("/sensor", methods=["DELETE"])
def sensor_truncate():

    ch_mgr: ClickhouseManager = current_app.extensions['clickhouse']
    ch_mgr.model_truncate(Sensor)

    return jsonify({"status": "truncated"})

@bp.route("/sensor", methods=["GET"])
def sensor_list():

    ch_mgr: ClickhouseManager = current_app.extensions['clickhouse']
    rows: List[Sensor] = ch_mgr.model_select(Sensor)

    return jsonify([row.render_value() for row in rows])

@bp.route("/sensor", methods=["PUT"])
def sensor_insert():
    data = request.get_json(force=True)

    try:
        s = Sensor(
            message_type=data['message_type'],
            message=data['message'],
            timestamp=datetime.now()
        )
    except KeyError:
        abort(400, "'message_type', 'message' and 'timestamp' are required")

    ch_mgr: ClickhouseManager = current_app.extensions['clickhouse']
    ch_mgr.model_insert_rows(Sensor, [s])

    if s.message_type == Sensor.MESSAGE_TYPE_POSITION:
        update_position(s.message['x_pos'], s.message['y_pos'])
        send_client_position(s.message_type)
    elif s.message_type == Sensor.MESSAGE_TYPE_KALMAN_POSITION:
        update_kalman_position(s.message['x_pos'], s.message['y_pos'])
        send_client_position(s.message_type)

    return jsonify({'status': 'created'}), 201
