# function to run on exit
cleanup() {
    echo
    echo "⚠ Terminal died or script exiting — reconnecting to UQ Wi-Fi…"
    nmcli dev wifi connect UQ
}

# catch exit, INT (Ctrl-C), HUP (terminal close) and TERM
trap cleanup EXIT INT HUP TERM

# your startup sequence
docker compose up -d
bash ./tools/connect-ap.sh
./serial/.venv/bin/python3 ./serial/run.py
