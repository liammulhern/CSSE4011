
#!/usr/bin/env bash
set -euo pipefail

#–– USER CONFIGURATION ––
INTERFACE="wlo1"
SSID="Zephyr-AP"
PASSWORD="zephyr123"
CON_NAME="$SSID"

# IPv4 settings
IPADDR="192.168.4.2/24"
GATEWAY="192.168.4.254"
DNS="8.8.8.8"

#–– END CONFIG ––

# Ensure nmcli is available
if ! command -v nmcli &>/dev/null; then
  echo "Error: nmcli not found. Install NetworkManager." >&2
  exit 1
fi

echo "🔧 Configuring connection profile '$CON_NAME' on interface '$INTERFACE'..."

# If the profile exists, modify it; otherwise create it
if nmcli -t -f NAME connection show | grep -xq "$CON_NAME"; then
  nmcli connection modify "$CON_NAME" \
    connection.interface-name "$INTERFACE" \
    wifi.ssid "$SSID" \
    wifi-sec.key-mgmt wpa-psk \
    wifi-sec.psk "$PASSWORD" \
    ipv4.method manual \
    ipv4.addresses "$IPADDR" \
    ipv4.gateway "$GATEWAY"
else
  nmcli connection add type wifi \
    con-name "$CON_NAME" ifname "$INTERFACE" ssid "$SSID" \
    wifi-sec.key-mgmt wpa-psk \
    wifi-sec.psk "$PASSWORD" \
    ipv4.method manual \
    ipv4.addresses "$IPADDR" \
    ipv4.gateway "$GATEWAY"
fi

echo "🔌 Bringing up '$CON_NAME'..."
nmcli connection up "$CON_NAME"

echo "✅ Connected to '$SSID' on $INTERFACE with static IP $IPADDR"

