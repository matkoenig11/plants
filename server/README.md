# Plant Journal Sync Server

## Environment

- `PLANT_JOURNAL_DATABASE_URL`: Postgres connection string
- `PLANT_JOURNAL_ADMIN_SECRET`: secret required for `POST /api/v1/device-tokens`
- `PLANT_JOURNAL_DEFAULT_ACCOUNT`: optional, defaults to `default`

If `PLANT_JOURNAL_DATABASE_URL` is not set, the server will also try to build it from:

- `data/configurations/postgres/.env`
- `POSTGRES_HOST`
- `POSTGRES_PORT`
- `POSTGRES_DB`
- `POSTGRES_USER`
- `POSTGRES_PASSWORD`
- `POSTGRES_SSLMODE`

## Install

```bash
python -m pip install -r server/requirements.txt
```

## Run

```bash
uvicorn server.app.main:app --host 0.0.0.0 --port 8000
```

The server applies SQL files from `server/sql/` on startup before serving requests.

Example on PowerShell when using the repo's Postgres `.env` file:

```powershell
$env:PLANT_JOURNAL_ADMIN_SECRET="choose-a-secret"
server\.venv\Scripts\python -m uvicorn server.app.main:app --host 127.0.0.1 --port 8000
```
