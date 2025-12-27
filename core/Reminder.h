#pragma once

#include <QDate>
#include <QDateTime>
#include <QString>

struct Reminder
{
    int id = 0;
    QString customName;
    QString taskType;
    QString scheduleType;
    int intervalDays = 0;
    QDate startDate;
    QDate nextDueDate;
    QString notes;
    QDateTime createdAt;
    QDateTime updatedAt;

    bool isValid() const
    {
        return !taskType.trimmed().isEmpty() && !scheduleType.trimmed().isEmpty() && nextDueDate.isValid();
    }
};
