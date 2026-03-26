#!/usr/bin/env python3
"""
Check that the SQL migration steps produce at least one plant row on a fresh SQLite DB.
Usage: python tests/check_seed.py <repo_root>
"""

import sqlite3
import sys
from pathlib import Path


def apply_sql(cur, sql_path: Path):
    sql = sql_path.read_text(encoding="utf-8")
    try:
        cur.executescript(sql)
    except Exception as exc:
        raise RuntimeError(f"Failed executing {sql_path.name}: {exc}") from exc


def main():
    if len(sys.argv) < 2:
        print("Usage: check_seed.py <repo_root>", file=sys.stderr)
        sys.exit(1)
    repo = Path(sys.argv[1])
    scripts = [
        repo / "data" / "sql" / "schema" / "001_plants.sql",
        repo / "data" / "sql" / "schema" / "002_journal_entries.sql",
        repo / "data" / "sql" / "schema" / "003_reminders.sql",
        repo / "data" / "sql" / "plants" / "101_identity_and_location.sql",
        repo / "data" / "sql" / "plants" / "102_care_profile.sql",
        repo / "data" / "sql" / "plants" / "103_health_and_source.sql",
        repo / "data" / "sql" / "plants" / "104_plant_images.sql",
        repo / "data" / "sql" / "plants" / "106_library_fields.sql",
        repo / "data" / "sql" / "schedules" / "001_plant_care_schedules.sql",
        repo / "data" / "sql" / "reminders" / "101_settings.sql",
        repo / "data" / "sql" / "reminders" / "102_multi_plant_rework.sql",
        repo / "data" / "sql" / "reminders" / "201_custom_name.sql",
        repo / "data" / "sql" / "seeds" / "001_plants.sql",
    ]

    db_dir = repo / "build"
    db_dir.mkdir(exist_ok=True)
    db_path = db_dir / "seed_check.sqlite"
    if db_path.exists():
        db_path.unlink()

    conn = sqlite3.connect(db_path)
    cur = conn.cursor()

    for script in scripts:
        apply_sql(cur, script)
    conn.commit()

    cur.execute("SELECT COUNT(*) FROM plants")
    count = cur.fetchone()[0]
    conn.close()

    if count <= 0:
        print("Seed check failed: 0 plants inserted", file=sys.stderr)
        sys.exit(1)

    print(f"Seed check: {count} plants inserted")
    sys.exit(0)


if __name__ == "__main__":
    main()
