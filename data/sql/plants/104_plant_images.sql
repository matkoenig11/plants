CREATE TABLE IF NOT EXISTS plant_images (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    plant_id INTEGER NOT NULL,
    file_path TEXT NOT NULL,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    FOREIGN KEY (plant_id) REFERENCES plants(id) ON DELETE CASCADE,
    UNIQUE (plant_id, file_path)
);

CREATE INDEX IF NOT EXISTS idx_plant_images_plant_id
ON plant_images(plant_id, created_at, id);
