#pragma once

#include <QString>

struct ReminderSettings
{
    bool dayBeforeEnabled = true;
    QString dayBeforeTime = "18:00";
    bool dayOfEnabled = true;
    QString dayOfTime = "08:00";
    bool overdueEnabled = true;
    int overdueCadenceDays = 2;
    QString overdueTime = "09:00";
    QString quietHoursStart;
    QString quietHoursEnd;
};
