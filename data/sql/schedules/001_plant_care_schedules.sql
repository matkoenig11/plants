CREATE TABLE IF NOT EXISTS plant_care_schedules (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    plant_id INTEGER NOT NULL,
    care_type TEXT NOT NULL,
    season_name TEXT NOT NULL,
    interval_days INTEGER,
    is_enabled INTEGER NOT NULL DEFAULT 1,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now')),
    FOREIGN KEY (plant_id) REFERENCES plants(id) ON DELETE CASCADE,
    UNIQUE (plant_id, care_type, season_name)
);

CREATE INDEX IF NOT EXISTS idx_plant_care_schedules_lookup
ON plant_care_schedules(plant_id, care_type, season_name);
