#!/usr/bin/env bash
set -euo pipefail

# default server settings
HOST="192.168.4.1"
PORT=5000

usage() {
  cat <<EOF
Usage: $0 -x1 X1_POS -y1 Y1_POS -x2 X2_POS -y2 Y2_POS
  -x1  First X coordinate  (e.g. 1 or 2.5)
  -y1  First Y coordinate  (e.g. 1 or 3.14)
  -x2  Second X coordinate (e.g. 4 or 5.5)
  -y2  Second Y coordinate (e.g. 2 or 6.28)
EOF
  exit 1
}

# no args → help
if [[ $# -eq 0 ]]; then
  usage
fi

# parse flags
while [[ $# -gt 0 ]]; do
  case "$1" in
    -x1) X1_POS="$2"; shift 2 ;;
    -y1) Y1_POS="$2"; shift 2 ;;
    -x2) X2_POS="$2"; shift 2 ;;
    -y2) Y2_POS="$2"; shift 2 ;;
    -h|--help) usage ;;
    *)
      echo "ERROR: Unknown option: $1" >&2
      usage
      ;;
  esac
done

# require all four
: "${X1_POS:?ERROR: -x1 is required}"
: "${Y1_POS:?ERROR: -y1 is required}"
: "${X2_POS:?ERROR: -x2 is required}"
: "${Y2_POS:?ERROR: -y2 is required}"

# build *compact* JSON
JSON=$(
  printf "{\"type\":1,\"message\":{\"x1_pos\":$X1_POS,\"y1_pos\":$Y1_POS,\"x2_pos\":$X2_POS,\"y2_pos\":$Y2_POS}}"
)

# send it (no newline!) and quit immediately after send
printf '%s' "$JSON" | nc -w1 -q0 "$HOST" "$PORT"
