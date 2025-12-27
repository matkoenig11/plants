#include "PlantRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {
QString dateToString(const QDate &date)
{
    return date.isValid() ? date.toString(Qt::ISODate) : QString();
}

QString dateTimeToString(const QDateTime &dateTime)
{
    return dateTime.isValid() ? dateTime.toString(Qt::ISODate) : QString();
}

QDate stringToDate(const QString &value)
{
    return value.isEmpty() ? QDate() : QDate::fromString(value, Qt::ISODate);
}

QDateTime stringToDateTime(const QString &value)
{
    return value.isEmpty() ? QDateTime() : QDateTime::fromString(value, Qt::ISODate);
}
}

PlantRepository::PlantRepository(QSqlDatabase db)
    : m_db(db)
{
}

int PlantRepository::create(const Plant &plant)
{
    if (!m_db.isOpen() || !plant.isValid()) {
        return 0;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO plants ("
        "name, scientific_name, plant_type, winter_location, summer_location, "
        "light_requirement, watering_frequency, watering_notes, humidity_preference, "
        "soil_type, pot_size, last_watered, fertilizing_schedule, last_fertilized, "
        "pruning_time, pruning_notes, last_pruned, growth_rate, current_health_status, "
        "issues_pests, temperature_tolerance, toxic_to_pets, acquired_on, source, notes"
        ") VALUES ("
        ":name, :scientific_name, :plant_type, :winter_location, :summer_location, "
        ":light_requirement, :watering_frequency, :watering_notes, :humidity_preference, "
        ":soil_type, :pot_size, :last_watered, :fertilizing_schedule, :last_fertilized, "
        ":pruning_time, :pruning_notes, :last_pruned, :growth_rate, :current_health_status, "
        ":issues_pests, :temperature_tolerance, :toxic_to_pets, :acquired_on, :source, :notes"
        ");"
    );
    query.bindValue(":name", plant.name);
    query.bindValue(":scientific_name", plant.scientificName);
    query.bindValue(":plant_type", plant.plantType);
    query.bindValue(":winter_location", plant.winterLocation);
    query.bindValue(":summer_location", plant.summerLocation);
    query.bindValue(":light_requirement", plant.lightRequirement);
    query.bindValue(":watering_frequency", plant.wateringFrequency);
    query.bindValue(":watering_notes", plant.wateringNotes);
    query.bindValue(":humidity_preference", plant.humidityPreference);
    query.bindValue(":soil_type", plant.soilType);
    query.bindValue(":pot_size", plant.potSize);
    query.bindValue(":last_watered", dateToString(plant.lastWatered));
    query.bindValue(":fertilizing_schedule", plant.fertilizingSchedule);
    query.bindValue(":last_fertilized", dateToString(plant.lastFertilized));
    query.bindValue(":pruning_time", plant.pruningTime);
    query.bindValue(":pruning_notes", plant.pruningNotes);
    query.bindValue(":last_pruned", dateToString(plant.lastPruned));
    query.bindValue(":growth_rate", plant.growthRate);
    query.bindValue(":current_health_status", plant.currentHealthStatus);
    query.bindValue(":issues_pests", plant.issuesPests);
    query.bindValue(":temperature_tolerance", plant.temperatureTolerance);
    query.bindValue(":toxic_to_pets", plant.toxicToPets);
    query.bindValue(":acquired_on", dateToString(plant.acquiredDate));
    query.bindValue(":source", plant.source);
    query.bindValue(":notes", plant.notes);

    if (!query.exec()) {
        return 0;
    }

    return query.lastInsertId().toInt();
}

bool PlantRepository::update(const Plant &plant)
{
    if (!m_db.isOpen() || plant.id <= 0 || !plant.isValid()) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE plants "
        "SET name = :name, "
        "    scientific_name = :scientific_name, "
        "    plant_type = :plant_type, "
        "    winter_location = :winter_location, "
        "    summer_location = :summer_location, "
        "    light_requirement = :light_requirement, "
        "    watering_frequency = :watering_frequency, "
        "    watering_notes = :watering_notes, "
        "    humidity_preference = :humidity_preference, "
        "    soil_type = :soil_type, "
        "    pot_size = :pot_size, "
        "    last_watered = :last_watered, "
        "    fertilizing_schedule = :fertilizing_schedule, "
        "    last_fertilized = :last_fertilized, "
        "    pruning_time = :pruning_time, "
        "    pruning_notes = :pruning_notes, "
        "    last_pruned = :last_pruned, "
        "    growth_rate = :growth_rate, "
        "    current_health_status = :current_health_status, "
        "    issues_pests = :issues_pests, "
        "    temperature_tolerance = :temperature_tolerance, "
        "    toxic_to_pets = :toxic_to_pets, "
        "    acquired_on = :acquired_on, "
        "    source = :source, "
        "    notes = :notes, "
        "    updated_at = datetime('now') "
        "WHERE id = :id;"
    );
    query.bindValue(":name", plant.name);
    query.bindValue(":scientific_name", plant.scientificName);
    query.bindValue(":plant_type", plant.plantType);
    query.bindValue(":winter_location", plant.winterLocation);
    query.bindValue(":summer_location", plant.summerLocation);
    query.bindValue(":light_requirement", plant.lightRequirement);
    query.bindValue(":watering_frequency", plant.wateringFrequency);
    query.bindValue(":watering_notes", plant.wateringNotes);
    query.bindValue(":humidity_preference", plant.humidityPreference);
    query.bindValue(":soil_type", plant.soilType);
    query.bindValue(":pot_size", plant.potSize);
    query.bindValue(":last_watered", dateToString(plant.lastWatered));
    query.bindValue(":fertilizing_schedule", plant.fertilizingSchedule);
    query.bindValue(":last_fertilized", dateToString(plant.lastFertilized));
    query.bindValue(":pruning_time", plant.pruningTime);
    query.bindValue(":pruning_notes", plant.pruningNotes);
    query.bindValue(":last_pruned", dateToString(plant.lastPruned));
    query.bindValue(":growth_rate", plant.growthRate);
    query.bindValue(":current_health_status", plant.currentHealthStatus);
    query.bindValue(":issues_pests", plant.issuesPests);
    query.bindValue(":temperature_tolerance", plant.temperatureTolerance);
    query.bindValue(":toxic_to_pets", plant.toxicToPets);
    query.bindValue(":acquired_on", dateToString(plant.acquiredDate));
    query.bindValue(":source", plant.source);
    query.bindValue(":notes", plant.notes);
    query.bindValue(":id", plant.id);

    return query.exec();
}

