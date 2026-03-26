from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime, timezone
import hashlib
import secrets
from typing import Any

import psycopg
from psycopg.rows import dict_row

from .config import Settings


PLANT_FIELDS = [
    "name",
    "species",
    "location",
    "scientific_name",
    "plant_type",
    "light_requirement",
    "watering_frequency",
    "watering_notes",
    "humidity_preference",
    "soil_type",
    "last_watered",
    "fertilizing_schedule",
    "last_fertilized",
    "pruning_time",
    "pruning_notes",
    "last_pruned",
    "growth_rate",
    "issues_pests",
    "temperature_tolerance",
    "toxic_to_pets",
    "poisonous_to_humans",
    "poisonous_to_pets",
    "indoor",
    "flowering_season",
    "acquired_on",
    "source",
    "notes",
]


@dataclass
class SyncValidationError(Exception):
    status_code: int
    detail: str


def utc_now() -> datetime:
    return datetime.now(timezone.utc)


def parse_timestamp(value: Any) -> datetime | None:
    if value is None:
        return None
    if isinstance(value, datetime):
        if value.tzinfo is None:
            return value.replace(tzinfo=timezone.utc)
        return value.astimezone(timezone.utc)
    if not isinstance(value, str):
        return None
    normalized = value.strip().replace("Z", "+00:00")
    return datetime.fromisoformat(normalized).astimezone(timezone.utc)


def to_iso(value: Any) -> str | None:
    if value is None:
        return None
    if isinstance(value, str):
        return value
    return value.astimezone(timezone.utc).isoformat().replace("+00:00", "Z")


def hash_token(token: str) -> str:
    return hashlib.sha256(token.encode("utf-8")).hexdigest()


