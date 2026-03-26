from __future__ import annotations

from pathlib import Path
import sys

import psycopg


ROOT = Path(__file__).resolve().parents[2]
ENV_PATH = ROOT / "data" / "configurations" / "postgres" / ".env"


def load_env_file(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        values[key.strip()] = value.strip()
    return values


def require(values: dict[str, str], key: str) -> str:
    value = values.get(key, "").strip()
    if not value:
        raise RuntimeError(f"Missing required env value: {key}")
    return value


def statements() -> list[str]:
    return [
        """
        CREATE TABLE IF NOT EXISTS plants (
            id SERIAL PRIMARY KEY,
            sync_uuid TEXT UNIQUE NOT NULL,
            name TEXT NOT NULL,
            species TEXT,
            location TEXT,
            scientific_name TEXT,
            plant_type TEXT,
            light_requirement TEXT,
            watering_frequency TEXT,
            watering_notes TEXT,
            humidity_preference TEXT,
            soil_type TEXT,
            last_watered TEXT,
            fertilizing_schedule TEXT,
            last_fertilized TEXT,
            pruning_time TEXT,
            pruning_notes TEXT,
            last_pruned TEXT,
            growth_rate TEXT,
            issues_pests TEXT,
            temperature_tolerance TEXT,
            toxic_to_pets TEXT,
            poisonous_to_humans TEXT,
            poisonous_to_pets TEXT,
            indoor TEXT,
            flowering_season TEXT,
            acquired_on TEXT,
            source TEXT,
            notes TEXT,
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS journal_entries (
            id SERIAL PRIMARY KEY,
            sync_uuid TEXT UNIQUE NOT NULL,
            plant_id INTEGER NOT NULL REFERENCES plants(id) ON DELETE CASCADE,
            entry_type TEXT NOT NULL,
            entry_date TEXT NOT NULL,
            notes TEXT,
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS reminders (
            id SERIAL PRIMARY KEY,
            sync_uuid TEXT UNIQUE NOT NULL,
            custom_name TEXT,
            task_type TEXT NOT NULL,
            schedule_type TEXT NOT NULL,
            interval_days INTEGER,
            start_date TEXT,
            next_due_date TEXT NOT NULL,
            notes TEXT,
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS reminder_plants (
            reminder_id INTEGER NOT NULL REFERENCES reminders(id) ON DELETE CASCADE,
            plant_id INTEGER NOT NULL REFERENCES plants(id) ON DELETE CASCADE,
            PRIMARY KEY (reminder_id, plant_id)
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS reminder_settings (
            id INTEGER PRIMARY KEY,
            day_before_enabled INTEGER NOT NULL DEFAULT 1,
            day_before_time TEXT NOT NULL DEFAULT '18:00',
            day_of_enabled INTEGER NOT NULL DEFAULT 1,
            day_of_time TEXT NOT NULL DEFAULT '08:00',
            overdue_enabled INTEGER NOT NULL DEFAULT 1,
            overdue_cadence_days INTEGER NOT NULL DEFAULT 2,
            overdue_time TEXT NOT NULL DEFAULT '09:00',
            quiet_hours_start TEXT,
            quiet_hours_end TEXT,
            updated_at TEXT NOT NULL
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS plant_images (
            id SERIAL PRIMARY KEY,
            sync_uuid TEXT UNIQUE NOT NULL,
            plant_id INTEGER NOT NULL REFERENCES plants(id) ON DELETE CASCADE,
            file_path TEXT NOT NULL,
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL,
            UNIQUE (plant_id, file_path)
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS plant_care_schedules (
            id SERIAL PRIMARY KEY,
            sync_uuid TEXT UNIQUE NOT NULL,
            plant_id INTEGER NOT NULL REFERENCES plants(id) ON DELETE CASCADE,
            care_type TEXT NOT NULL,
            season_name TEXT NOT NULL,
            interval_days INTEGER,
            is_enabled INTEGER NOT NULL DEFAULT 1,
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL,
            UNIQUE (plant_id, care_type, season_name)
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS sync_tombstones (
            table_name TEXT NOT NULL,
            sync_uuid TEXT NOT NULL,
            deleted_at TEXT NOT NULL,
            PRIMARY KEY (table_name, sync_uuid)
        )
        """,
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_plants_sync_uuid ON plants(sync_uuid)",
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_journal_entries_sync_uuid ON journal_entries(sync_uuid)",
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_reminders_sync_uuid ON reminders(sync_uuid)",
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_plant_images_sync_uuid ON plant_images(sync_uuid)",
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_plant_care_schedules_sync_uuid ON plant_care_schedules(sync_uuid)",
        "CREATE INDEX IF NOT EXISTS idx_plant_images_plant_id ON plant_images(plant_id, created_at, id)",
        "CREATE INDEX IF NOT EXISTS idx_plant_care_schedules_lookup ON plant_care_schedules(plant_id, care_type, season_name)",
        """
        INSERT INTO reminder_settings (id, updated_at)
        VALUES (1, CURRENT_TIMESTAMP::text)
        ON CONFLICT (id) DO NOTHING
        """,
    ]


def main() -> int:
    if not ENV_PATH.exists():
        raise RuntimeError(f"Env file not found: {ENV_PATH}")

    env = load_env_file(ENV_PATH)
    conninfo = {
        "host": require(env, "POSTGRES_HOST"),
        "port": int(require(env, "POSTGRES_PORT")),
        "dbname": require(env, "POSTGRES_DB"),
        "user": require(env, "POSTGRES_USER"),
        "password": require(env, "POSTGRES_PASSWORD"),
        "sslmode": env.get("POSTGRES_SSLMODE", "prefer").strip() or "prefer",
    }

    with psycopg.connect(**conninfo) as connection:
        with connection.cursor() as cursor:
            for statement in statements():
                cursor.execute(statement)
        connection.commit()

    print(
        "Initialized Postgres schema for "
        f"{conninfo['host']}:{conninfo['port']}/{conninfo['dbname']}"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:  # pragma: no cover
        print(f"Initialization failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
