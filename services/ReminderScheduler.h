#pragma once

#include <QDate>
#include <QVector>

#include "Reminder.h"
#include "ReminderSettings.h"

class ReminderScheduler
{
public:
    QVector<int> dueReminderIds(const QVector<Reminder> &reminders,
                                const ReminderSettings &settings,
                                const QDate &date) const;
};
