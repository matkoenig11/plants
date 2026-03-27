#include "DatabaseSyncService.h"

#include <QDateTime>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVector>

namespace {
struct SyncCounters
{
    int pushed = 0;
    int pulled = 0;
    int deleted = 0;
};

struct PlantRecord
{
    QString syncUuid;
    QString name;
    QString species;
    QString location;
    QString scientificName;
    QString plantType;
    QString lightRequirement;
    QString wateringFrequency;
    QString wateringNotes;
    QString humidityPreference;
    QString soilType;
    QString lastWatered;
    QString fertilizingSchedule;
    QString lastFertilized;
    QString pruningTime;
    QString pruningNotes;
    QString lastPruned;
    QString growthRate;
    QString issuesPests;
    QString temperatureTolerance;
    QString toxicToPets;
    QString poisonousToHumans;
    QString poisonousToPets;
    QString indoor;
    QString floweringSeason;
    QString tags;
    QString acquiredOn;
    QString source;
    QString notes;
    QString createdAt;
    QString updatedAt;
};

struct JournalEntrySyncRecord
{
    QString syncUuid;
    QString plantSyncUuid;
    QString entryType;
    QString entryDate;
    QString notes;
    QString createdAt;
    QString updatedAt;
};

struct ReminderSyncRecord
{
    QString syncUuid;
    QString customName;
    QString taskType;
    QString scheduleType;
    int intervalDays = 0;
    QString startDate;
    QString nextDueDate;
    QString notes;
    QString createdAt;
    QString updatedAt;
    QStringList plantSyncUuids;
};

struct PlantImageSyncRecord
{
    QString syncUuid;
    QString plantSyncUuid;
    QString filePath;
    QString createdAt;
    QString updatedAt;
};

struct CareScheduleSyncRecord
{
    QString syncUuid;
    QString plantSyncUuid;
    QString careType;
    QString seasonName;
    int intervalDays = 0;
    bool enabled = true;
    QString createdAt;
    QString updatedAt;
};

struct TagCatalogSyncRecord
{
    QString syncUuid;
    QString name;
    QString createdAt;
    QString updatedAt;
};

struct ReminderSettingsSyncRecord
{
    bool exists = false;
    bool dayBeforeEnabled = true;
    QString dayBeforeTime = QStringLiteral("18:00");
    bool dayOfEnabled = true;
    QString dayOfTime = QStringLiteral("08:00");
    bool overdueEnabled = true;
    int overdueCadenceDays = 2;
    QString overdueTime = QStringLiteral("09:00");
    QString quietHoursStart;
    QString quietHoursEnd;
    QString updatedAt;
};

struct TombstoneRecord
{
    QString entityType;
    QString syncUuid;
    QString deletedAt;
};

struct SyncDataset
{
    QMap<QString, PlantRecord> plants;
    QMap<QString, JournalEntrySyncRecord> journalEntries;
    QMap<QString, ReminderSyncRecord> reminders;
    QMap<QString, PlantImageSyncRecord> plantImages;
    QMap<QString, CareScheduleSyncRecord> careSchedules;
    QMap<QString, TagCatalogSyncRecord> tagCatalog;
    ReminderSettingsSyncRecord reminderSettings;
    QVector<TombstoneRecord> tombstones;
};

QString currentTimestampUtc()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

QDateTime parseTimestamp(const QString &value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    const QList<QString> formats = {
        QStringLiteral("yyyy-MM-ddTHH:mm:ss.zzzZ"),
        QStringLiteral("yyyy-MM-ddTHH:mm:ssZ"),
        QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"),
        QStringLiteral("yyyy-MM-dd HH:mm:ss")
    };

    for (const QString &format : formats) {
        QDateTime parsed = QDateTime::fromString(trimmed, format);
        if (parsed.isValid()) {
            if (format.endsWith(QLatin1Char('Z'))) {
                parsed.setTimeSpec(Qt::UTC);
            }
            return parsed.toUTC();
        }
    }

    QDateTime parsed = QDateTime::fromString(trimmed, Qt::ISODateWithMs);
    if (!parsed.isValid()) {
        parsed = QDateTime::fromString(trimmed, Qt::ISODate);
    }
    return parsed.isValid() ? parsed.toUTC() : QDateTime();
}

int compareTimestamps(const QString &left, const QString &right)
{
    const QDateTime leftTime = parseTimestamp(left);
    const QDateTime rightTime = parseTimestamp(right);

    if (leftTime.isValid() && rightTime.isValid()) {
        if (leftTime < rightTime) {
            return -1;
        }
        if (leftTime > rightTime) {
            return 1;
        }
        return 0;
    }

    const int lexical = QString::compare(left, right, Qt::CaseSensitive);
    if (lexical < 0) {
        return -1;
    }
    if (lexical > 0) {
        return 1;
    }
    return 0;
}

QString newerTimestamp(const QString &left, const QString &right)
{
    return compareTimestamps(left, right) >= 0 ? left : right;
}

bool execStatement(QSqlDatabase &database, const QString &statement, QString *errorMessage)
{
    QSqlQuery query(database);
    if (query.exec(statement)) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = query.lastError().text();
    }
    return false;
}

bool upsertTombstone(QSqlDatabase &database,
                     const QString &tableName,
                     const QString &syncUuid,
                     const QString &deletedAt,
                     QString *errorMessage)
{
    if (syncUuid.trimmed().isEmpty()) {
        return true;
    }

    QSqlQuery query(database);
    query.prepare(
        "INSERT INTO sync_tombstones (table_name, sync_uuid, deleted_at) "
        "VALUES (:table_name, :sync_uuid, :deleted_at) "
        "ON CONFLICT(table_name, sync_uuid) DO UPDATE SET deleted_at = excluded.deleted_at "
        "WHERE sync_tombstones.deleted_at < excluded.deleted_at;");
    query.bindValue(QStringLiteral(":table_name"), tableName);
    query.bindValue(QStringLiteral(":sync_uuid"), syncUuid);
    query.bindValue(QStringLiteral(":deleted_at"), deletedAt.isEmpty() ? currentTimestampUtc() : deletedAt);
    if (query.exec()) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = query.lastError().text();
    }
    return false;
}

QMap<QString, QString> loadTombstones(QSqlDatabase &database,
                                      const QString &tableName,
                                      QString *errorMessage)
{
    QMap<QString, QString> result;

    QSqlQuery query(database);
    query.prepare(
        "SELECT sync_uuid, deleted_at "
        "FROM sync_tombstones "
        "WHERE table_name = :table_name;");
    query.bindValue(QStringLiteral(":table_name"), tableName);
    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return result;
    }

    while (query.next()) {
        result.insert(query.value(0).toString(), query.value(1).toString());
    }
    return result;
}

int lookupIdBySyncUuid(QSqlDatabase &database,
                       const QString &tableName,
                       const QString &syncUuid,
                       QString *errorMessage)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral("SELECT id FROM %1 WHERE sync_uuid = :sync_uuid;").arg(tableName));
    query.bindValue(QStringLiteral(":sync_uuid"), syncUuid);
    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return -1;
    }
    if (!query.next()) {
        return 0;
    }
    return query.value(0).toInt();
}

bool deleteBySyncUuid(QSqlDatabase &database,
                      const QString &tableName,
                      const QString &syncUuid,
                      const QString &deletedAt,
                      QString *errorMessage)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral("DELETE FROM %1 WHERE sync_uuid = :sync_uuid;").arg(tableName));
    query.bindValue(QStringLiteral(":sync_uuid"), syncUuid);
    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    return upsertTombstone(database, tableName, syncUuid, deletedAt, errorMessage);
}

QMap<QString, PlantRecord> loadPlants(QSqlDatabase &database, QString *errorMessage)
{
    QMap<QString, PlantRecord> result;

    QSqlQuery query(database);
    if (!query.exec(
            "SELECT sync_uuid, name, species, location, scientific_name, plant_type, "
            "light_requirement, watering_frequency, watering_notes, humidity_preference, "
            "soil_type, last_watered, fertilizing_schedule, last_fertilized, pruning_time, "
            "pruning_notes, last_pruned, growth_rate, issues_pests, "
            "temperature_tolerance, toxic_to_pets, poisonous_to_humans, poisonous_to_pets, indoor, "
            "flowering_season, tags, acquired_on, source, notes, created_at, updated_at "
            "FROM plants "
            "WHERE sync_uuid IS NOT NULL AND trim(sync_uuid) <> '';")) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return result;
    }

    while (query.next()) {
        PlantRecord record;
        record.syncUuid = query.value(0).toString();
        record.name = query.value(1).toString();
        record.species = query.value(2).toString();
        record.location = query.value(3).toString();
        record.scientificName = query.value(4).toString();
        record.plantType = query.value(5).toString();
        record.lightRequirement = query.value(6).toString();
        record.wateringFrequency = query.value(7).toString();
        record.wateringNotes = query.value(8).toString();
        record.humidityPreference = query.value(9).toString();
        record.soilType = query.value(10).toString();
        record.lastWatered = query.value(11).toString();
        record.fertilizingSchedule = query.value(12).toString();
        record.lastFertilized = query.value(13).toString();
        record.pruningTime = query.value(14).toString();
        record.pruningNotes = query.value(15).toString();
        record.lastPruned = query.value(16).toString();
        record.growthRate = query.value(17).toString();
        record.issuesPests = query.value(18).toString();
        record.temperatureTolerance = query.value(19).toString();
        record.toxicToPets = query.value(20).toString();
        record.poisonousToHumans = query.value(21).toString();
        record.poisonousToPets = query.value(22).toString();
        record.indoor = query.value(23).toString();
        record.floweringSeason = query.value(24).toString();
        record.tags = query.value(25).toString();
        record.acquiredOn = query.value(26).toString();
        record.source = query.value(27).toString();
        record.notes = query.value(28).toString();
        record.createdAt = query.value(29).toString();
        record.updatedAt = query.value(30).toString();
        result.insert(record.syncUuid, record);
    }

    return result;
}

