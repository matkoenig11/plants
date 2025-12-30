#!/usr/bin/env python3
"""
Read plant names from the SQLite database and print them.

Usage:
    python read_plant.py [optional_db_path]

If no path is provided, it uses PLANT_JOURNAL_DB_PATH, otherwise the default
AppData location on Windows: %LOCALAPPDATA%/PlantJournal/plant_journal.sqlite.
"""

import os
import sqlite3
import sys
from pathlib import Path


def resolve_db_path(argv):
    if len(argv) > 1:
        return Path(argv[1])
    if os.getenv("PLANT_JOURNAL_DB_PATH"):
        return Path(os.getenv("PLANT_JOURNAL_DB_PATH"))
    base = Path(os.getenv("LOCALAPPDATA", "")) / "PlantJournal"
    return base / "plant_journal.sqlite"


def main(argv):
    db_path = resolve_db_path(argv)
    
    # copy file to local directory for testing
    copy_to_Path = Path.cwd() / "plant_journal.sqlite"
    if db_path != copy_to_Path:
        try:
            with open(db_path, "rb") as src_file:
                with open(copy_to_Path, "wb") as dst_file:
                    dst_file.write(src_file.read())
            db_path = copy_to_Path
        except Exception as exc:
            print(f"Failed to copy DB file: {exc}", file=sys.stderr)
            return 1

    if not db_path.exists():
        print(f"DB not found: {db_path}", file=sys.stderr)
        return 1

    conn = sqlite3.connect(db_path)
    cur = conn.cursor()
    try:
        cur.execute("SELECT name FROM plants ORDER BY name COLLATE NOCASE;")
    except Exception as exc:
        print(f"Query failed: {exc}", file=sys.stderr)
        return 1

    rows = cur.fetchall()
    if not rows:
        print("No plants found.")
    else:
        for (name,) in rows:
            print(name)

    conn.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