class PostgresSyncService:
    def __init__(self, settings: Settings) -> None:
        self._settings = settings

    def _connect(self) -> psycopg.Connection:
        return psycopg.connect(self._settings.database_url, row_factory=dict_row)

    def _account_id(self, conn: psycopg.Connection) -> int:
        row = conn.execute(
            "SELECT id FROM accounts WHERE slug = %(slug)s;",
            {"slug": self._settings.default_account_slug},
        ).fetchone()
        if not row:
            raise SyncValidationError(500, "Default account is missing.")
        return int(row["id"])

    def create_device_token(self, device_label: str) -> dict[str, Any]:
        raw_token = secrets.token_urlsafe(32)
        created_at = utc_now()
        with self._connect() as conn:
            with conn.transaction():
                account_id = self._account_id(conn)
                conn.execute(
                    """
                    INSERT INTO device_tokens (account_id, device_label, token_hash, created_at)
                    VALUES (%(account_id)s, %(device_label)s, %(token_hash)s, %(created_at)s);
                    """,
                    {
                        "account_id": account_id,
                        "device_label": device_label.strip(),
                        "token_hash": hash_token(raw_token),
                        "created_at": created_at,
                    },
                )
        return {
            "token": raw_token,
            "device_label": device_label.strip(),
            "created_at": to_iso(created_at),
        }

    def authenticate(self, token: str) -> int | None:
        token = token.strip()
        if not token:
            return None
        with self._connect() as conn:
            with conn.transaction():
                row = conn.execute(
                    """
                    SELECT id, account_id
                    FROM device_tokens
                    WHERE token_hash = %(token_hash)s;
                    """,
                    {"token_hash": hash_token(token)},
                ).fetchone()
                if not row:
                    return None
                conn.execute(
                    "UPDATE device_tokens SET last_used_at = NOW() WHERE id = %(id)s;",
                    {"id": row["id"]},
                )
                return int(row["account_id"])

    def sync(self, account_id: int, payload: dict[str, Any]) -> dict[str, Any]:
        changes = payload.get("changes") or {}
        last_sync_cursor = parse_timestamp(payload.get("last_sync_cursor"))
        applied = {
            "plants": 0,
            "journal_entries": 0,
            "reminders": 0,
            "plant_images": 0,
            "plant_care_schedules": 0,
            "reminder_settings": 0,
            "tombstones": 0,
        }
        conflicts: list[dict[str, Any]] = []

        with self._connect() as conn:
            with conn.transaction():
                for plant in changes.get("plants", []):
                    if self._apply_plant(conn, account_id, plant, conflicts):
                        applied["plants"] += 1

                for entry in changes.get("journal_entries", []):
                    if self._apply_journal_entry(conn, account_id, entry, conflicts):
                        applied["journal_entries"] += 1

                for reminder in changes.get("reminders", []):
                    if self._apply_reminder(conn, account_id, reminder, conflicts):
                        applied["reminders"] += 1

                for image in changes.get("plant_images", []):
                    if self._apply_plant_image(conn, account_id, image, conflicts):
                        applied["plant_images"] += 1

                for schedule in changes.get("plant_care_schedules", []):
                    if self._apply_care_schedule(conn, account_id, schedule, conflicts):
                        applied["plant_care_schedules"] += 1

                reminder_settings = changes.get("reminder_settings")
                if isinstance(reminder_settings, dict) and reminder_settings:
                    if self._apply_reminder_settings(conn, account_id, reminder_settings, conflicts):
                        applied["reminder_settings"] += 1

                for tombstone in changes.get("tombstones", []):
                    if self._apply_tombstone(conn, account_id, tombstone, conflicts):
                        applied["tombstones"] += 1

                new_cursor = to_iso(utc_now())
                conn.execute(
                    """
                    INSERT INTO sync_cursors (account_id, cursor_value, created_at)
                    VALUES (%(account_id)s, %(cursor_value)s, %(created_at)s);
                    """,
                    {
                        "account_id": account_id,
                        "cursor_value": new_cursor,
                        "created_at": parse_timestamp(new_cursor),
                    },
                )

                server_changes = self._fetch_changes(conn, account_id, last_sync_cursor)

        return {
            "new_cursor": new_cursor,
            "applied": applied,
            "server_changes": server_changes,
            "conflicts": conflicts,
        }

    def _fetch_existing(self, conn: psycopg.Connection, table: str, account_id: int, sync_uuid: str) -> dict[str, Any] | None:
        return conn.execute(
            f"SELECT * FROM {table} WHERE account_id = %(account_id)s AND sync_uuid = %(sync_uuid)s;",
            {"account_id": account_id, "sync_uuid": sync_uuid},
        ).fetchone()

    def _fetch_tombstone(self, conn: psycopg.Connection, account_id: int, entity_type: str, sync_uuid: str) -> dict[str, Any] | None:
        return conn.execute(
            """
            SELECT *
            FROM sync_tombstones
            WHERE sync_uuid = %(sync_uuid)s
              AND (
                  (account_id = %(account_id)s AND entity_type = %(entity_type)s)
                  OR table_name = %(entity_type)s
              );
            """,
            {"account_id": account_id, "entity_type": entity_type, "sync_uuid": sync_uuid},
        ).fetchone()

    def _record_conflict(self, conflicts: list[dict[str, Any]], entity_type: str, sync_uuid: str, reason: str) -> None:
        conflicts.append({"entity_type": entity_type, "sync_uuid": sync_uuid, "reason": reason})

    def _server_is_newer(self, server_timestamp: Any, client_value: str | None) -> bool:
        server_timestamp = parse_timestamp(server_timestamp)
        client_timestamp = parse_timestamp(client_value)
        if server_timestamp is None:
            return False
        if client_timestamp is None:
            return True
        return server_timestamp >= client_timestamp

    def _delete_stale_tombstone(self, conn: psycopg.Connection, account_id: int, entity_type: str, sync_uuid: str, updated_at: datetime) -> None:
        conn.execute(
            """
            DELETE FROM sync_tombstones
            WHERE sync_uuid = %(sync_uuid)s
              AND (
                  (account_id = %(account_id)s AND entity_type = %(entity_type)s)
                  OR table_name = %(entity_type)s
              );
            """,
            {
                "account_id": account_id,
                "entity_type": entity_type,
                "sync_uuid": sync_uuid,
            },
        )

    def _upsert_tombstone(self, conn: psycopg.Connection, account_id: int, entity_type: str, sync_uuid: str, deleted_at: datetime) -> None:
        params = {
            "account_id": account_id,
            "entity_type": entity_type,
            "sync_uuid": sync_uuid,
            "deleted_at": deleted_at,
        }

        updated = conn.execute(
            """
            UPDATE sync_tombstones
            SET account_id = %(account_id)s,
                entity_type = %(entity_type)s,
                table_name = %(entity_type)s,
                deleted_at = %(deleted_at)s
            WHERE sync_uuid = %(sync_uuid)s
              AND (
                  (account_id = %(account_id)s AND entity_type = %(entity_type)s)
                  OR table_name = %(entity_type)s
              )
              AND deleted_at < %(deleted_at)s
            RETURNING sync_uuid;
            """,
            params,
        ).fetchone()
        if updated:
            return

        existing = conn.execute(
            """
            SELECT sync_uuid
            FROM sync_tombstones
            WHERE sync_uuid = %(sync_uuid)s
              AND (
                  (account_id = %(account_id)s AND entity_type = %(entity_type)s)
                  OR table_name = %(entity_type)s
              );
            """,
            params,
        ).fetchone()
        if existing:
            return

        conn.execute(
            """
            INSERT INTO sync_tombstones (table_name, account_id, entity_type, sync_uuid, deleted_at)
            VALUES (%(entity_type)s, %(account_id)s, %(entity_type)s, %(sync_uuid)s, %(deleted_at)s);
            """,
            params,
        )

    def _prepare_upsert(self, conn: psycopg.Connection, account_id: int, entity_type: str, record: dict[str, Any], conflicts: list[dict[str, Any]]) -> tuple[dict[str, Any] | None, datetime | None]:
        sync_uuid = (record.get("sync_uuid") or "").strip()
        if not sync_uuid:
            raise SyncValidationError(400, f"{entity_type} record is missing sync_uuid.")

        updated_at = parse_timestamp(record.get("updated_at"))
        created_at = parse_timestamp(record.get("created_at"))
        if updated_at is None:
            raise SyncValidationError(400, f"{entity_type} record is missing updated_at.")
        if created_at is None:
            created_at = updated_at

        tombstone = self._fetch_tombstone(conn, account_id, entity_type, sync_uuid)
        if tombstone and self._server_is_newer(tombstone["deleted_at"], record.get("updated_at")):
            self._record_conflict(conflicts, entity_type, sync_uuid, "server_deleted_newer")
            return None, None

        return {"sync_uuid": sync_uuid, "updated_at": updated_at, "created_at": created_at}, updated_at

    def _apply_plant(self, conn: psycopg.Connection, account_id: int, record: dict[str, Any], conflicts: list[dict[str, Any]]) -> bool:
        prepared, updated_at = self._prepare_upsert(conn, account_id, "plants", record, conflicts)
        if prepared is None or updated_at is None:
            return False
        existing = self._fetch_existing(conn, "plants", account_id, prepared["sync_uuid"])
        if existing and self._server_is_newer(existing["updated_at"], record.get("updated_at")):
            self._record_conflict(conflicts, "plants", prepared["sync_uuid"], "server_newer")
            return False

        params = {
            "account_id": account_id,
            "sync_uuid": prepared["sync_uuid"],
            "created_at": prepared["created_at"],
            "updated_at": updated_at,
        }
        for field in PLANT_FIELDS:
            params[field] = record.get(field)

        conn.execute(
            """
            INSERT INTO plants (
                account_id, sync_uuid, name, species, location, scientific_name, plant_type,
                light_requirement, watering_frequency, watering_notes, humidity_preference,
                soil_type, last_watered, fertilizing_schedule, last_fertilized, pruning_time,
                pruning_notes, last_pruned, growth_rate, issues_pests,
                temperature_tolerance, toxic_to_pets, poisonous_to_humans, poisonous_to_pets, indoor,
                flowering_season, acquired_on, source, notes, created_at, updated_at
            ) VALUES (
                %(account_id)s, %(sync_uuid)s, %(name)s, %(species)s, %(location)s, %(scientific_name)s, %(plant_type)s,
                %(light_requirement)s, %(watering_frequency)s, %(watering_notes)s, %(humidity_preference)s,
                %(soil_type)s, %(last_watered)s, %(fertilizing_schedule)s, %(last_fertilized)s, %(pruning_time)s,
                %(pruning_notes)s, %(last_pruned)s, %(growth_rate)s, %(issues_pests)s,
                %(temperature_tolerance)s, %(toxic_to_pets)s, %(poisonous_to_humans)s, %(poisonous_to_pets)s, %(indoor)s,
                %(flowering_season)s, %(acquired_on)s, %(source)s, %(notes)s, %(created_at)s, %(updated_at)s
            )
            ON CONFLICT (account_id, sync_uuid) DO UPDATE SET
                name = EXCLUDED.name,
                species = EXCLUDED.species,
                location = EXCLUDED.location,
                scientific_name = EXCLUDED.scientific_name,
                plant_type = EXCLUDED.plant_type,
                light_requirement = EXCLUDED.light_requirement,
                watering_frequency = EXCLUDED.watering_frequency,
                watering_notes = EXCLUDED.watering_notes,
                humidity_preference = EXCLUDED.humidity_preference,
                soil_type = EXCLUDED.soil_type,
                last_watered = EXCLUDED.last_watered,
                fertilizing_schedule = EXCLUDED.fertilizing_schedule,
                last_fertilized = EXCLUDED.last_fertilized,
                pruning_time = EXCLUDED.pruning_time,
                pruning_notes = EXCLUDED.pruning_notes,
                last_pruned = EXCLUDED.last_pruned,
                growth_rate = EXCLUDED.growth_rate,
                issues_pests = EXCLUDED.issues_pests,
                temperature_tolerance = EXCLUDED.temperature_tolerance,
                toxic_to_pets = EXCLUDED.toxic_to_pets,
                poisonous_to_humans = EXCLUDED.poisonous_to_humans,
                poisonous_to_pets = EXCLUDED.poisonous_to_pets,
                indoor = EXCLUDED.indoor,
                flowering_season = EXCLUDED.flowering_season,
                acquired_on = EXCLUDED.acquired_on,
                source = EXCLUDED.source,
                notes = EXCLUDED.notes,
                created_at = EXCLUDED.created_at,
                updated_at = EXCLUDED.updated_at;
            """,
            params,
        )
        self._delete_stale_tombstone(conn, account_id, "plants", prepared["sync_uuid"], updated_at)
        return True

    def _plant_id(self, conn: psycopg.Connection, account_id: int, plant_sync_uuid: str) -> int:
        row = conn.execute(
            "SELECT id FROM plants WHERE account_id = %(account_id)s AND sync_uuid = %(sync_uuid)s;",
            {"account_id": account_id, "sync_uuid": plant_sync_uuid},
        ).fetchone()
        if not row:
            raise SyncValidationError(409, f"Plant {plant_sync_uuid} does not exist on the server.")
        return int(row["id"])

    def _reminder_id(self, conn: psycopg.Connection, account_id: int, reminder_sync_uuid: str) -> int:
        row = conn.execute(
            "SELECT id FROM reminders WHERE account_id = %(account_id)s AND sync_uuid = %(sync_uuid)s;",
            {"account_id": account_id, "sync_uuid": reminder_sync_uuid},
        ).fetchone()
        if not row:
            raise SyncValidationError(409, f"Reminder {reminder_sync_uuid} could not be resolved after upsert.")
        return int(row["id"])

    def _apply_journal_entry(self, conn: psycopg.Connection, account_id: int, record: dict[str, Any], conflicts: list[dict[str, Any]]) -> bool:
        prepared, updated_at = self._prepare_upsert(conn, account_id, "journal_entries", record, conflicts)
        if prepared is None or updated_at is None:
            return False
        existing = self._fetch_existing(conn, "journal_entries", account_id, prepared["sync_uuid"])
        if existing and self._server_is_newer(existing["updated_at"], record.get("updated_at")):
            self._record_conflict(conflicts, "journal_entries", prepared["sync_uuid"], "server_newer")
            return False

        conn.execute(
            """
            INSERT INTO journal_entries (account_id, sync_uuid, plant_id, entry_type, entry_date, notes, created_at, updated_at)
            VALUES (%(account_id)s, %(sync_uuid)s, %(plant_id)s, %(entry_type)s, %(entry_date)s, %(notes)s, %(created_at)s, %(updated_at)s)
            ON CONFLICT (account_id, sync_uuid) DO UPDATE SET
                plant_id = EXCLUDED.plant_id,
                entry_type = EXCLUDED.entry_type,
                entry_date = EXCLUDED.entry_date,
                notes = EXCLUDED.notes,
                created_at = EXCLUDED.created_at,
                updated_at = EXCLUDED.updated_at;
            """,
            {
                "account_id": account_id,
                "sync_uuid": prepared["sync_uuid"],
                "plant_id": self._plant_id(conn, account_id, record.get("plant_sync_uuid", "")),
                "entry_type": record.get("entry_type"),
                "entry_date": record.get("entry_date"),
                "notes": record.get("notes"),
                "created_at": prepared["created_at"],
                "updated_at": updated_at,
            },
        )
        self._delete_stale_tombstone(conn, account_id, "journal_entries", prepared["sync_uuid"], updated_at)
        return True

    def _apply_reminder(self, conn: psycopg.Connection, account_id: int, record: dict[str, Any], conflicts: list[dict[str, Any]]) -> bool:
        prepared, updated_at = self._prepare_upsert(conn, account_id, "reminders", record, conflicts)
        if prepared is None or updated_at is None:
            return False
        existing = self._fetch_existing(conn, "reminders", account_id, prepared["sync_uuid"])
        if existing and self._server_is_newer(existing["updated_at"], record.get("updated_at")):
            self._record_conflict(conflicts, "reminders", prepared["sync_uuid"], "server_newer")
            return False

        plant_sync_uuids = list(dict.fromkeys(record.get("plant_sync_uuids") or []))
        plant_ids = [self._plant_id(conn, account_id, plant_sync_uuid) for plant_sync_uuid in plant_sync_uuids]

        conn.execute(
            """
            INSERT INTO reminders (
                account_id, sync_uuid, custom_name, task_type, schedule_type, interval_days,
                start_date, next_due_date, notes, created_at, updated_at
            )
            VALUES (
                %(account_id)s, %(sync_uuid)s, %(custom_name)s, %(task_type)s, %(schedule_type)s, %(interval_days)s,
                %(start_date)s, %(next_due_date)s, %(notes)s, %(created_at)s, %(updated_at)s
            )
            ON CONFLICT (account_id, sync_uuid) DO UPDATE SET
                custom_name = EXCLUDED.custom_name,
                task_type = EXCLUDED.task_type,
                schedule_type = EXCLUDED.schedule_type,
                interval_days = EXCLUDED.interval_days,
                start_date = EXCLUDED.start_date,
                next_due_date = EXCLUDED.next_due_date,
                notes = EXCLUDED.notes,
                created_at = EXCLUDED.created_at,
                updated_at = EXCLUDED.updated_at;
            """,
            {
                "account_id": account_id,
                "sync_uuid": prepared["sync_uuid"],
                "custom_name": record.get("custom_name"),
                "task_type": record.get("task_type"),
                "schedule_type": record.get("schedule_type"),
                "interval_days": record.get("interval_days"),
                "start_date": record.get("start_date"),
                "next_due_date": record.get("next_due_date"),
                "notes": record.get("notes"),
                "created_at": prepared["created_at"],
                "updated_at": updated_at,
            },
        )

        reminder_id = self._reminder_id(conn, account_id, prepared["sync_uuid"])
        conn.execute("DELETE FROM reminder_plants WHERE reminder_id = %(reminder_id)s;", {"reminder_id": reminder_id})
        for plant_id in plant_ids:
            conn.execute(
                "INSERT INTO reminder_plants (reminder_id, plant_id) VALUES (%(reminder_id)s, %(plant_id)s);",
                {"reminder_id": reminder_id, "plant_id": plant_id},
            )

        self._delete_stale_tombstone(conn, account_id, "reminders", prepared["sync_uuid"], updated_at)
        return True

    def _apply_plant_image(self, conn: psycopg.Connection, account_id: int, record: dict[str, Any], conflicts: list[dict[str, Any]]) -> bool:
        prepared, updated_at = self._prepare_upsert(conn, account_id, "plant_images", record, conflicts)
        if prepared is None or updated_at is None:
            return False
        existing = self._fetch_existing(conn, "plant_images", account_id, prepared["sync_uuid"])
        if existing and self._server_is_newer(existing["updated_at"], record.get("updated_at")):
            self._record_conflict(conflicts, "plant_images", prepared["sync_uuid"], "server_newer")
            return False

        conn.execute(
            """
            INSERT INTO plant_images (account_id, sync_uuid, plant_id, file_path, created_at, updated_at)
            VALUES (%(account_id)s, %(sync_uuid)s, %(plant_id)s, %(file_path)s, %(created_at)s, %(updated_at)s)
            ON CONFLICT (account_id, sync_uuid) DO UPDATE SET
                plant_id = EXCLUDED.plant_id,
                file_path = EXCLUDED.file_path,
                created_at = EXCLUDED.created_at,
                updated_at = EXCLUDED.updated_at;
            """,
            {
                "account_id": account_id,
                "sync_uuid": prepared["sync_uuid"],
                "plant_id": self._plant_id(conn, account_id, record.get("plant_sync_uuid", "")),
                "file_path": record.get("file_path"),
                "created_at": prepared["created_at"],
                "updated_at": updated_at,
            },
        )
        self._delete_stale_tombstone(conn, account_id, "plant_images", prepared["sync_uuid"], updated_at)
        return True

    def _apply_care_schedule(self, conn: psycopg.Connection, account_id: int, record: dict[str, Any], conflicts: list[dict[str, Any]]) -> bool:
        prepared, updated_at = self._prepare_upsert(conn, account_id, "plant_care_schedules", record, conflicts)
        if prepared is None or updated_at is None:
            return False
        existing = self._fetch_existing(conn, "plant_care_schedules", account_id, prepared["sync_uuid"])
        if existing and self._server_is_newer(existing["updated_at"], record.get("updated_at")):
            self._record_conflict(conflicts, "plant_care_schedules", prepared["sync_uuid"], "server_newer")
            return False

        conn.execute(
            """
            INSERT INTO plant_care_schedules (
                account_id, sync_uuid, plant_id, care_type, season_name, interval_days, is_enabled, created_at, updated_at
            )
            VALUES (
                %(account_id)s, %(sync_uuid)s, %(plant_id)s, %(care_type)s, %(season_name)s, %(interval_days)s, %(is_enabled)s, %(created_at)s, %(updated_at)s
            )
            ON CONFLICT (account_id, sync_uuid) DO UPDATE SET
                plant_id = EXCLUDED.plant_id,
                care_type = EXCLUDED.care_type,
                season_name = EXCLUDED.season_name,
                interval_days = EXCLUDED.interval_days,
                is_enabled = EXCLUDED.is_enabled,
                created_at = EXCLUDED.created_at,
                updated_at = EXCLUDED.updated_at;
            """,
            {
                "account_id": account_id,
                "sync_uuid": prepared["sync_uuid"],
                "plant_id": self._plant_id(conn, account_id, record.get("plant_sync_uuid", "")),
                "care_type": record.get("care_type"),
                "season_name": record.get("season_name"),
                "interval_days": record.get("interval_days"),
                "is_enabled": bool(record.get("is_enabled", True)),
                "created_at": prepared["created_at"],
                "updated_at": updated_at,
            },
        )
        self._delete_stale_tombstone(conn, account_id, "plant_care_schedules", prepared["sync_uuid"], updated_at)
        return True

    def _apply_reminder_settings(self, conn: psycopg.Connection, account_id: int, record: dict[str, Any], conflicts: list[dict[str, Any]]) -> bool:
        updated_at = parse_timestamp(record.get("updated_at"))
        if updated_at is None:
            raise SyncValidationError(400, "reminder_settings is missing updated_at.")

        existing = conn.execute(
            "SELECT * FROM reminder_settings WHERE account_id = %(account_id)s;",
            {"account_id": account_id},
        ).fetchone()
        if existing and self._server_is_newer(existing["updated_at"], record.get("updated_at")):
            self._record_conflict(conflicts, "reminder_settings", "singleton", "server_newer")
            return False

        params = {
            "account_id": account_id,
            "day_before_enabled": bool(record.get("day_before_enabled", True)),
            "day_before_time": record.get("day_before_time") or "18:00",
            "day_of_enabled": bool(record.get("day_of_enabled", True)),
            "day_of_time": record.get("day_of_time") or "08:00",
            "overdue_enabled": bool(record.get("overdue_enabled", True)),
            "overdue_cadence_days": record.get("overdue_cadence_days") or 2,
            "overdue_time": record.get("overdue_time") or "09:00",
            "quiet_hours_start": record.get("quiet_hours_start"),
            "quiet_hours_end": record.get("quiet_hours_end"),
            "updated_at": updated_at,
        }

        if existing:
            conn.execute(
                """
                UPDATE reminder_settings
                SET day_before_enabled = %(day_before_enabled)s,
                    day_before_time = %(day_before_time)s,
                    day_of_enabled = %(day_of_enabled)s,
                    day_of_time = %(day_of_time)s,
                    overdue_enabled = %(overdue_enabled)s,
                    overdue_cadence_days = %(overdue_cadence_days)s,
                    overdue_time = %(overdue_time)s,
                    quiet_hours_start = %(quiet_hours_start)s,
                    quiet_hours_end = %(quiet_hours_end)s,
                    updated_at = %(updated_at)s
                WHERE account_id = %(account_id)s;
                """,
                params,
            )
        else:
            conn.execute(
                """
                INSERT INTO reminder_settings (
                    account_id, day_before_enabled, day_before_time, day_of_enabled, day_of_time,
                    overdue_enabled, overdue_cadence_days, overdue_time, quiet_hours_start, quiet_hours_end, updated_at
                )
                VALUES (
                    %(account_id)s, %(day_before_enabled)s, %(day_before_time)s, %(day_of_enabled)s, %(day_of_time)s,
                    %(overdue_enabled)s, %(overdue_cadence_days)s, %(overdue_time)s, %(quiet_hours_start)s, %(quiet_hours_end)s, %(updated_at)s
                );
                """,
                params,
            )
        return True

    def _apply_tombstone(self, conn: psycopg.Connection, account_id: int, record: dict[str, Any], conflicts: list[dict[str, Any]]) -> bool:
        entity_type = (record.get("entity_type") or "").strip()
        sync_uuid = (record.get("sync_uuid") or "").strip()
        deleted_at = parse_timestamp(record.get("deleted_at"))
        if not entity_type or not sync_uuid or deleted_at is None:
            raise SyncValidationError(400, "tombstone is missing required fields.")

        table = entity_type
        existing = self._fetch_existing(conn, table, account_id, sync_uuid)
        if existing and self._server_is_newer(existing["updated_at"], record.get("deleted_at")):
            self._record_conflict(conflicts, entity_type, sync_uuid, "server_newer")
            return False

        if existing:
            conn.execute(
                f"DELETE FROM {table} WHERE account_id = %(account_id)s AND sync_uuid = %(sync_uuid)s;",
                {"account_id": account_id, "sync_uuid": sync_uuid},
            )

        self._upsert_tombstone(conn, account_id, entity_type, sync_uuid, deleted_at)
        return True

    def _fetch_changes(self, conn: psycopg.Connection, account_id: int, since: datetime | None) -> dict[str, Any]:
        return {
            "plants": self._fetch_plants(conn, account_id, since),
            "journal_entries": self._fetch_journal_entries(conn, account_id, since),
            "reminders": self._fetch_reminders(conn, account_id, since),
            "plant_images": self._fetch_plant_images(conn, account_id, since),
            "plant_care_schedules": self._fetch_care_schedules(conn, account_id, since),
            "reminder_settings": self._fetch_reminder_settings(conn, account_id, since),
            "tombstones": self._fetch_tombstones(conn, account_id, since),
        }

    def _fetch_plants(self, conn: psycopg.Connection, account_id: int, since: datetime | None) -> list[dict[str, Any]]:
        rows = conn.execute(
            "SELECT * FROM plants WHERE account_id = %(account_id)s ORDER BY updated_at, sync_uuid",
            {"account_id": account_id},
        ).fetchall()
        result: list[dict[str, Any]] = []
        for row in rows:
            row_updated_at = parse_timestamp(row["updated_at"])
            if since and row_updated_at is not None and row_updated_at <= since:
                continue
            item = {"sync_uuid": row["sync_uuid"], "created_at": to_iso(row["created_at"]), "updated_at": to_iso(row["updated_at"])}
            for field in PLANT_FIELDS:
                item[field] = row[field]
            result.append(item)
        return result

    def _fetch_journal_entries(self, conn: psycopg.Connection, account_id: int, since: datetime | None) -> list[dict[str, Any]]:
        rows = conn.execute(
            """
            SELECT je.*, p.sync_uuid AS plant_sync_uuid
            FROM journal_entries je
            JOIN plants p ON p.id = je.plant_id
            WHERE je.account_id = %(account_id)s
            ORDER BY je.updated_at, je.sync_uuid
            """,
            {"account_id": account_id},
        ).fetchall()
        result: list[dict[str, Any]] = []
        for row in rows:
            row_updated_at = parse_timestamp(row["updated_at"])
            if since and row_updated_at is not None and row_updated_at <= since:
                continue
            result.append({
                "sync_uuid": row["sync_uuid"],
                "plant_sync_uuid": row["plant_sync_uuid"],
                "entry_type": row["entry_type"],
                "entry_date": row["entry_date"],
                "notes": row["notes"],
                "created_at": to_iso(row["created_at"]),
                "updated_at": to_iso(row["updated_at"]),
            })
        return result

    def _fetch_reminders(self, conn: psycopg.Connection, account_id: int, since: datetime | None) -> list[dict[str, Any]]:
        rows = conn.execute(
            """
            SELECT r.*, p.sync_uuid AS plant_sync_uuid
            FROM reminders r
            LEFT JOIN reminder_plants rp ON rp.reminder_id = r.id
            LEFT JOIN plants p ON p.id = rp.plant_id
            WHERE r.account_id = %(account_id)s
            ORDER BY r.updated_at, r.sync_uuid, p.sync_uuid
            """,
            {"account_id": account_id},
        ).fetchall()
        grouped: dict[str, dict[str, Any]] = {}
        for row in rows:
            row_updated_at = parse_timestamp(row["updated_at"])
            if since and row_updated_at is not None and row_updated_at <= since:
                continue
            item = grouped.setdefault(
                row["sync_uuid"],
                {
                    "sync_uuid": row["sync_uuid"],
                    "custom_name": row["custom_name"],
                    "task_type": row["task_type"],
                    "schedule_type": row["schedule_type"],
                    "interval_days": row["interval_days"],
                    "start_date": row["start_date"],
                    "next_due_date": row["next_due_date"],
                    "notes": row["notes"],
                    "created_at": to_iso(row["created_at"]),
                    "updated_at": to_iso(row["updated_at"]),
                    "plant_sync_uuids": [],
                },
            )
            if row["plant_sync_uuid"] and row["plant_sync_uuid"] not in item["plant_sync_uuids"]:
                item["plant_sync_uuids"].append(row["plant_sync_uuid"])
        return list(grouped.values())

    def _fetch_plant_images(self, conn: psycopg.Connection, account_id: int, since: datetime | None) -> list[dict[str, Any]]:
        rows = conn.execute(
            """
            SELECT pi.*, p.sync_uuid AS plant_sync_uuid
            FROM plant_images pi
            JOIN plants p ON p.id = pi.plant_id
            WHERE pi.account_id = %(account_id)s
            ORDER BY pi.updated_at, pi.sync_uuid
            """,
            {"account_id": account_id},
        ).fetchall()
        result: list[dict[str, Any]] = []
        for row in rows:
            row_updated_at = parse_timestamp(row["updated_at"])
            if since and row_updated_at is not None and row_updated_at <= since:
                continue
            result.append({
                "sync_uuid": row["sync_uuid"],
                "plant_sync_uuid": row["plant_sync_uuid"],
                "file_path": row["file_path"],
                "created_at": to_iso(row["created_at"]),
                "updated_at": to_iso(row["updated_at"]),
            })
        return result

    def _fetch_care_schedules(self, conn: psycopg.Connection, account_id: int, since: datetime | None) -> list[dict[str, Any]]:
        rows = conn.execute(
            """
            SELECT pcs.*, p.sync_uuid AS plant_sync_uuid
            FROM plant_care_schedules pcs
            JOIN plants p ON p.id = pcs.plant_id
            WHERE pcs.account_id = %(account_id)s
            ORDER BY pcs.updated_at, pcs.sync_uuid
            """,
            {"account_id": account_id},
        ).fetchall()
        result: list[dict[str, Any]] = []
        for row in rows:
            row_updated_at = parse_timestamp(row["updated_at"])
            if since and row_updated_at is not None and row_updated_at <= since:
                continue
            result.append({
                "sync_uuid": row["sync_uuid"],
                "plant_sync_uuid": row["plant_sync_uuid"],
                "care_type": row["care_type"],
                "season_name": row["season_name"],
                "interval_days": row["interval_days"],
                "is_enabled": row["is_enabled"],
                "created_at": to_iso(row["created_at"]),
                "updated_at": to_iso(row["updated_at"]),
            })
        return result

    def _fetch_reminder_settings(self, conn: psycopg.Connection, account_id: int, since: datetime | None) -> dict[str, Any] | None:
        row = conn.execute(
            "SELECT * FROM reminder_settings WHERE account_id = %(account_id)s;",
            {"account_id": account_id},
        ).fetchone()
        if not row:
            return None
        row_updated_at = parse_timestamp(row["updated_at"])
        if since and row_updated_at is not None and row_updated_at <= since:
            return None
        return {
            "day_before_enabled": row["day_before_enabled"],
            "day_before_time": row["day_before_time"],
            "day_of_enabled": row["day_of_enabled"],
            "day_of_time": row["day_of_time"],
            "overdue_enabled": row["overdue_enabled"],
            "overdue_cadence_days": row["overdue_cadence_days"],
            "overdue_time": row["overdue_time"],
            "quiet_hours_start": row["quiet_hours_start"],
            "quiet_hours_end": row["quiet_hours_end"],
            "updated_at": to_iso(row["updated_at"]),
        }

    def _fetch_tombstones(self, conn: psycopg.Connection, account_id: int, since: datetime | None) -> list[dict[str, Any]]:
        rows = conn.execute(
            """
            SELECT entity_type, sync_uuid, deleted_at
            FROM sync_tombstones
            WHERE account_id = %(account_id)s
            ORDER BY deleted_at, entity_type, sync_uuid
            """,
            {"account_id": account_id},
        ).fetchall()
        result: list[dict[str, Any]] = []
        for row in rows:
            row_deleted_at = parse_timestamp(row["deleted_at"])
            if since and row_deleted_at is not None and row_deleted_at <= since:
                continue
            result.append({
                "entity_type": row["entity_type"],
                "sync_uuid": row["sync_uuid"],
                "deleted_at": to_iso(row["deleted_at"]),
            })
        return result