QMap<QString, JournalEntrySyncRecord> loadJournalEntries(QSqlDatabase &database, QString *errorMessage)
{
    QMap<QString, JournalEntrySyncRecord> result;

    QSqlQuery query(database);
    if (!query.exec(
            "SELECT je.sync_uuid, p.sync_uuid, je.entry_type, je.entry_date, je.notes, je.created_at, je.updated_at "
            "FROM journal_entries je "
            "JOIN plants p ON p.id = je.plant_id "
            "WHERE je.sync_uuid IS NOT NULL AND trim(je.sync_uuid) <> '';")) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return result;
    }

    while (query.next()) {
        JournalEntrySyncRecord record;
        record.syncUuid = query.value(0).toString();
        record.plantSyncUuid = query.value(1).toString();
        record.entryType = query.value(2).toString();
        record.entryDate = query.value(3).toString();
        record.notes = query.value(4).toString();
        record.createdAt = query.value(5).toString();
        record.updatedAt = query.value(6).toString();
        result.insert(record.syncUuid, record);
    }

    return result;
}

QMap<QString, ReminderSyncRecord> loadReminders(QSqlDatabase &database, QString *errorMessage)
{
    QMap<QString, ReminderSyncRecord> result;

    QSqlQuery query(database);
    if (!query.exec(
            "SELECT r.sync_uuid, r.custom_name, r.task_type, r.schedule_type, r.interval_days, "
            "r.start_date, r.next_due_date, r.notes, r.created_at, r.updated_at, p.sync_uuid "
            "FROM reminders r "
            "LEFT JOIN reminder_plants rp ON rp.reminder_id = r.id "
            "LEFT JOIN plants p ON p.id = rp.plant_id "
            "WHERE r.sync_uuid IS NOT NULL AND trim(r.sync_uuid) <> '' "
            "ORDER BY r.sync_uuid, p.sync_uuid;")) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return result;
    }

    while (query.next()) {
        const QString syncUuid = query.value(0).toString();
        ReminderSyncRecord &record = result[syncUuid];
        record.syncUuid = syncUuid;
        record.customName = query.value(1).toString();
        record.taskType = query.value(2).toString();
        record.scheduleType = query.value(3).toString();
        record.intervalDays = query.value(4).toInt();
        record.startDate = query.value(5).toString();
        record.nextDueDate = query.value(6).toString();
        record.notes = query.value(7).toString();
        record.createdAt = query.value(8).toString();
        record.updatedAt = query.value(9).toString();

        const QString plantSyncUuid = query.value(10).toString();
        if (!plantSyncUuid.isEmpty() && !record.plantSyncUuids.contains(plantSyncUuid)) {
            record.plantSyncUuids.append(plantSyncUuid);
        }
    }

    return result;
}

QMap<QString, PlantImageSyncRecord> loadPlantImages(QSqlDatabase &database, QString *errorMessage)
{
    QMap<QString, PlantImageSyncRecord> result;

    QSqlQuery query(database);
    if (!query.exec(
            "SELECT pi.sync_uuid, p.sync_uuid, pi.file_path, pi.created_at, pi.updated_at "
            "FROM plant_images pi "
            "JOIN plants p ON p.id = pi.plant_id "
            "WHERE pi.sync_uuid IS NOT NULL AND trim(pi.sync_uuid) <> '';")) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return result;
    }

    while (query.next()) {
        PlantImageSyncRecord record;
        record.syncUuid = query.value(0).toString();
        record.plantSyncUuid = query.value(1).toString();
        record.filePath = query.value(2).toString();
        record.createdAt = query.value(3).toString();
        record.updatedAt = query.value(4).toString();
        result.insert(record.syncUuid, record);
    }

    return result;
}

QMap<QString, CareScheduleSyncRecord> loadCareSchedules(QSqlDatabase &database, QString *errorMessage)
{
    QMap<QString, CareScheduleSyncRecord> result;

    QSqlQuery query(database);
    if (!query.exec(
            "SELECT pcs.sync_uuid, p.sync_uuid, pcs.care_type, pcs.season_name, pcs.interval_days, "
            "pcs.is_enabled, pcs.created_at, pcs.updated_at "
            "FROM plant_care_schedules pcs "
            "JOIN plants p ON p.id = pcs.plant_id "
            "WHERE pcs.sync_uuid IS NOT NULL AND trim(pcs.sync_uuid) <> '';")) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return result;
    }

    while (query.next()) {
        CareScheduleSyncRecord record;
        record.syncUuid = query.value(0).toString();
        record.plantSyncUuid = query.value(1).toString();
        record.careType = query.value(2).toString();
        record.seasonName = query.value(3).toString();
        record.intervalDays = query.value(4).toInt();
        record.enabled = query.value(5).toInt() != 0;
        record.createdAt = query.value(6).toString();
        record.updatedAt = query.value(7).toString();
        result.insert(record.syncUuid, record);
    }

    return result;
}

QMap<QString, TagCatalogSyncRecord> loadTagCatalog(QSqlDatabase &database, QString *errorMessage)
{
    QMap<QString, TagCatalogSyncRecord> result;

    QSqlQuery query(database);
    if (!query.exec(
            "SELECT sync_uuid, name, created_at, updated_at "
            "FROM plant_tags_catalog "
            "WHERE sync_uuid IS NOT NULL AND trim(sync_uuid) <> '';")) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return result;
    }

    while (query.next()) {
        TagCatalogSyncRecord record;
        record.syncUuid = query.value(0).toString();
        record.name = query.value(1).toString();
        record.createdAt = query.value(2).toString();
        record.updatedAt = query.value(3).toString();
        result.insert(record.syncUuid, record);
    }

    return result;
}

ReminderSettingsSyncRecord loadReminderSettings(QSqlDatabase &database, QString *errorMessage)
{
    ReminderSettingsSyncRecord record;

    QSqlQuery query(database);
    if (!query.exec(
            "SELECT day_before_enabled, day_before_time, day_of_enabled, day_of_time, overdue_enabled, "
            "overdue_cadence_days, overdue_time, quiet_hours_start, quiet_hours_end, updated_at "
            "FROM reminder_settings "
            "WHERE id = 1;")) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return record;
    }

    if (!query.next()) {
        return record;
    }

    record.exists = true;
    record.dayBeforeEnabled = query.value(0).toInt() != 0;
    record.dayBeforeTime = query.value(1).toString();
    record.dayOfEnabled = query.value(2).toInt() != 0;
    record.dayOfTime = query.value(3).toString();
    record.overdueEnabled = query.value(4).toInt() != 0;
    record.overdueCadenceDays = query.value(5).toInt();
    record.overdueTime = query.value(6).toString();
    record.quietHoursStart = query.value(7).toString();
    record.quietHoursEnd = query.value(8).toString();
    record.updatedAt = query.value(9).toString();
    return record;
}

