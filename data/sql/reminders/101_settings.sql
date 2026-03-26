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
