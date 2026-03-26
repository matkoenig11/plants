#include "JournalEntryRepository.h"

#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
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

int plantIdForEntry(QSqlDatabase &db, int entryId)
{
    QSqlQuery query(db);
    query.prepare("SELECT plant_id FROM journal_entries WHERE id = :id;");
    query.bindValue(":id", entryId);
    if (!query.exec() || !query.next()) {
        return 0;
    }
    return query.value(0).toInt();
}

bool syncPlantCareDates(QSqlDatabase &db, int plantId)
{
    if (!db.isOpen() || plantId <= 0) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare(
        "UPDATE plants "
        "SET last_watered = ("
        "        SELECT MAX(entry_date) FROM journal_entries "
        "        WHERE plant_id = :plant_id AND lower(entry_type) = 'water'"
        "    ), "
        "    last_fertilized = ("
        "        SELECT MAX(entry_date) FROM journal_entries "
        "        WHERE plant_id = :plant_id AND lower(entry_type) = 'fertilize'"
        "    ), "
        "    updated_at = datetime('now') "
        "WHERE id = :plant_id;");
    query.bindValue(":plant_id", plantId);

    if (!query.exec()) {
        qWarning() << "Failed to sync plant care dates for plant" << plantId << ":"
                   << query.lastError().text();
        return false;
    }

    QSqlQuery verifyQuery(db);
    verifyQuery.prepare(
        "SELECT name, last_watered, last_fertilized "
        "FROM plants WHERE id = :plant_id;");
    verifyQuery.bindValue(":plant_id", plantId);
    if (verifyQuery.exec() && verifyQuery.next()) {
        qDebug() << "Synced plant care dates for plant" << plantId
                 << verifyQuery.value(0).toString()
                 << "lastWatered" << verifyQuery.value(1).toString()
                 << "lastFertilized" << verifyQuery.value(2).toString();
    }

    return true;
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

    const int id = query.lastInsertId().toInt();
    qDebug() << "Created journal entry" << id << "for plant" << entry.plantId
             << "type" << entry.entryType << "date" << entry.entryDate.toString(Qt::ISODate);
    syncPlantCareDates(m_db, entry.plantId);
    return id;
}

bool JournalEntryRepository::update(const JournalEntry &entry)
{
    if (!m_db.isOpen() || entry.id <= 0 || !entry.isValid()) {
        return false;
    }

    const int previousPlantId = plantIdForEntry(m_db, entry.id);
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

    if (!query.exec()) {
        return false;
    }

    qDebug() << "Updated journal entry" << entry.id << "plant" << previousPlantId
             << "->" << entry.plantId << "type" << entry.entryType
             << "date" << entry.entryDate.toString(Qt::ISODate);
    if (previousPlantId > 0 && previousPlantId != entry.plantId) {
        syncPlantCareDates(m_db, previousPlantId);
    }
    syncPlantCareDates(m_db, entry.plantId);
    return true;
}

bool JournalEntryRepository::remove(int id)
{
    if (!m_db.isOpen() || id <= 0) {
        return false;
    }

    const int plantId = plantIdForEntry(m_db, id);
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM journal_entries WHERE id = :id;");
    query.bindValue(":id", id);
    if (!query.exec()) {
        return false;
    }

    qDebug() << "Removed journal entry" << id << "for plant" << plantId;
    if (plantId > 0) {
        syncPlantCareDates(m_db, plantId);
    }
    return true;
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
