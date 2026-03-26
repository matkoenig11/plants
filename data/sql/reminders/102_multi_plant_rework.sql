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

INSERT INTO reminders (
    id,
    task_type,
    schedule_type,
    interval_days,
    start_date,
    next_due_date,
    notes,
    created_at,
    updated_at
)
SELECT
    id,
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
SELECT id, plant_id
FROM reminders_old;

DROP TABLE reminders_old;
