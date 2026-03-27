#include "PlantListViewModel.h"

#include <QDate>
#include <QDebug>
#include <QFileInfo>
#include <QPair>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QUrl>

namespace {
struct PlantImageRecord
{
    int id = 0;
    int plantId = 0;
    QString filePath;
    QString createdAt;
};

struct CareScheduleRecord
{
    int plantId = 0;
    QString careType;
    QString seasonName;
    int intervalDays = 0;
    bool enabled = true;
};

QStringList careSeasons()
{
    return {
        QStringLiteral("winter"),
        QStringLiteral("spring"),
        QStringLiteral("summer"),
        QStringLiteral("autumn")
    };
}

QString currentSeasonName(const QDate &date = QDate::currentDate())
{
    const int month = date.month();
    if (month == 12 || month <= 2) {
        return QStringLiteral("winter");
    }
    if (month <= 5) {
        return QStringLiteral("spring");
    }
    if (month <= 8) {
        return QStringLiteral("summer");
    }
    return QStringLiteral("autumn");
}

QString displaySeasonName(const QString &seasonName)
{
    if (seasonName.isEmpty()) {
        return QString();
    }
    QString label = seasonName;
    label[0] = label.at(0).toUpper();
    return label;
}

QString normalizeLocalPath(const QString &pathOrUrl)
{
    const QUrl url(pathOrUrl);
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }
    return pathOrUrl.trimmed();
}

QString dateToString(const QDate &date)
{
    return date.isValid() ? date.toString(Qt::ISODate) : QString();
}

QStringList parseTagList(const QString &value)
{
    const QStringList rawParts = value.split(QLatin1Char(','), Qt::SkipEmptyParts);
    QStringList tags;
    for (const QString &rawPart : rawParts) {
        const QString tag = rawPart.trimmed();
        if (!tag.isEmpty() && !tags.contains(tag, Qt::CaseInsensitive)) {
            tags.append(tag);
        }
    }
    return tags;
}

QString normalizedTagsString(const QString &value)
{
    return parseTagList(value).join(QStringLiteral(", "));
}

bool plantMatchesTagFilter(const TPlant &plant, const QString &filter)
{
    const QString normalizedFilter = filter.trimmed().toLower();
    if (normalizedFilter.isEmpty()) {
        return true;
    }

    const QStringList requiredTags = parseTagList(normalizedFilter);
    if (requiredTags.isEmpty()) {
        return true;
    }

    const QStringList plantTags = parseTagList(plant.tags.toLower());
    for (const QString &requiredTag : requiredTags) {
        bool matched = false;
        for (const QString &plantTag : plantTags) {
            if (plantTag.contains(requiredTag, Qt::CaseInsensitive)) {
                matched = true;
                break;
            }
        }
        if (!matched) {
            return false;
        }
    }

    return true;
}

QDate stringToDate(const QVariant &value)
{
    const QString text = value.toString();
    return text.isEmpty() ? QDate() : QDate::fromString(text, Qt::ISODate);
}

int extractIntervalDays(const QString &text)
{
    const QRegularExpression matchExpr(QStringLiteral("(\\d{1,3})"));
    const QRegularExpressionMatch match = matchExpr.match(text);
    if (!match.hasMatch()) {
        return 0;
    }
    return match.captured(1).toInt();
}

QString formatCareSummary(int defaultIntervalDays, const QMap<QString, CareScheduleRecord> &seasonalOverrides)
{
    if (defaultIntervalDays <= 0 && seasonalOverrides.isEmpty()) {
        return QStringLiteral("Not set");
    }

    QString summary = defaultIntervalDays > 0
        ? QStringLiteral("Every %1 days").arg(defaultIntervalDays)
        : QStringLiteral("No default interval");

    if (!seasonalOverrides.isEmpty()) {
        summary += QStringLiteral("; seasonal overrides");
    }
    return summary;
}

bool ensurePlantsTableExists(QSqlDatabase &db, QString *error = nullptr)
{
    if (!db.isOpen()) {
        if (error) {
            *error = QStringLiteral("Database is not open.");
        }
        return false;
    }

    QSqlQuery existsQuery(db);
    if (existsQuery.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='plants';")
        && existsQuery.next()) {
        return true;
    }

    QSqlQuery createQuery(db);
    const QString createSql =
        "CREATE TABLE IF NOT EXISTS plants ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "species TEXT,"
        "location TEXT,"
        "scientific_name TEXT,"
        "plant_type TEXT,"
        "light_requirement TEXT,"
        "watering_frequency TEXT,"
        "watering_notes TEXT,"
        "humidity_preference TEXT,"
        "soil_type TEXT,"
        "last_watered TEXT,"
        "fertilizing_schedule TEXT,"
        "last_fertilized TEXT,"
        "pruning_time TEXT,"
        "pruning_notes TEXT,"
        "last_pruned TEXT,"
        "growth_rate TEXT,"
        "issues_pests TEXT,"
        "temperature_tolerance TEXT,"
        "toxic_to_pets TEXT,"
        "poisonous_to_humans TEXT,"
        "poisonous_to_pets TEXT,"
        "indoor TEXT,"
        "flowering_season TEXT,"
        "tags TEXT,"
        "acquired_on TEXT,"
        "source TEXT,"
        "notes TEXT,"
        "created_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "updated_at TEXT NOT NULL DEFAULT (datetime('now'))"
        ");";

    if (!createQuery.exec(createSql)) {
        if (error) {
            *error = createQuery.lastError().text();
        }
        return false;
    }

    return true;
}

