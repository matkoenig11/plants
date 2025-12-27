#pragma once

#include <QSqlDatabase>
#include <QVector>

#include "JournalEntry.h"

class QSqlQuery;

class JournalEntryRepository
{
public:
    explicit JournalEntryRepository(QSqlDatabase db);

    int create(const JournalEntry &entry);
    bool update(const JournalEntry &entry);
    bool remove(int id);
    JournalEntry findById(int id);
    QVector<JournalEntry> listForPlant(int plantId);

private:
    JournalEntry mapRow(const QSqlQuery &query) const;

    QSqlDatabase m_db;
};
