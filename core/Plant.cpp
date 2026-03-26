#include "Plant.h"

Plant::Plant(QObject *parent)
    : QObject(parent)
{
}

Plant::Plant(const TPlant &data, QObject *parent)
    : QObject(parent)
    , m_data(data)
{
}

int Plant::id() const { return m_data.id; }
QString Plant::name() const { return m_data.name; }
QString Plant::scientificName() const { return m_data.scientificName; }
QString Plant::plantType() const { return m_data.plantType; }
QString Plant::lightRequirement() const { return m_data.lightRequirement; }
QString Plant::wateringFrequency() const { return m_data.wateringFrequency; }
QString Plant::wateringNotes() const { return m_data.wateringNotes; }
QString Plant::humidityPreference() const { return m_data.humidityPreference; }
QString Plant::soilType() const { return m_data.soilType; }
QDate Plant::lastWatered() const { return m_data.lastWatered; }
QString Plant::fertilizingSchedule() const { return m_data.fertilizingSchedule; }
QDate Plant::lastFertilized() const { return m_data.lastFertilized; }
QString Plant::pruningTime() const { return m_data.pruningTime; }
QString Plant::pruningNotes() const { return m_data.pruningNotes; }
QDate Plant::lastPruned() const { return m_data.lastPruned; }
QString Plant::growthRate() const { return m_data.growthRate; }
QString Plant::issuesPests() const { return m_data.issuesPests; }
QString Plant::temperatureTolerance() const { return m_data.temperatureTolerance; }
QString Plant::toxicToPets() const { return m_data.toxicToPets; }
QString Plant::poisonousToHumans() const { return m_data.poisonousToHumans; }
QString Plant::poisonousToPets() const { return m_data.poisonousToPets; }
QString Plant::indoor() const { return m_data.indoor; }
QString Plant::floweringSeason() const { return m_data.floweringSeason; }
QDate Plant::acquiredDate() const { return m_data.acquiredDate; }
QString Plant::source() const { return m_data.source; }
QString Plant::notes() const { return m_data.notes; }
QDateTime Plant::createdAt() const { return m_data.createdAt; }
QDateTime Plant::updatedAt() const { return m_data.updatedAt; }

void Plant::setId(int value) { m_data.id = value; }
void Plant::setName(const QString &value) { m_data.name = value; }
void Plant::setScientificName(const QString &value) { m_data.scientificName = value; }
void Plant::setPlantType(const QString &value) { m_data.plantType = value; }
void Plant::setLightRequirement(const QString &value) { m_data.lightRequirement = value; }
void Plant::setWateringFrequency(const QString &value) { m_data.wateringFrequency = value; }
void Plant::setWateringNotes(const QString &value) { m_data.wateringNotes = value; }
void Plant::setHumidityPreference(const QString &value) { m_data.humidityPreference = value; }
void Plant::setSoilType(const QString &value) { m_data.soilType = value; }
void Plant::setLastWatered(const QDate &value) { m_data.lastWatered = value; }
void Plant::setFertilizingSchedule(const QString &value) { m_data.fertilizingSchedule = value; }
void Plant::setLastFertilized(const QDate &value) { m_data.lastFertilized = value; }
void Plant::setPruningTime(const QString &value) { m_data.pruningTime = value; }
void Plant::setPruningNotes(const QString &value) { m_data.pruningNotes = value; }
void Plant::setLastPruned(const QDate &value) { m_data.lastPruned = value; }
void Plant::setGrowthRate(const QString &value) { m_data.growthRate = value; }
void Plant::setIssuesPests(const QString &value) { m_data.issuesPests = value; }
void Plant::setTemperatureTolerance(const QString &value) { m_data.temperatureTolerance = value; }
void Plant::setToxicToPets(const QString &value) { m_data.toxicToPets = value; }
void Plant::setPoisonousToHumans(const QString &value) { m_data.poisonousToHumans = value; }
void Plant::setPoisonousToPets(const QString &value) { m_data.poisonousToPets = value; }
void Plant::setIndoor(const QString &value) { m_data.indoor = value; }
void Plant::setFloweringSeason(const QString &value) { m_data.floweringSeason = value; }
void Plant::setAcquiredDate(const QDate &value) { m_data.acquiredDate = value; }
void Plant::setSource(const QString &value) { m_data.source = value; }
void Plant::setNotes(const QString &value) { m_data.notes = value; }
void Plant::setCreatedAt(const QDateTime &value) { m_data.createdAt = value; }
void Plant::setUpdatedAt(const QDateTime &value) { m_data.updatedAt = value; }

bool Plant::isValid() const
{
    return m_data.isValid();
}

const TPlant &Plant::data() const
{
    return m_data;
}

void Plant::setData(const TPlant &data)
{
    m_data = data;
}
