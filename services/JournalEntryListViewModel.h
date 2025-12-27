#pragma once

#include <QAbstractListModel>
#include <QSqlDatabase>
#include <QVector>

#include "JournalEntry.h"
#include "JournalEntryRepository.h"

class JournalEntryListViewModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int plantId READ plantId NOTIFY plantIdChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    enum EntryRoles {
        IdRole = Qt::UserRole + 1,
        PlantIdRole,
        EntryTypeRole,
        EntryDateRole,
        NotesRole
    };

    explicit JournalEntryListViewModel(QSqlDatabase db, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int plantId() const;
    QString lastError() const;

    Q_INVOKABLE void setPlantId(int plantId);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE int addEntry(int plantId,
                             const QString &entryType,
                             const QString &entryDateIso,
                             const QString &notes);
    Q_INVOKABLE bool updateEntry(int id,
                                 int plantId,
                                 const QString &entryType,
                                 const QString &entryDateIso,
                                 const QString &notes);
    Q_INVOKABLE bool removeEntry(int id);

signals:
    void plantIdChanged();
    void lastErrorChanged();

private:
    bool validateInput(int plantId, const QString &entryType, const QString &entryDateIso);
    void setLastError(const QString &message);
    JournalEntry makeEntry(int id,
                           int plantId,
                           const QString &entryType,
                           const QString &entryDateIso,
                           const QString &notes) const;

    JournalEntryRepository m_repository;
    QVector<JournalEntry> m_entries;
    int m_plantId = -1;
    QString m_lastError;
};
