#pragma once

#include <QDate>
#include <QDateTime>
#include <QString>

struct Plant
{
    int id = 0;
    QString name;
    QString scientificName;
    QString plantType;
    QString winterLocation;
    QString summerLocation;
    QString lightRequirement;
    QString wateringFrequency;
    QString wateringNotes;
    QString humidityPreference;
    QString soilType;
    QString potSize;
    QDate lastWatered;
    QString fertilizingSchedule;
    QDate lastFertilized;
    QString pruningTime;
    QString pruningNotes;
    QDate lastPruned;
    QString growthRate;
    QString currentHealthStatus;
    QString issuesPests;
    QString temperatureTolerance;
    QString toxicToPets;
    QDate acquiredDate;
    QString source;
    QString notes;
    QDateTime createdAt;
    QDateTime updatedAt;

    bool isValid() const { return !name.trimmed().isEmpty(); }
};
