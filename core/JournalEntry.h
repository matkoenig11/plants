#pragma once

#include <QDate>
#include <QDateTime>
#include <QString>

struct JournalEntry
{
    int id = 0;
    int plantId = 0;
    QString entryType;
    QDate entryDate;
    QString notes;
    QDateTime createdAt;

    bool isValid() const
    {
        return plantId > 0 && !entryType.trimmed().isEmpty() && entryDate.isValid();
    }
};
