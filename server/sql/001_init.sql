CREATE TABLE IF NOT EXISTS accounts (
    id BIGSERIAL PRIMARY KEY,
    slug TEXT UNIQUE NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

INSERT INTO accounts (slug)
VALUES ('default')
ON CONFLICT (slug) DO NOTHING;

CREATE TABLE IF NOT EXISTS device_tokens (
    id BIGSERIAL PRIMARY KEY,
    account_id BIGINT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    device_label TEXT NOT NULL,
    token_hash TEXT UNIQUE NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    last_used_at TIMESTAMPTZ
);

CREATE TABLE IF NOT EXISTS sync_cursors (
    id BIGSERIAL PRIMARY KEY,
    account_id BIGINT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    cursor_value TEXT UNIQUE NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS plants (
    id BIGSERIAL PRIMARY KEY,
    account_id BIGINT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    sync_uuid TEXT NOT NULL,
    name TEXT NOT NULL,
    species TEXT,
    location TEXT,
    scientific_name TEXT,
    plant_type TEXT,
    light_requirement TEXT,
    watering_frequency TEXT,
    watering_notes TEXT,
    humidity_preference TEXT,
    soil_type TEXT,
    last_watered TEXT,
    fertilizing_schedule TEXT,
    last_fertilized TEXT,
    pruning_time TEXT,
    pruning_notes TEXT,
    last_pruned TEXT,
    growth_rate TEXT,
    issues_pests TEXT,
    temperature_tolerance TEXT,
    toxic_to_pets TEXT,
    poisonous_to_humans TEXT,
    poisonous_to_pets TEXT,
    indoor TEXT,
    flowering_season TEXT,
    acquired_on TEXT,
    source TEXT,
    notes TEXT,
    created_at TIMESTAMPTZ NOT NULL,
    updated_at TIMESTAMPTZ NOT NULL,
    UNIQUE (account_id, sync_uuid)
);

ALTER TABLE plants ADD COLUMN IF NOT EXISTS account_id BIGINT;
UPDATE plants
SET account_id = (SELECT id FROM accounts WHERE slug = 'default')
WHERE account_id IS NULL;
ALTER TABLE plants DROP COLUMN IF EXISTS winter_location;
ALTER TABLE plants DROP COLUMN IF EXISTS summer_location;
ALTER TABLE plants DROP COLUMN IF EXISTS pot_size;
ALTER TABLE plants DROP COLUMN IF EXISTS current_health_status;
ALTER TABLE plants ADD COLUMN IF NOT EXISTS poisonous_to_humans TEXT;
ALTER TABLE plants ADD COLUMN IF NOT EXISTS poisonous_to_pets TEXT;
ALTER TABLE plants ADD COLUMN IF NOT EXISTS indoor TEXT;
ALTER TABLE plants ADD COLUMN IF NOT EXISTS flowering_season TEXT;

CREATE INDEX IF NOT EXISTS idx_plants_account_updated_at
ON plants(account_id, updated_at);

CREATE TABLE IF NOT EXISTS journal_entries (
    id BIGSERIAL PRIMARY KEY,
    account_id BIGINT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    sync_uuid TEXT NOT NULL,
    plant_id BIGINT NOT NULL REFERENCES plants(id) ON DELETE CASCADE,
    entry_type TEXT NOT NULL,
    entry_date TEXT NOT NULL,
    notes TEXT,
    created_at TIMESTAMPTZ NOT NULL,
    updated_at TIMESTAMPTZ NOT NULL,
    UNIQUE (account_id, sync_uuid)
);

ALTER TABLE journal_entries ADD COLUMN IF NOT EXISTS account_id BIGINT;
UPDATE journal_entries
SET account_id = (SELECT id FROM accounts WHERE slug = 'default')
WHERE account_id IS NULL;

CREATE INDEX IF NOT EXISTS idx_journal_entries_account_updated_at
ON journal_entries(account_id, updated_at);

CREATE TABLE IF NOT EXISTS reminders (
    id BIGSERIAL PRIMARY KEY,
    account_id BIGINT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    sync_uuid TEXT NOT NULL,
    custom_name TEXT,
    task_type TEXT NOT NULL,
    schedule_type TEXT NOT NULL,
    interval_days INTEGER,
    start_date TEXT,
    next_due_date TEXT NOT NULL,
    notes TEXT,
    created_at TIMESTAMPTZ NOT NULL,
    updated_at TIMESTAMPTZ NOT NULL,
    UNIQUE (account_id, sync_uuid)
);

ALTER TABLE reminders ADD COLUMN IF NOT EXISTS account_id BIGINT;
UPDATE reminders
SET account_id = (SELECT id FROM accounts WHERE slug = 'default')
WHERE account_id IS NULL;

CREATE INDEX IF NOT EXISTS idx_reminders_account_updated_at
ON reminders(account_id, updated_at);

CREATE TABLE IF NOT EXISTS reminder_plants (
    reminder_id BIGINT NOT NULL REFERENCES reminders(id) ON DELETE CASCADE,
    plant_id BIGINT NOT NULL REFERENCES plants(id) ON DELETE CASCADE,
    PRIMARY KEY (reminder_id, plant_id)
);

CREATE TABLE IF NOT EXISTS reminder_settings (
    id BIGSERIAL PRIMARY KEY,
    account_id BIGINT NOT NULL UNIQUE REFERENCES accounts(id) ON DELETE CASCADE,
    day_before_enabled BOOLEAN NOT NULL DEFAULT TRUE,
    day_before_time TEXT NOT NULL DEFAULT '18:00',
    day_of_enabled BOOLEAN NOT NULL DEFAULT TRUE,
    day_of_time TEXT NOT NULL DEFAULT '08:00',
    overdue_enabled BOOLEAN NOT NULL DEFAULT TRUE,
    overdue_cadence_days INTEGER NOT NULL DEFAULT 2,
    overdue_time TEXT NOT NULL DEFAULT '09:00',
    quiet_hours_start TEXT,
    quiet_hours_end TEXT,
    updated_at TIMESTAMPTZ NOT NULL
);

ALTER TABLE reminder_settings ADD COLUMN IF NOT EXISTS account_id BIGINT;
UPDATE reminder_settings
SET account_id = (SELECT id FROM accounts WHERE slug = 'default')
WHERE account_id IS NULL;

DO $$
BEGIN
    IF EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = 'reminder_settings'
          AND column_name = 'day_before_enabled'
          AND data_type = 'integer'
    ) THEN
        EXECUTE 'ALTER TABLE reminder_settings ALTER COLUMN day_before_enabled DROP DEFAULT';
        EXECUTE 'ALTER TABLE reminder_settings ALTER COLUMN day_before_enabled TYPE BOOLEAN USING (COALESCE(day_before_enabled, 0) <> 0)';
        EXECUTE 'ALTER TABLE reminder_settings ALTER COLUMN day_before_enabled SET DEFAULT TRUE';
    END IF;

    IF EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = 'reminder_settings'
          AND column_name = 'day_of_enabled'
          AND data_type = 'integer'
    ) THEN
        EXECUTE 'ALTER TABLE reminder_settings ALTER COLUMN day_of_enabled DROP DEFAULT';
        EXECUTE 'ALTER TABLE reminder_settings ALTER COLUMN day_of_enabled TYPE BOOLEAN USING (COALESCE(day_of_enabled, 0) <> 0)';
        EXECUTE 'ALTER TABLE reminder_settings ALTER COLUMN day_of_enabled SET DEFAULT TRUE';
    END IF;

    IF EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = 'reminder_settings'
          AND column_name = 'overdue_enabled'
          AND data_type = 'integer'
    ) THEN
        EXECUTE 'ALTER TABLE reminder_settings ALTER COLUMN overdue_enabled DROP DEFAULT';
        EXECUTE 'ALTER TABLE reminder_settings ALTER COLUMN overdue_enabled TYPE BOOLEAN USING (COALESCE(overdue_enabled, 0) <> 0)';
        EXECUTE 'ALTER TABLE reminder_settings ALTER COLUMN overdue_enabled SET DEFAULT TRUE';
    END IF;

    IF EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = 'reminder_settings'
          AND column_name = 'updated_at'
          AND data_type = 'text'
    ) THEN
        EXECUTE 'ALTER TABLE reminder_settings ALTER COLUMN updated_at TYPE TIMESTAMPTZ USING COALESCE(NULLIF(updated_at, ''''), NOW()::text)::timestamptz';
    END IF;
END $$;

CREATE UNIQUE INDEX IF NOT EXISTS idx_reminder_settings_account_id
ON reminder_settings(account_id);

INSERT INTO reminder_settings (account_id, updated_at)
SELECT id, NOW()
FROM accounts
WHERE NOT EXISTS (
    SELECT 1 FROM reminder_settings rs WHERE rs.account_id = accounts.id
);

CREATE TABLE IF NOT EXISTS plant_images (
    id BIGSERIAL PRIMARY KEY,
    account_id BIGINT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    sync_uuid TEXT NOT NULL,
    plant_id BIGINT NOT NULL REFERENCES plants(id) ON DELETE CASCADE,
    file_path TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL,
    updated_at TIMESTAMPTZ NOT NULL,
    UNIQUE (account_id, sync_uuid),
    UNIQUE (account_id, plant_id, file_path)
);

ALTER TABLE plant_images ADD COLUMN IF NOT EXISTS account_id BIGINT;
UPDATE plant_images
SET account_id = (SELECT id FROM accounts WHERE slug = 'default')
WHERE account_id IS NULL;

DO $$
BEGIN
    IF EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = 'plants'
          AND column_name = 'created_at'
          AND data_type = 'text'
    ) THEN
        EXECUTE 'ALTER TABLE plants ALTER COLUMN created_at TYPE TIMESTAMPTZ USING COALESCE(NULLIF(created_at, ''''), NOW()::text)::timestamptz';
    END IF;

    IF EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = 'plants'
          AND column_name = 'updated_at'
          AND data_type = 'text'
    ) THEN
        EXECUTE 'ALTER TABLE plants ALTER COLUMN updated_at TYPE TIMESTAMPTZ USING COALESCE(NULLIF(updated_at, ''''), NOW()::text)::timestamptz';
    END IF;

    IF EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = 'journal_entries'
          AND column_name = 'created_at'
          AND data_type = 'text'
    ) THEN
        EXECUTE 'ALTER TABLE journal_entries ALTER COLUMN created_at TYPE TIMESTAMPTZ USING COALESCE(NULLIF(created_at, ''''), NOW()::text)::timestamptz';
    END IF;

    IF EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = 'journal_entries'
          AND column_name = 'updated_at'
          AND data_type = 'text'
    ) THEN
        EXECUTE 'ALTER TABLE journal_entries ALTER COLUMN updated_at TYPE TIMESTAMPTZ USING COALESCE(NULLIF(updated_at, ''''), NOW()::text)::timestamptz';
    END IF;

    IF EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = 'reminders'
          AND column_name = 'created_at'
          AND data_type = 'text'
    ) THEN
        EXECUTE 'ALTER TABLE reminders ALTER COLUMN created_at TYPE TIMESTAMPTZ USING COALESCE(NULLIF(created_at, ''''), NOW()::text)::timestamptz';
    END IF;

    IF EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = 'reminders'
          AND column_name = 'updated_at'
          AND data_type = 'text'
    ) THEN
        EXECUTE 'ALTER TABLE reminders ALTER COLUMN updated_at TYPE TIMESTAMPTZ USING COALESCE(NULLIF(updated_at, ''''), NOW()::text)::timestamptz';
    END IF;

    IF EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = 'plant_images'
          AND column_name = 'created_at'
          AND data_type = 'text'
    ) THEN
        EXECUTE 'ALTER TABLE plant_images ALTER COLUMN created_at TYPE TIMESTAMPTZ USING COALESCE(NULLIF(created_at, ''''), NOW()::text)::timestamptz';
    END IF;

    IF EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = 'plant_images'
          AND column_name = 'updated_at'
          AND data_type = 'text'
    ) THEN
        EXECUTE 'ALTER TABLE plant_images ALTER COLUMN updated_at TYPE TIMESTAMPTZ USING COALESCE(NULLIF(updated_at, ''''), NOW()::text)::timestamptz';
    END IF;

    IF EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = 'plant_care_schedules'
          AND column_name = 'is_enabled'
          AND data_type = 'integer'
    ) THEN
        EXECUTE 'ALTER TABLE plant_care_schedules ALTER COLUMN is_enabled DROP DEFAULT';
        EXECUTE 'ALTER TABLE plant_care_schedules ALTER COLUMN is_enabled TYPE BOOLEAN USING (COALESCE(is_enabled, 0) <> 0)';
        EXECUTE 'ALTER TABLE plant_care_schedules ALTER COLUMN is_enabled SET DEFAULT TRUE';
    END IF;

    IF EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = 'plant_care_schedules'
          AND column_name = 'created_at'
          AND data_type = 'text'
    ) THEN
        EXECUTE 'ALTER TABLE plant_care_schedules ALTER COLUMN created_at TYPE TIMESTAMPTZ USING COALESCE(NULLIF(created_at, ''''), NOW()::text)::timestamptz';
    END IF;

    IF EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = 'plant_care_schedules'
          AND column_name = 'updated_at'
          AND data_type = 'text'
    ) THEN
        EXECUTE 'ALTER TABLE plant_care_schedules ALTER COLUMN updated_at TYPE TIMESTAMPTZ USING COALESCE(NULLIF(updated_at, ''''), NOW()::text)::timestamptz';
    END IF;

    IF EXISTS (
        SELECT 1
        FROM information_schema.columns
        WHERE table_schema = 'public'
          AND table_name = 'sync_tombstones'
          AND column_name = 'deleted_at'
          AND data_type = 'text'
    ) THEN
        EXECUTE 'ALTER TABLE sync_tombstones ALTER COLUMN deleted_at TYPE TIMESTAMPTZ USING COALESCE(NULLIF(deleted_at, ''''), NOW()::text)::timestamptz';
    END IF;