bool ensurePlantImagesTableExists(QSqlDatabase &db, QString *error = nullptr)
{
    if (!db.isOpen()) {
        if (error) {
            *error = QStringLiteral("Database is not open.");
        }
        return false;
    }

    QSqlQuery query(db);
    const QString createSql =
        "CREATE TABLE IF NOT EXISTS plant_images ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "plant_id INTEGER NOT NULL,"
        "file_path TEXT NOT NULL,"
        "created_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "FOREIGN KEY (plant_id) REFERENCES plants(id) ON DELETE CASCADE,"
        "UNIQUE (plant_id, file_path)"
        ");";
    if (!query.exec(createSql)) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }

    QSqlQuery indexQuery(db);
    if (!indexQuery.exec(
            "CREATE INDEX IF NOT EXISTS idx_plant_images_plant_id "
            "ON plant_images(plant_id, created_at, id);")) {
        if (error) {
            *error = indexQuery.lastError().text();
        }
        return false;
    }

    return true;
}

bool ensurePlantCareSchedulesTableExists(QSqlDatabase &db, QString *error = nullptr)
{
    if (!db.isOpen()) {
        if (error) {
            *error = QStringLiteral("Database is not open.");
        }
        return false;
    }

    QSqlQuery query(db);
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS plant_care_schedules ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "plant_id INTEGER NOT NULL,"
            "care_type TEXT NOT NULL,"
            "season_name TEXT NOT NULL,"
            "interval_days INTEGER,"
            "is_enabled INTEGER NOT NULL DEFAULT 1,"
            "created_at TEXT NOT NULL DEFAULT (datetime('now')),"
            "updated_at TEXT NOT NULL DEFAULT (datetime('now')),"
            "FOREIGN KEY (plant_id) REFERENCES plants(id) ON DELETE CASCADE,"
            "UNIQUE (plant_id, care_type, season_name)"
            ");")) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }

    QSqlQuery indexQuery(db);
    if (!indexQuery.exec(
            "CREATE INDEX IF NOT EXISTS idx_plant_care_schedules_lookup "
            "ON plant_care_schedules(plant_id, care_type, season_name);")) {
        if (error) {
            *error = indexQuery.lastError().text();
        }
        return false;
    }

    return true;
}

TPlant mapPlantRow(const QSqlQuery &query)
{
    TPlant plant;
    plant.id = query.value(0).toInt();
    plant.name = query.value(1).toString();
    plant.scientificName = query.value(2).toString();
    plant.plantType = query.value(3).toString();
    plant.lightRequirement = query.value(4).toString();
    plant.wateringFrequency = query.value(5).toString();
    plant.wateringNotes = query.value(6).toString();
    plant.humidityPreference = query.value(7).toString();
    plant.soilType = query.value(8).toString();
    plant.lastWatered = stringToDate(query.value(9));
    plant.fertilizingSchedule = query.value(10).toString();
    plant.lastFertilized = stringToDate(query.value(11));
    plant.pruningTime = query.value(12).toString();
    plant.pruningNotes = query.value(13).toString();
    plant.lastPruned = stringToDate(query.value(14));
    plant.growthRate = query.value(15).toString();
    plant.issuesPests = query.value(16).toString();
    plant.temperatureTolerance = query.value(17).toString();
    plant.toxicToPets = query.value(18).toString();
    plant.poisonousToHumans = query.value(19).toString();
    plant.poisonousToPets = query.value(20).toString();
    plant.indoor = query.value(21).toString();
    plant.floweringSeason = query.value(22).toString();
    plant.tags = query.value(23).toString();
    plant.acquiredDate = stringToDate(query.value(24));
    plant.source = query.value(25).toString();
    plant.notes = query.value(26).toString();
    return plant;
}

PlantImageRecord mapPlantImageRow(const QSqlQuery &query)
{
    PlantImageRecord record;
    record.id = query.value(0).toInt();
    record.plantId = query.value(1).toInt();
    record.filePath = query.value(2).toString();
    record.createdAt = query.value(3).toString();
    return record;
}

CareScheduleRecord mapCareScheduleRow(const QSqlQuery &query)
{
    CareScheduleRecord record;
    record.plantId = query.value(0).toInt();
    record.careType = query.value(1).toString();
    record.seasonName = query.value(2).toString();
    record.intervalDays = query.value(3).toInt();
    record.enabled = query.value(4).toInt() != 0;
    return record;
}

QVector<TPlant> loadPlants(QSqlDatabase &db, QString *error = nullptr)
{
    QVector<TPlant> plants;
    if (!ensurePlantsTableExists(db, error)) {
        return plants;
    }

    QSqlQuery query(db);
    if (!query.exec(
            "SELECT id, name, scientific_name, plant_type, "
            "light_requirement, watering_frequency, watering_notes, humidity_preference, "
            "soil_type, "
            "COALESCE(("
            "    SELECT MAX(entry_date) FROM journal_entries je "
            "    WHERE je.plant_id = plants.id AND lower(je.entry_type) = 'water'"
            "), last_watered), "
            "fertilizing_schedule, "
            "COALESCE(("
            "    SELECT MAX(entry_date) FROM journal_entries je "
            "    WHERE je.plant_id = plants.id AND lower(je.entry_type) = 'fertilize'"
            "), last_fertilized), "
            "pruning_time, pruning_notes, last_pruned, growth_rate, "
            "issues_pests, temperature_tolerance, toxic_to_pets, poisonous_to_humans, "
            "poisonous_to_pets, indoor, flowering_season, tags, acquired_on, source, notes "
            "FROM plants ORDER BY name COLLATE NOCASE;")) {
        if (error) {
            *error = query.lastError().text();
        }
        return plants;
    }

    while (query.next()) {
        plants.append(mapPlantRow(query));
    }

    return plants;
}

