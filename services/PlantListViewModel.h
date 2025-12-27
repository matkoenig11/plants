#pragma once

#include <QAbstractListModel>
#include <QSqlDatabase>
#include <QVariantMap>
#include <QVector>

#include "Plant.h"
#include "PlantRepository.h"

class PlantListViewModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    enum PlantRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        ScientificNameRole,
        PlantTypeRole,
        WinterLocationRole,
        SummerLocationRole,
        LightRequirementRole,
        WateringFrequencyRole,
        WateringNotesRole,
        HumidityPreferenceRole,
        SoilTypeRole,
        PotSizeRole,
        LastWateredRole,
        FertilizingScheduleRole,
        LastFertilizedRole,
        PruningTimeRole,
        PruningNotesRole,
        LastPrunedRole,
        GrowthRateRole,
        CurrentHealthStatusRole,
        IssuesPestsRole,
        TemperatureToleranceRole,
        ToxicToPetsRole,
        AcquiredDateRole,
        SourceRole,
        NotesRole
    };

    explicit PlantListViewModel(QSqlDatabase db, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE int addPlant(const QVariantMap &data);
    Q_INVOKABLE bool updatePlant(int id, const QVariantMap &data);
    Q_INVOKABLE bool removePlant(int id);
    QString lastError() const;

signals:
    void lastErrorChanged();

private:
    bool validateInput(const QVariantMap &data);
    void setLastError(const QString &message);

    Plant makePlant(int id, const QVariantMap &data) const;

    PlantRepository m_repository;
    QVector<Plant> m_plants;
    QString m_lastError;
};
