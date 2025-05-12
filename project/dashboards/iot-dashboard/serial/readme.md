# Serial CLI JSON Forwarder

A Python command-line interface (CLI) tool that emulates a serial terminal, allowing you to send and receive data over a serial port. Incoming JSON messages matching a specific schema are automatically forwarded via HTTP to a configured server.

## Features

* **Interactive serial terminal**: Send and receive data on a serial port.
* **Configurable connection**: Specify port, baud rate, and logging verbosity via command-line arguments.
* **JSON parsing**: Detects newline-terminated JSON messages matching the schema:

  ```json
  {"type": <int>, "timestamp": <int>, "message": { ... }}
  ```
* **HTTP forwarding**: Automatically POSTs valid JSON messages to a server URL defined in a configuration file.
* **Threaded I/O**: Non-blocking serial read loop in a background thread.

## Project Structure

```
my_serial_cli/
├── config.ini          # INI file for server URL
├── requirements.txt    # Python dependencies
├── README.md           # This README
├── settings.py         # Loads config values
├── serial_client.py    # Serial I/O client (threaded)
├── message_parser.py   # Parses buffer and forwards JSON
├── http_client.py      # HTTP POST helper
└── main.py             # CLI entry point (argparse)
```

## Prerequisites

* Python 3.7+
* pip (for installing dependencies)
* A serial device (e.g., USB-to-UART adapter)
* Access to the target HTTP server endpoint

## Installation

1. (Optional) Create and activate a virtual environment:

   ```bash
   python3 -m venv venv
   source venv/bin/activate
   ```

2. Install Python dependencies:

   ```bash
   pip install -r requirements.txt
   ```

## Configuration

Edit the `config.ini` file to set your server URL:

```ini
[server]
url = http://your-server.example.com/api/sensor
```

## Usage

Run the CLI with the desired serial port and baud rate:

```bash
python main.py --port /dev/ttyACM0 --baud 115200
```

* **--port / -p**: Serial port device (default: `/dev/ttyACM0`)
* **--baud / -b**: Baud rate (default: `115200`)
* **--verbose / -v**: Enable debug logging

Once running, type any text and press **Enter** to send it over serial. Incoming lines that parse as valid JSON matching the schema will be POSTed to your configured server.

To exit, type `exit` or press <kbd>Ctrl+C</kbd>.

## Examples

Start the client listening on `/dev/ttyACM0` at 115200 baud with verbose logging:

```bash
python main.py -p /dev/ttyACM0 -b 115200 -v
```

Sample incoming JSON that will be forwarded:

```json
{"type": 1, "timestamp": 1746137544, "message": {"x": 10, "y": 20}}
```