QVector<PlantImageRecord> loadPlantImages(QSqlDatabase &db, int plantId, QString *error = nullptr)
{
    QVector<PlantImageRecord> images;
    if (plantId <= 0) {
        return images;
    }

    if (!ensurePlantImagesTableExists(db, error)) {
        return images;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT id, plant_id, file_path, created_at "
        "FROM plant_images "
        "WHERE plant_id = :plant_id "
        "ORDER BY created_at ASC, id ASC;");
    query.bindValue(":plant_id", plantId);
    if (!query.exec()) {
        if (error) {
            *error = query.lastError().text();
        }
        return images;
    }

    while (query.next()) {
        images.append(mapPlantImageRow(query));
    }
    return images;
}

QVector<CareScheduleRecord> loadCareScheduleRows(QSqlDatabase &db, int plantId, QString *error = nullptr)
{
    QVector<CareScheduleRecord> rows;
    if (plantId <= 0) {
        return rows;
    }

    if (!ensurePlantCareSchedulesTableExists(db, error)) {
        return rows;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT plant_id, care_type, season_name, interval_days, is_enabled "
        "FROM plant_care_schedules "
        "WHERE plant_id = :plant_id "
        "ORDER BY care_type, season_name;");
    query.bindValue(":plant_id", plantId);
    if (!query.exec()) {
        if (error) {
            *error = query.lastError().text();
        }
        return rows;
    }

    while (query.next()) {
        rows.append(mapCareScheduleRow(query));
    }
    return rows;
}

QString loadPrimaryPlantImagePath(QSqlDatabase &db, int plantId, QString *error = nullptr)
{
    if (plantId <= 0) {
        return {};
    }

    if (!ensurePlantImagesTableExists(db, error)) {
        return {};
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT file_path "
        "FROM plant_images "
        "WHERE plant_id = :plant_id "
        "ORDER BY created_at ASC, id ASC "
        "LIMIT 1;");
    query.bindValue(":plant_id", plantId);
    if (!query.exec()) {
        if (error) {
            *error = query.lastError().text();
        }
        return {};
    }

    if (!query.next()) {
        return {};
    }
    return query.value(0).toString();
}

bool insertPlant(QSqlDatabase &db, const TPlant &plant, int *insertedId = nullptr, QString *error = nullptr)
{
    if (!ensurePlantsTableExists(db, error)) {
        return false;
    }

    QSqlQuery query(db);
    if (!query.prepare(
            "INSERT INTO plants ("
            "name, species, location, scientific_name, plant_type, "
            "light_requirement, watering_frequency, watering_notes, humidity_preference, soil_type, "
            "last_watered, fertilizing_schedule, last_fertilized, pruning_time, pruning_notes, last_pruned, "
            "growth_rate, issues_pests, temperature_tolerance, toxic_to_pets, poisonous_to_humans, "
            "poisonous_to_pets, indoor, flowering_season, tags, "
            "acquired_on, source, notes"
            ") VALUES ("
            "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?"
            ");")) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }

    query.addBindValue(plant.name);
    query.addBindValue(QString());
    query.addBindValue(QString());
    query.addBindValue(plant.scientificName);
    query.addBindValue(plant.plantType);
    query.addBindValue(plant.lightRequirement);
    query.addBindValue(plant.wateringFrequency);
    query.addBindValue(plant.wateringNotes);
    query.addBindValue(plant.humidityPreference);
    query.addBindValue(plant.soilType);
    query.addBindValue(dateToString(plant.lastWatered));
    query.addBindValue(plant.fertilizingSchedule);
    query.addBindValue(dateToString(plant.lastFertilized));
    query.addBindValue(plant.pruningTime);
    query.addBindValue(plant.pruningNotes);
    query.addBindValue(dateToString(plant.lastPruned));
    query.addBindValue(plant.growthRate);
    query.addBindValue(plant.issuesPests);
    query.addBindValue(plant.temperatureTolerance);
    query.addBindValue(plant.toxicToPets);
    query.addBindValue(plant.poisonousToHumans);
    query.addBindValue(plant.poisonousToPets);
    query.addBindValue(plant.indoor);
    query.addBindValue(plant.floweringSeason);
    query.addBindValue(plant.tags);
    query.addBindValue(dateToString(plant.acquiredDate));
    query.addBindValue(plant.source);
    query.addBindValue(plant.notes);

    if (!query.exec()) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }

    if (insertedId) {
        *insertedId = query.lastInsertId().toInt();
    }
    return true;
}

bool insertPlantImageRow(QSqlDatabase &db, int plantId, const QString &filePath, QString *error = nullptr)
{
    if (plantId <= 0) {
        if (error) {
            *error = QStringLiteral("Select a plant first.");
        }
        return false;
    }

    if (!ensurePlantImagesTableExists(db, error)) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare(
        "INSERT OR IGNORE INTO plant_images (plant_id, file_path) "
        "VALUES (:plant_id, :file_path);");
    query.bindValue(":plant_id", plantId);
    query.bindValue(":file_path", filePath);
    if (!query.exec()) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }

    return true;
}

