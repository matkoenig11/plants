#include "PlantListViewModel.h"

#include <QDate>

PlantListViewModel::PlantListViewModel(QSqlDatabase db, QObject *parent)
    : QAbstractListModel(parent)
    , m_repository(db)
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

    const Plant &plant = m_plants.at(index.row());
    switch (role) {
    case IdRole:
        return plant.id;
    case NameRole:
        return plant.name;
    case ScientificNameRole:
        return plant.scientificName;
    case PlantTypeRole:
        return plant.plantType;
    case WinterLocationRole:
        return plant.winterLocation;
    case SummerLocationRole:
        return plant.summerLocation;
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
    case PotSizeRole:
        return plant.potSize;
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
    case CurrentHealthStatusRole:
        return plant.currentHealthStatus;
    case IssuesPestsRole:
        return plant.issuesPests;
    case TemperatureToleranceRole:
        return plant.temperatureTolerance;
    case ToxicToPetsRole:
        return plant.toxicToPets;
    case AcquiredDateRole:
        return plant.acquiredDate.isValid() ? plant.acquiredDate.toString(Qt::ISODate) : QString();
    case SourceRole:
        return plant.source;
    case NotesRole:
        return plant.notes;
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
        {WinterLocationRole, "winterLocation"},
        {SummerLocationRole, "summerLocation"},
        {LightRequirementRole, "lightRequirement"},
        {WateringFrequencyRole, "wateringFrequency"},
        {WateringNotesRole, "wateringNotes"},
        {HumidityPreferenceRole, "humidityPreference"},
        {SoilTypeRole, "soilType"},
        {PotSizeRole, "potSize"},
        {LastWateredRole, "lastWatered"},
        {FertilizingScheduleRole, "fertilizingSchedule"},
        {LastFertilizedRole, "lastFertilized"},
        {PruningTimeRole, "pruningTime"},
        {PruningNotesRole, "pruningNotes"},
        {LastPrunedRole, "lastPruned"},
        {GrowthRateRole, "growthRate"},
        {CurrentHealthStatusRole, "currentHealthStatus"},
        {IssuesPestsRole, "issuesPests"},
        {TemperatureToleranceRole, "temperatureTolerance"},
        {ToxicToPetsRole, "toxicToPets"},
        {AcquiredDateRole, "acquiredDate"},
        {SourceRole, "source"},
        {NotesRole, "notes"}
    };
}

void PlantListViewModel::refresh()
{
    beginResetModel();
    m_plants = m_repository.listAll();
    qDebug() << "Loaded" << m_plants.size() << "plants from database.";
    endResetModel();
    setLastError(QString());
}

int PlantListViewModel::addPlant(const QVariantMap &data)
{
    if (!validateInput(data)) {
        return 0;
    }

    Plant plant = makePlant(0, data);
    const int id = m_repository.create(plant);
    if (id <= 0) {
        setLastError(QStringLiteral("Failed to add plant."));
        return 0;
    }

    plant.id = id;
    beginInsertRows(QModelIndex(), m_plants.size(), m_plants.size());
    m_plants.append(plant);
    endInsertRows();
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

    Plant plant = makePlant(id, data);
    if (!m_repository.update(plant)) {
        setLastError(QStringLiteral("Failed to update plant."));
        return false;
    }

    for (int i = 0; i < m_plants.size(); ++i) {
        if (m_plants[i].id == id) {
            m_plants[i] = plant;
            const QModelIndex modelIndex = index(i);
            emit dataChanged(modelIndex, modelIndex);
            break;
        }
    }

    setLastError(QString());
    return true;
}

bool PlantListViewModel::removePlant(int id)
{
    if (id <= 0) {
        setLastError(QStringLiteral("Select a plant to delete."));
        return false;
    }

    if (!m_repository.remove(id)) {
        setLastError(QStringLiteral("Failed to delete plant."));
        return false;
    }

    for (int i = 0; i < m_plants.size(); ++i) {
        if (m_plants[i].id == id) {
            beginRemoveRows(QModelIndex(), i, i);
            m_plants.removeAt(i);
            endRemoveRows();
            break;
        }
    }

    setLastError(QString());
    return true;
}

QString PlantListViewModel::lastError() const
{
    return m_lastError;
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

Plant PlantListViewModel::makePlant(int id, const QVariantMap &data) const
{
    Plant plant;
    plant.id = id;
    plant.name = data.value("name").toString();
    plant.scientificName = data.value("scientificName").toString();
    plant.plantType = data.value("plantType").toString();
    plant.winterLocation = data.value("winterLocation").toString();
    plant.summerLocation = data.value("summerLocation").toString();
    plant.lightRequirement = data.value("lightRequirement").toString();
    plant.wateringFrequency = data.value("wateringFrequency").toString();
    plant.wateringNotes = data.value("wateringNotes").toString();
    plant.humidityPreference = data.value("humidityPreference").toString();
    plant.soilType = data.value("soilType").toString();
    plant.potSize = data.value("potSize").toString();
    plant.lastWatered = QDate::fromString(data.value("lastWatered").toString(), Qt::ISODate);
    plant.fertilizingSchedule = data.value("fertilizingSchedule").toString();
    plant.lastFertilized = QDate::fromString(data.value("lastFertilized").toString(), Qt::ISODate);
    plant.pruningTime = data.value("pruningTime").toString();
    plant.pruningNotes = data.value("pruningNotes").toString();
    plant.lastPruned = QDate::fromString(data.value("lastPruned").toString(), Qt::ISODate);
    plant.growthRate = data.value("growthRate").toString();
    plant.currentHealthStatus = data.value("currentHealthStatus").toString();
    plant.issuesPests = data.value("issuesPests").toString();
    plant.temperatureTolerance = data.value("temperatureTolerance").toString();
    plant.toxicToPets = data.value("toxicToPets").toString();
    plant.acquiredDate = QDate::fromString(data.value("acquiredDate").toString(), Qt::ISODate);
    plant.source = data.value("source").toString();
    plant.notes = data.value("notes").toString();
    return plant;
}
