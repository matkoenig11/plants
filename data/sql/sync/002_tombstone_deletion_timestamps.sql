DROP TRIGGER IF EXISTS plants_sync_tombstone_after_delete;
CREATE TRIGGER IF NOT EXISTS plants_sync_tombstone_after_delete
AFTER DELETE ON plants
FOR EACH ROW
WHEN OLD.sync_uuid IS NOT NULL AND trim(OLD.sync_uuid) <> ''
BEGIN
    INSERT OR REPLACE INTO sync_tombstones (table_name, sync_uuid, deleted_at)
    VALUES ('plants', OLD.sync_uuid, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'));
END;

DROP TRIGGER IF EXISTS journal_entries_sync_tombstone_after_delete;
CREATE TRIGGER IF NOT EXISTS journal_entries_sync_tombstone_after_delete
AFTER DELETE ON journal_entries
FOR EACH ROW
WHEN OLD.sync_uuid IS NOT NULL AND trim(OLD.sync_uuid) <> ''
BEGIN
    INSERT OR REPLACE INTO sync_tombstones (table_name, sync_uuid, deleted_at)
    VALUES ('journal_entries', OLD.sync_uuid, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'));
END;

DROP TRIGGER IF EXISTS reminders_sync_tombstone_after_delete;
CREATE TRIGGER IF NOT EXISTS reminders_sync_tombstone_after_delete
AFTER DELETE ON reminders
FOR EACH ROW
WHEN OLD.sync_uuid IS NOT NULL AND trim(OLD.sync_uuid) <> ''
BEGIN
    INSERT OR REPLACE INTO sync_tombstones (table_name, sync_uuid, deleted_at)
    VALUES ('reminders', OLD.sync_uuid, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'));
END;

DROP TRIGGER IF EXISTS plant_images_sync_tombstone_after_delete;
CREATE TRIGGER IF NOT EXISTS plant_images_sync_tombstone_after_delete
AFTER DELETE ON plant_images
FOR EACH ROW
WHEN OLD.sync_uuid IS NOT NULL AND trim(OLD.sync_uuid) <> ''
BEGIN
    INSERT OR REPLACE INTO sync_tombstones (table_name, sync_uuid, deleted_at)
    VALUES ('plant_images', OLD.sync_uuid, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'));
END;

DROP TRIGGER IF EXISTS plant_care_schedules_sync_tombstone_after_delete;
CREATE TRIGGER IF NOT EXISTS plant_care_schedules_sync_tombstone_after_delete
AFTER DELETE ON plant_care_schedules
FOR EACH ROW
WHEN OLD.sync_uuid IS NOT NULL AND trim(OLD.sync_uuid) <> ''
BEGIN
    INSERT OR REPLACE INTO sync_tombstones (table_name, sync_uuid, deleted_at)
    VALUES ('plant_care_schedules', OLD.sync_uuid, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'));
END;
