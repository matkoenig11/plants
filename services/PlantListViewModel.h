#pragma once

#include <QAbstractListModel>
#include <QSharedPointer>
#include <QSqlDatabase>
#include <QVariantMap>
#include <QVector>

#include "Plant.h"

class PlantListViewModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY lastErrorChanged)
public:
    enum PlantRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        ScientificNameRole,
        PlantTypeRole,
        LightRequirementRole,
        WateringFrequencyRole,
        WateringNotesRole,
        HumidityPreferenceRole,
        SoilTypeRole,
        LastWateredRole,
        FertilizingScheduleRole,
        LastFertilizedRole,
        PruningTimeRole,
        PruningNotesRole,
        LastPrunedRole,
        GrowthRateRole,
        IssuesPestsRole,
        TemperatureToleranceRole,
        ToxicToPetsRole,
        PoisonousToHumansRole,
        PoisonousToPetsRole,
        IndoorRole,
        FloweringSeasonRole,
        AcquiredDateRole,
        SourceRole,
        NotesRole,
        ImageSourceRole,
        ThumbnailSourceRole
    };

    explicit PlantListViewModel(QSqlDatabase db, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE int addPlant(const QVariantMap &data);
    Q_INVOKABLE bool updatePlant(int id, const QVariantMap &data);
    Q_INVOKABLE bool removePlant(int id);
    Q_INVOKABLE bool importFromSqlite(const QString &filePath);
    Q_INVOKABLE QVariantList plantImages(int plantId) const;
    Q_INVOKABLE bool addPlantImage(int plantId, const QString &filePath);
    Q_INVOKABLE bool removePlantImage(int imageId);
    Q_INVOKABLE QVariantMap careSchedule(int plantId) const;
    Q_INVOKABLE bool saveCareSchedule(int plantId, const QVariantMap &schedule);
    Q_INVOKABLE QString toUrl(const QString &path) const;
    Q_INVOKABLE QString lastError() const;

signals:
    void lastErrorChanged();
    void countChanged();

private:
    bool validateInput(const QVariantMap &data);
    void setLastError(const QString &message);

    TPlant makePlant(int id, const QVariantMap &data) const;

    QSqlDatabase m_db;
    QVector<QSharedPointer<Plant>> m_plants;
    QString m_lastError;
};