bool upsertPlant(QSqlDatabase &database, const PlantRecord &record, QString *errorMessage)
{
    const int existingId = lookupIdBySyncUuid(database, QStringLiteral("plants"), record.syncUuid, errorMessage);
    if (existingId < 0) {
        return false;
    }

    QSqlQuery query(database);
    if (existingId > 0) {
        query.prepare(
            "UPDATE plants SET "
            "name = :name, species = :species, location = :location, scientific_name = :scientific_name, "
            "plant_type = :plant_type, "
            "light_requirement = :light_requirement, watering_frequency = :watering_frequency, "
            "watering_notes = :watering_notes, humidity_preference = :humidity_preference, soil_type = :soil_type, "
            "last_watered = :last_watered, fertilizing_schedule = :fertilizing_schedule, "
            "last_fertilized = :last_fertilized, pruning_time = :pruning_time, pruning_notes = :pruning_notes, "
            "last_pruned = :last_pruned, growth_rate = :growth_rate, "
            "issues_pests = :issues_pests, temperature_tolerance = :temperature_tolerance, "
            "toxic_to_pets = :toxic_to_pets, poisonous_to_humans = :poisonous_to_humans, "
            "poisonous_to_pets = :poisonous_to_pets, indoor = :indoor, "
            "flowering_season = :flowering_season, tags = :tags, acquired_on = :acquired_on, source = :source, notes = :notes, "
            "created_at = :created_at, updated_at = :updated_at "
            "WHERE sync_uuid = :sync_uuid;");
    } else {
        query.prepare(
            "INSERT INTO plants ("
            "sync_uuid, name, species, location, scientific_name, plant_type, "
            "light_requirement, watering_frequency, watering_notes, humidity_preference, soil_type, "
            "last_watered, fertilizing_schedule, last_fertilized, pruning_time, pruning_notes, last_pruned, "
            "growth_rate, issues_pests, temperature_tolerance, toxic_to_pets, poisonous_to_humans, "
            "poisonous_to_pets, indoor, flowering_season, tags, "
            "acquired_on, source, notes, created_at, updated_at"
            ") VALUES ("
            ":sync_uuid, :name, :species, :location, :scientific_name, :plant_type, "
            ":light_requirement, :watering_frequency, :watering_notes, :humidity_preference, :soil_type, "
            ":last_watered, :fertilizing_schedule, :last_fertilized, :pruning_time, :pruning_notes, :last_pruned, "
            ":growth_rate, :issues_pests, :temperature_tolerance, :toxic_to_pets, :poisonous_to_humans, "
            ":poisonous_to_pets, :indoor, :flowering_season, :tags, "
            ":acquired_on, :source, :notes, :created_at, :updated_at"
            ");");
    }

    query.bindValue(QStringLiteral(":sync_uuid"), record.syncUuid);
    query.bindValue(QStringLiteral(":name"), record.name);
    query.bindValue(QStringLiteral(":species"), record.species);
    query.bindValue(QStringLiteral(":location"), record.location);
    query.bindValue(QStringLiteral(":scientific_name"), record.scientificName);
    query.bindValue(QStringLiteral(":plant_type"), record.plantType);
    query.bindValue(QStringLiteral(":light_requirement"), record.lightRequirement);
    query.bindValue(QStringLiteral(":watering_frequency"), record.wateringFrequency);
    query.bindValue(QStringLiteral(":watering_notes"), record.wateringNotes);
    query.bindValue(QStringLiteral(":humidity_preference"), record.humidityPreference);
    query.bindValue(QStringLiteral(":soil_type"), record.soilType);
    query.bindValue(QStringLiteral(":last_watered"), record.lastWatered);
    query.bindValue(QStringLiteral(":fertilizing_schedule"), record.fertilizingSchedule);
    query.bindValue(QStringLiteral(":last_fertilized"), record.lastFertilized);
    query.bindValue(QStringLiteral(":pruning_time"), record.pruningTime);
    query.bindValue(QStringLiteral(":pruning_notes"), record.pruningNotes);
    query.bindValue(QStringLiteral(":last_pruned"), record.lastPruned);
    query.bindValue(QStringLiteral(":growth_rate"), record.growthRate);
    query.bindValue(QStringLiteral(":issues_pests"), record.issuesPests);
    query.bindValue(QStringLiteral(":temperature_tolerance"), record.temperatureTolerance);
    query.bindValue(QStringLiteral(":toxic_to_pets"), record.toxicToPets);
    query.bindValue(QStringLiteral(":poisonous_to_humans"), record.poisonousToHumans);
    query.bindValue(QStringLiteral(":poisonous_to_pets"), record.poisonousToPets);
    query.bindValue(QStringLiteral(":indoor"), record.indoor);
    query.bindValue(QStringLiteral(":flowering_season"), record.floweringSeason);
    query.bindValue(QStringLiteral(":tags"), record.tags);
    query.bindValue(QStringLiteral(":acquired_on"), record.acquiredOn);
    query.bindValue(QStringLiteral(":source"), record.source);
    query.bindValue(QStringLiteral(":notes"), record.notes);
    query.bindValue(QStringLiteral(":created_at"), record.createdAt);
    query.bindValue(QStringLiteral(":updated_at"), record.updatedAt);
    if (query.exec()) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = query.lastError().text();
    }
    return false;
}

bool upsertJournalEntry(QSqlDatabase &database,
                        const JournalEntrySyncRecord &record,
                        QString *errorMessage)
{
    const int plantId = lookupIdBySyncUuid(database, QStringLiteral("plants"), record.plantSyncUuid, errorMessage);
    if (plantId <= 0) {
        if (errorMessage && errorMessage->isEmpty()) {
            *errorMessage = QStringLiteral("Plant mapping missing for journal entry sync.");
        }
        return false;
    }

    const int existingId = lookupIdBySyncUuid(database, QStringLiteral("journal_entries"), record.syncUuid, errorMessage);
    if (existingId < 0) {
        return false;
    }

    QSqlQuery query(database);
    if (existingId > 0) {
        query.prepare(
            "UPDATE journal_entries SET "
            "plant_id = :plant_id, entry_type = :entry_type, entry_date = :entry_date, notes = :notes, "
            "created_at = :created_at, updated_at = :updated_at "
            "WHERE sync_uuid = :sync_uuid;");
    } else {
        query.prepare(
            "INSERT INTO journal_entries (sync_uuid, plant_id, entry_type, entry_date, notes, created_at, updated_at) "
            "VALUES (:sync_uuid, :plant_id, :entry_type, :entry_date, :notes, :created_at, :updated_at);");
    }

    query.bindValue(QStringLiteral(":sync_uuid"), record.syncUuid);
    query.bindValue(QStringLiteral(":plant_id"), plantId);
    query.bindValue(QStringLiteral(":entry_type"), record.entryType);
    query.bindValue(QStringLiteral(":entry_date"), record.entryDate);
    query.bindValue(QStringLiteral(":notes"), record.notes);
    query.bindValue(QStringLiteral(":created_at"), record.createdAt);
    query.bindValue(QStringLiteral(":updated_at"), record.updatedAt);
    if (query.exec()) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = query.lastError().text();
    }
    return false;
}

bool upsertReminder(QSqlDatabase &database,
                    const ReminderSyncRecord &record,
                    QString *errorMessage)
{
    QVector<int> plantIds;
    plantIds.reserve(record.plantSyncUuids.size());
    for (const QString &plantSyncUuid : record.plantSyncUuids) {
        const int plantId = lookupIdBySyncUuid(database, QStringLiteral("plants"), plantSyncUuid, errorMessage);
        if (plantId <= 0) {
            if (errorMessage && errorMessage->isEmpty()) {
                *errorMessage = QStringLiteral("Plant mapping missing for reminder sync.");
            }
            return false;
        }
        plantIds.append(plantId);
    }

    const int existingId = lookupIdBySyncUuid(database, QStringLiteral("reminders"), record.syncUuid, errorMessage);
    if (existingId < 0) {
        return false;
    }

    if (!database.transaction()) {
        if (errorMessage) {
            *errorMessage = database.lastError().text();
        }
        return false;
    }

    QSqlQuery query(database);
    if (existingId > 0) {
        query.prepare(
            "UPDATE reminders SET "
            "custom_name = :custom_name, task_type = :task_type, schedule_type = :schedule_type, "
            "interval_days = :interval_days, start_date = :start_date, next_due_date = :next_due_date, "
            "notes = :notes, created_at = :created_at, updated_at = :updated_at "
            "WHERE sync_uuid = :sync_uuid;");
    } else {
        query.prepare(
            "INSERT INTO reminders (sync_uuid, custom_name, task_type, schedule_type, interval_days, start_date, "
            "next_due_date, notes, created_at, updated_at) "
            "VALUES (:sync_uuid, :custom_name, :task_type, :schedule_type, :interval_days, :start_date, "
            ":next_due_date, :notes, :created_at, :updated_at);");
    }

    query.bindValue(QStringLiteral(":sync_uuid"), record.syncUuid);
    query.bindValue(QStringLiteral(":custom_name"), record.customName);
    query.bindValue(QStringLiteral(":task_type"), record.taskType);
    query.bindValue(QStringLiteral(":schedule_type"), record.scheduleType);
    query.bindValue(QStringLiteral(":interval_days"), record.intervalDays > 0 ? QVariant(record.intervalDays) : QVariant());
    query.bindValue(QStringLiteral(":start_date"), record.startDate);
    query.bindValue(QStringLiteral(":next_due_date"), record.nextDueDate);
    query.bindValue(QStringLiteral(":notes"), record.notes);
    query.bindValue(QStringLiteral(":created_at"), record.createdAt);
    query.bindValue(QStringLiteral(":updated_at"), record.updatedAt);
    if (!query.exec()) {
        const QString failure = query.lastError().text();
        database.rollback();
        if (errorMessage) {
            *errorMessage = failure;
        }
        return false;
    }

    const int reminderId = existingId > 0
        ? existingId
        : lookupIdBySyncUuid(database, QStringLiteral("reminders"), record.syncUuid, errorMessage);
    if (reminderId <= 0) {
        database.rollback();
        if (errorMessage && errorMessage->isEmpty()) {
            *errorMessage = QStringLiteral("Failed to resolve reminder id after insert.");
        }
        return false;
    }

    QSqlQuery clearQuery(database);
    clearQuery.prepare("DELETE FROM reminder_plants WHERE reminder_id = :reminder_id;");
    clearQuery.bindValue(QStringLiteral(":reminder_id"), reminderId);
    if (!clearQuery.exec()) {
        const QString failure = clearQuery.lastError().text();
        database.rollback();
        if (errorMessage) {
            *errorMessage = failure;
        }
        return false;
    }

    if (!plantIds.isEmpty()) {
        QSqlQuery insertQuery(database);
        insertQuery.prepare(
            "INSERT INTO reminder_plants (reminder_id, plant_id) "
            "VALUES (:reminder_id, :plant_id);");
        for (const int plantId : plantIds) {
            insertQuery.bindValue(QStringLiteral(":reminder_id"), reminderId);
            insertQuery.bindValue(QStringLiteral(":plant_id"), plantId);
            if (!insertQuery.exec()) {
                const QString failure = insertQuery.lastError().text();
                database.rollback();
                if (errorMessage) {
                    *errorMessage = failure;
                }
                return false;
            }
        }
    }

    if (database.commit()) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = database.lastError().text();
    }
    database.rollback();
    return false;
}

