#!/usr/bin/env bash
set -euo pipefail

echo "[entrypoint] Running migrations…"
if ! uv run python manage.py migrate --noinput; then
  echo >&2 "[entrypoint][error] Migrations failed! Aborting."
  exit 1
fi

echo "[entrypoint] Collecting static files…"
uv run python manage.py collectstatic --noinput

echo "[entrypoint] Launching server…"
# hands off to CMD below
exec "$@"