bool replaceCareScheduleRows(QSqlDatabase &db,
                             int plantId,
                             const QVector<CareScheduleRecord> &rows,
                             const QString &wateringSummary,
                             const QString &fertilizingSummary,
                             QString *error = nullptr)
{
    if (plantId <= 0) {
        if (error) {
            *error = QStringLiteral("Select a plant first.");
        }
        return false;
    }

    if (!ensurePlantCareSchedulesTableExists(db, error)) {
        return false;
    }

    const bool inTransaction = db.transaction();

    QSqlQuery deleteQuery(db);
    deleteQuery.prepare("DELETE FROM plant_care_schedules WHERE plant_id = :plant_id;");
    deleteQuery.bindValue(":plant_id", plantId);
    if (!deleteQuery.exec()) {
        if (inTransaction) {
            db.rollback();
        }
        if (error) {
            *error = deleteQuery.lastError().text();
        }
        return false;
    }

    QSqlQuery insertQuery(db);
    insertQuery.prepare(
        "INSERT INTO plant_care_schedules ("
        "plant_id, care_type, season_name, interval_days, is_enabled, updated_at"
        ") VALUES ("
        ":plant_id, :care_type, :season_name, :interval_days, :is_enabled, datetime('now')"
        ");");

    for (const CareScheduleRecord &row : rows) {
        insertQuery.bindValue(":plant_id", plantId);
        insertQuery.bindValue(":care_type", row.careType);
        insertQuery.bindValue(":season_name", row.seasonName);
        insertQuery.bindValue(":interval_days", row.enabled && row.intervalDays > 0 ? QVariant(row.intervalDays)
                                                                                    : QVariant());
        insertQuery.bindValue(":is_enabled", row.enabled ? 1 : 0);
        if (!insertQuery.exec()) {
            if (inTransaction) {
                db.rollback();
            }
            if (error) {
                *error = insertQuery.lastError().text();
            }
            return false;
        }
    }

    QSqlQuery updatePlant(db);
    updatePlant.prepare(
        "UPDATE plants "
        "SET watering_frequency = :watering_frequency, "
        "    fertilizing_schedule = :fertilizing_schedule, "
        "    updated_at = datetime('now') "
        "WHERE id = :plant_id;");
    updatePlant.bindValue(":watering_frequency", wateringSummary);
    updatePlant.bindValue(":fertilizing_schedule", fertilizingSummary);
    updatePlant.bindValue(":plant_id", plantId);
    if (!updatePlant.exec()) {
        if (inTransaction) {
            db.rollback();
        }
        if (error) {
            *error = updatePlant.lastError().text();
        }
        return false;
    }

    if (inTransaction && !db.commit()) {
        if (error) {
            *error = db.lastError().text();
        }
        return false;
    }

    return true;
}

bool updatePlantRow(QSqlDatabase &db, const TPlant &plant, QString *error = nullptr)
{
    QSqlQuery query(db);
    query.prepare(
        "UPDATE plants "
        "SET name = :name, "
        "    scientific_name = :scientific_name, "
        "    plant_type = :plant_type, "
        "    light_requirement = :light_requirement, "
        "    watering_frequency = :watering_frequency, "
        "    watering_notes = :watering_notes, "
        "    humidity_preference = :humidity_preference, "
        "    soil_type = :soil_type, "
        "    last_watered = :last_watered, "
        "    fertilizing_schedule = :fertilizing_schedule, "
        "    last_fertilized = :last_fertilized, "
        "    pruning_time = :pruning_time, "
        "    pruning_notes = :pruning_notes, "
        "    last_pruned = :last_pruned, "
        "    growth_rate = :growth_rate, "
        "    issues_pests = :issues_pests, "
        "    temperature_tolerance = :temperature_tolerance, "
        "    toxic_to_pets = :toxic_to_pets, "
        "    poisonous_to_humans = :poisonous_to_humans, "
        "    poisonous_to_pets = :poisonous_to_pets, "
        "    indoor = :indoor, "
        "    flowering_season = :flowering_season, "
        "    tags = :tags, "
        "    acquired_on = :acquired_on, "
        "    source = :source, "
        "    notes = :notes, "
        "    updated_at = datetime('now') "
        "WHERE id = :id;"
    );
    query.bindValue(":name", plant.name);
    query.bindValue(":scientific_name", plant.scientificName);
    query.bindValue(":plant_type", plant.plantType);
    query.bindValue(":light_requirement", plant.lightRequirement);
    query.bindValue(":watering_frequency", plant.wateringFrequency);
    query.bindValue(":watering_notes", plant.wateringNotes);
    query.bindValue(":humidity_preference", plant.humidityPreference);
    query.bindValue(":soil_type", plant.soilType);
    query.bindValue(":last_watered", dateToString(plant.lastWatered));
    query.bindValue(":fertilizing_schedule", plant.fertilizingSchedule);
    query.bindValue(":last_fertilized", dateToString(plant.lastFertilized));
    query.bindValue(":pruning_time", plant.pruningTime);
    query.bindValue(":pruning_notes", plant.pruningNotes);
    query.bindValue(":last_pruned", dateToString(plant.lastPruned));
    query.bindValue(":growth_rate", plant.growthRate);
    query.bindValue(":issues_pests", plant.issuesPests);
    query.bindValue(":temperature_tolerance", plant.temperatureTolerance);
    query.bindValue(":toxic_to_pets", plant.toxicToPets);
    query.bindValue(":poisonous_to_humans", plant.poisonousToHumans);
    query.bindValue(":poisonous_to_pets", plant.poisonousToPets);
    query.bindValue(":indoor", plant.indoor);
    query.bindValue(":flowering_season", plant.floweringSeason);
    query.bindValue(":tags", plant.tags);
    query.bindValue(":acquired_on", dateToString(plant.acquiredDate));
    query.bindValue(":source", plant.source);
    query.bindValue(":notes", plant.notes);
    query.bindValue(":id", plant.id);

    if (!query.exec()) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }

    return true;
}

