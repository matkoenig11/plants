#include "JournalEntryListViewModel.h"

#include <QDate>

JournalEntryListViewModel::JournalEntryListViewModel(QSqlDatabase db, QObject *parent)
    : QAbstractListModel(parent)
    , m_repository(db)
{
}

int JournalEntryListViewModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_entries.size();
}

QVariant JournalEntryListViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }

    const JournalEntry &entry = m_entries.at(index.row());
    switch (role) {
    case IdRole:
        return entry.id;
    case PlantIdRole:
        return entry.plantId;
    case EntryTypeRole:
        return entry.entryType;
    case EntryDateRole:
        return entry.entryDate.toString(Qt::ISODate);
    case NotesRole:
        return entry.notes;
    default:
        return {};
    }
}

QHash<int, QByteArray> JournalEntryListViewModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {PlantIdRole, "plantId"},
        {EntryTypeRole, "entryType"},
        {EntryDateRole, "entryDate"},
        {NotesRole, "notes"}
    };
}

int JournalEntryListViewModel::plantId() const
{
    return m_plantId;
}

QString JournalEntryListViewModel::lastError() const
{
    return m_lastError;
}

void JournalEntryListViewModel::setPlantId(int plantId)
{
    if (m_plantId == plantId) {
        return;
    }

    m_plantId = plantId;
    emit plantIdChanged();
    refresh();
}

void JournalEntryListViewModel::refresh()
{
    beginResetModel();
    if (m_plantId > 0) {
        m_entries = m_repository.listForPlant(m_plantId);
    } else {
        m_entries.clear();
    }
    endResetModel();
    setLastError(QString());
}

int JournalEntryListViewModel::addEntry(int plantId,
                                        const QString &entryType,
                                        const QString &entryDateIso,
                                        const QString &notes)
{
    if (!validateInput(plantId, entryType, entryDateIso)) {
        return 0;
    }

    JournalEntry entry = makeEntry(0, plantId, entryType, entryDateIso, notes);
    const int id = m_repository.create(entry);
    if (id <= 0) {
        setLastError(QStringLiteral("Failed to add journal entry."));
        return 0;
    }

    if (plantId == m_plantId) {
        entry.id = id;
        beginInsertRows(QModelIndex(), 0, 0);
        m_entries.prepend(entry);
        endInsertRows();
    }

    setLastError(QString());
    emit entriesChanged(plantId);
    return id;
}

bool JournalEntryListViewModel::updateEntry(int id,
                                            int plantId,
                                            const QString &entryType,
                                            const QString &entryDateIso,
                                            const QString &notes)
{
    if (id <= 0) {
        setLastError(QStringLiteral("Select a journal entry to update."));
        return false;
    }

    if (!validateInput(plantId, entryType, entryDateIso)) {
        return false;
    }

    const JournalEntry previousEntry = m_repository.findById(id);
    JournalEntry entry = makeEntry(id, plantId, entryType, entryDateIso, notes);
    if (!m_repository.update(entry)) {
        setLastError(QStringLiteral("Failed to update journal entry."));
        return false;
    }

    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].id == id) {
            m_entries[i] = entry;
            const QModelIndex modelIndex = index(i);
            emit dataChanged(modelIndex, modelIndex);
            break;
        }
    }

    setLastError(QString());
    if (previousEntry.plantId > 0 && previousEntry.plantId != plantId) {
        emit entriesChanged(previousEntry.plantId);
    }
    emit entriesChanged(plantId);
    return true;
}

bool JournalEntryListViewModel::removeEntry(int id)
{
    if (id <= 0) {
        setLastError(QStringLiteral("Select a journal entry to delete."));
        return false;
    }

    const JournalEntry entry = m_repository.findById(id);
    if (!m_repository.remove(id)) {
        setLastError(QStringLiteral("Failed to delete journal entry."));
        return false;
    }

    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].id == id) {
            beginRemoveRows(QModelIndex(), i, i);
            m_entries.removeAt(i);
            endRemoveRows();
            break;
        }
    }

    setLastError(QString());
    if (entry.plantId > 0) {
        emit entriesChanged(entry.plantId);
    }
    return true;
}

bool JournalEntryListViewModel::validateInput(int plantId,
                                              const QString &entryType,
                                              const QString &entryDateIso)
{
    if (plantId <= 0) {
        setLastError(QStringLiteral("Select a plant before adding entries."));
        return false;
    }

    if (entryType.trimmed().isEmpty()) {
        setLastError(QStringLiteral("Entry type is required."));
        return false;
    }

    const QDate parsed = QDate::fromString(entryDateIso, Qt::ISODate);
    if (!parsed.isValid()) {
        setLastError(QStringLiteral("Entry date must be YYYY-MM-DD."));
        return false;
    }

    setLastError(QString());
    return true;
}

void JournalEntryListViewModel::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }
    m_lastError = message;
    emit lastErrorChanged();
}

JournalEntry JournalEntryListViewModel::makeEntry(int id,
                                                  int plantId,
                                                  const QString &entryType,
                                                  const QString &entryDateIso,
                                                  const QString &notes) const
{
    JournalEntry entry;
    entry.id = id;
    entry.plantId = plantId;
    entry.entryType = entryType;
    entry.entryDate = QDate::fromString(entryDateIso, Qt::ISODate);
    entry.notes = notes;
    return entry;
}
