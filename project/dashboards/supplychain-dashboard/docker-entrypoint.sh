#!/usr/bin/env bash
set -euo pipefail

echo "[entrypoint] Running migrations…"
if ! python manage.py migrate --noinput; then
  echo >&2 "[entrypoint][error] Migrations failed! Aborting startup."
  exit 1
fi

echo "[entrypoint] Launching server…"
exec "$@"

