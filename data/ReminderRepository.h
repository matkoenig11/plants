#pragma once

#include <QSqlDatabase>
#include <QVector>

#include "Reminder.h"

class QSqlQuery;

class ReminderRepository
{
public:
    explicit ReminderRepository(QSqlDatabase db);

    int create(const Reminder &reminder, const QVector<int> &plantIds);
    bool update(const Reminder &reminder, const QVector<int> &plantIds);
    bool remove(int id);
    Reminder findById(int id);
    QVector<Reminder> listAll();
    QVector<int> plantIdsForReminder(int reminderId);

private:
    Reminder mapRow(const QSqlQuery &query) const;
    bool replacePlantLinks(int reminderId, const QVector<int> &plantIds);

    QSqlDatabase m_db;
};
