import azure.functions as func
import logging
import json
import requests
import os

def main(event: func.EventHubEvent):
    body = event.get_body().decode("utf-8")
    logging.info(f"Recieved: {body}")

    try:
        data = json.loads(body)
        logging.info("Parsed telemetry: %s", data)

    except json.JSONDecodeError:
        logging.warning("Received invalid JSON payload")
        return

    headers = {
        "Authorization": f"Api-Key {os.getenv('PATHLEDGER_GATEWAY_TELEMETRY_API_KEY')}",
        "Content-Type": "application/json",
    }

    try:
        resp = requests.post(os.getenv("PATHLEDGER_GATEWAY_TELEMETRY_API_URL", ""), headers=headers, json=data, timeout=5)
        logging.info(f"Succeeded with: {resp.status_code}")
    except requests.exceptions.HTTPError as errh:
        logging.error(f"HTTP error: {errh}")
    except requests.exceptions.ConnectionError as errc:
        logging.error(f"Connection error: {errc}")
    except requests.exceptions.Timeout as errt:
        logging.error(f"Timeout: {errt}")
    except requests.exceptions.RequestException as err:
        logging.error(f"{err}")
