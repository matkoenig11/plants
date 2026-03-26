from __future__ import annotations

from pathlib import Path

import psycopg
from psycopg.rows import dict_row

from .config import Settings


def connect(settings: Settings) -> psycopg.Connection:
    return psycopg.connect(settings.database_url, row_factory=dict_row)


def _column_type(conn: psycopg.Connection, table_name: str, column_name: str) -> str | None:
    row = conn.execute(
        """
        SELECT data_type
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = %(table_name)s
          AND column_name = %(column_name)s;
        """,
        {"table_name": table_name, "column_name": column_name},
    ).fetchone()
    return row["data_type"] if row else None


def _promote_timestamp_column(conn: psycopg.Connection, table_name: str, column_name: str) -> None:
    if _column_type(conn, table_name, column_name) != "text":
        return
    conn.execute(
        f"""
        ALTER TABLE {table_name}
        ALTER COLUMN {column_name} TYPE TIMESTAMPTZ
        USING COALESCE(NULLIF({column_name}, ''), NOW()::text)::timestamptz;
        """
    )


def _promote_integer_bool_column(conn: psycopg.Connection, table_name: str, column_name: str) -> None:
    if _column_type(conn, table_name, column_name) != "integer":
        return
    conn.execute(f"ALTER TABLE {table_name} ALTER COLUMN {column_name} DROP DEFAULT;")
    conn.execute(
        f"""
        ALTER TABLE {table_name}
        ALTER COLUMN {column_name} TYPE BOOLEAN
        USING (COALESCE({column_name}, 0) <> 0);
        """
    )
    conn.execute(f"ALTER TABLE {table_name} ALTER COLUMN {column_name} SET DEFAULT TRUE;")


def _column_default(conn: psycopg.Connection, table_name: str, column_name: str) -> str | None:
    row = conn.execute(
        """
        SELECT column_default
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = %(table_name)s
          AND column_name = %(column_name)s;
        """,
        {"table_name": table_name, "column_name": column_name},
    ).fetchone()
    return row["column_default"] if row else None


def _ensure_id_sequence_default(conn: psycopg.Connection, table_name: str) -> None:
    if _column_type(conn, table_name, "id") != "integer":
        return
    if _column_default(conn, table_name, "id"):
        return

    sequence_name = f"{table_name}_id_seq"
    conn.execute(f"CREATE SEQUENCE IF NOT EXISTS {sequence_name};")
    conn.execute(f"ALTER SEQUENCE {sequence_name} OWNED BY {table_name}.id;")
    conn.execute(
        f"""
        SELECT setval(
            '{sequence_name}',
            COALESCE((SELECT MAX(id) FROM {table_name}), 0) + 1,
            false
        );
        """
    )
    conn.execute(
        f"""
        ALTER TABLE {table_name}
        ALTER COLUMN id SET DEFAULT nextval('{sequence_name}');
        """
    )


def _apply_compatibility_upgrades(conn: psycopg.Connection) -> None:
    for table_name in ("plants", "journal_entries", "reminders", "plant_images"):
        _promote_timestamp_column(conn, table_name, "created_at")
        _promote_timestamp_column(conn, table_name, "updated_at")

    _ensure_id_sequence_default(conn, "reminder_settings")

    for column_name in ("day_before_enabled", "day_of_enabled", "overdue_enabled"):
        _promote_integer_bool_column(conn, "reminder_settings", column_name)
    _promote_timestamp_column(conn, "reminder_settings", "updated_at")

    _promote_integer_bool_column(conn, "plant_care_schedules", "is_enabled")
    _promote_timestamp_column(conn, "plant_care_schedules", "created_at")
    _promote_timestamp_column(conn, "plant_care_schedules", "updated_at")
    _promote_timestamp_column(conn, "sync_tombstones", "deleted_at")


def apply_migrations(settings: Settings) -> None:
    sql_files = sorted(Path(settings.sql_dir).glob("*.sql"))
    with connect(settings) as conn:
        with conn.transaction():
            for sql_file in sql_files:
                conn.execute(sql_file.read_text(encoding="utf-8"))
            _apply_compatibility_upgrades(conn)
