#!/usr/bin/env bash
set -euo pipefail

echo "[entrypoint] making migrations…"
if ! uv run python manage.py makemigrations accounts notifications supplychain telemetry; then
  echo >&2 "[entrypoint][error] migrations failed! aborting."
  exit 1
fi

echo "[entrypoint] running migrations…"
if ! uv run python manage.py migrate --noinput; then
  echo >&2 "[entrypoint][error] migrations failed! aborting."
  exit 1
fi

echo "[entrypoint] Collecting static files…"
uv run python manage.py collectstatic --noinput

echo "[entrypoint] Launching server…"
# hands off to CMD below
exec "$@"