bool removePlantImageRow(QSqlDatabase &db, int imageId, QString *error = nullptr)
{
    if (imageId <= 0) {
        if (error) {
            *error = QStringLiteral("Select an image to delete.");
        }
        return false;
    }

    if (!ensurePlantImagesTableExists(db, error)) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare("DELETE FROM plant_images WHERE id = :id;");
    query.bindValue(":id", imageId);
    if (!query.exec()) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }

    return true;
}

bool removePlantRow(QSqlDatabase &db, int id, QString *error = nullptr)
{
    QSqlQuery query(db);
    query.prepare("DELETE FROM plants WHERE id = :id;");
    query.bindValue(":id", id);
    if (!query.exec()) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }
    return true;
}

bool replacePlantsFromSource(QSqlDatabase &targetDb, const QVector<TPlant> &plants, QString *error = nullptr)
{
    if (!ensurePlantsTableExists(targetDb, error)) {
        return false;
    }

    const bool inTransaction = targetDb.transaction();
    QSqlQuery deleteQuery(targetDb);
    if (!deleteQuery.exec("DELETE FROM plants;")) {
        if (inTransaction) {
            targetDb.rollback();
        }
        if (error) {
            *error = deleteQuery.lastError().text();
        }
        return false;
    }

    for (const TPlant &plant : plants) {
        if (!insertPlant(targetDb, plant, nullptr, error)) {
            if (inTransaction) {
                targetDb.rollback();
            }
            return false;
        }
    }

    if (inTransaction && !targetDb.commit()) {
        if (error) {
            *error = targetDb.lastError().text();
        }
        return false;
    }

    return true;
}

QVector<TPlant> loadPlantsFromSqliteFile(const QString &databasePath, QString *error)
{
    QVector<TPlant> plants;
    const QString connectionName =
        QStringLiteral("plant_import_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(databasePath);
        if (!db.open()) {
            if (error) {
                *error = db.lastError().text();
            }
            return plants;
        }

        plants = loadPlants(db, error);
        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
    return plants;
}
} // namespace

PlantListViewModel::PlantListViewModel(QSqlDatabase db, QObject *parent)
    : QAbstractListModel(parent)
    , m_db(db)
{
    refresh();
}

int PlantListViewModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_plants.size();
}

QVariant PlantListViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_plants.size()) {
        return {};
    }

    const TPlant &plant = m_plants.at(index.row())->data();
    switch (role) {
    case IdRole:
        return plant.id;
    case NameRole:
        return plant.name;
    case ScientificNameRole:
        return plant.scientificName;
    case PlantTypeRole:
        return plant.plantType;
    case LightRequirementRole:
        return plant.lightRequirement;
    case WateringFrequencyRole:
        return plant.wateringFrequency;
    case WateringNotesRole:
        return plant.wateringNotes;
    case HumidityPreferenceRole:
        return plant.humidityPreference;
    case SoilTypeRole:
        return plant.soilType;
    case LastWateredRole:
        return plant.lastWatered.isValid() ? plant.lastWatered.toString(Qt::ISODate) : QString();
    case FertilizingScheduleRole:
        return plant.fertilizingSchedule;
    case LastFertilizedRole:
        return plant.lastFertilized.isValid() ? plant.lastFertilized.toString(Qt::ISODate) : QString();
    case PruningTimeRole:
        return plant.pruningTime;
    case PruningNotesRole:
        return plant.pruningNotes;
    case LastPrunedRole:
        return plant.lastPruned.isValid() ? plant.lastPruned.toString(Qt::ISODate) : QString();
    case GrowthRateRole:
        return plant.growthRate;
    case IssuesPestsRole:
        return plant.issuesPests;
    case TemperatureToleranceRole:
        return plant.temperatureTolerance;
    case ToxicToPetsRole:
        return plant.toxicToPets;
    case PoisonousToHumansRole:
        return plant.poisonousToHumans;
    case PoisonousToPetsRole:
        return plant.poisonousToPets;
    case IndoorRole:
        return plant.indoor;
    case FloweringSeasonRole:
        return plant.floweringSeason;
    case TagsRole:
        return plant.tags;
    case AcquiredDateRole:
        return plant.acquiredDate.isValid() ? plant.acquiredDate.toString(Qt::ISODate) : QString();
    case SourceRole:
        return plant.source;
    case NotesRole:
        return plant.notes;
    case ImageSourceRole: {
        QSqlDatabase db = m_db;
        return toUrl(loadPrimaryPlantImagePath(db, plant.id));
    }
    case ThumbnailSourceRole: {
        QSqlDatabase db = m_db;
        return toUrl(loadPrimaryPlantImagePath(db, plant.id));
    }
    default:
        return {};
    }
}

QHash<int, QByteArray> PlantListViewModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {NameRole, "name"},
        {ScientificNameRole, "scientificName"},
        {PlantTypeRole, "plantType"},
        {LightRequirementRole, "lightRequirement"},
        {WateringFrequencyRole, "wateringFrequency"},
        {WateringNotesRole, "wateringNotes"},
        {HumidityPreferenceRole, "humidityPreference"},
        {SoilTypeRole, "soilType"},
        {LastWateredRole, "lastWatered"},
        {FertilizingScheduleRole, "fertilizingSchedule"},
        {LastFertilizedRole, "lastFertilized"},
        {PruningTimeRole, "pruningTime"},
        {PruningNotesRole, "pruningNotes"},
        {LastPrunedRole, "lastPruned"},
        {GrowthRateRole, "growthRate"},
        {IssuesPestsRole, "issuesPests"},
        {TemperatureToleranceRole, "temperatureTolerance"},
        {ToxicToPetsRole, "toxicToPets"},
        {PoisonousToHumansRole, "poisonousToHumans"},
        {PoisonousToPetsRole, "poisonousToPets"},
        {IndoorRole, "indoor"},
        {FloweringSeasonRole, "floweringSeason"},
        {TagsRole, "tags"},
        {AcquiredDateRole, "acquiredDate"},
        {SourceRole, "source"},
        {NotesRole, "notes"},
        {ImageSourceRole, "imageSource"},
        {ThumbnailSourceRole, "thumbnailSource"}
    };
}

