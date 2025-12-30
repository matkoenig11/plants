#!/usr/bin/env python3
"""
Check that migrations (001-005) produce at least one plant row when run on a fresh SQLite DB.
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
    migrations = [
        repo / "data" / "migrations" / "001_init.sql",
        repo / "data" / "migrations" / "002_add_plant_fields.sql",
        repo / "data" / "migrations" / "003_reminders_multi.sql",
        repo / "data" / "migrations" / "004_add_reminder_name.sql",
        repo / "data" / "migrations" / "005_seed_plants.sql",
    ]

    db_path = repo / "build" / "seed_check.sqlite"
    if db_path.exists():
        db_path.unlink()

    conn = sqlite3.connect(db_path)
    cur = conn.cursor()

    for m in migrations:
        apply_sql(cur, m)
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
