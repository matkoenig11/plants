#include "ReminderRepository.h"

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

ReminderRepository::ReminderRepository(QSqlDatabase db)
    : m_db(db)
{
}

int ReminderRepository::create(const Reminder &reminder, const QVector<int> &plantIds)
{
    if (!m_db.isOpen() || !reminder.isValid() || plantIds.isEmpty()) {
        return 0;
    }

    if (!m_db.transaction()) {
        return 0;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO reminders (custom_name, task_type, schedule_type, interval_days, start_date, next_due_date, notes) "
        "VALUES (:custom_name, :task_type, :schedule_type, :interval_days, :start_date, :next_due_date, :notes);"
    );
    query.bindValue(":custom_name", reminder.customName);
    query.bindValue(":task_type", reminder.taskType);
    query.bindValue(":schedule_type", reminder.scheduleType);
    query.bindValue(":interval_days", reminder.intervalDays > 0 ? reminder.intervalDays : QVariant());
    query.bindValue(":start_date", dateToString(reminder.startDate));
    query.bindValue(":next_due_date", dateToString(reminder.nextDueDate));
    query.bindValue(":notes", reminder.notes);

    if (!query.exec()) {
        m_db.rollback();
        return 0;
    }

    const int id = query.lastInsertId().toInt();
    if (id <= 0 || !replacePlantLinks(id, plantIds)) {
        m_db.rollback();
        return 0;
    }

    if (!m_db.commit()) {
        m_db.rollback();
        return 0;
    }

    return id;
}

bool ReminderRepository::update(const Reminder &reminder, const QVector<int> &plantIds)
{
    if (!m_db.isOpen() || reminder.id <= 0 || !reminder.isValid() || plantIds.isEmpty()) {
        return false;
    }

    if (!m_db.transaction()) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE reminders "
        "SET custom_name = :custom_name, "
        "    task_type = :task_type, "
        "    schedule_type = :schedule_type, "
        "    interval_days = :interval_days, "
        "    start_date = :start_date, "
        "    next_due_date = :next_due_date, "
        "    notes = :notes, "
        "    updated_at = datetime('now') "
        "WHERE id = :id;"
    );
    query.bindValue(":custom_name", reminder.customName);
    query.bindValue(":task_type", reminder.taskType);
    query.bindValue(":schedule_type", reminder.scheduleType);
    query.bindValue(":interval_days", reminder.intervalDays > 0 ? reminder.intervalDays : QVariant());
    query.bindValue(":start_date", dateToString(reminder.startDate));
    query.bindValue(":next_due_date", dateToString(reminder.nextDueDate));
    query.bindValue(":notes", reminder.notes);
    query.bindValue(":id", reminder.id);

    if (!query.exec()) {
        m_db.rollback();
        return false;
    }

    if (!replacePlantLinks(reminder.id, plantIds)) {
        m_db.rollback();
        return false;
    }

    if (!m_db.commit()) {
        m_db.rollback();
        return false;
    }

    return true;
}

bool ReminderRepository::remove(int id)
{
    if (!m_db.isOpen() || id <= 0) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM reminders WHERE id = :id;");
    query.bindValue(":id", id);
    return query.exec();
}

Reminder ReminderRepository::findById(int id)
{
    Reminder reminder;
    if (!m_db.isOpen() || id <= 0) {
        return reminder;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "SELECT id, custom_name, task_type, schedule_type, interval_days, start_date, next_due_date, notes, created_at, updated_at "
        "FROM reminders WHERE id = :id;"
    );
    query.bindValue(":id", id);

    if (!query.exec()) {
        return reminder;
    }

    if (query.next()) {
        reminder = mapRow(query);
    }

    return reminder;
}

QVector<Reminder> ReminderRepository::listAll()
{
    QVector<Reminder> reminders;
    if (!m_db.isOpen()) {
        return reminders;
    }

    QSqlQuery query(m_db);
    if (!query.exec(
            "SELECT id, custom_name, task_type, schedule_type, interval_days, start_date, next_due_date, notes, created_at, updated_at "
            "FROM reminders ORDER BY next_due_date;")) {
        return reminders;
    }

    while (query.next()) {
        reminders.append(mapRow(query));
    }

    return reminders;
}

QVector<int> ReminderRepository::plantIdsForReminder(int reminderId)
{
    QVector<int> plantIds;
    if (!m_db.isOpen() || reminderId <= 0) {
        return plantIds;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT plant_id FROM reminder_plants WHERE reminder_id = :reminder_id;");
    query.bindValue(":reminder_id", reminderId);

    if (!query.exec()) {
        return plantIds;
    }

    while (query.next()) {
        plantIds.append(query.value(0).toInt());
    }

    return plantIds;
}

Reminder ReminderRepository::mapRow(const QSqlQuery &query) const
{
    Reminder reminder;
    reminder.id = query.value(0).toInt();
    reminder.customName = query.value(1).toString();
    reminder.taskType = query.value(2).toString();
    reminder.scheduleType = query.value(3).toString();
    reminder.intervalDays = query.value(4).toInt();
    reminder.startDate = stringToDate(query.value(5).toString());
    reminder.nextDueDate = stringToDate(query.value(6).toString());
    reminder.notes = query.value(7).toString();
    reminder.createdAt = stringToDateTime(query.value(8).toString());
    reminder.updatedAt = stringToDateTime(query.value(9).toString());
    return reminder;
}

bool ReminderRepository::replacePlantLinks(int reminderId, const QVector<int> &plantIds)
{
    QSqlQuery clearQuery(m_db);
    clearQuery.prepare("DELETE FROM reminder_plants WHERE reminder_id = :reminder_id;");
    clearQuery.bindValue(":reminder_id", reminderId);
    if (!clearQuery.exec()) {
        return false;
    }

    QSqlQuery insertQuery(m_db);
    insertQuery.prepare("INSERT INTO reminder_plants (reminder_id, plant_id) VALUES (:reminder_id, :plant_id);");
    for (int plantId : plantIds) {
        if (plantId <= 0) {
            continue;
        }
        insertQuery.bindValue(":reminder_id", reminderId);
        insertQuery.bindValue(":plant_id", plantId);
        if (!insertQuery.exec()) {
            return false;
        }
    }

    return true;
}