END $$;

CREATE INDEX IF NOT EXISTS idx_plant_images_account_updated_at
ON plant_images(account_id, updated_at);

CREATE TABLE IF NOT EXISTS plant_care_schedules (
    id BIGSERIAL PRIMARY KEY,
    account_id BIGINT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    sync_uuid TEXT NOT NULL,
    plant_id BIGINT NOT NULL REFERENCES plants(id) ON DELETE CASCADE,
    care_type TEXT NOT NULL,
    season_name TEXT NOT NULL,
    interval_days INTEGER,
    is_enabled BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMPTZ NOT NULL,
    updated_at TIMESTAMPTZ NOT NULL,
    UNIQUE (account_id, sync_uuid),
    UNIQUE (account_id, plant_id, care_type, season_name)
);

ALTER TABLE plant_care_schedules ADD COLUMN IF NOT EXISTS account_id BIGINT;
UPDATE plant_care_schedules
SET account_id = (SELECT id FROM accounts WHERE slug = 'default')
WHERE account_id IS NULL;

CREATE INDEX IF NOT EXISTS idx_plant_care_schedules_account_updated_at
ON plant_care_schedules(account_id, updated_at);

CREATE TABLE IF NOT EXISTS sync_tombstones (
    account_id BIGINT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    entity_type TEXT NOT NULL,
    sync_uuid TEXT NOT NULL,
    deleted_at TIMESTAMPTZ NOT NULL,
    PRIMARY KEY (account_id, entity_type, sync_uuid)
);