QString PlantListViewModel::toUrl(const QString &path) const
{
    if (path.isEmpty()) {
        return {};
    }
    if (path.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive)) {
        return path;
    }
    return QUrl::fromLocalFile(path).toString();
}

void PlantListViewModel::refresh()
{
    QString error;
    const QVector<TPlant> plants = loadPlants(m_db, &error);
    m_allPlants.clear();
    for (const TPlant &plant : plants) {
        m_allPlants.append(QSharedPointer<Plant>::create(plant));
    }
    qDebug() << "Loaded" << m_allPlants.size() << "plants from SQLite.";
    applyTagFilter();
    setLastError(error);
    emit plantsChanged();
}

int PlantListViewModel::addPlant(const QVariantMap &data)
{
    if (!validateInput(data)) {
        return 0;
    }

    TPlant plant = makePlant(0, data);
    int id = 0;
    QString error;
    if (!insertPlant(m_db, plant, &id, &error) || id <= 0) {
        setLastError(error.isEmpty() ? QStringLiteral("Failed to add plant.") : error);
        return 0;
    }

    refresh();
    setLastError(QString());
    return id;
}

bool PlantListViewModel::updatePlant(int id, const QVariantMap &data)
{
    if (id <= 0) {
        setLastError(QStringLiteral("Select a plant to update."));
        return false;
    }

    if (!validateInput(data)) {
        return false;
    }

    TPlant plant = makePlant(id, data);
    QString error;
    if (!updatePlantRow(m_db, plant, &error)) {
        setLastError(error.isEmpty() ? QStringLiteral("Failed to update plant.") : error);
        return false;
    }

    refresh();
    setLastError(QString());
    return true;
}

bool PlantListViewModel::removePlant(int id)
{
    if (id <= 0) {
        setLastError(QStringLiteral("Select a plant to delete."));
        return false;
    }

    QString error;
    if (!removePlantRow(m_db, id, &error)) {
        setLastError(error.isEmpty() ? QStringLiteral("Failed to delete plant.") : error);
        return false;
    }

    refresh();
    setLastError(QString());
    return true;
}

bool PlantListViewModel::importFromSqlite(const QString &filePath)
{
    qDebug() << "Importing plants from SQLite file:" << filePath;

    const QString databasePath = normalizeLocalPath(filePath);
    const QFileInfo info(databasePath);
    if (!info.exists() || !info.isFile()) {
        setLastError(QStringLiteral("Import failed: file not found."));
        return false;
    }

    QString loadError;
    const QVector<TPlant> importedPlants = loadPlantsFromSqliteFile(info.absoluteFilePath(), &loadError);
    if (!loadError.isEmpty()) {
        qDebug() << "Error could not read file with error msg:" << loadError;
        setLastError(QStringLiteral("Import failed: could not read plants from SQLite."));
        return false;
    }

    if (importedPlants.isEmpty()) {
        setLastError(QStringLiteral("Import completed, but no plants were found."));
        return false;
    }

    QString replaceError;
    if (!replacePlantsFromSource(m_db, importedPlants, &replaceError)) {
        qDebug() << "Import failed while writing current database:" << replaceError;
        setLastError(QStringLiteral("Import failed: could not replace local plants."));
        return false;
    }

    refresh();
    setLastError(QString());
    return true;
}

QVariantList PlantListViewModel::plantImages(int plantId) const
{
    QVariantList result;
    QString error;
    QSqlDatabase db = m_db;
    const QVector<PlantImageRecord> images = loadPlantImages(db, plantId, &error);
    for (const PlantImageRecord &image : images) {
        QVariantMap entry;
        entry.insert(QStringLiteral("id"), image.id);
        entry.insert(QStringLiteral("plantId"), image.plantId);
        entry.insert(QStringLiteral("path"), image.filePath);
        entry.insert(QStringLiteral("url"), toUrl(image.filePath));
        entry.insert(QStringLiteral("createdAt"), image.createdAt);
        result.append(entry);
    }
    return result;
}

bool PlantListViewModel::addPlantImage(int plantId, const QString &filePath)
{
    const QString localPath = normalizeLocalPath(filePath);
    if (localPath.isEmpty()) {
        setLastError(QStringLiteral("Select an image file."));
        return false;
    }

    const QFileInfo info(localPath);
    if (!info.exists() || !info.isFile()) {
        setLastError(QStringLiteral("Image file not found."));
        return false;
    }

    QString error;
    if (!insertPlantImageRow(m_db, plantId, info.absoluteFilePath(), &error)) {
        setLastError(error.isEmpty() ? QStringLiteral("Failed to add image.") : error);
        return false;
    }

    for (int row = 0; row < m_plants.size(); ++row) {
        if (m_plants.at(row)->id() == plantId) {
            const QModelIndex modelIndex = index(row);
            emit dataChanged(modelIndex, modelIndex, {ImageSourceRole, ThumbnailSourceRole});
            break;
        }
    }

    setLastError(QString());
    return true;
}