bool upsertPlantImage(QSqlDatabase &database,
                      const PlantImageSyncRecord &record,
                      QString *errorMessage)
{
    const int plantId = lookupIdBySyncUuid(database, QStringLiteral("plants"), record.plantSyncUuid, errorMessage);
    if (plantId <= 0) {
        if (errorMessage && errorMessage->isEmpty()) {
            *errorMessage = QStringLiteral("Plant mapping missing for image sync.");
        }
        return false;
    }

    const int existingId = lookupIdBySyncUuid(database, QStringLiteral("plant_images"), record.syncUuid, errorMessage);
    if (existingId < 0) {
        return false;
    }

    QSqlQuery query(database);
    if (existingId > 0) {
        query.prepare(
            "UPDATE plant_images SET "
            "plant_id = :plant_id, file_path = :file_path, created_at = :created_at, updated_at = :updated_at "
            "WHERE sync_uuid = :sync_uuid;");
    } else {
        query.prepare(
            "INSERT INTO plant_images (sync_uuid, plant_id, file_path, created_at, updated_at) "
            "VALUES (:sync_uuid, :plant_id, :file_path, :created_at, :updated_at);");
    }

    query.bindValue(QStringLiteral(":sync_uuid"), record.syncUuid);
    query.bindValue(QStringLiteral(":plant_id"), plantId);
    query.bindValue(QStringLiteral(":file_path"), record.filePath);
    query.bindValue(QStringLiteral(":created_at"), record.createdAt);
    query.bindValue(QStringLiteral(":updated_at"), record.updatedAt);
    if (query.exec()) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = query.lastError().text();
    }
    return false;
}

bool upsertCareSchedule(QSqlDatabase &database,
                        const CareScheduleSyncRecord &record,
                        QString *errorMessage)
{
    const int plantId = lookupIdBySyncUuid(database, QStringLiteral("plants"), record.plantSyncUuid, errorMessage);
    if (plantId <= 0) {
        if (errorMessage && errorMessage->isEmpty()) {
            *errorMessage = QStringLiteral("Plant mapping missing for care schedule sync.");
        }
        return false;
    }

    const int existingId = lookupIdBySyncUuid(database, QStringLiteral("plant_care_schedules"), record.syncUuid, errorMessage);
    if (existingId < 0) {
        return false;
    }

    QSqlQuery query(database);
    if (existingId > 0) {
        query.prepare(
            "UPDATE plant_care_schedules SET "
            "plant_id = :plant_id, care_type = :care_type, season_name = :season_name, "
            "interval_days = :interval_days, is_enabled = :is_enabled, "
            "created_at = :created_at, updated_at = :updated_at "
            "WHERE sync_uuid = :sync_uuid;");
    } else {
        query.prepare(
            "INSERT INTO plant_care_schedules (sync_uuid, plant_id, care_type, season_name, interval_days, is_enabled, created_at, updated_at) "
            "VALUES (:sync_uuid, :plant_id, :care_type, :season_name, :interval_days, :is_enabled, :created_at, :updated_at);");
    }

    query.bindValue(QStringLiteral(":sync_uuid"), record.syncUuid);
    query.bindValue(QStringLiteral(":plant_id"), plantId);
    query.bindValue(QStringLiteral(":care_type"), record.careType);
    query.bindValue(QStringLiteral(":season_name"), record.seasonName);
    query.bindValue(QStringLiteral(":interval_days"), record.enabled && record.intervalDays > 0 ? QVariant(record.intervalDays) : QVariant());
    query.bindValue(QStringLiteral(":is_enabled"), record.enabled ? 1 : 0);
    query.bindValue(QStringLiteral(":created_at"), record.createdAt);
    query.bindValue(QStringLiteral(":updated_at"), record.updatedAt);
    if (query.exec()) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = query.lastError().text();
    }
    return false;
}

bool upsertTagCatalog(QSqlDatabase &database,
                      const TagCatalogSyncRecord &record,
                      QString *errorMessage)
{
    QSqlQuery query(database);
    query.prepare(
        "INSERT INTO plant_tags_catalog (name, sync_uuid, created_at, updated_at) "
        "VALUES (:name, :sync_uuid, :created_at, :updated_at) "
        "ON CONFLICT(sync_uuid) DO UPDATE SET "
        "name = excluded.name, "
        "created_at = excluded.created_at, "
        "updated_at = excluded.updated_at;");
    query.bindValue(QStringLiteral(":name"), record.name.trimmed().toLower());
    query.bindValue(QStringLiteral(":sync_uuid"), record.syncUuid);
    query.bindValue(QStringLiteral(":created_at"), record.createdAt);
    query.bindValue(QStringLiteral(":updated_at"), record.updatedAt);
    if (query.exec()) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = query.lastError().text();
    }
    return false;
}

bool upsertReminderSettings(QSqlDatabase &database,
                            const ReminderSettingsSyncRecord &record,
                            QString *errorMessage)
{
    QSqlQuery query(database);
    query.prepare(
        "UPDATE reminder_settings SET "
        "day_before_enabled = :day_before_enabled, "
        "day_before_time = :day_before_time, "
        "day_of_enabled = :day_of_enabled, "
        "day_of_time = :day_of_time, "
        "overdue_enabled = :overdue_enabled, "
        "overdue_cadence_days = :overdue_cadence_days, "
        "overdue_time = :overdue_time, "
        "quiet_hours_start = :quiet_hours_start, "
        "quiet_hours_end = :quiet_hours_end, "
        "updated_at = :updated_at "
        "WHERE id = 1;");
    query.bindValue(QStringLiteral(":day_before_enabled"), record.dayBeforeEnabled ? 1 : 0);
    query.bindValue(QStringLiteral(":day_before_time"), record.dayBeforeTime);
    query.bindValue(QStringLiteral(":day_of_enabled"), record.dayOfEnabled ? 1 : 0);
    query.bindValue(QStringLiteral(":day_of_time"), record.dayOfTime);
    query.bindValue(QStringLiteral(":overdue_enabled"), record.overdueEnabled ? 1 : 0);
    query.bindValue(QStringLiteral(":overdue_cadence_days"), record.overdueCadenceDays);
    query.bindValue(QStringLiteral(":overdue_time"), record.overdueTime);
    query.bindValue(QStringLiteral(":quiet_hours_start"), record.quietHoursStart);
    query.bindValue(QStringLiteral(":quiet_hours_end"), record.quietHoursEnd);
    query.bindValue(QStringLiteral(":updated_at"), record.updatedAt);
    if (query.exec()) {
        return true;
    }

    QSqlQuery insertQuery(database);
    insertQuery.prepare(
        "INSERT INTO reminder_settings (id, day_before_enabled, day_before_time, day_of_enabled, day_of_time, "
        "overdue_enabled, overdue_cadence_days, overdue_time, quiet_hours_start, quiet_hours_end, updated_at) "
        "VALUES (1, :day_before_enabled, :day_before_time, :day_of_enabled, :day_of_time, "
        ":overdue_enabled, :overdue_cadence_days, :overdue_time, :quiet_hours_start, :quiet_hours_end, :updated_at);");
    insertQuery.bindValue(QStringLiteral(":day_before_enabled"), record.dayBeforeEnabled ? 1 : 0);
    insertQuery.bindValue(QStringLiteral(":day_before_time"), record.dayBeforeTime);
    insertQuery.bindValue(QStringLiteral(":day_of_enabled"), record.dayOfEnabled ? 1 : 0);
    insertQuery.bindValue(QStringLiteral(":day_of_time"), record.dayOfTime);
    insertQuery.bindValue(QStringLiteral(":overdue_enabled"), record.overdueEnabled ? 1 : 0);
    insertQuery.bindValue(QStringLiteral(":overdue_cadence_days"), record.overdueCadenceDays);
    insertQuery.bindValue(QStringLiteral(":overdue_time"), record.overdueTime);
    insertQuery.bindValue(QStringLiteral(":quiet_hours_start"), record.quietHoursStart);
    insertQuery.bindValue(QStringLiteral(":quiet_hours_end"), record.quietHoursEnd);
    insertQuery.bindValue(QStringLiteral(":updated_at"), record.updatedAt);
    if (insertQuery.exec()) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = insertQuery.lastError().text();
    }
    return false;
}

bool changedSinceCursor(const QString &timestamp, const QString &cursor)
{
    return cursor.trimmed().isEmpty() || compareTimestamps(timestamp, cursor) > 0;
}

