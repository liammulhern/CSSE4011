#!/usr/bin/env bash
set -euo pipefail

VM_USER=liam
VM_HOST=pathledger.live

#–– CONFIGURE these before running: ––
: "${VM_USER:?Need to set VM_USER (e.g. liam)}"
: "${VM_HOST:?Need to set VM_HOST (e.g. pathledger.live)}"
: "${VM_PATH:=~/pathledger}"     # where your compose.yml lives on the VM

#–– SCRIPT START ––
TIMESTAMP=$(date +%Y%m%d%H%M%S)
ZIP_NAME="deploy_${TIMESTAMP}.zip"

echo "Creating archive ${ZIP_NAME}..."
zip -r "${ZIP_NAME}" . \
    -x '*.git*' \
    -x '*node_modules/*' \
    -x '*.env*' \
    -x '*/__pycache__/*' \
    -x '*.venv*'

echo "Copying to ${VM_USER}@${VM_HOST}:/tmp/${ZIP_NAME}..."
scp "${ZIP_NAME}" "${VM_USER}@${VM_HOST}:/tmp/${ZIP_NAME}"

echo "Connecting to ${VM_HOST} to unpack & restart containers..."
ssh "${VM_USER}@${VM_HOST}" bash <<EOF
  set -euo pipefail
  echo "  • Unpacking to ${VM_PATH}…"
  unzip -o "/tmp/${ZIP_NAME}" -d "${VM_PATH}"
  echo "  • Entering ${VM_PATH}…"
  cd "${VM_PATH}"
  echo "  • Pulling images & (re)building…"
  docker compose pull
  docker compose up -d --build
  echo "  • Cleaning up…"
  rm "/tmp/${ZIP_NAME}"
EOF

echo "Cleaning up local archive..."
rm "${ZIP_NAME}"

echo "Deployment complete!"