bool PlantListViewModel::removePlantImage(int imageId)
{
    int plantId = 0;
    {
        QString error;
        if (!ensurePlantImagesTableExists(m_db, &error)) {
            setLastError(error);
            return false;
        }

        QSqlQuery query(m_db);
        query.prepare("SELECT plant_id FROM plant_images WHERE id = :id;");
        query.bindValue(":id", imageId);
        if (!query.exec()) {
            setLastError(query.lastError().text());
            return false;
        }
        if (query.next()) {
            plantId = query.value(0).toInt();
        }
    }

    QString error;
    if (!removePlantImageRow(m_db, imageId, &error)) {
        setLastError(error.isEmpty() ? QStringLiteral("Failed to delete image.") : error);
        return false;
    }

    if (plantId > 0) {
        for (int row = 0; row < m_plants.size(); ++row) {
            if (m_plants.at(row)->id() == plantId) {
                const QModelIndex modelIndex = index(row);
                emit dataChanged(modelIndex, modelIndex, {ImageSourceRole, ThumbnailSourceRole});
                break;
            }
        }
    }

    setLastError(QString());
    return true;
}

QVariantMap PlantListViewModel::careSchedule(int plantId) const
{
    QVariantMap result;
    result.insert(QStringLiteral("activeSeason"), currentSeasonName());

    const auto buildDefaultSeasonalMap = []() {
        QVariantMap seasonal;
        for (const QString &season : careSeasons()) {
            QVariantMap entry;
            entry.insert(QStringLiteral("useOverride"), false);
            entry.insert(QStringLiteral("enabled"), true);
            entry.insert(QStringLiteral("intervalDays"), 0);
            seasonal.insert(season, entry);
        }
        return seasonal;
    };

    auto buildCareMap = [&](const QString &careType, const QString &legacyText) {
        QVariantMap care;
        care.insert(QStringLiteral("defaultIntervalDays"), extractIntervalDays(legacyText));
        care.insert(QStringLiteral("seasonal"), buildDefaultSeasonalMap());
        care.insert(QStringLiteral("activeEnabled"), extractIntervalDays(legacyText) > 0);
        care.insert(QStringLiteral("activeIntervalDays"), extractIntervalDays(legacyText));
        care.insert(QStringLiteral("summary"),
                    extractIntervalDays(legacyText) > 0
                        ? QStringLiteral("Every %1 days").arg(extractIntervalDays(legacyText))
                        : QStringLiteral("Not set"));
        return care;
    };

    QString legacyWateringText;
    QString legacyFertilizingText;
    for (const auto &plantPtr : m_allPlants) {
        if (plantPtr->id() == plantId) {
            const TPlant &plant = plantPtr->data();
            legacyWateringText = plant.wateringFrequency;
            legacyFertilizingText = plant.fertilizingSchedule;
            break;
        }
    }

    result.insert(QStringLiteral("water"), buildCareMap(QStringLiteral("Water"), legacyWateringText));
    result.insert(QStringLiteral("fertilize"), buildCareMap(QStringLiteral("Fertilize"), legacyFertilizingText));

    QString error;
    QSqlDatabase db = m_db;
    const QVector<CareScheduleRecord> rows = loadCareScheduleRows(db, plantId, &error);
    if (!error.isEmpty()) {
        qWarning() << "Failed loading care schedule for plant" << plantId << ":" << error;
        return result;
    }

    const QString activeSeason = result.value(QStringLiteral("activeSeason")).toString();

    auto applyRowsToCareMap = [&](const QString &careKey, const QString &careType) {
        QVariantMap care = result.value(careKey).toMap();
        QVariantMap seasonal = care.value(QStringLiteral("seasonal")).toMap();
        int defaultIntervalDays = care.value(QStringLiteral("defaultIntervalDays")).toInt();
        QMap<QString, CareScheduleRecord> overrides;

        for (const CareScheduleRecord &row : rows) {
            if (row.careType.compare(careType, Qt::CaseInsensitive) != 0) {
                continue;
            }

            if (row.seasonName == QStringLiteral("default")) {
                defaultIntervalDays = row.intervalDays;
                care.insert(QStringLiteral("defaultIntervalDays"), row.intervalDays);
                continue;
            }

            overrides.insert(row.seasonName, row);
            QVariantMap entry = seasonal.value(row.seasonName).toMap();
            entry.insert(QStringLiteral("useOverride"), true);
            entry.insert(QStringLiteral("enabled"), row.enabled);
            entry.insert(QStringLiteral("intervalDays"), row.enabled ? row.intervalDays : 0);
            seasonal.insert(row.seasonName, entry);
        }

        care.insert(QStringLiteral("seasonal"), seasonal);
        if (overrides.contains(activeSeason)) {
            const CareScheduleRecord &active = overrides.value(activeSeason);
            care.insert(QStringLiteral("activeEnabled"), active.enabled);
            care.insert(QStringLiteral("activeIntervalDays"), active.enabled ? active.intervalDays : 0);
        } else {
            care.insert(QStringLiteral("activeEnabled"), defaultIntervalDays > 0);
            care.insert(QStringLiteral("activeIntervalDays"), defaultIntervalDays);
        }
        care.insert(QStringLiteral("summary"), formatCareSummary(defaultIntervalDays, overrides));
        result.insert(careKey, care);
    };

    applyRowsToCareMap(QStringLiteral("water"), QStringLiteral("Water"));
    applyRowsToCareMap(QStringLiteral("fertilize"), QStringLiteral("Fertilize"));

    return result;
}