template <typename RecordMap>
RecordMap filterChangedRecords(const RecordMap &records, const QString &cursor)
{
    if (cursor.trimmed().isEmpty()) {
        return records;
    }

    RecordMap filtered;
    for (auto it = records.cbegin(); it != records.cend(); ++it) {
        if (changedSinceCursor(it.value().updatedAt, cursor)) {
            filtered.insert(it.key(), it.value());
        }
    }
    return filtered;
}

template <typename RecordMap>
RecordMap loadAllRecords(const RecordMap &records)
{
    return records;
}

QVector<TombstoneRecord> loadTombstoneRecords(QSqlDatabase &database,
                                              const QString &entityType,
                                              const QString &cursor,
                                              QString *errorMessage)
{
    QVector<TombstoneRecord> result;
    const QMap<QString, QString> rawTombstones = loadTombstones(database, entityType, errorMessage);
    if (errorMessage && !errorMessage->isEmpty()) {
        return result;
    }

    for (auto it = rawTombstones.cbegin(); it != rawTombstones.cend(); ++it) {
        if (!changedSinceCursor(it.value(), cursor)) {
            continue;
        }

        TombstoneRecord record;
        record.entityType = entityType;
        record.syncUuid = it.key();
        record.deletedAt = it.value();
        result.append(record);
    }
    return result;
}

SyncDataset collectLocalChanges(QSqlDatabase &database,
                                const QString &cursor,
                                QString *errorMessage)
{
    SyncDataset dataset;
    QString loadError;

    Q_UNUSED(cursor);

    dataset.plants = loadAllRecords(loadPlants(database, &loadError));
    if (!loadError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = loadError;
        }
        return {};
    }

    dataset.journalEntries = loadAllRecords(loadJournalEntries(database, &loadError));
    if (!loadError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = loadError;
        }
        return {};
    }

    dataset.reminders = loadAllRecords(loadReminders(database, &loadError));
    if (!loadError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = loadError;
        }
        return {};
    }

    dataset.plantImages = loadAllRecords(loadPlantImages(database, &loadError));
    if (!loadError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = loadError;
        }
        return {};
    }

    dataset.careSchedules = loadAllRecords(loadCareSchedules(database, &loadError));
    if (!loadError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = loadError;
        }
        return {};
    }

    dataset.tagCatalog = loadAllRecords(loadTagCatalog(database, &loadError));
    if (!loadError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = loadError;
        }
        return {};
    }

    dataset.reminderSettings = loadReminderSettings(database, &loadError);
    if (!loadError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = loadError;
        }
        return {};
    }
    if (!dataset.reminderSettings.exists) {
        dataset.reminderSettings = ReminderSettingsSyncRecord();
    }

    const QStringList tombstoneEntities = {
        QStringLiteral("plants"),
        QStringLiteral("journal_entries"),
        QStringLiteral("reminders"),
        QStringLiteral("plant_images"),
        QStringLiteral("plant_care_schedules"),
        QStringLiteral("plant_tags_catalog")
    };
    for (const QString &entityType : tombstoneEntities) {
        QVector<TombstoneRecord> tombstones = loadTombstoneRecords(database, entityType, QString(), &loadError);
        if (!loadError.isEmpty()) {
            if (errorMessage) {
                *errorMessage = loadError;
            }
            return {};
        }
        dataset.tombstones += tombstones;
    }

    return dataset;
}

QJsonObject plantToJson(const PlantRecord &record)
{
    return {
        { QStringLiteral("sync_uuid"), record.syncUuid },
        { QStringLiteral("name"), record.name },
        { QStringLiteral("species"), record.species },
        { QStringLiteral("location"), record.location },
        { QStringLiteral("scientific_name"), record.scientificName },
        { QStringLiteral("plant_type"), record.plantType },
        { QStringLiteral("light_requirement"), record.lightRequirement },
        { QStringLiteral("watering_frequency"), record.wateringFrequency },
        { QStringLiteral("watering_notes"), record.wateringNotes },
        { QStringLiteral("humidity_preference"), record.humidityPreference },
        { QStringLiteral("soil_type"), record.soilType },
        { QStringLiteral("last_watered"), record.lastWatered },
        { QStringLiteral("fertilizing_schedule"), record.fertilizingSchedule },
        { QStringLiteral("last_fertilized"), record.lastFertilized },
        { QStringLiteral("pruning_time"), record.pruningTime },
        { QStringLiteral("pruning_notes"), record.pruningNotes },
        { QStringLiteral("last_pruned"), record.lastPruned },
        { QStringLiteral("growth_rate"), record.growthRate },
        { QStringLiteral("issues_pests"), record.issuesPests },
        { QStringLiteral("temperature_tolerance"), record.temperatureTolerance },
        { QStringLiteral("toxic_to_pets"), record.toxicToPets },
        { QStringLiteral("poisonous_to_humans"), record.poisonousToHumans },
        { QStringLiteral("poisonous_to_pets"), record.poisonousToPets },
        { QStringLiteral("indoor"), record.indoor },
        { QStringLiteral("flowering_season"), record.floweringSeason },
        { QStringLiteral("tags"), record.tags },
        { QStringLiteral("acquired_on"), record.acquiredOn },
        { QStringLiteral("source"), record.source },
        { QStringLiteral("notes"), record.notes },
        { QStringLiteral("created_at"), record.createdAt },
        { QStringLiteral("updated_at"), record.updatedAt }
    };
}

QJsonObject journalEntryToJson(const JournalEntrySyncRecord &record)
{
    return {
        { QStringLiteral("sync_uuid"), record.syncUuid },
        { QStringLiteral("plant_sync_uuid"), record.plantSyncUuid },
        { QStringLiteral("entry_type"), record.entryType },
        { QStringLiteral("entry_date"), record.entryDate },
        { QStringLiteral("notes"), record.notes },
        { QStringLiteral("created_at"), record.createdAt },
        { QStringLiteral("updated_at"), record.updatedAt }
    };
}

QJsonObject reminderToJson(const ReminderSyncRecord &record)
{
    QJsonArray plantSyncUuids;
    for (const QString &plantSyncUuid : record.plantSyncUuids) {
        plantSyncUuids.append(plantSyncUuid);
    }

    return {
        { QStringLiteral("sync_uuid"), record.syncUuid },
        { QStringLiteral("custom_name"), record.customName },
        { QStringLiteral("task_type"), record.taskType },
        { QStringLiteral("schedule_type"), record.scheduleType },
        { QStringLiteral("interval_days"), record.intervalDays },
        { QStringLiteral("start_date"), record.startDate },
        { QStringLiteral("next_due_date"), record.nextDueDate },
        { QStringLiteral("notes"), record.notes },
        { QStringLiteral("created_at"), record.createdAt },
        { QStringLiteral("updated_at"), record.updatedAt },
        { QStringLiteral("plant_sync_uuids"), plantSyncUuids }
    };
}

QJsonObject plantImageToJson(const PlantImageSyncRecord &record)
{
    return {
        { QStringLiteral("sync_uuid"), record.syncUuid },
        { QStringLiteral("plant_sync_uuid"), record.plantSyncUuid },
        { QStringLiteral("file_path"), record.filePath },
        { QStringLiteral("created_at"), record.createdAt },
        { QStringLiteral("updated_at"), record.updatedAt }
    };
}

QJsonObject careScheduleToJson(const CareScheduleSyncRecord &record)
{
    return {
        { QStringLiteral("sync_uuid"), record.syncUuid },
        { QStringLiteral("plant_sync_uuid"), record.plantSyncUuid },
        { QStringLiteral("care_type"), record.careType },
        { QStringLiteral("season_name"), record.seasonName },
        { QStringLiteral("interval_days"), record.intervalDays },
        { QStringLiteral("is_enabled"), record.enabled },
        { QStringLiteral("created_at"), record.createdAt },
        { QStringLiteral("updated_at"), record.updatedAt }
    };
}

QJsonObject reminderSettingsToJson(const ReminderSettingsSyncRecord &record)
{
    return {
        { QStringLiteral("day_before_enabled"), record.dayBeforeEnabled },
        { QStringLiteral("day_before_time"), record.dayBeforeTime },
        { QStringLiteral("day_of_enabled"), record.dayOfEnabled },
        { QStringLiteral("day_of_time"), record.dayOfTime },
        { QStringLiteral("overdue_enabled"), record.overdueEnabled },
        { QStringLiteral("overdue_cadence_days"), record.overdueCadenceDays },
        { QStringLiteral("overdue_time"), record.overdueTime },
        { QStringLiteral("quiet_hours_start"), record.quietHoursStart },
        { QStringLiteral("quiet_hours_end"), record.quietHoursEnd },
        { QStringLiteral("updated_at"), record.updatedAt }
    };
}

QJsonObject tagCatalogToJson(const TagCatalogSyncRecord &record)
{
    return {
        { QStringLiteral("sync_uuid"), record.syncUuid },
        { QStringLiteral("name"), record.name },
        { QStringLiteral("created_at"), record.createdAt },
        { QStringLiteral("updated_at"), record.updatedAt }
    };
}

QJsonObject tombstoneToJson(const TombstoneRecord &record)
{
    return {
        { QStringLiteral("entity_type"), record.entityType },
        { QStringLiteral("sync_uuid"), record.syncUuid },
        { QStringLiteral("deleted_at"), record.deletedAt }
    };
}

