# HTTP Sync Guide

## What Changed

The app no longer syncs by opening Postgres directly through Qt's `QPSQL` driver. Sync now happens over HTTP:

- The Qt client keeps SQLite as the local source of truth.
- The app sends JSON payloads to a FastAPI sync service.
- The sync service stores canonical data in Postgres.
- Both desktop and mobile use the same sync path.

Main code changes:

- Client sync transport moved to HTTP in [services/DatabaseSyncService.cpp](/c:/Z_Programming_Backup/Programming/plants/services/DatabaseSyncService.cpp)
- Sync settings are now URL/token based in [services/DatabaseConnectionViewModel.h](/c:/Z_Programming_Backup/Programming/plants/services/DatabaseConnectionViewModel.h)
- The mobile sync UI now asks for server URL and device token in [mobile_app/pages/DatabaseConnectionPage.qml](/c:/Z_Programming_Backup/Programming/plants/mobile_app/pages/DatabaseConnectionPage.qml)
- The FastAPI server lives under [server/app/main.py](/c:/Z_Programming_Backup/Programming/plants/server/app/main.py)
- The server Postgres schema is created from [server/sql/001_init.sql](/c:/Z_Programming_Backup/Programming/plants/server/sql/001_init.sql)

Old Postgres-driver-specific client settings and the `QPSQL` driver test were removed.

## How It Works

### Client Side

The app stores data locally in SQLite and continues to work offline.

When the user presses `Synchronize`:

1. The app reads local records with sync metadata such as `sync_uuid` and `updated_at`.
2. It filters the local rows to only the changes newer than the last saved sync cursor.
3. It sends one `POST /api/v1/sync` request with:
   - `client_id`
   - `last_sync_cursor`
   - `changes`
4. The server applies those changes to Postgres.
5. The server returns:
   - a new cursor
   - applied counts
   - newer server-side rows and tombstones
   - conflict info
6. The app applies the returned server changes back into local SQLite.
7. The app saves the new cursor locally for the next incremental sync.

### Server Side

The FastAPI service:

- authenticates the caller with a bearer device token
- maps the token to one account namespace
- upserts entity rows into Postgres
- stores deletes in `sync_tombstones`
- returns all server-side changes newer than the client cursor

Current entity coverage:

- plants
- journal entries
- reminders
- reminder settings
- plant images
- plant care schedules
- tombstones

### Conflict Rules

The current rule is last-write-wins by `updated_at`.

- If the server copy is newer than the client copy, the server keeps its row and reports a conflict.
- If a tombstone is newer than a stale client upsert, the delete wins.
- The client then receives the canonical server state on the next sync response.

## How A User Uses It

### 1. Start the Sync Server

Install dependencies:

```bash
python -m pip install -r server/requirements.txt
```

Set environment variables:

```bash
set PLANT_JOURNAL_DATABASE_URL=postgresql://USER:PASSWORD@HOST:5432/DBNAME
set PLANT_JOURNAL_ADMIN_SECRET=choose-a-secret
```

Optional:

```bash
set PLANT_JOURNAL_DEFAULT_ACCOUNT=default
```

Run the server:

```bash
uvicorn server.app.main:app --host 0.0.0.0 --port 8000
```

The server applies SQL files from `server/sql/` on startup.

### 2. Create a Device Token

Before a client can sync, create a long-lived token for that device.

Example with `curl`:

```bash
curl -X POST http://YOUR-SERVER:8000/api/v1/device-tokens ^
  -H "Content-Type: application/json" ^
  -H "X-Admin-Secret: choose-a-secret" ^
  -d "{\"device_label\":\"Pixel 8\"}"
```

The response returns:

- `token`
- `device_label`
- `created_at`

Give the returned token to the device user securely.

### 3. Configure the App

In the app, open the sync settings page and enter:

- `Server URL`
  - Example: `http://100.x.y.z:8000`
  - If you are using a VPN hostname, use that host instead.
- `Device token`
  - The token issued by `/api/v1/device-tokens`
- `Device label`
  - Optional display label stored locally in app settings

Press `Save`, then `Synchronize`.

### 4. Normal Sync Behavior

On first sync:

- the client sends `last_sync_cursor = null`
- the server returns the full dataset for that account

On later syncs:

- only records newer than the saved cursor are exchanged

If the device was offline:

- local changes remain in SQLite
- the next successful sync uploads them and downloads newer remote changes

## Operating Notes

- The intended deployment is behind VPN, not public direct database exposure.
- The client no longer requires the Qt Postgres driver, which avoids the Android `QPSQL` problem.
- The sync server still requires a reachable Postgres instance.
- Device tokens are hashed before storage in Postgres.

## Tests Added

Client-side:

- [tests/services/test_database_sync_service.cpp](/c:/Z_Programming_Backup/Programming/plants/tests/services/test_database_sync_service.cpp)
  - verifies HTTP sync applies server changes locally
  - verifies HTTP failures do not mutate SQLite

Server-side:

- [server/tests/test_api.py](/c:/Z_Programming_Backup/Programming/plants/server/tests/test_api.py)
  - verifies bearer auth is required
  - verifies admin secret is required for token creation
  - verifies a successful sync request shape

## Verification Commands

Qt/C++ side:

```bash
cmake --build build
ctest --test-dir build -C Debug --output-on-failure
```

Python server side:

```bash
python -m pytest server/tests -q
```

## Current Limitations

- The server tests currently cover API behavior, auth, and basic sync wiring, not full Postgres integration with real sync conflict scenarios.
- The client sync request is synchronous from the app process perspective.
- The current server model is effectively single-account by default, even though rows are namespaced by `account_id`.

## Recommended Next Steps

- Add Postgres integration tests for full round-trip entity sync and delete tombstones.
- Add a small admin script for device token creation so users do not need to call the endpoint manually.
- Move sync execution off the UI thread if you want a non-blocking user experience during longer network operations.
