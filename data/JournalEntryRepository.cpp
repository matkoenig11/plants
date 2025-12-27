#include "JournalEntryRepository.h"

#include <QSqlQuery>
#include <QVariant>

namespace {
QString dateToString(const QDate &date)
{
    return date.isValid() ? date.toString(Qt::ISODate) : QString();
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

JournalEntryRepository::JournalEntryRepository(QSqlDatabase db)
    : m_db(db)
{
}

int JournalEntryRepository::create(const JournalEntry &entry)
{
    if (!m_db.isOpen() || !entry.isValid()) {
        return 0;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO journal_entries (plant_id, entry_type, entry_date, notes) "
        "VALUES (:plant_id, :entry_type, :entry_date, :notes);"
    );
    query.bindValue(":plant_id", entry.plantId);
    query.bindValue(":entry_type", entry.entryType);
    query.bindValue(":entry_date", dateToString(entry.entryDate));
    query.bindValue(":notes", entry.notes);

    if (!query.exec()) {
        return 0;
    }

    return query.lastInsertId().toInt();
}

bool JournalEntryRepository::update(const JournalEntry &entry)
{
    if (!m_db.isOpen() || entry.id <= 0 || !entry.isValid()) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE journal_entries "
        "SET entry_type = :entry_type, "
        "    entry_date = :entry_date, "
        "    notes = :notes "
        "WHERE id = :id;"
    );
    query.bindValue(":entry_type", entry.entryType);
    query.bindValue(":entry_date", dateToString(entry.entryDate));
    query.bindValue(":notes", entry.notes);
    query.bindValue(":id", entry.id);

    return query.exec();
}

bool JournalEntryRepository::remove(int id)
{
    if (!m_db.isOpen() || id <= 0) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM journal_entries WHERE id = :id;");
    query.bindValue(":id", id);
    return query.exec();
}

JournalEntry JournalEntryRepository::findById(int id)
{
    JournalEntry entry;
    if (!m_db.isOpen() || id <= 0) {
        return entry;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "SELECT id, plant_id, entry_type, entry_date, notes, created_at "
        "FROM journal_entries WHERE id = :id;"
    );
    query.bindValue(":id", id);

    if (!query.exec()) {
        return entry;
    }

    if (query.next()) {
        entry = mapRow(query);
    }

    return entry;
}

QVector<JournalEntry> JournalEntryRepository::listForPlant(int plantId)
{
    QVector<JournalEntry> entries;
    if (!m_db.isOpen() || plantId <= 0) {
        return entries;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "SELECT id, plant_id, entry_type, entry_date, notes, created_at "
        "FROM journal_entries WHERE plant_id = :plant_id "
        "ORDER BY entry_date DESC, id DESC;"
    );
    query.bindValue(":plant_id", plantId);

    if (!query.exec()) {
        return entries;
    }

    while (query.next()) {
        entries.append(mapRow(query));
    }

    return entries;
}

JournalEntry JournalEntryRepository::mapRow(const QSqlQuery &query) const
{
    JournalEntry entry;
    entry.id = query.value(0).toInt();
    entry.plantId = query.value(1).toInt();
    entry.entryType = query.value(2).toString();
    entry.entryDate = stringToDate(query.value(3).toString());
    entry.notes = query.value(4).toString();
    entry.createdAt = stringToDateTime(query.value(5).toString());
    return entry;
}