template <typename RecordMap, typename Converter>
QJsonArray recordMapToJsonArray(const RecordMap &records, Converter converter)
{
    QJsonArray array;
    for (auto it = records.cbegin(); it != records.cend(); ++it) {
        array.append(converter(it.value()));
    }
    return array;
}

QJsonObject datasetToJson(const SyncDataset &dataset)
{
    QJsonArray tombstones;
    for (const TombstoneRecord &record : dataset.tombstones) {
        tombstones.append(tombstoneToJson(record));
    }

    return {
        { QStringLiteral("plants"), recordMapToJsonArray(dataset.plants, plantToJson) },
        { QStringLiteral("journal_entries"), recordMapToJsonArray(dataset.journalEntries, journalEntryToJson) },
        { QStringLiteral("reminders"), recordMapToJsonArray(dataset.reminders, reminderToJson) },
        { QStringLiteral("plant_images"), recordMapToJsonArray(dataset.plantImages, plantImageToJson) },
        { QStringLiteral("plant_care_schedules"), recordMapToJsonArray(dataset.careSchedules, careScheduleToJson) },
        { QStringLiteral("plant_tags_catalog"), recordMapToJsonArray(dataset.tagCatalog, tagCatalogToJson) },
        { QStringLiteral("reminder_settings"),
          dataset.reminderSettings.exists ? QJsonValue(reminderSettingsToJson(dataset.reminderSettings)) : QJsonValue::Null },
        { QStringLiteral("tombstones"), tombstones }
    };
}

PlantRecord plantFromJson(const QJsonObject &json)
{
    PlantRecord record;
    record.syncUuid = json.value(QStringLiteral("sync_uuid")).toString();
    record.name = json.value(QStringLiteral("name")).toString();
    record.species = json.value(QStringLiteral("species")).toString();
    record.location = json.value(QStringLiteral("location")).toString();
    record.scientificName = json.value(QStringLiteral("scientific_name")).toString();
    record.plantType = json.value(QStringLiteral("plant_type")).toString();
    record.lightRequirement = json.value(QStringLiteral("light_requirement")).toString();
    record.wateringFrequency = json.value(QStringLiteral("watering_frequency")).toString();
    record.wateringNotes = json.value(QStringLiteral("watering_notes")).toString();
    record.humidityPreference = json.value(QStringLiteral("humidity_preference")).toString();
    record.soilType = json.value(QStringLiteral("soil_type")).toString();
    record.lastWatered = json.value(QStringLiteral("last_watered")).toString();
    record.fertilizingSchedule = json.value(QStringLiteral("fertilizing_schedule")).toString();
    record.lastFertilized = json.value(QStringLiteral("last_fertilized")).toString();
    record.pruningTime = json.value(QStringLiteral("pruning_time")).toString();
    record.pruningNotes = json.value(QStringLiteral("pruning_notes")).toString();
    record.lastPruned = json.value(QStringLiteral("last_pruned")).toString();
    record.growthRate = json.value(QStringLiteral("growth_rate")).toString();
    record.issuesPests = json.value(QStringLiteral("issues_pests")).toString();
    record.temperatureTolerance = json.value(QStringLiteral("temperature_tolerance")).toString();
    record.toxicToPets = json.value(QStringLiteral("toxic_to_pets")).toString();
    record.poisonousToHumans = json.value(QStringLiteral("poisonous_to_humans")).toString();
    record.poisonousToPets = json.value(QStringLiteral("poisonous_to_pets")).toString();
    record.indoor = json.value(QStringLiteral("indoor")).toString();
    record.floweringSeason = json.value(QStringLiteral("flowering_season")).toString();
    record.tags = json.value(QStringLiteral("tags")).toString();
    record.acquiredOn = json.value(QStringLiteral("acquired_on")).toString();
    record.source = json.value(QStringLiteral("source")).toString();
    record.notes = json.value(QStringLiteral("notes")).toString();
    record.createdAt = json.value(QStringLiteral("created_at")).toString();
    record.updatedAt = json.value(QStringLiteral("updated_at")).toString();
    return record;
}

JournalEntrySyncRecord journalEntryFromJson(const QJsonObject &json)
{
    JournalEntrySyncRecord record;
    record.syncUuid = json.value(QStringLiteral("sync_uuid")).toString();
    record.plantSyncUuid = json.value(QStringLiteral("plant_sync_uuid")).toString();
    record.entryType = json.value(QStringLiteral("entry_type")).toString();
    record.entryDate = json.value(QStringLiteral("entry_date")).toString();
    record.notes = json.value(QStringLiteral("notes")).toString();
    record.createdAt = json.value(QStringLiteral("created_at")).toString();
    record.updatedAt = json.value(QStringLiteral("updated_at")).toString();
    return record;
}

ReminderSyncRecord reminderFromJson(const QJsonObject &json)
{
    ReminderSyncRecord record;
    record.syncUuid = json.value(QStringLiteral("sync_uuid")).toString();
    record.customName = json.value(QStringLiteral("custom_name")).toString();
    record.taskType = json.value(QStringLiteral("task_type")).toString();
    record.scheduleType = json.value(QStringLiteral("schedule_type")).toString();
    record.intervalDays = json.value(QStringLiteral("interval_days")).toInt();
    record.startDate = json.value(QStringLiteral("start_date")).toString();
    record.nextDueDate = json.value(QStringLiteral("next_due_date")).toString();
    record.notes = json.value(QStringLiteral("notes")).toString();
    record.createdAt = json.value(QStringLiteral("created_at")).toString();
    record.updatedAt = json.value(QStringLiteral("updated_at")).toString();

    const QJsonArray plantSyncUuids = json.value(QStringLiteral("plant_sync_uuids")).toArray();
    for (const QJsonValue &value : plantSyncUuids) {
        const QString plantSyncUuid = value.toString();
        if (!plantSyncUuid.isEmpty()) {
            record.plantSyncUuids.append(plantSyncUuid);
        }
    }
    return record;
}

PlantImageSyncRecord plantImageFromJson(const QJsonObject &json)
{
    PlantImageSyncRecord record;
    record.syncUuid = json.value(QStringLiteral("sync_uuid")).toString();
    record.plantSyncUuid = json.value(QStringLiteral("plant_sync_uuid")).toString();
    record.filePath = json.value(QStringLiteral("file_path")).toString();
    record.createdAt = json.value(QStringLiteral("created_at")).toString();
    record.updatedAt = json.value(QStringLiteral("updated_at")).toString();
    return record;
}

CareScheduleSyncRecord careScheduleFromJson(const QJsonObject &json)
{
    CareScheduleSyncRecord record;
    record.syncUuid = json.value(QStringLiteral("sync_uuid")).toString();
    record.plantSyncUuid = json.value(QStringLiteral("plant_sync_uuid")).toString();
    record.careType = json.value(QStringLiteral("care_type")).toString();
    record.seasonName = json.value(QStringLiteral("season_name")).toString();
    record.intervalDays = json.value(QStringLiteral("interval_days")).toInt();
    record.enabled = json.value(QStringLiteral("is_enabled")).toBool(true);
    record.createdAt = json.value(QStringLiteral("created_at")).toString();
    record.updatedAt = json.value(QStringLiteral("updated_at")).toString();
    return record;
}

ReminderSettingsSyncRecord reminderSettingsFromJson(const QJsonObject &json)
{
    ReminderSettingsSyncRecord record;
    record.exists = !json.isEmpty();
    record.dayBeforeEnabled = json.value(QStringLiteral("day_before_enabled")).toBool(record.dayBeforeEnabled);
    record.dayBeforeTime = json.value(QStringLiteral("day_before_time")).toString(record.dayBeforeTime);
    record.dayOfEnabled = json.value(QStringLiteral("day_of_enabled")).toBool(record.dayOfEnabled);
    record.dayOfTime = json.value(QStringLiteral("day_of_time")).toString(record.dayOfTime);
    record.overdueEnabled = json.value(QStringLiteral("overdue_enabled")).toBool(record.overdueEnabled);
    record.overdueCadenceDays = json.value(QStringLiteral("overdue_cadence_days")).toInt(record.overdueCadenceDays);
    record.overdueTime = json.value(QStringLiteral("overdue_time")).toString(record.overdueTime);
    record.quietHoursStart = json.value(QStringLiteral("quiet_hours_start")).toString();
    record.quietHoursEnd = json.value(QStringLiteral("quiet_hours_end")).toString();
    record.updatedAt = json.value(QStringLiteral("updated_at")).toString();
    return record;
}

TagCatalogSyncRecord tagCatalogFromJson(const QJsonObject &json)
{
    TagCatalogSyncRecord record;
    record.syncUuid = json.value(QStringLiteral("sync_uuid")).toString();
    record.name = json.value(QStringLiteral("name")).toString();
    record.createdAt = json.value(QStringLiteral("created_at")).toString();
    record.updatedAt = json.value(QStringLiteral("updated_at")).toString();
    return record;
}

TombstoneRecord tombstoneFromJson(const QJsonObject &json)
{
    TombstoneRecord record;
    record.entityType = json.value(QStringLiteral("entity_type")).toString();
    record.syncUuid = json.value(QStringLiteral("sync_uuid")).toString();
    record.deletedAt = json.value(QStringLiteral("deleted_at")).toString();
    return record;
}

