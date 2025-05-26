"""
Entry point for the serial-terminal CLI.

Uses argparse to parse CLI options, starts the
SerialClient, and reads user input lines to send.

Example

python3 main.py --port /dev/ttyACM0 --baud 115200
"""

import argparse
import logging
from serial_client import SerialClient
from message_parser import parse_buffer
from prompt_toolkit import PromptSession
from prompt_toolkit.patch_stdout import patch_stdout
from azure_iothub import send_json_to_azure_iot_hub, send_test_message_to_azure_iot_hub
from http_client import send_test_message_to_local_server

# Configure root logger
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s"
)


def main():
    """
    Parse arguments, start serial client, and handle user I/O loop.
    """
    parser = argparse.ArgumentParser(
        description="Serial terminal that forwards JSON messages via HTTP"
    )
    parser.add_argument(
        "-p", "--port",
        default="/dev/ttyACM0",
        help="Serial port device"
    )
    parser.add_argument(
        "-b", "--baud",
        type=int,
        default=115200,
        help="Baud rate"
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Enable debug logging"
    )
    parser.add_argument(
        "--az-test",
        action="store_true",
        help="Run a test to send a message to Azure IoT Hub"
    )
    parser.add_argument(
        "--local-test",
        action="store_true",
        help="Run a test to send a message to the local HTTP server"
    )

    args = parser.parse_args()

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    if args.az_test:
        send_test_message_to_azure_iot_hub()
        return

    if args.local_test:
        send_test_message_to_local_server()
        return
 
    client = SerialClient(args.port, args.baud, parser_callback=parse_buffer, received_callback=send_json_to_azure_iot_hub)
    client.start()

    print(f"Connected to {args.port} @ {args.baud}. Type 'exit' to quit.")

    session = PromptSession("> ")
    print(f"Connected to {args.port} @ {args.baud}. Type 'exit' to quit.")

    # patch_stdout lets us safely print() from other threads without breaking the prompt.
    with patch_stdout():
        try:
            while True:
                line = session.prompt()
                if line.strip().lower() == "exit":
                    break
                client.write(line)
        except (KeyboardInterrupt, EOFError):
            print("\nExiting…")
        finally:
            client.stop()

if __name__ == "__main__":
    main()
