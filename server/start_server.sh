#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
POSTGRES_ENV_FILE="${REPO_ROOT}/data/configurations/postgres/.env"
SERVER_ENV_FILE="${SCRIPT_DIR}/.env"

if [[ -f "${POSTGRES_ENV_FILE}" ]]; then
  set -a
  # shellcheck disable=SC1090
  source "${POSTGRES_ENV_FILE}"
  set +a
fi

if [[ -f "${SERVER_ENV_FILE}" ]]; then
  set -a
  # shellcheck disable=SC1090
  source "${SERVER_ENV_FILE}"
  set +a
fi

if [[ -z "${PLANT_JOURNAL_ADMIN_SECRET:-}" ]]; then
  cat >&2 <<'EOF'
PLANT_JOURNAL_ADMIN_SECRET is not set.

Set it in one of these ways:
  export PLANT_JOURNAL_ADMIN_SECRET='your-secret'
or create server/.env with:
  PLANT_JOURNAL_ADMIN_SECRET=your-secret
EOF
  exit 1
fi

cd "${REPO_ROOT}"

if [[ -f "${SCRIPT_DIR}/.venv/bin/activate" ]]; then
  # shellcheck disable=SC1091
  source "${SCRIPT_DIR}/.venv/bin/activate"
fi

HOST="${PLANT_JOURNAL_SERVER_HOST:-0.0.0.0}"
PORT="${PLANT_JOURNAL_SERVER_PORT:-8000}"

echo "Starting Plant Journal sync server on ${HOST}:${PORT}"
echo "Database host: ${POSTGRES_HOST:-<from PLANT_JOURNAL_DATABASE_URL>}"
echo "Database name: ${POSTGRES_DB:-<from PLANT_JOURNAL_DATABASE_URL>}"

exec uvicorn server.app.main:app --host "${HOST}" --port "${PORT}"
