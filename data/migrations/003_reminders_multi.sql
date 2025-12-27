-- Rework reminders to support multi-plant associations and global settings.

CREATE TABLE IF NOT EXISTS reminder_settings (
    id INTEGER PRIMARY KEY CHECK (id = 1),
    day_before_enabled INTEGER NOT NULL DEFAULT 1,
    day_before_time TEXT NOT NULL DEFAULT '18:00',
    day_of_enabled INTEGER NOT NULL DEFAULT 1,
    day_of_time TEXT NOT NULL DEFAULT '08:00',
    overdue_enabled INTEGER NOT NULL DEFAULT 1,
    overdue_cadence_days INTEGER NOT NULL DEFAULT 2,
    overdue_time TEXT NOT NULL DEFAULT '09:00',
    quiet_hours_start TEXT,
    quiet_hours_end TEXT
);

INSERT OR IGNORE INTO reminder_settings (id) VALUES (1);

ALTER TABLE reminders RENAME TO reminders_old;

CREATE TABLE IF NOT EXISTS reminders (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    task_type TEXT NOT NULL,
    schedule_type TEXT NOT NULL,
    interval_days INTEGER,
    start_date TEXT,
    next_due_date TEXT NOT NULL,
    notes TEXT,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS reminder_plants (
    reminder_id INTEGER NOT NULL,
    plant_id INTEGER NOT NULL,
    PRIMARY KEY (reminder_id, plant_id),
    FOREIGN KEY (reminder_id) REFERENCES reminders(id) ON DELETE CASCADE,
    FOREIGN KEY (plant_id) REFERENCES plants(id) ON DELETE CASCADE
);

-- Migrate existing reminders into the new structure.
INSERT INTO reminders (id, task_type, schedule_type, interval_days, start_date, next_due_date, notes, created_at, updated_at)
SELECT id,
       reminder_type,
       'fixed',
       NULL,
       NULL,
       due_date,
       notes,
       created_at,
       created_at
FROM reminders_old;

INSERT INTO reminder_plants (reminder_id, plant_id)
SELECT id, plant_id FROM reminders_old;

DROP TABLE reminders_old;
