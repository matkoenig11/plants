CREATE TABLE IF NOT EXISTS plant_tags_catalog (
    name TEXT PRIMARY KEY,
    created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now'))
);
