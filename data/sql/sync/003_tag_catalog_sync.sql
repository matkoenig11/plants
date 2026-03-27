ALTER TABLE plant_tags_catalog ADD COLUMN sync_uuid TEXT;
ALTER TABLE plant_tags_catalog ADD COLUMN updated_at TEXT;

UPDATE plant_tags_catalog
SET sync_uuid = lower(trim(name))
WHERE sync_uuid IS NULL OR trim(sync_uuid) = '';

UPDATE plant_tags_catalog
SET updated_at = COALESCE(updated_at, created_at, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'))
WHERE updated_at IS NULL OR trim(updated_at) = '';

CREATE UNIQUE INDEX IF NOT EXISTS idx_plant_tags_catalog_sync_uuid
ON plant_tags_catalog(sync_uuid);

CREATE TRIGGER IF NOT EXISTS plant_tags_catalog_sync_defaults_after_insert
AFTER INSERT ON plant_tags_catalog
FOR EACH ROW
WHEN NEW.sync_uuid IS NULL OR trim(NEW.sync_uuid) = '' OR NEW.updated_at IS NULL OR trim(NEW.updated_at) = ''
BEGIN
    UPDATE plant_tags_catalog
    SET sync_uuid = COALESCE(NULLIF(NEW.sync_uuid, ''), lower(trim(NEW.name))),
        updated_at = COALESCE(NULLIF(NEW.updated_at, ''), COALESCE(NULLIF(NEW.created_at, ''), strftime('%Y-%m-%dT%H:%M:%fZ', 'now')))
    WHERE name = NEW.name;
END;

CREATE TRIGGER IF NOT EXISTS plant_tags_catalog_sync_touch_after_update
AFTER UPDATE ON plant_tags_catalog
FOR EACH ROW
WHEN COALESCE(NEW.updated_at, '') = COALESCE(OLD.updated_at, '')
BEGIN
    UPDATE plant_tags_catalog
    SET updated_at = strftime('%Y-%m-%dT%H:%M:%fZ', 'now')
    WHERE name = NEW.name;
END;

CREATE TRIGGER IF NOT EXISTS plant_tags_catalog_sync_tombstone_after_delete
AFTER DELETE ON plant_tags_catalog
FOR EACH ROW
WHEN OLD.sync_uuid IS NOT NULL AND trim(OLD.sync_uuid) <> ''
BEGIN
    INSERT OR REPLACE INTO sync_tombstones (table_name, sync_uuid, deleted_at)
    VALUES ('plant_tags_catalog', OLD.sync_uuid, COALESCE(NULLIF(OLD.updated_at, ''), strftime('%Y-%m-%dT%H:%M:%fZ', 'now')));
END;