bool PlantListViewModel::saveCareSchedule(int plantId, const QVariantMap &schedule)
{
    if (plantId <= 0) {
        setLastError(QStringLiteral("Select a plant before saving care schedule."));
        return false;
    }

    QVector<CareScheduleRecord> rows;

    const auto parseCareConfig = [&](const QString &careKey, const QString &careType, QString *summary) -> bool {
        const QVariantMap care = schedule.value(careKey).toMap();
        const int defaultIntervalDays = care.value(QStringLiteral("defaultIntervalDays")).toInt();
        const QVariantMap seasonal = care.value(QStringLiteral("seasonal")).toMap();
        QMap<QString, CareScheduleRecord> overrides;

        if (defaultIntervalDays > 0) {
            CareScheduleRecord defaultRow;
            defaultRow.plantId = plantId;
            defaultRow.careType = careType;
            defaultRow.seasonName = QStringLiteral("default");
            defaultRow.intervalDays = defaultIntervalDays;
            defaultRow.enabled = true;
            rows.append(defaultRow);
        }

        for (const QString &season : careSeasons()) {
            const QVariantMap entry = seasonal.value(season).toMap();
            if (!entry.value(QStringLiteral("useOverride")).toBool()) {
                continue;
            }

            CareScheduleRecord row;
            row.plantId = plantId;
            row.careType = careType;
            row.seasonName = season;
            row.enabled = entry.value(QStringLiteral("enabled")).toBool();
            row.intervalDays = row.enabled ? entry.value(QStringLiteral("intervalDays")).toInt() : 0;
            rows.append(row);
            overrides.insert(season, row);
        }

        if (summary) {
            *summary = formatCareSummary(defaultIntervalDays, overrides);
        }
        return true;
    };

    QString wateringSummary;
    QString fertilizingSummary;
    parseCareConfig(QStringLiteral("water"), QStringLiteral("Water"), &wateringSummary);
    parseCareConfig(QStringLiteral("fertilize"), QStringLiteral("Fertilize"), &fertilizingSummary);

    QString error;
    if (!replaceCareScheduleRows(m_db, plantId, rows, wateringSummary, fertilizingSummary, &error)) {
        setLastError(error.isEmpty() ? QStringLiteral("Failed to save care schedule.") : error);
        return false;
    }

    qDebug() << "Saved care schedule for plant" << plantId << schedule;
    refresh();
    setLastError(QString());
    return true;
}

QString PlantListViewModel::lastError() const
{
    return m_lastError;
}

QString PlantListViewModel::tagFilter() const
{
    return m_tagFilter;
}

void PlantListViewModel::setTagFilter(const QString &value)
{
    const QString normalized = normalizedTagsString(value);
    if (m_tagFilter == normalized) {
        return;
    }
    m_tagFilter = normalized;
    applyTagFilter();
    emit tagFilterChanged();
}

bool PlantListViewModel::validateInput(const QVariantMap &data)
{
    const QString name = data.value("name").toString();
    if (name.trimmed().isEmpty()) {
        setLastError(QStringLiteral("Name is required."));
        return false;
    }

    const QStringList dateKeys = {
        "acquiredDate",
        "lastWatered",
        "lastFertilized",
        "lastPruned"
    };
    for (const QString &key : dateKeys) {
        const QString value = data.value(key).toString().trimmed();
        if (!value.isEmpty()) {
            const QDate parsed = QDate::fromString(value, Qt::ISODate);
            if (!parsed.isValid()) {
                setLastError(QStringLiteral("Dates must be YYYY-MM-DD."));
                return false;
            }
        }
    }

    setLastError(QString());
    return true;
}

void PlantListViewModel::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }
    m_lastError = message;
    emit lastErrorChanged();
}

void PlantListViewModel::applyTagFilter()
{
    beginResetModel();
    m_plants.clear();
    for (const auto &plantPtr : m_allPlants) {
        if (plantMatchesTagFilter(plantPtr->data(), m_tagFilter)) {
            m_plants.append(plantPtr);
        }
    }
    endResetModel();
    emit countChanged();
}

TPlant PlantListViewModel::makePlant(int id, const QVariantMap &data) const
{
    TPlant plant;
    plant.id = id;
    plant.name = data.value("name").toString();
    plant.scientificName = data.value("scientificName").toString();
    plant.plantType = data.value("plantType").toString();
    plant.lightRequirement = data.value("lightRequirement").toString();
    plant.wateringFrequency = data.value("wateringFrequency").toString();
    plant.wateringNotes = data.value("wateringNotes").toString();
    plant.humidityPreference = data.value("humidityPreference").toString();
    plant.soilType = data.value("soilType").toString();
    plant.lastWatered = QDate::fromString(data.value("lastWatered").toString(), Qt::ISODate);
    plant.fertilizingSchedule = data.value("fertilizingSchedule").toString();
    plant.lastFertilized = QDate::fromString(data.value("lastFertilized").toString(), Qt::ISODate);
    plant.pruningTime = data.value("pruningTime").toString();
    plant.pruningNotes = data.value("pruningNotes").toString();
    plant.lastPruned = QDate::fromString(data.value("lastPruned").toString(), Qt::ISODate);
    plant.growthRate = data.value("growthRate").toString();
    plant.issuesPests = data.value("issuesPests").toString();
    plant.temperatureTolerance = data.value("temperatureTolerance").toString();
    plant.toxicToPets = data.value("toxicToPets").toString();
    plant.poisonousToHumans = data.value("poisonousToHumans").toString();
    plant.poisonousToPets = data.value("poisonousToPets").toString();
    plant.indoor = data.value("indoor").toString();
    plant.floweringSeason = data.value("floweringSeason").toString();
    plant.tags = normalizedTagsString(data.value("tags").toString());
    plant.acquiredDate = QDate::fromString(data.value("acquiredDate").toString(), Qt::ISODate);
    plant.source = data.value("source").toString();
    plant.notes = data.value("notes").toString();
    return plant;
}