template <typename RecordMap, typename Parser>
RecordMap jsonArrayToRecordMap(const QJsonArray &array, Parser parser)
{
    RecordMap result;
    for (const QJsonValue &value : array) {
        if (!value.isObject()) {
            continue;
        }
        const auto record = parser(value.toObject());
        if (!record.syncUuid.trimmed().isEmpty()) {
            result.insert(record.syncUuid, record);
        }
    }
    return result;
}

QString buildSyncEndpoint(const QString &baseUrl)
{
    QString normalized = baseUrl.trimmed();
    while (normalized.endsWith(QLatin1Char('/'))) {
        normalized.chop(1);
    }
    return normalized + QStringLiteral("/api/v1/sync");
}

QString buildPingEndpoint(const QString &baseUrl)
{
    QString normalized = baseUrl.trimmed();
    while (normalized.endsWith(QLatin1Char('/'))) {
        normalized.chop(1);
    }
    return normalized + QStringLiteral("/api/v1/ping");
}

QString extractErrorMessage(const QByteArray &payload, const QString &fallback)
{
    QJsonParseError parseError;
    const QJsonDocument json = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !json.isObject()) {
        return fallback;
    }

    const QJsonObject object = json.object();
    const QString detail = object.value(QStringLiteral("detail")).toString();
    if (!detail.isEmpty()) {
        return detail;
    }

    const QString error = object.value(QStringLiteral("error")).toString();
    if (!error.isEmpty()) {
        return error;
    }

    return fallback;
}

bool executeAuthorizedJsonRequest(const QUrl &url,
                                  const QString &deviceToken,
                                  const char *verb,
                                  const QByteArray &requestBody,
                                  QJsonObject *responseObject,
                                  QString *errorMessage)
{
    if (!url.isValid()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Sync server URL is invalid.");
        }
        return false;
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(deviceToken.trimmed()).toUtf8());

    QNetworkAccessManager manager;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);

    QNetworkReply *reply = manager.sendCustomRequest(request, verb, requestBody);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, reply, &QNetworkReply::abort);
    timeout.start(30000);
    loop.exec();
    timeout.stop();

    const QByteArray payload = reply->readAll();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString networkMessage = reply->errorString();
    reply->deleteLater();

    if (networkError != QNetworkReply::NoError && statusCode == 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Request failed (%1): %2. Check the server URL, server host binding, VPN/LAN reachability, and firewall.")
                                .arg(static_cast<int>(networkError))
                                .arg(networkMessage);
        }
        return false;
    }

    if (statusCode < 200 || statusCode >= 300) {
        if (errorMessage) {
            *errorMessage = extractErrorMessage(
                payload,
                QStringLiteral("Sync server returned HTTP %1.").arg(statusCode));
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument responseJson = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !responseJson.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Sync server returned invalid JSON.");
        }
        return false;
    }

    if (responseObject) {
        *responseObject = responseJson.object();
    }
    return true;
}

bool executeSyncRequest(const SyncServerSettings &settings,
                        const QJsonObject &requestObject,
                        QJsonObject *responseObject,
                        QString *errorMessage)
{
    return executeAuthorizedJsonRequest(QUrl(buildSyncEndpoint(settings.baseUrl)),
                                        settings.deviceToken,
                                        "POST",
                                        QJsonDocument(requestObject).toJson(QJsonDocument::Compact),
                                        responseObject,
                                        errorMessage);
}

bool executePingRequest(const SyncServerSettings &settings,
                        QJsonObject *responseObject,
                        QString *errorMessage)
{
    return executeAuthorizedJsonRequest(QUrl(buildPingEndpoint(settings.baseUrl)),
                                        settings.deviceToken,
                                        "GET",
                                        QByteArray(),
                                        responseObject,
                                        errorMessage);
}

bool parseSyncResponse(const QJsonObject &responseObject,
                       QString *newCursor,
                       SyncDataset *serverChanges,
                       QJsonObject *appliedCounts,
                       int *conflictCount,
                       QString *errorMessage)
{
    const QString cursor = responseObject.value(QStringLiteral("new_cursor")).toString();
    if (cursor.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Sync response did not include a new cursor.");
        }
        return false;
    }

    const QJsonObject changesObject = responseObject.value(QStringLiteral("server_changes")).toObject();
    SyncDataset parsedChanges;
    parsedChanges.plants =
        jsonArrayToRecordMap<QMap<QString, PlantRecord>>(changesObject.value(QStringLiteral("plants")).toArray(),
                                                         plantFromJson);
    parsedChanges.journalEntries =
        jsonArrayToRecordMap<QMap<QString, JournalEntrySyncRecord>>(
            changesObject.value(QStringLiteral("journal_entries")).toArray(),
            journalEntryFromJson);
    parsedChanges.reminders =
        jsonArrayToRecordMap<QMap<QString, ReminderSyncRecord>>(
            changesObject.value(QStringLiteral("reminders")).toArray(),
            reminderFromJson);
    parsedChanges.plantImages =
        jsonArrayToRecordMap<QMap<QString, PlantImageSyncRecord>>(
            changesObject.value(QStringLiteral("plant_images")).toArray(),
            plantImageFromJson);
    parsedChanges.careSchedules =
        jsonArrayToRecordMap<QMap<QString, CareScheduleSyncRecord>>(
            changesObject.value(QStringLiteral("plant_care_schedules")).toArray(),
            careScheduleFromJson);
    parsedChanges.tagCatalog =
        jsonArrayToRecordMap<QMap<QString, TagCatalogSyncRecord>>(
            changesObject.value(QStringLiteral("plant_tags_catalog")).toArray(),
            tagCatalogFromJson);

    const QJsonValue reminderSettingsValue = changesObject.value(QStringLiteral("reminder_settings"));
    if (reminderSettingsValue.isObject()) {
        parsedChanges.reminderSettings = reminderSettingsFromJson(reminderSettingsValue.toObject());
    }

    const QJsonArray tombstonesArray = changesObject.value(QStringLiteral("tombstones")).toArray();
    for (const QJsonValue &value : tombstonesArray) {
        if (value.isObject()) {
            parsedChanges.tombstones.append(tombstoneFromJson(value.toObject()));
        }
    }

    if (newCursor) {
        *newCursor = cursor;
    }
    if (serverChanges) {
        *serverChanges = parsedChanges;
    }
    if (appliedCounts) {
        *appliedCounts = responseObject.value(QStringLiteral("applied")).toObject();
    }
    if (conflictCount) {
        *conflictCount = responseObject.value(QStringLiteral("conflicts")).toArray().size();
    }
    return true;
}

QString lookupUpdatedAtBySyncUuid(QSqlDatabase &database,
                                  const QString &tableName,
                                  const QString &syncUuid,
                                  QString *errorMessage)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral("SELECT updated_at FROM %1 WHERE sync_uuid = :sync_uuid;").arg(tableName));
    query.bindValue(QStringLiteral(":sync_uuid"), syncUuid);
    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return {};
    }

    if (!query.next()) {
        return {};
    }

    return query.value(0).toString();
}

bool applyServerTombstone(QSqlDatabase &database,
                          const TombstoneRecord &record,
                          QString *errorMessage)
{
    if (record.entityType.trimmed().isEmpty() || record.syncUuid.trimmed().isEmpty()) {
        return true;
    }

    const QString existingUpdatedAt =
        lookupUpdatedAtBySyncUuid(database, record.entityType, record.syncUuid, errorMessage);
    if (errorMessage && !errorMessage->isEmpty()) {
        return false;
    }

    if (!existingUpdatedAt.isEmpty()) {
        if (compareTimestamps(record.deletedAt, existingUpdatedAt) > 0) {
            return deleteBySyncUuid(database, record.entityType, record.syncUuid, record.deletedAt, errorMessage);
        }
        return true;
    }

    return upsertTombstone(database, record.entityType, record.syncUuid, record.deletedAt, errorMessage);
}

bool applyServerChanges(QSqlDatabase &database,
                        const SyncDataset &dataset,
                        SyncCounters *counters,
                        QString *errorMessage)
{
    for (auto it = dataset.plants.cbegin(); it != dataset.plants.cend(); ++it) {
        if (!upsertPlant(database, it.value(), errorMessage)) {
            return false;
        }
        if (counters) {
            ++counters->pulled;
        }
    }

    for (auto it = dataset.journalEntries.cbegin(); it != dataset.journalEntries.cend(); ++it) {
        if (!upsertJournalEntry(database, it.value(), errorMessage)) {
            return false;
        }
        if (counters) {
            ++counters->pulled;
        }
    }

    for (auto it = dataset.reminders.cbegin(); it != dataset.reminders.cend(); ++it) {
        if (!upsertReminder(database, it.value(), errorMessage)) {
            return false;
        }
        if (counters) {
            ++counters->pulled;
        }
    }

    for (auto it = dataset.plantImages.cbegin(); it != dataset.plantImages.cend(); ++it) {
        if (!upsertPlantImage(database, it.value(), errorMessage)) {
            return false;
        }
        if (counters) {
            ++counters->pulled;
        }
    }

    for (auto it = dataset.careSchedules.cbegin(); it != dataset.careSchedules.cend(); ++it) {
        if (!upsertCareSchedule(database, it.value(), errorMessage)) {
            return false;
        }
        if (counters) {
            ++counters->pulled;
        }
    }

    for (auto it = dataset.tagCatalog.cbegin(); it != dataset.tagCatalog.cend(); ++it) {
        if (!upsertTagCatalog(database, it.value(), errorMessage)) {
            return false;
        }
        if (counters) {
            ++counters->pulled;
        }
    }

    if (dataset.reminderSettings.exists) {
        if (!upsertReminderSettings(database, dataset.reminderSettings, errorMessage)) {
            return false;
        }
        if (counters) {
            ++counters->pulled;
        }
    }

    for (const TombstoneRecord &record : dataset.tombstones) {
        if (!applyServerTombstone(database, record, errorMessage)) {
            return false;
        }
        if (counters) {
            ++counters->deleted;
        }
    }

    return true;
}

