ALTER TABLE plants ADD COLUMN sync_uuid TEXT;

UPDATE plants
SET sync_uuid = lower(hex(randomblob(16)))
WHERE sync_uuid IS NULL OR trim(sync_uuid) = '';

CREATE UNIQUE INDEX IF NOT EXISTS idx_plants_sync_uuid
ON plants(sync_uuid);

ALTER TABLE journal_entries ADD COLUMN sync_uuid TEXT;
ALTER TABLE journal_entries ADD COLUMN updated_at TEXT;

UPDATE journal_entries
SET sync_uuid = lower(hex(randomblob(16)))
WHERE sync_uuid IS NULL OR trim(sync_uuid) = '';

UPDATE journal_entries
SET updated_at = COALESCE(updated_at, created_at, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'))
WHERE updated_at IS NULL OR trim(updated_at) = '';

CREATE UNIQUE INDEX IF NOT EXISTS idx_journal_entries_sync_uuid
ON journal_entries(sync_uuid);
    
ALTER TABLE reminders ADD COLUMN sync_uuid TEXT;

UPDATE reminders
SET sync_uuid = lower(hex(randomblob(16)))
WHERE sync_uuid IS NULL OR trim(sync_uuid) = '';

CREATE UNIQUE INDEX IF NOT EXISTS idx_reminders_sync_uuid
ON reminders(sync_uuid);

ALTER TABLE reminder_settings ADD COLUMN updated_at TEXT;

UPDATE reminder_settings
SET updated_at = COALESCE(updated_at, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'))
WHERE updated_at IS NULL OR trim(updated_at) = '';

ALTER TABLE plant_images ADD COLUMN sync_uuid TEXT;
ALTER TABLE plant_images ADD COLUMN updated_at TEXT;

UPDATE plant_images
SET sync_uuid = lower(hex(randomblob(16)))
WHERE sync_uuid IS NULL OR trim(sync_uuid) = '';

UPDATE plant_images
SET updated_at = COALESCE(updated_at, created_at, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'))
WHERE updated_at IS NULL OR trim(updated_at) = '';

CREATE UNIQUE INDEX IF NOT EXISTS idx_plant_images_sync_uuid
ON plant_images(sync_uuid);

ALTER TABLE plant_care_schedules ADD COLUMN sync_uuid TEXT;

UPDATE plant_care_schedules
SET sync_uuid = lower(hex(randomblob(16)))
WHERE sync_uuid IS NULL OR trim(sync_uuid) = '';

CREATE UNIQUE INDEX IF NOT EXISTS idx_plant_care_schedules_sync_uuid
ON plant_care_schedules(sync_uuid);

CREATE TABLE IF NOT EXISTS sync_tombstones (
    table_name TEXT NOT NULL,
    sync_uuid TEXT NOT NULL,
    deleted_at TEXT NOT NULL,
    PRIMARY KEY (table_name, sync_uuid)
);

CREATE TRIGGER IF NOT EXISTS plants_sync_defaults_after_insert
AFTER INSERT ON plants
FOR EACH ROW
WHEN NEW.sync_uuid IS NULL OR trim(NEW.sync_uuid) = '' OR NEW.updated_at IS NULL OR trim(NEW.updated_at) = ''
BEGIN
    UPDATE plants
    SET sync_uuid = COALESCE(NULLIF(NEW.sync_uuid, ''), lower(hex(randomblob(16)))),
        updated_at = COALESCE(NULLIF(NEW.updated_at, ''), COALESCE(NULLIF(NEW.created_at, ''), strftime('%Y-%m-%dT%H:%M:%fZ', 'now')))
    WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS plants_sync_touch_after_update
AFTER UPDATE ON plants
FOR EACH ROW
WHEN COALESCE(NEW.updated_at, '') = COALESCE(OLD.updated_at, '')
BEGIN
    UPDATE plants
    SET updated_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now')
    WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS plants_sync_tombstone_after_delete
AFTER DELETE ON plants
FOR EACH ROW
WHEN OLD.sync_uuid IS NOT NULL AND trim(OLD.sync_uuid) <> ''
BEGIN
    INSERT OR REPLACE INTO sync_tombstones (table_name, sync_uuid, deleted_at)
    VALUES ('plants', OLD.sync_uuid, COALESCE(NULLIF(OLD.updated_at, ''), strftime('%Y-%m-%dT%H:%M:%fZ', 'now')));
END;

CREATE TRIGGER IF NOT EXISTS journal_entries_sync_defaults_after_insert
AFTER INSERT ON journal_entries
FOR EACH ROW
WHEN NEW.sync_uuid IS NULL OR trim(NEW.sync_uuid) = '' OR NEW.updated_at IS NULL OR trim(NEW.updated_at) = ''
BEGIN
    UPDATE journal_entries
    SET sync_uuid = COALESCE(NULLIF(NEW.sync_uuid, ''), lower(hex(randomblob(16)))),
        updated_at = COALESCE(NULLIF(NEW.updated_at, ''), COALESCE(NULLIF(NEW.created_at, ''), strftime('%Y-%m-%dT%H:%M:%fZ', 'now')))
    WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS journal_entries_sync_touch_after_update
AFTER UPDATE ON journal_entries
FOR EACH ROW
WHEN COALESCE(NEW.updated_at, '') = COALESCE(OLD.updated_at, '')
BEGIN
    UPDATE journal_entries
    SET updated_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now')
    WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS journal_entries_sync_tombstone_after_delete
AFTER DELETE ON journal_entries
FOR EACH ROW
WHEN OLD.sync_uuid IS NOT NULL AND trim(OLD.sync_uuid) <> ''
BEGIN
    INSERT OR REPLACE INTO sync_tombstones (table_name, sync_uuid, deleted_at)
    VALUES ('journal_entries', OLD.sync_uuid, COALESCE(NULLIF(OLD.updated_at, ''), strftime('%Y-%m-%dT%H:%M:%fZ', 'now')));
END;

CREATE TRIGGER IF NOT EXISTS reminders_sync_defaults_after_insert
AFTER INSERT ON reminders
FOR EACH ROW
WHEN NEW.sync_uuid IS NULL OR trim(NEW.sync_uuid) = '' OR NEW.updated_at IS NULL OR trim(NEW.updated_at) = ''
BEGIN
    UPDATE reminders
    SET sync_uuid = COALESCE(NULLIF(NEW.sync_uuid, ''), lower(hex(randomblob(16)))),
        updated_at = COALESCE(NULLIF(NEW.updated_at, ''), COALESCE(NULLIF(NEW.created_at, ''), strftime('%Y-%m-%dT%H:%M:%fZ', 'now')))
    WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS reminders_sync_touch_after_update
AFTER UPDATE ON reminders
FOR EACH ROW
WHEN COALESCE(NEW.updated_at, '') = COALESCE(OLD.updated_at, '')
BEGIN
    UPDATE reminders
    SET updated_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now')
    WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS reminders_sync_tombstone_after_delete
AFTER DELETE ON reminders
FOR EACH ROW
WHEN OLD.sync_uuid IS NOT NULL AND trim(OLD.sync_uuid) <> ''
BEGIN
    INSERT OR REPLACE INTO sync_tombstones (table_name, sync_uuid, deleted_at)
    VALUES ('reminders', OLD.sync_uuid, COALESCE(NULLIF(OLD.updated_at, ''), strftime('%Y-%m-%dT%H:%M:%fZ', 'now')));
END;

CREATE TRIGGER IF NOT EXISTS reminder_settings_sync_defaults_after_insert
AFTER INSERT ON reminder_settings
FOR EACH ROW
WHEN NEW.updated_at IS NULL OR trim(NEW.updated_at) = ''
BEGIN
    UPDATE reminder_settings
    SET updated_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now')
    WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS reminder_settings_sync_touch_after_update
AFTER UPDATE ON reminder_settings
FOR EACH ROW
WHEN COALESCE(NEW.updated_at, '') = COALESCE(OLD.updated_at, '')
BEGIN
    UPDATE reminder_settings
    SET updated_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now')
    WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS plant_images_sync_defaults_after_insert
AFTER INSERT ON plant_images
FOR EACH ROW
WHEN NEW.sync_uuid IS NULL OR trim(NEW.sync_uuid) = '' OR NEW.updated_at IS NULL OR trim(NEW.updated_at) = ''
BEGIN
    UPDATE plant_images
    SET sync_uuid = COALESCE(NULLIF(NEW.sync_uuid, ''), lower(hex(randomblob(16)))),
        updated_at = COALESCE(NULLIF(NEW.updated_at, ''), COALESCE(NULLIF(NEW.created_at, ''), strftime('%Y-%m-%dT%H:%M:%fZ', 'now')))
    WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS plant_images_sync_touch_after_update
AFTER UPDATE ON plant_images
FOR EACH ROW
WHEN COALESCE(NEW.updated_at, '') = COALESCE(OLD.updated_at, '')
BEGIN
    UPDATE plant_images
    SET updated_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now')
    WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS plant_images_sync_tombstone_after_delete
AFTER DELETE ON plant_images
FOR EACH ROW
WHEN OLD.sync_uuid IS NOT NULL AND trim(OLD.sync_uuid) <> ''
BEGIN
    INSERT OR REPLACE INTO sync_tombstones (table_name, sync_uuid, deleted_at)
    VALUES ('plant_images', OLD.sync_uuid, COALESCE(NULLIF(OLD.updated_at, ''), strftime('%Y-%m-%dT%H:%M:%fZ', 'now')));
END;

CREATE TRIGGER IF NOT EXISTS plant_care_schedules_sync_defaults_after_insert
AFTER INSERT ON plant_care_schedules
FOR EACH ROW
WHEN NEW.sync_uuid IS NULL OR trim(NEW.sync_uuid) = '' OR NEW.updated_at IS NULL OR trim(NEW.updated_at) = ''
BEGIN
    UPDATE plant_care_schedules
    SET sync_uuid = COALESCE(NULLIF(NEW.sync_uuid, ''), lower(hex(randomblob(16)))),
        updated_at = COALESCE(NULLIF(NEW.updated_at, ''), COALESCE(NULLIF(NEW.created_at, ''), strftime('%Y-%m-%dT%H:%M:%fZ', 'now')))
    WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS plant_care_schedules_sync_touch_after_update
AFTER UPDATE ON plant_care_schedules
FOR EACH ROW
WHEN COALESCE(NEW.updated_at, '') = COALESCE(OLD.updated_at, '')
BEGIN
    UPDATE plant_care_schedules
    SET updated_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now')
    WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS plant_care_schedules_sync_tombstone_after_delete
AFTER DELETE ON plant_care_schedules
FOR EACH ROW
WHEN OLD.sync_uuid IS NOT NULL AND trim(OLD.sync_uuid) <> ''
BEGIN
    INSERT OR REPLACE INTO sync_tombstones (table_name, sync_uuid, deleted_at)
    VALUES ('plant_care_schedules', OLD.sync_uuid, COALESCE(NULLIF(OLD.updated_at, ''), strftime('%Y-%m-%dT%H:%M:%fZ', 'now')));
END;