ALTER TABLE sync_tombstones ADD COLUMN IF NOT EXISTS account_id BIGINT;
ALTER TABLE sync_tombstones ADD COLUMN IF NOT EXISTS entity_type TEXT;
UPDATE sync_tombstones
SET account_id = (SELECT id FROM accounts WHERE slug = 'default')
WHERE account_id IS NULL;
UPDATE sync_tombstones
SET entity_type = table_name
WHERE entity_type IS NULL AND table_name IS NOT NULL;

CREATE INDEX IF NOT EXISTS idx_sync_tombstones_account_deleted_at
ON sync_tombstones(account_id, entity_type, deleted_at);

CREATE UNIQUE INDEX IF NOT EXISTS idx_sync_tombstones_account_entity_sync_uuid
ON sync_tombstones(account_id, entity_type, sync_uuid);

CREATE UNIQUE INDEX IF NOT EXISTS idx_plants_account_sync_uuid
ON plants(account_id, sync_uuid);

CREATE UNIQUE INDEX IF NOT EXISTS idx_journal_entries_account_sync_uuid
ON journal_entries(account_id, sync_uuid);

CREATE UNIQUE INDEX IF NOT EXISTS idx_reminders_account_sync_uuid
ON reminders(account_id, sync_uuid);

CREATE UNIQUE INDEX IF NOT EXISTS idx_plant_images_account_sync_uuid
ON plant_images(account_id, sync_uuid);

CREATE UNIQUE INDEX IF NOT EXISTS idx_plant_care_schedules_account_sync_uuid
ON plant_care_schedules(account_id, sync_uuid);