bool PlantRepository::remove(int id)
{
    if (!m_db.isOpen() || id <= 0) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM plants WHERE id = :id;");
    query.bindValue(":id", id);
    return query.exec();
}

Plant PlantRepository::findById(int id)
{
    Plant plant;
    if (!m_db.isOpen() || id <= 0) {
        return plant;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "SELECT id, name, scientific_name, plant_type, winter_location, summer_location, "
        "light_requirement, watering_frequency, watering_notes, humidity_preference, "
        "soil_type, pot_size, last_watered, fertilizing_schedule, last_fertilized, "
        "pruning_time, pruning_notes, last_pruned, growth_rate, current_health_status, "
        "issues_pests, temperature_tolerance, toxic_to_pets, acquired_on, source, notes, "
        "created_at, updated_at "
        "FROM plants WHERE id = :id;"
    );
    query.bindValue(":id", id);

    if (!query.exec()) {
        return plant;
    }

    if (query.next()) {
        plant = mapRow(query);
    }

    return plant;
}

QVector<Plant> PlantRepository::listAll()
{
    QVector<Plant> plants;
    if (!m_db.isOpen()) {
        return plants;
    }

    QSqlQuery query(m_db);
    if (!query.exec(
            "SELECT id, name, scientific_name, plant_type, winter_location, summer_location, "
            "light_requirement, watering_frequency, watering_notes, humidity_preference, "
            "soil_type, pot_size, last_watered, fertilizing_schedule, last_fertilized, "
            "pruning_time, pruning_notes, last_pruned, growth_rate, current_health_status, "
            "issues_pests, temperature_tolerance, toxic_to_pets, acquired_on, source, notes, "
            "created_at, updated_at "
            "FROM plants ORDER BY name COLLATE NOCASE;")) {
        return plants;
    }

    while (query.next()) {
        plants.append(mapRow(query));
    }

    return plants;
}

Plant PlantRepository::mapRow(const QSqlQuery &query) const
{
    Plant plant;
    plant.id = query.value(0).toInt();
    plant.name = query.value(1).toString();
    plant.scientificName = query.value(2).toString();
    plant.plantType = query.value(3).toString();
    plant.winterLocation = query.value(4).toString();
    plant.summerLocation = query.value(5).toString();
    plant.lightRequirement = query.value(6).toString();
    plant.wateringFrequency = query.value(7).toString();
    plant.wateringNotes = query.value(8).toString();
    plant.humidityPreference = query.value(9).toString();
    plant.soilType = query.value(10).toString();
    plant.potSize = query.value(11).toString();
    plant.lastWatered = stringToDate(query.value(12).toString());
    plant.fertilizingSchedule = query.value(13).toString();
    plant.lastFertilized = stringToDate(query.value(14).toString());
    plant.pruningTime = query.value(15).toString();
    plant.pruningNotes = query.value(16).toString();
    plant.lastPruned = stringToDate(query.value(17).toString());
    plant.growthRate = query.value(18).toString();
    plant.currentHealthStatus = query.value(19).toString();
    plant.issuesPests = query.value(20).toString();
    plant.temperatureTolerance = query.value(21).toString();
    plant.toxicToPets = query.value(22).toString();
    plant.acquiredDate = stringToDate(query.value(23).toString());
    plant.source = query.value(24).toString();
    plant.notes = query.value(25).toString();
    plant.createdAt = stringToDateTime(query.value(26).toString());
    plant.updatedAt = stringToDateTime(query.value(27).toString());
    return plant;
}
