#pragma once

#include <QDate>
#include <QDateTime>
#include <QObject>
#include <QString>

struct TPlant
{
    int id = 0;
    QString name;
    QString scientificName;
    QString plantType;
    QString lightRequirement;
    QString wateringFrequency;
    QString wateringNotes;
    QString humidityPreference;
    QString soilType;
    QDate lastWatered;
    QString fertilizingSchedule;
    QDate lastFertilized;
    QString pruningTime;
    QString pruningNotes;
    QDate lastPruned;
    QString growthRate;
    QString issuesPests;
    QString temperatureTolerance;
    QString toxicToPets;
    QString poisonousToHumans;
    QString poisonousToPets;
    QString indoor;
    QString floweringSeason;
    QString tags;
    QDate acquiredDate;
    QString source;
    QString notes;
    QDateTime createdAt;
    QDateTime updatedAt;

    bool isValid() const { return !name.trimmed().isEmpty(); }
};

class Plant : public QObject
{
    Q_OBJECT
public:
    explicit Plant(QObject *parent = nullptr);
    explicit Plant(const TPlant &data, QObject *parent = nullptr);

    // Getters
    int id() const;
    QString name() const;
    QString scientificName() const;
    QString plantType() const;
    QString lightRequirement() const;
    QString wateringFrequency() const;
    QString wateringNotes() const;
    QString humidityPreference() const;
    QString soilType() const;
    QDate lastWatered() const;
    QString fertilizingSchedule() const;
    QDate lastFertilized() const;
    QString pruningTime() const;
    QString pruningNotes() const;
    QDate lastPruned() const;
    QString growthRate() const;
    QString issuesPests() const;
    QString temperatureTolerance() const;
    QString toxicToPets() const;
    QString poisonousToHumans() const;
    QString poisonousToPets() const;
    QString indoor() const;
    QString floweringSeason() const;
    QString tags() const;
    QDate acquiredDate() const;
    QString source() const;
    QString notes() const;
    QDateTime createdAt() const;
    QDateTime updatedAt() const;

    // Setters
    void setId(int value);
    void setName(const QString &value);
    void setScientificName(const QString &value);
    void setPlantType(const QString &value);
    void setLightRequirement(const QString &value);
    void setWateringFrequency(const QString &value);
    void setWateringNotes(const QString &value);
    void setHumidityPreference(const QString &value);
    void setSoilType(const QString &value);
    void setLastWatered(const QDate &value);
    void setFertilizingSchedule(const QString &value);
    void setLastFertilized(const QDate &value);
    void setPruningTime(const QString &value);
    void setPruningNotes(const QString &value);
    void setLastPruned(const QDate &value);
    void setGrowthRate(const QString &value);
    void setIssuesPests(const QString &value);
    void setTemperatureTolerance(const QString &value);
    void setToxicToPets(const QString &value);
    void setPoisonousToHumans(const QString &value);
    void setPoisonousToPets(const QString &value);
    void setIndoor(const QString &value);
    void setFloweringSeason(const QString &value);
    void setTags(const QString &value);
    void setAcquiredDate(const QDate &value);
    void setSource(const QString &value);
    void setNotes(const QString &value);
    void setCreatedAt(const QDateTime &value);
    void setUpdatedAt(const QDateTime &value);

    bool isValid() const;
    const TPlant &data() const;
    void setData(const TPlant &data);

private:
    TPlant m_data;
};