bool clearLocalSyncData(QSqlDatabase &database, QString *errorMessage)
{
    const QStringList statements = {
        QStringLiteral("DELETE FROM reminder_plants;"),
        QStringLiteral("DELETE FROM journal_entries;"),
        QStringLiteral("DELETE FROM reminders;"),
        QStringLiteral("DELETE FROM plant_images;"),
        QStringLiteral("DELETE FROM plant_care_schedules;"),
        QStringLiteral("DELETE FROM plant_tags_catalog;"),
        QStringLiteral("DELETE FROM sync_tombstones;"),
        QStringLiteral("DELETE FROM reminder_settings;"),
        QStringLiteral("DELETE FROM plants;"),
        QStringLiteral(
            "DELETE FROM sqlite_sequence WHERE name IN ("
            "'plants', 'journal_entries', 'reminders', 'plant_images', 'plant_care_schedules'"
            ");"),
        QStringLiteral(
            "INSERT INTO reminder_settings (id, updated_at) "
            "VALUES (1, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'));")
    };

    for (const QString &statement : statements) {
        if (!execStatement(database, statement, errorMessage)) {
            return false;
        }
    }
    return true;
}

int sumJsonCounts(const QJsonObject &countsObject)
{
    int sum = 0;
    for (auto it = countsObject.begin(); it != countsObject.end(); ++it) {
        sum += it.value().toInt();
    }
    return sum;
}
}

DatabaseSyncService::DatabaseSyncService(QSqlDatabase localDatabase)
    : m_localDatabase(localDatabase)
{
}

bool DatabaseSyncService::testConnection(const SyncServerSettings &settings, QString *message)
{
    if (!settings.isConfigured()) {
        if (message) {
            *message = QStringLiteral("Sync server URL and device token are required.");
        }
        return false;
    }

    QJsonObject responseObject;
    QString requestError;
    if (!executePingRequest(settings, &responseObject, &requestError)) {
        if (message) {
            *message = requestError;
        }
        return false;
    }

    const QString status = responseObject.value(QStringLiteral("status")).toString();
    if (status != QStringLiteral("ok")) {
        if (message) {
            *message = QStringLiteral("Sync server returned an unexpected ping response.");
        }
        return false;
    }

    if (message) {
        const int accountId = responseObject.value(QStringLiteral("account_id")).toInt();
        *message = QStringLiteral("Connection test passed. Server reachable and device token accepted for account %1.")
                       .arg(accountId);
    }
    return true;
}

bool DatabaseSyncService::clearLocalDatabase(QString *errorMessage)
{
    if (!m_localDatabase.isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Local SQLite database is not open.");
        }
        return false;
    }

    if (!m_localDatabase.transaction()) {
        if (errorMessage) {
            *errorMessage = m_localDatabase.lastError().text();
        }
        return false;
    }

    QString clearError;
    if (!clearLocalSyncData(m_localDatabase, &clearError)) {
        m_localDatabase.rollback();
        if (errorMessage) {
            *errorMessage = clearError.isEmpty() ? QStringLiteral("Failed to clear local data.") : clearError;
        }
        return false;
    }

    if (!m_localDatabase.commit()) {
        m_localDatabase.rollback();
        if (errorMessage) {
            *errorMessage = m_localDatabase.lastError().text();
        }
        return false;
    }

    return true;
}

bool DatabaseSyncService::forcePull(SyncServerSettings *settings,
                                    QString *summary,
                                    QString *errorMessage)
{
    if (!settings) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Sync settings are missing.");
        }
        return false;
    }

    if (!m_localDatabase.isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Local SQLite database is not open.");
        }
        return false;
    }

    if (!settings->isConfigured()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Sync server URL and device token are required.");
        }
        return false;
    }

    const QJsonObject requestObject = {
        { QStringLiteral("client_id"), settings->clientId.trimmed() },
        { QStringLiteral("last_sync_cursor"), QJsonValue::Null },
        { QStringLiteral("changes"), datasetToJson(SyncDataset()) }
    };

    QJsonObject responseObject;
    QString syncError;
    if (!executeSyncRequest(*settings, requestObject, &responseObject, &syncError)) {
        if (errorMessage) {
            *errorMessage = syncError;
        }
        return false;
    }

    QString newCursor;
    SyncDataset serverChanges;
    QJsonObject appliedCounts;
    int conflictCount = 0;
    if (!parseSyncResponse(responseObject,
                           &newCursor,
                           &serverChanges,
                           &appliedCounts,
                           &conflictCount,
                           &syncError)) {
        if (errorMessage) {
            *errorMessage = syncError;
        }
        return false;
    }

    if (!m_localDatabase.transaction()) {
        if (errorMessage) {
            *errorMessage = m_localDatabase.lastError().text();
        }
        return false;
    }

    SyncCounters counters;
    if (!clearLocalSyncData(m_localDatabase, &syncError)) {
        m_localDatabase.rollback();
        if (errorMessage) {
            *errorMessage = syncError.isEmpty() ? QStringLiteral("Failed to clear local data.") : syncError;
        }
        return false;
    }

    if (!applyServerChanges(m_localDatabase, serverChanges, &counters, &syncError)) {
        m_localDatabase.rollback();
        if (errorMessage) {
            *errorMessage = syncError.isEmpty() ? QStringLiteral("Failed to apply server changes.") : syncError;
        }
        return false;
    }

    if (!m_localDatabase.commit()) {
        m_localDatabase.rollback();
        if (errorMessage) {
            *errorMessage = m_localDatabase.lastError().text();
        }
        return false;
    }

    settings->lastSyncCursor = newCursor;

    if (summary) {
        *summary = QStringLiteral("Force pull completed. Local data was replaced with %1 downloaded change(s); %2 conflict(s) reported by the server.")
                       .arg(counters.pulled + counters.deleted)
                       .arg(conflictCount);
    }

    return true;
}

bool DatabaseSyncService::synchronize(SyncServerSettings *settings,
                                      QString *summary,
                                      QString *errorMessage)
{
    if (!settings) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Sync settings are missing.");
        }
        return false;
    }

    if (!m_localDatabase.isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Local SQLite database is not open.");
        }
        return false;
    }

    if (!settings->isConfigured()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Sync server URL and device token are required.");
        }
        return false;
    }

    QString syncError;
    SyncDataset localChanges = collectLocalChanges(m_localDatabase, settings->lastSyncCursor, &syncError);
    if (!syncError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = syncError;
        }
        return false;
    }

    const QJsonObject requestObject = {
        { QStringLiteral("client_id"), settings->clientId.trimmed() },
        { QStringLiteral("last_sync_cursor"),
          settings->lastSyncCursor.trimmed().isEmpty() ? QJsonValue::Null : QJsonValue(settings->lastSyncCursor) },
        { QStringLiteral("changes"), datasetToJson(localChanges) }
    };

    QJsonObject responseObject;
    if (!executeSyncRequest(*settings, requestObject, &responseObject, &syncError)) {
        if (errorMessage) {
            *errorMessage = syncError;
        }
        return false;
    }

    QString newCursor;
    SyncDataset serverChanges;
    QJsonObject appliedCounts;
    int conflictCount = 0;
    if (!parseSyncResponse(responseObject,
                           &newCursor,
                           &serverChanges,
                           &appliedCounts,
                           &conflictCount,
                           &syncError)) {
        if (errorMessage) {
            *errorMessage = syncError;
        }
        return false;
    }

    if (!m_localDatabase.transaction()) {
        if (errorMessage) {
            *errorMessage = m_localDatabase.lastError().text();
        }
        return false;
    }

    SyncCounters counters;
    if (!applyServerChanges(m_localDatabase, serverChanges, &counters, &syncError)) {
        m_localDatabase.rollback();
        if (errorMessage) {
            *errorMessage = syncError.isEmpty() ? QStringLiteral("Failed to apply server changes.") : syncError;
        }
        return false;
    }

    if (!m_localDatabase.commit()) {
        m_localDatabase.rollback();
        if (errorMessage) {
            *errorMessage = m_localDatabase.lastError().text();
        }
        return false;
    }

    settings->lastSyncCursor = newCursor;

    if (summary) {
        *summary = QStringLiteral("Synchronization finished. Uploaded %1 change(s), downloaded %2 change(s), resolved %3 conflict(s).")
            .arg(sumJsonCounts(appliedCounts))
            .arg(counters.pulled + counters.deleted)
            .arg(conflictCount);
    }

    return true;
}
